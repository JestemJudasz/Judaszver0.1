// Plik: Source/grarts2d/Swiat/TerenKafle.cpp
#include "Swiat/TerenKafle.h"
#include "Swiat/SwiatWysokosci.h"
#include "Kamera/KameraMapy.h"
#include "Async/Async.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Detal tekstury: 1 jednostka UV0 = 4 m; okres 1024 jednostek (4096 m), zeby liczby UV byly male i ciagle miedzy kaflami. */
	constexpr double DetalCm = 400.0;
	constexpr double DetalOkresCm = DetalCm * 1024.0;

	/** Modele w ATerenKafle::Modele: gatunek * 2 + wariant. */
	enum EGatunek : int32 { Dab, Buk, Brzoza, Sosna, Swierk, Krzak, Kamien, Trawa, Gatunkow };
	using EP = FSwiat::EPokrycie;

	/** Szansa na drzewo / krzak w oczku, wg klasy pokrycia. */
	constexpr float GestDrzew[int32(EP::Ile)] = { 0.f, 0.03f, 0.06f, 0.8f, 0.85f, 0.82f, 0.35f, 0.25f, 0.03f, 0.f, 0.08f };
	constexpr float GestKrzakow[int32(EP::Ile)] = { 0.f, 0.04f, 0.12f, 0.08f, 0.05f, 0.07f, 0.2f, 0.2f, 0.03f, 0.f, 0.1f };
	/** Udzial gatunkow (dab, buk, brzoza, sosna, swierk) wg klasy pokrycia. */
	constexpr float Udzialy[int32(EP::Ile)][5] = {
		{ 0, 0, 0, 0, 0 },  // woda
		{ 5, 2, 3, 2, 0 },  // laka: samotne drzewa
		{ 1, 0, 4, 4, 1 },  // wrzosowisko
		{ 4, 4, 2, 0.5f, 0 }, // lisciasty
		{ 0, 0, 1, 5, 5 },  // iglasty
		{ 2, 2, 2, 3, 3 },  // mieszany
		{ 3, 1, 4, 3, 0 },  // rzadki / skraj
		{ 0, 0, 6, 3, 1 },  // mokradlo
		{ 0, 0, 1, 3, 3 },  // skaly
		{ 0, 0, 0, 0, 0 },  // snieg
		{ 0, 0, 6, 2, 2 },  // tundra
	};
	constexpr float UdzialyGory[5] = { 0, 0, 1, 2, 6 }; // powyzej 1100 m

	uint64 Mieszaj(uint64 X)
	{
		X ^= X >> 31; X *= 0xBF58476D1CE4E5B9ull;
		X ^= X >> 29; X *= 0x94D049BB133111EBull;
		return X ^ (X >> 32);
	}

	/** Wysokosc w punkcie kafla (Lx, Ly od rogu) na tych samych trojkatach co siatka (przekatna A-C jak w Zbuduj) i spadek. */
	float WysNaKaflu(const TArray<float>& H, int32 R, int32 K, double Krok, double Lx, double Ly, float& Spadek)
	{
		auto Wys = [&](int32 i, int32 j) { return FMath::Max(H[(i + 1) * R + (j + 1)], 0.f); };
		const int32 i = FMath::Min(int32(Lx / Krok), K - 1), j = FMath::Min(int32(Ly / Krok), K - 1);
		const float Fx = float(Lx / Krok - i), Fy = float(Ly / Krok - j);
		const float H00 = Wys(i, j), H10 = Wys(i + 1, j), H01 = Wys(i, j + 1), H11 = Wys(i + 1, j + 1);
		Spadek = FVector2f(H10 - H00 + H11 - H01, H01 - H00 + H11 - H10).Size() * 0.5f / float(Krok);
		return Fx >= Fy ? H00 + Fx * (H10 - H00) + Fy * (H11 - H10) : H00 + Fy * (H01 - H00) + Fx * (H11 - H01);
	}

	/** Losowanie oczka siatki swiata: stale dla danego (gi, gj) i numeru k. */
	struct FLos
	{
		uint64 Ziarno;
		FLos(int64 gi, int64 gj, uint64 Sol = 0) : Ziarno(Mieszaj((uint64(gi) * 0x9E3779B97F4A7C15ull ^ uint64(gj) * 0xC2B2AE3D27D4EB4Full) + Sol)) {}
		float operator()(uint64 k) const { return float(Mieszaj(Ziarno + k * 0x632BE59BD9B4E019ull) & 0xFFFFFF) / 16777216.f; }
	};

	/**
	 * Drzewa i krzaki kafla. Kazde oczko siatki swiata ma stale losowanie (hash z numeru oczka), wiec drzewo stoi w tym
	 * samym miejscu na kazdym poziomie kafli. Grubsze poziomy zostawiaja tylko czesc drzew (wieksze), krzaki tylko najblizej.
	 * Wysokosc z siatki kafla (te same trojkaty co teren), wiec drzewa nie wisza ani nie tona.
	 */
	void Sadz(ATerenKafle::FDane& D, FBox2D Obszar, int32 K, const TArray<float>& H, int32 R, int32 Rzadkosc, double Komorka, uint32 Dostepne, float SkalaDrzew)
	{
		static constexpr float Zostaw[] = { 1.f, 0.4f, 0.15f };
		const float Czesc = Zostaw[FMath::Min(Rzadkosc, 2)];
		const float Powieksz = FMath::Pow(Czesc, -0.25f) * SkalaDrzew;
		const double Krok = Obszar.GetSize().X / K;

		D.Drzewa.SetNum(Gatunkow * 2);
		const int64 I0 = FMath::FloorToInt64((Obszar.Min.X + FSwiat::Polowa) / Komorka), I1 = FMath::CeilToInt64((Obszar.Max.X + FSwiat::Polowa) / Komorka);
		const int64 J0 = FMath::FloorToInt64((Obszar.Min.Y + FSwiat::Polowa) / Komorka), J1 = FMath::CeilToInt64((Obszar.Max.Y + FSwiat::Polowa) / Komorka);
		for (int64 gi = I0; gi < I1; ++gi)
		{
			for (int64 gj = J0; gj < J1; ++gj)
			{
				const FLos Los(gi, gj);
				if (Los(2) >= Czesc)
				{
					continue;
				}
				const double Lx = (gi + 0.15 + 0.7 * Los(0)) * Komorka - FSwiat::Polowa - Obszar.Min.X;
				const double Ly = (gj + 0.15 + 0.7 * Los(1)) * Komorka - FSwiat::Polowa - Obszar.Min.Y;
				const double Bok = Obszar.GetSize().X;
				if (Lx < 0.0 || Ly < 0.0 || Lx >= Bok || Ly >= Bok)
				{
					continue; // oczko na granicy: drzewo nalezy do sasiedniego kafla
				}
				const double Wx = Obszar.Min.X + Lx, Wy = Obszar.Min.Y + Ly;
				const int32 Klasa = int32(FSwiat::Pokrycie(Wx, Wy));
				const float L3 = Los(3);
				const bool bDrzewo = L3 < GestDrzew[Klasa];
				if (!bDrzewo && (Rzadkosc > 0 || L3 >= GestDrzew[Klasa] + GestKrzakow[Klasa]))
				{
					continue;
				}

				float Spadek;
				const float Z = WysNaKaflu(H, R, K, Krok, Lx, Ly, Spadek);
				if (Z < 60.f || Spadek > 1.3f ||FSwiat::Woda(Wx, Wy) != FSwiat::EWoda::Lad)
				{
					continue; // morze i plaza, strome zbocza (tam skaly), jeziora i rzeki
				}

				int32 Gatunek = Krzak;
				if (bDrzewo)
				{
					const float* U = Z / FSwiat::MetryNaCm > 1100.0 ? UdzialyGory : Udzialy[Klasa];
					const float Suma = U[0] + U[1] + U[2] + U[3] + U[4];
					float Wybor = Los(4) * Suma;
					for (Gatunek = 0; Gatunek < 4 && Wybor >= U[Gatunek]; ++Gatunek)
					{
						Wybor -= U[Gatunek];
					}
				}
				int32 Model = Gatunek * 2 + (Los(5) < 0.5f ? 0 : 1);
				if (!(Dostepne & (1u << Model)))
				{
					Model ^= 1; // drugi wariant
					if (!(Dostepne & (1u << Model)))
					{
						continue;
					}
				}
				const float Skala = (0.75f + 0.5f * Los(6)) * (bDrzewo ? Powieksz : 1.f);
				// Drzewa: lekkie pochylenie (0-7 st., w losowa strone) i rozne proporcje (szersze/wezsze, nizsze/wyzsze); krzaki bez zmian.
				FQuat Obrot(FVector::UpVector, FMath::DegreesToRadians(Los(7) * 360.0));
				FVector Rozmiar(Skala);
				if (bDrzewo)
				{
					const double Kier = FMath::DegreesToRadians(Los(8) * 360.0);
					Obrot = FQuat(FVector(-FMath::Sin(Kier), FMath::Cos(Kier), 0.0), FMath::DegreesToRadians(Los(9) * 7.0)) * Obrot;
					Rozmiar = FVector(Skala * (0.85f + 0.35f * Los(10)), Skala * (0.85f + 0.35f * Los(10)), Skala * (0.85f + 0.3f * Los(11)));
				}
				D.Drzewa[Model].Add(FTransform(Obrot, FVector(Lx, Ly, Z - 30.f * Skala), Rozmiar));
			}
		}
	}

	/**
	 * Drobiazgi mapy grywalnej (jak na zrzutach wzorcowych): kepki trawy, kamyki (gesto przy wodzie i na zboczach)
	 * i male krzaczki na lakach. Wlasna siatka oczek co 4 m, niezalezna od drzew.
	 */
	void SadzDrobne(ATerenKafle::FDane& D, FBox2D Obszar, int32 K, const TArray<float>& H, int32 R, uint32 Dostepne)
	{
		constexpr double Oczko = 400.0;
		constexpr double Brzeg = 1200.0; // kamyki: woda blizej niz 12 m
		// Szansa w oczku wg klasy pokrycia (woda, laka, wrzos, lisc, igl, miesz, rzadki, mokr, skaly, snieg, tundra).
		constexpr float PTrawa[int32(EP::Ile)] = { 0.f, 0.22f, 0.18f, 0.06f, 0.04f, 0.05f, 0.18f, 0.25f, 0.05f, 0.f, 0.15f };
		constexpr float PKrzak[int32(EP::Ile)] = { 0.f, 0.012f, 0.03f, 0.01f, 0.005f, 0.01f, 0.03f, 0.02f, 0.005f, 0.f, 0.02f };
		const double Krok = Obszar.GetSize().X / K, Bok = Obszar.GetSize().X;
		const int64 I0 = FMath::FloorToInt64((Obszar.Min.X + FSwiat::Polowa) / Oczko), I1 = FMath::CeilToInt64((Obszar.Max.X + FSwiat::Polowa) / Oczko);
		const int64 J0 = FMath::FloorToInt64((Obszar.Min.Y + FSwiat::Polowa) / Oczko), J1 = FMath::CeilToInt64((Obszar.Max.Y + FSwiat::Polowa) / Oczko);
		for (int64 gi = I0; gi < I1; ++gi)
		{
			for (int64 gj = J0; gj < J1; ++gj)
			{
				const FLos Los(gi, gj, 0x5EED);
				const float L = Los(0);
				if (L >= 0.3f)
				{
					continue; // zadna szansa nie przekracza 0,3 - szybkie odrzucenie 70% oczek
				}
				const double Lx = (gi + Los(1)) * Oczko - FSwiat::Polowa - Obszar.Min.X;
				const double Ly = (gj + Los(2)) * Oczko - FSwiat::Polowa - Obszar.Min.Y;
				if (Lx < 0.0 || Ly < 0.0 || Lx >= Bok || Ly >= Bok)
				{
					continue;
				}
				const double Wx = Obszar.Min.X + Lx, Wy = Obszar.Min.Y + Ly;
				if (FSwiat::Woda(Wx, Wy) != FSwiat::EWoda::Lad)
				{
					continue;
				}
				float Spadek;
				const float Z = WysNaKaflu(H, R, K, Krok, Lx, Ly, Spadek);
				if (Z < 60.f)
				{
					continue;
				}
				const int32 Klasa = int32(FSwiat::Pokrycie(Wx, Wy));
				bool bPrzyWodzie = false;
				for (const FVector2D Kier : { FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1) })
				{
					bPrzyWodzie |= FSwiat::Woda(Wx + Kier.X * Brzeg, Wy + Kier.Y * Brzeg) != FSwiat::EWoda::Lad;
				}
				const float PKamien = bPrzyWodzie ? 0.25f : (Spadek > 0.5f ? 0.08f : 0.008f);
				const float PT = bPrzyWodzie ? 0.f : PTrawa[Klasa];

				int32 Gatunek;
				float Skala;
				float Zanurz; // cm w ziemie na jednostke skali
				if (L < PKamien)
				{
					Gatunek = Kamien; Skala = 0.6f + 0.9f * Los(3) * Los(3); Zanurz = 10.f;
				}
				else if (L < PKamien + PKrzak[Klasa] && !bPrzyWodzie)
				{
					Gatunek = Krzak; Skala = 0.3f + 0.3f * Los(3); Zanurz = 30.f;
				}
				else if (L < PKamien + PKrzak[Klasa] + PT)
				{
					Gatunek = Trawa; Skala = 0.9f + 0.9f * Los(3); Zanurz = 5.f;
				}
				else
				{
					continue;
				}
				int32 Model = Gatunek * 2 + (Los(4) < 0.5f ? 0 : 1);
				if (!(Dostepne & (1u << Model)))
				{
					Model ^= 1;
					if (!(Dostepne & (1u << Model)))
					{
						continue;
					}
				}
				D.Drzewa[Model].Add(FTransform(FRotator(0.0, Los(5) * 360.0, 0.0), FVector(Lx, Ly, Z - Zanurz * Skala), FVector(Skala)));
			}
		}
	}

	/**
	 * Liczy siatke kafla. Wywolywane na watku roboczym: tylko FSwiat (tylko odczyt) i wartosci. Rzadkosc < 0 = bez drzew.
	 * bPlaski: mapa papierowa - plaska kartka na wysokosci 0 (gory rysuja znaki w M_Teren), wiec dzielenie kafli przy
	 * przyblizaniu niczego nie zmienia na ekranie (dawniej rzezba 3D doczytywala sie na oczach gracza).
	 */
	TSharedPtr<ATerenKafle::FDane> Zbuduj(FBox2D Obszar, int32 K, int32 Rzadkosc, double Komorka, uint32 Dostepne, float SkalaDrzew, bool bPlaski)
	{
		TSharedPtr<ATerenKafle::FDane> D = MakeShared<ATerenKafle::FDane>();
		const double Krok = Obszar.GetSize().X / K;
		const int32 R = K + 3; // siatka wysokosci z obwodka 1 kratki (do normalnych)
		TArray<float> H, OdRzeki;
		H.SetNumUninitialized(R * R);
		OdRzeki.SetNumUninitialized(R * R);
		for (int32 i = 0; i < R; ++i)
		{
			for (int32 j = 0; j < R; ++j)
			{
				OdRzeki[i * R + j] = 0.f;
				H[i * R + j] = bPlaski ? 0.f : FSwiat::Wysokosc(Obszar.Min.X + (i - 1) * Krok, Obszar.Min.Y + (j - 1) * Krok, &OdRzeki[i * R + j]);
			}
		}
		auto Wys = [&](int32 i, int32 j) { return FMath::Max(H[(i + 1) * R + (j + 1)], 0.f); }; // morze plaskie na 0

		const int32 N1 = K + 1;
		D->V.Reserve(N1 * N1 + 8 * N1);
		// Kafle zaczynaja sie od -Polowa w wielokrotnosciach 512 m, wiec reszta z okresu jest dokladna.
		const FVector2D Det0(FMath::Fmod(Obszar.Min.X + FSwiat::Polowa, DetalOkresCm) / DetalCm,
		                     FMath::Fmod(Obszar.Min.Y + FSwiat::Polowa, DetalOkresCm) / DetalCm);
		for (int32 i = 0; i <= K; ++i)
		{
			for (int32 j = 0; j <= K; ++j)
			{
				const float Z = Wys(i, j);
				const FVector Nor = FVector(-(Wys(i + 1, j) - Wys(i - 1, j)), -(Wys(i, j + 1) - Wys(i, j - 1)), 2.0 * Krok).GetSafeNormal();
				D->V.Add(FVector(i * Krok, j * Krok, Z));
				D->N.Add(Nor);
				D->UV0.Add(Det0 + FVector2D(i * Krok, j * Krok) / DetalCm);
				// Pozycja w kaflu 0..1 (i/K: dokladna takze w 16-bitowych UV ProceduralMesh). M_Teren liczy z niej
				// UV mapy Europy razem z rogiem i bokiem kafla (Custom Primitive Data, double z C++).
				D->UV1.Add(FVector2D(double(i) / K, double(j) / K));
				// Co 4 m na mapie grywalnej: liniowo miedzy wierzcholkami daje gladki brzeg rzeki w materiale.
				D->UV2.Add(FVector2D(OdRzeki[(i + 1) * R + (j + 1)] / 100.0, 0.0));
			}
		}
		// Kolejnosc sprawdzona w grze: odwrotna (A,B,C) dawala teren widoczny tylko od spodu przy materiale jednostronnym.
		for (int32 i = 0; i < K; ++i)
		{
			for (int32 j = 0; j < K; ++j)
			{
				const int32 A = i * N1 + j, B = (i + 1) * N1 + j, C = (i + 1) * N1 + j + 1, E = i * N1 + j + 1;
				D->T.Append({ A, C, B, A, E, C });
			}
		}
		// Fartuchy: pionowe paski w dol na krawedziach, zaslaniaja szpary miedzy kaflami roznych poziomow.
		const float Fartuch = float(Krok * 1.5 + 500.0);
		auto Krawedz = [&](TFunctionRef<int32(int32)> Idx)
		{
			for (int32 s = 0; s < K; ++s)
			{
				const int32 A = Idx(s), B = Idx(s + 1);
				// Kopie przed Add: element tej samej tablicy przestaje byc wazny po realokacji.
				const FVector Va = D->V[A] - FVector(0, 0, Fartuch), Vb = D->V[B] - FVector(0, 0, Fartuch);
				const int32 A2 = D->V.Add(Va);
				const int32 B2 = D->V.Add(Vb);
				for (int32 Src : { A, B })
				{
					const FVector N = D->N[Src];
					const FVector2D U0 = D->UV0[Src], U1 = D->UV1[Src], U2 = D->UV2[Src];
					D->N.Add(N);
					D->UV0.Add(U0);
					D->UV1.Add(U1);
					D->UV2.Add(U2);
				}
				D->T.Append({ A, B, B2, A, B2, A2, A, B2, B, A, A2, B2 }); // obie strony
			}
		};
		Krawedz([&](int32 s) { return s; });                 // i = 0
		Krawedz([&](int32 s) { return K * N1 + s; });        // i = K
		Krawedz([&](int32 s) { return s * N1; });            // j = 0
		Krawedz([&](int32 s) { return s * N1 + K; });        // j = K

		if (Rzadkosc >= 0 && Dostepne)
		{
			Sadz(*D, Obszar, K, H, R, Rzadkosc, Komorka, Dostepne, SkalaDrzew);
			// Rzadkosc 0 ma tylko mapa grywalna (poza nia drzewa od MinPoziomDrzew = 99, czyli nigdy).
			if (Rzadkosc == 0)
			{
				SadzDrobne(*D, Obszar, K, H, R, Dostepne);
			}
		}
		return D;
	}
}

