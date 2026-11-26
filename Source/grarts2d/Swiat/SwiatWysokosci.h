// Plik: Source/grarts2d/Swiat/SwiatWysokosci.h
// Jedno zrodlo prawdy o ksztalcie swiata: wysokosc terenu w kazdym punkcie (cm, jednostki UE).
// Uzywaja tego kafle terenu, kamera, a pozniej natura, budynki i jednostki.
// Uklad: +X = polnoc (gora ekranu przy Yaw 0), +Y = wschod. Srodek swiata w (0,0).
// Po Wczytaj() wolno czytac z wielu watkow naraz (dane tylko do odczytu).
#pragma once

#include "CoreMinimal.h"

struct GRARTS2D_API FSwiat
{
	/** Bok calego swiata (cm): 1024 kafle po 512 m = 524 km (Europa w skali 1:8). */
	static constexpr double Bok = 52428800.0;
	static constexpr double Polowa = Bok * 0.5;

	/** Przesada pionowa wzgledem skali 1:8 (bez niej gory i skarpy bylyby prawie plaskie). */
	static constexpr double PrzesadaPionowa = 2.5;

	/** Prawdziwe metry wysokosci -> cm w grze. */
	static constexpr double MetryNaCm = 100.0 / 8.0 * PrzesadaPionowa;

	/** Wczytuje Data/europa_wys.r16 (+ europa_wys.txt: "szer wys min_m max_m"). Bez plikow: teren testowy z szumu. */
	static void Wczytaj();

	/** Czy wczytano prawdziwe dane wysokosci Europy. */
	static bool MaDane();

	/** Wysokosc nad poziomem morza (cm). Ponizej 0 = dno morza/jeziora. OdRzekiCm: przy okazji OdRzeki (jedno szukanie zamiast dwoch). */
	static float Wysokosc(double X, double Y, float* OdRzekiCm = nullptr);
	/** Wysokosc tak, jak ja rysuje kafel mapy grywalnej (siatka 4 m, trojkaty jak w TerenKafle), wieksza z niej i z gladkiej.
	 *  Do rzeczy kladzionych na terenie (lan pola, droga, nasyp pod budynkiem), zeby nie chowaly sie pod trojkatami (26.11.2026). */
	static float WysokoscKafla(double X, double Y);

	/**
	 * Mapa grywalna: rzezba wyrazniejsza na calej mapie (rzeczywiste roznice wysokosci x PrzesadaRegionu wzgledem
	 * gladkiej "podlogi" okolicy + lagodne pagorki), bo w skali 1:8 nizina wychodzila plaska jak stol.
	 * Podloga jest ciagla, wiec kafle doczytywane podczas jazdy kamera pasuja bez szwow.
	 * Wolac, gdy nic nie liczy kafli (przy wejsciu/wyjsciu z mapy grywalnej).
	 */
	static void UstawRegion(bool bWlacz);
	static constexpr float PrzesadaRegionu = 3.f;
	/** Mapa grywalna: rzezba wyzsza o D m nad podloga dostaje x PrzesadaRegionu tylko u dolu, wyzej lagodniej:
	 *  3 * Z * ln(1 + D / Z). Przy Z = 1200 m: 100 m -> 288, 500 m -> 1254, 1500 m -> 2919 (bylo 300, 1500, 4500). */
	static constexpr float ZlagodzenieGor = 1200.f;

	/** Wysokosc w prawdziwych metrach (do progow: plaza, skaly, snieg). */
	static float WysokoscMetry(double X, double Y) { return float(Wysokosc(X, Y) / MetryNaCm); }

	/** Punkt startowy kamery (Krakow, gdy sa dane; inaczej srodek swiata). */
	static FVector2D Start();

	/** Mapa grywalna: odleglosc (cm) od brzegu najblizszej rzeki z Data/rzeki.bin, ujemna w wodzie; poza nia 1e5. */
	static float OdRzeki(double X, double Y);

	/** Linie rzek z Data/rzeki.bin (mapa strategiczna): punkty po kolei (cm), polowa szerokosci (cm), 1 = ostatni punkt linii. */
	static void Rzeki(const TArray<FVector2D>*& Punkty, const TArray<float>*& Polowy, const TArray<uint8>*& Koniec);

	/** Rodzaj wody z Data/europa_woda.r8 (piksel 64 m w grze); na mapie grywalnej rzeki z linii. */
	enum class EWoda : uint8 { Lad = 0, Morze = 1, Jezioro = 2, Rzeka = 3 };
	static EWoda Woda(double X, double Y);

	/** Pokrycie terenu z Data/europa_pokrycie.r8 (klasy jak w Tools/pokrycie_europa.py). Granice klas falowane szumem. */
	enum class EPokrycie : uint8 { Woda, Laka, Wrzos, Lisciasty, Iglasty, Mieszany, Rzadki, Mokradlo, Skaly, Snieg, Tundra, Ile };
	static EPokrycie Pokrycie(double X, double Y);
};
