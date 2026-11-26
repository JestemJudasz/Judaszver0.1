// Plik: Source/grarts2d/Swiat/SwiatWysokosci.cpp
#include "Swiat/SwiatWysokosci.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Async/ParallelFor.h"
#include <atomic>

namespace
{
	bool GWczytane = false;
	TArray<uint16> GDem;   // wiersz 0 = polnoc, kolumna 0 = zachod
	int32 GSzer = 0, GWys = 0;
	float GMinM = 0.f, GMaxM = 1.f;
	TArray<uint8> GWoda;   // ten sam uklad co GDem
	TArray<uint8> GPokrycie; // ten sam uklad co GDem
	TArray<uint8> GDetal;    // Data/teren_detal.rgb8 = T_Detal (512x512 RGB), do falowania granic jak w M_Teren
	constexpr int32 DetalN = 512;

	/** Kanal tekstury detalu (0..1), dwuliniowo z zawijaniem; U, V w kaflach tekstury. */
	float DetalKanal(double U, double V, int32 Kanal)
	{
		const double X = (U - FMath::FloorToDouble(U)) * DetalN - 0.5, Y = (V - FMath::FloorToDouble(V)) * DetalN - 0.5;
		const int32 C = FMath::FloorToInt32(X), R = FMath::FloorToInt32(Y);
		const float Fx = float(X - C), Fy = float(Y - R);
		auto Px = [&](int32 c, int32 r) { return GDetal[((r & (DetalN - 1)) * DetalN + (c & (DetalN - 1))) * 3 + Kanal] / 255.f; };
		return FMath::Lerp(FMath::Lerp(Px(C, R), Px(C + 1, R), Fx), FMath::Lerp(Px(C, R + 1), Px(C + 1, R + 1), Fx), Fy);
	}
	FVector2D GStart = FVector2D::ZeroVector;

	/** Szum fraktalny ~[-1,1]. */
	float Fbm(FVector2D P, int32 Oktawy)
	{
		float Suma = 0.f, Amp = 1.f, Norma = 0.f;
		for (int32 i = 0; i < Oktawy; ++i)
		{
			Suma += Amp * FMath::PerlinNoise2D(P);
			Norma += Amp;
			P = P * 2.03 + FVector2D(17.3, -9.1);
			Amp *= 0.5f;
		}
		return Suma / Norma;
	}

	/** bBezWciec: mapa grywalna rysuje rzeki z linii (Data/rzeki.bin), wiec oddaje 4 m, o ktore dem_europa.py wcial
	 *  rzeki z pikseli (inaczej zostalby schodkowy row 64 m obok nowego koryta). */
	float DemPiksel(int32 C, int32 R, bool bBezWciec = false)
	{
		C = FMath::Clamp(C, 0, GSzer - 1);
		R = FMath::Clamp(R, 0, GWys - 1);
		const float M = GMinM + (GMaxM - GMinM) * (GDem[R * GSzer + C] / 65535.f);
		return bBezWciec && GWoda.Num() && GWoda[R * GSzer + C] == uint8(FSwiat::EWoda::Rzeka) ? M + 4.f : M;
	}

	/** Prawdziwe metry z DEM (dwuliniowo). */
	float DemMetry(double X, double Y, bool bBezWciec)
	{
		const double U = (Y + FSwiat::Polowa) / FSwiat::Bok * GSzer - 0.5;  // kolumna (wschod)
		const double V = (FSwiat::Polowa - X) / FSwiat::Bok * GWys - 0.5;   // wiersz (poludnie)
		const int32 C = FMath::FloorToInt32(U), R = FMath::FloorToInt32(V);
		const float Fu = float(U - C), Fv = float(V - R);
		const float A = FMath::Lerp(DemPiksel(C, R, bBezWciec), DemPiksel(C + 1, R, bBezWciec), Fu);
		const float B = FMath::Lerp(DemPiksel(C, R + 1, bBezWciec), DemPiksel(C + 1, R + 1, bBezWciec), Fu);
		return FMath::Lerp(A, B, Fv);
	}