ATerenKafle::ATerenKafle()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Material terenu budowany skryptem Tools/ue_teren.py (Tools/Zbuduj_Teren.bat).
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Game/Swiat/Teren/M_Teren.M_Teren"));
	Material = Mat.Object;

	// Drzewa i krzaki z Tools/ue_natura.py (Tools/Zbuduj_Nature.bat). Brakujace = ten gatunek nie rosnie.
	for (const TCHAR* Nazwa : { TEXT("Dab"), TEXT("Buk"), TEXT("Brzoza"), TEXT("Sosna"), TEXT("Swierk"), TEXT("Krzak"), TEXT("Kamien"), TEXT("Trawa") })
	{
		for (int32 Wariant = 1; Wariant <= 2; ++Wariant)
		{
			const FString Sciezka = FString::Printf(TEXT("/Game/Swiat/Natura/SM_%s_%d.SM_%s_%d"), Nazwa, Wariant, Nazwa, Wariant);
			ConstructorHelpers::FObjectFinderOptional<UStaticMesh> Model(*Sciezka);
			Modele.Add(Model.Get());
		}
	}
}

FBox2D ATerenKafle::Obszar(const FIntVector& K)
{
	const double Bok = FSwiat::Bok / double(1 << K.Z);
	const FVector2D Min(-FSwiat::Polowa + K.X * Bok, -FSwiat::Polowa + K.Y * Bok);
	return FBox2D(Min, Min + FVector2D(Bok, Bok));
}

