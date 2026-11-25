// Plik: Source/grarts2d/Efekty/Efekty.cpp
#include "Efekty/Efekty.h"
#include "Swiat/SwiatWysokosci.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"

namespace
{
	TWeakObjectPtr<AEfekty> Jedyny;
	constexpr int32 MaxCzastek[] = { 900, 520, 600, 420, 300 }; // plomien, dym, iskra, kurz, gruz
	constexpr int32 MaxSwiatel = 6;

	float Los(float A, float B) { return FMath::FRandRange(A, B); }
}

AEfekty::AEfekty()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	static const TCHAR* Nazwy[] = { TEXT("Plomienie"), TEXT("Dym"), TEXT("Iskry"), TEXT("Kurz"), TEXT("Gruz") };
	for (int32 r = 0; r < Rodzajow; ++r)
	{
		UInstancedStaticMeshComponent* C = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Nazwy[r]);
		C->SetupAttachment(RootComponent);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(r == Gruz);
		C->NumCustomDataFloats = 4; // R, G, B, A (jasnosc / krycie)
		C->SetTranslucentSortPriority(r == Plomien || r == Iskra ? 3 : r == Dym ? 2 : 1);
		Ism.Add(C);
	}
}

AEfekty* AEfekty::W(UWorld* Swiat)
{
	if (!Swiat)
	{
		return nullptr;
	}
	if (Jedyny.IsValid() && Jedyny->GetWorld() == Swiat)
	{
		return Jedyny.Get();
	}
	for (TActorIterator<AEfekty> It(Swiat); It; ++It)
	{
		Jedyny = *It;
		return *It;
	}
	AEfekty* E = Swiat->SpawnActor<AEfekty>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!E)
	{
		return nullptr;
	}
	auto Siatka = [](const TCHAR* N) { return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Swiat/Efekty/%s.%s"), N, N)); };
	UStaticMesh* Plom = Siatka(TEXT("SM_Efekt_Plomien"));
	UStaticMesh* Kleks = Siatka(TEXT("SM_Efekt_Kleks"));
	UStaticMesh* Gr = Siatka(TEXT("SM_Efekt_Gruz"));
	UE_CLOG(!Plom || !Kleks || !Gr, LogTemp, Warning, TEXT("Efekty: brak modeli /Game/Swiat/Efekty (uruchom Tools/Zbuduj_Efekty.bat)"));
	E->Ism[Plomien]->SetStaticMesh(Plom);
	E->Ism[Dym]->SetStaticMesh(Kleks);
	E->Ism[Iskra]->SetStaticMesh(Kleks);
	E->Ism[KurzR]->SetStaticMesh(Kleks);
	E->Ism[Gruz]->SetStaticMesh(Gr);
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Swiat/Efekty/M_Efekt_Iskra.M_Efekt_Iskra")))
	{
		E->Ism[Iskra]->SetMaterial(0, M); // ten sam kwadrat co dym, material iskry
	}
	for (int32 k = 0; k < MaxSwiatel; ++k)
	{
		UPointLightComponent* L = NewObject<UPointLightComponent>(E);
		L->SetupAttachment(E->GetRootComponent());
		L->SetMobility(EComponentMobility::Movable);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetLightColor(FLinearColor(1.f, 0.55f, 0.24f));
		L->SetCastShadows(false); // tanio; cien i tak daje slonce
		L->SetVisibility(false);
		L->RegisterComponent();
		E->Swiatla.Add(L);
	}
	Jedyny = E;
	return E;
}

FVector AEfekty::Wiatr()
{
	return FVector(55.f, 35.f, 0.f); // cm/s: dym i iskry znosi lekko w jedna strone (jak na zrzutach: z lewej gory)
}

int32 AEfekty::Pozar(FVector Srodek, FVector2D Polowa, float Obrot, float Wys)
{
	FPozar F;
	F.Id = NastId++;
	F.S = Srodek;
	F.Pol = Polowa;
	F.Obrot = Obrot;
	F.Wys = FMath::Max(150.f, Wys);
	F.Faza = Los(0.f, 10.f);
	Pozary.Add(F);
	SetActorTickEnabled(true);
	return F.Id;
}