	// ---- Rzeki mapy grywalnej (Tools/rzeki.py -> Data/rzeki.bin): linie co 20 m w grze ----
	TArray<FVector2D> GRzP;     // punkty wszystkich linii po kolei (cm)
	TArray<float> GRzPolowa;    // polowa szerokosci koryta w punkcie (cm)
	TArray<uint8> GRzKoniec;    // 1 = ostatni punkt linii (odcinek i..i+1 nie istnieje)
	TArray<float> GRzPoziom;    // poziom wody w punkcie (cm), liczony przy pierwszym wejsciu na mape grywalna
	TMap<int32, TArray<int32>> GRzSiatka; // komorka 512 m -> odcinki (indeks punktu poczatku), ktore moga na nia wplywac
	constexpr double RzKomorka = 51200.0;
	constexpr float RzZasieg = 30000.f; // jak daleko od brzegu rzeka zmienia teren (dolina bez pagorkow 200 m, skarpa do 120 m)

	int32 RzKlucz(int32 Cx, int32 Cy) { return Cx * 4096 + Cy; }

	void WczytajRzeki(const FString& Plik)
	{
		TArray<uint8> B;
		if (!FFileHelper::LoadFileToArray(B, *Plik) || B.Num() < 4)
		{
			UE_LOG(LogTemp, Warning, TEXT("Swiat: brak Data/rzeki.bin (Tools/rzeki.py) - mapa grywalna bez rzek."));
			return;
		}
		int64 O = 0;
		auto Czytaj = [&](void* Do, int64 Ile) { if (O + Ile > B.Num()) { return false; } FMemory::Memcpy(Do, B.GetData() + O, Ile); O += Ile; return true; };
		int32 Linii = 0;
		Czytaj(&Linii, 4);
		for (int32 l = 0; l < Linii; ++l)
		{
			int32 N = 0;
			float Polowa = 0.f;
			if (!Czytaj(&N, 4) || !Czytaj(&Polowa, 4) || N < 0)
			{
				break;
			}
			for (int32 i = 0; i < N; ++i)
			{
				float XY[2];
				if (!Czytaj(XY, 8))
				{
					break;
				}
				GRzP.Add(FVector2D(XY[0], XY[1]));
				GRzPolowa.Add(Polowa);
				GRzKoniec.Add(i == N - 1);
			}
		}
		if (GRzP.Num() != GRzKoniec.Num() || O != B.Num())
		{
			UE_LOG(LogTemp, Error, TEXT("Swiat: zly Data/rzeki.bin - mapa grywalna bez rzek."));
			GRzP.Reset(); GRzPolowa.Reset(); GRzKoniec.Reset();
			return;
		}
		for (int32 i = 0; i + 1 < GRzP.Num(); ++i)
		{
			if (GRzKoniec[i])
			{
				continue;
			}
			const FBox2D Pudlo = FBox2D(TArray<FVector2D>{ GRzP[i], GRzP[i + 1] }).ExpandBy(GRzPolowa[i] + RzZasieg);
			const int32 X0 = FMath::FloorToInt32((Pudlo.Min.X + FSwiat::Polowa) / RzKomorka), X1 = FMath::FloorToInt32((Pudlo.Max.X + FSwiat::Polowa) / RzKomorka);
			const int32 Y0 = FMath::FloorToInt32((Pudlo.Min.Y + FSwiat::Polowa) / RzKomorka), Y1 = FMath::FloorToInt32((Pudlo.Max.Y + FSwiat::Polowa) / RzKomorka);
			for (int32 Cx = X0; Cx <= X1; ++Cx)
			{
				for (int32 Cy = Y0; Cy <= Y1; ++Cy)
				{
					GRzSiatka.FindOrAdd(RzKlucz(Cx, Cy)).Add(i);
				}
			}
		}
		UE_LOG(LogTemp, Log, TEXT("Swiat: rzeki %d linii, %d punktow."), Linii, GRzP.Num());
	}