void ATerenKafle::Zbierz(int32 Poziom, int32 X, int32 Y, const FVector& Kamera, float NadZiemia, TArray<FIntVector>& Out) const
{
	const FIntVector K(X, Y, Poziom);
	const FBox2D B = Obszar(K);
	const double Dx = FMath::Max(0.0, FMath::Max(B.Min.X - Kamera.X, Kamera.X - B.Max.X));
	const double Dy = FMath::Max(0.0, FMath::Max(B.Min.Y - Kamera.Y, Kamera.Y - B.Max.Y));
	const double Odl = FMath::Sqrt(Dx * Dx + Dy * Dy + double(NadZiemia) * NadZiemia);
	if (Poziom < MaxPoziomMapy && Odl < B.GetSize().X * Podzial)
	{
		for (int32 c = 0; c < 4; ++c)
		{
			Zbierz(Poziom + 1, X * 2 + (c & 1), Y * 2 + (c >> 1), Kamera, NadZiemia, Out);
		}
		return;
	}
	Out.Add(K);
}

bool ATerenKafle::PokrytyGotowymi(const FIntVector& K, const TArray<FIntVector>& Lista) const
{
	const FBox2D B = Obszar(K).ExpandBy(-1.0);
	for (const FIntVector& C : Lista)
	{
		if (Obszar(C).Intersect(B))
		{
			const FKafel* Kaf = Kafle.Find(C);
			if (!Kaf || !Kaf->bGotowy)
			{
				return false;
			}
		}
	}
	return true;
}

