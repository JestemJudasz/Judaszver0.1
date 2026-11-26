// Plik: Source/grarts2d/Ludzie/Ludzik.cpp
#include "Ludzie/Ludzik.h"
#include "Budowy/Budowa.h"
#include "Dzwiek/Dzwieki.h"
#include "Osada/Osada.h"
#include "Osada/Drogi.h"
#include "Swiat/SwiatWysokosci.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

namespace
{
	UStaticMesh* Model(const TCHAR* Nazwa)
	{
		const FString Sciezka = FString::Printf(TEXT("/Game/Swiat/Ludzie/%s.%s"), Nazwa, Nazwa);
		UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Sciezka);
		UE_CLOG(!M, LogTemp, Warning, TEXT("Ludzik: brak modelu %s (uruchom Tools/Zbuduj_Ludzi.bat)"), *Sciezka);
		return M;
	}

	/** "niesie kłodę / deski / kamień". */
	const TCHAR* Biernik(ESurowiec S)
	{
		switch (S)
		{
		case ESurowiec::Kloda: return TEXT("kłodę");
		case ESurowiec::Deska: return TEXT("deski");
		case ESurowiec::Jedzenie: return TEXT("jedzenie");
		case ESurowiec::Zboze: return TEXT("snop zboża");
		case ESurowiec::Maka: return TEXT("worek mąki");
		case ESurowiec::Welna: return TEXT("wełnę");
		case ESurowiec::Sukno: return TEXT("belę sukna");
		case ESurowiec::Ruda: return TEXT("rudę");
		case ESurowiec::Bron: return TEXT("włócznie");
		case ESurowiec::Opal: return TEXT("polana");
		case ESurowiec::Ryby: return TEXT("ryby");
		case ESurowiec::Skory: return TEXT("skóry");
		case ESurowiec::Skora: return TEXT("skórę");
		case ESurowiec::Buty: return TEXT("buty");
		case ESurowiec::Ubrania: return TEXT("ubrania");
		case ESurowiec::Slod: return TEXT("worek słodu");
		case ESurowiec::Piwo: return TEXT("baryłkę piwa");
		default: return TEXT("kamień");
		}
	}

	/** "na plac" / "do spichlerza" / "do obozu" (dokad noszacy niesie towar). */
	FString Dokad(const ABudowa* B)
	{
		const FString& M = B->Rodzaj().Model;
		if (M.StartsWith(TEXT("SM_Spichlerz"))) { return TEXT("do spichlerza"); }
		if (M == TEXT("SM_Ognisko")) { return TEXT("do obozu"); }
		return TEXT("na plac");
	}

	const TCHAR* Dopelniacz(ESurowiec S)
	{
		switch (S)
		{
		case ESurowiec::Kloda: return TEXT("kłody");
		case ESurowiec::Deska: return TEXT("deski");
		case ESurowiec::Jedzenie: return TEXT("jedzenie");
		case ESurowiec::Zboze: return TEXT("zboża");
		case ESurowiec::Maka: return TEXT("mąki");
		case ESurowiec::Welna: return TEXT("wełny");
		case ESurowiec::Sukno: return TEXT("sukna");
		case ESurowiec::Ruda: return TEXT("rudy");
		case ESurowiec::Bron: return TEXT("broni");
		case ESurowiec::Opal: return TEXT("polan");
		case ESurowiec::Ryby: return TEXT("ryb");
		case ESurowiec::Skory: return TEXT("skór");
		case ESurowiec::Skora: return TEXT("skóry");
		case ESurowiec::Buty: return TEXT("butów");
		case ESurowiec::Ubrania: return TEXT("ubrań");
		case ESurowiec::Slod: return TEXT("słodu");
		case ESurowiec::Piwo: return TEXT("piwa");
		default: return TEXT("kamień");
		}
	}
}

ALudzik::ALudzik()
{
	PrimaryActorTick.bCanEverTick = true;
	Korzen = CreateDefaultSubobject<USceneComponent>(TEXT("Korzen"));
	RootComponent = Korzen;
	Korzen->SetMobility(EComponentMobility::Movable);
	Korzen->SetRelativeScale3D(FVector(Skala));
	SetCanBeDamaged(false);

	// Stawy w cm (model w prawdziwej skali; calosc powieksza Skala na korzeniu). Biodro na 90 cm.
	Tulow = Czesc(TEXT("Tulow"), Korzen, FVector(0, 0, 90));
	Klatka = Czesc(TEXT("Klatka"), Tulow, FVector(0, 0, 8));
	Glowa = Czesc(TEXT("Glowa"), Klatka, FVector(0, 0, 42));
	RekaL = Czesc(TEXT("RekaL"), Klatka, FVector(0, -23, 37));
	RekaP = Czesc(TEXT("RekaP"), Klatka, FVector(0, 23, 37));
	PrzedL = Czesc(TEXT("PrzedL"), RekaL, FVector(0, 0, -30));
	PrzedP = Czesc(TEXT("PrzedP"), RekaP, FVector(0, 0, -30));
	NogaL = Czesc(TEXT("NogaL"), Korzen, FVector(0, -9, 90));
	NogaP = Czesc(TEXT("NogaP"), Korzen, FVector(0, 9, 90));
	LydkaL = Czesc(TEXT("LydkaL"), NogaL, FVector(0, 0, -44));
	LydkaP = Czesc(TEXT("LydkaP"), NogaP, FVector(0, 0, -44));
	Mlotek = Czesc(TEXT("Mlotek"), PrzedP, FVector(0, 0, -25));
	Kamien = Czesc(TEXT("Kamien"), Tulow, FVector(38, 0, 12));
}

UStaticMeshComponent* ALudzik::Czesc(const TCHAR* Nazwa, USceneComponent* Rodzic, FVector Staw)
{
	UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Nazwa);
	C->SetupAttachment(Rodzic);
	C->SetRelativeLocation(Staw);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetMobility(EComponentMobility::Movable);
	C->SetGenerateOverlapEvents(false);
	return C;
}

void ALudzik::Zaladuj()
{
	// Modele wczytywane tutaj (nie w konstruktorze), zeby brak assetow nie psul ladowania klasy.
	if (bZaladowany)
	{
		return;
	}
	bZaladowany = true;
	Tulow->SetStaticMesh(Model(bKobieta ? TEXT("SM_Ludz_TulowK") : TEXT("SM_Ludz_Biodra")));
	Klatka->SetStaticMesh(bKobieta ? nullptr : Model(TEXT("SM_Ludz_Klatka")));
	Glowa->SetStaticMesh(Model(bKobieta ? TEXT("SM_Ludz_GlowaK") : TEXT("SM_Ludz_Glowa")));
	RekaL->SetStaticMesh(Model(TEXT("SM_Ludz_Ramie")));
	RekaP->SetStaticMesh(Model(TEXT("SM_Ludz_Ramie")));
	PrzedL->SetStaticMesh(Model(TEXT("SM_Ludz_Przedramie")));
	PrzedP->SetStaticMesh(Model(TEXT("SM_Ludz_Przedramie")));
	NogaL->SetStaticMesh(Model(bKobieta ? TEXT("SM_Ludz_UdoK") : TEXT("SM_Ludz_Udo")));
	NogaP->SetStaticMesh(Model(bKobieta ? TEXT("SM_Ludz_UdoK") : TEXT("SM_Ludz_Udo")));
	LydkaL->SetStaticMesh(Model(bKobieta ? TEXT("SM_Ludz_LydkaK") : TEXT("SM_Ludz_Lydka")));
	LydkaP->SetStaticMesh(Model(bKobieta ? TEXT("SM_Ludz_LydkaK") : TEXT("SM_Ludz_Lydka")));
	Rytm = FMath::FRandRange(0.9f, 1.12f); // kazdy chodzi troche innym krokiem
	FazaChodu = FMath::FRand();
	Mlotek->SetStaticMesh(Model(TEXT("SM_Ludz_Mlotek")));
	Kamien->SetStaticMesh(Model(TEXT("SM_Ludz_Kamien")));
}

void ALudzik::Pokaz(const TCHAR* Niesione)
{
	bNiesie = Niesione != nullptr;
	if (Niesione)
	{
		Kamien->SetStaticMesh(Model(Niesione));
	}
	Kamien->SetVisibility(bNiesie);
}

void ALudzik::UstawDom(FVector2D Gdzie)
{
	Dom = Gdzie;
	bMaDom = true;
	if (Wolny())
	{
		Przerwa = FMath::FRandRange(0.5f, 3.f); // zaraz idzie pod nowy dom
		CzasStanu = 0.f;
	}
}

void ALudzik::Zwolnij()
{
	if (bTowarWRekach && Zrodlo.IsValid())
	{
		Zrodlo->ZmienZapas(Towar, Ladunek); // przerwany kurs: towar wraca tam, skad go wzial
	}
	if (bRezZrodla && Zrodlo.IsValid())
	{
		Zrodlo->Zarezerwowane[Towar] = FMath::Max(0, Zrodlo->Zarezerwowane[Towar] - Ladunek);
	}
	if (bRezCelu && Budowa.IsValid())
	{
		Budowa->WDrodze[Towar] = FMath::Max(0, Budowa->WDrodze[Towar] - Ladunek);
	}
	if (bRezDrzewa && Osada.IsValid())
	{
		Osada->ZwolnijDrzewo(Drzewo);
	}
	if (bRezKrzaka && Osada.IsValid())
	{
		Osada->ZwolnijKrzak(Drzewo);
	}
	if (bRez2 && Budowa.IsValid() && Budowa->Rodzaj().Wsad2 != ESurowiec::Ile)
	{
		const ESurowiec W2 = Budowa->Rodzaj().Wsad2;
		Budowa->Zarezerwowane[W2] = FMath::Max(0, Budowa->Zarezerwowane[W2] - 1);
	}
	if (Owca >= 0 && Budowa.IsValid())
	{
		Budowa->ZwolnijOwce(Owca);
	}
	Owca = -1;
	bRezZrodla = bRezCelu = bRezDrzewa = bRezKrzaka = bTowarWRekach = bRez2 = false;
	Ladunek = 1;
	Zrodlo.Reset();
	Pokaz(nullptr);
}

void ALudzik::Przydziel(ABudowa* B, ERolaLudzika R, bool bNaMiejscu)
{
	if (bWalka)
	{
		// W bitwie: osada moze mu zmienic prace, ale zacznie ja dopiero po powrocie.
		if (bZOsady)
		{
			PracaPrzed = B;
			RolaPrzed = R;
		}
		return;
	}
	Zaladuj();
	Zwolnij();
	ZdejmijZbroje();
	Mlotek->SetStaticMesh(Model(TEXT("SM_Ludz_Mlotek")));
	Mlotek->SetVisibility(B && R == ERolaLudzika::Budowniczy);

	if (!B || (R != ERolaLudzika::Budowniczy && R != ERolaLudzika::Tragarz))
	{
		Stala.Reset(); // pomocnik przeniesiony na inna budowe dalej pamieta swoja prace
	}
	BezRoboty = 0.f;
	Budowa = B;
	Rola = R;
	Krok = EKrok::Start;
	Robota = 0;
	Faza = FMath::FRand();
	Kierunek = GetActorRotation().Yaw;
	if (!B)
	{
		Stan = EStan::Stoi;
		CzasStanu = 0.f;
		Przerwa = FMath::FRandRange(2.f, 8.f);
		Status = TEXT("odpoczywa");
		if (bMaDom)
		{
			IdzDo(Dom + FVector2D(FMath::FRandRange(-250.f, 250.f), FMath::FRandRange(-250.f, 250.f)));
		}
		return;
	}
	if (R != ERolaLudzika::Budowniczy && R != ERolaLudzika::Tragarz)
	{
		// Budynek pracy: od razu do roboty (bNaMiejscu: zaczyna przy budynku).
		if (bNaMiejscu)
		{
			const FVector2D W = B->ZUkladu(B->Rodzaj().Wejscie + FVector2D(FMath::FRandRange(-120.f, 120.f), 60.f));
			SetActorLocation(FVector(W, FSwiat::Wysokosc(W.X, W.Y)));
		}
		Stan = EStan::Stoi;
		NastepneZadanie();
		return;
	}
	if (bNaMiejscu)
	{
		// Od razu przy pracy: budowniczy w losowym miejscu obwodu, tragarz w drodze pod mur.
		const FRodzajBudynku& RB = B->Rodzaj();
		FVector2D Lok;
		if (R == ERolaLudzika::Budowniczy)
		{
			S = FMath::FRand() * ObwodDlugosc();
			Lok = NaObwodzie(S);
		}
		else
		{
			Lok = FVector2D(FMath::Lerp(RB.ZrzutX.X, RB.ZrzutX.Y, FMath::FRand()), FMath::Lerp(RB.Sterta.Y - 120.f, RB.ZrzutY, FMath::FRand()));
		}
		const FVector2D W = ZBudynku(Lok);
		SetActorLocation(FVector(W, FSwiat::Wysokosc(W.X, W.Y)));
		if (R == ERolaLudzika::Budowniczy)
		{
			Stan = EStan::Idzie;
			bPoObwodzie = true;
			SCel = S;
			Cel = W;
		}
		else
		{
			Stan = EStan::Stoi;
		}
		NastepneZadanie();
		return;
	}
	Stan = EStan::Idzie;
	if (R == ERolaLudzika::Budowniczy)
	{
		// Najpierw prosto do najblizszego punktu obwodu, potem praca wzdluz niego.
		const FVector2D Lok(B->GetActorTransform().InverseTransformPosition(GetActorLocation()));
		S = NajblizszeS(Lok);
		IdzDo(ZBudynku(NaObwodzie(S)));
		SCel = S;
		Status = TEXT("idzie na budowę");
	}
	else
	{
		Stan = EStan::Stoi;
		NastepneZadanie();
	}
}

void ALudzik::Przewiez(ABudowa* Z, ABudowa* CelB, ESurowiec Sur)
{
	if (bWalka)
	{
		return;
	}
	Zaladuj();
	Zwolnij();
	Mlotek->SetVisibility(false);
	Budowa = CelB;
	Zrodlo = Z;
	Towar = Sur;
	Rola = ERolaLudzika::Noszacy;
	// Balans 21.11.2026 ("donoszenie prostsze i szybsze"): noszacy tez bierze do 3 sztuk naraz.
	Ladunek = FMath::Clamp(FMath::Min(Z->Dostepne(Sur), CelB->Miejsce(Sur)), 1, 3);
	Z->Zarezerwowane[Sur] += Ladunek;
	CelB->WDrodze[Sur] += Ladunek;
	bRezZrodla = bRezCelu = true;
	Krok = EKrok::PoTowar;
	IdzDo(Z->PrzyStosie(Sur));
	Status = FString::Printf(TEXT("idzie po %s"), Dopelniacz(Sur));
}

void ALudzik::Odejdz(FVector2D Gdzie)
{
	Przydziel(nullptr, ERolaLudzika::Budowniczy, false);
	bMaDom = false;
	bOdchodzi = true;
	if (bWalka)
	{
		// Poprawka 25.11.2026: rodzina odchodzi, a maz walczy - nie schodzi z pola; po bitwie wraca do obozu
		// (WrocZBitwy, bez domu) i stamtad odchodzi.
		return;
	}
	IdzDo(Gdzie);
	Status = TEXT("odchodzi z osady");
}

FString ALudzik::CoRobi() const
{
	if (Stala.IsValid() && Budowa.IsValid())
	{
		return FString::Printf(TEXT("pomaga przy budowie (%s)"), *Budowa->Rodzaj().Nazwa);
	}
	return Status;
}

void ALudzik::UstawKolory(FLinearColor Tunika, FLinearColor Wlosy)
{
	// M_Ludzie: CPD 0..2 = tunika (maska alfa 1), 3..5 = wlosy (maska 0,5).
	if (!bPrzeszywanica)
	{
		KolorTuniki = Tunika; // przeszywanica woja tylko przykrywa tunike
		KolorWlosow = Wlosy;
	}
	for (UStaticMeshComponent* C : { Tulow.Get(), Klatka.Get(), Glowa.Get(), RekaL.Get(), RekaP.Get(), PrzedL.Get(), PrzedP.Get(), NogaL.Get(), NogaP.Get(), LydkaL.Get(), LydkaP.Get(), Mlotek.Get(), Kamien.Get() })
	{
		C->SetCustomPrimitiveDataVector3(0, FVector(Tunika.R, Tunika.G, Tunika.B));
		C->SetCustomPrimitiveDataVector3(3, FVector(Wlosy.R, Wlosy.G, Wlosy.B));
	}
}

FVector2D ALudzik::ZBudynku(FVector2D L) const
{
	if (!Budowa.IsValid())
	{
		return L;
	}
	return FVector2D(Budowa->GetActorTransform().TransformPosition(FVector(L, 0.0)));
}

float ALudzik::ObwodDlugosc() const
{
	if (!Budowa.IsValid())
	{
		return 1.f;
	}
	const FVector2D R = Budowa->Rodzaj().ObwodMax - Budowa->Rodzaj().ObwodMin;
	return 2.f * (R.X + R.Y);
}

FVector2D ALudzik::NaObwodzie(float Sx, FVector2D* DoSrodka) const
{
	const FVector2D A = Budowa->Rodzaj().ObwodMin, B = Budowa->Rodzaj().ObwodMax;
	const float W = B.X - A.X, H = B.Y - A.Y;
	Sx = FMath::Fmod(Sx, 2.f * (W + H));
	if (Sx < 0.f) { Sx += 2.f * (W + H); }
	FVector2D P, N;
	if (Sx < W)              { P = FVector2D(A.X + Sx, A.Y);               N = FVector2D(0, 1); }
	else if (Sx < W + H)     { P = FVector2D(B.X, A.Y + (Sx - W));         N = FVector2D(-1, 0); }
	else if (Sx < 2 * W + H) { P = FVector2D(B.X - (Sx - W - H), B.Y);     N = FVector2D(0, -1); }
	else                     { P = FVector2D(A.X, B.Y - (Sx - 2 * W - H)); N = FVector2D(1, 0); }
	if (DoSrodka) { *DoSrodka = N; }
	return P;
}