	struct FRzeka
	{
		float Odl = TNumericLimits<float>::Max(); // od brzegu (cm), ujemna w wodzie
		float Poziom = 0.f;                       // poziom wody (cm)
	};

	/** Najblizszy brzeg rzeki (w zasiegu RzZasieg). Poziom tylko gdy policzony (GRzPoziom). */
	FRzeka NajblizszaRzeka(double X, double Y)
	{
		FRzeka W;
		const TArray<int32>* Odcinki = GRzSiatka.Find(RzKlucz(FMath::FloorToInt32((X + FSwiat::Polowa) / RzKomorka), FMath::FloorToInt32((Y + FSwiat::Polowa) / RzKomorka)));
		if (!Odcinki)
		{
			return W;
		}
		const FVector2D P(X, Y);
		const bool bPoziom = GRzPoziom.Num() == GRzP.Num();
		// shortcut: przy ujsciu doplywu wygrywa blizszy brzeg, wiec poziom wody moze tam skoczyc o kilkadziesiat cm.
		for (const int32 i : *Odcinki)
		{
			const FVector2D A = GRzP[i], AB = GRzP[i + 1] - A;
			const double T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1.0), 0.0, 1.0);
			const float Odl = float(FVector2D::Distance(P, A + AB * T)) - FMath::Lerp(GRzPolowa[i], GRzPolowa[i + 1], float(T));
			if (Odl < W.Odl)
			{
				W.Odl = Odl;
				W.Poziom = bPoziom ? FMath::Lerp(GRzPoziom[i], GRzPoziom[i + 1], float(T)) : 0.f;
			}
		}
		return W;
	}

	/** Teren testowy (bez danych): kontynent z morzem, nizinami i pasmem gor. Metry. */
	float TestoweMetry(double X, double Y)
	{
		const FVector2D P(X, Y);
		const float Lad = Fbm(P / 1.5e7, 5) * 1400.f + 150.f;
		const float Grzbiet = 1.f - FMath::Abs(FMath::PerlinNoise2D(P / 4.0e6 + FVector2D(3.7, 1.1)));
		const float GdzieGory = FMath::SmoothStep(0.05f, 0.35f, Fbm(P / 3.0e7 + FVector2D(-5.2, 8.4), 3));
		return Lad + FMath::Pow(Grzbiet, 3.f) * 2800.f * GdzieGory;
	}
}