void ATerenKafle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Odbior gotowych siatek z watkow roboczych. Podczas jazdy po mapie grywalnej najwyzej KafliNaKlatke na klatke (bez przyciec).
	int32 Odebrane = 0;
	TArray<FIntVector> Porzucone;
	for (auto& Para : Kafle)
	{
		FKafel& Kaf = Para.Value;
		if (Kaf.bGotowy || !Kaf.Zadanie.IsValid() || !Kaf.Zadanie.IsReady())
		{
			continue;
		}
		// Kamera odjechala, zanim kafel sie policzyl: nie budujemy komponentow, ktore zaraz trzeba by usunac.
		if (bRegion && (FMath::Abs(Para.Key.X - SrodekRegionu.X) > PromienRegionu + 1 || FMath::Abs(Para.Key.Y - SrodekRegionu.Y) > PromienRegionu + 1))
		{
			Kaf.Zadanie.Reset();
			--Zadania;
			Porzucone.Add(Para.Key);
			continue;
		}
		if (bRegion && bByloGotowe && Odebrane++ >= KafliNaKlatke)
		{
			break;
		}
		TSharedPtr<FDane> D = Kaf.Zadanie.Get();
		Kaf.Zadanie.Reset();
		--Zadania;

		const FBox2D B = Obszar(Para.Key);
		UProceduralMeshComponent* M = NewObject<UProceduralMeshComponent>(this);
		M->SetupAttachment(RootComponent);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->RegisterComponent();
		M->SetWorldLocation(FVector(B.Min.X, B.Min.Y, 0.0));
		// M_Teren: UV rogu kafla w pelnej precyzji (UV siatki ProceduralMesh sa 16-bitowe), reszte dolicza z pozycji lokalnej.
		M->SetCustomPrimitiveDataFloat(0, float((B.Min.Y + FSwiat::Polowa) / FSwiat::Bok));
		M->SetCustomPrimitiveDataFloat(1, float((FSwiat::Polowa - B.Min.X) / FSwiat::Bok));
		M->SetCustomPrimitiveDataFloat(2, float(FMath::Fmod(B.Min.X + FSwiat::Polowa, DetalOkresCm) / DetalCm));
		M->SetCustomPrimitiveDataFloat(3, float(FMath::Fmod(B.Min.Y + FSwiat::Polowa, DetalOkresCm) / DetalCm));
		M->SetCustomPrimitiveDataFloat(4, float(B.GetSize().X / FSwiat::Bok));
		M->CreateMeshSection(0, D->V, D->T, D->N, D->UV0, D->UV1, D->UV2, TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
		if (bRegion && !MaterialRegionu && Material)
		{
			MaterialRegionu = UMaterialInstanceDynamic::Create(Material, this);
			MaterialRegionu->SetScalarParameterValue(TEXT("Region"), 1.f); // M_Teren: rowna trawa jak na zrzucie
		}
		if (!bRegion && !MaterialMapy && Material)
		{
			MaterialMapy = UMaterialInstanceDynamic::Create(Material, this);
		}
		M->SetMaterial(0, bRegion ? (MaterialRegionu ? MaterialRegionu.Get() : Material.Get()) : (MaterialMapy ? MaterialMapy.Get() : Material.Get()));
		Kaf.Siatka = M;

		for (int32 i = 0; i < D->Drzewa.Num() && i < Modele.Num(); ++i)
		{
			if (D->Drzewa[i].Num() == 0 || !Modele[i])
			{
				continue;
			}
			UInstancedStaticMeshComponent* Drz = NewObject<UInstancedStaticMeshComponent>(this);
			Drz->SetupAttachment(RootComponent);
			Drz->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Drz->SetStaticMesh(Modele[i]);
			// Kepki trawy bez cienia: setki tysiecy drobiazgow zapchalyby pamiec cieni (VSM) - cienie urywaly sie juz przy samych drzewach.
			Drz->SetCastShadow(i / 2 != Trawa);
			// Wiatr (WPO w M_Natura*, 21.11.2026) tylko do 600 m od kamery: dalej ruchu i tak nie widac, a kazde ruszane
			// drzewo kosztuje (Nanite + odswiezanie cieni VSM).
			Drz->WorldPositionOffsetDisableDistance = 60000;
			if (i / 2 == Trawa || i / 2 == Kamien)
			{
				Drz->SetCullDistances(0, int32(ZasiegDrobnych));
			}
			Drz->RegisterComponent();
			Drz->SetWorldLocation(FVector(B.Min.X, B.Min.Y, 0.0));
			if (Polany.Num() == 0)
			{
				Drz->AddInstances(D->Drzewa[i], false, false, false);
			}
			else
			{
				TArray<FTransform> Zostaja;
				Zostaja.Reserve(D->Drzewa[i].Num());
				for (const FTransform& T : D->Drzewa[i])
				{
					if (!NaPolanie(B.Min.X + T.GetLocation().X, B.Min.Y + T.GetLocation().Y))
					{
						Zostaja.Add(T);
					}
				}
				Drz->AddInstances(Zostaja, false, false, false);
			}
			Kaf.Drzewa.Add(Drz);
		}
		Kaf.bGotowy = true;
	}
	for (const FIntVector& K : Porzucone)
	{
		Kafle.Remove(K);
	}

	// Mapa strategiczna: przejscie teren -> pergamin i skala wzorkow (drzewka, linia brzegu) stala na ekranie:
	// ~20 kafli T_Mapa na szerokosc widoku (8 drzewek na kafel). Gorna granica 14: precyzja UV mapy w materiale.
	if (MaterialMapy)
	{
		const APlayerController* PCk = GetWorld()->GetFirstPlayerController();
		if (const AKameraMapy* Kam = PCk ? Cast<AKameraMapy>(PCk->GetPawn()) : nullptr)
		{
			MaterialMapy->SetScalarParameterValue(TEXT("Mapa"), Kam->StrefaMapy());
			const double Skala = FMath::Log2(20.0 * FSwiat::Bok / FMath::Max(1.0, double(Kam->SzerokoscWidoku())));
			MaterialMapy->SetScalarParameterValue(TEXT("SkalaMapy"), float(FMath::Clamp(Skala, 0.0, 16.0)));

			// Znaki mapy (gory, pagorki, drzewa): ZnakiGestosc oczek na szerokosc widoku, stoja prosto wzgledem ekranu.
			// UV mapy: U = wschod (+Y swiata), V = poludnie (-X swiata).
			const double Oczko = double(Kam->SzerokoscWidoku()) / (ZnakiGestosc * FSwiat::Bok);
			MaterialMapy->SetScalarParameterValue(TEXT("ZnakiPoziom"), float(-FMath::Log2(FMath::Max(Oczko, 1e-9))));
			if (PCk->PlayerCameraManager)
			{
				const FRotator R = PCk->PlayerCameraManager->GetCameraRotation();
				const double Yaw = FMath::DegreesToRadians(R.Yaw);
				const double Nachylenie = FMath::Clamp(-R.Pitch, 20.0, 90.0);
				MaterialMapy->SetVectorParameterValue(TEXT("ZnakiOs"), FLinearColor(float(FMath::Cos(Yaw)), float(FMath::Sin(Yaw)), float(FMath::Sin(Yaw)), float(-FMath::Cos(Yaw))));
				MaterialMapy->SetScalarParameterValue(TEXT("ZnakiRozciag"), float(1.0 / FMath::Sin(FMath::DegreesToRadians(Nachylenie))));
			}
		}
	}

	const double Teraz = GetWorld()->GetRealTimeSeconds();
	bByloGotowe |= Gotowe();
	if (Teraz >= NastepneOdswiezenie || !bByloGotowe)
	{
		NastepneOdswiezenie = Teraz + 0.15;
		Odswiez();
	}
}