void AEfekty::UstawPozar(int32 Id, float Sila, float Wys)
{
	for (FPozar& F : Pozary)
	{
		if (F.Id == Id)
		{
			F.Cel = FMath::Clamp(Sila, 0.f, 1.f);
			F.bKoniec = false;
			if (Wys >= 0.f)
			{
				F.Wys = FMath::Max(80.f, Wys);
			}
		}
	}
}

void AEfekty::ZakonczPozar(int32 Id)
{
	for (FPozar& F : Pozary)
	{
		if (F.Id == Id)
		{
			F.bKoniec = true;
		}
	}
}

FVector AEfekty::PunktPozaru(const FPozar& F, float ZOd, float ZDo) const
{
	const FVector2D L(Los(-0.85f, 0.85f) * F.Pol.X, Los(-0.85f, 0.85f) * F.Pol.Y);
	const FVector2D Wsp = L.GetRotated(F.Obrot);
	return F.S + FVector(Wsp, Los(ZOd, ZDo));
}

FVector AEfekty::PunktPowierzchni(const FPozar& F, bool& bSciana) const
{
	// 24.11.2026 (prosba: "ogien powinien byc bardziej na budynku"): dach dwu- / czterospadowy - kalenica na Wys, okap ok. 0,55 Wys;
	// sciany - obwod obrysu od 0,1 do 0,55 Wys (ogien wychodzi z okien i spod okapu).
	float U = Los(-1.f, 1.f), V = Los(-1.f, 1.f), Z;
	bSciana = FMath::FRand() < 0.35f;
	if (bSciana)
	{
		(FMath::RandBool() ? U : V) = FMath::RandBool() ? 1.f : -1.f;
		Z = F.Wys * Los(0.1f, 0.55f);
	}
	else
	{
		Z = F.Wys * (0.55f + 0.45f * (1.f - FMath::Max(FMath::Abs(U), FMath::Abs(V))));
	}
	const FVector2D Wsp = FVector2D(U * F.Pol.X * 0.92f, V * F.Pol.Y * 0.92f).GetRotated(F.Obrot);
	return F.S + FVector(Wsp, Z);
}
void AEfekty::Dodaj(ERodzaj R, const FCzastka& C)
{
	if (Cz[R].Num() < MaxCzastek[R])
	{
		Cz[R].Add(C);
	}
}

int32 AEfekty::Czastek() const
{
	int32 N = 0;
	for (int32 r = 0; r < Rodzajow; ++r) { N += Cz[r].Num(); }
	return N;
}