void FSwiat::Wczytaj()
{
	if (GWczytane)
	{
		return;
	}
	GWczytane = true;

	const FString Dir = FPaths::ProjectDir() / TEXT("Data");
	FString Meta;
	TArray<uint8> Bajty;
	if (!FFileHelper::LoadFileToString(Meta, *(Dir / TEXT("europa_wys.txt")))
		|| !FFileHelper::LoadFileToArray(Bajty, *(Dir / TEXT("europa_wys.r16"))))
	{
		UE_LOG(LogTemp, Warning, TEXT("Swiat: brak Data/europa_wys.r16 - teren testowy z szumu."));
		return;
	}
	TArray<FString> Pola;
	Meta.ParseIntoArrayWS(Pola);
	if (Pola.Num() < 4)
	{
		UE_LOG(LogTemp, Error, TEXT("Swiat: zly europa_wys.txt (potrzeba: szer wys min_m max_m)."));
		return;
	}
	const int32 W = FCString::Atoi(*Pola[0]), H = FCString::Atoi(*Pola[1]);
	if (W <= 1 || H <= 1 || Bajty.Num() != W * H * 2)
	{
		UE_LOG(LogTemp, Error, TEXT("Swiat: rozmiar europa_wys.r16 nie zgadza sie z %dx%d."), W, H);
		return;
	}
	GSzer = W;
	GWys = H;
	GMinM = FCString::Atof(*Pola[2]);
	GMaxM = FCString::Atof(*Pola[3]);
	GDem.SetNumUninitialized(W * H);
	FMemory::Memcpy(GDem.GetData(), Bajty.GetData(), Bajty.Num()); // little-endian jak na PC
	if (Pola.Num() >= 6)
	{
		GStart = FVector2D(FCString::Atod(*Pola[4]), FCString::Atod(*Pola[5]));
	}
	if (!FFileHelper::LoadFileToArray(GWoda, *(Dir / TEXT("europa_woda.r8"))) || GWoda.Num() != W * H)
	{
		UE_LOG(LogTemp, Warning, TEXT("Swiat: brak/zly Data/europa_woda.r8 - jeziora i rzeki niewidoczne."));
		GWoda.Reset();
	}
	if (!FFileHelper::LoadFileToArray(GPokrycie, *(Dir / TEXT("europa_pokrycie.r8"))) || GPokrycie.Num() != W * H)
	{
		UE_LOG(LogTemp, Warning, TEXT("Swiat: brak/zly Data/europa_pokrycie.r8 - drzewa jak w lesie lisciastym."));
		GPokrycie.Reset();
	}
	if (!FFileHelper::LoadFileToArray(GDetal, *(Dir / TEXT("teren_detal.rgb8"))) || GDetal.Num() != DetalN * DetalN * 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Swiat: brak Data/teren_detal.rgb8 - drzewa nie trzymaja sie falowanych granic terenu."));
		GDetal.Reset();
	}
	WczytajRzeki(Dir / TEXT("rzeki.bin"));
	UE_LOG(LogTemp, Log, TEXT("Swiat: DEM %dx%d, %.0f..%.0f m."), W, H, GMinM, GMaxM);
}

bool FSwiat::MaDane()
{
	return GDem.Num() > 0;
}

namespace
{
	// Atomowo: watek gry przelacza tryb, gdy stare zadania kafli jeszcze licza w tle (ich wyniki i tak przepadaja).
	std::atomic<bool> GRegion{ false };
	/** Baza przesady mapy grywalnej (m): gladka "podloga" terenu, siatka BazaN x BazaN (blok 16 pikseli DEM = 1 km w grze). */
	constexpr int32 BazaN = 512;
	TArray<float> GBaza;

	float MetryBezRegionu(double X, double Y, bool bBezWciec);

	/**
	 * Podloga = minimum ladu w okolicy +-7 km, wygladzone (3 x usrednianie +-2 km). Zawsze <= wysokosci DEM w danym km,
	 * wiec przesada podnosi teren od podlogi w gore (nic nie schodzi pod morze), a na calej mapie nie ma uskokow.
	 */
	void PoliczBaze()
	{
		const int32 Blok = FMath::Max(1, GSzer / BazaN);
		TArray<float> Min;
		Min.Init(TNumericLimits<float>::Max(), BazaN * BazaN);
		for (int32 R = 0; R < GWys; ++R)
		{
			for (int32 C = 0; C < GSzer; ++C)
			{
				const float M = DemPiksel(C, R);
				float& B = Min[FMath::Min(R / Blok, BazaN - 1) * BazaN + FMath::Min(C / Blok, BazaN - 1)];
				if (M > 0.f && M < B)
				{
					B = M;
				}
			}
		}
		auto Przebieg = [](const TArray<float>& Z, int32 Promien, bool bMinimum)
		{
			TArray<float> Do;
			Do.SetNumUninitialized(BazaN * BazaN);
			for (int32 R = 0; R < BazaN; ++R)
			{
				for (int32 C = 0; C < BazaN; ++C)
				{
					float Wynik = bMinimum ? TNumericLimits<float>::Max() : 0.f;
					int32 Ile = 0;
					for (int32 dr = -Promien; dr <= Promien; ++dr)
					{
						for (int32 dc = -Promien; dc <= Promien; ++dc)
						{
							const float V = Z[FMath::Clamp(R + dr, 0, BazaN - 1) * BazaN + FMath::Clamp(C + dc, 0, BazaN - 1)];
							Wynik = bMinimum ? FMath::Min(Wynik, V) : Wynik + V;
							++Ile;
						}
					}
					Do[R * BazaN + C] = bMinimum ? Wynik : Wynik / Ile;
				}
			}
			return Do;
		};
		// Promien minimum (7) >= laczny promien usredniania (3 x 2) + 1 blok dwuliniowo -> podloga <= teren.
		GBaza = Przebieg(Min, 7, true);
		for (float& B : GBaza)
		{
			B = B == TNumericLimits<float>::Max() ? 0.f : B; // otwarte morze
		}
		for (int32 k = 0; k < 3; ++k)
		{
			GBaza = Przebieg(GBaza, 2, false);
		}
	}

