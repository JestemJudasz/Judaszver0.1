// Plik: Source/grarts2d/Budowy/Budowa.cpp
#include "Budowy/Budowa.h"
#include "Swiat/SwiatWysokosci.h"
#include "Efekty/Efekty.h"
#include "Dzwiek/Dzwieki.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

float ABudowa::Tempo = 1.f;
bool ABudowa::bNatychmiast = false;
int32 ABudowa::NastepnyWariant = -1;

const FRodzajBudynku& FRodzajBudynku::Kosciol()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Kościół kamienny");
		K.Opis = TEXT("Kościół, poziom 2: kamienny, z wieżą. Długa przebudowa: mury, rusztowania, więźba i dach z gontu. Można go jeszcze rozbudować.");
		K.Poziom = 2;
		K.Ulepszenie = &FRodzajBudynku::KosciolWielki;
		K.Kategoria = TEXT("Religia");
		K.Model = TEXT("SM_Kosciol");
		K.Ikona = TEXT("T_Ikona_Kosciol");
		K.Wejscie = FVector2D(-1250.f, 0.f);
		K.Fazy = 5;
		K.CzasBudowy = 240.f;
		// Obrys z Tools/budynki.py: x -11..11.5 m (wieza, nawa, prezbiterium), y -4..4 m, plac z materialem do -8 m.
		K.Polowa = FVector2D(1200.f, 850.f);
		K.Promien = 1600.f; // 14 m zostawialo drzewo przy rogu wiezy (zrzut 20.11.2026)
		K.Koszt = FSurowce(10, 20, 60);
		K.Wymaga = 1; // paczka A: poziom osady
		return K;
	}();
	return R;
}

// Kosciol, poziomy 1 i 3 (22.11.2026). Wymiary z Tools/budynki.py (kosciol_drewniany, kosciol_wielki).
const FRodzajBudynku& FRodzajBudynku::KosciolDrewniany()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Kościół drewniany");
		K.Opis = TEXT("Kościół, poziom 1: z bali, z wieżą-dzwonnicą i podcieniem dookoła. Później można go przebudować na kamienny.");
		K.Kategoria = TEXT("Religia");
		K.Model = TEXT("SM_KosciolDrewniany");
		K.Ikona = TEXT("T_Ikona_KosciolDrewniany");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Poziom = 1;
		K.Ulepszenie = &FRodzajBudynku::Kosciol;
		K.Fazy = 4;
		K.CzasBudowy = 180.f;
		// x -8,1..7,9 m (wieza na -X, prezbiterium na +X), y +-4,9 m z podcieniem, material do 7,6 m przed kosciolem.
		K.Polowa = FVector2D(850.f, 650.f);
		K.Promien = 1200.f;
		K.ObwodMin = FVector2D(-950.f, -560.f);
		K.ObwodMax = FVector2D(880.f, 560.f);
		K.Sterta = FVector2D(-200.f, 680.f);
		K.ZrzutX = FVector2D(-300.f, 300.f);
		K.ZrzutY = 580.f;
		K.Wejscie = FVector2D(-900.f, 0.f);
		K.Koszt = FSurowce(16, 12, 0);
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::KosciolWielki()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K = Kosciol();
		K.Nazwa = TEXT("Kościół z placem");
		K.Opis = TEXT("Kościół, poziom 3: kamienny z kaplicą, brukowanym placem, murem z bramą, studnią, lipami i figurą.");
		K.Model = TEXT("SM_KosciolWielki");
		K.Ikona = TEXT("T_Ikona_Kosciol");
		K.Poziom = 3;
		K.Ulepszenie = nullptr;
		K.Fazy = 5;
		K.CzasBudowy = 260.f;
		// Plac z murem: x -15,2..15,6 m, y -11..9,8 m (w Unrealu Y odwrocone: front z brama po +Y).
		K.Polowa = FVector2D(1560.f, 1120.f);
		K.Promien = 1800.f;
		K.ObwodMin = FVector2D(-1300.f, -600.f);
		K.ObwodMax = FVector2D(1300.f, 820.f);
		K.Sterta = FVector2D(-450.f, 880.f);
		K.ZrzutX = FVector2D(100.f, 600.f);
		K.ZrzutY = 790.f;
		K.Koszt = FSurowce(10, 25, 80);
		K.Wymaga = 2; // paczka A: poziom osady
		return K;
	}();
	return R;
}

// Domy: wymiary z Tools/budynki.py (Y odwrocone: drzwi i material budowy po stronie +Y w Unrealu).
const FRodzajBudynku& FRodzajBudynku::Chata()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Chata z bali");
		K.Opis = TEXT("Dom, poziom 1: mała chata z bali kryta strzechą. Szybka i tania budowa, mieszka w niej jedna rodzina. Można ją rozbudować w oknie budynku.");
		K.Kategoria = TEXT("Mieszkalne");
		K.Model = TEXT("SM_Chata");
		K.Poziom = 1;
		K.Wariantow = 4;
		K.Ulepszenie = &FRodzajBudynku::DomPruski;
		K.Ikona = TEXT("T_Ikona_Chata");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.Rodzin = 1;
		K.CzasBudowy = 120.f;
		K.Polowa = FVector2D(420.f, 450.f);   // sciany 7 x 5 m, okap, bale przed chata
		K.Promien = 750.f;
		K.ObwodMin = FVector2D(-480.f, -400.f);
		K.ObwodMax = FVector2D(480.f, 400.f);
		K.Sterta = FVector2D(-100.f, 590.f);  // stos bali
		K.ZrzutX = FVector2D(-250.f, 250.f);
		K.ZrzutY = 340.f;
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(-110.f, 430.f);
		K.Funkcja = EFunkcja::Mieszkalny;
		K.Koszt = FSurowce(4, 0, 0); // 22.11.2026: taniej (bylo 8), zeby na starcie starczylo na tartak i domy
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::DomPruski()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Dom z muru pruskiego");
		K.Opis = TEXT("Dom, poziom 2: biały dom z ciemnym szkieletem belek, kryty strzechą albo gontem. Mieszka w nim jedna rodzina.");
		K.Kategoria = TEXT("Mieszkalne");
		K.Model = TEXT("SM_DomPruski");
		K.Poziom = 2;
		K.Wariantow = 4;
		K.Ulepszenie = &FRodzajBudynku::DomDuzy;
		K.Ikona = TEXT("T_Ikona_DomPruski");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.Rodzin = 1;
		K.CzasBudowy = 200.f;
		K.Polowa = FVector2D(520.f, 500.f);   // 9 x 6 m
		K.Promien = 850.f;
		K.ObwodMin = FVector2D(-580.f, -480.f);
		K.ObwodMax = FVector2D(580.f, 480.f);
		K.Sterta = FVector2D(-150.f, 640.f);
		K.ZrzutX = FVector2D(-300.f, 300.f);
		K.ZrzutY = 400.f;
		K.Budowniczych = 2;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(0.f, 500.f);
		K.Funkcja = EFunkcja::Mieszkalny;
		K.Koszt = FSurowce(6, 6, 2);
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::DomDuzy()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Duży dom");
		K.Opis = TEXT("Dom, poziom 3: piętrowy, kamienny parter, piętro z muru pruskiego, dach z gontu i kominy. Mieszkają w nim dwie rodziny.");
		K.Kategoria = TEXT("Mieszkalne");
		K.Model = TEXT("SM_DomDuzy");
		K.Poziom = 3;
		K.Wariantow = 4;
		K.Ikona = TEXT("T_Ikona_DomDuzy");
		K.Fazy = 4;
		K.Rodzin = 2;
		K.CzasBudowy = 320.f;
		K.Polowa = FVector2D(650.f, 650.f);   // 11 x 7 m, rusztowanie i material dalej od muru
		K.Promien = 1000.f;
		K.ObwodMin = FVector2D(-700.f, -580.f);
		K.ObwodMax = FVector2D(700.f, 580.f);
		K.Sterta = FVector2D(-250.f, 810.f);
		K.ZrzutX = FVector2D(-400.f, 400.f);
		K.ZrzutY = 610.f;
		K.Budowniczych = 2;
		K.Tragarzy = 2;
		K.Wejscie = FVector2D(-210.f, 620.f);
		K.Funkcja = EFunkcja::Mieszkalny;
		K.Koszt = FSurowce(6, 10, 12);
		K.Wymaga = 2; // paczka A: poziom osady
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Namiot()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Namiot");
		K.Model = TEXT("SM_Namiot");
		K.Fazy = 1;
		K.Rodzin = 1;
		K.Polowa = FVector2D(240.f, 200.f);
		K.Promien = 350.f;
		K.Wejscie = FVector2D(-330.f, 0.f); // wejscie od -X
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Ognisko()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Ognisko");
		K.Model = TEXT("SM_Ognisko");
		K.Opis = TEXT("Ognisko obozu. Obok leży zapas przywieziony przez osadników: na start 20 kłód i 25 porcji jedzenia.");
		K.Fazy = 1;
		K.Polowa = FVector2D(100.f, 100.f);
		K.Promien = 250.f;
		// Oboz jest pierwszym magazynem: stosy w lukach miedzy namiotami (namioty co 72 st. od 90 st., R = 6,5 m).
		K.Funkcja = EFunkcja::Magazyn;
		K.Pojemnosc = 30;
		K.MaskaStosow = 0b1111;
		K.Stosy[int32(ESurowiec::Kloda)] = FVector2D(-410.f, -135.f); // 198 st.
		K.Stosy[int32(ESurowiec::Deska)] = FVector2D(0.f, -430.f);    // 270 st.
		K.Stosy[int32(ESurowiec::Kamien)] = FVector2D(410.f, -135.f); // 342 st.
		K.Stosy[int32(ESurowiec::Jedzenie)] = FVector2D(-255.f, 350.f); // 126 st.
		return K;
	}();
	return R;
}

// Budynki drewna (etap 1 ekonomii). Wymiary z Tools/budynki.py (Y odwrocone: front, drzwi i material po stronie +Y).
const FRodzajBudynku& FRodzajBudynku::PlacSkladowy()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Plac składowy");
		K.Opis = TEXT("Ogrodzony plac z wiatą. Tragarze znoszą tu kłody, deski i kamień, a budowy biorą stąd materiał. Jedzenie trzyma się w spichlerzu.");
		K.Kategoria = TEXT("Surowce");
		K.Model = TEXT("SM_PlacSkladowy");
		K.Klepisko = FLinearColor(0.36f, 0.28f, 0.18f);
		K.Ikona = TEXT("T_Ikona_PlacSkladowy");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 80.f;
		K.Polowa = FVector2D(530.f, 430.f);   // plac 10 x 8 m
		K.Promien = 760.f;
		K.ObwodMin = FVector2D(-570.f, -470.f);
		K.ObwodMax = FVector2D(570.f, 470.f);
		K.Sterta = FVector2D(-150.f, 250.f);  // bale na placu, przy bramie
		K.ZrzutX = FVector2D(-450.f, -200.f); // ploty przy bramie
		K.ZrzutY = 400.f;
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(0.f, 470.f);
		K.Funkcja = EFunkcja::Magazyn;
		K.Koszt = FSurowce(6, 0, 0);
		K.Pojemnosc = 30;
		K.MaskaStosow = 0b111 | (1 << int32(ESurowiec::Opal));
		K.Stosy[int32(ESurowiec::Kloda)] = FVector2D(-260.f, 80.f);
		K.Stosy[int32(ESurowiec::Deska)] = FVector2D(240.f, 80.f);
		K.Stosy[int32(ESurowiec::Kamien)] = FVector2D(260.f, -230.f);
		K.Stosy[int32(ESurowiec::Opal)] = FVector2D(-260.f, -230.f); // paczka A: drewno opalowe
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::ChataDrwala()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Chata drwala");
		K.Opis = TEXT("Drwal ścina drzewa w promieniu 40 m i znosi kłody pod chatę. Wycięty las znika z mapy, więc warto postawić obok chatę borowego. Pierwsza chata drwala w osadzie jest za darmo.");
		K.Kategoria = TEXT("Surowce");
		K.Model = TEXT("SM_ChataDrwala");
		K.Ikona = TEXT("T_Ikona_ChataDrwala");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 100.f;
		K.Polowa = FVector2D(450.f, 480.f);   // chata 5,6 x 4,4 m + drewutnia, kloc i klody przed chata
		K.Promien = 750.f;
		K.ObwodMin = FVector2D(-380.f, -300.f);
		K.ObwodMax = FVector2D(300.f, 300.f);
		K.Sterta = FVector2D(-80.f, 520.f);
		K.ZrzutX = FVector2D(-250.f, 150.f);
		K.ZrzutY = 270.f;
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(-100.f, 330.f);
		K.Funkcja = EFunkcja::Drwal;
		K.Koszt = FSurowce(4, 0, 0);
		K.Miejsc = 2;                         // 22.11.2026: od razu 2 rodziny (bylo 1)
		K.bPierwszyDarmowy = true;            // pierwsza chata drwala w osadzie bez materialu
		K.Poziom = 1;
		K.Ulepszenie = &ChataDrwala2;
		K.Pojemnosc = 10;
		K.Wyrob = ESurowiec::Kloda;
		K.Zasieg = 4000.f;
		K.MaskaStosow = 0b001;
		K.Stosy[int32(ESurowiec::Kloda)] = FVector2D(140.f, 400.f);
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::ChataDrwala2()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K = ChataDrwala();
		K.Nazwa = TEXT("Chata drwala z kozłami");
		K.Opis = TEXT("Drwale ścinają drzewa w promieniu 50 m. Przed chatą kozły z piłą, przy chacie mieści się więcej kłód.");
		K.Model = TEXT("SM_ChataDrwala2");
		K.Koszt = FSurowce(4, 4, 0);
		K.bPierwszyDarmowy = false;
		K.Zasieg = 5000.f;
		K.Pojemnosc = 14;
		K.Poziom = 2;
		K.Ulepszenie = &ChataDrwala3;
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::ChataDrwala3()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K = ChataDrwala2();
		K.Nazwa = TEXT("Zagroda drwali");
		K.Opis = TEXT("Długa chata z dwojgiem drzwi, sanie do zwózki kłód. Pracują tu trzy rodziny drwali, ścinają w promieniu 55 m.");
		K.Model = TEXT("SM_ChataDrwala3");
		K.Polowa = FVector2D(500.f, 520.f);
		K.Promien = 820.f;
		K.ObwodMin = FVector2D(-520.f, -320.f);
		K.Koszt = FSurowce(6, 6, 4);
		K.Miejsc = 3;
		K.Zasieg = 5500.f;
		K.Pojemnosc = 18;
		K.Poziom = 3;
		K.Ulepszenie = nullptr;
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::ChataBorowego()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Chata borowego");
		K.Opis = TEXT("Borowy pilnuje lasu: sadzi młode drzewa w promieniu 35 m. Po kilku minutach wyrastają z nich dorosłe drzewa dla drwala.");
		K.Kategoria = TEXT("Surowce");
		K.Model = TEXT("SM_ChataBorowego");
		K.Ikona = TEXT("T_Ikona_ChataBorowego");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 100.f;
		K.Polowa = FVector2D(520.f, 480.f);   // chata + szkolka mlodych drzew z boku
		K.Promien = 800.f;
		K.ObwodMin = FVector2D(-480.f, -300.f);
		K.ObwodMax = FVector2D(140.f, 300.f);
		K.Sterta = FVector2D(-200.f, 520.f);
		K.ZrzutX = FVector2D(-380.f, 0.f);
		K.ZrzutY = 270.f;
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(-200.f, 330.f);
		K.Funkcja = EFunkcja::Borowy;
		K.Koszt = FSurowce(4, 0, 0);
		K.Miejsc = 1;
		K.Zasieg = 3500.f;
		K.Poziom = 1;
		K.Ulepszenie = &ChataBorowego2;
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::ChataBorowego2()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K = ChataBorowego();
		K.Nazwa = TEXT("Chata leśnika");
		K.Opis = TEXT("Borowy z większą szkółką: sadzi młode drzewa w promieniu 45 m. Chata kryta gontem, z kominem.");
		K.Model = TEXT("SM_ChataBorowego2");
		K.Polowa = FVector2D(520.f, 520.f);
		K.Promien = 850.f;
		K.Koszt = FSurowce(6, 4, 0);
		K.Zasieg = 4500.f;
		K.Poziom = 2;
		K.Ulepszenie = &ChataBorowego3;
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::ChataBorowego3()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K = ChataBorowego2();
		K.Nazwa = TEXT("Leśniczówka");
		K.Opis = TEXT("Dwie rodziny borowych, dwie szkółki i grządka siewek: sadzą młode drzewa w promieniu 50 m.");
		K.Model = TEXT("SM_ChataBorowego3");
		K.Polowa = FVector2D(560.f, 560.f);
		K.Promien = 900.f;
		K.ObwodMin = FVector2D(-580.f, -320.f);
		K.Koszt = FSurowce(6, 6, 4);
		K.Miejsc = 2;
		K.Zasieg = 5000.f;
		K.Poziom = 3;
		K.Ulepszenie = nullptr;
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Pilarnia()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Pilarnia");
		K.Opis = TEXT("Wiata z dołem do piłowania. Pilarze przynoszą kłody i tną je ręczną piłą na deski: z jednej kłody dwie deski.");
		K.Kategoria = TEXT("Surowce");
		K.Model = TEXT("SM_Pilarnia");
		K.Klepisko = FLinearColor(0.36f, 0.28f, 0.18f);
		K.Ikona = TEXT("T_Ikona_Pilarnia");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 160.f;
		K.Polowa = FVector2D(480.f, 480.f);   // wiata 8 x 4,8 m, stosy klod i desek przed nia
		K.Promien = 780.f;
		K.ObwodMin = FVector2D(-480.f, -320.f);
		K.ObwodMax = FVector2D(480.f, 320.f);
		K.Sterta = FVector2D(0.f, 460.f);
		K.ZrzutX = FVector2D(-300.f, 300.f);
		K.ZrzutY = 300.f;
		K.Budowniczych = 2;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(0.f, 330.f);
		K.Funkcja = EFunkcja::Pilarnia;
		K.Koszt = FSurowce(8, 0, 0);
		K.Miejsc = 1;
		K.Pojemnosc = 12;
		K.Wyrob = ESurowiec::Deska;
		K.MaskaStosow = 0b011;
		K.Stosy[int32(ESurowiec::Kloda)] = FVector2D(-220.f, 400.f);
		K.Stosy[int32(ESurowiec::Deska)] = FVector2D(220.f, 400.f);
		K.Stanowisko = FVector2D(0.f, 95.f);  // przy dole pod wiata
		return K;
	}();
	return R;
}

