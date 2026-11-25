// Plik: Source/grarts2d/Ludzie/Wol.cpp
#include "Ludzie/Wol.h"
#include "Osada/Osada.h" // Ludzik.h (UstawOsade) potrzebuje pelnego AOsada
#include "Ludzie/Ludzik.h"
#include "Budowy/Budowa.h"
#include "Osada/Drogi.h"
#include "Swiat/SwiatWysokosci.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	UStaticMesh* ModelWolu(const TCHAR* Nazwa)
	{
		const FString Sciezka = FString::Printf(TEXT("/Game/Swiat/Ludzie/%s.%s"), Nazwa, Nazwa);
		UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Sciezka);
		UE_CLOG(!M, LogTemp, Warning, TEXT("Wol: brak modelu %s (uruchom Tools/Zbuduj_Ludzi.bat)"), *Sciezka);
		return M;
	}
	constexpr float Skala = 1.4f;         // jak osadnicy
	constexpr float PredkoscPusty = 115.f; // cm/s (przed skala czasu osadnikow)
	constexpr float PredkoscZKlodami = 80.f;
	constexpr float PredkoscKonia = 170.f;   // kon z pustym wozem
	constexpr float PredkoscKoniaZWozem = 135.f;
}

AWol::AWol()
{
	PrimaryActorTick.bCanEverTick = true;
	Korzen = CreateDefaultSubobject<USceneComponent>(TEXT("Korzen"));
	RootComponent = Korzen;
	Korzen->SetMobility(EComponentMobility::Movable);
	Korzen->SetRelativeScale3D(FVector(Skala));
	SetCanBeDamaged(false);
	// Stawy w cm modelu (Tools/ludzie.py: tulow 1,05 m nad ziemia, nogi od stawow 0,75 m nad ziemia).
	Tulow = Czesc(TEXT("Tulow"), Korzen, FVector(0, 0, 105));
	Klody = Czesc(TEXT("Klody"), Korzen, FVector::ZeroVector);
	Woz = Czesc(TEXT("Woz"), Korzen, FVector::ZeroVector);
	const FVector Stawy[4] = { FVector(72, -24, 75), FVector(72, 24, 75), FVector(-68, -24, 75), FVector(-68, 24, 75) };
	const TCHAR* Nazwy[4] = { TEXT("NogaLP"), TEXT("NogaPP"), TEXT("NogaLT"), TEXT("NogaPT") };
	for (int32 k = 0; k < 4; ++k)
	{
		Nogi.Add(Czesc(Nazwy[k], Korzen, Stawy[k]));
	}
}

UStaticMeshComponent* AWol::Czesc(const TCHAR* Nazwa, USceneComponent* Rodzic, FVector Staw)
{
	UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Nazwa);
	C->SetupAttachment(Rodzic);
	C->SetRelativeLocation(Staw);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetMobility(EComponentMobility::Movable);
	C->SetGenerateOverlapEvents(false);
	return C;
}

void AWol::Zaladuj()
{
	if (bZaladowany)
	{
		return;
	}
	bZaladowany = true;
	Tulow->SetStaticMesh(ModelWolu(bKon ? TEXT("SM_Ludz_KonTulow") : TEXT("SM_Ludz_WolTulow")));
	Klody->SetStaticMesh(bKon ? nullptr : ModelWolu(TEXT("SM_Ludz_WolKlody")));
	Klody->SetVisibility(false);
	Woz->SetStaticMesh(bKon ? ModelWolu(TEXT("SM_Ludz_Woz")) : nullptr);
	// Kon (Tools/ludzie.py): tulow 1,25 m nad ziemia, stawy nog 1,05 m.
	const FVector Stawy[4] = { FVector(55, -17, 105), FVector(55, 17, 105), FVector(-58, -17, 105), FVector(-58, 17, 105) };
	for (int32 k = 0; k < 4; ++k)
	{
		Nogi[k]->SetStaticMesh(ModelWolu(bKon ? TEXT("SM_Ludz_KonNoga") : TEXT("SM_Ludz_WolNoga")));
		if (bKon)
		{
			Nogi[k]->SetRelativeLocation(Stawy[k]);
		}
	}
	Kierunek = GetActorRotation().Yaw;
	Faza = FMath::FRand() * 2.f * PI;
}