	float Baza(double X, double Y)
	{
		const double U = (Y + FSwiat::Polowa) / FSwiat::Bok * BazaN - 0.5;
		const double V = (FSwiat::Polowa - X) / FSwiat::Bok * BazaN - 0.5;
		const int32 C = FMath::FloorToInt32(U), R = FMath::FloorToInt32(V);
		const float Fu = float(U - C), Fv = float(V - R);
		auto Px = [](int32 c, int32 r) { return GBaza[FMath::Clamp(r, 0, BazaN - 1) * BazaN + FMath::Clamp(c, 0, BazaN - 1)]; };
		return FMath::Lerp(FMath::Lerp(Px(C, R), Px(C + 1, R), Fu), FMath::Lerp(Px(C, R + 1), Px(C + 1, R + 1), Fu), Fv);
	}

	/** Oczko siatki wygladzania DEM w pikselach (piksel = 64 m w grze). 2 dawalo jeszcze ostre zbocza przy podnozach gor. */
	constexpr int32 OczkoGladkie = 3;

	/**
	 * DEM gladko: B-spline 3. stopnia na siatce co OczkoGladkie pikseli (wezel = srednia OczkoGladkie^2 pikseli).
	 * Dwuliniowy DEM x PrzesadaRegionu dawal ostre grzbiety i zalamania na kazdym pikselu (64 m); tu zbocza sa gladkie,
	 * a ksztalty od ~200 m w gore.
	 */
	float DemGladki(double X, double Y)
	{
		constexpr int32 S = OczkoGladkie;
		const double U = ((Y + FSwiat::Polowa) / FSwiat::Bok * GSzer - 0.5 * S) / S;
		const double V = ((FSwiat::Polowa - X) / FSwiat::Bok * GWys - 0.5 * S) / S;
		const int32 C = FMath::FloorToInt32(U), R = FMath::FloorToInt32(V);
		auto Wagi = [](float t, float W[4])
		{
			W[0] = (1.f - t) * (1.f - t) * (1.f - t) / 6.f;
			W[1] = (3.f * t * t * t - 6.f * t * t + 4.f) / 6.f;
			W[2] = (-3.f * t * t * t + 3.f * t * t + 3.f * t + 1.f) / 6.f;
			W[3] = t * t * t / 6.f;
		};
		float Wu[4], Wv[4];
		Wagi(float(U - C), Wu);
		Wagi(float(V - R), Wv);
		float Suma = 0.f;
		for (int32 j = 0; j < 4; ++j)
		{
			for (int32 i = 0; i < 4; ++i)
			{
				const int32 c = S * (C - 1 + i), r = S * (R - 1 + j);
				float Wezel = 0.f;
				for (int32 b = 0; b < S; ++b)
				{
					for (int32 a = 0; a < S; ++a)
					{
						Wezel += DemPiksel(c + a, r + b, true);
					}
				}
				Suma += Wu[i] * Wv[j] * Wezel / float(S * S);
			}
		}
		return Suma;
	}