void AEfekty::Zawal(FVector Srodek, FVector2D Polowa, float Obrot, float Wys, bool bKamien)
{
	const float Pole = 4.f * Polowa.X * Polowa.Y / 1e4f; // m2
	auto WObrysie = [&](float Skala)
	{
		return FVector2D(Los(-Skala, Skala) * Polowa.X, Los(-Skala, Skala) * Polowa.Y).GetRotated(Obrot);
	};
	// Kurz: kleby przy ziemi rozchodza sie na boki i rosna.
	const int32 NK = FMath::Clamp(FMath::RoundToInt32(Pole * 0.35f), 18, 90);
	for (int32 k = 0; k < NK; ++k)
	{
		FCzastka C;
		const FVector2D O = WObrysie(0.9f);
		C.P = Srodek + FVector(O, Los(40.f, 250.f));
		const FVector2D Na = O.IsNearlyZero() ? FVector2D(1.f, 0.f).GetRotated(Los(0.f, 360.f)) : O.GetSafeNormal();
		C.V = FVector(Na * Los(250.f, 700.f), Los(40.f, 220.f));
		C.Zycie = Los(3.5f, 6.5f);
		C.Rozmiar = Los(280.f, 420.f);
		C.Obrot = Los(0.f, 360.f);
		C.Spin = Los(-25.f, 25.f);
		const float J = Los(0.85f, 1.1f);
		C.K = bKamien ? FLinearColor(0.78f * J, 0.74f * J, 0.68f * J, 0.75f) : FLinearColor(0.42f * J, 0.37f * J, 0.31f * J, 0.75f);
		Dodaj(KurzR, C);
	}
	// Gruz: kamienie (kostki) i zweglone belki (wydluzone) wylatuja z gory budynku.
	const int32 NG = FMath::Clamp(FMath::RoundToInt32(Pole * 0.18f), 12, 70);
	for (int32 k = 0; k < NG; ++k)
	{
		FCzastka C;
		const FVector2D O = WObrysie(0.8f);
		C.P = Srodek + FVector(O, Wys * Los(0.25f, 0.85f));
		const FVector2D Na = O.IsNearlyZero() ? FVector2D(1.f, 0.f).GetRotated(Los(0.f, 360.f)) : O.GetSafeNormal();
		C.V = FVector(Na * Los(150.f, 650.f), Los(150.f, 650.f));
		C.Zycie = Los(9.f, 15.f);
		C.Q = FQuat(FVector(Los(-1.f, 1.f), Los(-1.f, 1.f), Los(-1.f, 1.f)).GetSafeNormal(), Los(0.f, 6.28f));
		C.Spin = Los(-6.f, 6.f);
		const bool bBelka = !bKamien || FMath::FRand() < 0.3f;
		if (bBelka)
		{
			C.Skala = FVector(Los(1.1f, 2.6f), Los(0.15f, 0.24f), Los(0.15f, 0.24f));
			const float J = Los(0.75f, 1.3f);
			C.K = FLinearColor(0.10f * J, 0.075f * J, 0.06f * J, 1.f);
		}
		else
		{
			const float R = Los(0.18f, 0.5f);
			C.Skala = FVector(R * Los(0.8f, 1.3f), R, R * Los(0.6f, 1.f));
			const float J = Los(0.55f, 0.95f);
			C.K = FLinearColor(0.62f * J, 0.58f * J, 0.52f * J, 1.f);
		}
		Dodaj(Gruz, C);
	}
	// Snop iskier z pozaru.
	const int32 NI = FMath::Clamp(FMath::RoundToInt32(Pole * 0.5f), 30, 110);
	for (int32 k = 0; k < NI; ++k)
	{
		FCzastka C;
		C.P = Srodek + FVector(WObrysie(0.7f), Wys * Los(0.2f, 0.7f));
		C.V = FVector(Los(-260.f, 260.f), Los(-260.f, 260.f), Los(450.f, 1100.f));
		C.Zycie = Los(1.2f, 3.0f);
		C.Rozmiar = Los(10.f, 20.f);
		C.K = FLinearColor(1.f, Los(0.45f, 0.7f), 0.18f, Los(9.f, 16.f));
		Dodaj(Iskra, C);
	}
	SetActorTickEnabled(true);
}

void AEfekty::Kurz(FVector Gdzie, float Promien, int32 Ile)
{
	for (int32 k = 0; k < Ile; ++k)
	{
		FCzastka C;
		const FVector2D Na = FVector2D(1.f, 0.f).GetRotated(Los(0.f, 360.f));
		C.P = Gdzie + FVector(Na * Los(0.f, Promien * 0.5f), Los(20.f, 120.f));
		C.V = FVector(Na * Los(120.f, 380.f), Los(30.f, 140.f));
		C.Zycie = Los(1.8f, 3.2f);
		C.Rozmiar = Los(140.f, 240.f);
		C.Obrot = Los(0.f, 360.f);
		C.Spin = Los(-30.f, 30.f);
		C.K = FLinearColor(0.62f, 0.57f, 0.5f, 0.6f);
		Dodaj(KurzR, C);
	}
	SetActorTickEnabled(true);
}

void AEfekty::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float Dt = FMath::Min(DeltaSeconds, 0.1f);
	Czas += Dt;
	FVector Kamera(0.f, 0.f, 1e5f);
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController(); PC && PC->PlayerCameraManager)
	{
		Kamera = PC->PlayerCameraManager->GetCameraLocation();
	}
	Symuluj(Dt, Kamera);
	Rysuj(Kamera);
	Oswietl(Kamera);
	if (Pozary.Num() == 0 && Czastek() == 0)
	{
		SetActorTickEnabled(false); // nic sie nie pali: bez kosztu
	}
}