// Kamien i jedzenie (etap 2 ekonomii). Wymiary z Tools/budynki.py (Y odwrocone: front, drzwi i material po stronie +Y).
const FRodzajBudynku& FRodzajBudynku::Kamieniolom()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Kamieniołom");
		K.Opis = TEXT("Odsłonięta skała, żuraw i wiata na narzędzia. Kamieniarze kują kamień z lica skały i układają go na stosie: na domy z muru i na kościół.");
		K.Kategoria = TEXT("Surowce");
		K.Model = TEXT("SM_Kamieniolom");
		K.Klepisko = FLinearColor(0.35f, 0.31f, 0.26f); // z okruchami kamienia
		K.Ikona = TEXT("T_Ikona_Kamieniolom");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 120.f;
		K.Polowa = FVector2D(520.f, 450.f);   // klepisko 10,4 m, skala z tylu (-Y w UE) do 4,4 m
		K.Promien = 800.f;
		K.ObwodMin = FVector2D(-560.f, -480.f);
		K.ObwodMax = FVector2D(560.f, 460.f);
		K.Sterta = FVector2D(-200.f, 300.f);
		K.ZrzutX = FVector2D(-350.f, 250.f);
		K.ZrzutY = 60.f;                      // przed licem skaly
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(0.f, 440.f);
		K.Funkcja = EFunkcja::Kamieniolom;
		K.Koszt = FSurowce(6, 2, 0);
		K.Miejsc = 2;
		K.Pojemnosc = 12;
		K.Wyrob = ESurowiec::Kamien;
		K.MaskaStosow = 0b0100;
		K.Stosy[int32(ESurowiec::Kamien)] = FVector2D(80.f, 320.f);
		K.Stanowisko = FVector2D(0.f, 15.f);  // lico skaly na Y = -100
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::ChataZbieracza()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Chata zbieracza");
		K.Opis = TEXT("Zbieracz obchodzi krzaki w promieniu 30 m i znosi jagody i grzyby. Krzak odrasta po kilku minutach.");
		K.Kategoria = TEXT("Jedzenie");
		K.Model = TEXT("SM_ChataZbieracza");
		K.Ikona = TEXT("T_Ikona_ChataZbieracza");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 90.f;
		K.Polowa = FVector2D(470.f, 460.f);   // chata 4,8 x 4 m, suszarnie z boku (+X), kosze z przodu
		K.Promien = 750.f;
		K.ObwodMin = FVector2D(-380.f, -280.f);
		K.ObwodMax = FVector2D(200.f, 280.f);
		K.Sterta = FVector2D(-120.f, 500.f);
		K.ZrzutX = FVector2D(-300.f, 100.f);
		K.ZrzutY = 260.f;
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(-130.f, 320.f);
		K.Funkcja = EFunkcja::Zbieracz;
		K.Koszt = FSurowce(4, 0, 0);
		K.Miejsc = 1;
		K.Pojemnosc = 10;
		K.Wyrob = ESurowiec::Jedzenie;
		K.Zasieg = 3000.f;
		K.MaskaStosow = 0b1000;
		K.Stosy[int32(ESurowiec::Jedzenie)] = FVector2D(120.f, 330.f);
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::ChataMysliwego()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Chata myśliwego");
		K.Opis = TEXT("Myśliwy z łukiem poluje w lesie w promieniu 50 m. Nie zawsze trafi, ale sarna to trzy porcje mięsa. Stawiaj przy lesie.");
		K.Kategoria = TEXT("Jedzenie");
		K.Model = TEXT("SM_ChataMysliwego");
		K.Ikona = TEXT("T_Ikona_ChataMysliwego");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 100.f;
		K.Polowa = FVector2D(480.f, 460.f);   // chata 5,2 x 4,2 m, ramy ze skorami z boku (+X)
		K.Promien = 780.f;
		K.ObwodMin = FVector2D(-400.f, -290.f);
		K.ObwodMax = FVector2D(220.f, 290.f);
		K.Sterta = FVector2D(-120.f, 510.f);
		K.ZrzutX = FVector2D(-300.f, 100.f);
		K.ZrzutY = 270.f;
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(-140.f, 330.f);
		K.Funkcja = EFunkcja::Mysliwy;
		K.Koszt = FSurowce(4, 0, 0);
		K.Miejsc = 1;
		K.Pojemnosc = 10;
		K.Wyrob = ESurowiec::Jedzenie;
		K.Wyrob2 = ESurowiec::Skory; // paczka A krok 3: skora z sarny dla garbarza
		K.Zasieg = 5000.f;
		K.MaskaStosow = 0b1000 | (1u << int32(ESurowiec::Skory));
		K.Stosy[int32(ESurowiec::Jedzenie)] = FVector2D(100.f, 330.f);
		K.Stosy[int32(ESurowiec::Skory)] = FVector2D(-320.f, 440.f);
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Spichlerz()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Spichlerz");
		K.Opis = TEXT("Drewniany spichlerz na kamiennych słupkach, z dala od myszy i wilgoci. Tragarze znoszą tu jedzenie, snopy zboża i mąkę, a rodziny biorą jedzenie na posiłki.");
		K.Kategoria = TEXT("Jedzenie");
		K.Model = TEXT("SM_Spichlerz");
		K.Ikona = TEXT("T_Ikona_Spichlerz");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 140.f;
		K.Polowa = FVector2D(420.f, 460.f);   // 7,2 x 4,8 m, schodki z przodu
		K.Promien = 800.f;
		K.ObwodMin = FVector2D(-440.f, -320.f);
		K.ObwodMax = FVector2D(440.f, 320.f);
		K.Sterta = FVector2D(-150.f, 540.f);
		K.ZrzutX = FVector2D(-300.f, 300.f);
		K.ZrzutY = 330.f;
		K.Budowniczych = 2;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(0.f, 400.f);
		K.Funkcja = EFunkcja::Magazyn;
		K.Koszt = FSurowce(6, 4, 0);
		K.Pojemnosc = 40;
		K.MaskaStosow = 0b111000 | (1 << int32(ESurowiec::Ryby)); // etap 3: tez zboze i maka; paczka A: ryby
		K.Stosy[int32(ESurowiec::Ryby)] = FVector2D(110.f, 620.f);
		K.Stosy[int32(ESurowiec::Jedzenie)] = FVector2D(-210.f, 340.f);
		K.Stosy[int32(ESurowiec::Zboze)] = FVector2D(-470.f, 600.f);
		K.Stosy[int32(ESurowiec::Maka)] = FVector2D(420.f, 520.f);
		return K;
	}();
	return R;
}

// Zboze i chleb (etap 3 ekonomii). Wymiary z Tools/budynki.py (Y odwrocone: front, drzwi i material po stronie +Y).
const FRodzajBudynku& FRodzajBudynku::Pole()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Pole");
		K.Opis = TEXT("Ogrodzone pole: zaznacz na ziemi prostokąt (dwa rogi, od 10 x 10 do 60 x 60 m). Im większe, tym większy plon, ale więcej orki i siewu i więcej rodzin do pracy. Wiosną rolnicy orzą i sieją, latem zboże rośnie, jesienią żną je sierpem i znoszą snopy. Niezebrane do zimy zboże gnije.");
		K.Kategoria = TEXT("Rolnictwo");
		K.Model = TEXT("SM_Pole");
		K.Ikona = TEXT("T_Ikona_Pole");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 2;
		K.CzasBudowy = 40.f;
		K.Polowa = FVector2D(1050.f, 750.f);  // plot 20 x 14 m
		K.Promien = 1300.f;
		K.ObwodMin = FVector2D(-1080.f, -780.f);
		K.ObwodMax = FVector2D(1080.f, 780.f);
		K.Sterta = FVector2D(-400.f, 820.f);  // zerdzie na plot przed polem
		K.ZrzutX = FVector2D(-600.f, 600.f);
		K.ZrzutY = 720.f;
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(0.f, 800.f);    // brama od frontu
		K.Funkcja = EFunkcja::Pole;
		K.Koszt = FSurowce(2, 0, 0);
		K.Miejsc = 2;
		K.Pojemnosc = 20;
		K.Wyrob = ESurowiec::Zboze;
		K.Plon = 20;
		K.MaskaStosow = 0b010000;
		K.Stosy[int32(ESurowiec::Zboze)] = FVector2D(650.f, 560.f); // na miedzy przy bramie
		// 21.11.2026: pole zaznaczane na terenie; bazowe 20 x 14 m (z tego liczy sie plon, praca i koszt wiekszych).
		K.ObszarMin = FVector2D(1000.f, 1000.f);
		K.ObszarMax = FVector2D(6000.f, 6000.f);
		return NaObszar(K, 2000.f, 1400.f);
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Mlyn()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Młyn");
		K.Opis = TEXT("Wiatrak koźlak: drewniana budka na słupie. Młynarz przynosi snopy z pól i spichlerza i miele je na mąkę; skrzydła kręcą się przy pracy.");
		K.Kategoria = TEXT("Rolnictwo");
		K.Model = TEXT("SM_Mlyn");
		K.Ikona = TEXT("T_Ikona_Mlyn");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 160.f;
		K.Polowa = FVector2D(450.f, 600.f);   // kozioł 4,8 m, z tylu schody i dyszel do 6 m
		K.Promien = 850.f;
		K.ObwodMin = FVector2D(-480.f, -420.f);
		K.ObwodMax = FVector2D(480.f, 380.f);
		K.Sterta = FVector2D(-50.f, 420.f);
		K.ZrzutX = FVector2D(-250.f, 250.f);
		K.ZrzutY = 300.f;
		K.Budowniczych = 2;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(0.f, 360.f);
		K.Funkcja = EFunkcja::Przetworca;
		K.Koszt = FSurowce(6, 8, 2);
		K.Miejsc = 1;
		K.Pojemnosc = 12;
		K.Wsad = ESurowiec::Zboze;
		K.Wyrob = ESurowiec::Maka;
		K.Uzysk = 1;
		K.CzasPracy = 8.f;
		K.CoRobi = TEXT("miele zboże na mąkę");
		K.MaskaStosow = 0b110000;
		K.Stosy[int32(ESurowiec::Zboze)] = FVector2D(-330.f, 400.f);
		K.Stosy[int32(ESurowiec::Maka)] = FVector2D(300.f, 400.f);
		K.Stanowisko = FVector2D(0.f, 290.f);  // przed budka, pod skrzydlami
		K.Ruchome = TEXT("SM_Mlyn_Skrzydla");
		K.RuchomeGdzie = FVector(0.f, 195.f, 600.f); // piasta (Tools/budynki.py: MLYN_PIASTA)
		K.RuchomeOs = FVector(0.f, 1.f, 0.f);
		K.RuchomePredkosc = 1.2f;
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Piekarnia()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Piekarnia");
		K.Opis = TEXT("Bielona chata z glinianym piecem chlebowym. Piekarz bierze mąkę z młyna albo spichlerza i piecze chleb: z worka mąki cztery porcje jedzenia.");
		K.Kategoria = TEXT("Rolnictwo");
		K.Model = TEXT("SM_Piekarnia");
		K.Ikona = TEXT("T_Ikona_Piekarnia");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 140.f;
		K.Polowa = FVector2D(470.f, 420.f);   // chata 4,6 x 4,2 m, piec pod daszkiem z boku (-X)
		K.Promien = 780.f;
		K.ObwodMin = FVector2D(-520.f, -320.f);
		K.ObwodMax = FVector2D(420.f, 320.f);
		K.Sterta = FVector2D(-120.f, 510.f);
		K.ZrzutX = FVector2D(-200.f, 250.f);
		K.ZrzutY = 290.f;
		K.Budowniczych = 2;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(160.f, 340.f);
		K.Funkcja = EFunkcja::Przetworca;
		K.Koszt = FSurowce(6, 4, 6);
		K.Miejsc = 1;
		K.Pojemnosc = 16;
		K.Wsad = ESurowiec::Maka;
		K.Wyrob = ESurowiec::Jedzenie;
		K.Uzysk = 4;
		K.CzasPracy = 10.f;
		K.CoRobi = TEXT("piecze chleb");
		K.MaskaStosow = 0b101000;
		K.Stosy[int32(ESurowiec::Maka)] = FVector2D(-330.f, 420.f);
		K.Stosy[int32(ESurowiec::Jedzenie)] = FVector2D(300.f, 480.f);
		K.Stanowisko = FVector2D(-300.f, 190.f); // przed otworem pieca (piec: -300, -40)
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::TartakWodny()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Tartak wodny");
		K.Opis = TEXT("Tartak z kołem wodnym, stawiany nad rzeką (koło od strony wody). Woda napędza piłę ramową: kłody idą na deski dużo szybciej niż w pilarni.");
		K.Kategoria = TEXT("Surowce");
		K.Model = TEXT("SM_TartakWodny");
		K.Ikona = TEXT("T_Ikona_TartakWodny");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 200.f;
		K.Polowa = FVector2D(450.f, 340.f);   // dom 6,6 x 4,8 m, kolo z boku (-X)
		K.Promien = 850.f;
		K.ObwodMin = FVector2D(-520.f, -340.f);
		K.ObwodMax = FVector2D(460.f, 340.f);
		K.Sterta = FVector2D(-100.f, 540.f);
		K.ZrzutX = FVector2D(-250.f, 250.f);
		K.ZrzutY = 310.f;
		K.Budowniczych = 2;
		K.Tragarzy = 2;
		K.Wejscie = FVector2D(200.f, 340.f);
		K.Funkcja = EFunkcja::Przetworca;
		K.Koszt = FSurowce(6, 8, 6);
		K.Miejsc = 1;
		K.Pojemnosc = 16;
		K.Wsad = ESurowiec::Kloda;
		K.Wyrob = ESurowiec::Deska;
		K.Uzysk = 2;
		K.CzasPracy = 4.f;
		K.CoRobi = TEXT("tnie deski na traku");
		K.MaskaStosow = 0b000011;
		K.Stosy[int32(ESurowiec::Kloda)] = FVector2D(-250.f, 480.f);
		K.Stosy[int32(ESurowiec::Deska)] = FVector2D(240.f, 480.f);
		K.Stanowisko = FVector2D(210.f, 190.f); // przed pila ramowa w otwartej czesci
		K.Ruchome = TEXT("SM_TartakWodny_Kolo");
		K.RuchomeGdzie = FVector(-375.f, 0.f, 170.f); // os kola (Tools/budynki.py: KOLO)
		K.RuchomeOs = FVector(1.f, 0.f, 0.f);
		K.RuchomePredkosc = 0.9f;
		K.bRuchStaly = true;
		K.bNadRzeka = true;
		K.Woda = FVector2D(-375.f, -800.f); // koniec koryta mlynowki
		return K;
	}();
	return R;
}

