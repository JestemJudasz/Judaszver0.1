// Plik: Source/grarts2d/Osada/OsadaRodu.cpp
#include "Osada/OsadaRodu.h"
#include "Misc/FileHelper.h"
#include "Osada/Rody.h"
#include "Budowy/Budowa.h"
#include "Swiat/SwiatWysokosci.h"
#include "Swiat/TerenKafle.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Algo/Reverse.h"
#include "Osada/Osada.h"
#include "Ludzie/Ludzik.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Efekty/Efekty.h"
#include "Dzwiek/Dzwieki.h"

namespace
{
	UStaticMesh* Model(const FString& Nazwa)
	{
		return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Swiat/Budynki/%s.%s"), *Nazwa, *Nazwa));
	}
	/** Model zniszczony (_Z1 uszkodzony, _Z2 zgliszcza) z nazwy gotowego albo fazy budowy; bez niego ten sam. */
	UStaticMesh* Zniszczony(UStaticMesh* M, int32 Etap)
	{
		if (!M) { return M; }
		FString N = M->GetName();
		const int32 F = N.Find(TEXT("_F"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (F > 0 && N.Mid(F + 2).IsNumeric()) { N.LeftInline(F); }
		const FString Z = FString::Printf(TEXT("%s_Z%d"), *N, Etap);
		UStaticMesh* Out = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Swiat/Budynki/%s.%s"), *Z, *Z), nullptr, LOAD_NoWarn | LOAD_Quiet);
		return Out ? Out : M;
	}
	UStaticMesh* GotowyModel(const FRodzajBudynku& B, int32 Wariant = 0)
	{
		// Warianty domow (SM_Chata_W1..3 itd., jak ABudowa::ModelWariantu): inne dachy, kominy, strona drzwi.
		const FString Nazwa = B.Wariantow > 1 && Wariant % B.Wariantow > 0 ? FString::Printf(TEXT("%s_W%d"), *B.Model, Wariant % B.Wariantow) : B.Model;
		return Model(B.Fazy == 1 ? Nazwa : FString::Printf(TEXT("%s_F%d"), *Nazwa, B.Fazy));
	}
	constexpr int32 MaxBudynkow = 90;
	constexpr float PlacU = 1700.f;        // cm: pol dlugosci placu (wzdluz ulicy glownej)
	constexpr float PlacV = 1250.f;        // cm: pol szerokosci placu
	constexpr float UlicaPol = 180.f;      // cm: polowa szerokosci ulicy (droga gracza ma 160)
	constexpr float Podworze = 170.f;      // cm: od skraju ulicy do frontu budynku
	constexpr float Ogrod = 700.f;         // cm: ogrodek z plotem za domem
	constexpr float Segment = 400.f;       // cm: dlugosc modelu odcinka muru (SM_Palisada, SM_Mur)
	constexpr float WnetrzeDo = 11000.f;   // cm: do tylu od placu ulica glowna ma zabudowe osady
	constexpr float DlugoscDrogi = 27000.f;// cm: ulica glowna z droga do pol za brama
	constexpr float Margines = 500.f;      // cm: od dzialek do muru
	constexpr float PolBramy = 420.f;      // cm: polowa szerokosci bramy wzdluz muru
	constexpr float KrokZiemi = 150.f;     // cm: siatka ubitej ziemi
	constexpr float BliskoZiemi = 150000.f;// cm: ziemie buduje dopiero kamera blizej niz 1,5 km

	/** Za murami: gospodarstwo i las (duze obejscia), w srodku: domy, rzemioslo, kosciol, targ, magazyny. */
	bool ZaMurami(const FRodzajBudynku& B)
	{
		switch (B.Funkcja)
		{
		case EFunkcja::Drwal: case EFunkcja::Borowy: case EFunkcja::Kamieniolom: case EFunkcja::Zbieracz:
		case EFunkcja::Mysliwy: case EFunkcja::Owczarnia: case EFunkcja::Rybak: case EFunkcja::Pole:
			return true;
		default:
			return B.Model.Contains(TEXT("Mlyn")) || B.Model.Contains(TEXT("Zagroda")) || B.Model.Contains(TEXT("Stajnia")) || B.Model.Contains(TEXT("Kopalnia"));
		}
	}
	/** Przy placu: kosciol, magazyny, targ, karczma. */
	bool PrzyPlacu(const FRodzajBudynku& B)
	{
		return B.Religia() || B.Funkcja == EFunkcja::Magazyn || B.Model.Contains(TEXT("Targ")) || B.Model.Contains(TEXT("Karczma"));
	}

	FVector2D Lewa(const FVector2D& D) { return FVector2D(-D.Y, D.X); }

	/** Prostokat obrocony: srodek, os X (jednostkowa), polowy bokow. */
	struct FProst
	{
		FVector2D C = FVector2D::ZeroVector, Os = FVector2D(1.0, 0.0);
		float Hx = 0.f, Hy = 0.f;
		float Promien() const { return FMath::Sqrt(Hx * Hx + Hy * Hy); }
		FVector2D Rog(int32 k) const { return C + Os * ((k == 0 || k == 3) ? -Hx : Hx) + Lewa(Os) * (k < 2 ? -Hy : Hy); }
	};
	bool Nachodza(const FProst& A, const FProst& B)
	{
		if (FVector2D::DistSquared(A.C, B.C) > FMath::Square(A.Promien() + B.Promien()))
		{
			return false;
		}
		const FVector2D Osie[4] = { A.Os, Lewa(A.Os), B.Os, Lewa(B.Os) };
		const FVector2D D = B.C - A.C;
		for (const FVector2D& L : Osie)
		{
			const double Ra = A.Hx * FMath::Abs(A.Os | L) + A.Hy * FMath::Abs(Lewa(A.Os) | L);
			const double Rb = B.Hx * FMath::Abs(B.Os | L) + B.Hy * FMath::Abs(Lewa(B.Os) | L);
			if (FMath::Abs(D | L) > Ra + Rb)
			{
				return false;
			}
		}
		return true;
	}

	float Dlugosc(const TArray<FVector2D>& P)
	{
		double L = 0.0;
		for (int32 i = 0; i + 1 < P.Num(); ++i) { L += FVector2D::Distance(P[i], P[i + 1]); }
		return float(L);
	}
	/** Punkt i kierunek linii S cm od poczatku (za koncem: przedluzenie ostatniego odcinka). */
	void NaLinii(const TArray<FVector2D>& P, float S, FVector2D& Q, FVector2D& T)
	{
		for (int32 i = 0; i + 1 < P.Num(); ++i)
		{
			const FVector2D D = P[i + 1] - P[i];
			const float L = FMath::Max(float(D.Size()), 1.f);
			if (S <= L || i + 2 == P.Num())
			{
				T = D / L;
				Q = P[i] + T * S;
				return;
			}
			S -= L;
		}
		Q = P.Num() ? P[0] : FVector2D::ZeroVector;
		T = FVector2D(1.0, 0.0);
	}
	float OdOdcinka(FVector2D X, FVector2D A, FVector2D B)
	{
		return FMath::PointDistToSegment(FVector(X, 0.0), FVector(A, 0.0), FVector(B, 0.0));
	}
	bool Przeciecie(FVector2D A, FVector2D B, FVector2D C, FVector2D D, FVector2D& X)
	{
		const FVector2D R = B - A, S = D - C;
		const double Mian = R ^ S;
		if (FMath::Abs(Mian) < 1e-6) { return false; }
		const double T = ((C - A) ^ S) / Mian, U = ((C - A) ^ R) / Mian;
		if (T < 0.0 || T > 1.0 || U < 0.0 || U > 1.0) { return false; }
		X = A + R * T;
		return true;
	}
	/** Otoczka wypukla (Andrew), przeciwnie do zegara. */
	TArray<FVector2D> Otoczka(TArray<FVector2D> P)
	{
		P.Sort([](const FVector2D& A, const FVector2D& B) { return A.X < B.X || (A.X == B.X && A.Y < B.Y); });
		if (P.Num() < 3) { return P; }
		TArray<FVector2D> H;
		H.SetNum(2 * P.Num());
		int32 k = 0;
		auto Kr = [](FVector2D O, FVector2D A, FVector2D B) { return (A - O) ^ (B - O); };
		for (int32 i = 0; i < P.Num(); ++i)
		{
			while (k >= 2 && Kr(H[k - 2], H[k - 1], P[i]) <= 0.0) { --k; }
			H[k++] = P[i];
		}
		for (int32 i = P.Num() - 2, t = k + 1; i >= 0; --i)
		{
			while (k >= t && Kr(H[k - 2], H[k - 1], P[i]) <= 0.0) { --k; }
			H[k++] = P[i];
		}
		H.SetNum(k - 1);
		return H;
	}
	/** Punkt w otoczce (przeciwnie do zegara) poszerzonej o Zapas cm. */
	bool WOtoczce(const TArray<FVector2D>& H, FVector2D P, float Zapas)
	{
		for (int32 i = 0; i < H.Num(); ++i)
		{
			const FVector2D A = H[i], B = H[(i + 1) % H.Num()];
			const FVector2D D = (B - A).GetSafeNormal();
			if (((P - A) | Lewa(D)) < -Zapas) { return false; }
		}
		return H.Num() >= 3;
	}
	float Los01(uint32 H) { return float(H % 10007u) / 10007.f; }

	/** Dzialka: budynek (z odstepem po bokach) i ogrodek za nim. */
	FProst DzialkaProst(const AOsadaRodu::FDzialka& D)
	{
		const FVector2D Pol = FRodzajBudynku::DoBudowania()[D.Typ]->Polowa;
		const float G = D.bOgrod ? Ogrod : 0.f;
		return { D.P - D.Przod * (G * 0.5f), Lewa(D.Przod), float(Pol.X) + 120.f, float(Pol.Y) + G * 0.5f + 60.f };
	}
}

AOsadaRodu::FSprawdzenie AOsadaRodu::Sprawdz() const
{
	FSprawdzenie S;
	S.Chcianych = Chcianych;
	S.Budynkow = Dzialki.Num();
	S.Bram = Bram;
	S.Baszt = Baszt;
	S.Odcinkow = Odcinkow;
	S.Ulic = UliceZiemi.Num();
	for (int32 i = 0; i < Dzialki.Num(); ++i)
	{
		const FProst A = DzialkaProst(Dzialki[i]);
		for (int32 j = i + 1; j < Dzialki.Num(); ++j)
		{
			S.Nachodzi += Nachodza(A, DzialkaProst(Dzialki[j])) ? 1 : 0;
		}
		for (int32 r = 0; !Dzialki[i].bZewn && r < 4; ++r)
		{
			if (!WOtoczce(ObrysMuru, A.Rog(r), 0.f)) { ++S.PozaMurem; break; }
		}
	}
	return S;
}

void AOsadaRodu::ZapiszPlan(const FString& Plik) const
{
	FString T;
	for (int32 u = 0; u < UliceZiemi.Num(); ++u)
	{
		T += FString::Printf(TEXT("U %.0f"), UliceSzer[u]);
		for (const FVector2D& P : UliceZiemi[u]) { T += FString::Printf(TEXT(" %.0f %.0f"), P.X - Srodek.X, P.Y - Srodek.Y); }
		T += TEXT("\n");
	}
	T += TEXT("O");
	for (const FVector2D& P : ObrysMuru) { T += FString::Printf(TEXT(" %.0f %.0f"), P.X - Srodek.X, P.Y - Srodek.Y); }
	T += TEXT("\n");
	for (const FDzialka& D : Dzialki)
	{
		const FRodzajBudynku& B = *FRodzajBudynku::DoBudowania()[D.Typ];
		T += FString::Printf(TEXT("D %s %.0f %.0f %.3f %.3f %.0f %.0f %d %d\n"), *B.Model, D.P.X - Srodek.X, D.P.Y - Srodek.Y, D.Przod.X, D.Przod.Y, B.Polowa.X, B.Polowa.Y, D.bOgrod ? 1 : 0, D.bZewn ? 1 : 0);
	}
	FFileHelper::SaveStringToFile(T, *Plik);
}

AOsadaRodu::AOsadaRodu()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Korzen"));
	SetCanBeDamaged(false);
}

void AOsadaRodu::Karczuj(ATerenKafle* Teren, FVector2D P, float Promien)
{
	const FIntVector K(FMath::RoundToInt32(P.X / 100.0), FMath::RoundToInt32(P.Y / 100.0), FMath::RoundToInt32(Promien / 50.f));
	if (Teren && !Karczowane.Contains(K))
	{
		Karczowane.Add(K);
		Teren->Wykarczuj(P, Promien);
	}
}

void AOsadaRodu::Odswiez(const FRod& R, EPora Pora)
{
	Srodek = R.Gdzie;
	Barwa = R.Barwa;
	Rodzin = R.Rodzin;
	Woje = R.Woje;
	PoraTeraz = Pora;
	if (R.Budynkow == Ostatnio && R.Mury == OstatnieMury && R.Buduje == OstatnioBuduje && int32(Pora) == OstatniaPora)
	{
		return;
	}
	if (OstatniRod.Get() != &R)
	{
		OstatniRod = MakeShared<FRod>(R);
	}
	if (int32(Pora) != OstatniaPora && Pozary.Num() == 0 && !bOblezona)
	{
		// Nowa pora roku: rod naprawia bramy i odbudowuje dwa spalone budynki.
		BramyO.Reset();
		int32 Odbudowa = 2;
		for (auto It = Zniszczone.CreateIterator(); It && Odbudowa > 0; ++It, --Odbudowa) { It.RemoveCurrent(); }
	}
	Ostatnio = R.Budynkow;
	OstatnieMury = R.Mury;
	OstatnioBuduje = R.Buduje;
	OstatniaPora = int32(Pora);
	Miejsca.Reset();
	ATerenKafle* Teren = nullptr;
	for (TActorIterator<ATerenKafle> It(GetWorld()); It; ++It) { Teren = *It; break; }
	auto Z = [](FVector2D P) { return FSwiat::Wysokosc(P.X, P.Y); };

	// Instancje: wszystko zbierane po modelach, na koncu jedno AddInstances na model.
	TMap<UStaticMesh*, TArray<FTransform>> Nowe;
	auto Dodaj = [&Nowe](UStaticMesh* M, FVector Gdzie, float Yaw, FVector Skala = FVector::OneVector, float Pitch = 0.f)
	{
		if (M) { Nowe.FindOrAdd(M).Add(FTransform(FRotator(Pitch, Yaw, 0.f), Gdzie, Skala)); }
	};
	auto YawFrontu = [](FVector2D Przod) { return FMath::RadiansToDegrees(FMath::Atan2(-Przod.X, Przod.Y)); }; // +Y modelu = front
	auto YawOsi = [](FVector2D D) { return FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X)); };

	// ---------------- ulice (z nazwy rodu: te same przy kazdym odswiezeniu) ----------------
	FRandomStream LosU(int32(GetTypeHash(R.Nazwa) ^ 0x5157u));
	const float Kier = LosU.FRandRange(0.f, 2.f * PI);
	const FVector2D Ou(FMath::Cos(Kier), FMath::Sin(Kier)), Ov = Lewa(Ou);
	OsPlacu = Ou;
	auto W = [&](float U, float V) { return Srodek + Ou * U + Ov * V; };
	struct FUlica { TArray<FVector2D> P; float Pol = UlicaPol; int32 Glowna = -1; bool bZewn = false; };
	TArray<FUlica> Ulice;
	for (int32 k = 0; k < 2; ++k)
	{
		// Ulica glowna w dwie strony od placu, lagodnie wygieta; dalej droga do pol.
		const float Zn = k == 0 ? 1.f : -1.f;
		const float Amp = LosU.FRandRange(150.f, 450.f), Fala = LosU.FRandRange(5000.f, 8000.f), Faza = LosU.FRandRange(0.f, 2.f * PI);
		FUlica U;
		for (float S = 0.f; S <= DlugoscDrogi + 1.f; S += 250.f)
		{
			const float V = Amp * (FMath::Sin(2.f * PI * S / Fala + Faza) - FMath::Sin(Faza)) * FMath::Min(1.f, S / 2500.f);
			U.P.Add(W(Zn * (PlacU + S), V));
		}
		Ulice.Add(U);
	}
	for (int32 k = 0; k < 2; ++k)
	{
		// Boczne uliczki w osadzie i drogi polne za brama: odchodza od glownej w obie strony, lekko skosem i lukiem.
		for (const float S0 : { 2700.f, 6000.f, 9300.f, 14800.f, 21000.f })
		{
			for (const float Strona : { 1.f, -1.f })
			{
				const float S = S0 + LosU.FRandRange(-400.f, 400.f);
				const float Skret = LosU.FRandRange(-12.f, 12.f), Luk = LosU.FRandRange(-1.f, 1.f);
				const float Dl = S0 > WnetrzeDo ? LosU.FRandRange(6000.f, 9000.f) : LosU.FRandRange(4200.f, 5600.f);
				FVector2D Q, T;
				NaLinii(Ulice[k].P, S, Q, T);
				FVector2D D = (Lewa(T) * Strona).GetRotated(Skret);
				FUlica U;
				U.Glowna = k;
				U.bZewn = S0 > WnetrzeDo;
				U.Pol = U.bZewn ? 150.f : UlicaPol;
				for (float Dd = 0.f; Dd <= Dl; Dd += 250.f)
				{
					U.P.Add(Q);
					D = D.GetRotated(Luk);
					Q += D * 250.f;
				}
				Ulice.Add(U);
			}
		}
	}
	const int32 PierwszyPlac = Ulice.Num();
	for (const float Strona : { 1.f, -1.f })
	{
		FUlica U; // brzeg placu (nierysowany): budynki frontem do placu
		U.Pol = 0.f;
		for (float Uu = -PlacU + 150.f; Uu <= PlacU - 149.f; Uu += 250.f) { U.P.Add(W(Uu, Strona * PlacV)); }
		Ulice.Add(U);
	}
	TArray<FProst> Jezdnie; // odcinki ulic jako przeszkody dla dzialek
	for (const FUlica& U : Ulice)
	{
		for (int32 i = 0; U.Pol > 0.f && i + 1 < U.P.Num(); ++i)
		{
			const FVector2D D = U.P[i + 1] - U.P[i];
			const float Dl = FMath::Max(float(D.Size()), 1.f);
			Jezdnie.Add({ (U.P[i] + U.P[i + 1]) * 0.5, D / Dl, Dl * 0.5f + 20.f, U.Pol + 40.f });
		}
	}
	const FProst PlacP{ Srodek, Ou, PlacU, PlacV };
	struct FFront { int32 Ulica; float Strona, Od, Do, Kursor; bool bPlac; };
	auto Fronty = [&](bool bZewn, const TArray<FVector2D>* Obrys)
	{
		TArray<FFront> F;
		for (int32 u = 0; u < Ulice.Num(); ++u)
		{
			const bool bPlac = u >= PierwszyPlac;
			if (bPlac && bZewn) { continue; }
			if (u >= 2 && !bPlac && Ulice[u].bZewn != bZewn) { continue; }
			float Od = Ulice[u].Glowna >= 0 ? Ulice[u].Pol + 120.f : 0.f;
			float Do = Dlugosc(Ulice[u].P);
			if (u < 2)
			{
				if (!bZewn)
				{
					Do = WnetrzeDo;
				}
				else if (Obrys)
				{
					// za brama: od miejsca, gdzie droga wychodzi z obrysu muru
					FVector2D Q, T;
					for (Od = 0.f; Od < Do; Od += 250.f)
					{
						NaLinii(Ulice[u].P, Od, Q, T);
						if (!WOtoczce(*Obrys, Q, Margines)) { break; }
					}
				}
			}
			for (const float St : { 1.f, -1.f })
			{
				if (bPlac && St != (u == PierwszyPlac ? 1.f : -1.f)) { continue; }
				F.Add({ u, St, Od, Do, Od, bPlac });
			}
		}
		return F;
	};
	const TArray<const FRodzajBudynku*>& L = FRodzajBudynku::DoBudowania();

	// ---------------- budynki do postawienia (Typ, Nr) ----------------
	struct FSztuka { int32 Typ, Nr; UStaticMesh* M; bool bBudowa, bZewn; };
	TArray<FSztuka> Sztuki;
	const uint32 Ziarno = GetTypeHash(R.Nazwa);
	for (int32 i = 0; i < L.Num(); ++i)
	{
		for (int32 n = 0; n < R.Ile(i) && Sztuki.Num() < MaxBudynkow; ++n)
		{
			UStaticMesh* M = GotowyModel(*L[i], int32(HashCombine(Ziarno, HashCombine(i, n)) % 4u));
			if (!M) { M = GotowyModel(*L[i]); }
			if (M) { Sztuki.Add({ i, n, M, false, ZaMurami(*L[i]) }); }
		}
	}
	if (L.IsValidIndex(R.Buduje) && L[R.Buduje]->Fazy > 1)
	{
		// Plac budowy: budowany budynek w polowie (faza ze srodka budowy) na swojej przyszlej dzialce.
		const FRodzajBudynku& B = *L[R.Buduje];
		if (UStaticMesh* M = Model(FString::Printf(TEXT("%s_F%d"), *B.Model, FMath::Max(1, B.Fazy / 2))))
		{
			Sztuki.Add({ R.Buduje, R.Ile(R.Buduje), M, true, ZaMurami(B) });
		}
	}
	TArray<FDzialka> Stare = MoveTemp(Dzialki);
	Dzialki.Reset();
	auto Stara = [&Stare](const FSztuka& S) { return Stare.IndexOfByPredicate([&S](const FDzialka& D) { return D.Typ == S.Typ && D.Nr == S.Nr; }); };
	auto Kolejnosc = [&](TArray<int32>& Lista)
	{
		// kosciol, magazyny i targ najpierw (zajmuja plac), reszta przemieszana (domy miedzy warsztatami)
		Lista.Sort([&](int32 A, int32 B)
		{
			const bool Pa = PrzyPlacu(*L[Sztuki[A].Typ]), Pb = PrzyPlacu(*L[Sztuki[B].Typ]);
			if (Pa != Pb) { return Pa; }
			return HashCombine(Ziarno, HashCombine(Sztuki[A].Typ, Sztuki[A].Nr)) < HashCombine(Ziarno, HashCombine(Sztuki[B].Typ, Sztuki[B].Nr));
		});
	};
	// Szukanie miejsca: na kazdym froncie pierwsze wolne miejsce od kursora, wygrywa najblizsze placu.
	auto Szukaj = [&](const FSztuka& S, TArray<FFront>& Fr, const TArray<FProst>& Przeszkody, const TArray<FVector2D>* Obrys) -> bool
	{
		const FRodzajBudynku& B = *L[S.Typ];
		const float Wx = B.Polowa.X, Dy = B.Polowa.Y;
		int32 Naj = -1;
		float NajOcena = TNumericLimits<float>::Max(), NajS = 0.f;
		FDzialka NajD;
		for (int32 f = 0; f < Fr.Num(); ++f)
		{
			FFront& F = Fr[f];
			const FUlica& U = Ulice[F.Ulica];
			for (int32 Proba = 0; Proba < 200; ++Proba)
			{
				const float Sc = F.Kursor + Wx + 60.f;
				if (Sc + Wx > F.Do) { break; }
				FVector2D Q, T;
				NaLinii(U.P, Sc, Q, T);
				const FVector2D N = Lewa(T) * F.Strona; // od ulicy do budynku
				FDzialka D;
				D.Typ = S.Typ;
				D.Nr = S.Nr;
				D.Przod = -N;
				D.bOgrod = B.Funkcja == EFunkcja::Mieszkalny;
				D.bZewn = S.bZewn;
				D.P = Q + N * (U.Pol + Podworze + Dy);
				const FProst P = DzialkaProst(D);
				bool bOk = !Nachodza(P, PlacP);
				for (int32 j = 0; bOk && j < Jezdnie.Num(); ++j) { bOk = !Nachodza(P, Jezdnie[j]); }
				for (int32 j = 0; bOk && j < Przeszkody.Num(); ++j) { bOk = !Nachodza(P, Przeszkody[j]); }
				for (int32 r = 0; bOk && Obrys && r < 4; ++r) { bOk = !WOtoczce(*Obrys, P.Rog(r), Margines + 200.f); }
				if (bOk)
				{
					float Ocena = float(FVector2D::Distance(D.P, Srodek));
					if (F.bPlac) { Ocena += PrzyPlacu(B) ? -3000.f : 1200.f; }
					if (Ocena < NajOcena) { NajOcena = Ocena; Naj = f; NajD = D; NajS = Sc; }
					break;
				}
				F.Kursor += 150.f;
			}
		}
		if (Naj < 0)
		{
			if (!S.bZewn) { return false; } // w srodku brak miejsca: pojdzie na przedmiescie
			UE_LOG(LogTemp, Warning, TEXT("OsadaRodu: %s - brak miejsca dla %s (%s)"), *R.Nazwa, *B.Model, S.bZewn ? TEXT("za murem") : TEXT("w srodku"));
			return false;
		}
		Fr[Naj].Kursor = NajS + Wx + 60.f + 300.f * Los01(HashCombine(Ziarno, HashCombine(S.Typ * 7 + 1, S.Nr)));
		Dzialki.Add(NajD);
		return true;
	};

	// 1. W srodku: stare dzialki zostaja, nowe szukaja miejsca.
	TArray<FProst> Zajete;
	TArray<int32> DoPostawienia;
	for (int32 k = 0; k < Sztuki.Num(); ++k)
	{
		if (Sztuki[k].bZewn) { continue; }
		const int32 s = Stara(Sztuki[k]);
		if (s >= 0 && !Stare[s].bZewn) { Zajete.Add(DzialkaProst(Stare[s])); Dzialki.Add(Stare[s]); }
		else { DoPostawienia.Add(k); }
	}
	Kolejnosc(DoPostawienia);
	{
		TArray<FFront> Fr = Fronty(false, nullptr);
		for (int32 k : DoPostawienia)
		{
			if (Szukaj(Sztuki[k], Fr, Zajete, nullptr)) { Zajete.Add(DzialkaProst(Dzialki.Last())); }
			else { Sztuki[k].bZewn = true; } // przedmiescie: dom lub warsztat za brama
		}
	}
	// 2. Obrys muru: otoczka dzialek (z ogrodami) i placu z marginesem, bez prawie prostych zalaman.
	TArray<FVector2D> Pkt;
	for (int32 r = 0; r < 4; ++r) { Pkt.Add(FProst{ Srodek, Ou, PlacU + 700.f, PlacV + 700.f }.Rog(r)); }
	for (FProst P : Zajete)
	{
		P.Hx += Margines;
		P.Hy += Margines;
		for (int32 r = 0; r < 4; ++r) { Pkt.Add(P.Rog(r)); }
	}
	TArray<FVector2D> Obrys = Otoczka(Pkt);
	for (bool bZmiana = true; bZmiana && Obrys.Num() > 5;)
	{
		bZmiana = false;
		for (int32 i = 0; i < Obrys.Num(); ++i)
		{
			const int32 N = Obrys.Num();
			if (OdOdcinka(Obrys[i], Obrys[(i + N - 1) % N], Obrys[(i + 1) % N]) < 180.f)
			{
				Obrys.RemoveAt(i);
				bZmiana = true;
				break;
			}
		}
	}
	// 3. Za murami: stare zostaja, jesli nie weszly w mur ani na nowe domy; nowe przy drogach za bramami.
	TArray<FProst> Wszystkie = Zajete;
	DoPostawienia.Reset();
	for (int32 k = 0; k < Sztuki.Num(); ++k)
	{
		if (!Sztuki[k].bZewn) { continue; }
		const int32 s = Stara(Sztuki[k]);
		bool bOk = s >= 0 && Stare[s].bZewn;
		if (bOk)
		{
			const FProst P = DzialkaProst(Stare[s]);
			for (int32 r = 0; bOk && r < 4; ++r) { bOk = !WOtoczce(Obrys, P.Rog(r), Margines + 200.f); }
			for (int32 j = 0; bOk && j < Wszystkie.Num(); ++j) { bOk = !Nachodza(P, Wszystkie[j]); }
			if (bOk) { Wszystkie.Add(P); Dzialki.Add(Stare[s]); }
		}
		if (!bOk) { DoPostawienia.Add(k); }
	}
	Kolejnosc(DoPostawienia);
	{
		TArray<FFront> Fr = Fronty(true, &Obrys);
		for (int32 k : DoPostawienia)
		{
			if (Szukaj(Sztuki[k], Fr, Wszystkie, &Obrys)) { Wszystkie.Add(DzialkaProst(Dzialki.Last())); }
		}
	}

	// ---------------- budynki, ogrodki, ploty ----------------
	static const TCHAR* Uprawa[4] = { TEXT("Zasiane"), TEXT("Rosnie"), TEXT("Dojrzale"), TEXT("Scierniska") };
	const int32 PoraI = FMath::Clamp(int32(PoraTeraz), 0, 3);
	UStaticMesh* Plot = Model(TEXT("SM_Plot"));
	UStaticMesh* Palik = Model(TEXT("SM_Plot_Palik"));
	UStaticMesh* Grzadki = Model(FString::Printf(TEXT("SM_Pole_Kafel_%s"), Uprawa[PoraI]));
	UStaticMesh* UprawaPola = Model(FString::Printf(TEXT("SM_Pole_%s"), Uprawa[PoraI]));
	auto Plotek = [&](FVector2D A, FVector2D B)
	{
		const float Dl = float(FVector2D::Distance(A, B));
		const int32 N = FMath::Max(1, FMath::RoundToInt32(Dl / 200.f));
		const FVector2D D = (B - A) / Dl;
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D P = A + D * (Dl * i / N);
			Dodaj(Plot, FVector(P, Z(P)), YawOsi(D), FVector(Dl / N / 200.f, 1.f, 1.f));
		}
		Dodaj(Palik, FVector(B, Z(B)), YawOsi(D));
	};
	Podworza.Reset();
	int32 WSrodku = 0;
	for (const FDzialka& D : Dzialki)
	{
		const FSztuka* S = Sztuki.FindByPredicate([&D](const FSztuka& X) { return X.Typ == D.Typ && X.Nr == D.Nr; });
		if (!S) { continue; }
		const FRodzajBudynku& B = *L[D.Typ];
		const FVector2D Os = Lewa(D.Przod);
		const float Yaw = YawFrontu(D.Przod) + 5.f * (Los01(HashCombine(Ziarno, HashCombine(D.Typ * 13 + 5, D.Nr))) - 0.5f);
		// Na najwyzszym punkcie obrysu (podloga nad trawa), podmurowka schodzi w dol po nizszej stronie - jak u gracza.
		float Zb = Z(D.P);
		if (B.Funkcja != EFunkcja::Pole)
		{
			for (int32 i = -1; i <= 1; ++i)
			{
				for (int32 j = -1; j <= 1; ++j)
				{
					const FVector2D Q = D.P + Os * (B.Polowa.X * i) + D.Przod * (B.Polowa.Y * j);
					Zb = FMath::Max(Zb, Z(Q) - 20.f);
				}
			}
			Podworza.Add({ D.P, D.Przod, B.Polowa });
		}
		const int32* Zn = Zniszczone.Find(FIntPoint(D.Typ, D.Nr));
		Dodaj(Zn ? Zniszczony(S->M, *Zn) : S->M, FVector(D.P, Zb), Yaw);
		if (B.Funkcja == EFunkcja::Pole && !S->bBudowa)
		{
			Dodaj(UprawaPola, FVector(D.P, Zb), Yaw); // uprawa wedlug pory roku, jak u gracza
		}
		if (D.bOgrod)
		{
			// Ogrodek za domem: grzadki (kafel pola wedlug pory roku) i plot z zerdzi.
			const float Gx = B.Polowa.X, Gy = Ogrod * 0.5f - 40.f;
			const FVector2D G = D.P - D.Przod * (B.Polowa.Y + 80.f + Gy);
			Dodaj(Grzadki, FVector(G, Z(G)), Yaw, FVector((2.f * Gx - 80.f) / 500.f, (2.f * Gy - 80.f) / 500.f, 1.f));
			const FVector2D R0 = D.P - D.Przod * (B.Polowa.Y + 40.f), R1 = G - D.Przod * Gy;
			Plotek(R0 - Os * Gx, R1 - Os * Gx);
			Plotek(R1 - Os * Gx, R1 + Os * Gx);
			Plotek(R1 + Os * Gx, R0 + Os * Gx);
			Karczuj(Teren, G, FMath::Max(Gx, Gy) * 1.2f);
		}
		Karczuj(Teren, D.P, FMath::Max(B.Polowa.X, B.Polowa.Y) * 1.25f);
		if (!Zn || *Zn < 2)
		{
			Miejsca.Add({ D.P, &B, S->bBudowa, D.Przod }); // w zgliszczach nikt nie pracuje
		}
		WSrodku += D.bZewn ? 0 : 1;
	}

	// ---------------- plac: studnia, choragiew ----------------
	{
		const FVector2D P = W(PlacU * 0.62f, PlacV * 0.6f);
		Dodaj(Model(TEXT("SM_Studnia")), FVector(P, Z(P)), YawOsi(Ou));
		Karczuj(Teren, Srodek, PlacU + 300.f);
	}
	if (!Choragiew)
	{
		if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Swiat/Ludzie/SM_Ludz_Choragiew.SM_Ludz_Choragiew")))
		{
			Choragiew = NewObject<UStaticMeshComponent>(this);
			Choragiew->SetStaticMesh(M);
			Choragiew->SetMobility(EComponentMobility::Movable);
			Choragiew->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Choragiew->SetupAttachment(RootComponent);
			Choragiew->RegisterComponent();
			Choragiew->SetWorldLocation(FVector(R.Gdzie, Z(R.Gdzie) + 150.f));
			Choragiew->SetRelativeScale3D(FVector(4.f)); // choragiew woja powiekszona na maszt placu
			Choragiew->SetCustomPrimitiveDataVector3(0, FVector(R.Barwa.R, R.Barwa.G, R.Barwa.B));
		}
	}

	// ---------------- ulice do narysowania: tak daleko, jak siega zabudowa ----------------
	UliceZiemi.Reset();
	UliceSzer.Reset();
	TArray<float> Zasieg;
	Zasieg.Init(-1.f, Ulice.Num());
	for (const FDzialka& D : Dzialki)
	{
		const FVector2D Pol = L[D.Typ]->Polowa;
		for (int32 u = 0; u < PierwszyPlac; ++u)
		{
			// Dzialka przy ulicy u: front (P + Przod * glebokosc) lezy przy jezdni.
			const FVector2D F = D.P + D.Przod * (Pol.Y + Podworze + Ulice[u].Pol);
			float S = 0.f;
			for (int32 i = 0; i + 1 < Ulice[u].P.Num(); ++i)
			{
				const FVector2D A = Ulice[u].P[i], B = Ulice[u].P[i + 1];
				if (OdOdcinka(F, A, B) < 150.f) { Zasieg[u] = FMath::Max(Zasieg[u], S + float(FVector2D::Distance(A, B))); }
				S += float(FVector2D::Distance(A, B));
			}
		}
	}
	for (int32 u = 0; u < 2; ++u)
	{
		// glowna: przez brame i kawalek dalej, do ostatniego obejscia za murem
		FVector2D Q, T;
		float Wyjscie = 0.f;
		for (; Wyjscie < DlugoscDrogi; Wyjscie += 250.f)
		{
			NaLinii(Ulice[u].P, Wyjscie, Q, T);
			if (!WOtoczce(Obrys, Q, 0.f)) { break; }
		}
		Zasieg[u] = FMath::Max(Zasieg[u] + 800.f, Wyjscie + 3000.f);
	}
	for (int32 u = 2; u < PierwszyPlac; ++u)
	{
		const FUlica& U = Ulice[u];
		float Dl = Zasieg[u] > 0.f ? Zasieg[u] + 500.f : 0.f;
		// pusta uliczka w zabudowanej czesci: krotki przesmyk miedzy domami (do ogrodow)
		const FVector2D Poczatek = U.P[0];
		if (Dl <= 0.f && !U.bZewn && WOtoczce(Obrys, Poczatek, -1500.f)) { Dl = 1300.f; }
		if (Dl > 0.f) { Zasieg[u] = Dl; }
	}
	for (int32 u = 0; u < PierwszyPlac; ++u)
	{
		if (Zasieg[u] <= 0.f) { continue; }
		TArray<FVector2D> P;
		float S = 0.f;
		for (int32 i = 0; i < Ulice[u].P.Num() && S <= Zasieg[u]; ++i)
		{
			P.Add(Ulice[u].P[i]);
			if (i + 1 < Ulice[u].P.Num()) { S += float(FVector2D::Distance(Ulice[u].P[i], Ulice[u].P[i + 1])); }
		}
		if (P.Num() < 2) { continue; }
		for (int32 i = 0; i < P.Num(); i += 2) { Karczuj(Teren, P[i], Ulice[u].Pol + 220.f); }
		UliceZiemi.Add(MoveTemp(P));
		UliceSzer.Add(Ulice[u].Pol);
	}

	// ---------------- mury po obrysie: odcinki, baszty w rogach, schody, bramy na ulicy glownej ----------------
	Odcinkow = 0;
	Baszt = 0;
	Bram = 0;
	ObrysMuru = Obrys;
	Chcianych = Sztuki.Num();
	const TArray<FBrama> StareBramy = MoveTemp(BramyO);
	BramyO.Reset();
	if (R.Mury > 0 && Obrys.Num() >= 3)
	{
		const bool bKamien = R.Mury >= 2;
		bMuryKamienne = bKamien;
		UStaticMesh* Odc = Model(bKamien ? TEXT("SM_Mur") : TEXT("SM_Palisada"));
		UStaticMesh* Schody = Model(bKamien ? TEXT("SM_Mur_Schody") : TEXT("SM_Palisada_Schody"));
		UStaticMesh* Wieza = Model(bKamien ? TEXT("SM_Mur_Wieza") : TEXT("SM_Palisada_Wieza"));
		UStaticMesh* BramaM = Model(bKamien ? TEXT("SM_Mur_Brama") : TEXT("SM_Palisada_Brama"));
		// Obwod zgodnie z zegarem: wtedy +Y modelu (lewa strona kierunku) patrzy na zewnatrz osady.
		TArray<FVector2D> Cw = Obrys;
		Algo::Reverse(Cw);
		const int32 N = Cw.Num();
		TArray<float> Luk;
		Luk.Add(0.f);
		for (int32 i = 0; i < N; ++i) { Luk.Add(Luk.Last() + float(FVector2D::Distance(Cw[i], Cw[(i + 1) % N]))); }
		const float Obwod = Luk.Last();
		auto NaObwodzie = [&](float A, FVector2D& P, FVector2D& T)
		{
			A = FMath::Fmod(A + Obwod, Obwod);
			int32 e = 0;
			while (e + 1 < N && Luk[e + 1] <= A) { ++e; }
			T = (Cw[(e + 1) % N] - Cw[e]).GetSafeNormal();
			P = Cw[e] + T * (A - Luk[e]);
		};
		auto ZNajnizej = [&](FVector2D P, float R0)
		{
			float H = Z(P);
			for (int32 k = 0; k < 4; ++k) { H = FMath::Min(H, Z(P + FVector2D(FMath::Cos(k * PI * 0.5f), FMath::Sin(k * PI * 0.5f)) * R0)); }
			return H - 10.f;
		};
		TArray<float> Bramy;
		for (int32 k = 0; k < 2; ++k)
		{
			const TArray<FVector2D>& P = Ulice[k].P;
			for (int32 i = 0; i + 1 < P.Num(); ++i)
			{
				if (WOtoczce(Obrys, P[i], 0.f) && !WOtoczce(Obrys, P[i + 1], 0.f))
				{
					for (int32 e = 0; e < N; ++e)
					{
						FVector2D X;
						if (Przeciecie(P[i], P[i + 1], Cw[e], Cw[(e + 1) % N], X)) { Bramy.Add(Luk[e] + float(FVector2D::Distance(Cw[e], X))); break; }
					}
					break;
				}
			}
		}
		auto OdBramy = [&](float A)
		{
			float M = TNumericLimits<float>::Max();
			for (const float G : Bramy) { const float D = FMath::Abs(A - G); M = FMath::Min(M, FMath::Min(D, Obwod - D)); }
			return M;
		};
		for (const float G : Bramy)
		{
			FVector2D P, T;
			NaObwodzie(G, P, T);
			// Wytrzymalosc bramy zostaje przy przebudowie (ta sama brama = blisko starej).
			const float Max = bKamien ? 1400.f : 600.f;
			FBrama Br{ P, Lewa(T), Max, Max };
			for (const FBrama& S : StareBramy)
			{
				if (FVector2D::Distance(S.P, P) < 500.f) { Br.Punkty = FMath::Min(S.Punkty, Max); break; }
			}
			BramyO.Add(Br);
			const float U = Br.Punkty / Max;
			Dodaj(U <= 0.f ? Zniszczony(BramaM, 2) : (U < 0.6f ? Zniszczony(BramaM, 1) : BramaM), FVector(P, ZNajnizej(P, 400.f)), YawOsi(T));
			Karczuj(Teren, P, 1000.f);
			++Bram;
		}
		// Odcinki: kazdy bok obrysu bez przerw na bramy, dzielony na rowne kawalki ~4 m; w dluzszym ciagu jedne schody.
		for (int32 e = 0; e < N; ++e)
		{
			TArray<TPair<float, float>> Kawalki;
			Kawalki.Add({ Luk[e], Luk[e + 1] });
			for (const float G : Bramy)
			{
				for (const float Gg : { G - Obwod, G, G + Obwod })
				{
					TArray<TPair<float, float>> Dalej;
					for (const TPair<float, float>& K : Kawalki)
					{
						const float A = Gg - PolBramy, B = Gg + PolBramy;
						if (B <= K.Key || A >= K.Value) { Dalej.Add(K); continue; }
						if (A > K.Key) { Dalej.Add({ K.Key, A }); }
						if (B < K.Value) { Dalej.Add({ B, K.Value }); }
					}
					Kawalki = MoveTemp(Dalej);
				}
			}
			const FVector2D T = (Cw[(e + 1) % N] - Cw[e]).GetSafeNormal();
			for (const TPair<float, float>& K : Kawalki)
			{
				const float Dl = K.Value - K.Key;
				if (Dl < 80.f) { continue; }
				const int32 Ile = FMath::Max(1, FMath::RoundToInt32(Dl / Segment));
				const float Sl = Dl / Ile;
				for (int32 j = 0; j < Ile; ++j)
				{
					const FVector2D P0 = Cw[e] + T * (K.Key - Luk[e] + j * Sl), P1 = P0 + T * Sl;
					const float Z0 = Z(P0), Z1 = Z(P1);
					const bool bSchody = Ile >= 4 && j == Ile / 2 - 1;
					Dodaj(bSchody ? Schody : Odc, FVector(P0, Z0), YawOsi(T), FVector(Sl / Segment * 1.02f, 1.f, 1.f),
						FMath::RadiansToDegrees(FMath::Atan2(Z1 - Z0, Sl)));
					if (j % 2 == 0) { Karczuj(Teren, (P0 + P1) * 0.5, 700.f); }
					++Odcinkow;
				}
			}
			// Baszta w rogu (wyrazne zalamanie) i co ~34 m na dlugim boku, nie przy bramie.
			const FVector2D Tp = (Cw[e] - Cw[(e + N - 1) % N]).GetSafeNormal();
			const float Zalamanie = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(float(Tp | T), -1.f, 1.f)));
			TArray<float> Gdzie;
			if (Zalamanie > 12.f) { Gdzie.Add(Luk[e]); }
			const int32 Dodatkowe = FMath::FloorToInt32((Luk[e + 1] - Luk[e]) / 3400.f);
			for (int32 j = 1; j <= Dodatkowe; ++j) { Gdzie.Add(Luk[e] + (Luk[e + 1] - Luk[e]) * j / (Dodatkowe + 1)); }
			for (const float A : Gdzie)
			{
				if (OdBramy(A) < PolBramy + 500.f) { continue; }
				FVector2D P, Tt;
				NaObwodzie(A, P, Tt);
				const FVector2D Kier2 = A == Luk[e] ? (Tp + T).GetSafeNormal() : T;
				Dodaj(Wieza, FVector(P, ZNajnizej(P, 300.f)), YawOsi(Kier2));
				Karczuj(Teren, P, 900.f);
				++Baszt;
			}
		}
	}

	// ---------------- instancje do komponentow ----------------
	for (TPair<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Para : Instancje)
	{
		if (Para.Value) { Para.Value->ClearInstances(); }
	}
	for (TPair<UStaticMesh*, TArray<FTransform>>& Para : Nowe)
	{
		TObjectPtr<UInstancedStaticMeshComponent>& C = Instancje.FindOrAdd(Para.Key);
		if (!C)
		{
			C = NewObject<UInstancedStaticMeshComponent>(this);
			C->SetStaticMesh(Para.Key);
			C->SetMobility(EComponentMobility::Movable);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetupAttachment(RootComponent);
			C->RegisterComponent();
		}
		C->AddInstances(Para.Value, false, true, false);
	}
	bZiemiaDoZbudowania = true;
	UE_LOG(LogTemp, Log, TEXT("OsadaRodu: %s - %d budynków w środku, %d za murami, mury %d: %d odcinków, %d baszt, %d bram, %d ulic"),
		*R.Nazwa, WSrodku, Dzialki.Num() - WSrodku, R.Mury, Odcinkow, Baszt, Bram, UliceZiemi.Num());
	for (int32 k = 0; k < Stan.Num(); ++k)
	{
		NowyCel(k); // budynki przestawione: nowe cele
	}
}