void AEfekty::Symuluj(float Dt, FVector Kamera)
{
	const FVector W = Wiatr();
	// Pozary wypuszczaja czastki wedlug sily; sila plynnie dochodzi do docelowej (rozpalanie, wygasanie).
	for (int32 i = Pozary.Num() - 1; i >= 0; --i)
	{
		FPozar& F = Pozary[i];
		F.Sila = FMath::FInterpTo(F.Sila, F.bKoniec ? 0.f : F.Cel, Dt, F.bKoniec ? 0.35f : 0.6f);
		if (F.bKoniec && F.Sila < 0.02f)
		{
			Pozary.RemoveAt(i);
			continue;
		}
		const float Pole = FMath::Max(4.f, 4.f * F.Pol.X * F.Pol.Y / 1e4f);
		const float Plom = FMath::Max(0.f, F.Sila - 0.12f) / 0.88f;  // slaby ogien = same zgliszcza, bez plomieni
		F.Akum[0] += Dt * FMath::Clamp(Pole * 0.8f, 10.f, 70.f) * Plom; // wiecej, ale mniejszych (na powierzchni)
		F.Akum[1] += Dt * (1.2f + 3.2f * F.Sila) * FMath::Clamp(Pole / 70.f, 0.6f, 2.6f);
		F.Akum[2] += Dt * 12.f * Plom * FMath::Clamp(Pole / 60.f, 0.5f, 2.f);
		for (; F.Akum[0] >= 1.f; F.Akum[0] -= 1.f)
		{
			FCzastka C;
			// Wiekszosc plomieni liże dach i sciany (mniejsze, nisko przy powierzchni), reszta to duze jezyki nad dachem.
			bool bSciana = false;
			const bool bNaBudynku = FMath::FRand() < 0.65f;
			C.P = bNaBudynku ? PunktPowierzchni(F, bSciana) : PunktPozaru(F, F.Wys * 0.6f, F.Wys * 0.95f);
			C.V = FVector(W.X * 0.3f + Los(-25.f, 25.f), W.Y * 0.3f + Los(-25.f, 25.f), Los(70.f, 170.f) * (bNaBudynku ? 0.6f : 1.f));
			C.Zycie = Los(0.45f, 0.95f);
			C.Rozmiar = Los(130.f, 270.f) * (0.55f + 0.6f * Plom) * (bNaBudynku ? (bSciana ? 0.45f : 0.6f) : 1.f);
			C.Obrot = Los(-12.f, 12.f);
			C.Spin = Los(-20.f, 20.f);
			C.K = FLinearColor(1.f, Los(0.55f, 0.7f), Los(0.22f, 0.32f), Los(4.5f, 7.5f));
			Dodaj(Plomien, C);
		}
		for (; F.Akum[1] >= 1.f; F.Akum[1] -= 1.f)
		{
			FCzastka C;
			C.P = PunktPozaru(F, F.Wys * 0.75f, F.Wys * 1.1f);
			C.V = FVector(W.X * 0.4f + Los(-30.f, 30.f), W.Y * 0.4f + Los(-30.f, 30.f), Los(110.f, 200.f) * (0.6f + 0.5f * F.Sila));
			C.Zycie = Los(7.f, 11.f) * (0.6f + 0.5f * F.Sila);
			C.Rozmiar = Los(220.f, 320.f);
			C.Obrot = Los(0.f, 360.f);
			C.Spin = Los(-14.f, 14.f);
			// Silny ogien: gesty, prawie czarny dym; tlace sie zgliszcza: rzadszy, szary.
			const float S = FMath::Clamp(F.Sila, 0.f, 1.f);
			const float Jas = FMath::Lerp(0.62f, 0.16f, S) * Los(0.85f, 1.15f);
			C.K = FLinearColor(Jas, Jas * 0.97f, Jas * 0.93f, 0.28f + 0.5f * S);
			Dodaj(Dym, C);
		}
		for (; F.Akum[2] >= 1.f; F.Akum[2] -= 1.f)
		{
			FCzastka C;
			// Polowa iskier to zar: zarzace sie plamy na dachu i scianach (prawie bez ruchu), reszta leci w gore.
			bool bSciana = false;
			const bool bZar = FMath::RandBool();
			C.P = bZar ? PunktPowierzchni(F, bSciana) : PunktPozaru(F, F.Wys * 0.55f, F.Wys);
			C.V = bZar ? FVector(Los(-6.f, 6.f), Los(-6.f, 6.f), Los(2.f, 14.f)) : FVector(Los(-90.f, 90.f), Los(-90.f, 90.f), Los(220.f, 520.f));
			C.Zycie = bZar ? Los(1.5f, 3.5f) : Los(1.f, 2.6f);
			C.Rozmiar = bZar ? Los(26.f, 52.f) : Los(8.f, 16.f);
			C.bLezy = bZar; // zar zostaje na miejscu
			C.K = FLinearColor(1.f, Los(0.45f, 0.7f), 0.16f, Los(8.f, 14.f));
			Dodaj(Iskra, C);
		}
	}
	// Ruch czastek.
	for (int32 r = 0; r < Rodzajow; ++r)
	{
		TArray<FCzastka>& L = Cz[r];
		for (int32 k = L.Num() - 1; k >= 0; --k)
		{
			FCzastka& C = L[k];
			C.Wiek += Dt;
			if (C.Wiek >= C.Zycie)
			{
				L.RemoveAtSwap(k, EAllowShrinking::No);
				continue;
			}
			C.Obrot += C.Spin * Dt;
			switch (r)
			{
			case Plomien:
				C.P += C.V * Dt;
				break;
			case Dym:
				C.V += (W - FVector(C.V.X, C.V.Y, 0.f)) * (0.35f * Dt); // znosi z wiatrem
				C.V.Z = FMath::Max(25.f, C.V.Z - 8.f * Dt);            // wznosi sie coraz wolniej
				C.P += C.V * Dt;
				break;
			case Iskra:
				if (C.bLezy)
				{
					break; // zar na dachu / scianie
				}
				C.V += FVector(W.X, W.Y, 0.f) * Dt + FVector(Los(-300.f, 300.f), Los(-300.f, 300.f), -60.f) * Dt; // wiruja
				C.P += C.V * Dt;
				break;
			case KurzR:
				C.V *= FMath::Exp(-1.7f * Dt);
				C.V += W * (0.4f * Dt);
				C.P += C.V * Dt;
				break;
			case Gruz:
				if (!C.bLezy)
				{
					C.V.Z -= 980.f * Dt;
					C.P += C.V * Dt;
					C.Q = FQuat(FVector(0.3f, 1.f, 0.2f).GetSafeNormal(), C.Spin * Dt) * C.Q;
					const float Ziemia = FSwiat::Wysokosc(C.P.X, C.P.Y) + 8.f;
					if (C.P.Z <= Ziemia && C.V.Z < 0.f)
					{
						C.P.Z = Ziemia;
						if (C.V.Z < -220.f)
						{
							C.V = FVector(C.V.X * 0.35f, C.V.Y * 0.35f, -C.V.Z * 0.28f); // odbicie
							C.Spin *= 0.5f;
						}
						else
						{
							C.bLezy = true;
						}
					}
				}
				else if (C.Zycie - C.Wiek < 1.5f)
				{
					C.P.Z -= 30.f * Dt; // na koniec zapada sie w ziemie (zamiast znikac nagle)
				}
				break;
			default:
				break;
			}
		}
	}
}