// Etap 4: owce i sukno, ruda i bron. Wymiary z Tools/budynki.py (Y odwrocone: front, drzwi i material po stronie +Y).
const FRodzajBudynku& FRodzajBudynku::Owczarnia()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Owczarnia");
		K.Opis = TEXT("Niska szopa pod strzechą i ogrodzone pastwisko: zaznacz na ziemi prostokąt (dwa rogi, od 14 x 10 do 50 x 50 m), szopa stanie przy tylnym płocie. Im większe pastwisko, tym więcej owiec. Pasterz strzyże te, które mają runo, i znosi wełnę na stos. Runo odrasta po kilku minutach.");
		K.Kategoria = TEXT("Rolnictwo");
		K.Model = TEXT("SM_Owczarnia");
		K.Ikona = TEXT("T_Ikona_Owczarnia");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 130.f;
		K.Polowa = FVector2D(700.f, 560.f);   // plot 13,2 x 9,4 m, szopa z tylu (-Y w UE)
		K.Promien = 950.f;
		K.ObwodMin = FVector2D(-700.f, -470.f);
		K.ObwodMax = FVector2D(700.f, 530.f);
		K.Sterta = FVector2D(0.f, 660.f);
		K.ZrzutX = FVector2D(-300.f, 300.f);
		K.ZrzutY = 560.f;                     // przed plotem
		K.Budowniczych = 2;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(380.f, 580.f);  // brama w plocie (x 3,0..4,6 m)
		K.Funkcja = EFunkcja::Owczarnia;
		K.Koszt = FSurowce(8, 4, 0);
		K.Miejsc = 1;
		K.Pojemnosc = 12;
		K.Wyrob = ESurowiec::Welna;
		K.MaskaStosow = 1 << int32(ESurowiec::Welna);
		K.Stosy[int32(ESurowiec::Welna)] = FVector2D(580.f, 650.f); // za plotem, obok bramy
		K.Stanowisko = FVector2D(310.f, 400.f); // zerdz do strzyzenia przy bramie
		K.Narzedzie = TEXT("SM_Ludz_Nozyce");
		K.CoRobi = TEXT("strzyże owcę");
		K.Owiec = 6;
		K.OdrostWelny = 150.f;
		K.Zagroda = FVector2D(580.f, 440.f);
		// 21.11.2026: pastwisko zaznaczane na terenie; bazowe 14 x 10 m (6 owiec). Szopa: x -6..-1,6 m, y 1..4 m w budynki.py.
		K.ObszarMin = FVector2D(1400.f, 1000.f);
		K.ObszarMax = FVector2D(5000.f, 5000.f);
		K.BudynekMin = FVector2D(-650.f, -450.f);
		K.BudynekMax = FVector2D(-110.f, -50.f);
		return NaObszar(K, 1400.f, 1000.f);
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::ChataTkacza()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Chata tkacza");
		K.Opis = TEXT("Chata z bali z krosnem pod wiatą. Tkacz przynosi wełnę z owczarni i tka sukno: na przeszywanice dla wojów (zbrojownia).");
		K.Kategoria = TEXT("Rzemiosło");
		K.Model = TEXT("SM_ChataTkacza");
		K.Klepisko = FLinearColor(0.36f, 0.28f, 0.18f);
		K.Ikona = TEXT("T_Ikona_ChataTkacza");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 130.f;
		K.Polowa = FVector2D(500.f, 440.f);   // chata 5,2 x 4,4 m, wiata z krosnem z boku (-X)
		K.Promien = 800.f;
		K.ObwodMin = FVector2D(-520.f, -300.f);
		K.ObwodMax = FVector2D(360.f, 300.f);
		K.Sterta = FVector2D(-100.f, 520.f);
		K.ZrzutX = FVector2D(-250.f, 250.f);
		K.ZrzutY = 290.f;
		K.Budowniczych = 2;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(100.f, 330.f);
		K.Funkcja = EFunkcja::Przetworca;
		K.Koszt = FSurowce(6, 6, 0);
		K.Miejsc = 1;
		K.Pojemnosc = 10;
		K.Wsad = ESurowiec::Welna;
		K.Wyrob = ESurowiec::Sukno;
		K.Uzysk = 1;
		K.CzasPracy = 12.f;
		K.CoRobi = TEXT("tka sukno na krośnie");
		K.MaskaStosow = (1 << int32(ESurowiec::Welna)) | (1 << int32(ESurowiec::Sukno));
		K.Stosy[int32(ESurowiec::Welna)] = FVector2D(-250.f, 420.f);
		K.Stosy[int32(ESurowiec::Sukno)] = FVector2D(260.f, 430.f);
		K.Stanowisko = FVector2D(-380.f, 0.f);  // przy krosnie pod wiata
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::KopalniaRudy()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Kopalnia rudy");
		K.Opis = TEXT("Rdzawe wzgórze ze sztolnią i wiatą na narzędzia. Górnicy kują rudę żelaza z lica skały; kowal przetapia ją na groty włóczni.");
		K.Kategoria = TEXT("Surowce");
		K.Model = TEXT("SM_KopalniaRudy");
		K.Klepisko = FLinearColor(0.38f, 0.25f, 0.17f); // rdzawa od rudy
		K.Ikona = TEXT("T_Ikona_KopalniaRudy");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 140.f;
		K.Polowa = FVector2D(520.f, 450.f);   // jak kamieniolom: klepisko 10,4 m, skala z tylu (-Y w UE)
		K.Promien = 800.f;
		K.ObwodMin = FVector2D(-560.f, -480.f);
		K.ObwodMax = FVector2D(560.f, 460.f);
		K.Sterta = FVector2D(-200.f, 300.f);
		K.ZrzutX = FVector2D(-350.f, 250.f);
		K.ZrzutY = 60.f;                      // przed licem skaly
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(0.f, 440.f);
		K.Funkcja = EFunkcja::Kamieniolom;    // gornicy pracuja jak kamieniarze
		K.Koszt = FSurowce(8, 4, 0);
		K.Miejsc = 2;
		K.Pojemnosc = 12;
		K.Wyrob = ESurowiec::Ruda;
		K.MaskaStosow = 1 << int32(ESurowiec::Ruda);
		K.Stosy[int32(ESurowiec::Ruda)] = FVector2D(-120.f, 330.f);
		K.Stanowisko = FVector2D(0.f, 15.f);  // lico skaly na Y = -92
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Kuznia()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Kuźnia");
		K.Opis = TEXT("Wiata z kamiennym paleniskiem, miechem i kowadłem. Kowal przynosi rudę i kłody (na węgiel do paleniska) i kuje groty: z kosza rudy i kłody jedna włócznia.");
		K.Kategoria = TEXT("Rzemiosło");
		K.Model = TEXT("SM_Kuznia");
		K.Klepisko = FLinearColor(0.36f, 0.28f, 0.18f);
		K.Ikona = TEXT("T_Ikona_Kuznia");
		K.Niesione = TEXT("SM_Ludz_Kamien");
		K.Fazy = 4;
		K.CzasBudowy = 160.f;
		K.Polowa = FVector2D(520.f, 480.f);   // wiata 5,6 x 4,4 m, stosy z przodu i z lewej (-X)
		K.Promien = 820.f;
		K.ObwodMin = FVector2D(-320.f, -270.f);
		K.ObwodMax = FVector2D(350.f, 270.f);
		K.Sterta = FVector2D(40.f, 480.f);
		K.ZrzutX = FVector2D(-260.f, 260.f);
		K.ZrzutY = 290.f;
		K.Budowniczych = 2;
		K.Tragarzy = 2;
		K.Wejscie = FVector2D(-100.f, 330.f);
		K.Funkcja = EFunkcja::Przetworca;
		K.Koszt = FSurowce(6, 4, 8);
		K.Wymaga = 1; // paczka A: poziom osady
		K.Miejsc = 1;
		K.Pojemnosc = 10;
		K.Wsad = ESurowiec::Ruda;
		K.Wsad2 = ESurowiec::Kloda;
		K.Wyrob = ESurowiec::Bron;
		K.Uzysk = 1;
		K.CzasPracy = 14.f;
		K.Narzedzie = TEXT("SM_Ludz_Mlotek");
		K.CoRobi = TEXT("kuje groty włóczni");
		K.MaskaStosow = (1 << int32(ESurowiec::Kloda)) | (1 << int32(ESurowiec::Ruda)) | (1 << int32(ESurowiec::Bron));
		K.Stosy[int32(ESurowiec::Ruda)] = FVector2D(-330.f, 450.f);
		K.Stosy[int32(ESurowiec::Kloda)] = FVector2D(330.f, 470.f);
		K.Stosy[int32(ESurowiec::Bron)] = FVector2D(-470.f, 60.f);
		K.Stanowisko = FVector2D(20.f, 130.f); // przed kowadlem (20, 40)
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Zbrojownia()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Zbrojownia");
		K.Opis = TEXT("Zrąb na kamiennym cokole, przed nim plac ćwiczeń ze słomianymi manekinami. Tu trafiają włócznie z kuźni i sukno od tkacza. Mężczyzna z przydzielonej rodziny bierze włócznię i przeszywanicę i ćwiczy jako woj.");
		K.Kategoria = TEXT("Wojsko");
		K.Model = TEXT("SM_Zbrojownia");
		K.Klepisko = FLinearColor(0.36f, 0.28f, 0.18f);
		K.Ikona = TEXT("T_Ikona_Zbrojownia");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 180.f;
		K.Polowa = FVector2D(460.f, 880.f);   // zrab 7,2 x 4,8 m, plac cwiczen do 9 m przed nim
		K.Promien = 1000.f;
		K.ObwodMin = FVector2D(-460.f, -330.f);
		K.ObwodMax = FVector2D(460.f, 330.f);
		K.Sterta = FVector2D(0.f, 560.f);
		K.ZrzutX = FVector2D(-300.f, 300.f);
		K.ZrzutY = 340.f;
		K.Budowniczych = 2;
		K.Tragarzy = 2;
		K.Wejscie = FVector2D(0.f, 340.f);
		K.Funkcja = EFunkcja::Zbrojownia;
		K.Koszt = FSurowce(8, 8, 6);
		K.Wymaga = 1; // paczka A: poziom osady
		K.Miejsc = 3;
		K.Pojemnosc = 20;
		K.MaskaStosow = (1 << int32(ESurowiec::Sukno)) | (1 << int32(ESurowiec::Bron));
		K.Stosy[int32(ESurowiec::Bron)] = FVector2D(-260.f, 440.f);
		K.Stosy[int32(ESurowiec::Sukno)] = FVector2D(260.f, 440.f);
		K.Stanowisko = FVector2D(0.f, 700.f);  // plac cwiczen; manekiny na Y = 820, X = -260 / 0 / 260
		K.CoRobi = TEXT("ćwiczy z włócznią");
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Targ()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Targ");
		K.Opis = TEXT("Plac z trzema kramami pod daszkami i wagą kupiecką. Tragarze znoszą tu towar jak na plac składowy, a kupcy z innych rodów kupują go i sprzedają za grosze (okno rodów, klawisz H).");
		K.Kategoria = TEXT("Handel");
		K.Model = TEXT("SM_Targ");
		K.Klepisko = FLinearColor(0.36f, 0.28f, 0.18f);
		K.Ikona = TEXT("T_Ikona_Targ");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 150.f;
		K.Polowa = FVector2D(650.f, 620.f);   // 12 x 7,4 m: kramy z tylu, waga i stosy z przodu
		K.Promien = 950.f;
		K.ObwodMin = FVector2D(-650.f, -520.f);
		K.ObwodMax = FVector2D(650.f, 520.f);
		K.Sterta = FVector2D(0.f, 650.f);
		K.ZrzutX = FVector2D(-400.f, 400.f);
		K.ZrzutY = 300.f;
		K.Budowniczych = 2;
		K.Tragarzy = 2;
		K.Wejscie = FVector2D(0.f, 600.f);
		K.Funkcja = EFunkcja::Magazyn;
		K.Koszt = FSurowce(10, 6, 0);
		K.Wymaga = 1; // paczka A: poziom osady
		K.Pojemnosc = 20;
		K.MaskaStosow = 0xFFF | (1u << int32(ESurowiec::Ubrania)) | (1u << int32(ESurowiec::Buty)); // kazdy towar do ryb; krok 3: ubrania i buty
		K.Stosy[int32(ESurowiec::Ubrania)] = FVector2D(-200.f, 560.f);
		K.Stosy[int32(ESurowiec::Buty)] = FVector2D(200.f, 560.f);
		K.Stosy[int32(ESurowiec::Opal)] = FVector2D(-480.f, 560.f);
		K.Stosy[int32(ESurowiec::Ryby)] = FVector2D(480.f, 560.f);
		K.Stosy[int32(ESurowiec::Kloda)] = FVector2D(-480.f, 330.f);
		K.Stosy[int32(ESurowiec::Deska)] = FVector2D(-240.f, 330.f);
		K.Stosy[int32(ESurowiec::Kamien)] = FVector2D(0.f, 330.f);
		K.Stosy[int32(ESurowiec::Jedzenie)] = FVector2D(240.f, 330.f);
		K.Stosy[int32(ESurowiec::Zboze)] = FVector2D(480.f, 330.f);
		K.Stosy[int32(ESurowiec::Maka)] = FVector2D(-480.f, 90.f);
		K.Stosy[int32(ESurowiec::Welna)] = FVector2D(-240.f, 90.f);
		K.Stosy[int32(ESurowiec::Sukno)] = FVector2D(0.f, 90.f);
		K.Stosy[int32(ESurowiec::Ruda)] = FVector2D(240.f, 90.f);
		K.Stosy[int32(ESurowiec::Bron)] = FVector2D(480.f, 90.f);
		return K;
	}();
	return R;
}