// ---------------- ubita ziemia: plac, ulice, podworza ----------------
float AOsadaRodu::WysokoscZiemi(int32 I, int32 J)
{
	if (const float* H = Wysokosci.Find(FIntPoint(I, J))) { return *H; }
	const float H = FSwiat::Wysokosc(I * double(KrokZiemi), J * double(KrokZiemi));
	Wysokosci.Add(FIntPoint(I, J), H);
	return H;
}

void AOsadaRodu::ZbudujZiemie()
{
	bZiemiaDoZbudowania = false;
	if (!Ziemia)
	{
		Ziemia = NewObject<UProceduralMeshComponent>(this);
		Ziemia->SetupAttachment(RootComponent);
		Ziemia->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ziemia->SetCastShadow(false);
		Ziemia->RegisterComponent();
		Ziemia->SetWorldLocationAndRotation(GetActorLocation(), FRotator::ZeroRotator);
	}
	// Obszar: plac, ulice i podworza z zapasem na rozmyty brzeg.
	FBox2D Box(ForceInit);
	const float RPlacu = FMath::Max(PlacU, PlacV) + 500.f;
	Box += Srodek - FVector2D(RPlacu);
	Box += Srodek + FVector2D(RPlacu);
	for (int32 u = 0; u < UliceZiemi.Num(); ++u)
	{
		for (const FVector2D& P : UliceZiemi[u]) { Box += P - FVector2D(UliceSzer[u] * 1.4f + 200.f); Box += P + FVector2D(UliceSzer[u] * 1.4f + 200.f); }
	}
	for (const FPodworze& P : Podworza)
	{
		const float Rr = float(P.Pol.Size()) + 600.f;
		Box += P.P - FVector2D(Rr);
		Box += P.P + FVector2D(Rr);
	}
	const int32 I0 = FMath::FloorToInt32(Box.Min.X / KrokZiemi), J0 = FMath::FloorToInt32(Box.Min.Y / KrokZiemi);
	const int32 Nx = FMath::Min(FMath::CeilToInt32(Box.Max.X / KrokZiemi) - I0 + 1, 800);
	const int32 Ny = FMath::Min(FMath::CeilToInt32(Box.Max.Y / KrokZiemi) - J0 + 1, 800);
	TArray<float> A;
	A.Init(0.f, Nx * Ny);
	TArray<uint8> Rodzaj;
	Rodzaj.Init(0, Nx * Ny);
	auto Pkt = [&](int32 i, int32 j) { return FVector2D((I0 + i) * double(KrokZiemi), (J0 + j) * double(KrokZiemi)); };
	auto Obszar = [&](FVector2D Mn, FVector2D Mx, TFunctionRef<void(int32, int32, FVector2D)> F)
	{
		const int32 a0 = FMath::Max(0, FMath::FloorToInt32(Mn.X / KrokZiemi) - I0), a1 = FMath::Min(Nx - 1, FMath::CeilToInt32(Mx.X / KrokZiemi) - I0);
		const int32 b0 = FMath::Max(0, FMath::FloorToInt32(Mn.Y / KrokZiemi) - J0), b1 = FMath::Min(Ny - 1, FMath::CeilToInt32(Mx.Y / KrokZiemi) - J0);
		for (int32 i = a0; i <= a1; ++i) { for (int32 j = b0; j <= b1; ++j) { F(i, j, Pkt(i, j)); } }
	};
	auto Ustaw = [&](int32 i, int32 j, float Alfa, uint8 R)
	{
		const int32 k = i * Ny + j;
		if (Alfa > A[k]) { A[k] = Alfa; Rodzaj[k] = R; }
	};
	// plac (elipsa)
	Obszar(Srodek - FVector2D(RPlacu), Srodek + FVector2D(RPlacu), [&](int32 i, int32 j, FVector2D P)
	{
		const FVector2D D = P - Srodek;
		const float U = float(D | OsPlacu) / PlacU, V = float(D | Lewa(OsPlacu)) / PlacV;
		const float Odl = (FMath::Sqrt(U * U + V * V) - 1.f) * FMath::Min(PlacU, PlacV);
		Ustaw(i, j, FMath::Clamp((50.f - Odl) / 250.f, 0.f, 1.f), 1);
	});
	// ulice: jak ADrogi::Wstega (pelna do ~0,65 polowy szerokosci, zanika do 1,3)
	for (int32 u = 0; u < UliceZiemi.Num(); ++u)
	{
		const TArray<FVector2D>& P = UliceZiemi[u];
		const float Pw = UliceSzer[u], Wz = Pw * 1.3f;
		for (int32 s = 0; s + 1 < P.Num(); ++s)
		{
			FBox2D B(ForceInit);
			B += P[s];
			B += P[s + 1];
			Obszar(B.Min - FVector2D(Wz), B.Max + FVector2D(Wz), [&](int32 i, int32 j, FVector2D X)
			{
				Ustaw(i, j, FMath::Clamp((Wz - OdOdcinka(X, P[s], P[s + 1])) / (0.65f * Pw), 0.f, 1.f), 0);
			});
		}
	}
	// podworza: klepisko wokol budynku (jak ABudowa::Podloze) i sciezka od frontu do ulicy
	for (const FPodworze& Pd : Podworza)
	{
		const FVector2D Os = Lewa(Pd.Przod);
		const float Rog = 200.f;
		const FVector2D Pol(FMath::Max(Pd.Pol.X - 50.0, double(Rog)), FMath::Max(Pd.Pol.Y - 50.0, double(Rog)));
		const FVector2D Rdzen = Pol - FVector2D(Rog);
		const float Rr = float(Pd.Pol.Size()) + 450.f;
		Obszar(Pd.P - FVector2D(Rr), Pd.P + FVector2D(Rr), [&](int32 i, int32 j, FVector2D X)
		{
			const FVector2D D = X - Pd.P;
			const FVector2D Q(FMath::Abs(D | Os), FMath::Abs(D | Pd.Przod));
			const FVector2D Poza(FMath::Max(Q.X - Rdzen.X, 0.0), FMath::Max(Q.Y - Rdzen.Y, 0.0));
			const float Odl = float(Poza.Size()) - Rog;
			Ustaw(i, j, FMath::Clamp((150.f - Odl) / 250.f, 0.f, 1.f) * 0.9f, 2);
		});
		const FVector2D F0 = Pd.P + Pd.Przod * Pd.Pol.Y, F1 = Pd.P + Pd.Przod * (Pd.Pol.Y + Podworze + UlicaPol);
		FBox2D B(ForceInit);
		B += F0;
		B += F1;
		Obszar(B.Min - FVector2D(250.f), B.Max + FVector2D(250.f), [&](int32 i, int32 j, FVector2D X)
		{
			Ustaw(i, j, FMath::Clamp((190.f - OdOdcinka(X, F0, F1)) / 120.f, 0.f, 1.f), 0);
		});
	}
	// Siatka: tylko czworokaty z ziemia, wysokosc terenu + 15 cm (jak drogi gracza).
	const FVector Baza = GetActorLocation();
	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> Nrm;
	TArray<FVector2D> UV;
	TArray<FLinearColor> Kol;
	TArray<FProcMeshTangent> Tg;
	TArray<int32> Indeks;
	Indeks.Init(-1, Nx * Ny);
	static const FLinearColor Barwy[3] = { FLinearColor(0.35f, 0.27f, 0.17f), FLinearColor(0.39f, 0.32f, 0.22f), FLinearColor(0.36f, 0.28f, 0.18f) };
	auto Wierzcholek = [&](int32 i, int32 j) -> int32
	{
		int32& k = Indeks[i * Ny + j];
		if (k >= 0) { return k; }
		const int32 I = I0 + i, J = J0 + j;
		const float H = WysokoscZiemi(I, J);
		const float Hx = WysokoscZiemi(I + 1, J) - WysokoscZiemi(I - 1, J), Hy = WysokoscZiemi(I, J + 1) - WysokoscZiemi(I, J - 1);
		const FVector2D P = Pkt(i, j);
		k = V.Add(FVector(P.X - Baza.X, P.Y - Baza.Y, H + 15.f - Baza.Z));
		Nrm.Add(FVector(-Hx / (2.f * KrokZiemi), -Hy / (2.f * KrokZiemi), 1.f).GetSafeNormal());
		UV.Add(FVector2D((P.X - Baza.X) / 400.0, (P.Y - Baza.Y) / 400.0));
		const float Szum = 0.92f + 0.14f * Los01((uint32(I) * 73856093u) ^ (uint32(J) * 19349663u));
		const FLinearColor C = Barwy[Rodzaj[i * Ny + j]] * Szum;
		Kol.Add(FLinearColor(C.R, C.G, C.B, A[i * Ny + j]));
		Tg.Add(FProcMeshTangent(1.f, 0.f, 0.f));
		return k;
	};
	for (int32 i = 0; i + 1 < Nx; ++i)
	{
		for (int32 j = 0; j + 1 < Ny; ++j)
		{
			if (A[i * Ny + j] + A[i * Ny + j + 1] + A[(i + 1) * Ny + j] + A[(i + 1) * Ny + j + 1] < 0.01f) { continue; }
			const int32 Aa = Wierzcholek(i, j), B = Wierzcholek(i, j + 1), C = Wierzcholek(i + 1, j), D = Wierzcholek(i + 1, j + 1);
			T.Append({ Aa, B, C, B, D, C }); // jak ABudowa::Podloze
		}
	}
	Ziemia->ClearAllMeshSections();
	if (T.Num())
	{
		Ziemia->CreateMeshSection_LinearColor(0, V, T, Nrm, UV, Kol, Tg, false);
		if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Swiat/Drogi/M_Droga.M_Droga")))
		{
			Ziemia->SetMaterial(0, M);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("OsadaRodu: ubita ziemia %d x %d, %d wierzcholkow"), Nx, Ny, V.Num());
}

// ---------------- ludziki przy kamerze ----------------
namespace
{
	constexpr float Zasieg = 45000.f;   // cm: kamera blizej srodka osady - ludziki sa
	constexpr int32 MaxLudzi = 26;      // na osade (wydajnosc: zwykle widac jedna osade naraz)
	constexpr float Chod = 150.f;       // cm/s

	/** Narzedzie w reku przy danej pracy (modele SM_Ludz_*). */
	const TCHAR* Narzedzie(const FRodzajBudynku* B, bool bBudowa, EPora P)
	{
		if (!B) { return TEXT("SM_Ludz_Kosz"); }
		if (bBudowa) { return TEXT("SM_Ludz_Mlotek"); }
		switch (B->Funkcja)
		{
		case EFunkcja::Drwal: return TEXT("SM_Ludz_Siekiera");
		case EFunkcja::Borowy: return TEXT("SM_Ludz_Motyka");
		case EFunkcja::Kamieniolom: return TEXT("SM_Ludz_Kilof");
		case EFunkcja::Pole: return P == EPora::Jesien ? TEXT("SM_Ludz_Sierp") : TEXT("SM_Ludz_Motyka");
		case EFunkcja::Mysliwy: return TEXT("SM_Ludz_Wlocznia");
		case EFunkcja::Rybak: return TEXT("SM_Ludz_Wedka");
		case EFunkcja::Owczarnia: return TEXT("SM_Ludz_Nozyce");
		case EFunkcja::Pilarnia: return TEXT("SM_Ludz_Pila");
		case EFunkcja::Przetworca: return B->Model.Contains(TEXT("Kopalnia")) ? TEXT("SM_Ludz_Kilof") : TEXT("SM_Ludz_Mlotek");
		default: return TEXT("SM_Ludz_Kosz");
		}
	}
	bool Machanie(const TCHAR* N) { return FCString::Strstr(N, TEXT("Kosz")) == nullptr && FCString::Strstr(N, TEXT("Wedka")) == nullptr; }
}

void AOsadaRodu::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Pozary.Num() > 0)
	{
		PozaryTick(DeltaSeconds * ABudowa::Tempo);
	}
	DoSprawdzenia -= DeltaSeconds;
	if (DoSprawdzenia <= 0.f)
	{
		DoSprawdzenia = 1.f;
		const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
		const APawn* Kam = PC ? PC->GetPawn() : nullptr;
		const bool bBlisko = Kam && FVector2D::Distance(FVector2D(Kam->GetActorLocation()), Srodek) < Zasieg && !bOblezona;
		if (bBlisko && Ludzie.Num() == 0 && Miejsca.Num() > 0) { Zaludnij(); }
		if (!bBlisko && Ludzie.Num() > 0) { Wyludnij(); } // przy oblezeniu mieszkancy sie chowaja
		if (bZiemiaDoZbudowania && Kam && FVector2D::Distance(FVector2D(Kam->GetActorLocation()), Srodek) < BliskoZiemi) { ZbudujZiemie(); }
	}
	const float Dt = DeltaSeconds * FMath::Min(ABudowa::Tempo, 3.f);
	for (int32 k = 0; k < Ludzie.Num(); ++k)
	{
		ALudzik* L = Ludzie[k];
		FMieszkaniec& S = Stan[k];
		if (!L) { continue; }
		const bool bNaMiejscu = FVector2D::Distance(FVector2D(L->GetActorLocation()), S.Cel) < 120.f;
		switch (S.Faza)
		{
		case EFaza::DoPracy:
		case EFaza::DoDomu:
			if (bNaMiejscu || !L->WRuchu())
			{
				if (!bNaMiejscu) { L->WalkaIdz(S.Cel, Chod); break; } // nie doszedl (np. zatrzymal sie): idzie dalej
				const bool bPraca = S.Faza == EFaza::DoPracy;
				S.Faza = bPraca ? EFaza::Pracuje : EFaza::Odpoczywa;
				S.Czas = bPraca ? FMath::FRandRange(14.f, 28.f) : FMath::FRandRange(3.f, 9.f);
				S.DoCiosu = FMath::FRandRange(0.2f, 1.f);
				const FVector2D Na = Miejsca.IsValidIndex(S.Praca) ? Miejsca[S.Praca].P : Srodek;
				L->WalkaStoj(S.bWoj || !bPraca ? Srodek : Na);
			}
			break;
		case EFaza::Pracuje:
			S.Czas -= Dt;
			S.DoCiosu -= Dt;
			if (S.DoCiosu <= 0.f)
			{
				S.DoCiosu = S.bWoj ? FMath::FRandRange(1.0f, 1.8f) : FMath::FRandRange(1.1f, 1.6f);
				const FMiejsce* M = Miejsca.IsValidIndex(S.Praca) ? &Miejsca[S.Praca] : nullptr;
				if (S.bWoj || Machanie(Narzedzie(M ? M->B : nullptr, M && M->bBudowa, PoraTeraz)))
				{
					L->WalkaCios(S.bWoj ? Srodek : (M ? M->P : Srodek));
				}
			}
			if (S.Czas <= 0.f)
			{
				S.Faza = EFaza::DoDomu;
				S.Cel = S.bWoj ? Srodek + FVector2D(FMath::FRandRange(-900.f, 900.f), FMath::FRandRange(-900.f, 900.f)) : S.Dom;
				L->WalkaIdz(S.Cel, Chod);
			}
			break;
		case EFaza::Odpoczywa:
			S.Czas -= Dt;
			if (S.Czas <= 0.f)
			{
				NowyCel(k);
			}
			break;
		}
	}
}

