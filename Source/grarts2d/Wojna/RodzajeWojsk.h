// Plik: Source/grarts2d/Wojna/RodzajeWojsk.h
// Rodzaje wojsk (21.11.2026, etap W2 dokumentu "Wojna, bitwy i oblezenia: projekt"; piesi i strzelcy, konni pozniej).
// Kazdy rodzaj: bron, sila ciosu, pancerz, tarcza (ile ciosow z przodu przechodzi), tempo ciosow, przebicie pancerza,
// wyglad (helmy, tarcze, kolor tuniki / kolczugi), strzelcy: zasieg, przeladowanie, zapas, celnosc.
//   Wlocznicy - trzon, rowni we wszystkim.          Palkarze - tani, bija przez pancerz, slabo sie zaslaniaja.
//   Tarczownicy z toporem - mur tarcz, tna glowy.   Miecznicy z tarcza - kolczuga, szybkie ciosy, drozsi.
//   Lucznicy - szybki ostrzal lukiem, slabi wrecz. Kusznicy - wolno, ale belt przebija pancerz i tarcze.
//   Chlopi (pospolite ruszenie) i bandyci - jak dotad.
#pragma once

#include "CoreMinimal.h"

enum class ERodzajWojska : uint8 { Wlocznicy, Palkarze, Tarczownicy, Miecznicy, Lucznicy, Kusznicy, Chlopi, Bandyci, Ile };

/** Ikona broni na karcie oddzialu (Wojna/PanelBitwy). */
enum class EIkonaBroni : uint8 { Wlocznia, Topor, Miecz, Palka, Luk, Kusza };

struct FRodzajWojska
{
	const TCHAR* Nazwa;
	const TCHAR* Bron;          // model w dloni
	EIkonaBroni Ikona;
	float Trafienie;            // podstawowa szansa trafienia wrecz
	float Pancerz;              // odejmowane od szansy trafienia przeciwnika
	float Zdrowie;              // ciosy do smierci
	float Blok;                 // czesc ciosow z przodu, ktora przechodzi przez tarcze / zaslone (mniej = lepiej)
	float Tempo;                // mnoznik przerwy miedzy ciosami (miecz szybciej)
	float Przebicie;            // 0..1: ile pancerza przeciwnika nie liczy sie (palka, belt)
	float Ciecie;               // szansa odciecia glowy / reki przy zabiciu
	float Morale;               // duch na poczatku
	float Tarcza;               // szansa na tarcze u woja
	float Helm, Czapka, Kaptur; // szanse nakrycia glowy
	FLinearColor Tunika;        // kolczuga = szara
	// Strzelcy (Zasieg 0 = walka wrecz).
	float Zasieg = 0.f;         // cm
	float Przeladowanie = 0.f;  // s miedzy strzalami
	int32 Pociski = 0;          // zapas na woja
	float Rozrzut = 0.f;        // odchyl miejsca upadku na kazde 10 m odleglosci (cm)
	float Rana = 1.f;           // ile zdrowia zabiera trafienie
	bool Strzela() const { return Zasieg > 0.f; }
};

inline const FRodzajWojska& RodzajWojska(ERodzajWojska R)
{
	static const FRodzajWojska Tab[int32(ERodzajWojska::Ile)] = {
		//  nazwa                     bron                        ikona                 traf   panc  zdr   blok  tempo przeb ciecie duch  tarcza helm czapka kaptur tunika
		{ TEXT("Włócznicy"),          TEXT("SM_Ludz_Wlocznia"), EIkonaBroni::Wlocznia, 0.42f, 0.00f, 3.0f, 0.30f, 1.00f, 0.0f, 0.12f, 100.f, 1.0f, 0.55f, 0.30f, 0.0f, FLinearColor(0.42f, 0.36f, 0.25f) },
		{ TEXT("Pałkarze"),           TEXT("SM_Ludz_Maczuga"),  EIkonaBroni::Palka,    0.40f, 0.00f, 3.0f, 0.45f, 0.90f, 1.0f, 0.03f,  90.f, 0.0f, 0.10f, 0.60f, 0.2f, FLinearColor(0.46f, 0.33f, 0.22f) },
		{ TEXT("Tarczownicy"),        TEXT("SM_Ludz_Siekiera"), EIkonaBroni::Topor,    0.40f, 0.04f, 3.0f, 0.18f, 1.10f, 0.2f, 0.30f, 100.f, 1.0f, 0.70f, 0.20f, 0.0f, FLinearColor(0.36f, 0.32f, 0.24f) },
		{ TEXT("Miecznicy"),          TEXT("SM_Ludz_Miecz"),    EIkonaBroni::Miecz,    0.48f, 0.10f, 3.5f, 0.25f, 0.70f, 0.0f, 0.22f, 110.f, 1.0f, 1.00f, 0.00f, 0.0f, FLinearColor(0.47f, 0.48f, 0.50f) },
		{ TEXT("Łucznicy"),           TEXT("SM_Ludz_Luk"),      EIkonaBroni::Luk,      0.28f, 0.00f, 2.5f, 0.50f, 1.10f, 0.0f, 0.05f,  90.f, 0.0f, 0.00f, 0.40f, 0.5f, FLinearColor(0.33f, 0.38f, 0.22f),
			4600.f, 2.6f, 24, 95.f, 1.f },
		{ TEXT("Kusznicy"),           TEXT("SM_Ludz_Kusza"),    EIkonaBroni::Kusza,    0.30f, 0.03f, 3.0f, 0.45f, 1.10f, 1.0f, 0.05f,  95.f, 0.0f, 0.80f, 0.20f, 0.0f, FLinearColor(0.40f, 0.30f, 0.26f),
			5000.f, 5.5f, 16, 60.f, 1.5f },
		{ TEXT("Ruszenie"),           TEXT("SM_Ludz_Mlotek"),   EIkonaBroni::Topor,    0.32f, 0.00f, 2.0f, 0.30f, 1.00f, 0.0f, 0.20f,  75.f, 0.15f, 0.00f, 0.30f, 0.0f, FLinearColor(0.45f, 0.38f, 0.28f) },
		{ TEXT("Bandyci"),            TEXT("SM_Ludz_Siekiera"), EIkonaBroni::Topor,    0.36f, 0.00f, 2.0f, 0.30f, 1.00f, 0.0f, 0.30f, 100.f, 0.4f, 0.00f, 0.30f, 0.35f, FLinearColor(0.22f, 0.24f, 0.17f) },
	};
	return Tab[FMath::Clamp(int32(R), 0, int32(ERodzajWojska::Ile) - 1)];
}