// Paczka A (23.11.2026). Wymiary z Tools/budynki.py (chata_opalowa, chata_rybaka; Y odwrocone: front po stronie +Y).
const FRodzajBudynku& FRodzajBudynku::ChataOpalowa()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Chata opałowa");
		K.Opis = TEXT("Chatka z wiatą na sągi. Drwal opałowy rąbie kłody na polana: z jednej kłody trzy naręcza. Zimą każda rodzina pali naręcze przy posiłku (bez polan pali kłody).");
		K.Kategoria = TEXT("Surowce");
		K.Model = TEXT("SM_ChataOpalowa");
		K.Ikona = TEXT("T_Ikona_ChataOpalowa");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 80.f;
		K.Polowa = FVector2D(400.f, 440.f);   // chata 4 x 3,8 m, wiata 2,7 m z boku (+X), pniak z przodu
		K.Promien = 700.f;
		K.ObwodMin = FVector2D(-360.f, -270.f);
		K.ObwodMax = FVector2D(160.f, 270.f);
		K.Sterta = FVector2D(-100.f, 500.f);
		K.ZrzutX = FVector2D(-250.f, 50.f);
		K.ZrzutY = 250.f;
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(-140.f, 300.f);
		K.Funkcja = EFunkcja::Przetworca;
		K.Koszt = FSurowce(4, 0, 0);
		K.Miejsc = 1;
		K.Pojemnosc = 16;
		K.Wsad = ESurowiec::Kloda;
		K.Wyrob = ESurowiec::Opal;
		K.Uzysk = 3;
		K.CzasPracy = 6.f;
		K.Narzedzie = TEXT("SM_Ludz_Siekiera");
		K.CoRobi = TEXT("rąbie polana");
		K.MaskaStosow = (1 << int32(ESurowiec::Kloda)) | (1 << int32(ESurowiec::Opal));
		K.Stosy[int32(ESurowiec::Kloda)] = FVector2D(-260.f, 540.f);
		K.Stosy[int32(ESurowiec::Opal)] = FVector2D(240.f, 420.f);
		K.Stanowisko = FVector2D(-60.f, 430.f); // przed pniakiem (-60, 340)
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::ChataRybaka()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Chata rybaka");
		K.Opis = TEXT("Stawiana tyłem do rzeki. Rybak siada na brzegu z wędką i przynosi ryby. Ryby to drugi rodzaj jedzenia: rodziny, które mają jagody lub chleb i ryby, są bardziej zadowolone.");
		K.Kategoria = TEXT("Jedzenie");
		K.Model = TEXT("SM_ChataRybaka");
		K.Ikona = TEXT("T_Ikona_ChataRybaka");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 90.f;
		K.Polowa = FVector2D(510.f, 470.f);   // chata 4,8 x 3,8 m, sieci z boku (-X), lodz z tylu (od wody)
		K.Promien = 750.f;
		K.ObwodMin = FVector2D(-300.f, -270.f);
		K.ObwodMax = FVector2D(300.f, 270.f);
		K.Sterta = FVector2D(0.f, 500.f);
		K.ZrzutX = FVector2D(-200.f, 200.f);
		K.ZrzutY = 250.f;
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(-100.f, 300.f);
		K.Funkcja = EFunkcja::Rybak;
		K.Koszt = FSurowce(5, 2, 0);
		K.Miejsc = 1;
		K.Pojemnosc = 10;
		K.Wyrob = ESurowiec::Ryby;
		K.CzasPracy = 14.f; // sekundy z wedka na jedna probe
		K.MaskaStosow = 1 << int32(ESurowiec::Ryby);
		K.Stosy[int32(ESurowiec::Ryby)] = FVector2D(200.f, 340.f);
		K.bNadRzeka = true;
		K.Woda = FVector2D(0.f, -800.f); // za chata: rzeka do 6 m od tego punktu
		return K;
	}();
	return R;
}

FBox2D FRodzajBudynku::Obrys() const
{
	if (ZObszarem())
	{
		return FBox2D(PlotMin - FVector2D(50.f, 50.f), PlotMax + FVector2D(50.f, 50.f));
	}
	return FBox2D(-Polowa, Polowa);
}

FString FRodzajBudynku::OpisObszaru() const
{
	const int32 M2 = FMath::RoundToInt32(Metry2());
	return FString::Printf(TEXT("%d × %d m, %s ha (%d m²)"), FMath::RoundToInt32(Obszar.X / 100.f), FMath::RoundToInt32(Obszar.Y / 100.f),
		*FString::Printf(TEXT("%.2f"), M2 / 1e4f).Replace(TEXT("."), TEXT(",")), M2);
}

FRodzajBudynku FRodzajBudynku::NaObszar(const FRodzajBudynku& Baza, float W, float H)
{
	FRodzajBudynku K = Baza;
	if (Baza.ObszarMax.X <= 0.f)
	{
		return K;
	}
	W = FMath::Clamp(W, float(Baza.ObszarMin.X), float(Baza.ObszarMax.X));
	H = FMath::Clamp(H, float(Baza.ObszarMin.Y), float(Baza.ObszarMax.Y));
	K.Obszar = FVector2D(W, H);
	const float Plotu = 2.f * (W + H) / 100.f; // metry plotu
	const float Hx = W / 2.f, Hy = H / 2.f;
	if (Baza.Funkcja == EFunkcja::Pole)
	{
		// Bazowe 20 x 14 m: plon 20 snopow, 2 rodziny, orka i siew razy CzynnikObszaru (AOsada::PracujNaPolu).
		const float F = W * H / (2000.f * 1400.f);
		K.CzynnikObszaru = F;
		K.SrodekObszaru = FVector2D::ZeroVector;
		K.PlotMin = FVector2D(-Hx, -Hy);
		K.PlotMax = FVector2D(Hx, Hy);
		K.Brama = FVector2D(-140.f, 140.f);
		K.UprawaMin = FVector2D(-Hx + 60.f, -Hy + 60.f);
		K.UprawaMax = FVector2D(Hx - 60.f, Hy - 260.f); // od frontu miedza 2,6 m ze stosem snopow
		K.Polowa = FVector2D(Hx + 50.f, Hy + 50.f);
		K.Promien = float(FVector2D(Hx, Hy).Size()) + 100.f;
		K.ObwodMin = FVector2D(-Hx - 80.f, -Hy - 80.f);
		K.ObwodMax = FVector2D(Hx + 80.f, Hy + 80.f);
		K.Sterta = FVector2D(-FMath::Min(400.f, Hx - 100.f), Hy + 120.f);
		K.ZrzutX = FVector2D(-FMath::Min(600.f, Hx - 100.f), FMath::Min(600.f, Hx - 100.f));
		K.ZrzutY = Hy + 20.f;
		K.Wejscie = FVector2D(0.f, Hy + 100.f);
		K.Plon = FMath::Max(4, FMath::RoundToInt32(20.f * F));
		K.Pojemnosc = FMath::Max(20, K.Plon);
		K.Miejsc = FMath::Clamp(FMath::RoundToInt32(2.f * F), 1, 8);
		K.Koszt = FSurowce(FMath::Max(2, FMath::RoundToInt32(Plotu / 30.f)), 0, 0);
		K.CzasBudowy = 40.f * Plotu / 68.f;
		K.Stosy[int32(ESurowiec::Zboze)] = FVector2D(FMath::Max(0.f, Hx - 350.f), Hy - 140.f);
	}
	else
	{
		// Owczarnia: szopa (model) przy tylnym boku, pastwisko przed nia. Bazowe 14 x 10 m: 6 owiec, 1 rodzina.
		const float Sy = 440.f, Yf = H - Sy;
		K.CzynnikObszaru = W * H / (1400.f * 1000.f);
		K.SrodekObszaru = FVector2D(0.f, (Yf - Sy) / 2.f);
		K.PlotMin = FVector2D(-Hx, -Sy);
		K.PlotMax = FVector2D(Hx, Yf);
		const float Gx = FMath::Clamp(380.f, -Hx + 150.f, Hx - 150.f);
		K.Brama = FVector2D(Gx - 80.f, Gx + 80.f);
		K.Polowa = FVector2D(Hx + 50.f, FMath::Max(Sy, Yf) + 50.f);
		K.Promien = float(FMath::Max(FVector2D(Hx, Sy).Size(), FVector2D(Hx, Yf).Size())) + 150.f;
		K.Wejscie = FVector2D(Gx, Yf + 80.f);
		K.Stosy[int32(ESurowiec::Welna)] = FVector2D(Gx + 200.f, Yf + 90.f); // za plotem, obok bramy
		K.Zagroda = FVector2D(Hx - 100.f, Yf - 100.f);
		const float Pastwisko = W * (Yf - 80.f) / 1e4f; // m2, na ktorych chodza owce
		K.Owiec = FMath::Clamp(FMath::RoundToInt32(Pastwisko / 11.f), 4, 30);
		K.Miejsc = FMath::Clamp(FMath::CeilToInt32(K.Owiec / 10.f), 1, 3);
		K.Pojemnosc = 12 + K.Owiec / 2;
		K.Koszt = FSurowce(4 + FMath::RoundToInt32(Plotu / 12.f), 4, 0);
		K.CzasBudowy = 90.f + Plotu * 0.85f;
	}
	return K;
}

// Woly (23.11.2026, prosba uzytkownika): zagroda daje osadzie woly, ktore ciagna klody (Ludzie/Wol.h). Bez rodzin.
// Wymiary z Tools/budynki.py (zagroda_wolow; Y odwrocone: wybieg z przodu +Y, wiata / obora z tylu).
const FRodzajBudynku& FRodzajBudynku::ZagrodaWolow()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Zagroda wołów");
		K.Opis = TEXT("Wiata pod strzechą i ogrodzony wybieg. Daje osadzie dwa woły, które same ciągną kłody (do 6 naraz) od drwali na budowy i do magazynów. Ulepszona do obory daje dwa kolejne. Na start osada ma już dwa woły przy obozie.");
		K.Kategoria = TEXT("Surowce");
		K.Model = TEXT("SM_ZagrodaWolow");
		K.Ikona = TEXT("T_Ikona_ZagrodaWolow");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 90.f;
		K.Polowa = FVector2D(650.f, 580.f);   // wybieg 12 x 10,6 m
		K.Promien = 850.f;
		K.ObwodMin = FVector2D(-420.f, -540.f);
		K.ObwodMax = FVector2D(420.f, -80.f);
		K.Sterta = FVector2D(-250.f, 680.f);
		K.ZrzutX = FVector2D(-300.f, 300.f);
		K.ZrzutY = -60.f;
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(0.f, 300.f);
		K.Koszt = FSurowce(5, 2, 0);
		K.Poziom = 1;
		K.Ulepszenie = &ZagrodaWolow2;
		K.Wolow = 2;
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::ZagrodaWolow2()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K = ZagrodaWolow();
		K.Nazwa = TEXT("Obora");
		K.Opis = TEXT("Obora z bali i większy paśnik. Daje osadzie razem cztery woły do zwózki kłód.");
		K.Model = TEXT("SM_ZagrodaWolow2");
		K.ObwodMin = FVector2D(-500.f, -580.f);
		K.ObwodMax = FVector2D(500.f, -60.f);
		K.Koszt = FSurowce(6, 4, 2);
		K.Poziom = 2;
		K.Ulepszenie = nullptr;
		K.Wolow = 4;
		return K;
	}();
	return R;
}

// Konie (23.11.2026, prosba uzytkownika "woly i konie jak w Manor Lords"): stajnia daje 2 konie z wozami.
// Wymiary z Tools/budynki.py (stajnia: chata 10 x 3,2 m z tylu, wybieg jak w zagrodzie wolow).
const FRodzajBudynku& FRodzajBudynku::Stajnia()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K = ZagrodaWolow();
		K.Nazwa = TEXT("Stajnia");
		K.Opis = TEXT("Stajnia z wybiegiem. Daje osadzie dwa konie z wozami: same wożą deski i kamień (do 8 naraz) od pilarni i kamieniołomów na budowy i do magazynów. Szybsze od wołów. Od poziomu wsi.");
		K.Model = TEXT("SM_Stajnia");
		K.Ikona = TEXT("T_Ikona_Stajnia");
		K.ObwodMin = FVector2D(-580.f, -560.f);
		K.ObwodMax = FVector2D(580.f, -60.f);
		K.Koszt = FSurowce(6, 4, 2);
		K.CzasBudowy = 110.f;
		K.Poziom = 0;
		K.Ulepszenie = nullptr;
		K.Wolow = 0;
		K.Koni = 2;
		K.Wymaga = 1;
		return K;
	}();
	return R;
}

// Paczka A, krok 3 (23.11.2026): rzemioslo i piwo. Modele Tools/budynki.py (warsztat: chata 5,2 x 4 m, wyposazenie
// przed nia; karczma 8,8 x 5,2 m). Y odwrocone: front +Y.
namespace
{
	FRodzajBudynku Warsztat(const TCHAR* Nazwa, const TCHAR* Opis, const TCHAR* Model, ESurowiec Wsad, ESurowiec Wyrob, int32 Uzysk,
		float CzasPracy, const TCHAR* Narzedzie, const TCHAR* CoRobi, FSurowce Koszt)
	{
		FRodzajBudynku K;
		K.Nazwa = Nazwa;
		K.Opis = Opis;
		K.Kategoria = TEXT("Rzemiosło");
		K.Model = Model;
		K.Ikona = FString(TEXT("T_Ikona_")) + (Model + 3); // SM_Garbarnia -> T_Ikona_Garbarnia
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 100.f;
		K.Polowa = FVector2D(420.f, 520.f);
		K.Promien = 750.f;
		K.ObwodMin = FVector2D(-340.f, -260.f);
		K.ObwodMax = FVector2D(340.f, 260.f);
		K.Sterta = FVector2D(-80.f, 520.f);
		K.ZrzutX = FVector2D(-250.f, 150.f);
		K.ZrzutY = 260.f;
		K.Budowniczych = 1;
		K.Tragarzy = 1;
		K.Wejscie = FVector2D(0.f, 300.f);
		K.Funkcja = EFunkcja::Przetworca;
		K.Koszt = Koszt;
		K.Miejsc = 1;
		K.Pojemnosc = 12;
		K.Wsad = Wsad;
		K.Wyrob = Wyrob;
		K.Uzysk = Uzysk;
		K.CzasPracy = CzasPracy;
		K.Narzedzie = Narzedzie;
		K.CoRobi = CoRobi;
		K.Wymaga = 1; // od wsi
		K.MaskaStosow = (1u << int32(Wsad)) | (1u << int32(Wyrob));
		K.Stosy[int32(Wsad)] = FVector2D(-300.f, 480.f);
		K.Stosy[int32(Wyrob)] = FVector2D(320.f, 480.f);
		K.Stanowisko = FVector2D(0.f, 380.f);
		return K;
	}
}