void AOsadaRodu::NowyCel(int32 k)
{
	FMieszkaniec& S = Stan[k];
	ALudzik* L = Ludzie[k];
	if (!L) { return; }
	if (S.bWoj)
	{
		// Cwiczenia na placu: szereg wokol choragwi, kazdy woj na swoim miejscu.
		const float A = 2.f * PI * k / FMath::Max(1, Ludzie.Num()) + 0.3f;
		S.Cel = Srodek + FVector2D(FMath::Cos(A), FMath::Sin(A)) * FMath::FRandRange(700.f, 1100.f);
	}
	else if (Miejsca.IsValidIndex(S.Praca))
	{
		const FMiejsce& M = Miejsca[S.Praca];
		// Przed budynkiem, od strony ulicy (front), w roznych miejscach wzdluz sciany.
		S.Cel = M.P + M.Przod * (M.B->Polowa.Y + 250.f) + Lewa(M.Przod) * (FMath::FRandRange(-1.f, 1.f) * M.B->Polowa.X * 0.6f);
	}
	else
	{
		S.Cel = S.Dom;
	}
	S.Faza = EFaza::DoPracy;
	L->WalkaIdz(S.Cel, Chod);
}

void AOsadaRodu::Zaludnij()
{
	// Prace: budowa (3 osoby), potem kazdy budynek z miejscami pracy po jednej osobie, woje na placu.
	TArray<int32> Prace, Domy;
	for (int32 i = 0; i < Miejsca.Num(); ++i)
	{
		if (Miejsca[i].bBudowa) { Prace.Append({ i, i, i }); }
		else if (Miejsca[i].B->Funkcja == EFunkcja::Mieszkalny) { Domy.Add(i); }
	}
	for (int32 i = 0; i < Miejsca.Num(); ++i)
	{
		if (!Miejsca[i].bBudowa && Miejsca[i].B->Miejsc > 0 && Miejsca[i].B->Funkcja != EFunkcja::Zbrojownia) { Prace.Add(i); }
	}
	const int32 IleWojow = FMath::Min(Woje, 8);
	const int32 Ile = FMath::Min(MaxLudzi, FMath::Max(Rodzin, IleWojow + 2));
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 k = 0; k < Ile; ++k)
	{
		FMieszkaniec S;
		S.bWoj = k < IleWojow;
		S.Praca = !S.bWoj && Prace.Num() > 0 ? Prace[(k - IleWojow) % Prace.Num()] : -1;
		const FMiejsce* MD = Domy.Num() > 0 ? &Miejsca[Domy[k % Domy.Num()]] : nullptr; // przed drzwiami domu
		S.Dom = MD ? MD->P + MD->Przod * (MD->B->Polowa.Y + 160.f) + Lewa(MD->Przod) * FMath::FRandRange(-200.f, 200.f) : Srodek;
		const FVector2D Start = S.Dom + FVector2D(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-200.f, 200.f));
		ALudzik* L = GetWorld()->SpawnActor<ALudzik>(FVector(Start, FSwiat::Wysokosc(Start.X, Start.Y)), FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), P);
		if (!L) { continue; }
		const FMiejsce* M = Miejsca.IsValidIndex(S.Praca) ? &Miejsca[S.Praca] : nullptr;
		const TCHAR* Bron = S.bWoj ? TEXT("SM_Ludz_Wlocznia") : Narzedzie(M ? M->B : nullptr, M && M->bBudowa, PoraTeraz);
		L->UstawPlec(!S.bWoj && FCString::Strstr(Bron, TEXT("Kosz")) != nullptr && k % 2 == 0);
		const FLinearColor Wlosy = FMath::Lerp(FLinearColor(0.05f, 0.03f, 0.02f), FLinearColor(0.45f, 0.32f, 0.15f), FMath::FRand());
		// Pracownicy w szarych i brazowych lnach, woje w barwie rodu.
		const FLinearColor Len = FMath::Lerp(FLinearColor(0.18f, 0.14f, 0.09f), FLinearColor(0.45f, 0.40f, 0.30f), FMath::FRand());
		L->WejdzDoBitwy(S.bWoj ? Barwa * FMath::FRandRange(0.85f, 1.1f) : Len, Wlosy, Bron);
		if (S.bWoj) { L->UbierzDoBitwy(1, FMath::RandBool(), true, false, Barwa); }
		Ludzie.Add(L);
		Stan.Add(S);
		NowyCel(Ludzie.Num() - 1);
	}
	UE_LOG(LogTemp, Log, TEXT("OsadaRodu: kamera blisko - %d ludzi (%d wojów, %d miejsc pracy)"), Ludzie.Num(), IleWojow, Prace.Num());
}