bool ATerenKafle::NaPolanie(double X, double Y) const
{
	const TArray<int32>* W = PolanyKomorki.Find(FIntPoint(FMath::FloorToInt32(X / KomorkaPolan), FMath::FloorToInt32(Y / KomorkaPolan)));
	if (!W)
	{
		return false;
	}
	for (const int32 i : *W)
	{
		const FVector& P = Polany[i];
		if (FMath::Square(X - P.X) + FMath::Square(Y - P.Y) < FMath::Square(P.Z))
		{
			return true;
		}
	}
	return false;
}

bool ATerenKafle::DodajPolane(FVector2D Srodek, float Promien)
{
	const FIntPoint K(FMath::FloorToInt32(Srodek.X / KomorkaPolan), FMath::FloorToInt32(Srodek.Y / KomorkaPolan));
	if (const TArray<int32>* W = PolanyKomorki.Find(K))
	{
		for (const int32 i : *W)
		{
			if (FVector2D::Distance(FVector2D(Polany[i]), Srodek) + Promien <= Polany[i].Z)
			{
				return false;
			}
		}
	}
	const int32 Nowa = Polany.Add(FVector(Srodek.X, Srodek.Y, Promien));
	const int32 X0 = FMath::FloorToInt32((Srodek.X - Promien) / KomorkaPolan), X1 = FMath::FloorToInt32((Srodek.X + Promien) / KomorkaPolan);
	const int32 Y0 = FMath::FloorToInt32((Srodek.Y - Promien) / KomorkaPolan), Y1 = FMath::FloorToInt32((Srodek.Y + Promien) / KomorkaPolan);
	for (int32 x = X0; x <= X1; ++x)
	{
		for (int32 y = Y0; y <= Y1; ++y)
		{
			PolanyKomorki.FindOrAdd(FIntPoint(x, y)).Add(Nowa);
		}
	}
	return true;
}