float ALudzik::NajblizszeS(FVector2D L) const
{
	float Najl = 0.f, Odl = TNumericLimits<float>::Max();
	const float P = ObwodDlugosc();
	for (float Sx = 0.f; Sx < P; Sx += 50.f)
	{
		const float D = FVector2D::DistSquared(NaObwodzie(Sx), L);
		if (D < Odl) { Odl = D; Najl = Sx; }
	}
	return Najl;
}

namespace
{
	// Omijanie budynkow (22.11.2026): obrys budynku (uklad budynku) z zapasem na szerokosc czlowieka.
	constexpr float ZapasOminiecia = 25.f;
	struct FPrzeszkoda
	{
		FTransform T;
		FBox2D B;
		FVector2D Drzwi; // uklad budynku, tuz za obrysem
		bool bWejscie;   // gotowy budynek: do srodka i ze srodka przez drzwi
	};
	FVector2D DoUkladu(const FPrzeszkoda& P, FVector2D W) { return FVector2D(P.T.InverseTransformPosition(FVector(W, 0.0))); }
	FVector2D ZUkl(const FPrzeszkoda& P, FVector2D L) { return FVector2D(P.T.TransformPosition(FVector(L, 0.0))); }

	/** Czy odcinek A-B wchodzi do wnetrza prostokata (Liang-Barsky); T0 = gdzie wchodzi, 0..1. */
	bool Przecina(const FBox2D& Bx, FVector2D A, FVector2D B, float& T0)
	{
		double Lo = 0.0, Hi = 1.0;
		const FVector2D D = B - A;
		for (int32 Os = 0; Os < 2; ++Os)
		{
			if (FMath::Abs(D[Os]) < 1e-3)
			{
				if (A[Os] <= Bx.Min[Os] || A[Os] >= Bx.Max[Os]) { return false; }
				continue;
			}
			double T1 = (Bx.Min[Os] - A[Os]) / D[Os], T2 = (Bx.Max[Os] - A[Os]) / D[Os];
			if (T1 > T2) { Swap(T1, T2); }
			Lo = FMath::Max(Lo, T1);
			Hi = FMath::Min(Hi, T2);
			if (Lo >= Hi) { return false; }
		}
		T0 = float(Lo);
		return true;
	}

	/** Wstawia do Pkt (punkty po kolei, ostatni = cel) drzwi budynkow, do ktorych sie wchodzi / z ktorych wychodzi,
	 *  i narozniki budynkow stojacych na drodze. Pola i pastwiska nie sa przeszkoda. */
	void OminBudynki(UWorld* W, FVector2D Start, TArray<FVector2D>& Pkt, float Zapas = ZapasOminiecia)
	{
		FBox2D Zasieg(Start, Start);
		for (const FVector2D& P : Pkt) { Zasieg += P; }
		TArray<FPrzeszkoda> Prz;
		for (TActorIterator<ABudowa> It(W); It; ++It)
		{
			const ABudowa* B = *It;
			const FRodzajBudynku& R = B->Rodzaj();
			if (B->Podglad() || R.ZObszarem() || R.bUmocnienie) // umocnien nie omijamy (shortcut)
			{
				continue;
			}
			const FBox2D Bx = R.Obrys().ExpandBy(Zapas);
			if (!Zasieg.ExpandBy(Bx.GetExtent().Size()).IsInside(FVector2D(B->GetActorLocation())))
			{
				continue;
			}
			// Drzwi: punkt wejscia, wypchniety za najblizszy bok obrysu, jesli lezy w srodku.
			FVector2D D = R.Wejscie;
			if (Bx.IsInside(D))
			{
				const float Od[4] = { D.X - Bx.Min.X, Bx.Max.X - D.X, D.Y - Bx.Min.Y, Bx.Max.Y - D.Y };
				int32 N = 0;
				for (int32 k = 1; k < 4; ++k) { if (Od[k] < Od[N]) { N = k; } }
				if (N == 0) { D.X = Bx.Min.X - 30.f; } else if (N == 1) { D.X = Bx.Max.X + 30.f; }
				else if (N == 2) { D.Y = Bx.Min.Y - 30.f; } else { D.Y = Bx.Max.Y + 30.f; }
			}
			Prz.Add({ B->GetActorTransform(), Bx, D, B->Gotowy() });
		}
		if (Prz.Num() == 0)
		{
			return;
		}
		// Wejscie frontem: cel w budynku = najpierw drzwi; start w budynku = najpierw wyjscie drzwiami.
		const FVector2D Koniec = Pkt.Last();
		for (const FPrzeszkoda& P : Prz)
		{
			const bool bS = P.B.IsInside(DoUkladu(P, Start)), bK = P.B.IsInside(DoUkladu(P, Koniec));
			if (P.bWejscie && bK && !bS) { Pkt.Insert(ZUkl(P, P.Drzwi), Pkt.Num() - 1); }
			if (P.bWejscie && bS && !bK) { Pkt.Insert(ZUkl(P, P.Drzwi), 0); }
		}
		// Kazdy odcinek: pierwszy budynek na nim obchodzi naroznikiem (najkrotsza droga), potem sprawdza od nowa.
		int32 Limit = 24; // shortcut: zachlanne narozniki, nie pelne szukanie drogi; gesta zabudowa moze dac objazd na okolo
		FVector2D A = Start;
		for (int32 i = 0; i < Pkt.Num(); ++i)
		{
			const FVector2D Bp = Pkt[i];
			int32 Naj = -1;
			float NajT = 2.f;
			for (int32 k = 0; k < Prz.Num(); ++k)
			{
				const FVector2D La = DoUkladu(Prz[k], A), Lb = DoUkladu(Prz[k], Bp);
				float T;
				// Odcinek zaczyna sie albo konczy w budynku (praca w obejsciu, drzwi): ten budynek go nie blokuje.
				if (!Prz[k].B.IsInside(La) && !Prz[k].B.IsInside(Lb) && Przecina(Prz[k].B, La, Lb, T) && T < NajT)
				{
					NajT = T;
					Naj = k;
				}
			}
			if (Naj >= 0 && Limit > 0)
			{
				const FPrzeszkoda& P = Prz[Naj];
				const FBox2D Szer = P.B.ExpandBy(15.f);
				const FVector2D La = DoUkladu(P, A), Lb = DoUkladu(P, Bp);
				const FVector2D Rogi[4] = { Szer.Min, FVector2D(Szer.Min.X, Szer.Max.Y), Szer.Max, FVector2D(Szer.Max.X, Szer.Min.Y) };
				float Best = TNumericLimits<float>::Max();
				int32 Wyb = -1;
				for (int32 r = 0; r < 4; ++r)
				{
					float T;
					const float K = FVector2D::Distance(La, Rogi[r]) + FVector2D::Distance(Rogi[r], Lb);
					if (!Przecina(P.B, La, Rogi[r], T) && K < Best)
					{
						Best = K;
						Wyb = r;
					}
				}
				if (Wyb >= 0)
				{
					Pkt.Insert(ZUkl(P, Rogi[Wyb]), i);
					--Limit;
					--i; // sprawdz jeszcze odcinek A -> naroznik
					continue;
				}
			}
			A = Bp;
		}
	}
}

void ALudzik::OminBudynkiTrasa(UWorld* W, FVector2D Start, TArray<FVector2D>& Pkt, float Zapas)
{
	OminBudynki(W, Start, Pkt, Zapas);
}

void ALudzik::IdzDo(FVector2D C)
{
	Cel = C;
	// Drogi: jesli nimi szybciej (o Premia), idzie przez nie (poza bitwa).
	Trasa.Reset();
	TrasaI = 0;
	if (ADrogi* D = !bWalka ? ADrogi::Obecne() : nullptr)
	{
		D->Trasa(FVector2D(GetActorLocation()), C, Trasa);
	}
	// Budynki: obchodzi je i wchodzi drzwiami (poza bitwa).
	if (!bWalka)
	{
		if (Trasa.Num() == 0)
		{
			Trasa.Add(C);
		}
		OminBudynki(GetWorld(), FVector2D(GetActorLocation()), Trasa);
		Cel = Trasa[0];
	}
	bPoObwodzie = false;
	Stan = EStan::Idzie;
}

void ALudzik::Czekaj(float Sekundy, const TCHAR* Co)
{
	Stan = EStan::Stoi;
	CzasStanu = 0.f;
	Przerwa = Sekundy;
	Status = Co;
}

void ALudzik::Pracuj(float Sekundy, FVector2D PatrzNa, const TCHAR* Narzedzie, const TCHAR* Co)
{
	BezRoboty = 0.f;
	Stan = EStan::Pracuje;
	CzasStanu = -Sekundy;
	Patrz = (PatrzNa - FVector2D(GetActorLocation())).GetSafeNormal();
	if (Patrz.IsNearlyZero())
	{
		Patrz = FVector2D(1.f, 0.f);
	}
	const bool bNarzedzie = Narzedzie && *Narzedzie;
	if (bNarzedzie)
	{
		Mlotek->SetStaticMesh(Model(Narzedzie));
	}
	Mlotek->SetVisibility(bNarzedzie);
	Status = Co;
}

void ALudzik::Schyl(EStan St, float Sekundy, FVector2D PatrzNa)
{
	Stan = St;
	CzasStanu = -Sekundy;
	const FVector2D D = (PatrzNa - FVector2D(GetActorLocation())).GetSafeNormal();
	if (!D.IsNearlyZero())
	{
		Patrz = D;
	}
}

bool ALudzik::WezZeZrodla()
{
	ABudowa* Z = Zrodlo.Get();
	if (bRezZrodla && Z)
	{
		Z->Zarezerwowane[Towar] = FMath::Max(0, Z->Zarezerwowane[Towar] - Ladunek);
	}
	bRezZrodla = false;
	if (!Z || Z->Zapas[Towar] <= 0)
	{
		return false;
	}
	// Tragarz moze brac kilka sztuk; gdy na stosie jest mniej, budowa czeka na reszte od innych.
	const int32 Wziete = FMath::Clamp(Z->Zapas[Towar], 1, Ladunek);
	if (bRezCelu && Budowa.IsValid() && Wziete < Ladunek)
	{
		Budowa->WDrodze[Towar] = FMath::Max(0, Budowa->WDrodze[Towar] - (Ladunek - Wziete));
	}
	Ladunek = Wziete;
	Z->ZmienZapas(Towar, -Ladunek);
	bTowarWRekach = true;
	Pokaz(NiesionySurowiec(Towar));
	return true;
}

void ALudzik::Pomoz(ABudowa* B, ERolaLudzika R)
{
	ABudowa* Moja = Budowa.Get();
	const ERolaLudzika MojaRola = Rola;
	Przydziel(B, R, false);
	Stala = Moja;
	RolaStala = MojaRola;
}

void ALudzik::Odwolaj()
{
	ABudowa* Wroc = Stala.Get();
	Przydziel(Wroc, Wroc ? RolaStala : ERolaLudzika::Budowniczy, false);
}

void ALudzik::KoniecPracy(bool bOdejdz)
{
	if (Stala.IsValid() && (Rola == ERolaLudzika::Budowniczy || Rola == ERolaLudzika::Tragarz))
	{
		Odwolaj(); // pomagal przy budowie: wraca do swojej pracy
		return;
	}
	ABudowa* B = Budowa.Get();
	Zwolnij();
	ZdejmijZbroje();
	Mlotek->SetVisibility(false);
	Budowa.Reset();
	Krok = EKrok::Start;
	Status = TEXT("odpoczywa");
	if (bOdejdz && B)
	{
		const FVector2D Kier = FVector2D(GetActorLocation() - B->GetActorLocation()).GetSafeNormal();
		IdzDo(FVector2D(GetActorLocation()) + Kier * FMath::FRandRange(600.f, 1400.f));
		return;
	}
	Stan = EStan::Stoi;
	CzasStanu = 0.f;
	Przerwa = FMath::FRandRange(3.f, 10.f);
}

void ALudzik::NastepneZadanie()
{
	CzasStanu = 0.f;
	switch (Rola)
	{
	case ERolaLudzika::Budowniczy: ZadanieBudowniczego(); break;
	case ERolaLudzika::Tragarz: ZadanieTragarza(); break;
	case ERolaLudzika::Drwal: ZadanieDrwala(); break;
	case ERolaLudzika::Borowy: ZadanieBorowego(); break;
	case ERolaLudzika::Pilarz: ZadaniePilarza(); break;
	case ERolaLudzika::Noszacy: ZadanieNoszacego(); break;
	case ERolaLudzika::Kamieniarz: ZadanieKamieniarza(); break;
	case ERolaLudzika::Zbieracz: ZadanieZbieracza(); break;
	case ERolaLudzika::Mysliwy: ZadanieMysliwego(); break;
	case ERolaLudzika::Rolnik: ZadanieRolnika(); break;
	case ERolaLudzika::Rzemieslnik: ZadanieRzemieslnika(); break;
	case ERolaLudzika::Pasterz: ZadaniePasterza(); break;
	case ERolaLudzika::Woj: ZadanieWoja(); break;
	case ERolaLudzika::Rybak: ZadanieRybaka(); break;
	}
}

void ALudzik::ZadanieBudowniczego()
{
	ABudowa* B = Budowa.Get();
	if (!B || B->Gotowy())
	{
		KoniecPracy(B && Stan != EStan::Stoi && Stan != EStan::Idzie);
		return;
	}
	if (Stan == EStan::Idzie)
	{
		FVector2D N;
		NaObwodzie(S, &N);
		Patrz = FVector2D(B->GetActorTransform().TransformVectorNoScale(FVector(N, 0.0)));
		if (B->CzekaNaMaterial())
		{
			// Brak materialu: stoi przy budowie, za chwile przejdzie w inne miejsce i sprawdzi znowu.
			Mlotek->SetVisibility(false);
			Czekaj(FMath::FRandRange(2.5f, 5.f), TEXT("czeka na materiał"));
			return;
		}
		// Doszedl: stuka mlotkiem, twarza do budynku.
		Mlotek->SetStaticMesh(Model(TEXT("SM_Ludz_Mlotek")));
		Mlotek->SetVisibility(true);
		Stan = EStan::Pracuje;
		CzasStanu = -FMath::FRandRange(4.f, 8.f); // pracuje do 0
		Status = TEXT("buduje");
		return;
	}
	// Przejscie w inne miejsce wzdluz obwodu (do 1/3 obwodu w jedna albo druga strone).
	SCel = S + FMath::FRandRange(-1.f, 1.f) * ObwodDlugosc() / 3.f;
	bPoObwodzie = true;
	Stan = EStan::Idzie;
}