void AOsadaRodu::Wyludnij()
{
	for (ALudzik* L : Ludzie)
	{
		if (L) { L->Destroy(); }
	}
	Ludzie.Reset();
	Stan.Reset();
}

void AOsadaRodu::EndPlay(const EEndPlayReason::Type Powod)
{
	Wyludnij();
	if (AEfekty* E = Pozary.Num() > 0 ? AEfekty::W(GetWorld()) : nullptr)
	{
		for (const FPozar& P : Pozary) { E->ZakonczPozar(P.Id); }
	}
	Pozary.Reset();
	Super::EndPlay(Powod);
}

// ---------------- podboj: brama, mur, pozary ----------------
void AOsadaRodu::Przebuduj()
{
	if (!OstatniRod.IsValid())
	{
		return;
	}
	const bool bZiemia = bZiemiaDoZbudowania; // dzialki zostaja na miejscach: ubitej ziemi nie trzeba budowac od nowa
	Ostatnio.Reset();
	Odswiez(*OstatniRod, PoraTeraz);
	bZiemiaDoZbudowania = bZiemia;
}

bool AOsadaRodu::WMurach(FVector2D P) const
{
	return BramyO.Num() > 0 && WOtoczce(ObrysMuru, P, 0.f);
}

bool AOsadaRodu::PrzyRozbitejBramie(FVector2D P, float Promien) const
{
	for (const FBrama& B : BramyO)
	{
		if (B.Punkty <= 0.f && FVector2D::Distance(B.P, P) < Promien) { return true; }
	}
	return false;
}