	/**
	 * Przesada rzezby nad podloga (m -> m): niskie wzniesienia x PrzesadaRegionu jak dawniej, wysokie coraz mniej
	 * (logarytmicznie, skala ZlagodzenieGor). Czyste x3 robilo z Tatr i Alp sciany 50-70 st.; teraz zbocza gor
	 * lagodnieja ku gorze, a podnoza przechodza w nizine plynnie. Gladka (ciagla pochodna), wiec bez zalaman.
	 */
	float Przesada(float D)
	{
		if (D <= 0.f)
		{
			return D * FSwiat::PrzesadaRegionu;
		}
		return FSwiat::PrzesadaRegionu * FSwiat::ZlagodzenieGor * FMath::Loge(1.f + D / FSwiat::ZlagodzenieGor);
	}

	/**
	 * Mapa grywalna bez koryt rzek (cm): przesada (Przesada()) od podlogi + pagorki ~2,5 km i ~700 m w grze
	 * (+-31 / +-5 m w grze), wygaszane przy morzu jak detal i w dolinie rzeki (Pagorkowosc 0..1).
	 * Zwraca < 0, gdy to morze / dno (wtedy bez przesady).
	 */
	float RegionBezKoryt(double X, double Y, float Pagorkowosc)
	{
		float M = MetryBezRegionu(X, Y, true);
		if (M <= 0.f)
		{
			return -1.f;
		}
		const FVector2D P(X, Y);
		// Gladka rzezba (te same 2 najwieksze oktawy pagorkow co na mapie Europy, bez drobnicy); przy brzegu jak dawniej, zeby linia brzegu sie nie przesuwala.
		float G = DemGladki(X, Y);
		G += Fbm(P / 3.0e5, 2) * (12.f + 0.06f * G) * FMath::SmoothStep(0.f, 20.f, G);
		M = FMath::Lerp(M, G, FMath::SmoothStep(0.f, 30.f, M));
		// Pagorki duze i lagodne: ~2,5 km i ~700 m w grze.
		const float Pagorki = Fbm(P / 2.5e5 + FVector2D(1.3, 7.1), 2) * 100.f + Fbm(P / 7.0e4 + FVector2D(-4.4, 2.6), 2) * 15.f;
		const float B = Baza(X, Y);
		M = B + Przesada(M - B) + Pagorki * FMath::SmoothStep(0.f, 20.f, M) * Pagorkowosc;
		return float(FMath::Max(M, 1.f) * FSwiat::MetryNaCm);
	}

	/**
	 * Poziom wody w kazdym punkcie rzeki: teren doliny (bez pagorkow) pod punktem, potem wzdluz linii minimum +-300 m
	 * i srednia +-200 m (gladko, nigdy wyzej niz najblizszy dol doliny), 60 cm pod brzegiem.
	 */
	void PoliczPoziomyRzek()
	{
		TArray<float> Teren;
		Teren.SetNumUninitialized(GRzP.Num());
		ParallelFor(GRzP.Num(), [&](int32 i) { Teren[i] = FMath::Max(RegionBezKoryt(GRzP[i].X, GRzP[i].Y, 0.f), 0.f); });
		TArray<float> Poziom;
		Poziom.SetNumUninitialized(GRzP.Num());
		for (int32 Od = 0; Od < GRzP.Num();)
		{
			int32 Do = Od;
			while (Do < GRzP.Num() - 1 && !GRzKoniec[Do])
			{
				++Do;
			}
			TArray<float> Min;
			Min.SetNumUninitialized(Do - Od + 1);
			for (int32 i = Od; i <= Do; ++i)
			{
				float V = Teren[i];
				for (int32 k = FMath::Max(Od, i - 15); k <= FMath::Min(Do, i + 15); ++k)
				{
					V = FMath::Min(V, Teren[k]);
				}
				Min[i - Od] = V;
			}
			for (int32 i = Od; i <= Do; ++i)
			{
				float Suma = 0.f;
				int32 Ile = 0;
				for (int32 k = FMath::Max(Od, i - 10); k <= FMath::Min(Do, i + 10); ++k, ++Ile)
				{
					Suma += Min[k - Od];
				}
				Poziom[i] = Suma / Ile - 60.f;
			}
			Od = Do + 1;
		}
		GRzPoziom = MoveTemp(Poziom); // przed GRegion = true: zadania w tle czytaja poziomy tylko w trybie regionu
	}
}