void ATerenKafle::Wykarczuj(FVector2D Srodek, float Promien)
{
	if (!DodajPolane(Srodek, Promien))
	{
		return; // ta sama polana jeszcze raz (osady rodow przy odswiezaniu): drzewa juz wyciete
	}
	const FBox2D Kolo(Srodek - FVector2D(Promien), Srodek + FVector2D(Promien));
	TSet<int32> Zmienione;
	for (int32 k = Sadzonki.Num() - 1; k >= 0; --k)
	{
		if (FVector2D::DistSquared(Sadzonki[k].P, Srodek) < double(Promien) * Promien)
		{
			Zmienione.Add(Sadzonki[k].Model);
			Sadzonki.RemoveAt(k);
		}
	}
	for (int32 M : Zmienione)
	{
		OdbudujMlode(M);
	}
	for (auto& Para : Kafle)
	{
		if (!Obszar(Para.Key).Intersect(Kolo))
		{
			continue; // tylko kafle pod polana (wczesniej przegladane wszystkie drzewa wszystkich kafli)
		}
		for (UInstancedStaticMeshComponent* Drz : Para.Value.Drzewa)
		{
			if (!Drz)
			{
				continue;
			}
			TArray<int32> Usun;
			const int32 N = Drz->GetInstanceCount();
			for (int32 i = 0; i < N; ++i)
			{
				FTransform T;
				if (Drz->GetInstanceTransform(i, T, true) && FVector2D::DistSquared(FVector2D(T.GetLocation()), Srodek) < FMath::Square(Promien))
				{
					Usun.Add(i);
				}
			}
			if (Usun.Num())
			{
				Drz->RemoveInstances(Usun);
			}
		}
	}
}

bool ATerenKafle::NajblizszeZKafli(FVector2D Srodek, float Promien, const TArray<FVector2D>* Pomin, UInstancedStaticMeshComponent*& Komp, int32& Indeks, FVector2D& Gdzie, bool bKrzaki) const
{
	// Tylko drzewa (gatunki Dab..Swierk = modele 0..9) albo tylko krzaki (bKrzaki), tylko kafle nachodzace na kolo.
	double Najl = double(Promien) * Promien;
	Komp = nullptr;
	Indeks = -1;
	const FBox2D Kolo(Srodek - FVector2D(Promien), Srodek + FVector2D(Promien));
	for (const auto& Para : Kafle)
	{
		if (!Para.Value.bGotowy || !Obszar(Para.Key).Intersect(Kolo))
		{
			continue;
		}
		for (UInstancedStaticMeshComponent* Drz : Para.Value.Drzewa)
		{
			if (!Drz)
			{
				continue;
			}
			const int32 Model = Modele.IndexOfByKey(Drz->GetStaticMesh());
			if (Model < 0 || (bKrzaki ? (Model < Krzak * 2 || Model >= Krzak * 2 + 2) : Model >= Krzak * 2))
			{
				continue;
			}
			const int32 N = Drz->GetInstanceCount();
			for (int32 i = 0; i < N; ++i)
			{
				FTransform T;
				if (!Drz->GetInstanceTransform(i, T, true))
				{
					continue;
				}
				const FVector2D P(T.GetLocation());
				const double D = FVector2D::DistSquared(P, Srodek);
				if (D >= Najl)
				{
					continue;
				}
				bool bPomin = false;
				if (Pomin)
				{
					for (const FVector2D& Q : *Pomin)
					{
						bPomin |= FVector2D::DistSquared(Q, P) < 100.0 * 100.0;
					}
				}
				if (!bPomin)
				{
					Najl = D;
					Komp = Drz;
					Indeks = i;
					Gdzie = P;
				}
			}
		}
	}
	return Komp != nullptr;
}

bool ATerenKafle::ZnajdzDrzewo(FVector2D Srodek, float Promien, const TArray<FVector2D>& Pomin, FVector2D& Out) const
{
	UInstancedStaticMeshComponent* Komp = nullptr;
	int32 Indeks = -1;
	FVector2D Gdzie = FVector2D::ZeroVector;
	bool bJest = NajblizszeZKafli(Srodek, Promien, &Pomin, Komp, Indeks, Gdzie);
	double Najl = bJest ? FVector2D::DistSquared(Gdzie, Srodek) : double(Promien) * Promien;
	for (const FSadzonka& S : Sadzonki)
	{
		if (S.Wiek < CzasWzrostu)
		{
			continue;
		}
		const double D = FVector2D::DistSquared(S.P, Srodek);
		bool bPomin = false;
		for (const FVector2D& Q : Pomin)
		{
			bPomin |= FVector2D::DistSquared(Q, S.P) < 100.0 * 100.0;
		}
		if (D < Najl && !bPomin)
		{
			Najl = D;
			Gdzie = S.P;
			bJest = true;
		}
	}
	if (bJest)
	{
		Out = Gdzie;
	}
	return bJest;
}

bool ATerenKafle::ZnajdzKrzak(FVector2D Srodek, float Promien, const TArray<FVector2D>& Pomin, FVector2D& Out) const
{
	UInstancedStaticMeshComponent* Komp = nullptr;
	int32 Indeks = -1;
	return NajblizszeZKafli(Srodek, Promien, &Pomin, Komp, Indeks, Out, true);
}