bool AOsadaRodu::UderzBrame(int32 k, float Ile)
{
	if (!BramyO.IsValidIndex(k) || BramyO[k].Punkty <= 0.f)
	{
		return false;
	}
	FBrama& B = BramyO[k];
	const float Bylo = B.Punkty / B.Max;
	B.Punkty = FMath::Max(0.f, B.Punkty - Ile);
	const float Teraz = B.Punkty / B.Max;
	const FVector Gdzie(B.P, FSwiat::Wysokosc(B.P.X, B.P.Y));
	const FVector2D Wzdluz(B.Na.Y, -B.Na.X);
	const float Obrot = FMath::RadiansToDegrees(FMath::Atan2(Wzdluz.Y, Wzdluz.X));
	if (Teraz <= 0.f)
	{
		if (AEfekty* E = AEfekty::W(GetWorld()))
		{
			E->Zawal(Gdzie, FVector2D(PolBramy, 200.f), Obrot, bMuryKamienne ? 900.f : 650.f, bMuryKamienne);
		}
		UDzwieki::Graj(this, "Zawalenie", Gdzie, EDzwiek::Walka, 1.f);
		Przebuduj();
		return true;
	}
	if (Bylo >= 0.6f && Teraz < 0.6f)
	{
		if (AEfekty* E = AEfekty::W(GetWorld())) { E->Kurz(Gdzie + FVector(0.f, 0.f, 250.f), 300.f, 10); }
		Przebuduj(); // pekniete wrota: model _Z1
	}
	return false;
}