void FSwiat::UstawRegion(bool bWlacz)
{
	GRegion = false;
	if (bWlacz && GBaza.Num() == 0 && MaDane())
	{
		PoliczBaze(); // raz, przy pierwszym wejsciu (pod ekranem ladowania)
		PoliczPoziomyRzek();
	}
	GRegion = bWlacz && GBaza.Num() > 0;
}

float FSwiat::Wysokosc(double X, double Y, float* OdRzekiCm)
{
	if (OdRzekiCm)
	{
		*OdRzekiCm = 1.0e5f;
	}
	if (!GRegion)
	{
		return float(MetryBezRegionu(X, Y, false) * MetryNaCm);
	}
	const FRzeka Rz = NajblizszaRzeka(X, Y);
	if (OdRzekiCm)
	{
		*OdRzekiCm = FMath::Min(Rz.Odl, 1.0e5f);
	}
	// Dolina rzeki plaska (bez pagorkow) do ~200 m od brzegu.
	float Z = RegionBezKoryt(X, Y, FMath::SmoothStep(0.f, 20000.f, Rz.Odl));
	if (Z < 0.f)
	{
		return float(MetryBezRegionu(X, Y, true) * MetryNaCm); // morze
	}
	if (Rz.Odl < RzZasieg && GRzPoziom.Num())
	{
		if (Rz.Odl <= 0.f)
		{
			Z = Rz.Poziom; // lustro wody: plasko
		}
		else
		{
			// Gladka rzezba podnosi dno waskich dolin (poziom wody liczony na osi): do ~200 m od brzegu teren nie schodzi pod wode.
			const float Dno = Rz.Poziom + 100.f + Rz.Odl * 0.02f;
			Z = FMath::Lerp(FMath::Max(Z, Dno), Z, FMath::SmoothStep(0.7f * RzZasieg, RzZasieg, Rz.Odl));
			// Brzeg: maly prog 40 cm nad woda (ostra krawedz jak na zrzutach), dalej skarpa ~1:2 do terenu (25-120 m).
			const float Skarpa = FMath::Clamp((Z - Rz.Poziom) * 2.f, 2500.f, 12000.f);
			if (Rz.Odl < Skarpa)
			{
				Z = FMath::Lerp(Rz.Poziom + 40.f, Z, FMath::SmoothStep(0.f, Skarpa, Rz.Odl));
			}
		}
	}
	return Z;
}

float FSwiat::OdRzeki(double X, double Y)
{
	return GRegion ? FMath::Min(NajblizszaRzeka(X, Y).Odl, 1.0e5f) : 1.0e5f;
}

void FSwiat::Rzeki(const TArray<FVector2D>*& Punkty, const TArray<float>*& Polowy, const TArray<uint8>*& Koniec)
{
	Punkty = &GRzP;
	Polowy = &GRzPolowa;
	Koniec = &GRzKoniec;
}

namespace
{
float MetryBezRegionu(double X, double Y, bool bBezWciec)
{
	float M = FSwiat::MaDane() ? DemMetry(X, Y, bBezWciec) : TestoweMetry(X, Y);

	// Detal, ktorego nie ma w danych (piksel DEM ~ 60 m w grze): pagorki i nierownosci, mocniejsze w gorach.
	if (M > 0.f)
	{
		const FVector2D P(X, Y);
		const float Pagorki = Fbm(P / 3.0e5, 4) * (12.f + 0.06f * M);
		const float Drobne = Fbm(P / 2.5e4 + FVector2D(7.7, 2.2), 3) * 1.5f;
		// Przy brzegu detal wygasa, zeby nie robic wysepek i dziur w linii brzegowej.
		M += (Pagorki + Drobne) * FMath::SmoothStep(0.f, 20.f, M);
	}
	return M;
}
}