int32 ATerenKafle::DrzewWKole(FVector2D Srodek, float Promien) const
{
	int32 N = 0;
	const FBox2D Kolo(Srodek - FVector2D(Promien), Srodek + FVector2D(Promien));
	for (const auto& Para : Kafle)
	{
		if (!Para.Value.bGotowy || !Obszar(Para.Key).Intersect(Kolo))
		{
			continue;
		}
		for (UInstancedStaticMeshComponent* Drz : Para.Value.Drzewa)
		{
			const int32 Model = Drz ? Modele.IndexOfByKey(Drz->GetStaticMesh()) : -1;
			if (Model < 0 || Model >= Krzak * 2)
			{
				continue;
			}
			for (int32 i = 0, Ile = Drz->GetInstanceCount(); i < Ile; ++i)
			{
				FTransform T;
				if (Drz->GetInstanceTransform(i, T, true) && FVector2D::DistSquared(FVector2D(T.GetLocation()), Srodek) < double(Promien) * Promien)
				{
					++N;
				}
			}
		}
	}
	for (const FSadzonka& S : Sadzonki)
	{
		N += (S.Wiek >= CzasWzrostu && FVector2D::DistSquared(S.P, Srodek) < double(Promien) * Promien) ? 1 : 0;
	}
	return N;
}

bool ATerenKafle::Zetnij(FVector2D Gdzie)
{
	for (int32 k = 0; k < Sadzonki.Num(); ++k)
	{
		if (Sadzonki[k].Wiek >= CzasWzrostu && FVector2D::DistSquared(Sadzonki[k].P, Gdzie) < 150.0 * 150.0)
		{
			const int32 Model = Sadzonki[k].Model;
			Sadzonki.RemoveAt(k);
			OdbudujMlode(Model);
			return true;
		}
	}
	UInstancedStaticMeshComponent* Komp = nullptr;
	int32 Indeks = -1;
	FVector2D P = FVector2D::ZeroVector;
	if (!NajblizszeZKafli(Gdzie, 150.f, nullptr, Komp, Indeks, P))
	{
		return false;
	}
	Komp->RemoveInstance(Indeks);
	DodajPolane(P, 60.f); // kafel liczony od nowa nie postawi go z powrotem
	return true;
}

bool ATerenKafle::DrzewoBlisko(FVector2D Gdzie, float Promien) const
{
	for (const FSadzonka& S : Sadzonki)
	{
		if (FVector2D::DistSquared(S.P, Gdzie) < double(Promien) * Promien)
		{
			return true;
		}
	}
	UInstancedStaticMeshComponent* Komp = nullptr;
	int32 Indeks = -1;
	FVector2D P = FVector2D::ZeroVector;
	return NajblizszeZKafli(Gdzie, Promien, nullptr, Komp, Indeks, P);
}

FTransform ATerenKafle::TransformSadzonki(const FSadzonka& S) const
{
	// Od 12% do pelnej wielkosci; osadzenie w ziemi jak w Sadz (Z - 30 * skala).
	const float T = FMath::Clamp(S.Wiek / FMath::Max(1.f, CzasWzrostu), 0.f, 1.f);
	const float Sk = S.Skala * FMath::Lerp(0.12f, 1.f, T);
	const float Z = FSwiat::Wysokosc(S.P.X, S.P.Y);
	return FTransform(FRotator(0.f, S.Obrot, 0.f), FVector(S.P, Z - 30.f * Sk), FVector(Sk));
}

void ATerenKafle::OdbudujMlode(int32 Model)
{
	if (!Modele.IsValidIndex(Model) || !Modele[Model])
	{
		return;
	}
	if (Mlode.Num() < Modele.Num())
	{
		Mlode.SetNum(Modele.Num());
	}
	UInstancedStaticMeshComponent* C = Mlode[Model];
	if (!C)
	{
		C = NewObject<UInstancedStaticMeshComponent>(this);
		C->SetupAttachment(RootComponent);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetStaticMesh(Modele[Model]);
		C->RegisterComponent();
		C->SetWorldLocation(FVector::ZeroVector);
		C->SetVisibility(bRegion);
		Mlode[Model] = C;
	}
	C->ClearInstances();
	TArray<FTransform> T;
	for (const FSadzonka& S : Sadzonki)
	{
		if (S.Model == Model)
		{
			T.Add(TransformSadzonki(S));
		}
	}
	C->AddInstances(T, false, true, false);
}

void ATerenKafle::Zasadz(FVector2D Gdzie)
{
	// Gatunek jak w okolicznym lesie (dab, buk, sosna, swierk; brzoz borowy nie sadzi), losowy wariant.
	const int32 Gatunki[] = { Dab, Buk, Sosna, Swierk };
	FSadzonka S;
	S.P = Gdzie;
	S.Obrot = FMath::FRand() * 360.f;
	S.Skala = (0.75f + 0.5f * FMath::FRand()) * SkalaDrzewRegionu;
	S.Model = -1;
	for (int32 Proba = 0; Proba < 8 && S.Model < 0; ++Proba)
	{
		const int32 M = Gatunki[FMath::RandHelper(4)] * 2 + FMath::RandHelper(2);
		if (Modele.IsValidIndex(M) && Modele[M])
		{
			S.Model = M;
		}
	}
	if (S.Model < 0)
	{
		return;
	}
	Sadzonki.Add(S);
	OdbudujMlode(S.Model);
}

int32 ATerenKafle::Sadzonek(FVector2D Srodek, float Promien) const
{
	int32 N = 0;
	for (const FSadzonka& S : Sadzonki)
	{
		N += (S.Wiek < CzasWzrostu && FVector2D::DistSquared(S.P, Srodek) < double(Promien) * Promien) ? 1 : 0;
	}
	return N;
}

void ATerenKafle::Rosnij(float Sekundy)
{
	TSet<int32> Zmienione;
	for (FSadzonka& S : Sadzonki)
	{
		if (S.Wiek < CzasWzrostu)
		{
			S.Wiek += Sekundy;
			Zmienione.Add(S.Model);
		}
	}
	for (int32 M : Zmienione)
	{
		OdbudujMlode(M);
	}
}

void ATerenKafle::Wyczysc()
{
	for (auto& Para : Kafle)
	{
		// Zadania w tle licza do konca na wlasnych kopiach danych; wynik przepada razem z TFuture.
		if (Para.Value.Siatka)
		{
			Para.Value.Siatka->DestroyComponent();
		}
		for (UInstancedStaticMeshComponent* Drz : Para.Value.Drzewa)
		{
			Drz->DestroyComponent();
		}
	}
	Kafle.Empty();
	Chciane.Reset();
	Zadania = 0;
	bByloGotowe = false;
}