int32 AOsadaRodu::DoSpalenia(FVector2D P, float Promien) const
{
	const TArray<const FRodzajBudynku*>& L = FRodzajBudynku::DoBudowania();
	int32 Naj = -1;
	float NajD = Promien;
	for (int32 i = 0; i < Dzialki.Num(); ++i)
	{
		const FDzialka& D = Dzialki[i];
		const FRodzajBudynku& B = *L[D.Typ];
		if (D.bZewn || B.Funkcja == EFunkcja::Pole || Zniszczone.Contains(FIntPoint(D.Typ, D.Nr))
			|| Pozary.ContainsByPredicate([&D](const FPozar& F) { return F.K == FIntPoint(D.Typ, D.Nr); }))
		{
			continue;
		}
		// Odleglosc do obrysu (klik w dach duzego budynku tez trafia).
		const float Dd = FMath::Max(0.f, float(FVector2D::Distance(P, D.P)) - float(B.Polowa.GetMin()));
		if (Dd < NajD)
		{
			UStaticMesh* M = GotowyModel(B);
			if (M && Zniszczony(M, 2) != M) { NajD = Dd; Naj = i; }
		}
	}
	return Naj;
}

FVector2D AOsadaRodu::MiejsceDzialki(int32 i, float& Promien) const
{
	if (!Dzialki.IsValidIndex(i))
	{
		Promien = 0.f;
		return FVector2D::ZeroVector;
	}
	Promien = float(FRodzajBudynku::DoBudowania()[Dzialki[i].Typ]->Polowa.GetMax());
	return Dzialki[i].P;
}