void ALudzik::ZadanieTragarza()
{
	ABudowa* B = Budowa.Get();
	if (!B || B->Gotowy())
	{
		KoniecPracy(B && Stan != EStan::Stoi && Stan != EStan::Idzie);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	const FVector2D Sterta = ZBudynku(FVector2D(R.Sterta.X + FMath::FRandRange(-60.f, 60.f), R.Sterta.Y - 90.f));
	switch (Krok)
	{
	case EKrok::Start:
	{
		// Najpierw material, ktorego brakuje: najblizsze zrodlo (plac skladowy, oboz, stos przy chacie drwala / pilarni).
		for (ESurowiec Sur : WszystkieSurowce)
		{
			if (B->Brakuje(Sur) <= 0 || !Osada.IsValid())
			{
				continue;
			}
			if (ABudowa* Z = Osada->ZrodloSurowca(Sur, FVector2D(B->GetActorLocation()), nullptr))
			{
				// Balans 21.11.2026 (uzytkownik: "ponoszenie materialow trwa strasznie dlugo", potem "jeszcze o 20%
				// szybciej"): do 4 sztuk naraz, chod o 20% szybszy (TempoNoszenia), krotsze schylanie.
				Zrodlo = Z;
				Towar = Sur;
				Ladunek = FMath::Clamp(FMath::Min(B->Brakuje(Sur), Z->Dostepne(Sur)), 1, 4);
				Z->Zarezerwowane[Sur] += Ladunek;
				B->WDrodze[Sur] += Ladunek;
				bRezZrodla = bRezCelu = true;
				Krok = EKrok::PoTowar;
				IdzDo(Z->PrzyStosie(Sur));
				Status = FString::Printf(TEXT("idzie po %s"), Dopelniacz(Sur));
				return;
			}
		}
		if (B->CzekaNaMaterial())
		{
			// Nie ma skad wziac (albo inni juz niosa): czeka przy stercie.
			if (FVector2D::Distance(FVector2D(GetActorLocation()), Sterta) > 400.f)
			{
				Krok = EKrok::Wraca;
				IdzDo(Sterta);
				Status = TEXT("wraca na budowę");
			}
			else
			{
				Czekaj(1.5f, TEXT("czeka na materiał"));
			}
			return;
		}
		// Material jest: nosi go ze sterty pod mur (to tez praca przy budowie).
		Krok = EKrok::DoSterty;
		IdzDo(Sterta);
		Status = TEXT("nosi materiał");
		return;
	}
	case EKrok::Wraca:
		Krok = EKrok::Start;
		Czekaj(2.f, TEXT("czeka na materiał"));
		return;
	case EKrok::PoTowar:
		Krok = EKrok::Bierze;
		Schyl(EStan::Bierze, 0.6f, Zrodlo.IsValid() ? Zrodlo->ZUkladu(Zrodlo->Rodzaj().Stosy[int32(Towar)]) : FVector2D(GetActorLocation()));
		return;
	case EKrok::Bierze:
		if (!WezZeZrodla())
		{
			Zwolnij(); // ktos zabral ostatnia sztuke
			Krok = EKrok::Start;
			Czekaj(0.5f, TEXT("nosi materiał"));
			return;
		}
		Krok = EKrok::Niesie;
		IdzDo(Sterta);
		Status = Ladunek > 1 ? FString::Printf(TEXT("niesie %s"), *IleSurowca(Towar, Ladunek)) : FString::Printf(TEXT("niesie %s"), Biernik(Towar));
		return;
	case EKrok::Niesie:
		Krok = EKrok::Odklada;
		Schyl(EStan::Odklada, 0.5f, ZBudynku(R.Sterta));
		return;
	case EKrok::Odklada:
		if (bTowarWRekach)
		{
			for (int32 k = 0; k < Ladunek; ++k)
			{
				B->Dostarcz(Towar);
			}
		}
		bTowarWRekach = bRezCelu = false;
		Ladunek = 1;
		Zrodlo.Reset();
		Pokaz(nullptr);
		Krok = EKrok::Start;
		ZadanieTragarza();
		return;
	case EKrok::DoSterty:
		Krok = EKrok::ZeSterty;
		Schyl(EStan::Bierze, 0.6f, ZBudynku(R.Sterta));
		return;
	case EKrok::ZeSterty:
		Pokaz(*R.Niesione);
		Krok = EKrok::PodMur;
		IdzDo(ZBudynku(FVector2D(FMath::FRandRange(R.ZrzutX.X, R.ZrzutX.Y), R.ZrzutY)));
		return;
	case EKrok::PodMur:
		Krok = EKrok::NaMur;
		Patrz = FVector2D(B->GetActorTransform().TransformVectorNoScale(FVector(0, -1, 0)));
		Stan = EStan::Odklada;
		CzasStanu = -0.5f;
		return;
	case EKrok::NaMur:
		B->DodajPrace(9.f); // jeden kamien / bal pod mur
		Pokaz(nullptr);
		Krok = EKrok::Start;
		ZadanieTragarza();
		return;
	default:
		Krok = EKrok::Start;
		Czekaj(1.f, TEXT("nosi materiał"));
		return;
	}
}

void ALudzik::ZadanieDrwala()
{
	ABudowa* B = Budowa.Get();
	if (!B || !Osada.IsValid())
	{
		KoniecPracy(false);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	switch (Krok)
	{
	case EKrok::Start:
	{
		Mlotek->SetVisibility(false);
		if (B->Zapas[ESurowiec::Kloda] >= R.Pojemnosc)
		{
			Bezrobotny(4.f, TEXT("stos kłód pełny"));
			return;
		}
		FVector2D D;
		// Najpierw zasieg chaty, a gdy tam juz nie ma drzew - coraz dalej (DalszeZasiegi), zamiast stac.
		int32 Dalej = 0;
		while (Dalej < IleZasiegow && !Osada->ZarezerwujDrzewo(FVector2D(B->GetActorLocation()), R.Zasieg * DalszeZasiegi[Dalej], D))
		{
			++Dalej;
		}
		if (Dalej >= IleZasiegow)
		{
			Bezrobotny(5.f, TEXT("brak drzew w okolicy"));
			return;
		}
		Drzewo = D;
		bRezDrzewa = true;
		Krok = EKrok::DoCelu;
		// Staje 1,1 m od pnia, od strony, z ktorej przyszedl.
		const FVector2D Od = (FVector2D(GetActorLocation()) - D).GetSafeNormal();
		IdzDo(D + (Od.IsNearlyZero() ? FVector2D(1.f, 0.f) : Od) * 110.f);
		Status = Dalej > 0 ? TEXT("idzie po drzewo daleko") : TEXT("idzie do drzewa");
		return;
	}
	case EKrok::DoCelu:
		Krok = EKrok::Praca;
		Pracuj(FMath::FRandRange(5.f, 7.f), Drzewo, TEXT("SM_Ludz_Siekiera"), TEXT("ścina drzewo"));
		return;
	case EKrok::Praca:
		Mlotek->SetVisibility(false);
		Osada->ZetnijDrzewo(Drzewo);
		bRezDrzewa = false;
		Krok = EKrok::Bierze;
		Schyl(EStan::Bierze, 1.2f, Drzewo);
		return;
	case EKrok::Bierze:
		Pokaz(NiesionySurowiec(ESurowiec::Kloda));
		Krok = EKrok::Niesie;
		IdzDo(B->PrzyStosie(ESurowiec::Kloda));
		Status = TEXT("niesie kłodę");
		return;
	case EKrok::Niesie:
		Krok = EKrok::Odklada;
		Schyl(EStan::Odklada, 1.f, B->ZUkladu(R.Stosy[int32(ESurowiec::Kloda)]));
		return;
	case EKrok::Odklada:
		// Drzewo daje dwie klody (balans 21.11.2026), stos nie przepelnia sie ponad Pojemnosc.
		B->ZmienZapas(ESurowiec::Kloda, FMath::Clamp(R.Pojemnosc - B->Zapas[ESurowiec::Kloda], 1, 2));
		Pokaz(nullptr);
		Krok = EKrok::Start;
		ZadanieDrwala();
		return;
	default:
		Krok = EKrok::Start;
		Czekaj(1.f, TEXT("odpoczywa"));
		return;
	}
}

void ALudzik::ZadanieBorowego()
{
	ABudowa* B = Budowa.Get();
	if (!B || !Osada.IsValid())
	{
		KoniecPracy(false);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	const FVector2D Wej = B->ZUkladu(R.Wejscie + FVector2D(FMath::FRandRange(-100.f, 100.f), 40.f));
	switch (Krok)
	{
	case EKrok::Start:
	{
		if (Osada->Sadzonek(FVector2D(B->GetActorLocation()), R.Zasieg) >= 10) // 26.11.2026: bylo 16 ("borowy za szybko produkuje drzewa")
		{
			Bezrobotny(6.f, TEXT("pilnuje młodego lasu"));
			return;
		}
		FVector2D P;
		if (!Osada->MiejsceNaDrzewo(FVector2D(B->GetActorLocation()), R.Zasieg, P))
		{
			Bezrobotny(5.f, TEXT("brak miejsca na drzewa"));
			return;
		}
		Drzewo = P;
		bRezDrzewa = true;
		Krok = EKrok::PoSadzonke;
		IdzDo(Wej);
		Status = TEXT("idzie po sadzonkę");
		return;
	}
	case EKrok::PoSadzonke:
		Krok = EKrok::Bierze;
		Schyl(EStan::Bierze, 1.f, B->ZUkladu(FVector2D(R.Polowa.X * 0.7f, 0.f)));
		return;
	case EKrok::Bierze:
		Pokaz(TEXT("SM_Ludz_Sadzonka"));
		Krok = EKrok::Niesie;
		IdzDo(Drzewo + FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal() * 70.f);
		Status = TEXT("niesie sadzonkę");
		return;
	case EKrok::Niesie:
		Krok = EKrok::Praca;
		Pokaz(nullptr);
		Mlotek->SetStaticMesh(Model(TEXT("SM_Ludz_Motyka")));
		Mlotek->SetVisibility(true);
		Schyl(EStan::Kopie, FMath::FRandRange(4.f, 6.f), Drzewo);
		Status = TEXT("sadzi drzewo");
		return;
	case EKrok::Praca:
		Mlotek->SetVisibility(false);
		Osada->Zasadz(Drzewo);
		bRezDrzewa = false;
		Krok = EKrok::Wraca;
		IdzDo(Wej);
		Status = TEXT("wraca do chaty");
		return;
	case EKrok::Wraca:
	default:
		Krok = EKrok::Start;
		Czekaj(FMath::FRandRange(1.5f, 3.f), TEXT("odpoczywa"));
		return;
	}
}

void ALudzik::ZadaniePilarza()
{
	ABudowa* B = Budowa.Get();
	if (!B || !Osada.IsValid())
	{
		KoniecPracy(false);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	// Dwoch pilarzy po obu stronach dolu (mezczyzna z przodu, kobieta z tylu).
	const FVector2D Stan0 = FVector2D(R.Stanowisko.X + (bKobieta ? 60.f : -60.f), bKobieta ? -R.Stanowisko.Y : R.Stanowisko.Y);
	switch (Krok)
	{
	case EKrok::Start:
		Mlotek->SetVisibility(false);
		if (B->Zapas[ESurowiec::Deska] >= R.Pojemnosc)
		{
			Bezrobotny(4.f, TEXT("stos desek pełny"));
			return;
		}
		if (B->Dostepne(ESurowiec::Kloda) > 0)
		{
			Zrodlo = B;
			Towar = ESurowiec::Kloda;
			B->Zarezerwowane[ESurowiec::Kloda] += 1;
			bRezZrodla = true;
			Krok = EKrok::DoCelu;
			IdzDo(B->ZUkladu(Stan0));
			Status = TEXT("idzie piłować");
			return;
		}
		if (B->Zapas[ESurowiec::Kloda] + B->WDrodze[ESurowiec::Kloda] >= 4)
		{
			Bezrobotny(3.f, TEXT("czeka na kłody"));
			return;
		}
		if (ABudowa* Z = Osada->ZrodloSurowca(ESurowiec::Kloda, FVector2D(B->GetActorLocation()), B))
		{
			Zrodlo = Z;
			Towar = ESurowiec::Kloda;
			Z->Zarezerwowane[ESurowiec::Kloda] += 1;
			B->WDrodze[ESurowiec::Kloda] += 1;
			bRezZrodla = bRezCelu = true;
			Krok = EKrok::PoTowar;
			IdzDo(Z->PrzyStosie(ESurowiec::Kloda));
			Status = TEXT("idzie po kłody");
			return;
		}
		Bezrobotny(5.f, TEXT("brak kłód"));
		return;
	case EKrok::PoTowar:
		Krok = EKrok::Bierze;
		Schyl(EStan::Bierze, 1.2f, Zrodlo.IsValid() ? Zrodlo->ZUkladu(Zrodlo->Rodzaj().Stosy[int32(Towar)]) : FVector2D(GetActorLocation()));
		return;
	case EKrok::Bierze:
		if (!WezZeZrodla())
		{
			Zwolnij();
			Krok = EKrok::Start;
			Bezrobotny(1.f, TEXT("czeka na kłody"));
			return;
		}
		Krok = EKrok::Niesie;
		IdzDo(B->PrzyStosie(ESurowiec::Kloda));
		Status = TEXT("niesie kłodę");
		return;
	case EKrok::Niesie:
		Krok = EKrok::Odklada;
		Schyl(EStan::Odklada, 1.f, B->ZUkladu(R.Stosy[int32(ESurowiec::Kloda)]));
		return;
	case EKrok::Odklada:
		if (bTowarWRekach)
		{
			B->WDrodze[ESurowiec::Kloda] = FMath::Max(0, B->WDrodze[ESurowiec::Kloda] - 1);
			B->ZmienZapas(ESurowiec::Kloda, 1);
		}
		bTowarWRekach = bRezCelu = false;
		Zrodlo.Reset();
		Pokaz(nullptr);
		Krok = EKrok::Start;
		ZadaniePilarza();
		return;
	case EKrok::DoCelu:
		Krok = EKrok::Praca;
		Pracuj(FMath::FRandRange(6.f, 8.f), B->ZUkladu(FVector2D(Stan0.X, 0.f)), TEXT("SM_Ludz_Pila"), TEXT("piłuje deski"));
		return;
	case EKrok::Praca:
		Mlotek->SetVisibility(false);
		if (bRezZrodla && B->Zapas[ESurowiec::Kloda] > 0)
		{
			B->Zarezerwowane[ESurowiec::Kloda] = FMath::Max(0, B->Zarezerwowane[ESurowiec::Kloda] - 1);
			B->ZmienZapas(ESurowiec::Kloda, -1);
			B->ZmienZapas(ESurowiec::Deska, 2); // z jednej klody dwie deski
		}
		bRezZrodla = false;
		Zrodlo.Reset();
		Krok = EKrok::Start;
		ZadaniePilarza();
		return;
	default:
		Krok = EKrok::Start;
		Czekaj(1.f, TEXT("odpoczywa"));
		return;
	}
}

void ALudzik::ZadanieNoszacego()
{
	ABudowa* CelB = Budowa.Get();
	if (!CelB || (!Zrodlo.IsValid() && !bTowarWRekach))
	{
		KoniecPracy(false);
		return;
	}
	switch (Krok)
	{
	case EKrok::PoTowar:
		Krok = EKrok::Bierze;
		Schyl(EStan::Bierze, 0.6f, Zrodlo->ZUkladu(Zrodlo->Rodzaj().Stosy[int32(Towar)]));
		return;
	case EKrok::Bierze:
		if (!WezZeZrodla())
		{
			KoniecPracy(false);
			return;
		}
		Krok = EKrok::Niesie;
		IdzDo(CelB->PrzyStosie(Towar));
		Status = FString::Printf(TEXT("niesie %s %s"), Biernik(Towar), *Dokad(CelB));
		return;
	case EKrok::Niesie:
		Krok = EKrok::Odklada;
		Schyl(EStan::Odklada, 0.5f, CelB->ZUkladu(CelB->Rodzaj().Stosy[int32(Towar)]));
		return;
	case EKrok::Odklada:
		if (bTowarWRekach)
		{
			CelB->WDrodze[Towar] = FMath::Max(0, CelB->WDrodze[Towar] - Ladunek);
			CelB->ZmienZapas(Towar, Ladunek);
		}
		bTowarWRekach = bRezCelu = false;
		Ladunek = 1;
		KoniecPracy(false); // kurs skonczony: znow wolny
		return;
	default:
		KoniecPracy(false);
		return;
	}
}

void ALudzik::ZadanieKamieniarza()
{
	ABudowa* B = Budowa.Get();
	if (!B || !Osada.IsValid())
	{
		KoniecPracy(false);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	// Kamieniolom: kamien, kopalnia rudy: ruda (Wyrob budynku); ten sam stos i ta sama praca kilofem.
	const ESurowiec W = R.Wyrob == ESurowiec::Ile ? ESurowiec::Kamien : R.Wyrob;
	// Kieruje sie do lica skaly (-Y budynku w UE).
	const FVector2D DoSkaly = FVector2D(B->GetActorTransform().TransformVectorNoScale(FVector(0.f, -100.f, 0.f)));
	switch (Krok)
	{
	case EKrok::Start:
		Mlotek->SetVisibility(false);
		if (B->Zapas[W] >= R.Pojemnosc)
		{
			Bezrobotny(4.f, W == ESurowiec::Kamien ? TEXT("stos kamieni pełny") : TEXT("pełny stos rudy"));
			return;
		}
		Krok = EKrok::DoCelu;
		IdzDo(B->ZUkladu(R.Stanowisko + FVector2D(FMath::FRandRange(-280.f, 280.f), FMath::FRandRange(-10.f, 30.f))));
		Status = TEXT("idzie pod skałę");
		return;
	case EKrok::DoCelu:
		Krok = EKrok::Praca;
		Pracuj(FMath::FRandRange(5.5f, 7.5f), FVector2D(GetActorLocation()) + DoSkaly, TEXT("SM_Ludz_Kilof"), *FString::Printf(TEXT("kuje %s"), Biernik(W)));
		return;
	case EKrok::Praca:
		Mlotek->SetVisibility(false);
		Krok = EKrok::Bierze;
		Schyl(EStan::Bierze, 1.2f, FVector2D(GetActorLocation()) + DoSkaly);
		return;
	case EKrok::Bierze:
		Pokaz(NiesionySurowiec(W));
		Krok = EKrok::Niesie;
		IdzDo(B->PrzyStosie(W));
		Status = FString::Printf(TEXT("niesie %s"), Biernik(W));
		return;
	case EKrok::Niesie:
		Krok = EKrok::Odklada;
		Schyl(EStan::Odklada, 1.f, B->ZUkladu(R.Stosy[int32(W)]));
		return;
	case EKrok::Odklada:
		B->ZmienZapas(W, 1);
		Pokaz(nullptr);
		Krok = EKrok::Start;
		ZadanieKamieniarza();
		return;
	default:
		Krok = EKrok::Start;
		Czekaj(1.f, TEXT("odpoczywa"));
		return;
	}
}

void ALudzik::ZadanieZbieracza()
{
	ABudowa* B = Budowa.Get();
	if (!B || !Osada.IsValid())
	{
		KoniecPracy(false);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	switch (Krok)
	{
	case EKrok::Start:
	{
		if (Osada->Pora() == EPora::Zima)
		{
			Bezrobotny(8.f, TEXT("zima: krzaki bez owoców"));
			return;
		}
		if (B->Zapas[ESurowiec::Jedzenie] >= R.Pojemnosc)
		{
			Bezrobotny(4.f, TEXT("kosze pełne"));
			return;
		}
		FVector2D K;
		// Gdy krzaki w zasiegu obrane albo ich brak - szuka coraz dalej (DalszeZasiegi).
		int32 Dalej = 0;
		while (Dalej < IleZasiegow && !Osada->ZarezerwujKrzak(FVector2D(B->GetActorLocation()), R.Zasieg * DalszeZasiegi[Dalej], K))
		{
			++Dalej;
		}
		if (Dalej >= IleZasiegow)
		{
			Bezrobotny(6.f, TEXT("krzaki obrane, czeka aż odrosną"));
			return;
		}
		Drzewo = K;
		bRezKrzaka = true;
		Krok = EKrok::DoCelu;
		const FVector2D Od = (FVector2D(GetActorLocation()) - K).GetSafeNormal();
		IdzDo(K + (Od.IsNearlyZero() ? FVector2D(1.f, 0.f) : Od) * 90.f);
		Status = TEXT("idzie do krzaków");
		return;
	}
	case EKrok::DoCelu:
		Krok = EKrok::Praca;
		Schyl(EStan::Zbiera, FMath::FRandRange(4.f, 5.5f), Drzewo);
		Status = FMath::RandBool() ? TEXT("zbiera jagody") : TEXT("zbiera grzyby");
		return;
	case EKrok::Praca:
		Osada->ZerwijKrzak(Drzewo);
		bRezKrzaka = false;
		Pokaz(TEXT("SM_Ludz_Kosz"));
		Krok = EKrok::Niesie;
		IdzDo(B->PrzyStosie(ESurowiec::Jedzenie));
		Status = TEXT("niesie kosz jagód");
		return;
	case EKrok::Niesie:
		Krok = EKrok::Odklada;
		Schyl(EStan::Odklada, 1.f, B->ZUkladu(R.Stosy[int32(ESurowiec::Jedzenie)]));
		return;
	case EKrok::Odklada:
		B->ZmienZapas(ESurowiec::Jedzenie, FMath::Clamp(R.Pojemnosc - B->Zapas[ESurowiec::Jedzenie], 1, 2)); // pelny kosz = 2 porcje
		Pokaz(nullptr);
		Krok = EKrok::Start;
		ZadanieZbieracza();
		return;
	default:
		Krok = EKrok::Start;
		Czekaj(1.f, TEXT("odpoczywa"));
		return;
	}
}

void ALudzik::ZadanieRybaka()
{
	// Paczka A (23.11.2026): idzie od tylu chaty w strone rzeki (R.Woda) do ostatniego suchego miejsca, lowi z brzegu,
	// co druga-trzecia proba ryba; odnosi je na stos przy chacie.
	ABudowa* B = Budowa.Get();
	if (!B || !Osada.IsValid())
	{
		KoniecPracy(false);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	switch (Krok)
	{
	case EKrok::Start:
	{
		if (B->Zapas[ESurowiec::Ryby] >= R.Pojemnosc)
		{
			Bezrobotny(4.f, TEXT("kosze pełne"));
			return;
		}
		const float Bok = bKobieta ? 160.f : -160.f;
		const FVector2D Od = B->ZUkladu(FVector2D(Bok, -300.f)), Do = B->ZUkladu(FVector2D(Bok, R.Woda.Y));
		FVector2D Brzeg = Od, Woda = Do;
		for (int32 k = 1; k <= 24; ++k)
		{
			const FVector2D P = FMath::Lerp(Od, Do, k / 24.f);
			if (FSwiat::Woda(P.X, P.Y) != FSwiat::EWoda::Lad)
			{
				Woda = P;
				break;
			}
			Brzeg = P;
		}
		Drzewo = Woda;
		Krok = EKrok::DoCelu;
		IdzDo(Brzeg);
		Status = TEXT("idzie nad rzekę");
		return;
	}
	case EKrok::DoCelu:
		Krok = EKrok::Praca;
		Pracuj(R.CzasPracy * FMath::FRandRange(0.8f, 1.2f), Drzewo, TEXT("SM_Ludz_Wedka"), TEXT("łowi ryby"));
		return;
	case EKrok::Praca:
		if (FMath::FRand() < (Osada->Pora() == EPora::Zima ? 0.25f : 0.45f))
		{
			Mlotek->SetVisibility(false);
			Pokaz(TEXT("SM_Ludz_Ryby"));
			Krok = EKrok::Niesie;
			IdzDo(B->PrzyStosie(ESurowiec::Ryby));
			Status = TEXT("niesie ryby");
			return;
		}
		Krok = EKrok::DoCelu; // nic nie bierze: zarzuca jeszcze raz
		NastepneZadanie();
		return;
	case EKrok::Niesie:
		Krok = EKrok::Odklada;
		Schyl(EStan::Odklada, 1.f, B->ZUkladu(R.Stosy[int32(ESurowiec::Ryby)]));
		return;
	case EKrok::Odklada:
		B->ZmienZapas(ESurowiec::Ryby, FMath::Clamp(R.Pojemnosc - B->Zapas[ESurowiec::Ryby], 1, 2));
		Pokaz(nullptr);
		Krok = EKrok::Start;
		ZadanieRybaka();
		return;
	default:
		Krok = EKrok::Start;
		Czekaj(1.f, TEXT("odpoczywa"));
		return;
	}
}

void ALudzik::ZadanieMysliwego()
{
	ABudowa* B = Budowa.Get();
	if (!B || !Osada.IsValid())
	{
		KoniecPracy(false);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	switch (Krok)
	{
	case EKrok::Start:
	{
		Mlotek->SetVisibility(false);
		if (B->Zapas[ESurowiec::Jedzenie] >= R.Pojemnosc)
		{
			Bezrobotny(4.f, TEXT("pełna spiżarnia"));
			return;
		}
		FVector2D P;
		int32 Las = 0;
		// Gdy w zasiegu nie ma lasu - idzie polowac dalej (DalszeZasiegi).
		int32 Dalej = 0;
		while (Dalej < IleZasiegow && !Osada->MiejscePolowania(FVector2D(B->GetActorLocation()), R.Zasieg * DalszeZasiegi[Dalej], P, Las))
		{
			++Dalej;
		}
		if (Dalej >= IleZasiegow)
		{
			Bezrobotny(6.f, TEXT("brak lasu w okolicy"));
			return;
		}
		Drzewo = P;
		// Szansa na trafienie rosnie z gestoscia lasu: 3 drzewa w 15 m -> 25%, 12 i wiecej -> 60%.
		Faza = 0.f;
		Szansa = FMath::Clamp(0.25f + 0.04f * (Las - 3), 0.25f, 0.6f);
		Krok = EKrok::DoCelu;
		IdzDo(P);
		Status = TEXT("idzie na polowanie");
		return;
	}
	case EKrok::DoCelu:
	{
		Krok = EKrok::Praca;
		const float Kat = FMath::FRand() * 2.f * PI;
		Pracuj(FMath::FRandRange(5.f, 8.f), FVector2D(GetActorLocation()) + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * 500.f, TEXT("SM_Ludz_Luk"), TEXT("poluje"));
		return;
	}
	case EKrok::Praca:
		Mlotek->SetVisibility(false);
		if (FMath::FRand() >= Szansa)
		{
			Krok = EKrok::Start;
			Czekaj(2.f, TEXT("zwierzyna uciekła"));
			return;
		}
		Krok = EKrok::Bierze;
		Schyl(EStan::Bierze, 2.f, FVector2D(GetActorLocation()) + Patrz * 300.f);
		Status = bKobieta ? TEXT("trafiła sarnę") : TEXT("trafił sarnę");
		return;
	case EKrok::Bierze:
		Pokaz(TEXT("SM_Ludz_Sarna"));
		Krok = EKrok::Niesie;
		IdzDo(B->PrzyStosie(ESurowiec::Jedzenie));
		Status = TEXT("niesie upolowaną sarnę");
		return;
	case EKrok::Niesie:
		Krok = EKrok::Odklada;
		Schyl(EStan::Odklada, 1.2f, B->ZUkladu(R.Stosy[int32(ESurowiec::Jedzenie)]));
		return;
	case EKrok::Odklada:
		B->ZmienZapas(ESurowiec::Jedzenie, 3); // sarna = 3 porcje miesa
		if (B->Zapas[ESurowiec::Skory] < 4) // stos skor przy chacie ma 4 poziomy
		{
			B->ZmienZapas(ESurowiec::Skory, 1); // paczka A krok 3: skora dla garbarza
		}
		Pokaz(nullptr);
		Krok = EKrok::Start;
		Czekaj(FMath::FRandRange(2.f, 4.f), TEXT("oprawia mięso"));
		return;
	default:
		Krok = EKrok::Start;
		Czekaj(1.f, TEXT("odpoczywa"));
		return;
	}
}

void ALudzik::ZadanieRolnika()
{
	ABudowa* B = Budowa.Get();
	if (!B || !Osada.IsValid())
	{
		KoniecPracy(false);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	const EPora P = Osada->Pora();
	// Losowe miejsce na uprawie (UprawaMin..Max w ukladzie pola; od frontu miedza ze snopami). Pole ma dowolna wielkosc (21.11.2026).
	auto NaPolu = [B, &R]()
	{
		return B->ZUkladu(FVector2D(FMath::FRandRange(R.UprawaMin.X + 40.f, R.UprawaMax.X - 40.f), FMath::FRandRange(R.UprawaMin.Y + 20.f, R.UprawaMax.Y - 20.f)));
	};
	switch (Krok)
	{
	case EKrok::Start:
		Mlotek->SetVisibility(false);
		Pokaz(nullptr);
		Robota = 0;
		if (P == EPora::Zima)
		{
			// Zima: pole lezy odlogiem, rodzina siedzi w domu.
			if (bMaDom && FVector2D::Distance(FVector2D(GetActorLocation()), Dom) > 450.f)
			{
				Krok = EKrok::Wraca;
				IdzDo(Dom + FVector2D(FMath::FRandRange(-150.f, 150.f), FMath::FRandRange(-150.f, 150.f)));
				Status = TEXT("wraca do domu na zimę");
				return;
			}
			Bezrobotny(8.f, TEXT("zimuje w domu"));
			return;
		}
		if (P == EPora::Wiosna && (B->Uprawa == EUprawa::Ugor || B->Uprawa == EUprawa::Zaorane))
		{
			Krok = EKrok::DoCelu;
			IdzDo(NaPolu());
			Status = B->Uprawa == EUprawa::Ugor ? TEXT("idzie orać") : TEXT("idzie siać");
			return;
		}
		if (P == EPora::Jesien && B->Uprawa == EUprawa::Dojrzale && B->DoZebrania > 0)
		{
			if (B->Zapas[ESurowiec::Zboze] >= R.Pojemnosc)
			{
				Bezrobotny(4.f, TEXT("stos snopów pełny"));
				return;
			}
			Krok = EKrok::DoCelu;
			IdzDo(NaPolu());
			Status = TEXT("idzie żąć");
			return;
		}
		if (P == EPora::Lato && (B->Uprawa == EUprawa::Rosnie || B->Uprawa == EUprawa::Zasiane))
		{
			Krok = EKrok::DoCelu;
			IdzDo(NaPolu());
			Status = TEXT("idzie pielić");
			return;
		}
		if (P == EPora::Wiosna)
		{
			Bezrobotny(6.f, TEXT("pole obsiane, czeka na lato"));
		}
		else if (B->Uprawa == EUprawa::Scierniska)
		{
			Bezrobotny(6.f, TEXT("pole zebrane, czeka na wiosnę"));
		}
		else
		{
			Bezrobotny(6.f, TEXT("pole nieobsiane, czeka na wiosnę"));
		}
		return;
	case EKrok::DoCelu:
		Krok = EKrok::Praca;
		if (P == EPora::Wiosna && B->Uprawa == EUprawa::Ugor)
		{
			Mlotek->SetStaticMesh(Model(TEXT("SM_Ludz_Motyka")));
			Mlotek->SetVisibility(true);
			CzasRoboty = FMath::FRandRange(6.f, 8.f);
			Schyl(EStan::Kopie, CzasRoboty, FVector2D(GetActorLocation()) + Patrz * 100.f);
			Status = TEXT("orze pole");
		}
		else if (P == EPora::Wiosna && B->Uprawa == EUprawa::Zaorane)
		{
			Robota = 1;
			CzasRoboty = FMath::FRandRange(5.f, 7.f);
			Pracuj(CzasRoboty, FVector2D(GetActorLocation()) + Patrz * 100.f, TEXT("SM_Ludz_Siewka"), TEXT("sieje zboże"));
		}
		else if (P == EPora::Jesien && B->Uprawa == EUprawa::Dojrzale && B->DoZebrania > 0)
		{
			Robota = 2;
			CzasRoboty = 0.f;
			Pracuj(FMath::FRandRange(6.f, 8.f), FVector2D(GetActorLocation()) + Patrz * 100.f, TEXT("SM_Ludz_Sierp"), TEXT("żnie zboże sierpem"));
		}
		else if (P == EPora::Lato)
		{
			CzasRoboty = 0.f;
			Schyl(EStan::Zbiera, FMath::FRandRange(6.f, 8.f), FVector2D(GetActorLocation()) + Patrz * 100.f);
			Status = TEXT("pieli grządki");
		}
		else
		{
			Krok = EKrok::Start;
			Czekaj(1.f, TEXT("odpoczywa"));
		}
		return;
	case EKrok::Praca:
		Mlotek->SetVisibility(false);
		if (Robota == 2)
		{
			Robota = 0;
			// Snop: tylko jesli ktos inny nie zzal juz ostatniego.
			if (B->Uprawa == EUprawa::Dojrzale && B->DoZebrania > 0)
			{
				if (--B->DoZebrania <= 0)
				{
					B->UstawUprawe(EUprawa::Scierniska);
				}
				Pokaz(NiesionySurowiec(ESurowiec::Zboze));
				Krok = EKrok::Niesie;
				IdzDo(B->PrzyStosie(ESurowiec::Zboze));
				Status = TEXT("niesie snop zboża");
				return;
			}
		}
		else if (CzasRoboty > 0.f)
		{
			Osada->PracujNaPolu(B, CzasRoboty);
		}
		Robota = 0;
		CzasRoboty = 0.f;
		Krok = EKrok::Start;
		Czekaj(FMath::FRandRange(0.5f, 1.5f), TEXT("prostuje plecy"));
		return;
	case EKrok::Niesie:
		Krok = EKrok::Odklada;
		Schyl(EStan::Odklada, 1.f, B->ZUkladu(R.Stosy[int32(ESurowiec::Zboze)]));
		return;
	case EKrok::Odklada:
		B->ZmienZapas(ESurowiec::Zboze, 1);
		Pokaz(nullptr);
		Krok = EKrok::Start;
		ZadanieRolnika();
		return;
	case EKrok::Wraca:
	default:
		Krok = EKrok::Start;
		Czekaj(P == EPora::Zima ? 8.f : 1.f, P == EPora::Zima ? TEXT("zimuje w domu") : TEXT("odpoczywa"));
		return;
	}
}

void ALudzik::ZadanieRzemieslnika()
{
	// Mlyn, piekarnia, tartak wodny, tkacz, kuznia: jak pilarz, ale wedlug rodzaju budynku (Wsad -> Uzysk sztuk Wyrobu
	// w CzasPracy). Kuznia ma drugi wsad (Wsad2: kloda na wegiel) - po sztuce obu na jedna przerobke.
	ABudowa* B = Budowa.Get();
	if (!B || !Osada.IsValid() || B->Rodzaj().Wsad == ESurowiec::Ile)
	{
		KoniecPracy(false);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	const ESurowiec W = R.Wsad;
	const ESurowiec W2 = R.Wsad2;
	const FVector2D Stan0 = R.Stanowisko + FVector2D(bKobieta ? 90.f : -90.f, 0.f);
	switch (Krok)
	{
	case EKrok::Start:
	{
		Mlotek->SetVisibility(false);
		if (B->Zapas[R.Wyrob] + R.Uzysk > R.Pojemnosc)
		{
			Bezrobotny(4.f, *FString::Printf(TEXT("pełny stos: %s"), *FString(NazwaSurowca(R.Wyrob)).ToLower()));
			return;
		}
		const bool bMa1 = B->Dostepne(W) > 0;
		const bool bMa2 = W2 == ESurowiec::Ile || B->Dostepne(W2) > 0;
		if (bMa1 && bMa2)
		{
			Zrodlo = B;
			Towar = W;
			B->Zarezerwowane[W] += 1;
			bRezZrodla = true;
			if (W2 != ESurowiec::Ile)
			{
				B->Zarezerwowane[W2] += 1;
				bRez2 = true;
			}
			Krok = EKrok::DoCelu;
			IdzDo(B->ZUkladu(Stan0));
			Status = TEXT("idzie do roboty");
			return;
		}
		// Brakuje wsadu (albo drugiego wsadu): przynosi go sam.
		const ESurowiec Brak = bMa1 ? W2 : W;
		if (B->Zapas[Brak] + B->WDrodze[Brak] >= 4)
		{
			Bezrobotny(3.f, *FString::Printf(TEXT("czeka na %s"), Dopelniacz(Brak)));
			return;
		}
		if (ABudowa* Z = Osada->ZrodloSurowca(Brak, FVector2D(B->GetActorLocation()), B))
		{
			Zrodlo = Z;
			Towar = Brak;
			Z->Zarezerwowane[Brak] += 1;
			B->WDrodze[Brak] += 1;
			bRezZrodla = bRezCelu = true;
			Krok = EKrok::PoTowar;
			IdzDo(Z->PrzyStosie(Brak));
			Status = FString::Printf(TEXT("idzie po %s"), Dopelniacz(Brak));
			return;
		}
		Bezrobotny(5.f, *FString::Printf(TEXT("brak %s"), Dopelniacz(Brak)));
		return;
	}
	case EKrok::PoTowar:
		Krok = EKrok::Bierze;
		Schyl(EStan::Bierze, 1.2f, Zrodlo.IsValid() ? Zrodlo->ZUkladu(Zrodlo->Rodzaj().Stosy[int32(Towar)]) : FVector2D(GetActorLocation()));
		return;
	case EKrok::Bierze:
		if (!WezZeZrodla())
		{
			Zwolnij();
			Krok = EKrok::Start;
			Bezrobotny(1.f, *FString::Printf(TEXT("czeka na %s"), Dopelniacz(Towar)));
			return;
		}
		Krok = EKrok::Niesie;
		IdzDo(B->PrzyStosie(Towar));
		Status = FString::Printf(TEXT("niesie %s"), Biernik(Towar));
		return;
	case EKrok::Niesie:
		Krok = EKrok::Odklada;
		Schyl(EStan::Odklada, 1.f, B->ZUkladu(R.Stosy[int32(Towar)]));
		return;
	case EKrok::Odklada:
		if (bTowarWRekach)
		{
			B->WDrodze[Towar] = FMath::Max(0, B->WDrodze[Towar] - 1);
			B->ZmienZapas(Towar, 1);
		}
		bTowarWRekach = bRezCelu = false;
		Zrodlo.Reset();
		Pokaz(nullptr);
		Krok = EKrok::Start;
		ZadanieRzemieslnika();
		return;
	case EKrok::DoCelu:
	{
		Krok = EKrok::Praca;
		const float T = R.CzasPracy * FMath::FRandRange(0.9f, 1.15f);
		Pracuj(T, FVector2D(B->GetActorLocation()), *R.Narzedzie, *R.CoRobi);
		B->Napedzaj(T + 1.f);
		return;
	}
	case EKrok::Praca:
	{
		Mlotek->SetVisibility(false);
		const bool bJest = bRezZrodla && B->Zapas[W] > 0 && (!bRez2 || B->Zapas[W2] > 0);
		if (bRezZrodla)
		{
			B->Zarezerwowane[W] = FMath::Max(0, B->Zarezerwowane[W] - 1);
		}
		if (bRez2)
		{
			B->Zarezerwowane[W2] = FMath::Max(0, B->Zarezerwowane[W2] - 1);
		}
		if (bJest)
		{
			B->ZmienZapas(W, -1);
			if (bRez2)
			{
				B->ZmienZapas(W2, -1);
			}
			B->ZmienZapas(R.Wyrob, R.Uzysk);
		}
		bRezZrodla = bRez2 = false;
		Zrodlo.Reset();
		Krok = EKrok::Start;
		ZadanieRzemieslnika();
		return;
	}
	default:
		Krok = EKrok::Start;
		Czekaj(1.f, TEXT("odpoczywa"));
		return;
	}
}

void ALudzik::ZadaniePasterza()
{
	// Owczarnia: wybiera owce z runem (staje, gdy pasterz do niej idzie), strzyze ja nozycami, welne niesie na stos
	// przy bramie. Gdy wszystkie ostrzyzone - chodzi po pastwisku i pilnuje stada, az runo odrosnie.
	ABudowa* B = Budowa.Get();
	if (!B || !Osada.IsValid())
	{
		KoniecPracy(false);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	const ESurowiec W = ESurowiec::Welna;
	switch (Krok)
	{
	case EKrok::Start:
		Mlotek->SetVisibility(false);
		Pokaz(nullptr);
		if (B->Zapas[W] >= R.Pojemnosc)
		{
			Bezrobotny(4.f, TEXT("pełny stos wełny"));
			return;
		}
		Owca = B->ZarezerwujOwce();
		if (Owca < 0)
		{
			Krok = EKrok::Wraca;
			IdzDo(B->ZUkladu(R.PunktPastwiska()));
			Status = TEXT("pilnuje owiec (runo odrasta)");
			return;
		}
		Krok = EKrok::DoCelu;
		IdzDo(B->GdzieOwca(Owca) + FVector2D(bKobieta ? 75.f : -75.f, 55.f));
		Status = TEXT("idzie do owcy");
		return;
	case EKrok::Wraca:
		Krok = EKrok::Start;
		Bezrobotny(FMath::FRandRange(3.f, 6.f), TEXT("pilnuje owiec (runo odrasta)"));
		return;
	case EKrok::DoCelu:
		Krok = EKrok::Praca;
		Pracuj(FMath::FRandRange(4.5f, 6.f), B->GdzieOwca(Owca), *R.Narzedzie, *R.CoRobi);
		return;
	case EKrok::Praca:
		Mlotek->SetVisibility(false);
		B->OstrzyzOwce(Owca);
		Owca = -1;
		Pokaz(NiesionySurowiec(W));
		Krok = EKrok::Niesie;
		IdzDo(B->PrzyStosie(W));
		Status = TEXT("niesie wełnę");
		return;
	case EKrok::Niesie:
		Krok = EKrok::Odklada;
		Schyl(EStan::Odklada, 1.f, B->ZUkladu(R.Stosy[int32(W)]));
		return;
	case EKrok::Odklada:
		B->ZmienZapas(W, 1);
		Pokaz(nullptr);
		Krok = EKrok::Start;
		ZadaniePasterza();
		return;
	default:
		Krok = EKrok::Start;
		Czekaj(1.f, TEXT("odpoczywa"));
		return;
	}
}

void ALudzik::ZdejmijZbroje()
{
	if (!bUzbrojony && !bPrzeszywanica)
	{
		return;
	}
	if (ABudowa* B = Budowa.Get())
	{
		// Wraca do zbrojowni, w ktorej je wzial (moze trafic do nastepnego woja).
		if (bUzbrojony)
		{
			B->ZmienZapas(ESurowiec::Bron, 1);
		}
		if (bPrzeszywanica)
		{
			B->ZmienZapas(ESurowiec::Sukno, 1);
		}
	}
	bUzbrojony = false;
	bPrzeszywanica = false;
	UstawKolory(KolorTuniki, KolorWlosow);
	Mlotek->SetVisibility(false);
}

void ALudzik::ZadanieWoja()
{
	// Zbrojownia: bierze wlocznie (ze zbrojowni, a gdy tam pusto - prosto z kuzni) i przeszywanice z sukna,
	// potem cwiczy pchniecia przy manekinach na placu przed zbrojownia, z przerwami.
	ABudowa* B = Budowa.Get();
	if (!B || !Osada.IsValid())
	{
		KoniecPracy(false);
		return;
	}
	const FRodzajBudynku& R = B->Rodzaj();
	const ESurowiec W = ESurowiec::Bron;
	switch (Krok)
	{
	case EKrok::Start:
		if (!bUzbrojony)
		{
			Mlotek->SetVisibility(false);
			ABudowa* Z = B->Dostepne(W) > 0 ? B : Osada->ZrodloSurowca(W, FVector2D(B->GetActorLocation()), nullptr);
			if (!Z)
			{
				// 23.11.2026: po rozwiazaniu oddzialu wlocznie moga przepasc (polegli gubia swoje) - wtedy czeka przy
				// zbrojowni, a nie tam, gdzie stal oddzial.
				const FVector2D Plac = B->ZUkladu(B->Rodzaj().Wejscie + FVector2D(FMath::FRandRange(-200.f, 200.f), 250.f));
				if (FVector2D::Distance(FVector2D(GetActorLocation()), Plac) > 600.f)
				{
					IdzDo(Plac);
					Status = TEXT("wraca do zbrojowni");
					return;
				}
				Czekaj(5.f, TEXT("czeka na włócznię"));
				return;
			}
			Zrodlo = Z;
			Towar = W;
			Z->Zarezerwowane[W] += 1;
			bRezZrodla = true;
			Krok = EKrok::PoTowar;
			IdzDo(Z->PrzyStosie(W));
			Status = TEXT("idzie po włócznię");
			return;
		}
		if (!bPrzeszywanica && B->Dostepne(ESurowiec::Sukno) > 0)
		{
			// Przeszywanica: pikowany kaftan z sukna, jasnobury (przykrywa kolor tuniki).
			B->ZmienZapas(ESurowiec::Sukno, -1);
			bPrzeszywanica = true;
			for (UStaticMeshComponent* C : { Tulow.Get(), Klatka.Get(), RekaL.Get(), RekaP.Get(), PrzedL.Get(), PrzedP.Get() })
			{
				C->SetCustomPrimitiveDataVector3(0, FVector(0.42f, 0.36f, 0.25f));
			}
		}
		Mlotek->SetStaticMesh(Model(TEXT("SM_Ludz_Wlocznia")));
		Mlotek->SetVisibility(true);
		Krok = EKrok::DoCelu;
		Drzewo = FVector2D(-260.f + 260.f * FMath::RandHelper(3), 820.f); // manekin (uklad budynku)
		IdzDo(B->ZUkladu(Drzewo + FVector2D(FMath::FRandRange(-60.f, 60.f), -FMath::FRandRange(120.f, 170.f))));
		Status = TEXT("idzie na plac ćwiczeń");
		return;
	case EKrok::PoTowar:
		Krok = EKrok::Bierze;
		Schyl(EStan::Bierze, 1.2f, Zrodlo.IsValid() ? Zrodlo->ZUkladu(Zrodlo->Rodzaj().Stosy[int32(W)]) : FVector2D(GetActorLocation()));
		return;
	case EKrok::Bierze:
		if (!WezZeZrodla())
		{
			Zwolnij();
			Krok = EKrok::Start;
			Czekaj(2.f, TEXT("czeka na włócznię"));
			return;
		}
		// Wlocznia zostaje u woja; gdy rodzina dostanie inna prace, oddaje ja do zbrojowni (ZdejmijZbroje).
		bTowarWRekach = false;
		Zrodlo.Reset();
		Pokaz(nullptr);
		bUzbrojony = true;
		Krok = EKrok::Start;
		ZadanieWoja();
		return;
	case EKrok::DoCelu:
		Krok = EKrok::Praca;
		Pracuj(FMath::FRandRange(8.f, 12.f), B->ZUkladu(Drzewo), TEXT("SM_Ludz_Wlocznia"), *R.CoRobi);
		return;
	case EKrok::Praca:
		Krok = EKrok::DoCelu;
		Czekaj(FMath::FRandRange(2.f, 4.f), TEXT("odpoczywa po ćwiczeniu"));
		return;
	default:
		Krok = EKrok::Start;
		Czekaj(1.f, TEXT("odpoczywa"));
		return;
	}
}

void ALudzik::WejdzDoBitwy(FLinearColor Tunika, FLinearColor Wlosy, const TCHAR* Bron)
{
	Zaladuj();
	if (Budowa.IsValid())
	{
		KoniecPracy(false);
	}
	bWalka = true;
	bKusza = FCString::Strstr(Bron, TEXT("Kusza")) != nullptr;
	bLuk = bKusza || FCString::Strstr(Bron, TEXT("Luk")) != nullptr;
	bTopor = !bLuk && FCString::Strstr(Bron, TEXT("Wlocznia")) == nullptr; // topor, miecz, maczuga, narzedzia: zamach znad glowy
	RodzajBroni = !bTopor ? 0 : FCString::Strstr(Bron, TEXT("Miecz")) ? 2 : 1;
	bFilm = false;
	bNapor = false;
	Ranny = Zgiecie = Przyklek = Wstrzas = 0.f;
	Mlotek->SetRelativeRotation(FRotator::ZeroRotator);
	Rola = ERolaLudzika::Woj;
	Krok = EKrok::Start;
	Status = TEXT("w bitwie");
	Pokaz(nullptr);
	UstawKolory(Tunika, Wlosy);
	Mlotek->SetStaticMesh(Model(Bron));
	Mlotek->SetVisibility(true);
	Stan = EStan::Stoi;
	CzasStanu = 0.f;
	Faza = FMath::FRandRange(0.f, 10.f);
	Rytm = FMath::FRandRange(0.8f, 1.25f);
}

void ALudzik::WalkaIdz(FVector2D Gdzie, float Szybkosc)
{
	if (Stan == EStan::Lezy)
	{
		return;
	}
	Predkosc = Szybkosc;
	IdzDo(Gdzie);
}

void ALudzik::WalkaStoj(FVector2D PatrzNa)
{
	if (Stan == EStan::Lezy)
	{
		return;
	}
	Stan = EStan::Stoi;
	CzasStanu = 0.f;
	const FVector2D D = (PatrzNa - FVector2D(GetActorLocation())).GetSafeNormal();
	if (!D.IsNearlyZero())
	{
		Patrz = D;
	}
}

void ALudzik::WalkaCios(FVector2D PatrzNa)
{
	if (Stan == EStan::Lezy)
	{
		return;
	}
	WalkaStoj(PatrzNa);
	Stan = EStan::Pracuje;
	CzasStanu = -0.5f;
	Faza = 0.f;
	bZLewej = !bZLewej && FMath::FRand() < 0.85f; // miecz: zwykle na zmiane z prawej i z lewej
}

void ALudzik::WalkaTrafiony(FVector2D Kier, bool bStrzala)
{
	const FVector2D Przod(FMath::Cos(FMath::DegreesToRadians(Kierunek)), FMath::Sin(FMath::DegreesToRadians(Kierunek)));
	const FVector2D Prawo(-Przod.Y, Przod.X);
	BokCiosu = FMath::Clamp(FVector2D::DotProduct(Kier, Prawo), -1.f, 1.f);
	if (bStrzala)
	{
		Zgiecie = 1.f;
		if (FMath::FRand() < 0.3f)
		{
			Przyklek = 1.f;
		}
		return;
	}
	Odrzut = 1.f;
}

void ALudzik::UstawRane(float R, bool bKrewNaTunice)
{
	Ranny = FMath::Clamp(R, 0.f, 1.f);
	// Krew na tunice: im ciezej ranny, tym ciemniejsza i bardziej czerwona.
	const FLinearColor K = FMath::Lerp(KolorTuniki, FLinearColor(0.2f, 0.018f, 0.012f), bKrewNaTunice ? 0.4f * Ranny : 0.f);
	for (UStaticMeshComponent* C : { Tulow.Get(), Klatka.Get(), RekaL.Get(), RekaP.Get(), PrzedL.Get(), PrzedP.Get() })
	{
		C->SetCustomPrimitiveDataVector3(0, FVector(K.R, K.G, K.B));
	}
}

void ALudzik::WbijStrzale(FVector2D Lot, bool bBelt, bool bWTarcze)
{
	if (Strzaly.Num() >= 5 || (bWTarcze && !TarczaC))
	{
		return;
	}
	USceneComponent* Rodzic = bWTarcze ? static_cast<USceneComponent*>(TarczaC) : bKobieta ? static_cast<USceneComponent*>(Tulow) : Klatka;
	UStaticMeshComponent* Pocisk = Dodatek(bBelt ? TEXT("SM_Ludz_Belt") : TEXT("SM_Ludz_Strzala"), Rodzic, FVector::ZeroVector);
	const FVector K = FVector(Lot, -0.3f).GetSafeNormal();
	// Miejsce wejscia: tarcza (lico) albo klatka (piers / bok), grot ok. 12 cm w srodku.
	const FVector Wej = bWTarcze
		? TarczaC->GetComponentLocation() - K * 6.f + FVector(FMath::FRandRange(-14.f, 14.f), FMath::FRandRange(-14.f, 14.f), FMath::FRandRange(-16.f, 16.f))
		: Rodzic->GetComponentTransform().TransformPosition(FVector(0.f, FMath::FRandRange(-12.f, 12.f), FMath::FRandRange(4.f, 30.f))) - K * 15.f;
	Pocisk->SetWorldLocationAndRotation(Wej - K * 44.f, K.Rotation());
	Pocisk->SetCastShadow(false);
	Strzaly.Add(Pocisk);
}

bool ALudzik::Kikut(FVector& Gdzie) const
{
	if (!bKikut)
	{
		return false;
	}
	Gdzie = Klatka->GetComponentTransform().TransformPosition(KikutLok);
	return true;
}

void ALudzik::WalkaStrzal(FVector2D PatrzNa)
{
	if (Stan == EStan::Lezy)
	{
		return;
	}
	WalkaStoj(PatrzNa);
	Stan = EStan::Pracuje;
	CzasStanu = bKusza ? -1.1f : -0.8f;
	Faza = 0.f;
}

bool ALudzik::MaTarcze() const
{
	return TarczaC && TarczaC->GetAttachParent() == PrzedL;
}

void ALudzik::ZabityWNapadzie(FVector2D Kier)
{
	if (Budowa.IsValid())
	{
		KoniecPracy(false);
	}
	bWalka = true; // osada nie daje mu juz pracy (jak polegly z ruszenia)
	WalkaPadnij(Kier, false, FMath::FRand() < 0.3f);
}

void ALudzik::WalkaPadnij(FVector2D Kier, bool bStrzala, bool bKona)
{
	Stan = EStan::Lezy;
	CzasStanu = 0.f;
	Opad = 0.f;
	FVector2D Przod = FVector2D(FMath::Cos(FMath::DegreesToRadians(Kierunek)), FMath::Sin(FMath::DegreesToRadians(Kierunek)));
	// Upadek w strone ciosu (22.11.2026): bity z tylu pada na twarz, z przodu na plecy. Strzala czesto rzuca na kolana,
	// mocny cios odrzuca do tylu albo obraca.
	Kier = Kier.GetSafeNormal();
	if (Kier.IsNearlyZero())
	{
		Kier = -Przod;
	}
	KierUpadku = Kier;
	PozUpadku = FVector2D(GetActorLocation());
	const float R = FMath::FRand();
	Upadek = bStrzala ? (R < 0.45f ? 1 : R < 0.75f ? 3 : 0) : (R < 0.2f ? 1 : R < 0.4f ? 2 : R < 0.55f ? 3 : 0);
	bWTyl = Upadek != 1 && FVector2D::DotProduct(Przod, Kier) < 0.3f;
	if (Upadek != 1)
	{
		// Pada wzdluz ciosu: na plecy = twarza do napastnika, na twarz = plecami do niego.
		Kierunek = FMath::RadiansToDegrees(FMath::Atan2(bWTyl ? -Kier.Y : Kier.Y, bWTyl ? -Kier.X : Kier.X));
	}
	ObrotUpadku = Upadek == 2 ? (FMath::RandBool() ? 75.f : -75.f) : 0.f;
	Kona = bKona ? FMath::FRandRange(4.f, 8.f) : 0.f;
	bFilm = false;
	bNapor = false;
	// Bron, tarcza i choragiew wypadaja z rak i zostaja na ziemi obok ciala.
	Przod = FVector2D(FMath::Cos(FMath::DegreesToRadians(Kierunek)), FMath::Sin(FMath::DegreesToRadians(Kierunek)));
	auto Wyrzut = [&](float Sila)
	{
		const FVector2D K = (Przod * FMath::FRandRange(-0.6f, 1.f) + FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f))).GetSafeNormal();
		return FVector(K * Sila, FMath::FRandRange(120.f, 260.f));
	};
	const float Yaw = FMath::FRandRange(0.f, 360.f);
	if (Mlotek->IsVisible())
	{
		Rzuc(Mlotek, Wyrzut(FMath::FRandRange(80.f, 220.f)), FRotator(0.f, Yaw, 90.f));
	}
	if (TarczaC)
	{
		Rzuc(TarczaC, Wyrzut(FMath::FRandRange(40.f, 140.f)), FRotator(FMath::RandBool() ? 90.f : -90.f, Yaw + 40.f, 0.f));
	}
	if (ChoragiewC)
	{
		Rzuc(ChoragiewC, Wyrzut(60.f), FRotator(-88.f, Yaw, 0.f));
	}
	Status = TEXT("polegl");
}

void ALudzik::DoRuszenia(const TCHAR* Bron)
{
	Zaladuj();
	PracaPrzed = Budowa;
	RolaPrzed = Rola;
	bZOsady = true;
	// Kazdy lapie to, czym pracuje (jak w pospolitym ruszeniu): drwal siekiere, kamieniarz kilof, borowy motyke...
	const TCHAR* Narzedzie = TEXT("SM_Ludz_Mlotek");
	switch (Rola)
	{
	case ERolaLudzika::Drwal: case ERolaLudzika::Pilarz: case ERolaLudzika::Mysliwy: Narzedzie = TEXT("SM_Ludz_Siekiera"); break;
	case ERolaLudzika::Kamieniarz: Narzedzie = TEXT("SM_Ludz_Kilof"); break;
	case ERolaLudzika::Borowy: case ERolaLudzika::Zbieracz: Narzedzie = TEXT("SM_Ludz_Motyka"); break;
	case ERolaLudzika::Rolnik: Narzedzie = TEXT("SM_Ludz_Sierp"); break;
	case ERolaLudzika::Woj: Narzedzie = TEXT("SM_Ludz_Wlocznia"); break;
	default: break;
	}
	WejdzDoBitwy(KolorTuniki, KolorWlosow, Bron ? Bron : Narzedzie); // wlocznia z zapasow, gdy byla
	Status = TEXT("pospolite ruszenie");
}

void ALudzik::WrocZBitwy()
{
	if (!bWalka || Stan == EStan::Lezy)
	{
		return;
	}
	bWalka = false;
	bLuk = bKusza = false;
	Mlotek->SetRelativeRotation(FRotator::ZeroRotator);
	for (TObjectPtr<UStaticMeshComponent>* C : { &TarczaC, &HelmC, &BrodaC, &ChoragiewC })
	{
		if (*C)
		{
			(*C)->DestroyComponent();
			*C = nullptr;
		}
	}
	for (UStaticMeshComponent* Sc : Strzaly)
	{
		if (Sc) { Sc->DestroyComponent(); }
	}
	Strzaly.Reset();
	Okrzyk = OkrzykZa = Zaslona = Odrzut = Wstrzas = Zgiecie = Przyklek = 0.f;
	bFilm = false;
	bNapor = false;
	Ranny = 0.f;
	UstawKolory(KolorTuniki, KolorWlosow);
	Glowa->SetRelativeRotation(FRotator::ZeroRotator);
	CustomTimeDilation = 1.f;
	Predkosc = 190.f;
	bZOsady = false;
	ABudowa* B = PracaPrzed.Get();
	Przydziel(B, B ? RolaPrzed : ERolaLudzika::Budowniczy, false);
	Status = B ? Status : FString(TEXT("wrócił z bitwy"));
	if (!B && !bMaDom && Osada.IsValid() && Osada->MaOboz())
	{
		// Bez pracy i domu: nie zostaje na polu bitwy, wraca do obozu (tam osada da mu budowe).
		IdzDo(Osada->Oboz() + FVector2D(FMath::FRandRange(-400.f, 400.f), FMath::FRandRange(-400.f, 400.f)));
	}
}

UStaticMeshComponent* ALudzik::Dodatek(const TCHAR* Nazwa, USceneComponent* Rodzic, FVector Staw)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Movable);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetGenerateOverlapEvents(false);
	C->SetStaticMesh(Model(Nazwa));
	C->SetupAttachment(Rodzic);
	C->SetRelativeLocation(Staw);
	C->RegisterComponent();
	return C;
}

void ALudzik::UbierzDoBitwy(int32 Helm, bool bBroda, bool bTarcza, bool bChoragiew, FLinearColor Barwa)
{
	Zaladuj();
	const FVector Tun(KolorTuniki.R, KolorTuniki.G, KolorTuniki.B);
	const FVector Wl(KolorWlosow.R, KolorWlosow.G, KolorWlosow.B);
	static const TCHAR* Helmy[] = { nullptr, TEXT("SM_Ludz_Helm"), TEXT("SM_Ludz_Czapka"), TEXT("SM_Ludz_Kaptur") };
	if (Helm > 0 && Helm < 4 && !HelmC)
	{
		HelmC = Dodatek(Helmy[Helm], Glowa, FVector::ZeroVector);
		HelmC->SetCustomPrimitiveDataVector3(0, Tun); // kaptur w kolorze tuniki
		HelmC->SetCustomPrimitiveDataVector3(3, Wl);
	}
	if (bBroda && !BrodaC)
	{
		BrodaC = Dodatek(TEXT("SM_Ludz_Broda"), Glowa, FVector::ZeroVector);
		BrodaC->SetCustomPrimitiveDataVector3(0, Tun);
		BrodaC->SetCustomPrimitiveDataVector3(3, Wl);
	}
	const FVector Rod(Barwa.R, Barwa.G, Barwa.B);
	if (bChoragiew && !ChoragiewC)
	{
		ChoragiewC = Dodatek(TEXT("SM_Ludz_Choragiew"), PrzedL, FVector(4.f, 0.f, -26.f));
		ChoragiewC->SetCustomPrimitiveDataVector3(0, Rod);
	}
	else if (bTarcza && !TarczaC)
	{
		TarczaC = Dodatek(TEXT("SM_Ludz_Tarcza"), PrzedL, FVector(0.f, -5.f, -12.f));
		TarczaC->SetCustomPrimitiveDataVector3(0, Rod);
	}
}

FVector ALudzik::Odetnij(int32 Co, FVector2D Od)
{
	UStaticMeshComponent* C = Co == 0 ? Glowa.Get() : Co == 1 ? RekaL.Get() : RekaP.Get();
	const FVector Ciecie = C->GetComponentLocation();
	KikutLok = C->GetRelativeLocation(); // staw na klatce
	bKikut = true;
	FVector2D K = (FVector2D(GetActorLocation()) - Od).GetSafeNormal();
	K = (K + FVector2D(FMath::FRandRange(-0.5f, 0.5f), FMath::FRandRange(-0.5f, 0.5f))).GetSafeNormal();
	Rzuc(C, FVector(K * FMath::FRandRange(150.f, 300.f), FMath::FRandRange(200.f, 380.f)),
		Co == 0 ? FRotator(0.f, FMath::FRandRange(0.f, 360.f), FMath::RandBool() ? 90.f : -90.f) : FRotator(-90.f, FMath::FRandRange(0.f, 360.f), 0.f));
	return Ciecie;
}

void ALudzik::Rzuc(USceneComponent* C, FVector V, FRotator NaZiemi)
{
	if (!C)
	{
		return;
	}
	C->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	FLot L;
	L.C = C;
	L.V = V;
	L.Wir = FRotator(FMath::FRandRange(-500.f, 500.f), FMath::FRandRange(-300.f, 300.f), FMath::FRandRange(-500.f, 500.f));
	L.NaZiemi = NaZiemi;
	Loty.Add(L);
}

void ALudzik::Leca(float Dt)
{
	// Proste rzuty: grawitacja, obrot w locie, na ziemi czesc kladzie sie plasko i juz sie nie rusza.
	for (FLot& L : Loty)
	{
		USceneComponent* C = L.C.Get();
		if (!C || L.bLezy)
		{
			continue;
		}
		L.V.Z -= 1500.f * Dt;
		FVector P = C->GetComponentLocation() + L.V * Dt;
		const float Ziemia = FSwiat::Wysokosc(P.X, P.Y) + 6.f;
		if (P.Z <= Ziemia)
		{
			P.Z = Ziemia;
			L.bLezy = true;
			C->SetWorldLocationAndRotation(P, L.NaZiemi);
			continue;
		}
		C->SetWorldLocationAndRotation(P, C->GetComponentRotation() + L.Wir * Dt);
	}
}

void ALudzik::Tick(float DtSwiata)
{
	Super::Tick(DtSwiata);
	// Osadnicy chodza i pracuja szybciej niz uplywa czas swiata (TempoOsadnikow); w bitwie czas zwykly.
	const float Dt = bWalka ? DtSwiata : DtSwiata * TempoOsadnikow;
	CzasStanu += Dt;
	FVector Poz = GetActorLocation();
	if (Loty.Num() > 0)
	{
		Leca(Dt);
	}

	if (Stan == EStan::Idzie)
	{
		FVector2D Do;
		bool bDoszedl = false;
		if (bPoObwodzie && Budowa.IsValid())
		{
			const float P = ObwodDlugosc();
			float D = FMath::Fmod(SCel - S, P);
			if (D > P * 0.5f) { D -= P; }
			if (D < -P * 0.5f) { D += P; }
			const float Odc = Predkosc * Dt;
			if (FMath::Abs(D) <= Odc) { S = SCel; bDoszedl = true; }
			else { S += FMath::Sign(D) * Odc; }
			S = FMath::Fmod(S + P, P);
			Do = ZBudynku(NaObwodzie(S));
		}
		else
		{
			const FVector2D Teraz(Poz);
			const ADrogi* Dr = ADrogi::Obecne();
			NaDrodze = !bWalka && Dr && Dr->NaDrodze(Teraz) ? ADrogi::Premia : 1.f;
			const FVector2D Wektor = Cel - Teraz;
			const float L = Wektor.Size();
			const float Odc = PredkoscChodu() * Dt;
			if (L <= Odc)
			{
				Do = Cel;
				if (TrasaI + 1 < Trasa.Num() && Cel.Equals(Trasa[TrasaI], 1.f))
				{
					Cel = Trasa[++TrasaI]; // nastepny punkt trasy po drodze
				}
				else
				{
					bDoszedl = true;
				}
			}
			else { Do = Teraz + Wektor / L * Odc; }
		}
		const FVector2D Ruch = Do - FVector2D(Poz);
		if (!Ruch.IsNearlyZero(0.01f))
		{
			Patrz = Ruch.GetSafeNormal();
		}
		Poz = FVector(Do, FSwiat::Wysokosc(Do.X, Do.Y));
		SetActorLocation(Poz);
		if (bDoszedl && bOdchodzi && !bWalka) // Poprawka 25.11.2026: w bitwie nie znika (odejdzie po powrocie)
		{
			Destroy(); // rodzina odeszla z osady
			return;
		}
		if (bDoszedl && bWalka)
		{
			Stan = EStan::Stoi; // w bitwie: czeka na nastepny rozkaz (ABitwa)
			CzasStanu = 0.f;
		}
		else if (bDoszedl)
		{
			if (!bPoObwodzie && Rola == ERolaLudzika::Budowniczy && Budowa.IsValid())
			{
				bPoObwodzie = true; // dotarl do obwodu
			}
			NastepneZadanie();
		}
	}
	else if (Stan == EStan::Pracuje && bWalka)
	{
		if (CzasStanu >= 0.f)
		{
			Stan = EStan::Stoi; // cios skonczony
			CzasStanu = 0.f;
		}
	}
	else if (Stan == EStan::Pracuje)
	{
		if (Rola == ERolaLudzika::Budowniczy)
		{
			// Budowa: praca idzie w postep; koniec, gdy gotowe albo zabraklo materialu.
			if (Budowa.IsValid())
			{
				Budowa->DodajPrace(Dt);
			}
			if (CzasStanu >= 0.f || !Budowa.IsValid() || Budowa->Gotowy() || Budowa->CzekaNaMaterial())
			{
				NastepneZadanie();
			}
		}
		else if (CzasStanu >= 0.f || !Budowa.IsValid())
		{
			NastepneZadanie();
		}
	}
	else if (Stan == EStan::Stoi && !bWalka)
	{
		if (Budowa.IsValid())
		{
			// Przy pracy: po przerwie sprawdza znowu (material, drzewa, miejsce na stosie).
			if (CzasStanu > Przerwa)
			{
				NastepneZadanie();
			}
		}
		else if (bMaDom && CzasStanu > Przerwa)
		{
			// Bez pracy (26.11.2026, prosba: "wiecej zycia, chodza kolo domow"): co kilka sekund przejscie przy domu (2-6 m od wejscia),
			// czasem spacer do innego budynku osady (sasiad, targ, kosciol) albo pogawedka z kims wolnym, i dluzszy odpoczynek.
			const float Kat = FMath::FRand() * 2.f * PI;
			FVector2D Cel2 = Dom + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * FMath::FRandRange(200.f, 600.f);
			const float Los = FMath::FRand();
			if (Los < 0.4f)
			{
				TArray<ABudowa*> Gotowe;
				for (TActorIterator<ABudowa> It(GetWorld()); It; ++It)
				{
					if (!It->Podglad() && It->Gotowy() && !It->Rodzaj().bUmocnienie && !It->Rodzaj().ZObszarem()) { Gotowe.Add(*It); }
				}
				if (Gotowe.Num() > 0)
				{
					ABudowa* Odwiedza = Gotowe[FMath::RandRange(0, Gotowe.Num() - 1)];
					Cel2 = Odwiedza->ZUkladu(Odwiedza->Rodzaj().Wejscie + FVector2D(FMath::FRandRange(-150.f, 150.f), 120.f + FMath::FRandRange(0.f, 200.f)));
				}
			}
			else if (Los < 0.6f)
			{
				// Stanie we dwoje: podchodzi do wolnego sasiada.
				for (TActorIterator<ALudzik> It(GetWorld()); It; ++It)
				{
					const ALudzik* Inny = *It;
					if (Inny != this && GetUniqueID() < Inny->GetUniqueID() && Inny->Wolny() && !Inny->bWalka && Inny->Stan == EStan::Stoi
						&& FVector2D::Distance(FVector2D(Inny->GetActorLocation()), FVector2D(GetActorLocation())) < 2500.f)
					{
						const FVector2D Dok = FVector2D(Inny->GetActorLocation());
						Cel2 = Dok + (FVector2D(GetActorLocation()) - Dok).GetSafeNormal() * 130.f;
						break;
					}
				}
			}
			Przerwa = FMath::FRandRange(3.f, 12.f);
			IdzDo(Cel2);
		}
	}
	else if (Stan == EStan::Bierze || Stan == EStan::Odklada || Stan == EStan::Kopie || Stan == EStan::Zbiera)
	{
		if (CzasStanu >= 0.f)
		{
			NastepneZadanie();
		}
	}

	if (Stan == EStan::Lezy)
	{
		// Polegly (22.11.2026): pada w strone ciosu (obrot calej postaci wokol stop). Upadek 1: najpierw osuwa sie na kolana,
		// potem na twarz; 2: obraca sie w locie; 3: cios odrzuca go o pol kroku. Konajacy rusza jeszcze reka i glowa.
		Opad = FMath::Min(1.f, Opad + Dt / (Upadek == 1 ? 1.5f : Upadek == 3 ? 0.4f : 0.48f));
		float Kl = 0.f, E;
		if (Upadek == 1)
		{
			Kl = FMath::SmoothStep(0.f, 0.35f, Opad);
			const float F = FMath::Clamp((Opad - 0.55f) / 0.45f, 0.f, 1.f);
			E = F * F; // z kolan leci na twarz coraz szybciej
		}
		else
		{
			E = 1.f - FMath::Square(1.f - Opad);
		}
		const float Kleka = Kl * (1.f - E);
		const FVector2D Gdzie = PozUpadku + KierUpadku * (Upadek == 3 ? 55.f * E : 0.f);
		SetActorLocationAndRotation(FVector(Gdzie, FSwiat::Wysokosc(Gdzie.X, Gdzie.Y) + 16.f * E - 62.f * Kleka),
			FRotator((bWTyl ? 82.f : -84.f) * E, Kierunek + ObrotUpadku * E, 0.f));
		float Ruch = 0.f;
		if (Opad >= 1.f && Kona > 0.f)
		{
			Kona -= Dt;
			Ruch = FMath::Clamp(Kona / 3.f, 0.f, 1.f) * FMath::Sin(CzasStanu * 2.3f);
		}
		if (RekaL->GetAttachParent()) { RekaL->SetRelativeRotation(FRotator(FMath::Lerp(25.f, 150.f, E), 0.f, 0.f)); } // odcieta reka lezy osobno
		if (RekaP->GetAttachParent()) { RekaP->SetRelativeRotation(FRotator(FMath::Lerp(20.f, 120.f, E) + 25.f * Ruch, 0.f, 0.f)); }
		PrzedL->SetRelativeRotation(FRotator(FMath::Lerp(30.f, 15.f, E), 0.f, 0.f));
		PrzedP->SetRelativeRotation(FRotator(FMath::Lerp(25.f, 35.f, E) + 30.f * FMath::Abs(Ruch), 0.f, 0.f));
		if (Glowa->GetAttachParent()) { Glowa->SetRelativeRotation(FRotator((bWTyl ? 1.f : -1.f) * 18.f * FMath::Abs(Ruch), 0.f, 0.f)); }
		NogaL->SetRelativeLocationAndRotation(FVector(0, -9, 90), FRotator(FMath::Lerp(4.f * Kleka, -6.f, E), 0.f, 0.f));
		NogaP->SetRelativeLocationAndRotation(FVector(0, 9, 90), FRotator(FMath::Lerp(-4.f * Kleka, 8.f, E), 0.f, 0.f));
		LydkaL->SetRelativeRotation(FRotator(FMath::Lerp(-95.f * Kl, -10.f, E), 0.f, 0.f));
		LydkaP->SetRelativeRotation(FRotator(FMath::Lerp(-95.f * Kl, -25.f, E), 0.f, 0.f));
		Tulow->SetRelativeLocationAndRotation(FVector(0, 0, 90.f), FRotator(-8.f * Kleka, 0.f, 0.f));
		Klatka->SetRelativeRotation(FRotator(-10.f * Kleka, 0.f, 0.f));
		return;
	}

	// Obrot w strone ruchu / pracy.
	const float CelKier = FMath::RadiansToDegrees(FMath::Atan2(Patrz.Y, Patrz.X));
	Kierunek = FMath::FixedTurn(Kierunek, CelKier, 540.f * Dt);
	SetActorRotation(FRotator(0.f, Kierunek, 0.f));
	// Oszczednosc (22.11.2026): poza kadrem animacja co 4. klatke, daleko od kamery co 2. (z zebranym czasem). Filmowe
	// pary zawsze co klatke.
	DtAnim += Dt;
	int32 Co = 1;
	if (!bFilm)
	{
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		const bool bDaleko = PC && PC->PlayerCameraManager
			&& FVector::DistSquared(PC->PlayerCameraManager->GetCameraLocation(), GetActorLocation()) > FMath::Square(18000.f);
		Co = !WasRecentlyRendered(0.25f) ? 4 : bDaleko ? 2 : 1;
	}
	if (++KlatkaAnim % Co == 0)
	{
		Animuj(DtAnim);
		DtAnim = 0.f;
	}
}

void ALudzik::Animuj(float Dt)
{
	// Dodatni pitch czesci = wiszaca reka/noga idzie do przodu.
	float NL = 0.f, NP = 0.f, RL = 4.f, RP = 4.f, Pochyl = 0.f, Podskok = 0.f, Bok = 0.f;
	// Rece z lokciem (22.11.2026): RL / RP to kierunek przedramienia (i narzedzia w dloni), jak dawniej kat calej reki.
	// LL / LP = zgiecie lokcia (< 0: samo z kata reki), YawL / YawP = przedramie w bok (+ do srodka u lewej, na zewnatrz
	// u prawej), SkretK = skret klatki (+ = prawy bark do tylu).
	float LL = -1.f, LP = -1.f, YawL = 0.f, YawP = 0.f, SkretK = 0.f;
	// Nadgarstek miecza (+ = sztych w gore): w postawie i marszu klinga prawie pionowo.
	float Nadg = 24.f;
	const bool bTarcza = TarczaC && TarczaC->GetAttachParent() == PrzedL;
	// Chod (21.11.2026): cykl dwoch krokow, kazda stopa stoi na ziemi (bez slizgania), potem przenoszona lukiem do przodu.
	// Czestotliwosc rosnie z predkoscia, w biegu stopa krocej na ziemi i wyzej w gorze. Rece wahadlowo przeciwnie do nog.
	const float VModel = PredkoscChodu() / Skala;                 // cm/s w skali modelu
	const bool bBiegnie = VModel > 230.f;
	const float Czest = (0.85f + VModel / 260.f) * Rytm;   // cykle na sekunde
	const float NaZiemi = bBiegnie ? 0.42f : 0.62f;        // czesc cyklu, gdy stopa stoi
	const float Wykrok = NaZiemi * VModel / Czest;         // droga stopy wzgledem biodra w czasie podparcia
	auto Stopa = [&](float P, float& X, float& Z)
	{
		if (P < NaZiemi)
		{
			const float U = P / NaZiemi;
			X = Wykrok * (0.5f - U);
			Z = U > 0.55f ? 10.f * FMath::Square((U - 0.55f) / 0.45f) : 0.f; // pod koniec pieta w gore, odbicie z palcow
		}
		else
		{
			const float W = (P - NaZiemi) / (1.f - NaZiemi);
			X = Wykrok * (-0.5f + 0.5f * (1.f - FMath::Cos(W * PI)));
			Z = (bBiegnie ? 20.f : 11.f) * FMath::Sin(W * PI) + 3.f * FMath::Sin(W * 2.f * PI) * (W < 0.5f ? 1.f : 0.f);
		}
	};
	float XL = 0.f, ZL = 0.f, XP = 0.f, ZP = 0.f;
	if (Stan == EStan::Idzie)
	{
		FazaChodu = FMath::Frac(FazaChodu + Dt * Czest);
		Faza += Dt * Czest;
	}
	Stopa(FazaChodu, XL, ZL);
	Stopa(FMath::Frac(FazaChodu + 0.5f), XP, ZP);
	const float Wahadlo = Wykrok > 1.f ? FMath::Clamp(XL / (Wykrok * 0.5f), -1.f, 1.f) : 0.f; // +1 = lewa stopa z przodu
	// Dzwiek pracy: uderzenie, gdy cykl narzedzia sie domyka (Faza przechodzi przez pelna liczbe w galeziach nizej).
	const float FazaPrzed = FMath::Frac(Faza);
	FName Stuk = NAME_None;
	if (Stan == EStan::Idzie)
	{
		const float Sn = Wahadlo;
		if (bNiesie) { RL = RP = 62.f; Pochyl = 4.f; }
		else if (bWalka)
		{
			// Marsz / bieg z bronia: wlocznia nisko przed soba, topor uniesiony; w biegu pochylony do przodu.
			RP = (bTopor ? 70.f : 34.f) + 8.f * Sn;
			RL = 40.f - 20.f * Sn;
			YawL = bTarcza ? 14.f : 0.f; // tarcza przy ciele
			Pochyl = bBiegnie ? -15.f : -5.f;
		}
		else
		{
			const float Mach = bBiegnie ? 40.f : 24.f;
			RL = 5.f - Mach * Sn;
			RP = 5.f + Mach * Sn;
			Pochyl = bBiegnie ? -12.f : -3.f;
		}
		Bok = 2.2f * FMath::Sin(FazaChodu * 2.f * PI); // ciezar na nodze, ktora stoi
	}
	else if (Stan == EStan::Pracuje && Rola == ERolaLudzika::Pilarz)
	{
		// Pila: rowne posuwy w przod i w tyl, pochylony nad klodą.
		Faza += Dt * 1.6f;
		Stuk = "Pila";
		const float Sn = FMath::Sin(Faza * 2.f * PI);
		RP = 60.f + 22.f * Sn;
		RL = 55.f + 22.f * Sn;
		Pochyl = -14.f + 3.f * Sn;
	}
	else if (Stan == EStan::Pracuje && Rola == ERolaLudzika::Drwal)
	{
		// Siekiera: szeroki zamach znad barku, szybkie ciecie w bok pnia.
		Faza += Dt * 0.9f;
		Stuk = "Siekiera";
		const float F = FMath::Frac(Faza);
		const float Kat = F < 0.75f ? FMath::Lerp(40.f, 165.f, F / 0.75f) : FMath::Lerp(165.f, 40.f, (F - 0.75f) / 0.25f);
		RP = Kat;
		RL = Kat * 0.8f;
		Pochyl = F < 0.75f ? -2.f : -12.f;
	}
	else if (Stan == EStan::Pracuje && Rola == ERolaLudzika::Kamieniarz)
	{
		// Kilof: zamach znad glowy, uderzenie w dol w lico skaly.
		Faza += Dt * 0.8f;
		Stuk = "Kilof";
		const float F = FMath::Frac(Faza);
		const float Kat = F < 0.7f ? FMath::Lerp(50.f, 170.f, F / 0.7f) : FMath::Lerp(170.f, 50.f, (F - 0.7f) / 0.3f);
		RP = Kat;
		RL = Kat * 0.9f;
		Pochyl = F < 0.7f ? 0.f : -16.f;
	}
	else if (Stan == EStan::Pracuje && Rola == ERolaLudzika::Rybak)
	{
		// Wedka: obie rece z przodu trzymaja kij, co kilka sekund lekkie podciagniecie (zarzucenie).
		Faza += Dt * 0.25f;
		const float F = FMath::Frac(Faza);
		RP = 55.f + (F > 0.85f ? 35.f * FMath::Sin((F - 0.85f) / 0.15f * PI) : 0.f);
		RL = 50.f;
		Pochyl = -2.f;
	}
	else if (Stan == EStan::Pracuje && Rola == ERolaLudzika::Mysliwy)
	{
		// Luk: reka z lukiem wyciagnieta do przodu, druga naciaga cieciwe; czeka na zwierzyne.
		Faza += Dt * 0.5f;
		const float F = FMath::Frac(Faza);
		RP = 88.f;
		RL = 80.f - 25.f * FMath::Clamp(F * 3.f, 0.f, 1.f) * (F < 0.8f ? 1.f : 0.f);
		Pochyl = -3.f;
	}
	else if (Stan == EStan::Pracuje && Rola == ERolaLudzika::Rolnik && Robota == 1)
	{
		// Siew: idzie wolno do przodu (stoi w miejscu), prawa reka szerokim lukiem rzuca ziarno.
		Faza += Dt * 0.9f;
		const float Sn = FMath::Sin(Faza * 2.f * PI);
		RP = 50.f + 35.f * Sn;
		RL = 40.f;
		Pochyl = -4.f;
	}
	else if (Stan == EStan::Pracuje && Rola == ERolaLudzika::Rolnik && Robota == 2)
	{
		// Zniwa: pochylony, sierp szybko w dol i do siebie, lewa reka chwyta zdzbla.
		Faza += Dt * 1.4f;
		const float F = FMath::Frac(Faza);
		RP = F < 0.6f ? FMath::Lerp(30.f, 85.f, F / 0.6f) : FMath::Lerp(85.f, 30.f, (F - 0.6f) / 0.4f);
		RL = 60.f;
		Pochyl = -30.f;
	}
	else if (Stan == EStan::Pracuje && Rola == ERolaLudzika::Rzemieslnik && !Mlotek->IsVisible())
	{
		// Mlyn / piekarnia / tartak / krosno: obie rece przed soba (sypie ziarno, wyrabia ciasto, popycha klode).
		Faza += Dt * 1.2f;
		const float Sn = FMath::Sin(Faza * 2.f * PI);
		RP = 58.f + 14.f * Sn;
		RL = 58.f - 14.f * Sn;
		Pochyl = -12.f + 4.f * Sn;
	}
	else if (Stan == EStan::Pracuje && Rola == ERolaLudzika::Pasterz)
	{
		// Strzyzenie: pochylony nad owca, nozyce szybko kłapią, druga reka przytrzymuje runo.
		Faza += Dt * 2.4f;
		const float Sn = FMath::Sin(Faza * 2.f * PI);
		RP = 62.f + 10.f * Sn;
		RL = 52.f - 5.f * Sn;
		Pochyl = -27.f + 2.f * Sn;
	}
	else if (Stan == EStan::Pracuje && bWalka && bLuk)
	{
		// Strzal: luk w gore przed siebie, druga reka naciaga cieciwe do twarzy, puszcza; kusza: obie rece przy ramieniu,
		// po strzale odrzut. P: 0 = poczatek, 1 = koniec (CzasStanu od -0,8 / -1,1 do 0).
		const float Dl = bKusza ? 1.1f : 0.8f;
		const float P = FMath::Clamp(1.f + CzasStanu / Dl, 0.f, 1.f);
		const float Uniesienie = FMath::SmoothStep(0.f, 0.3f, P) * (1.f - FMath::SmoothStep(0.85f, 1.f, P));
		if (bKusza)
		{
			RP = FMath::Lerp(25.f, 80.f, Uniesienie);
			RL = FMath::Lerp(20.f, 74.f, Uniesienie);
			const float Kop = P > 0.6f ? FMath::Sin(FMath::Clamp((P - 0.6f) / 0.25f, 0.f, 1.f) * PI) : 0.f; // odrzut kuszy
			Pochyl = -4.f + 7.f * Kop;
			RP += 8.f * Kop;
		}
		else
		{
			const float Naciag = FMath::SmoothStep(0.25f, 0.65f, P) * (P < 0.75f ? 1.f : 0.f); // puszcza w 0,75
			RP = FMath::Lerp(25.f, 88.f, Uniesienie);
			LP = 5.f; // reka z lukiem wyprostowana
			// Druga reka: ramie w gore przed siebie, lokiec zgina sie mocno, dlon cofa sie z cieciwa do twarzy.
			LL = FMath::Lerp(15.f, 135.f, Naciag);
			RL = FMath::Lerp(20.f, 86.f, Uniesienie) + LL;
			SkretK = -12.f * Naciag; // bark reki z lukiem w strone celu
			Pochyl = -3.f + 2.f * Naciag;
		}
		NL = 10.f;
		NP = -12.f;
	}
	else if (Stan == EStan::Pracuje && bWalka && RodzajBroni == 2)
	{
		// Miecz: na zmiane ciecie z prawej (zamach za bark, klatka skrecona) i z lewej (zamach przez cialo), z wypadem.
		// Uderzenie w F = 0,52 (ALudzik::DoUderzenia przy tempie 2).
		Faza += Dt * 2.f;
		const float F = FMath::Min(FMath::Frac(Faza), 0.999f);
		const float Str = bZLewej ? -1.f : 1.f;
		float Luk; // +1 = bron po stronie zamachu, -1 = po drugiej stronie ciala
		if (F < 0.42f)
		{
			const float Z = FMath::SmoothStep(0.f, 1.f, F / 0.42f);
			RP = FMath::Lerp(62.f, 125.f, Z);
			LP = FMath::Lerp(25.f, 72.f, Z);
			Nadg = FMath::Lerp(24.f, 45.f, Z); // klinga za bark
			Luk = Z;
		}
		else if (F < 0.62f)
		{
			const float C = (F - 0.42f) / 0.2f;
			RP = FMath::Lerp(125.f, 42.f, C);
			LP = FMath::Lerp(72.f, 5.f, C);
			Nadg = FMath::Lerp(45.f, -5.f, C); // nadgarstek prowadzi ciecie
			Luk = 1.f - 2.f * C;
		}
		else
		{
			const float W = FMath::SmoothStep(0.f, 1.f, (F - 0.62f) / 0.38f);
			RP = FMath::Lerp(42.f, 62.f, W);
			LP = FMath::Lerp(5.f, 25.f, W);
			Nadg = FMath::Lerp(-5.f, 24.f, W);
			Luk = -1.f + W;
		}
		YawP = Str * 38.f * Luk;
		SkretK = Str * 24.f * Luk;
		RL = 42.f;
		YawL = bTarcza ? 16.f : 0.f;
		const float Wypad = F >= 0.42f && F < 0.85f ? 1.f : 0.f;
		Pochyl = F < 0.42f ? 3.f : -12.f * Wypad;
		NL = 22.f * Wypad;
		NP = -10.f * Wypad;
	}
	else if (Stan == EStan::Pracuje && bWalka && bTopor)
	{
		// Topor / maczuga w bitwie: zamach znad glowy ze zgietym lokciem i bark do tylu, ciecie w dol z wypadem
		// (lokiec prostuje sie jak bicz), powrot.
		Faza += Dt * 2.f;
		const float F = FMath::Min(FMath::Frac(Faza), 0.999f);
		const float Kat = F < 0.45f ? FMath::Lerp(70.f, 175.f, F / 0.45f) : F < 0.6f ? FMath::Lerp(175.f, 35.f, (F - 0.45f) / 0.15f) : FMath::Lerp(35.f, 70.f, (F - 0.6f) / 0.4f);
		LP = F < 0.45f ? FMath::Lerp(25.f, 68.f, F / 0.45f) : F < 0.6f ? FMath::Lerp(68.f, 4.f, (F - 0.45f) / 0.15f) : FMath::Lerp(4.f, 25.f, (F - 0.6f) / 0.4f);
		SkretK = F < 0.45f ? 16.f * F / 0.45f : F < 0.6f ? FMath::Lerp(16.f, -18.f, (F - 0.45f) / 0.15f) : FMath::Lerp(-18.f, 0.f, (F - 0.6f) / 0.4f);
		const float Wypad = F >= 0.45f && F < 0.8f ? 1.f : 0.f;
		RP = Kat;
		RL = 30.f + 20.f * Wypad;
		YawL = bTarcza ? 16.f : 0.f;
		Pochyl = F < 0.45f ? 4.f : -16.f * Wypad;
		NL = 22.f * Wypad;
		NP = -10.f * Wypad;
	}
	else if (Stan == EStan::Stoi && bWalka)
	{
		// Postawa bojowa: lekko w przysiadzie, bron przed soba, nogi w rozkroku, oddech (kazdy we wlasnym rytmie).
		Faza += Dt * 1.3f * Rytm;
		const float Sn = FMath::Sin(Faza * 2.f * PI);
		RP = (bLuk ? 22.f : RodzajBroni == 2 ? 62.f : bTopor ? 72.f : 30.f) + 3.f * Sn;
		RL = (bLuk ? 18.f : 48.f) + 3.f * Sn;
		YawL = bTarcza ? 16.f : 0.f; // tarcza przed soba
		Pochyl = -8.f + 1.5f * Sn;
		NL = 14.f;
		NP = -10.f;
		Podskok = -4.f + 1.f * Sn;
		// Napor tylnego szeregu: pochylony, tarcza wysunieta na plecy woja przed nim, noga zaparta z tylu, co chwile pchniecie.
		NaporW = FMath::FInterpTo(NaporW, bNapor && !bLuk ? 1.f : 0.f, Dt, 3.f);
		if (NaporW > 0.01f)
		{
			const float N = NaporW;
			const float Pchn = 0.5f + 0.5f * FMath::Sin(Faza * PI); // pchniecie co ~1.5 s
			RL = FMath::Lerp(RL, 76.f + 6.f * Pchn, N);
			YawL = FMath::Lerp(YawL, bTarcza ? 6.f : 0.f, N);
			if (RodzajBroni == 0) { RP = FMath::Lerp(RP, 56.f + 3.f * Sn, N); } // wlocznia nad ramieniem pierwszego
			Pochyl = FMath::Lerp(Pochyl, -15.f - 6.f * Pchn, N);
			NL = FMath::Lerp(NL, 20.f, N);
			NP = FMath::Lerp(NP, -22.f - 5.f * Pchn, N);
			Podskok = FMath::Lerp(Podskok, -7.f, N);
		}
	}
	else if (Stan == EStan::Pracuje && Rola == ERolaLudzika::Woj)
	{
		// Wlocznia: cofniecie, szybkie pchniecie z wypadem (lewa noga w przod), powrot do postawy. W bitwie szybciej.
		Faza += Dt * (bWalka ? 2.f : 0.9f);
		if (bWalka) { Faza = FMath::Min(Faza, 0.999f); } // jeden cios na WalkaCios
		const float F = FMath::Frac(Faza);
		float P; // -1 = cofnieta, 0 = postawa, 1 = pchniecie
		if (F < 0.45f) { P = -FMath::Sin(F / 0.45f * HALF_PI); }
		else if (F < 0.6f) { P = FMath::Lerp(-1.f, 1.f, (F - 0.45f) / 0.15f); }
		else { P = FMath::Lerp(1.f, 0.f, (F - 0.6f) / 0.4f); }
		const float Wypad = FMath::Max(0.f, P);
		RP = 8.f + 24.f * P;
		RL = 38.f + 16.f * P;
		// Lokiec: cofniety zgiety, pchniecie prostuje reke; bark idzie z pchnieciem do przodu.
		LP = FMath::Lerp(24.f, 4.f, (P + 1.f) * 0.5f);
		SkretK = -16.f * Wypad + 10.f * FMath::Max(0.f, -P);
		YawL = bTarcza ? 16.f : 0.f;
		Pochyl = -4.f - 12.f * Wypad;
		NL = 24.f * Wypad;
		NP = -12.f * Wypad;
	}
	else if (Stan == EStan::Zbiera)
	{
		// Zbieranie: pochylony nad krzakiem, rece na zmiane siegaja po jagody.
		Faza += Dt * 1.6f;
		const float Sn = FMath::Sin(Faza * 2.f * PI);
		RP = 55.f + 18.f * Sn;
		RL = 55.f - 18.f * Sn;
		Pochyl = -28.f;
	}
	else if (Stan == EStan::Pracuje)
	{
		// Mlotek: powolny zamach w gore, szybkie uderzenie.
		Faza += Dt * 1.3f;
		Stuk = Rola == ERolaLudzika::Rzemieslnik ? FName("Kowal") : FName("Mlotek"); // rzemieslnik z mlotkiem = kuznia
		const float F = FMath::Frac(Faza);
		const float Kat = F < 0.7f ? FMath::Lerp(55.f, 150.f, F / 0.7f) : FMath::Lerp(150.f, 55.f, (F - 0.7f) / 0.3f);
		RP = Kat;
		RL = 25.f;
		Pochyl = -6.f;
	}
	else if (Stan == EStan::Kopie)
	{
		// Sadzenie: pochylony, motyka w obu rekach w gore i w dol.
		Faza += Dt * 1.1f;
		const float Sn = FMath::Abs(FMath::Sin(Faza * PI));
		RP = RL = 50.f + 45.f * Sn;
		Pochyl = -22.f - 10.f * Sn;
	}
	else if (Stan == EStan::Bierze || Stan == EStan::Odklada)
	{
		// Schyla sie po kamien / odklada go.
		const float T = FMath::Clamp(1.f + CzasStanu, 0.f, 1.f);
		const float Schyl = FMath::Sin(T * PI);
		Pochyl = -35.f * Schyl;
		RL = RP = 40.f + 30.f * Schyl;
	}
	else
	{
		Faza += Dt;
		RL = RP = 4.f + 2.f * FMath::Sin(Faza * 1.7f);
	}
	if (!Stuk.IsNone() && FMath::Frac(Faza) < FazaPrzed)
	{
		UDzwieki::GrajPrace(this, Stuk, GetActorLocation() + FVector(0.f, 0.f, 100.f));
	}
	if (bWalka)
	{
		// Zycie w szeregu: okrzyki, rozgladanie sie, przenoszenie ciezaru z nogi na noge.
		float Glow = 0.f, Rozglad = 0.f;
		if (OkrzykZa > 0.f)
		{
			OkrzykZa -= Dt;
			if (OkrzykZa <= 0.f)
			{
				OkrzykZa = 0.f;
				Okrzyk = 1.f;
			}
		}
		if (Okrzyk > 0.f && Stan != EStan::Pracuje)
		{
			Okrzyk = FMath::Max(0.f, Okrzyk - Dt / 0.9f);
			const float Wz = FMath::Sin(Okrzyk * PI);
			RP = FMath::Lerp(RP, 165.f, Wz);
			Pochyl += 9.f * Wz;
			Podskok += 7.f * FMath::Abs(FMath::Sin(Okrzyk * 3.f * PI));
			Glow = -16.f * Wz;
		}
		if (Stan == EStan::Stoi)
		{
			const float Zerk = FMath::Max(0.f, FMath::Sin(Faza * 0.11f + Rytm * 5.f));
			Rozglad = 32.f * FMath::Sin(Faza * 0.23f + Rytm * 9.f) * Zerk;
			Bok = 3.5f * FMath::Sin(Faza * 0.4f);
			NL += 5.f * FMath::Sin(Faza * 0.4f);
			NP -= 5.f * FMath::Sin(Faza * 0.4f);
		}
		Glowa->SetRelativeRotation(FRotator(Glow, Rozglad - SkretK * 0.6f, 0.f)); // glowa patrzy na wroga mimo skretu klatki
	}
	if (bWalka && ChoragiewC)
	{
		// Choragiew: lewa reka trzyma drzewce przy sobie, w kazdym ruchu.
		RL = 22.f + 2.f * FMath::Sin(Faza * 1.3f);
	}
	else if (Zaslona > 0.f)
	{
		// Nadchodzacy cios: tarcza (lewa reka) szybko w gore i przez cialo przed twarz, lekki odchyl. Bez tarczy zbija
		// bronia: reka z bronia w gore na skos przed soba.
		Zaslona = FMath::Max(0.f, Zaslona - Dt / 0.45f);
		const float Z = FMath::Sin(Zaslona * PI);
		if (bTarcza || !bWalka)
		{
			RL = FMath::Lerp(RL, 92.f, Z);
			LL = FMath::Lerp(LL < 0.f ? 30.f : LL, 48.f, Z);
			YawL = FMath::Lerp(YawL, 30.f, Z);
		}
		else
		{
			RP = FMath::Lerp(RP, 105.f, Z);
			LP = FMath::Lerp(LP < 0.f ? 30.f : LP, 42.f, Z);
			YawP = FMath::Lerp(YawP, -34.f, Z);
		}
		Pochyl += 6.f * Z;
	}
	if (Wstrzas > 0.f)
	{
		// Cios na tarczy / broni: reka odskakuje, cialo drga do tylu (krotko, ok. 0,2 s).
		Wstrzas = FMath::Max(0.f, Wstrzas - Dt / 0.2f);
		const float W = FMath::Sin(Wstrzas * PI) * (0.6f + 0.4f * FMath::Sin(Wstrzas * 5.f * PI));
		RL -= 14.f * W;
		RP -= 8.f * W;
		Pochyl += 5.f * W;
		Podskok -= 2.f * W;
	}
	if (Odrzut > 0.f)
	{
		// Trafiony: zachwianie do tylu i w bok od ciosu, rece w gore, glowa odskakuje.
		Odrzut = FMath::Max(0.f, Odrzut - Dt / 0.45f);
		const float O = FMath::Sin(Odrzut * PI);
		Pochyl += 20.f * O;
		Bok += 9.f * BokCiosu * O;
		SkretK += 18.f * BokCiosu * O;
		RL += 25.f * O;
		RP += 15.f * O;
		NL -= 10.f * O;
		Podskok -= 3.f * O;
	}
	if (Zgiecie > 0.f)
	{
		// Strzala w ciele: zgina sie wpol i lapie wolna reka za rane, potem powoli sie prostuje.
		Zgiecie = FMath::Max(0.f, Zgiecie - Dt / 1.1f);
		const float G = FMath::Sin(FMath::Min(1.f, Zgiecie * 1.6f) * HALF_PI);
		Pochyl -= 26.f * G;
		if (!bTarcza && !ChoragiewC)
		{
			RL = FMath::Lerp(RL, 50.f, G);
			LL = FMath::Lerp(LL < 0.f ? 25.f : LL, 80.f, G);
			YawL = FMath::Lerp(YawL, 40.f, G);
		}
		RP = FMath::Lerp(RP, 20.f, 0.5f * G);
		Podskok -= 6.f * G;
	}
	if (Przyklek > 0.f)
	{
		// Na kolano po strzale i wstaje.
		Przyklek = FMath::Max(0.f, Przyklek - Dt / 1.8f);
		const float K = FMath::Sin(FMath::Min(1.f, Przyklek * 1.4f) * HALF_PI);
		Podskok -= 34.f * K;
		NL += 38.f * K;
		NP -= 22.f * K;
	}
	if (bWalka && Ranny > 0.f && Stan != EStan::Lezy)
	{
		// Ranny: garbi sie, kuleje (przysiada na prawej nodze), wolna reka przy boku.
		const float Kul = FMath::Max(0.f, FMath::Sin(FazaChodu * 2.f * PI + PI)) * (Stan == EStan::Idzie ? 1.f : 0.f);
		Pochyl -= 6.f * Ranny;
		Podskok -= 6.f * Ranny * Kul;
		Bok += 4.f * Ranny * Kul;
		if (!bTarcza && !ChoragiewC && !bLuk && Zgiecie <= 0.f)
		{
			RL = FMath::Lerp(RL, 30.f, Ranny);
			LL = 70.f * Ranny;
			YawL = 35.f * Ranny;
		}
	}
	// Nogi: postawa (katy NL / NP, stopy na ziemi) przechodzi plynnie w chod. Biodro obniza sie tyle, zeby stopa stojaca
	// siegala ziemi (rozkrok, wypad, przysiad zginaja kolana zamiast unosic stope).
	WagaChodu = FMath::FInterpTo(WagaChodu, Stan == EStan::Idzie ? 1.f : 0.f, Dt, 9.f);
	const float Zasieg = 89.f;
	auto Obnizenie = [&](float X) { return Zasieg - FMath::Sqrt(FMath::Max(0.f, Zasieg * Zasieg - X * X)); };
	const FVector2D PostL(90.f * FMath::Sin(FMath::DegreesToRadians(NL)), 0.f), PostP(90.f * FMath::Sin(FMath::DegreesToRadians(NP)), 0.f);
	const float HPost = 90.f + Podskok - FMath::Max(Obnizenie(PostL.X), Obnizenie(PostP.X)) - 1.f;
	float HChod = Zasieg + 0.5f; // stopa z tylu odbija sie z palcow (pieta w gorze), wiec biodro obniza tylko stopa z przodu
	if (ZL <= 0.f) { HChod = FMath::Min(HChod, Zasieg + 0.5f - Obnizenie(XL)); }
	if (ZP <= 0.f) { HChod = FMath::Min(HChod, Zasieg + 0.5f - Obnizenie(XP)); }
	HChod += Podskok;
	const float WC = WagaChodu;
	const float H = FMath::Lerp(HPost, HChod, WC);
	UstawNoge(NogaL, LydkaL, -9.f, H, FMath::Lerp(PostL, FVector2D(XL, ZL), WC));
	UstawNoge(NogaP, LydkaP, 9.f, H, FMath::Lerp(PostP, FVector2D(XP, ZP), WC));
	// Skret barkow przeciw biodrom i glowa patrzaca prosto przed siebie.
	const float Skret = 5.f * Wahadlo * WC;
	if (!bWalka && Glowa->GetAttachParent() == Klatka)
	{
		Glowa->SetRelativeRotation(FRotator(1.5f * FMath::Sin(FazaChodu * 4.f * PI) * WC, -Skret * 0.8f, 0.f));
	}
	// Lokiec sam z kata reki: reka nisko prawie prosta, im wyzej, tym mocniej zgiety (zamach znad glowy).
	auto Lokiec = [](float Kat) { return FMath::Clamp(8.f + 0.35f * Kat, 6.f, 58.f); };
	LL = LL < 0.f ? Lokiec(RL) : LL;
	LP = LP < 0.f ? Lokiec(RP) : LP;
	RekaL->SetRelativeRotation(FRotator(RL - LL, 0.f, 0.f));
	RekaP->SetRelativeRotation(FRotator(RP - LP, 0.f, 0.f));
	PrzedL->SetRelativeRotation(FRotator(LL, ChoragiewC ? 0.f : YawL, 0.f));
	PrzedP->SetRelativeRotation(FRotator(LP, bLuk ? 0.f : YawP, 0.f));
	if (bWalka && RodzajBroni == 2 && Mlotek->GetAttachParent() == PrzedP)
	{
		Mlotek->SetRelativeRotation(FRotator(Nadg, 0.f, 90.f)); // ostrzem (nie plazem) do wroga
	}
	// Luk zawsze pionowo, kusza zawsze poziomo do przodu (bron w dloni nie obraca sie z reka).
	if (bWalka && bLuk && Mlotek->GetAttachParent() == PrzedP)
	{
		Mlotek->SetRelativeRotation(FRotator(-RP - Pochyl, 0.f, 0.f));
	}
	// Tarcza i drzewce choragwi nie obracaja sie z reka: zostaja pionowo, licem do przodu (lekko z pochyleniem tulowia).
	if (TarczaC && TarczaC->GetAttachParent() == PrzedL)
	{
		TarczaC->SetWorldRotation(GetActorQuat() * FRotator(Pochyl + 4.f, 0.f, 0.f).Quaternion());
	}
	if (ChoragiewC && ChoragiewC->GetAttachParent() == PrzedL)
	{
		ChoragiewC->SetWorldRotation(GetActorQuat() * FRotator(3.f * FMath::Sin(Faza * 0.9f), 0.f, 2.f * FMath::Sin(Faza * 0.7f)).Quaternion());
	}
	// Pochylenie tulowia do przodu = ujemny pitch (gora tulowia idzie w +X). Biodra i klatka dziela pochylenie, klatka
	// skreca sie przeciw biodrom (chod) i z ciosem (SkretK). Suknia kobiety w calosci na biodrach.
	const float BokT = Bok * FMath::Max(WC, bWalka ? 1.f : 0.f);
	if (bKobieta)
	{
		Tulow->SetRelativeLocationAndRotation(FVector(0, 0, H), FRotator(Pochyl, Skret, BokT));
		Klatka->SetRelativeRotation(FRotator::ZeroRotator);
	}
	else
	{
		Tulow->SetRelativeLocationAndRotation(FVector(0, 0, H), FRotator(Pochyl * 0.4f, -Skret * 0.4f + SkretK * 0.25f, BokT));
		Klatka->SetRelativeRotation(FRotator(Pochyl * 0.6f, Skret * 1.4f + SkretK * 0.75f, 0.f));
	}
}

void ALudzik::UstawNoge(UStaticMeshComponent* Udo, UStaticMeshComponent* Lydka, float Y, float H, FVector2D Stopa)
{
	// Proste IK w plaszczyznie kroku: udo 44 cm, lydka 46 cm (skala modelu). Kolano zawsze do przodu.
	constexpr float U = 44.f, L = 46.f;
	const FVector2D Do(Stopa.X, Stopa.Y - H); // od biodra do stopy (Y = w gore)
	const float D = FMath::Clamp(Do.Size(), 10.f, U + L - 0.01f);
	const float Fi = FMath::Atan2(Do.X, -Do.Y);  // 0 = prosto w dol, + = do przodu
	const float Alfa = FMath::Acos(FMath::Clamp((U * U + D * D - L * L) / (2.f * U * D), -1.f, 1.f));
	const float Kolano = PI - FMath::Acos(FMath::Clamp((U * U + L * L - D * D) / (2.f * U * L), -1.f, 1.f));
	Udo->SetRelativeLocationAndRotation(FVector(0.f, Y, H), FRotator(FMath::RadiansToDegrees(Fi + Alfa), 0.f, 0.f));
	Lydka->SetRelativeRotation(FRotator(-FMath::RadiansToDegrees(Kolano), 0.f, 0.f));
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
// Omijanie budynkow: odcinek przez prostokat, obok niego i wzdluz boku (Tools\Testy.bat).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOminBudynkiTest, "grarts2d.Ludzie.Przecina", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FOminBudynkiTest::RunTest(const FString&)
{
	const FBox2D Bx(FVector2D(-100.0, -50.0), FVector2D(100.0, 50.0));
	float T = 0.f;
	TestTrue(TEXT("przez srodek"), Przecina(Bx, FVector2D(-300.0, 0.0), FVector2D(300.0, 0.0), T) && FMath::IsNearlyEqual(T, 1.f / 3.f, 0.01f));
	TestFalse(TEXT("obok"), Przecina(Bx, FVector2D(-300.0, 80.0), FVector2D(300.0, 80.0), T));
	TestFalse(TEXT("wzdluz boku"), Przecina(Bx, FVector2D(-300.0, 50.0), FVector2D(300.0, 50.0), T));
	TestFalse(TEXT("przed budynkiem"), Przecina(Bx, FVector2D(-300.0, 0.0), FVector2D(-150.0, 0.0), T));
	return true;
}
#endif