void ATerenKafle::WejdzDoRegionu()
{
	Wyczysc();
	FSwiat::UstawRegion(true); // wyrazniejsza rzezba na mapie grywalnej
	bRegion = true;
	for (UInstancedStaticMeshComponent* C : Mlode)
	{
		if (C)
		{
			C->SetVisibility(true);
		}
	}
}

void ATerenKafle::WyjdzZRegionu()
{
	Wyczysc();
	FSwiat::UstawRegion(false);
	bRegion = false;
	for (UInstancedStaticMeshComponent* C : Mlode)
	{
		if (C)
		{
			C->SetVisibility(false); // mapa papierowa jest plaska
		}
	}
}

float ATerenKafle::Postep() const
{
	int32 Ile = 0;
	for (const FIntVector& K : Chciane)
	{
		const FKafel* Kaf = Kafle.Find(K);
		Ile += Kaf && Kaf->bGotowy;
	}
	return Chciane.Num() ? float(Ile) / Chciane.Num() : 0.f;
}

void ATerenKafle::Odswiez()
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const FVector Kam = PC->PlayerCameraManager->GetCameraLocation();
	const float NadZiemia = FMath::Max(0.f, float(Kam.Z) - (bRegion ? FMath::Max(0.f, FSwiat::Wysokosc(Kam.X, Kam.Y)) : 0.f));

	Chciane.Reset();
	// Mapa grywalna: kafel pod ogniskiem kamery (pionek = punkt, na ktory patrzy kamera).
	const FVector2D Ognisko = PC->GetPawn() ? FVector2D(PC->GetPawn()->GetActorLocation()) : FVector2D(Kam);
	const double T = FSwiat::Bok / double(1 << MaxPoziom);
	const int32 Ostatni = (1 << MaxPoziom) - 1;
	const int32 Cx = FMath::Clamp(FMath::FloorToInt32((Ognisko.X + FSwiat::Polowa) / T), 0, Ostatni);
	const int32 Cy = FMath::Clamp(FMath::FloorToInt32((Ognisko.Y + FSwiat::Polowa) / T), 0, Ostatni);
	SrodekRegionu = FIntPoint(Cx, Cy);
	if (bRegion)
	{
		for (int32 X = FMath::Max(0, Cx - PromienRegionu); X <= FMath::Min(Ostatni, Cx + PromienRegionu); ++X)
		{
			for (int32 Y = FMath::Max(0, Cy - PromienRegionu); Y <= FMath::Min(Ostatni, Cy + PromienRegionu); ++Y)
			{
				Chciane.Add(FIntVector(X, Y, MaxPoziom));
			}
		}
	}
	else
	{
		Zbierz(0, 0, 0, Kam, NadZiemia, Chciane);
	}
	// Przy ekranie ladowania i na mapie grywalnej (liczenie z wyprzedzeniem nie moze nie nadazyc za kamera)
	// wszystkie rdzenie licza kafle; watek gry tylko odbiera gotowe.
	const int32 Limit = bByloGotowe && !bRegion ? MaxZadan : FMath::Max(MaxZadan, FPlatformMisc::NumberOfWorkerThreadsToSpawn());

	// Najpierw zgrubne poziomy (szybciej zakrywaja ekran), w obrebie poziomu najblizsze ogniska.
	const FVector2D Srodek = bRegion ? Ognisko : FVector2D(Kam);
	Chciane.Sort([&](const FIntVector& A, const FIntVector& B)
	{
		if (A.Z != B.Z) { return A.Z < B.Z; }
		return FVector2D::DistSquared(Obszar(A).GetCenter(), Srodek) < FVector2D::DistSquared(Obszar(B).GetCenter(), Srodek);
	});
	for (const FIntVector& K : Chciane)
	{
		if (Zadania >= Limit)
		{
			break;
		}
		if (!Kafle.Contains(K))
		{
			FKafel Nowy;
			const FBox2D B = Obszar(K);
			const int32 Kw = bRegion ? KwadyRegionu : Kwady;
			const int32 Rzadkosc = bRegion ? 0 : (K.Z >= MinPoziomDrzew ? MaxPoziom - K.Z : -1);
			const float Skala = bRegion ? SkalaDrzewRegionu : 1.f;
			const double Komorka = KomorkaDrzew;
			uint32 Dostepne = 0;
			for (int32 i = 0; i < Modele.Num() && i < 32; ++i)
			{
				Dostepne |= Modele[i] ? (1u << i) : 0u;
			}
			const bool bPlaski = !bRegion;
			Nowy.Zadanie = Async(EAsyncExecution::ThreadPool, [B, Kw, Rzadkosc, Komorka, Dostepne, Skala, bPlaski]() { return Zbuduj(B, Kw, Rzadkosc, Komorka, Dostepne, Skala, bPlaski); });
			Kafle.Add(K, MoveTemp(Nowy));
			++Zadania;
		}
	}

	// Usuwanie niepotrzebnych: na mapie Europy dopiero gdy ich miejsce zakrywaja gotowe nowe kafle; na mapie grywalnej
	// kafel znika dopiero kafel dalej niz pierscien (jazda tam i z powrotem nie liczy ciagle tych samych kafli).
	const TSet<FIntVector> ChcianeSet(Chciane);
	auto Zbedny = [&](const FIntVector& K, const FKafel& Kaf)
	{
		if (ChcianeSet.Contains(K))
		{
			return false;
		}
		if (bRegion)
		{
			return FMath::Abs(K.X - Cx) > PromienRegionu + 1 || FMath::Abs(K.Y - Cy) > PromienRegionu + 1;
		}
		return !Kaf.bGotowy || PokrytyGotowymi(K, Chciane);
	};
	TArray<FIntVector> DoUsuniecia;
	for (const auto& Para : Kafle)
	{
		if (Zbedny(Para.Key, Para.Value))
		{
			DoUsuniecia.Add(Para.Key);
		}
	}
	for (const FIntVector& K : DoUsuniecia)
	{
		FKafel& Kaf = Kafle[K];
		if (Kaf.Zadanie.IsValid())
		{
			// Liczy sie jeszcze w tle: zostawiamy wpis, odbior go zwolni (nie ma go w Chciane, wiec zniknie potem).
			continue;
		}
		if (Kaf.Siatka)
		{
			Kaf.Siatka->DestroyComponent();
		}
		for (UInstancedStaticMeshComponent* Drz : Kaf.Drzewa)
		{
			Drz->DestroyComponent();
		}
		Kafle.Remove(K);
	}
}
