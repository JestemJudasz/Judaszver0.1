// Plik: Source/grarts2d/Osada/Surowce.h
// Surowce osady (Etap 1 ekonomii, dokument "Ekonomia gry: plan", 20.11.2026): klody, deski, kamien.
// Etap 2 (21.11.2026): jedzenie - jeden rodzaj (jagody i grzyby zbieracza, mieso mysliwego), rodziny jedza je z zapasow.
// Etap 3 (21.11.2026): zboze (snopy z pol) i maka (z mlyna); piekarz robi z maki chleb, ktory liczy sie jako jedzenie.
// Etap 4 (21.11.2026): welna (strzyzone owce), sukno (tkacz), ruda zelaza (kopalnia), bron (kuznia: wlocznie);
// woje w zbrojowni biora wlocznie i przeszywanice z sukna.
// Towar lezy fizycznie w budynkach (ABudowa::Zapas): stos klod przy chacie drwala, desek przy pilarni, wszystko na
// placu skladowym i w obozie, jedzenie w spichlerzu. Licznik u gory ekranu to suma zapasow osady.
#pragma once

#include "CoreMinimal.h"

// Paczka A (23.11.2026): Opal (drewno opalowe z chaty opalowej, zima do pieca), Ryby (rybak; drugi rodzaj jedzenia).
// Paczka A, krok 3 (23.11.2026): Skory (surowe, od mysliwego), Skora (garbarnia), Buty (szewc), Ubrania (krawiec), Slod
// (slodownia, ze zboza), Piwo (browar, pija je w karczmie).
enum class ESurowiec : uint8 { Kloda, Deska, Kamien, Jedzenie, Zboze, Maka, Welna, Sukno, Ruda, Bron, Opal, Ryby,
	Skory, Skora, Buty, Ubrania, Slod, Piwo, Ile };

/** Ilosci kazdego surowca (np. zapas, koszt budowy). */
struct FSurowce
{
	int32 Ile[int32(ESurowiec::Ile)] = {};

	FSurowce() = default;
	FSurowce(int32 Klody, int32 Deski, int32 Kamien, int32 Jedzenie = 0) { Ile[0] = Klody; Ile[1] = Deski; Ile[2] = Kamien; Ile[3] = Jedzenie; }

	int32& operator[](ESurowiec S) { return Ile[int32(S)]; }
	int32 operator[](ESurowiec S) const { return Ile[int32(S)]; }

	int32 Suma() const
	{
		int32 N = 0;
		for (int32 i = 0; i < int32(ESurowiec::Ile); ++i) { N += Ile[i]; }
		return N;
	}
	bool Puste() const { return Suma() == 0; }

	FSurowce& operator+=(const FSurowce& B)
	{
		for (int32 i = 0; i < int32(ESurowiec::Ile); ++i) { Ile[i] += B.Ile[i]; }
		return *this;
	}
};

/** Wszystkie surowce po kolei (do petli). */
inline const ESurowiec WszystkieSurowce[] = { ESurowiec::Kloda, ESurowiec::Deska, ESurowiec::Kamien, ESurowiec::Jedzenie, ESurowiec::Zboze, ESurowiec::Maka,
	ESurowiec::Welna, ESurowiec::Sukno, ESurowiec::Ruda, ESurowiec::Bron, ESurowiec::Opal, ESurowiec::Ryby,
	ESurowiec::Skory, ESurowiec::Skora, ESurowiec::Buty, ESurowiec::Ubrania, ESurowiec::Slod, ESurowiec::Piwo };

/** Nazwa w mianowniku liczby mnogiej (licznik, okno budynku). */
inline const TCHAR* NazwaSurowca(ESurowiec S)
{
	switch (S)
	{
	case ESurowiec::Kloda: return TEXT("Kłody");
	case ESurowiec::Deska: return TEXT("Deski");
	case ESurowiec::Jedzenie: return TEXT("Jedzenie");
	case ESurowiec::Zboze: return TEXT("Zboże");
	case ESurowiec::Maka: return TEXT("Mąka");
	case ESurowiec::Welna: return TEXT("Wełna");
	case ESurowiec::Sukno: return TEXT("Sukno");
	case ESurowiec::Ruda: return TEXT("Ruda żelaza");
	case ESurowiec::Bron: return TEXT("Broń");
	case ESurowiec::Opal: return TEXT("Drewno opałowe");
	case ESurowiec::Ryby: return TEXT("Ryby");
	case ESurowiec::Skory: return TEXT("Skóry surowe");
	case ESurowiec::Skora: return TEXT("Skóra");
	case ESurowiec::Buty: return TEXT("Buty");
	case ESurowiec::Ubrania: return TEXT("Ubrania");
	case ESurowiec::Slod: return TEXT("Słód");
	case ESurowiec::Piwo: return TEXT("Piwo");
	default: return TEXT("Kamień");
	}
}