void AEfekty::Rysuj(FVector Kamera)
{
	for (int32 r = 0; r < Rodzajow; ++r)
	{
		UInstancedStaticMeshComponent* C = Ism[r];
		TArray<FCzastka>& L = Cz[r];
		if (!C || (L.Num() == 0 && C->GetInstanceCount() == 0))
		{
			continue;
		}
		if (r == Dym || r == KurzR)
		{
			// Przezroczyste: od najdalszego do najblizszego kamery.
			L.Sort([&Kamera](const FCzastka& A, const FCzastka& B) { return FVector::DistSquared(A.P, Kamera) > FVector::DistSquared(B.P, Kamera); });
		}
		TArray<FTransform> T;
		TArray<float> D;
		T.Reserve(L.Num());
		D.Reserve(L.Num() * 4);
		for (const FCzastka& X : L)
		{
			const float t = X.Wiek / FMath::Max(0.01f, X.Zycie);
			float A = X.K.A;
			FVector S;
			FQuat Q;
			if (r == Gruz)
			{
				Q = X.Q;
				S = X.Skala;
			}
			else
			{
				// Kwadrat frontem do kamery, gora jak najblizej pionu; obrot w plaszczyznie kwadratu.
				Q = FRotationMatrix::MakeFromXZ(Kamera - X.P, FVector::UpVector).ToQuat() * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(X.Obrot));
				float Roz = X.Rozmiar;
				switch (r)
				{
				case Plomien:
					Roz *= FMath::Sin(FMath::Min(1.f, t * 1.6f + 0.1f) * PI * 0.5f) * (1.f - 0.45f * t); // wyrasta i maleje
					A *= FMath::Min(1.f, (1.f - t) * 2.5f);
					S = FVector(1.f, Roz * 0.62f / 100.f, Roz / 100.f);
					break;
				case Dym:
					Roz *= 1.f + 3.4f * FMath::Sqrt(t);
					A *= FMath::Min(1.f, t * 6.f) * (1.f - t) * (1.f - t);
					S = FVector(Roz / 100.f);
					break;
				case KurzR:
					Roz *= 1.f + 2.6f * FMath::Sqrt(t);
					A *= FMath::Min(1.f, t * 8.f) * (1.f - t);
					S = FVector(Roz / 100.f);
					break;
				default: // iskra: gasnie i czerwienieje
					A *= (1.f - t);
					S = FVector(Roz / 100.f);
					break;
				}
			}
			T.Add(FTransform(Q, X.P, S));
			const float G = r == Iskra ? X.K.G * (1.f - 0.5f * t) : X.K.G;
			D.Append({ X.K.R, G, X.K.B, A });
		}
		C->ClearInstances();
		if (T.Num() > 0)
		{
			C->AddInstances(T, false, true);
			for (int32 i = 0; i < T.Num(); ++i)
			{
				C->SetCustomData(i, MakeArrayView(&D[i * 4], 4), false);
			}
			C->MarkRenderStateDirty();
		}
	}
}