const FRodzajBudynku& FRodzajBudynku::Garbarnia()
{
	static const FRodzajBudynku R = Warsztat(TEXT("Garbarnia"),
		TEXT("Garbarz moczy surowe skóry od myśliwego w kadziach i wyprawia je na skórę dla szewca. Od poziomu wsi."),
		TEXT("SM_Garbarnia"), ESurowiec::Skory, ESurowiec::Skora, 1, 14.f, TEXT("SM_Ludz_Nozyce"), TEXT("wyprawia skóry"), FSurowce(4, 2, 0));
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Szewc()
{
	static const FRodzajBudynku R = Warsztat(TEXT("Warsztat szewca"),
		TEXT("Szewc szyje ze skóry dwie pary butów. Buty idą na targ, a obute rodziny są bardziej zadowolone. Od poziomu wsi."),
		TEXT("SM_Szewc"), ESurowiec::Skora, ESurowiec::Buty, 2, 12.f, TEXT("SM_Ludz_Mlotek"), TEXT("szyje buty"), FSurowce(3, 3, 0));
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Krawiec()
{
	static const FRodzajBudynku R = Warsztat(TEXT("Warsztat krawca"),
		TEXT("Krawiec szyje z beli sukna dwa ubrania. Ubrania idą na targ i cieszą bardziej niż samo sukno. Od poziomu wsi."),
		TEXT("SM_Krawiec"), ESurowiec::Sukno, ESurowiec::Ubrania, 2, 12.f, TEXT("SM_Ludz_Nozyce"), TEXT("szyje ubrania"), FSurowce(3, 3, 0));
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Slodownia()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K = Warsztat(TEXT("Słodownia"),
			TEXT("Słodownik moczy zboże i suszy je nad piecem na słód dla browaru. Od poziomu wsi."),
			TEXT("SM_Slodownia"), ESurowiec::Zboze, ESurowiec::Slod, 1, 10.f, TEXT("SM_Ludz_Motyka"), TEXT("suszy słód"), FSurowce(4, 2, 4));
		K.Kategoria = TEXT("Jedzenie");
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Browar()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K = Warsztat(TEXT("Browar"),
			TEXT("Piwowar warzy ze słodu dwie baryłki piwa. Piwo trafia do karczmy. Od poziomu wsi."),
			TEXT("SM_Browar"), ESurowiec::Slod, ESurowiec::Piwo, 2, 14.f, TEXT("SM_Ludz_Motyka"), TEXT("warzy piwo"), FSurowce(4, 4, 4));
		K.Kategoria = TEXT("Jedzenie");
		return K;
	}();
	return R;
}

const FRodzajBudynku& FRodzajBudynku::Karczma()
{
	static const FRodzajBudynku R = []()
	{
		FRodzajBudynku K;
		K.Nazwa = TEXT("Karczma");
		K.Opis = TEXT("Długa karczma ze stołami przed wejściem. Tragarze znoszą tu piwo z browaru, a rodziny co drugi posiłek wypijają baryłkę (wygody). Bez karczmy osada nie zostanie miastem. Od poziomu wsi.");
		K.Kategoria = TEXT("Handel");
		K.Model = TEXT("SM_Karczma");
		K.Ikona = TEXT("T_Ikona_Karczma");
		K.Niesione = TEXT("SM_Ludz_Bal");
		K.Fazy = 4;
		K.CzasBudowy = 160.f;
		K.Polowa = FVector2D(560.f, 560.f);
		K.Promien = 850.f;
		K.ObwodMin = FVector2D(-520.f, -330.f);
		K.ObwodMax = FVector2D(520.f, 330.f);
		K.Sterta = FVector2D(-100.f, 600.f);
		K.ZrzutX = FVector2D(-300.f, 300.f);
		K.ZrzutY = 320.f;
		K.Budowniczych = 2;
		K.Tragarzy = 2;
		K.Wejscie = FVector2D(0.f, 380.f);
		K.Funkcja = EFunkcja::Magazyn;
		K.Koszt = FSurowce(8, 6, 4);
		K.Wymaga = 1;
		K.Pojemnosc = 16;
		K.MaskaStosow = 1u << int32(ESurowiec::Piwo);
		K.Stosy[int32(ESurowiec::Piwo)] = FVector2D(330.f, 520.f);
		return K;
	}();
	return R;
}

const TArray<const FRodzajBudynku*>& FRodzajBudynku::DoBudowania()
{
	// Poziomy (22.11.2026): na pasku tylko poziom 1 domu (chata) i kosciola (drewniany); wyzsze przez ulepszenie.
	static const TArray<const FRodzajBudynku*> L = { &Chata(),
		&PlacSkladowy(), &ChataDrwala(), &ChataBorowego(), &Pilarnia(), &Kamieniolom(),
		&ChataZbieracza(), &ChataMysliwego(), &Spichlerz(), &Pole(), &Mlyn(), &Piekarnia(), &TartakWodny(),
		&Owczarnia(), &ChataTkacza(), &KopalniaRudy(), &Kuznia(), &Zbrojownia(), &Targ(), &KosciolDrewniany(),
		&ChataOpalowa(), &ChataRybaka(), &ZagrodaWolow(), &Stajnia(), &Garbarnia(), &Szewc(), &Krawiec(), &Slodownia(),
		&Browar(), &Karczma() };
	return L;
}

ABudowa::ABudowa()
{
	// Tick tylko dla czesci ruchomej (skrzydla mlyna, kolo wodne) i owiec w owczarni, wlaczany w Ustaw.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	Siatka = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Siatka"));
	RootComponent = Siatka;
	Siatka->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Siatka->SetMobility(EComponentMobility::Movable);
	SetCanBeDamaged(false);
}

void ABudowa::Ustaw(const FRodzajBudynku& Rodzaj, bool bPodglad)
{
	RodzajB = Rodzaj;
	bPodgladB = bPodglad;
	// Poziomy (22.11.2026): dom losuje jeden z wariantow modelu (podglad pod kursorem zawsze wariant 0).
	if (!bPodglad && WariantB < 0)
	{
		WariantB = Rodzaj.Wariantow > 1 ? (NastepnyWariant >= 0 ? NastepnyWariant % Rodzaj.Wariantow : FMath::RandRange(0, Rodzaj.Wariantow - 1)) : 0;
		NastepnyWariant = -1;
	}
	OdbijWariant(RodzajB);
	const FString ModelW = ModelWariantu(Rodzaj);
	Modele.Reset();
	for (int32 F = 1; F <= Rodzaj.Fazy; ++F)
	{
		const FString Sciezka = Rodzaj.Fazy == 1
			? FString::Printf(TEXT("/Game/Swiat/Budynki/%s.%s"), *Rodzaj.Model, *Rodzaj.Model)
			: FString::Printf(TEXT("/Game/Swiat/Budynki/%s_F%d.%s_F%d"), *ModelW, F, *ModelW, F);
		UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Sciezka);
		UE_CLOG(!M, LogTemp, Warning, TEXT("Budowa: brak modelu %s (uruchom Tools/Zbuduj_Budynki.bat)"), *Sciezka);
		Modele.Add(M);
	}
	// Etap 3: czesc ruchoma (skrzydla wiatraka, kolo wodne) - osobny model, os przez jego poczatek ukladu.
	if (!Rodzaj.Ruchome.IsEmpty() && !Ruchoma)
	{
		const FString Sc = FString::Printf(TEXT("/Game/Swiat/Budynki/%s.%s"), *Rodzaj.Ruchome, *Rodzaj.Ruchome);
		UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Sc);
		UE_CLOG(!M, LogTemp, Warning, TEXT("Budowa: brak modelu %s (uruchom Tools/Zbuduj_Budynki.bat)"), *Sc);
		Ruchoma = NewObject<UStaticMeshComponent>(this);
		Ruchoma->SetupAttachment(Siatka);
		Ruchoma->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ruchoma->SetMobility(EComponentMobility::Movable);
		Ruchoma->SetRelativeLocation(Rodzaj.RuchomeGdzie);
		Ruchoma->SetStaticMesh(M);
		Ruchoma->SetCastShadow(!bPodglad);
		Ruchoma->RegisterComponent();
		Ruchoma->SetVisibility(bPodglad);
		SetActorTickEnabled(!bPodglad);
	}
	// Pole: wyglad wedlug uprawy (SM_Pole_<Uprawa>, Ugor = ostatnia faza budowy).
	ModeleUprawy.Reset();
	if (Rodzaj.Funkcja == EFunkcja::Pole && !bPodglad)
	{
		static const TCHAR* NazwyUpraw[] = { TEXT("Zaorane"), TEXT("Zasiane"), TEXT("Rosnie"), TEXT("Dojrzale"), TEXT("Scierniska") };
		for (const TCHAR* N : NazwyUpraw)
		{
			const FString Sc = FString::Printf(TEXT("/Game/Swiat/Budynki/SM_Pole_%s.SM_Pole_%s"), N, N);
			UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *Sc);
			UE_CLOG(!M, LogTemp, Warning, TEXT("Budowa: brak modelu %s (uruchom Tools/Zbuduj_Budynki.bat)"), *Sc);
			ModeleUprawy.Add(M);
		}
	}
	// Owczarnia (etap 4): stado chodzi po pastwisku przed szopa; widac je dopiero na gotowym budynku.
	Owce.Reset();
	Stado.Reset();
	if (Rodzaj.Funkcja == EFunkcja::Owczarnia && Rodzaj.Owiec > 0 && !bPodglad)
	{
		ModelOwcy = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Swiat/Budynki/SM_Owca.SM_Owca"));
		ModelOwcyStrzyzonej = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Swiat/Budynki/SM_Owca_Strzyzona.SM_Owca_Strzyzona"));
		UE_CLOG(!ModelOwcy || !ModelOwcyStrzyzonej, LogTemp, Warning, TEXT("Budowa: brak modelu SM_Owca (uruchom Tools/Zbuduj_Budynki.bat)"));
		const FVector2D Pas = Rodzaj.Zagroda;
		for (int32 k = 0; k < Rodzaj.Owiec; ++k)
		{
			FOwca O;
			O.Poz = FVector2D(FMath::FRandRange(-Pas.X, Pas.X), FMath::FRandRange(80.0, Pas.Y));
			O.Cel = O.Poz;
			O.Kierunek = FMath::FRandRange(0.f, 360.f);
			O.Postoj = FMath::FRandRange(0.f, 6.f);
			O.Odrost = k % 3 == 2 ? FMath::FRandRange(0.3f, 1.f) * Rodzaj.OdrostWelny : 0.f; // co trzecia swiezo ostrzyzona
			Stado.Add(O);
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
			C->SetupAttachment(Siatka);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetMobility(EComponentMobility::Movable);
			C->SetRelativeScale3D(FVector(1.25f)); // jak ludzie: troche wieksze niz naprawde, zeby bylo je widac
			C->SetStaticMesh(O.Odrost > 0.f ? ModelOwcyStrzyzonej.Get() : ModelOwcy.Get());
			C->RegisterComponent();
			C->SetVisibility(false);
			Owce.Add(C);
		}
		SetActorTickEnabled(true);
	}
	// Pole i pastwisko (21.11.2026): plot z segmentow, pole z kafli uprawy (InstancedStaticMesh na terenie).
	Odcinki.Reset();
	Konce.Reset();
	PlotuPelnego = -1;
	if (Rodzaj.ZObszarem())
	{
		auto Wczytaj = [](const TCHAR* Nazwa)
		{
			UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Swiat/Budynki/%s.%s"), Nazwa, Nazwa));
			UE_CLOG(!M, LogTemp, Warning, TEXT("Budowa: brak modelu %s (uruchom Tools/Zbuduj_Budynki.bat)"), Nazwa);
			return M;
		};
		auto NowyIsm = [this, bPodglad](UStaticMesh* M)
		{
			UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(this);
			C->SetupAttachment(Siatka);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetMobility(EComponentMobility::Movable);
			C->SetStaticMesh(M);
			C->SetCastShadow(!bPodglad);
			C->RegisterComponent();
			return C;
		};
		if (!Plot)
		{
			Plot = NowyIsm(Wczytaj(TEXT("SM_Plot")));
			Paliki = NowyIsm(Wczytaj(TEXT("SM_Plot_Palik")));
		}
		if (Rodzaj.Funkcja == EFunkcja::Pole && !Lan)
		{
			// 22.11.2026: zamiast kafli 5 x 5 m (plyty nad terenem) jedna siatka na terenie; material jak budynki (kolor wierzcholkow).
			Lan = NewObject<UProceduralMeshComponent>(this);
			Lan->SetupAttachment(Siatka);
			Lan->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Lan->SetMobility(EComponentMobility::Movable);
			Lan->SetCastShadow(!bPodglad);
			Lan->RegisterComponent();
			Lan->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Swiat/Budynki/M_Bud_Tynk.M_Bud_Tynk")));
			if (!bPodglad)
			{
				ModeleZboza = { Wczytaj(TEXT("SM_Zboze_Zielone")), Wczytaj(TEXT("SM_Zboze_Dojrzale")), Wczytaj(TEXT("SM_Zboze_Scierniska")) };
				Zboze = NowyIsm(nullptr);
				Zboze->SetCastShadow(false); // gesty lan sam sie zacienial i wychodzil prawie czarny
			}
		}
		// Odcinki plotu po kolei wokol prostokata, od bramy w lewo (front = +Y), co ok. 2 m; ostatni konczy sie slupkiem przy bramie.
		const FVector2D A = Rodzaj.PlotMin, B = Rodzaj.PlotMax;
		const FVector2D Biegi[][2] = {
			{ FVector2D(Rodzaj.Brama.X, B.Y), FVector2D(A.X, B.Y) },
			{ FVector2D(A.X, B.Y), FVector2D(A.X, A.Y) },
			{ FVector2D(A.X, A.Y), FVector2D(B.X, A.Y) },
			{ FVector2D(B.X, A.Y), FVector2D(B.X, B.Y) },
			{ FVector2D(B.X, B.Y), FVector2D(Rodzaj.Brama.Y, B.Y) } };
		for (const auto& Bieg : Biegi)
		{
			const double L = FVector2D::Distance(Bieg[0], Bieg[1]);
			if (L < 10.0)
			{
				continue;
			}
			const int32 N = FMath::Max(1, FMath::RoundToInt32(L / 200.0));
			for (int32 i = 0; i < N; ++i)
			{
				Odcinki.Add(TPair<FVector2D, FVector2D>(FMath::Lerp(Bieg[0], Bieg[1], double(i) / N), FMath::Lerp(Bieg[0], Bieg[1], double(i + 1) / N)));
			}
		}
		Konce.Add(FVector2D(Rodzaj.Brama.Y, B.Y));
	}
	if (bPodglad)
	{
		// Podglad: gotowy budynek, polprzezroczysty, bez cienia.
		Siatka->SetStaticMesh(Modele.Num() && !Lan ? Modele.Last().Get() : nullptr);
		Siatka->SetCastShadow(false);
		if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Swiat/Budynki/M_Bud_Podglad.M_Bud_Podglad")))
		{
			MaterialPodgladu = UMaterialInstanceDynamic::Create(Mat, this);
			for (int32 i = 0; i < Siatka->GetNumMaterials(); ++i)
			{
				Siatka->SetMaterial(i, MaterialPodgladu);
			}
			for (int32 i = 0; Ruchoma && i < Ruchoma->GetNumMaterials(); ++i)
			{
				Ruchoma->SetMaterial(i, MaterialPodgladu);
			}
			for (UInstancedStaticMeshComponent* C : { Plot.Get(), Paliki.Get() })
			{
				for (int32 i = 0; C && i < C->GetNumMaterials(); ++i)
				{
					C->SetMaterial(i, MaterialPodgladu);
				}
			}
			if (Lan)
			{
				Lan->SetMaterial(0, MaterialPodgladu);
			}
		}
		OdswiezObszar(true);
		return;
	}
	PostepBudowy = Rodzaj.Fazy == 1 ? 1.f : 0.f; // namiot, ognisko: od razu gotowe
	FazaTeraz = -1;

	// Stosy towaru: komponenty tylko dla surowcow z MaskaStosow, modele wszystkich poziomow wczytane z gory.
	ModeleStosow.Reset();
	for (ESurowiec S : WszystkieSurowce)
	{
		static const TCHAR* Nazwy[] = { TEXT("Kloda"), TEXT("Deska"), TEXT("Kamien"), TEXT("Jedzenie"), TEXT("Zboze"), TEXT("Maka"),
			TEXT("Welna"), TEXT("Sukno"), TEXT("Ruda"), TEXT("Bron"), TEXT("Opal"), TEXT("Ryby"),
			TEXT("Skory"), TEXT("Skora"), TEXT("Buty"), TEXT("Ubrania"), TEXT("Slod"), TEXT("Piwo") };
		static_assert(UE_ARRAY_COUNT(Nazwy) == int32(ESurowiec::Ile), "nazwa stosu dla kazdego surowca");
		for (int32 Poz = 1; Poz <= 4; ++Poz)
		{
			UStaticMesh* M = nullptr;
			if (Rodzaj.MaStos(S))
			{
				const FString Sc = FString::Printf(TEXT("/Game/Swiat/Budynki/SM_Stos_%s_%d.SM_Stos_%s_%d"), Nazwy[int32(S)], Poz, Nazwy[int32(S)], Poz);
				M = LoadObject<UStaticMesh>(nullptr, *Sc);
				UE_CLOG(!M, LogTemp, Warning, TEXT("Budowa: brak modelu %s (uruchom Tools/Zbuduj_Budynki.bat)"), *Sc);
			}
			ModeleStosow.Add(M);
		}
		UStaticMeshComponent* C = nullptr;
		if (Rodzaj.MaStos(S))
		{
			C = NewObject<UStaticMeshComponent>(this);
			C->SetupAttachment(Siatka);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetMobility(EComponentMobility::Movable);
			C->SetRelativeLocation(FVector(Rodzaj.Stosy[int32(S)], 0.0));
			if (Rodzaj.ZObszarem())
			{
				// Pole i pastwisko leza na pochylym terenie: stos na ziemi, nie na wysokosci srodka.
				const FVector2D W = ZUkladu(Rodzaj.Stosy[int32(S)]);
				C->SetWorldLocation(FVector(W, FSwiat::Wysokosc(W.X, W.Y)));
			}
			C->RegisterComponent();
			C->SetVisibility(false);
		}
		Stosy.Add(C);
	}
	Podloze();
	WczytajZniszczenia();
	OdswiezFaze();
	OdswiezObszar(true);
}