bool AOsadaRodu::Podpal(int32 i, float Opoznienie)
{
	AEfekty* E = AEfekty::W(GetWorld());
	if (!E || !Dzialki.IsValidIndex(i))
	{
		return false;
	}
	const FDzialka& D = Dzialki[i];
	const FIntPoint K(D.Typ, D.Nr);
	if (Zniszczone.Contains(K) || Pozary.ContainsByPredicate([&K](const FPozar& F) { return F.K == K; }))
	{
		return false;
	}
	const FRodzajBudynku& B = *FRodzajBudynku::DoBudowania()[D.Typ];
	const UStaticMesh* M = GotowyModel(B);
	FPozar P;
	P.K = K;
	P.S = FVector(D.P, FSwiat::Wysokosc(D.P.X, D.P.Y));
	P.Pol = B.Polowa;
	P.Obrot = FMath::RadiansToDegrees(FMath::Atan2(-D.Przod.X, D.Przod.Y)); // jak model (YawFrontu)
	P.Wys = M ? FMath::Max(250.f, float(M->GetBoundingBox().Max.Z)) : 400.f;
	P.Czas = -Opoznienie;
	P.Id = E->Pozar(P.S, P.Pol, P.Obrot, P.Wys);
	E->UstawPozar(P.Id, Opoznienie > 0.f ? 0.35f : 1.f);
	Pozary.Add(P);
	return true;
}