void AWol::UstawDom(FVector2D Gdzie, float Promien)
{
	Zaladuj();
	Dom = Gdzie;
	PromienDomu = Promien;
}

void AWol::IdzDo(FVector2D Gdzie)
{
	Trasa.Reset();
	TrasaI = 0;
	if (const ADrogi* D = ADrogi::Obecne())
	{
		D->Trasa(FVector2D(GetActorLocation()), Gdzie, Trasa);
	}
	if (Trasa.Num() == 0)
	{
		Trasa.Add(Gdzie);
	}
	ALudzik::OminBudynkiTrasa(GetWorld(), FVector2D(GetActorLocation()), Trasa, 150.f); // wol z klodami jest dlugi
	bIdzie = true;
}

void AWol::Ciagnij(ABudowa* Z, ABudowa* CelB, int32 Ile, ESurowiec S)
{
	Zaladuj();
	Towar = S;
	Zrodlo = Z;
	Cel = CelB;
	Ladunek = Ile;
	Z->Zarezerwowane[Towar] += Ile;
	CelB->WDrodze[Towar] += Ile;
	Krok = EKrok::DoZrodla;
	IdzDo(Z->PrzyStosie(Towar));
}

FVector2D AWol::MiejsceZrzutu() const
{
	const ABudowa* C = Cel.Get();
	return C->Gotowy() ? C->PrzyStosie(Towar) : C->ZUkladu(C->Rodzaj().Sterta);
}

void AWol::Porzuc()
{
	if (ABudowa* Z = Zrodlo.Get())
	{
		if (bMaKlody)
		{
			Z->ZmienZapas(Towar, Ladunek); // klody wracaja na stos, z ktorego je wzial
		}
		else
		{
			Z->Zarezerwowane[Towar] = FMath::Max(0, Z->Zarezerwowane[Towar] - Ladunek);
		}
	}
	if (ABudowa* C = Cel.Get())
	{
		C->WDrodze[Towar] = FMath::Max(0, C->WDrodze[Towar] - Ladunek);
	}
	Zrodlo.Reset();
	Cel.Reset();
	Ladunek = 0;
	bMaKlody = false;
	Klody->SetVisibility(false);
	Krok = EKrok::Pasie;
	bIdzie = false;
	Czekaj = 1.f;
}

void AWol::Dotarl()
{
	bIdzie = false;
	switch (Krok)
	{
	case EKrok::Pasie: Czekaj = FMath::FRandRange(6.f, 14.f); break; // pasie sie
	case EKrok::DoZrodla: Krok = EKrok::Laduje; Czekaj = 3.f; break;
	case EKrok::Ciagnie: Krok = EKrok::Zrzuca; Czekaj = 2.f; break;
	default: break;
	}
}