bool ABudowa::Wydaje(ESurowiec S) const
{
	return Gotowy() && (RodzajB.Magazynuje() || RodzajB.Wyrob == S || RodzajB.Wyrob2 == S);
}

int32 ABudowa::Miejsce(ESurowiec S) const
{
	if (!Gotowy() || !RodzajB.Magazynuje() || !RodzajB.MaStos(S))
	{
		return 0;
	}
	return FMath::Max(0, RodzajB.Pojemnosc - Zapas[S] - WDrodze[S]);
}

int32 ABudowa::Brakuje(ESurowiec S) const
{
	return Gotowy() ? 0 : FMath::Max(0, RodzajB.Koszt[S] - Dostarczone[S] - WDrodze[S]);
}

float ABudowa::Material() const
{
	const int32 Razem = RodzajB.Koszt.Suma();
	if (Razem <= 0)
	{
		return 1.f;
	}
	int32 Jest = 0;
	for (ESurowiec S : WszystkieSurowce)
	{
		Jest += FMath::Min(Dostarczone[S], RodzajB.Koszt[S]);
	}
	return float(Jest) / float(Razem);
}

bool ABudowa::CzekaNaMaterial() const
{
	const float M = Material();
	return !Gotowy() && M < 1.f && PostepBudowy >= M - 1e-4f;
}

void ABudowa::ZmienZapas(ESurowiec S, int32 Ile)
{
	Zapas[S] = FMath::Max(0, Zapas[S] + Ile);
	OdswiezStosy();
}

void ABudowa::Dostarcz(ESurowiec S)
{
	WDrodze[S] = FMath::Max(0, WDrodze[S] - 1);
	++Dostarczone[S];
}

FVector2D ABudowa::ZUkladu(FVector2D L) const
{
	return FVector2D(GetActorTransform().TransformPosition(FVector(L, 0.0)));
}

FVector2D ABudowa::PrzyStosie(ESurowiec S) const
{
	// 1,3 m od stosu w strone srodka budynku (stos lezy obok sciany albo przed nia).
	const FVector2D P = RodzajB.MaStos(S) ? RodzajB.Stosy[int32(S)] : RodzajB.Wejscie;
	const FVector2D Do = (-P).GetSafeNormal();
	return ZUkladu(P + Do * 130.f);
}

void ABudowa::OdswiezStosy()
{
	for (ESurowiec S : WszystkieSurowce)
	{
		UStaticMeshComponent* C = Stosy.IsValidIndex(int32(S)) ? Stosy[int32(S)].Get() : nullptr;
		if (!C)
		{
			continue;
		}
		// Poziom stosu 1..4 wedlug zapelnienia (budynek pracy: pojemnosc wyrobu; pilarnia trzyma do 4 klod).
		const int32 Poj = FMath::Max(1, RodzajB.Wyrob == S || RodzajB.Magazynuje() ? RodzajB.Pojemnosc : 4);
		const int32 N = Zapas[S];
		const int32 Poz = N <= 0 ? 0 : FMath::Clamp(FMath::CeilToInt32(4.f * N / Poj), 1, 4);
		const bool bWidac = Gotowy() && Poz > 0 && ModeleStosow.IsValidIndex(int32(S) * 4 + Poz - 1) && ModeleStosow[int32(S) * 4 + Poz - 1];
		if (bWidac)
		{
			C->SetStaticMesh(ModeleStosow[int32(S) * 4 + Poz - 1]);
		}
		C->SetVisibility(bWidac);
	}
}

void ABudowa::UstawKolorPodgladu(bool bMozna)
{
	if (MaterialPodgladu)
	{
		MaterialPodgladu->SetVectorParameterValue(TEXT("Kolor"), bMozna ? FLinearColor(0.2f, 0.9f, 0.3f) : FLinearColor(1.f, 0.15f, 0.1f));
	}
}

void ABudowa::DodajPrace(float Sekundy)
{
	if (bPodgladB || Gotowy() || bRuina || bZgliszcza)
	{
		return;
	}
	// Postep nie wyprzedza przyniesionego materialu (bez materialu budowa stoi).
	// Balans 21.11.2026: praca budowniczych liczy sie 1,5 raza (budowa trwala za dlugo).
	const float Nowy = PostepBudowy + 1.5f * Sekundy * Tempo / FMath::Max(1.f, RodzajB.CzasBudowy);
	PostepBudowy = FMath::Max(PostepBudowy, FMath::Min3(1.f, Nowy, Material()));
	OdswiezFaze();
	OdswiezObszar(false);
}

FString ABudowa::ModelWariantu(const FRodzajBudynku& R) const
{
	const int32 W = Wariant();
	return (R.Wariantow > 1 && W > 0 && W < R.Wariantow) ? FString::Printf(TEXT("%s_W%d"), *R.Model, W) : R.Model;
}

void ABudowa::OdbijWariant(FRodzajBudynku& R) const
{
	// Tools/budynki.py: LUSTRO = (nie, tak, nie, tak) - warianty 1 i 3 maja drzwi i material po drugiej stronie X.
	const int32 W = Wariant();
	if (R.Wariantow <= 1 || (W != 1 && W != 3))
	{
		return;
	}
	R.Wejscie.X = -R.Wejscie.X;
	R.Sterta.X = -R.Sterta.X;
	R.ZrzutX = FVector2D(-R.ZrzutX.Y, -R.ZrzutX.X);
	const double MinX = R.ObwodMin.X;
	R.ObwodMin.X = -R.ObwodMax.X;
	R.ObwodMax.X = -MinX;
}

void ABudowa::Ulepsz(const FRodzajBudynku& Nowy)
{
	if (bPodgladB || !Gotowy() || Nowy.Fazy < 2)
	{
		return;
	}
	// Ten sam aktor (rodziny mieszkaja dalej, okno budynku zostaje otwarte); wariant modelu bez zmian.
	RodzajPrzed = RodzajB;
	ModelePrzed = Modele;
	RodzajB = Nowy;
	OdbijWariant(RodzajB);
	const FString M = ModelWariantu(Nowy);
	Modele.Reset();
	for (const FString& N : { M + TEXT("_U"), FString::Printf(TEXT("%s_F%d"), *M, Nowy.Fazy - 1), FString::Printf(TEXT("%s_F%d"), *M, Nowy.Fazy) })
	{
		UStaticMesh* S = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Swiat/Budynki/%s.%s"), *N, *N));
		UE_CLOG(!S, LogTemp, Warning, TEXT("Budowa: brak modelu %s (uruchom Tools/Zbuduj_Budynki.bat)"), *N);
		Modele.Add(S);
	}
	bUlepszanie = true;
	PostepBudowy = 0.f;
	FazaTeraz = -1;
	Dostarczone = FSurowce();
	Podloze();
	WczytajZniszczenia();
	PunktyB = -1.f; // przebudowa naprawia tez uszkodzenia
	bUszkodzony = false;
	OdswiezFaze();
	if (bNatychmiast)
	{
		UstawPostep(1.f);
	}
}

FSurowce ABudowa::AnulujUlepszenie()
{
	if (!Przebudowa())
	{
		return FSurowce();
	}
	// Tragarze z towarem w drodze sami go odniosa (ALudzik::Zwolnij), gdy zobacza gotowy budynek.
	const FSurowce Zwrot = Dostarczone;
	RodzajB = RodzajPrzed;
	Modele = ModelePrzed;
	ModelePrzed.Reset();
	bUlepszanie = false;
	PostepBudowy = 1.f;
	FazaTeraz = -1;
	Dostarczone = FSurowce();
	Podloze();
	WczytajZniszczenia();
	OdswiezFaze();
	OdswiezStosy();
	return Zwrot;
}

void ABudowa::UstawPostep(float P)
{
	if (bRuina || bZgliszcza)
	{
		return;
	}
	PostepBudowy = FMath::Clamp(P, 0.f, 1.f);
	// Pokaz (-osada, -kosciol): material przyniesiony co najmniej do postepu.
	for (ESurowiec S : WszystkieSurowce)
	{
		Dostarczone[S] = FMath::Max(Dostarczone[S], FMath::CeilToInt32(RodzajB.Koszt[S] * PostepBudowy));
	}
	OdswiezFaze();
	OdswiezObszar(false);
}

void ABudowa::OdswiezFaze()
{
	const int32 N = Modele.Num();
	if (N == 0 || bRuina || bZgliszcza)
	{
		return; // ruina ma swoj model (Zawal)
	}
	// Fazy budowy (wszystkie poza ostatnia) dziela postep po rowno; ostatnia = gotowy budynek.
	const int32 F = (Gotowy() || N == 1) ? N - 1 : FMath::Min(N - 2, int32(PostepBudowy * (N - 1)));
	if (F != FazaTeraz)
	{
		FazaTeraz = F;
		UStaticMesh* M = (Gotowy() && bUszkodzony && ModelZ1) ? ModelZ1.Get() : Modele[F].Get();
		Siatka->SetStaticMesh(Lan ? nullptr : M); // pole na terenie: bez modelu faz (plot i lan osobno)
		if (Ruchoma)
		{
			Ruchoma->SetVisibility(Gotowy()); // skrzydla / kolo dopiero na gotowym budynku
		}
		for (UStaticMeshComponent* C : Owce)
		{
			if (C)
			{
				C->SetVisibility(Gotowy()); // owce dopiero w gotowej zagrodzie
			}
		}
		if (Gotowy() && Stado.Num())
		{
			RuszOwce(0.f);
		}
		OdswiezStosy();
		if (Gotowy() && Uprawa != EUprawa::Ugor)
		{
			UstawUprawe(Uprawa);
		}
	}
}

void ABudowa::UstawUprawe(EUprawa U)
{
	Uprawa = U;
	if (Lan)
	{
		ZbudujLan(false);
		return;
	}
	if (!Gotowy() || Modele.Num() == 0)
	{
		return;
	}
	const int32 K = int32(U) - 1; // ModeleUprawy: Zaorane..Scierniska
	UStaticMesh* M = (U != EUprawa::Ugor && ModeleUprawy.IsValidIndex(K) && ModeleUprawy[K]) ? ModeleUprawy[K].Get() : Modele.Last().Get();
	Siatka->SetStaticMesh(M);
}

void ABudowa::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickZniszczen(DeltaSeconds);
	if (bNatychmiast && !bPodgladB && !Gotowy())
	{
		UstawPostep(1.f); // tryb deweloperski: budowa od razu gotowa (material liczony jako przyniesiony)
	}
	if (!Gotowy())
	{
		return;
	}
	if (Stado.Num())
	{
		RuszOwce(DeltaSeconds);
	}
	if (!Ruchoma)
	{
		return;
	}
	// Kolo wodne kreci sie zawsze, skrzydla mlyna tylko przy pracy (Napedzaj) - zwalniaja na koniec.
	CzasRuchu = FMath::Max(0.f, CzasRuchu - DeltaSeconds);
	const float Moc = RodzajB.bRuchStaly ? 1.f : FMath::Clamp(CzasRuchu, 0.f, 1.f);
	if (Moc > 0.f)
	{
		Ruchoma->AddLocalRotation(FQuat(RodzajB.RuchomeOs.GetSafeNormal(), DeltaSeconds * RodzajB.RuchomePredkosc * Moc * FMath::Min(Tempo, 3.f)));
	}
}

// --- Pole i pastwisko zaznaczane na terenie (21.11.2026) ---

void ABudowa::OdswiezObszar(bool bWszystko)
{
	if (!RodzajB.ZObszarem() || !Plot || !Paliki)
	{
		return;
	}
	const FRotator Obr(0.f, GetActorRotation().Yaw, 0.f);
	auto NaTerenie = [this](FVector2D L)
	{
		const FVector2D W = ZUkladu(L);
		return FVector(W, FSwiat::Wysokosc(W.X, W.Y));
	};
	// Plot rosnie z budowa: pierwsze odcinki pelne (segment skalowany do odcinka, pochylony jak teren), reszta - same slupki.
	const int32 N = Odcinki.Num();
	const int32 Pelnych = (bPodgladB || Gotowy()) ? N : FMath::Clamp(FMath::FloorToInt32(PostepBudowy * N), 0, N);
	if (bWszystko || Pelnych != PlotuPelnego)
	{
		PlotuPelnego = Pelnych;
		TArray<FTransform> TP, TS;
		for (int32 i = 0; i < N; ++i)
		{
			const FVector A = NaTerenie(Odcinki[i].Key);
			if (i < Pelnych)
			{
				const FVector D = NaTerenie(Odcinki[i].Value) - A;
				TP.Add(FTransform(D.Rotation(), A, FVector(D.Size() / 200.0, 1.0, 1.0)));
			}
			else
			{
				TS.Add(FTransform(Obr, A));
			}
		}
		for (const FVector2D& K : Konce)
		{
			TS.Add(FTransform(Obr, NaTerenie(K)));
		}
		Plot->ClearInstances();
		Paliki->ClearInstances();
		Plot->AddInstances(TP, false, true);
		Paliki->AddInstances(TS, false, true);
	}
	if (Lan && bWszystko)
	{
		ZbudujLan(true);
	}
}