void AOsadaRodu::Spal(int32 Ile)
{
	TArray<int32> Kandydaci;
	for (int32 i = 0; i < Dzialki.Num(); ++i)
	{
		if (DoSpalenia(Dzialki[i].P, 1.f) == i) { Kandydaci.Add(i); }
	}
	for (int32 n = 0; n < Ile && Kandydaci.Num() > 0; ++n)
	{
		const int32 j = FMath::RandHelper(Kandydaci.Num());
		Podpal(Kandydaci[j], FMath::FRandRange(0.f, 10.f)); // nie wszystko zajmuje sie naraz
		Kandydaci.RemoveAtSwap(j);
	}
	UE_LOG(LogTemp, Log, TEXT("OsadaRodu: %s plonie - %d budynkow"), OstatniRod.IsValid() ? *OstatniRod->Nazwa : TEXT("?"), Pozary.Num());
}
void AOsadaRodu::PozaryTick(float DtG)
{
	AEfekty* E = AEfekty::W(GetWorld());
	bool bZmiana = false;
	float Najsilniej = 0.f;
	FVector Najblizej = FVector::ZeroVector;
	for (int32 k = Pozary.Num() - 1; k >= 0; --k)
	{
		FPozar& P = Pozary[k];
		const float Bylo = P.Czas;
		P.Czas += DtG;
		// Ogien rosnie, po ~12 s budynek uszkodzony, po ~35 s zawala sie i dogasa nad gruzem, po ~80 s gasnie.
		if (Bylo < 0.f && P.Czas >= 0.f && E) { E->UstawPozar(P.Id, 1.f); }
		if (Bylo < 12.f && P.Czas >= 12.f)
		{
			Zniszczone.Add(P.K, 1);
			bZmiana = true;
		}
		if (Bylo < 35.f && P.Czas >= 35.f)
		{
			Zniszczone.Add(P.K, 2);
			bZmiana = true;
			if (E)
			{
				E->Zawal(P.S, P.Pol, P.Obrot, P.Wys, false);
				E->UstawPozar(P.Id, 0.4f, P.Wys * 0.35f);
			}
			UDzwieki::Graj(this, "Zawalenie", P.S, EDzwiek::Walka, 1.f);
		}
		if (P.Czas >= 80.f)
		{
			if (E) { E->ZakonczPozar(P.Id); }
			Pozary.RemoveAtSwap(k);
			continue;
		}
		if (P.Czas > 0.f && P.Czas < 35.f)
		{
			Najsilniej = 1.f;
			Najblizej = P.S;
		}
	}
	if (Najsilniej > 0.f)
	{
		if (UDzwieki* D = UDzwieki::Z(this))
		{
			const float Odl = float(FVector2D::Distance(FVector2D(D->Ucho()), FVector2D(Najblizej)));
			UDzwieki::Petla(this, "Petla_Ogien", EDzwiek::Walka, FMath::Clamp(1.f - (Odl - 1500.f) / 12000.f, 0.f, 1.f));
		}
		if (FMath::FRand() < DtG * 0.05f)
		{
			UDzwieki::Graj(this, "Krzyk_Panika", Najblizej, EDzwiek::Glosy, 0.7f);
		}
	}
	if (bZmiana)
	{
		Przebuduj();
	}
}