void AEfekty::Oswietl(FVector Kamera)
{
	// Najwazniejsze pozary (silne i blisko kamery) dostaja cieple swiatlo; puls z sinusow = plynny, nie mruga.
	TArray<int32> Kolejnosc;
	for (int32 i = 0; i < Pozary.Num(); ++i)
	{
		if (Pozary[i].Sila > 0.08f) { Kolejnosc.Add(i); }
	}
	Kolejnosc.Sort([this, &Kamera](int32 A, int32 B)
	{
		const FPozar& X = Pozary[A];
		const FPozar& Y = Pozary[B];
		return X.Sila / (1.f + FVector::Dist(X.S, Kamera) * 1e-4f) > Y.Sila / (1.f + FVector::Dist(Y.S, Kamera) * 1e-4f);
	});
	for (int32 k = 0; k < Swiatla.Num(); ++k)
	{
		UPointLightComponent* L = Swiatla[k];
		if (!L)
		{
			continue;
		}
		if (!Kolejnosc.IsValidIndex(k))
		{
			L->SetVisibility(false);
			continue;
		}
		const FPozar& F = Pozary[Kolejnosc[k]];
		const float Puls = 0.86f + 0.09f * FMath::Sin(Czas * 7.3f + F.Faza) + 0.05f * FMath::Sin(Czas * 12.7f + 2.f * F.Faza);
		L->SetWorldLocation(F.S + FVector(0.f, 0.f, F.Wys * 0.75f));
		L->SetIntensity(140.f * F.Sila * Puls * FMath::Clamp(F.Pol.GetMax() / 400.f, 0.8f, 2.f));
		L->SetAttenuationRadius(900.f + 2.2f * F.Pol.GetMax());
		L->SetVisibility(true);
	}
}