void ABudowa::UstawWzrost(float W)
{
	Wzrost = FMath::Clamp(W, 0.f, 1.f);
	if (Lan && Uprawa == EUprawa::Rosnie)
	{
		ZbudujLan(false); // przebudowa tylko co 10% wzrostu
	}
}

// Lan pola (22.11.2026, zamiast kafli 5 x 5 m, ktore wisialy nad terenem): kazdy wierzcholek na wysokosci terenu,
// rzedy wzdluz X co 50 cm (bruzda / grzbiet), lan podniesiony nad ziemie wedlug uprawy, przy brzegu opada i przechodzi
// w kolor miedzy; brzeg to prostokat z zaokraglonymi rogami poszarpany szumem, wiec rzedy koncza sie nierowno.
void ABudowa::ZbudujLan(bool bSila)
{
	if (!Lan)
	{
		return;
	}
	const int32 Stopien = Uprawa == EUprawa::Rosnie ? FMath::FloorToInt32(Wzrost * 10.f) : 0;
	if (!bSila && Uprawa == LanUprawy && Stopien == LanStopnia)
	{
		return;
	}
	LanUprawy = Uprawa;
	LanStopnia = Stopien;
	Lan->ClearAllMeshSections();
	const FVector2D Min = RodzajB.UprawaMin, Max = RodzajB.UprawaMax, Roz = Max - Min;
	if (Roz.X < 200.0 || Roz.Y < 200.0)
	{
		return;
	}
	// Kolory sRGB jak w Tools/budynki.py, zamienione na liniowe jak przy imporcie modeli (inaczej lan wychodzil blady, kremowy).
	// Material M_Bud_Tynk mnozy je przez teksture. Podst = lan nad ziemia, H = grzbiet rzedu (cm).
	auto K = [](int32 R, int32 G, int32 B, float M = 1.f)
	{
		return FLinearColor(FColor(uint8(FMath::Min(255.f, R * M)), uint8(FMath::Min(255.f, G * M)), uint8(FMath::Min(255.f, B * M))));
	};
	const FLinearColor Miedza = K(116, 108, 74, 0.95f);
	const float G = Stopien / 10.f;
	float Podst = 0.f, H = 4.f;
	FLinearColor Dol = Miedza, Gora = K(110, 118, 70);
	switch (Uprawa)
	{
	case EUprawa::Zaorane: H = 12.f; Dol = K(98, 76, 56, 0.75f); Gora = K(122, 98, 72, 0.9f); break;
	case EUprawa::Zasiane: H = 7.f; Dol = K(98, 76, 56, 0.8f); Gora = K(112, 100, 64); break;
	case EUprawa::Rosnie: Podst = FMath::Lerp(4.f, 35.f, G); H = FMath::Lerp(10.f, 16.f, G); Dol = K(98, 76, 56, 0.85f);
		Gora = FMath::Lerp(K(104, 128, 66, 0.9f), K(150, 146, 76), G * G); Dol = FMath::Lerp(Dol, Gora * 0.7f, G); break;
	case EUprawa::Dojrzale: Podst = 55.f; H = 16.f; Dol = K(168, 138, 80, 0.72f); Gora = K(196, 166, 98); break;
	case EUprawa::Scierniska: H = 6.f; Dol = K(176, 160, 116, 0.78f); Gora = K(176, 160, 116); break;
	default: break; // ugor: zarosniety, ledwo widac rzedy
	}
	// Zboze w 3D (22.11.2026): kepy na grzbietach, lan pod nimi to sama ziemia w rzedach. Bez modeli zostaje podniesiony lan.
	const int32 Model = Uprawa == EUprawa::Dojrzale ? 1 : Uprawa == EUprawa::Scierniska ? 2 : (Uprawa == EUprawa::Rosnie || Uprawa == EUprawa::Zasiane) ? 0 : -1;
	UStaticMesh* MZ = (Zboze && ModeleZboza.IsValidIndex(Model)) ? ModeleZboza[Model].Get() : nullptr;
	const float SkalaZ = Uprawa == EUprawa::Zasiane ? 0.12f : Uprawa == EUprawa::Rosnie ? FMath::Lerp(0.2f, 0.95f, G) : 1.f;
	if (MZ && Uprawa != EUprawa::Scierniska)
	{
		Podst = 0.f;
		H = 9.f;
		Dol = K(98, 76, 56, 0.75f);
		Gora = Uprawa == EUprawa::Dojrzale ? K(140, 116, 72, 0.8f) : K(110, 94, 62, 0.85f);
	}
	TArray<FTransform> TZ;
	const FRotator ObrZ(0.f, GetActorRotation().Yaw, 0.f);
	const FTransform Tr = Lan->GetComponentTransform();
	// Siatka: wzdluz rzedow co ~1 m, w poprzek co 25 cm (parzyste = bruzda, nieparzyste = grzbiet).
	const int32 NX = FMath::CeilToInt32(Roz.X / 100.0), NY = 2 * FMath::Max(1, FMath::RoundToInt32(Roz.Y / 50.0));
	const int32 Kol = NY + 1;
	const FVector2D Krok(Roz.X / NX, Roz.Y / NY), Srodek = (Min + Max) * 0.5, Pol = Roz * 0.5;
	const double Zaokr = FMath::Min(250.0, FMath::Min(Pol.X, Pol.Y) * 0.5);
	const FVector2D Ziarno = FVector2D(GetActorLocation()) * 0.00071; // kazde pole ma inny szum
	const uint32 Ziarno32 = GetTypeHash(FIntPoint(FMath::RoundToInt32(GetActorLocation().X), FMath::RoundToInt32(GetActorLocation().Y)));
	TArray<FVector> V;
	TArray<FLinearColor> C;
	TArray<FVector2D> UV;
	TArray<bool> Jest;
	V.Reserve((NX + 1) * Kol);
	for (int32 i = 0; i <= NX; ++i)
	{
		for (int32 j = 0; j <= NY; ++j)
		{
			const FVector2D L = Min + Krok * FVector2D(i, j);
			const FVector2D Q(FMath::Abs(L.X - Srodek.X) - (Pol.X - Zaokr), FMath::Abs(L.Y - Srodek.Y) - (Pol.Y - Zaokr));
			const double Brzeg = Zaokr - FVector2D(FMath::Max(Q.X, 0.0), FMath::Max(Q.Y, 0.0)).Size() - FMath::Min(FMath::Max(Q.X, Q.Y), 0.0);
			const double W = Brzeg - (45.0 + 45.0 * FMath::PerlinNoise2D(L * 0.006 + Ziarno));
			Jest.Add(W > 0.0);
			const float Skraj = FMath::SmoothStep(0.f, 160.f, float(W));
			uint32 Hs = (uint32(i) * 73856093u) ^ (uint32(j / 2) * 19349663u) ^ Ziarno32;
			Hs ^= Hs >> 13;
			Hs *= 0x5bd1e995u;
			Hs ^= Hs >> 15;
			const float Los = (Hs & 1023u) / 1023.f;
			const float Kepy = 0.85f + 0.15f * (1.f + FMath::PerlinNoise2D(L * 0.012 + Ziarno * 3.0)); // kepy ok. 1 m
			const bool bGrzbiet = (j & 1) == 1;
			const float Z = 6.f + Skraj * Kepy * (Podst + (bGrzbiet ? H * (0.9f + 0.2f * Los) : 0.f));
			const FLinearColor Kk = FMath::Lerp(Miedza, (bGrzbiet ? Gora : Dol) * (0.93f + 0.14f * Los), 0.45f + 0.55f * Skraj);
			const FVector2D Sw = ZUkladu(L);
			V.Add(Tr.InverseTransformPosition(FVector(Sw, FSwiat::Wysokosc(Sw.X, Sw.Y) + Z)));
			C.Add(FLinearColor(Kk.R, Kk.G, Kk.B, 1.f));
			UV.Add(L / 200.0);
			// Kepa na odcinku grzbietu miedzy tym a nastepnym wierzcholkiem; przy brzegu nizsza.
			if (MZ && bGrzbiet && i < NX && W > 40.0)
			{
				const FVector2D Sm = ZUkladu(L + FVector2D(Krok.X * 0.5, 0.0));
				TZ.Add(FTransform(ObrZ, FVector(Sm, FSwiat::Wysokosc(Sm.X, Sm.Y) + 4.0),
					FVector(Krok.X / 100.0, 1.0, SkalaZ * Kepy * (0.55f + 0.45f * Skraj) * (0.9f + 0.2f * Los))));
			}
		}
	}
	TArray<int32> T;
	TArray<FVector> N;
	N.SetNumZeroed(V.Num());
	for (int32 i = 0; i < NX; ++i)
	{
		for (int32 j = 0; j < NY; ++j)
		{
			const int32 A = i * Kol + j, B = A + 1, Cc = A + Kol, D = Cc + 1;
			if (!(Jest[A] && Jest[B] && Jest[Cc] && Jest[D]))
			{
				continue;
			}
			T.Append({ A, B, Cc, B, D, Cc }); // jak ADrogi::Wstega (przod sciany patrzac z gory)
			for (const FIntVector& Tri : { FIntVector(A, B, Cc), FIntVector(B, D, Cc) })
			{
				const FVector Nt = FVector::CrossProduct(V[Tri.Z] - V[Tri.X], V[Tri.Y] - V[Tri.X]);
				N[Tri.X] += Nt;
				N[Tri.Y] += Nt;
				N[Tri.Z] += Nt;
			}
		}
	}
	for (FVector& Nv : N)
	{
		Nv = Nv.IsNearlyZero() ? FVector::UpVector : Nv.GetSafeNormal();
	}
	Lan->CreateMeshSection_LinearColor(0, V, T, N, UV, C, TArray<FProcMeshTangent>(), false);
	if (Zboze)
	{
		Zboze->ClearInstances();
		Zboze->SetStaticMesh(MZ);
		if (MZ)
		{
			Zboze->AddInstances(TZ, false, true);
		}
	}
}

// --- Owczarnia (etap 4) ---

void ABudowa::RuszOwce(float Dt)
{
	// Owca pasie sie chwile w miejscu, potem idzie kilka metrow w losowa strone (w obrebie pastwiska).
	// Zarezerwowana (pasterz idzie ja strzyc) stoi. Runo odrasta w czasie gry (Tempo).
	const FVector2D Pas = RodzajB.Zagroda;
	const float Yaw = GetActorRotation().Yaw;
	for (int32 k = 0; k < Stado.Num(); ++k)
	{
		FOwca& O = Stado[k];
		UStaticMeshComponent* C = Owce.IsValidIndex(k) ? Owce[k].Get() : nullptr;
		if (O.Odrost > 0.f)
		{
			O.Odrost -= Dt * Tempo;
			if (O.Odrost <= 0.f)
			{
				O.Odrost = 0.f;
				if (C)
				{
					C->SetStaticMesh(ModelOwcy);
				}
			}
		}
		if (!O.bRez && Dt > 0.f)
		{
			if (O.Postoj > 0.f)
			{
				O.Postoj -= Dt;
			}
			else
			{
				const FVector2D D = O.Cel - O.Poz;
				const float Dl = float(D.Size());
				const float Odc = 45.f * Dt;
				if (Dl <= Odc)
				{
					O.Poz = O.Cel;
					O.Postoj = FMath::FRandRange(3.f, 10.f);
					const FVector2D N = O.Poz + FVector2D(FMath::FRandRange(-260.f, 260.f), FMath::FRandRange(-200.f, 200.f));
					O.Cel = FVector2D(FMath::Clamp(N.X, -Pas.X, Pas.X), FMath::Clamp(N.Y, 80.0, double(Pas.Y)));
				}
				else
				{
					O.Poz += D / Dl * Odc;
					O.Kierunek = FMath::FixedTurn(O.Kierunek, float(FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X))), 180.f * Dt);
				}
			}
		}
		if (C)
		{
			const FVector2D W = ZUkladu(O.Poz);
			C->SetWorldLocationAndRotation(FVector(W, FSwiat::Wysokosc(W.X, W.Y)), FRotator(0.f, Yaw + O.Kierunek, 0.f));
		}
	}
}

int32 ABudowa::ZarezerwujOwce()
{
	TArray<int32> ZRunem;
	for (int32 k = 0; k < Stado.Num(); ++k)
	{
		if (Stado[k].Odrost <= 0.f && !Stado[k].bRez)
		{
			ZRunem.Add(k);
		}
	}
	if (ZRunem.Num() == 0)
	{
		return -1;
	}
	const int32 K = ZRunem[FMath::RandHelper(ZRunem.Num())];
	Stado[K].bRez = true;
	return K;
}

void ABudowa::ZwolnijOwce(int32 K)
{
	if (Stado.IsValidIndex(K))
	{
		Stado[K].bRez = false;
	}
}

void ABudowa::OstrzyzOwce(int32 K)
{
	if (!Stado.IsValidIndex(K))
	{
		return;
	}
	FOwca& O = Stado[K];
	O.bRez = false;
	O.Odrost = RodzajB.OdrostWelny * FMath::FRandRange(0.85f, 1.15f);
	O.Postoj = FMath::FRandRange(1.f, 3.f);
	if (Owce.IsValidIndex(K) && Owce[K])
	{
		Owce[K]->SetStaticMesh(ModelOwcyStrzyzonej);
	}
}

FVector2D ABudowa::GdzieOwca(int32 K) const
{
	return Stado.IsValidIndex(K) ? ZUkladu(Stado[K].Poz) : FVector2D(GetActorLocation());
}

int32 ABudowa::OwiecZRunem() const
{
	int32 N = 0;
	for (const FOwca& O : Stado)
	{
		N += O.Odrost <= 0.f ? 1 : 0;
	}
	return N;
}