void AWol::Tick(float DtSwiata)
{
	Super::Tick(DtSwiata);
	const float Dt = DtSwiata * ALudzik::TempoOsadnikow;
	if ((Krok == EKrok::DoZrodla || Krok == EKrok::Laduje) && (!Zrodlo.IsValid() || !Cel.IsValid()))
	{
		Porzuc();
	}
	else if ((Krok == EKrok::Ciagnie || Krok == EKrok::Zrzuca) && !Cel.IsValid())
	{
		Porzuc();
	}

	FVector Poz = GetActorLocation();
	if (bIdzie)
	{
		const FVector2D Teraz(Poz);
		const FVector2D Do = Trasa[TrasaI];
		const FVector2D W = Do - Teraz;
		const float L = W.Size();
		const float Odc = Predkosc() * Dt;
		FVector2D Nowa;
		if (L <= Odc)
		{
			Nowa = Do;
			if (++TrasaI >= Trasa.Num())
			{
				Dotarl();
			}
		}
		else
		{
			Nowa = Teraz + W / L * Odc;
			Kierunek = FMath::FixedTurn(Kierunek, FMath::RadiansToDegrees(FMath::Atan2(W.Y, W.X)), 120.f * Dt); // wol skreca powoli
		}
		Poz = FVector(Nowa, FSwiat::Wysokosc(Nowa.X, Nowa.Y));
		SetActorLocationAndRotation(Poz, FRotator(0.f, Kierunek, 0.f));
		Faza += Dt * 2.f * PI * Predkosc() / (bKon ? 210.f : 150.f);
	}
	else
	{
		Czekaj -= Dt;
		if (Czekaj <= 0.f)
		{
			switch (Krok)
			{
			case EKrok::Pasie:
			{
				// Przejscie kawalek dalej po wybiegu (albo powrot, gdy odszedl daleko).
				const float Kat = FMath::FRand() * 2.f * PI;
				IdzDo(Dom + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * FMath::FRandRange(0.f, PromienDomu));
				break;
			}
			case EKrok::Laduje:
			{
				ABudowa* Z = Zrodlo.Get();
				Z->Zarezerwowane[Towar] = FMath::Max(0, Z->Zarezerwowane[Towar] - Ladunek);
				const int32 Wziete = FMath::Min(Z->Zapas[Towar], Ladunek);
				Cel->WDrodze[Towar] = FMath::Max(0, Cel->WDrodze[Towar] - (Ladunek - Wziete));
				Ladunek = Wziete;
				if (Wziete <= 0)
				{
					Porzuc(); // ktos zabral klody
					break;
				}
				Z->ZmienZapas(Towar, -Wziete);
				bMaKlody = true;
				if (bKon)
				{
					Klody->SetStaticMesh(ModelWolu(Towar == ESurowiec::Kamien ? TEXT("SM_Ludz_WozKamien") : TEXT("SM_Ludz_WozDeski")));
				}
				Klody->SetVisibility(true);
				Krok = EKrok::Ciagnie;
				IdzDo(MiejsceZrzutu());
				break;
			}
			case EKrok::Zrzuca:
			{
				ABudowa* C = Cel.Get();
				if (!C->Gotowy())
				{
					for (int32 k = 0; k < Ladunek; ++k)
					{
						C->Dostarcz(Towar);
					}
				}
				else
				{
					// shortcut: gdy budowa skonczyla sie w czasie kursu, klody zostaja w jej zapasie (nie ida do magazynu).
					C->WDrodze[Towar] = FMath::Max(0, C->WDrodze[Towar] - Ladunek);
					C->ZmienZapas(Towar, Ladunek);
				}
				bMaKlody = false;
				Zrodlo.Reset();
				Cel.Reset();
				Ladunek = 0;
				Klody->SetVisibility(false);
				Krok = EKrok::Pasie;
				Czekaj = 1.5f;
				break;
			}
			default:
				break;
			}
		}
	}

	// Chod: nogi na krzyz (lewa przednia z prawa tylna), tulow lekko kolysze sie przy kazdym kroku.
	WagaChodu = FMath::FInterpTo(WagaChodu, bIdzie ? 1.f : 0.f, Dt, 4.f);
	const float Wychyl = 24.f * WagaChodu;
	const float Przes[4] = { 0.f, PI, PI, 0.f };
	for (int32 k = 0; k < 4; ++k)
	{
		Nogi[k]->SetRelativeRotation(FRotator(FMath::Sin(Faza + Przes[k]) * Wychyl, 0.f, 0.f));
	}
	Tulow->SetRelativeLocationAndRotation(FVector(0, 0, (bKon ? 125.f : 105.f) + 2.f * WagaChodu * FMath::Sin(2.f * Faza)),
		FRotator(0.f, 0.f, 1.5f * WagaChodu * FMath::Sin(Faza)));
}

float AWol::Predkosc() const
{
	return bKon ? (bMaKlody ? PredkoscKoniaZWozem : PredkoscKonia) : (bMaKlody ? PredkoscZKlodami : PredkoscPusty);
}