FVector2D FSwiat::Start()
{
	return GStart; // Krakow, liczony w Tools/dem_europa.py
}

FSwiat::EWoda FSwiat::Woda(double X, double Y)
{
	if (GWoda.Num() == 0)
	{
		return Wysokosc(X, Y) <= 0.f ? EWoda::Morze : EWoda::Lad;
	}
	const int32 C = FMath::Clamp(FMath::FloorToInt32((Y + Polowa) / Bok * GSzer), 0, GSzer - 1);
	const int32 R = FMath::Clamp(FMath::FloorToInt32((Polowa - X) / Bok * GWys), 0, GWys - 1);
	const EWoda W = EWoda(GWoda[R * GSzer + C]);
	if (!GRegion || (W != EWoda::Lad && W != EWoda::Rzeka))
	{
		return W;
	}
	// Mapa grywalna: rzeki z linii (Data/rzeki.bin) zamiast pikseli 64 m.
	return NajblizszaRzeka(X, Y).Odl < 0.f ? EWoda::Rzeka : EWoda::Lad;
}

FSwiat::EPokrycie FSwiat::Pokrycie(double X, double Y)
{
	if (GPokrycie.Num() == 0)
	{
		return EPokrycie::Lisciasty;
	}
	// Falowanie granic klas jak w M_Teren (Tools/ue_teren.py): przesuniecie w pikselach mapy
	// = (B - 0,5) * 8 + (G - 0,5) * 2, z T_Detal w kaflu 256 m; V z probki przesunietej o (0,37; 0,61).
	double DU = 0.0, DV = 0.0;
	if (GDetal.Num())
	{
		const double U = (X + Polowa) / 25600.0, V = (Y + Polowa) / 25600.0;
		DU = (DetalKanal(U, V, 2) - 0.5) * 8.0 + (DetalKanal(U, V, 1) - 0.5) * 2.0;
		DV = (DetalKanal(U + 0.37, V + 0.61, 2) - 0.5) * 8.0 + (DetalKanal(U + 0.37, V + 0.61, 1) - 0.5) * 2.0;
	}
	const int32 C = FMath::Clamp(FMath::FloorToInt32((Y + Polowa) / Bok * GSzer + DU), 0, GSzer - 1);
	const int32 R = FMath::Clamp(FMath::FloorToInt32((Polowa - X) / Bok * GWys + DV), 0, GWys - 1);
	return EPokrycie(FMath::Min<uint8>(GPokrycie[R * GSzer + C], uint8(EPokrycie::Ile) - 1));
}

float FSwiat::WysokoscKafla(double X, double Y)
{
	// Siatka co 4 m od -Polowa, trojkaty z przekatna A-C (TerenKafle.cpp, Zbuduj). Na wypuklych zboczach plaskie trojkaty
	// leza wyzej niz gladka Wysokosc, wiec bierzemy wieksza z obu.
	constexpr double K = 400.0;
	const double Gx = (X + Polowa) / K, Gy = (Y + Polowa) / K;
	const double I = FMath::FloorToDouble(Gx), J = FMath::FloorToDouble(Gy);
	const double Fx = Gx - I, Fy = Gy - J;
	const double X0 = I * K - Polowa, Y0 = J * K - Polowa;
	auto W = [](double Px, double Py) { return FMath::Max(0.0, double(Wysokosc(Px, Py))); };
	const double HA = W(X0, Y0), HB = W(X0 + K, Y0), HC = W(X0 + K, Y0 + K), HE = W(X0, Y0 + K);
	const double H = Fx >= Fy ? HA + Fx * (HB - HA) + Fy * (HC - HB) : HA + Fy * (HE - HA) + Fx * (HC - HE);
	return float(FMath::Max(H, double(Wysokosc(X, Y))));
}