void ABudowa::Podloze()
{
	// Zrzut 22.11.2026: plyty klepiska w modelach byly plaskie i kanciaste, gryzly sie z trawa. Teraz ziemia lezy na terenie,
	// na spadku robi nasyp do poziomu budynku, a brzeg rwie sie w trawe jak na drodze (maska M_Droga).
	const FLinearColor K = RodzajB.Klepisko;
	if (bPodgladB || K.A <= 0.f || RodzajB.ZObszarem())
	{
		if (PodlozeC)
		{
			PodlozeC->ClearAllMeshSections();
		}
		return;
	}
	if (!PodlozeC)
	{
		PodlozeC = NewObject<UProceduralMeshComponent>(this);
		PodlozeC->SetupAttachment(Siatka);
		PodlozeC->SetUsingAbsoluteRotation(true);
		PodlozeC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PodlozeC->SetCastShadow(false);
		PodlozeC->RegisterComponent();
	}
	PodlozeC->SetWorldRotation(FRotator::ZeroRotator);
	const FVector Baza = GetActorLocation();
	const float Rog = 200.f, Skarpa = 400.f;
	const FVector2D Pol(FMath::Max(RodzajB.Polowa.X - 50.f, Rog), FMath::Max(RodzajB.Polowa.Y - 50.f, Rog));
	const FVector2D Rdzen = Pol - FVector2D(Rog);
	const int32 Nx = FMath::CeilToInt32((Pol.X + Skarpa) / 50.f), Ny = FMath::CeilToInt32((Pol.Y + Skarpa) / 50.f); // krok ok. 1 m
	const int32 Wiersz = Ny + 1;
	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> Nrm;
	TArray<FVector2D> UV;
	TArray<FLinearColor> Kol;
	TArray<FProcMeshTangent> Tg;
	for (int32 i = 0; i <= Nx; ++i)
	{
		for (int32 j = 0; j <= Ny; ++j)
		{
			const FVector2D Q(-Pol.X - Skarpa + (Pol.X + Skarpa) * 2.0 * i / Nx, -Pol.Y - Skarpa + (Pol.Y + Skarpa) * 2.0 * j / Ny);
			const FVector2D Poza(FMath::Max(FMath::Abs(Q.X) - Rdzen.X, 0.0), FMath::Max(FMath::Abs(Q.Y) - Rdzen.Y, 0.0));
			const float D = float(Poza.Size()) - Rog; // odleglosc od brzegu zaokraglonego prostokata (< 0 w srodku)
			const FVector2D W = ZUkladu(Q);
			const float H = FSwiat::Wysokosc(W.X, W.Y);
			const float Nasyp = FMath::Max(0.f, float(Baza.Z) - H) * (1.f - FMath::SmoothStep(0.f, Skarpa, D));
			const float Alfa = FMath::Max(FMath::Clamp((150.f - D) / 250.f, 0.f, 1.f), FMath::Clamp(Nasyp / 20.f, 0.f, 1.f));
			V.Add(FVector(W.X - Baza.X, W.Y - Baza.Y, H + Nasyp + 10.f - Baza.Z));
			UV.Add(FVector2D((W.X - Baza.X) / 400.0, (W.Y - Baza.Y) / 400.0));
			Kol.Add(FLinearColor(K.R, K.G, K.B, Alfa));
			Tg.Add(FProcMeshTangent(1.f, 0.f, 0.f));
		}
	}
	for (int32 i = 0; i <= Nx; ++i)
	{
		for (int32 j = 0; j <= Ny; ++j)
		{
			const FVector Dx = V[FMath::Min(i + 1, Nx) * Wiersz + j] - V[FMath::Max(i - 1, 0) * Wiersz + j];
			const FVector Dy = V[i * Wiersz + FMath::Min(j + 1, Ny)] - V[i * Wiersz + FMath::Max(j - 1, 0)];
			FVector N = FVector::CrossProduct(Dx, Dy).GetSafeNormal();
			Nrm.Add(N.Z < 0.f ? -N : N);
			if (i < Nx && j < Ny)
			{
				const int32 A = i * Wiersz + j, B = A + 1, C = A + Wiersz, Dd = C + 1;
				T.Append({ A, B, C, B, Dd, C }); // jak ADrogi::Wstega
			}
		}
	}
	PodlozeC->CreateMeshSection_LinearColor(0, V, T, Nrm, UV, Kol, Tg, false);
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Swiat/Drogi/M_Droga.M_Droga")))
	{
		PodlozeC->SetMaterial(0, M);
	}
}

// --- Zniszczenia (23.11.2026, wojna) ---

bool ABudowa::Zniszczalny() const
{
	return !bPodgladB && RodzajB.Fazy > 1 && !Lan && RodzajB.Funkcja != EFunkcja::Pole;
}

float ABudowa::MaxPunktow() const
{
	// Kamien liczy sie 2,5 raza; budynek darmowy (bez kosztu) - wedlug powierzchni. Plac budowy tym slabszy, im mniej stoi.
	const FSurowce& K = RodzajB.Koszt;
	const float Pole = float(4.0 * RodzajB.Polowa.X * RodzajB.Polowa.Y / 1e4);
	const float Pelne = FMath::Max(40.f + 6.f * K.Suma() + 9.f * K[ESurowiec::Kamien], 1.2f * Pole);
	return Gotowy() ? Pelne : Pelne * FMath::Max(0.25f, PostepBudowy);
}

float ABudowa::Wytrzymalosc() const
{
	if (bRuina || bZgliszcza)
	{
		return 0.f;
	}
	return PunktyB < 0.f ? 1.f : FMath::Clamp(PunktyB / MaxPunktow(), 0.f, 1.f);
}

float ABudowa::Palnosc() const
{
	const FSurowce& K = RodzajB.Koszt;
	const int32 Drewno = K[ESurowiec::Kloda] + K[ESurowiec::Deska];
	const float Razem = Drewno + 1.5f * K[ESurowiec::Kamien]; // kamienne mury tlumia ogien mocniej niz wynika z liczby sztuk
	return Razem <= 0.f ? 0.85f : 0.25f + 0.75f * float(Drewno) / Razem;
}

float ABudowa::Wysokosc() const
{
	const UStaticMesh* M = Modele.Num() ? Modele.Last().Get() : nullptr;
	return M ? FMath::Max(200.f, float(M->GetBoundingBox().Max.Z)) : 500.f;
}

FVector ABudowa::SrodekObrysu() const
{
	return GetActorTransform().TransformPosition(FVector(RodzajB.Obrys().GetCenter(), 0.0));
}

void ABudowa::WczytajZniszczenia()
{
	ModelZ1 = ModelZ2 = nullptr;
	if (bPodgladB || RodzajB.Fazy <= 1 || RodzajB.Funkcja == EFunkcja::Pole)
	{
		return;
	}
	const FString M = ModelWariantu(RodzajB);
	ModelZ1 = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Swiat/Budynki/%s_Z1.%s_Z1"), *M, *M));
	ModelZ2 = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Swiat/Budynki/%s_Z2.%s_Z2"), *M, *M));
	UE_CLOG(!ModelZ1 || !ModelZ2, LogTemp, Warning, TEXT("Budowa: brak modeli zniszczen %s_Z1/_Z2 (uruchom Tools/Zbuduj_Budynki.bat)"), *M);
}

void ABudowa::Uszkodz(float Punkty)
{
	if (!Zniszczalny() || bRuina || bZgliszcza || Punkty <= 0.f)
	{
		return;
	}
	const float Max = MaxPunktow();
	PunktyB = (PunktyB < 0.f ? Max : FMath::Min(PunktyB, Max)) - Punkty;
	if (PunktyB <= 0.f)
	{
		Zawal();
		return;
	}
	if (!bUszkodzony && Gotowy() && PunktyB < 0.7f * Max && ModelZ1)
	{
		// Dziury w dachu, okopcone sciany; klab kurzu z gory, gdy dach sie zapada.
		bUszkodzony = true;
		Siatka->SetStaticMesh(ModelZ1);
		if (AEfekty* E = AEfekty::W(GetWorld()))
		{
			E->Kurz(SrodekObrysu() + FVector(0.f, 0.f, Wysokosc() * 0.7f), float(RodzajB.Obrys().GetExtent().GetMax()), 16);
		}
	}
}

void ABudowa::Podpal(float Sila)
{
	if (!Zniszczalny() || bRuina || bZgliszcza)
	{
		return;
	}
	OgienB = FMath::Clamp(FMath::Max(OgienB, Sila), 0.f, 1.f);
	if (!PozarId)
	{
		if (AEfekty* E = AEfekty::W(GetWorld()))
		{
			PozarId = E->Pozar(SrodekObrysu(), RodzajB.Obrys().GetExtent() * 0.8, float(GetActorRotation().Yaw), Wysokosc());
			E->UstawPozar(PozarId, OgienB);
		}
	}
	SetActorTickEnabled(true);
}

void ABudowa::Gas(float Ile)
{
	OgienB = FMath::Max(0.f, OgienB - Ile);
	if (OgienB <= 0.02f)
	{
		OgienB = 0.f;
		if (AEfekty* E = PozarId ? AEfekty::W(GetWorld()) : nullptr)
		{
			E->ZakonczPozar(PozarId);
		}
		PozarId = 0;
	}
}

void ABudowa::Zawal()
{
	if (bRuina || bZgliszcza)
	{
		return;
	}
	if (Przebudowa())
	{
		AnulujUlepszenie(); // rusztowanie i przyniesiony material przepadaja, wali sie stary budynek
	}
	const FVector S = SrodekObrysu();
	const FVector2D Pol = RodzajB.Obrys().GetExtent();
	const FSurowce& K = RodzajB.Koszt;
	const bool bKamien = K[ESurowiec::Kamien] > K[ESurowiec::Kloda] + K[ESurowiec::Deska];
	const float H = Gotowy() ? Wysokosc() : Wysokosc() * FMath::Max(0.3f, PostepBudowy);
	if (AEfekty* E = AEfekty::W(GetWorld()))
	{
		E->Zawal(S, Pol, float(GetActorRotation().Yaw), H, bKamien);
	}
	UDzwieki::Graj(this, "Zawalenie", S, EDzwiek::Walka, 1.f);
	Zapas = FSurowce(); // towar splonal albo jest pod gruzem
	Zarezerwowane = FSurowce();
	if (!Gotowy() || !ModelZ2)
	{
		// Plac budowy (rusztowanie) spala sie do cna: AOsada go usuwa.
		bZgliszcza = true;
		Siatka->SetVisibility(false, true);
		Gas(1.f);
		return;
	}
	// Kopia stojacego modelu zapada sie w ziemie w chmurze kurzu, a na jego miejscu zostaje ruina.
	Zapadajacy = NewObject<UStaticMeshComponent>(this);
	Zapadajacy->SetupAttachment(Siatka);
	Zapadajacy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Zapadajacy->SetMobility(EComponentMobility::Movable);
	Zapadajacy->SetStaticMesh(Siatka->GetStaticMesh());
	Zapadajacy->RegisterComponent();
	Zapada = 0.f;
	PochylenieZapadania = FRotator(FMath::FRandRange(-7.f, 7.f), FMath::FRandRange(-4.f, 4.f), FMath::FRandRange(-7.f, 7.f));
	// Zgliszcza tla sie jeszcze ok. minuty (ogien nisko nad gruzem, szary dym).
	Podpal(0.5f);
	bRuina = true; // od teraz Gotowy() = false: stosy, owce i czesc ruchoma znikaja
	bUszkodzony = false;
	Tlenie = FMath::FRandRange(45.f, 70.f);
	Siatka->SetStaticMesh(ModelZ2);
	if (Ruchoma)
	{
		Ruchoma->SetVisibility(false);
	}
	for (UStaticMeshComponent* C : Owce)
	{
		if (C)
		{
			C->SetVisibility(false);
		}
	}
	OdswiezStosy();
	SetActorTickEnabled(true);
}

void ABudowa::Odbuduj()
{
	if (!bRuina)
	{
		return;
	}
	Gas(1.f);
	bRuina = false;
	bUszkodzony = false;
	PunktyB = -1.f;
	Tlenie = 0.f;
	PostepBudowy = 0.f;
	FazaTeraz = -1;
	Dostarczone = WDrodze = Zapas = Zarezerwowane = FSurowce();
	if (Zapadajacy)
	{
		Zapadajacy->DestroyComponent();
		Zapadajacy = nullptr;
		Zapada = -1.f;
	}
	OdswiezFaze();
	OdswiezObszar(false);
}

void ABudowa::TickZniszczen(float Dt)
{
	// Zawalenie: najpierw drzy i siada, potem leci w dol coraz szybciej (kurz zaslania przenikanie z ruina).
	if (Zapada >= 0.f && Zapadajacy)
	{
		Zapada += Dt;
		const float T = Zapada / 2.6f;
		if (T >= 1.f)
		{
			Zapadajacy->DestroyComponent();
			Zapadajacy = nullptr;
			Zapada = -1.f;
		}
		else
		{
			const float Spadek = Wysokosc() * 1.1f * T * T;
			const float Drzenie = (1.f - T) * 7.f * FMath::Sin(Zapada * 55.f);
			Zapadajacy->SetRelativeLocationAndRotation(FVector(Drzenie, 0.f, -Spadek), PochylenieZapadania * FMath::Min(1.f, T * 1.6f));
		}
	}
	if (OgienB <= 0.f)
	{
		if (Zapada < 0.f && !Ruchoma && Stado.Num() == 0)
		{
			SetActorTickEnabled(false);
		}
		return;
	}
	// Czas gry (x10 przyspiesza tez pozar), efekty na ekranie w czasie rzeczywistym.
	const float DtG = Dt * Tempo;
	float Cel = Palnosc();
	if (bRuina)
	{
		Tlenie -= DtG;
		Cel = Tlenie > 0.f ? 0.3f : 0.f;
	}
	OgienB = FMath::FInterpConstantTo(OgienB, Cel, DtG, bRuina ? 0.012f : 0.022f);
	if (!bRuina)
	{
		Uszkodz(DtG * OgienB * MaxPunktow() / 80.f); // pelny ogien: drewniany dom wali sie po ok. 1,5 min
	}
	if (OgienB <= 0.01f)
	{
		Gas(1.f);
		return;
	}
	AEfekty* E = AEfekty::W(GetWorld());
	if (E && PozarId)
	{
		E->UstawPozar(PozarId, OgienB, bRuina && ModelZ2 ? FMath::Max(150.f, float(ModelZ2->GetBoundingBox().Max.Z) * 0.6f) : -1.f);
	}
	if (UDzwieki* D = UDzwieki::Z(this))
	{
		const float Odl = float(FVector2D::Distance(FVector2D(D->Ucho()), FVector2D(GetActorLocation())));
		UDzwieki::Petla(this, "Petla_Ogien", EDzwiek::Walka, OgienB * FMath::Clamp(1.f - (Odl - 1500.f) / 9000.f, 0.f, 1.f));
	}
	if (!bRuina && OgienB > 0.3f && FMath::FRand() < Dt * 0.25f)
	{
		UDzwieki::Graj(this, "Krzyk_Panika", SrodekObrysu(), EDzwiek::Glosy, 0.7f); // ludzie krzycza przy plonacym domu
	}
	// Przeskok ognia na sasiednie budynki (do ~9 m od brzegu): tym latwiej, im mocniej plonie i im bardziej palny sasiad.
	DoSkoku -= DtG;
	if (DoSkoku > 0.f)
	{
		return;
	}
	DoSkoku = 2.f;
	if (OgienB < 0.4f)
	{
		return;
	}
	const FVector2D Ja(SrodekObrysu());
	const float MojR = float(RodzajB.Obrys().GetExtent().Size()) * 0.75f;
	for (TActorIterator<ABudowa> It(GetWorld()); It; ++It)
	{
		ABudowa* B = *It;
		if (B == this || !B->Zniszczalny() || B->bRuina || B->bZgliszcza || B->OgienB > 0.f)
		{
			continue;
		}
		const float Odl = float(FVector2D::Distance(Ja, FVector2D(B->SrodekObrysu()))) - MojR - float(B->RodzajB.Obrys().GetExtent().Size()) * 0.75f;
		if (Odl < 900.f && FMath::FRand() < 0.1f * OgienB * B->Palnosc() * (1.f - FMath::Max(0.f, Odl) / 900.f))
		{
			B->Podpal(0.12f);
			UE_LOG(LogTemp, Log, TEXT("Pozar: ogien przeskoczyl z %s na %s"), *RodzajB.Nazwa, *B->RodzajB.Nazwa);
		}
	}
}

void ABudowa::EndPlay(const EEndPlayReason::Type Powod)
{
	if (PozarId)
	{
		if (AEfekty* E = AEfekty::W(GetWorld()))
		{
			E->ZakonczPozar(PozarId);
		}
		PozarId = 0;
	}
	Super::EndPlay(Powod);
}