/** "1 kłoda", "3 kłody", "5 kłód", "2 porcje jedzenia", "4 snopy zboża", "5 włóczni" (polska odmiana po liczebniku). */
inline FString IleSurowca(ESurowiec S, int32 N)
{
	static const TCHAR* Formy[int32(ESurowiec::Ile)][3] = {
		{ TEXT("kłoda"), TEXT("kłody"), TEXT("kłód") },
		{ TEXT("deska"), TEXT("deski"), TEXT("desek") },
		{ TEXT("kamień"), TEXT("kamienie"), TEXT("kamieni") },
		{ TEXT("porcja jedzenia"), TEXT("porcje jedzenia"), TEXT("porcji jedzenia") },
		{ TEXT("snop zboża"), TEXT("snopy zboża"), TEXT("snopów zboża") },
		{ TEXT("worek mąki"), TEXT("worki mąki"), TEXT("worków mąki") },
		{ TEXT("runo wełny"), TEXT("runa wełny"), TEXT("run wełny") },
		{ TEXT("bela sukna"), TEXT("bele sukna"), TEXT("bel sukna") },
		{ TEXT("kosz rudy"), TEXT("kosze rudy"), TEXT("koszy rudy") },
		{ TEXT("włócznia"), TEXT("włócznie"), TEXT("włóczni") },
		{ TEXT("naręcze polan"), TEXT("naręcza polan"), TEXT("naręczy polan") },
		{ TEXT("ryba"), TEXT("ryby"), TEXT("ryb") },
		{ TEXT("surowa skóra"), TEXT("surowe skóry"), TEXT("surowych skór") },
		{ TEXT("rulon skóry"), TEXT("rulony skóry"), TEXT("rulonów skóry") },
		{ TEXT("para butów"), TEXT("pary butów"), TEXT("par butów") },
		{ TEXT("ubranie"), TEXT("ubrania"), TEXT("ubrań") },
		{ TEXT("worek słodu"), TEXT("worki słodu"), TEXT("worków słodu") },
		{ TEXT("baryłka piwa"), TEXT("baryłki piwa"), TEXT("baryłek piwa") },
	};
	const int32 J = N % 10, D = N % 100;
	const int32 F = N == 1 ? 0 : ((J >= 2 && J <= 4 && (D < 12 || D > 14)) ? 1 : 2);
	return FString::Printf(TEXT("%d %s"), N, Formy[int32(S)][F]);
}

/** Model niesiony przez ludzika (Tools/ludzie.py). */
inline const TCHAR* NiesionySurowiec(ESurowiec S)
{
	switch (S)
	{
	case ESurowiec::Kloda: return TEXT("SM_Ludz_Bal");
	case ESurowiec::Deska: return TEXT("SM_Ludz_Deski");
	case ESurowiec::Jedzenie: return TEXT("SM_Ludz_Worek");
	case ESurowiec::Zboze: return TEXT("SM_Ludz_Snop");
	case ESurowiec::Maka: return TEXT("SM_Ludz_Maka");
	case ESurowiec::Welna: return TEXT("SM_Ludz_Welna");
	case ESurowiec::Sukno: return TEXT("SM_Ludz_Sukno");
	case ESurowiec::Ruda: return TEXT("SM_Ludz_Ruda");
	case ESurowiec::Bron: return TEXT("SM_Ludz_Bron");
	case ESurowiec::Opal: return TEXT("SM_Ludz_Opal");
	case ESurowiec::Ryby: return TEXT("SM_Ludz_Ryby");
	case ESurowiec::Skory: return TEXT("SM_Ludz_Skory");
	case ESurowiec::Skora: return TEXT("SM_Ludz_Skora");
	case ESurowiec::Buty: return TEXT("SM_Ludz_Buty");
	case ESurowiec::Ubrania: return TEXT("SM_Ludz_Ubrania");
	case ESurowiec::Slod: return TEXT("SM_Ludz_Slod");
	case ESurowiec::Piwo: return TEXT("SM_Ludz_Piwo");
	default: return TEXT("SM_Ludz_Kamien");
	}
}
