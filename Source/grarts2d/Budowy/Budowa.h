// Plik: Source/grarts2d/Budowy/Budowa.h
// Budynek na mapie grywalnej: plac budowy, ktory rosnie w fazach (modele SM_<Budynek>_F1..Fn z Tools/budynki.py)
// az do gotowego budynku. Postep daja tylko osadnicy (ALudzik -> DodajPrace): bez nich budowa stoi.
// Rodzaje: kosciol (Religia), chata / dom muru pruskiego / duzy dom (Mieszkalne, mieszkaja w nich rodziny),
// namiot i ognisko obozu startowego (bez faz, Fazy = 1: model SM_<Nazwa> bez _F).
// Ekonomia (etap 1, drewno): budowa potrzebuje materialu (Koszt) - tragarze przynosza go ze zrodel (plac skladowy,
// oboz, stos przy chacie drwala / pilarni), a postep budowy nie wyprzedza przyniesionego materialu.
// Gotowy budynek trzyma towar (Zapas) i pokazuje go jako stosy (SM_Stos_<Surowiec>_1..4). Budynki pracy
// (chata drwala, chata borowego, pilarnia) dostaja rodziny przez okno budynku (AOsada::DodajRodzine).
// Etap 2 (kamien i jedzenie, 21.11.2026): kamieniolom (kamieniarze kuja skale), chata zbieracza (jagody i grzyby z krzakow),
// chata mysliwego (zwierzyna w lesie), spichlerz (magazyn jedzenia). Domy i kosciol znow kosztuja kamien.
// Etap 3 (zboze i chleb, 21.11.2026): pole (orka i siew wiosna, zniwa jesienia; wyglad wedlug uprawy), mlyn (wiatrak
// kozlak: zboze -> maka, skrzydla kreca sie przy pracy), piekarnia (maka -> chleb), tartak wodny (tylko nad rzeka:
// kolo wodne, klody -> deski szybciej niz w pilarni). Budynki przerobki (Przetworca) opisuja Wsad / Wyrob / Uzysk.
// Etap 4 (owce, tkacz, kuznia, 21.11.2026): owczarnia (stado owiec chodzi po zagrodzie, pasterz strzyze je na welne),
// chata tkacza (welna -> sukno), kopalnia rudy (gornicy jak kamieniarze), kuznia (ruda + kloda na wegiel -> wlocznia;
// drugi wsad: Wsad2), zbrojownia (magazyn broni i sukna; rodzina tu przydzielona = maz zostaje wojem).
// Pole i pastwisko zaznaczane na terenie (21.11.2026): gracz zaznacza prostokat dwoma rogami (FRodzajBudynku::NaObszar),
// wielkosc zmienia plon, prace i rodziny (pole) albo stado (owczarnia). Plot z segmentow SM_Plot wzdluz obwodu rosnie
// z postepem budowy, pole to lan (ProceduralMesh) lezacy na terenie: rzedy wzdluz X, poszarpane brzegi (22.11.2026).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Osada/Surowce.h"
#include "Budowa.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;

/** Co budynek robi po zbudowaniu. */
enum class EFunkcja : uint8
{
	Brak,
	Mieszkalny, // mieszkaja w nim rodziny (Rodzin)
	Magazyn,    // plac skladowy, oboz: trzyma kazdy surowiec do Pojemnosc sztuk
	Drwal,      // scina drzewa w promieniu Zasieg na klody
	Borowy,     // sadzi mlode drzewa w promieniu Zasieg
	Pilarnia,   // klody -> deski
	Kamieniolom,// kamieniarze kuja kamien z lica skaly (Stanowisko)
	Zbieracz,   // jagody i grzyby z krzakow w promieniu Zasieg -> jedzenie
	Mysliwy,    // poluje w lesie w promieniu Zasieg -> jedzenie
	Pole,       // rolnicy: orka i siew wiosna, zniwa jesienia -> snopy zboza (Plon)
	Przetworca, // mlyn, piekarnia, tartak wodny, tkacz, kuznia: Wsad (+ Wsad2) -> Wyrob (Uzysk sztuk z jednej)
	Owczarnia,  // pasterz strzyze owce ze stada (owce chodza po zagrodzie) -> welna
	Zbrojownia, // magazyn broni i sukna; przydzielona rodzina: maz bierze wlocznie i przeszywanice i cwiczy jako woj
	Rybak,      // paczka A: lowi ryby nad rzeka (FRodzajBudynku::Woda) -> ryby
};

/** Co rosnie na polu (wyglad: model SM_Pole_<Uprawa>; Ugor = gotowe pole bez upraw). */
enum class EUprawa : uint8 { Ugor, Zaorane, Zasiane, Rosnie, Dojrzale, Scierniska, Ile };

/** Rodzaj budynku: modele faz i czas budowy. */
struct FRodzajBudynku
{
	FString Nazwa;          // do napisow, np. "Kościół"
	FString Opis;           // podpowiedz na pasku budowy
	FString Kategoria;      // zakladka paska budowy: "Mieszkalne", "Religia"
	FString Model;          // np. "SM_Kosciol" -> /Game/Swiat/Budynki/SM_Kosciol_F1..
	FString Ikona;          // np. "T_Ikona_Kosciol" (Tools/ikony.py)
	FString Niesione = TEXT("SM_Ludz_Kamien"); // co nosza tragarze (SM_Ludz_Kamien / SM_Ludz_Bal)
	int32 Fazy = 5;         // ostatnia faza = gotowy budynek; 1 = bez budowy (namiot, ognisko)
	int32 Rodzin = 0;       // ile rodzin w nim zamieszka po zbudowaniu
	float CzasBudowy = 240.f;// sekundy pracy jednej osoby (4 osadnikow: ~1 min)
	float Promien = 1300.f; // cm: wolne miejsce wokol srodka (inne budynki, wykarczowane drzewa)
	FVector2D Polowa = FVector2D(1200.f, 800.f); // cm: polowa obrysu w osiach budynku (do sprawdzania terenu)
	/** A > 0: ubita ziemia pod obrysem (ABudowa::Podloze, idzie za terenem i miekko przechodzi w trawe); RGB = kolor ziemi. */
	FLinearColor Klepisko = FLinearColor::Transparent;

	// Praca osadnikow, w ukladzie budynku w Unrealu (cm; Y jest odwrocone wzgledem Tools/budynki.py).
	// Wybieg zwierzat w ukladzie budynku (zagroda wolow, stajnia; 26.11.2026). Pusty = woly pasa sie przy wejsciu.
	FVector2D WybiegMin = FVector2D::ZeroVector;
	FVector2D WybiegMax = FVector2D::ZeroVector;
	FVector2D ObwodMin = FVector2D(-1250.f, -540.f); // prostokat, po ktorym chodza budowniczy (poza rusztowaniem)
	FVector2D ObwodMax = FVector2D(1300.f, 540.f);
	FVector2D Sterta = FVector2D(-200.f, 760.f);    // kamienie dla tragarzy
	FVector2D ZrzutX = FVector2D(-500.f, 500.f);    // gdzie tragarze odkladaja kamien: X od..do
	float ZrzutY = 570.f;                           // i Y (pod murem, przed rusztowaniem)
	int32 Budowniczych = 2, Tragarzy = 2;

	FVector2D Wejscie = FVector2D(0.f, 0.f); // gdzie stoja mieszkancy (przed drzwiami), uklad budynku w UE (cm)

	// Ekonomia.
	EFunkcja Funkcja = EFunkcja::Brak;
	FSurowce Koszt;          // material na budowe (kamien od etapu 2)
	int32 Miejsc = 0;        // ile rodzin moze tu pracowac
	int32 Pojemnosc = 0;     // magazyn: sztuk kazdego surowca; budynek pracy: sztuk wyrobu
	ESurowiec Wyrob = ESurowiec::Ile; // co budynek pracy wydaje innym (Ile = nic)
	ESurowiec Wyrob2 = ESurowiec::Ile; // drugi wyrob (paczka A: mysliwy daje tez surowe skory)
	float Zasieg = 0.f;      // cm: drwal / borowy pracuja w tym promieniu
	uint32 MaskaStosow = 0;  // bit i = stos surowca i przy budynku
	FVector2D Stosy[int32(ESurowiec::Ile)] = { FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector,
		FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector,
		FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector,
		FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector }; // gdzie leza stosy (uklad budynku w UE, cm)
	FVector2D Stanowisko = FVector2D(0.f, 0.f); // pilarnia: gdzie stoi pilarz; kamieniolom: przed licem skaly (X +- 280 cm)

	// Etap 3: przerobka (Przetworca) i pole.
	ESurowiec Wsad = ESurowiec::Ile; // co przerabia (Ile = nic)
	int32 Uzysk = 1;         // ile sztuk Wyrobu z jednej sztuki Wsadu
	float CzasPracy = 8.f;   // sekundy pracy na jedna sztuke Wsadu
	FString Narzedzie;       // model w rece przy pracy (pusty = bez narzedzia)
	FString CoRobi;          // napis w oknie budynku, np. "miele zboże"
	int32 Plon = 0;          // pole: snopy z jednych zniw
	/** Czesc ruchoma (skrzydla wiatraka, kolo wodne): model, os obrotu w ukladzie budynku, punkt osi (cm). */
	FString Ruchome;
	FVector RuchomeGdzie = FVector::ZeroVector;
	FVector RuchomeOs = FVector(0.f, 1.f, 0.f);
	float RuchomePredkosc = 0.6f; // rad/s
	bool bRuchStaly = false; // kolo wodne kreci sie zawsze (woda plynie), skrzydla tylko przy pracy
	/** Tartak wodny: ten punkt (uklad budynku, cm) musi lezec nad rzeka (do 6 m od jej srodka). */
	bool bNadRzeka = false;
	FVector2D Woda = FVector2D::ZeroVector;

	// Etap 4.
	ESurowiec Wsad2 = ESurowiec::Ile; // drugi wsad przerobki (kuznia: kloda na wegiel), po sztuce na jedna sztuke Wsadu
	int32 Owiec = 0;          // owczarnia: owce w stadzie
	int32 Wolow = 0;          // zagroda wolow (23.11.2026): tyle wolow daje osadzie gotowy budynek
	int32 Koni = 0;           // stajnia (23.11.2026): tyle koni z wozami daje osadzie gotowy budynek
	float OdrostWelny = 150.f; // owczarnia: sekundy, po ktorych ostrzyzona owca znow ma runo
	FVector2D Zagroda = FVector2D::ZeroVector; // owczarnia: polowa pastwiska (owce chodza w X -/+ Zagroda.X, Y od 80 do Zagroda.Y: przed szopa)

	// Pole i pastwisko zaznaczane na terenie (21.11.2026). Uklad budynku w UE (cm), +Y = front.
	FVector2D Obszar = FVector2D::ZeroVector;       // szerokosc (X) i glebokosc (Y) zaznaczonego prostokata; 0 = zwykly budynek
	FVector2D ObszarMin = FVector2D::ZeroVector;    // najmniejszy i najwiekszy prostokat
	FVector2D ObszarMax = FVector2D::ZeroVector;
	FVector2D SrodekObszaru = FVector2D::ZeroVector; // srodek prostokata wzgledem budynku (owczarnia: szopa przy tylnym boku)
	FVector2D PlotMin = FVector2D::ZeroVector;      // plot wzdluz tego prostokata
	FVector2D PlotMax = FVector2D::ZeroVector;
	FVector2D Brama = FVector2D::ZeroVector;        // przerwa w przednim boku plotu: X od..do
	FVector2D UprawaMin = FVector2D::ZeroVector;    // pole: kafle uprawy (od frontu miedza ze snopami)
	FVector2D UprawaMax = FVector2D::ZeroVector;
	FVector2D BudynekMin = FVector2D::ZeroVector;   // owczarnia: obrys szopy (teren pod nia jak pod zwyklym budynkiem)
	FVector2D BudynekMax = FVector2D::ZeroVector;
	float CzynnikObszaru = 1.f;                     // powierzchnia / powierzchnia bazowa (pole: praca orki i siewu)
	// Pola i owczarnie dowolnym ksztaltem (26.11.2026, uwaga z gry: "rysowane punktami w dowolny ksztalt jak Manor Lords"):
	// wielokat w ukladzie budynku (cm), przod = bok PrzodWielokata (od punktu i do i+1, rownolegly do X, na +Y, z brama).
	// Obszar / PlotMin / PlotMax to wtedy prostokat opisany (kolizje, teren). Pusty = prostokat jak dotad.
	TArray<FVector2D> Wielokat;
	int32 PrzodWielokata = -1;
	float PoleWielokata = 0.f;   // cm2
	float ObwodWielokata = 0.f;  // cm
	bool ZWielokatem() const { return Wielokat.Num() >= 3; }
	/** Punkt (uklad budynku) w srodku wielokata (bez wielokata: zawsze). */
	bool WWielokacie(FVector2D L) const;
	/** Odleglosc od brzegu wielokata albo prostokata plotu (cm), dodatnia w srodku. */
	float OdBrzegu(FVector2D L) const;
	/** Owczarnia: czy owca moze tu stac (w plocie, nie w szopie). */
	bool NaPastwisku(FVector2D L) const;
	/** Owczarnia: losowy punkt pastwiska (uklad budynku). */
	FVector2D PunktPastwiska() const;
	/** Rodzaj Baza (pole, owczarnia) na wielokat z punktow swiata: uklad budynku z najdluzszego boku (przod z brama),
	 *  owczarnia szukuje miejsca na szope w srodku. Zwraca pusty napis albo powod, czemu nie mozna. */
	static FString NaWielokat(const FRodzajBudynku& Baza, const TArray<FVector2D>& Punkty, FRodzajBudynku& Out, FVector2D& Gdzie, float& ObrotSt);

	// Poziomy (22.11.2026, prosba uzytkownika): dom 1..3 (chata z bali -> dom muru pruskiego -> duzy dom),
	// kosciol 1..3 (drewniany -> kamienny -> wielki z placem). Ulepszenie w oknie budynku (AOsada::Ulepsz):
	// ten sam budynek przechodzi przebudowe (rodzina mieszka dalej), material = Koszt nastepnego poziomu.
	int32 Poziom = 0;          // 0 = budynek bez poziomow
	int32 Wariantow = 1;       // warianty modelu: SM_<Model>_F<n> (wariant 0) i SM_<Model>_W<w>_F<n>; losowany przy stawianiu
	const FRodzajBudynku& (*Ulepszenie)() = nullptr; // nastepny poziom (nullptr = najwyzszy)
	/** Ostatni poziom lancucha Ulepszenie (sam budynek, gdy bez poziomow). Kolizja przy stawianiu (26.11.2026). */
	const FRodzajBudynku& NajwyzszyPoziom() const;
	bool Religia() const { return Kategoria == TEXT("Religia"); }
	// Pierwszy budynek tej Funkcji w osadzie stawiany bez materialu (22.11.2026: pierwsza pilarnia za darmo,
	// zeby na starcie nie utknac z domami bez desek). AOsada::Darmowy / ABudowa::BezKosztu.
	bool bPierwszyDarmowy = false;
	// Paczka A (23.11.2026): poziom osady (EPoziomOsady z Osada/Rody.h: 0 oboz, 1 wies, 2 osada targowa, 3 miasto),
	// od ktorego wolno to stawiac / ulepszyc do tego poziomu (AOsada::Poziom).
	uint8 Wymaga = 0;

	// Umocnienia gracza (26.11.2026): palisada, mur, wieze i bramy. Model bez faz (Fazy = 1) wyrasta z ziemi w czasie budowy
	// (Narasta = wysokosc w cm). bLinia: odcinek 4 m stawiany przeciaganiem linii (model od poczatku odcinka wzdluz +X).
	// Umocnienia nie koliduja ze soba i ludzie ich nie omijaja (shortcut: przechodza przez mur, bramy nie sa potrzebne).
	bool bUmocnienie = false;
	bool bLinia = false;
	float Narasta = 0.f;

	bool ZObszarem() const { return Obszar.X > 0.f; }
	float Metry2() const { return ZWielokatem() ? PoleWielokata / 1e4f : float(Obszar.X * Obszar.Y) / 1e4f; }
	/** "24 x 18 m, 0,04 ha (432 m²)" (okno budynku, panel zaznaczania). */
	FString OpisObszaru() const;
	/** Obrys w ukladzie budynku: zaznaczony prostokat z plotem albo +-Polowa. */
	FBox2D Obrys() const;
	/** Kopia rodzaju dla prostokata W x H cm (przycietego do ObszarMin..ObszarMax); rodzaj bez obszaru - bez zmian. */
	static FRodzajBudynku NaObszar(const FRodzajBudynku& Baza, float W, float H);

	bool MaStos(ESurowiec S) const { return (MaskaStosow >> int32(S)) & 1; }
	/** Magazyn (plac, oboz, spichlerz) albo zbrojownia: trzyma towar z MaskaStosow dla innych. */
	bool Magazynuje() const { return Funkcja == EFunkcja::Magazyn || Funkcja == EFunkcja::Zbrojownia; }

	static const FRodzajBudynku& Kosciol();
	static const FRodzajBudynku& Chata();
	static const FRodzajBudynku& DomPruski();
	static const FRodzajBudynku& DomDuzy();
	static const FRodzajBudynku& Namiot();
	static const FRodzajBudynku& Ognisko();
	static const FRodzajBudynku& PlacSkladowy();
	static const FRodzajBudynku& PlacSkladowy2();   // P3 (26.11.2026): wiekszy sklad po ulepszeniu
	static const FRodzajBudynku& PlacSkladowy3();
	static const FRodzajBudynku& Spichlerz2();
	static const FRodzajBudynku& ChataDrwala();
	static const FRodzajBudynku& ChataBorowego();
	static const FRodzajBudynku& Pilarnia();
	static const FRodzajBudynku& Kamieniolom();
	static const FRodzajBudynku& ChataZbieracza();
	static const FRodzajBudynku& ChataMysliwego();
	static const FRodzajBudynku& Spichlerz();
	static const FRodzajBudynku& Pole();
	static const FRodzajBudynku& Mlyn();
	static const FRodzajBudynku& Piekarnia();
	static const FRodzajBudynku& TartakWodny();
	static const FRodzajBudynku& Owczarnia();
	static const FRodzajBudynku& ChataTkacza();
	static const FRodzajBudynku& KopalniaRudy();
	static const FRodzajBudynku& Kuznia();
	static const FRodzajBudynku& Zbrojownia();
	static const FRodzajBudynku& Targ();      // etap 5: handel z rodami (magazyn kazdego towaru)
	static const FRodzajBudynku& ChataOpalowa(); // paczka A: kloda -> drewno opalowe
	static const FRodzajBudynku& ChataRybaka();  // paczka A: ryby z rzeki
	static const FRodzajBudynku& ZagrodaWolow();  // woly do zwozki klod (2), ulepszenie: obora (4)
	static const FRodzajBudynku& ZagrodaWolow2();
	static const FRodzajBudynku& Stajnia();       // konie z wozami do desek i kamienia
	static const FRodzajBudynku& Garbarnia();     // paczka A krok 3: skory -> skora
	static const FRodzajBudynku& Szewc();         // skora -> buty
	static const FRodzajBudynku& Krawiec();       // sukno -> ubrania
	static const FRodzajBudynku& Slodownia();     // zboze -> slod
	static const FRodzajBudynku& Browar();        // slod -> piwo
	static const FRodzajBudynku& Karczma();       // piwo dla rodzin
	static const FRodzajBudynku& KosciolDrewniany(); // kosciol poziom 1 (Kosciol() = poziom 2)
	static const FRodzajBudynku& KosciolWielki();    // kosciol poziom 3
	static const FRodzajBudynku& Palisada();         // umocnienia (26.11.2026)
	static const FRodzajBudynku& Mur();
	static const FRodzajBudynku& WiezaPalisadowa();
	static const FRodzajBudynku& WiezaMurowana();
	static const FRodzajBudynku& BramaPalisadowa();
	static const FRodzajBudynku& BramaMurowana();
	static const FRodzajBudynku& ChataDrwala2();     // drwal poziom 2 (wiekszy zasieg)
	static const FRodzajBudynku& ChataDrwala3();     // drwal poziom 3 (3 rodziny)
	static const FRodzajBudynku& ChataBorowego2();   // borowy poziom 2 (wiekszy zasieg)
	static const FRodzajBudynku& ChataBorowego3();   // borowy poziom 3 (2 rodziny)
	/** Budynki na pasku budowy, w kolejnosci przyciskow. */
	static const TArray<const FRodzajBudynku*>& DoBudowania();
};

UCLASS()
class GRARTS2D_API ABudowa : public AActor
{
	GENERATED_BODY()
	friend struct FZapisGry;

public:
	ABudowa();

	/** Ustawia rodzaj i wczytuje modele faz. bPodglad = polprzezroczysty gotowy budynek pod kursorem (bez budowy). */
	void Ustaw(const FRodzajBudynku& Rodzaj, bool bPodglad);

	/** Podglad: zielony = mozna stawiac, czerwony = nie. */
	void UstawKolorPodgladu(bool bMozna);

	/** Osadnicy dokladaja prace (sekundy pracy jednej osoby; razy Tempo). */
	void DodajPrace(float Sekundy);

	/** 0..1; 1 = gotowy. */
	float Postep() const { return PostepBudowy; }
	/** Gotowy i stoi (ruina nie jest gotowym budynkiem: nie wydaje towaru, nikt w niej nie mieszka). */
	bool Gotowy() const { return PostepBudowy >= 1.f && !bRuina; }
	bool Podglad() const { return bPodgladB; }
	int32 Faza() const { return FazaTeraz; }
	const FRodzajBudynku& Rodzaj() const { return RodzajB; }

	/** Do pokazu faz (opcja -kosciol): ustawia postep od razu. */
	void UstawPostep(float P);

	/** Poziomy (22.11.2026): przebudowa na nastepny poziom (Nowy = Rodzaj().Ulepszenie()). Modele: SM_<Nowy>[_W<w>]_U
	 *  (stary budynek w rusztowaniu), F(n-1) i gotowy; postep od zera, material od nowa. Wariant zostaje ten sam. */
	void Ulepsz(const FRodzajBudynku& Nowy);
	/** Trwa przebudowa na wyzszy poziom (do napisu w oknie budynku). */
	bool Przebudowa() const { return bUlepszanie && !Gotowy(); }
	/** 23.11.2026: cofa przebudowe (budynek wraca do poprzedniego poziomu, gotowy). Zwraca przyniesiony material do oddania. */
	FSurowce AnulujUlepszenie();
	/** Priorytet budowy 1..5 (3 = zwykly): budowniczy ida najpierw na wyzszy, a z nizszego sa zabierani (AOsada::Przydzielaj). */
	int32 Priorytet = 3;
	float ZOpuszczenie = 0.f; // umocnienia: o ile model jest jeszcze schowany w ziemi (cm)
	/** Pierwszy darmowy (AOsada::Postaw): plac budowy bez materialu, tylko praca budowniczych. */
	void BezKosztu() { RodzajB.Koszt = FSurowce(); }
	/** Wylosowany wariant modelu 0..Wariantow-1 (warianty 1 i 3 sa odbite w X). */
	int32 Wariant() const { return FMath::Max(0, WariantB); }
	/** Pokaz poziomow (-poziomy): wariant dla nastepnego Ustaw zamiast losowania (-1 = losuj). */
	static int32 NastepnyWariant;

	/** Mnoznik tempa budowy (klawisz T w ABudowniczy: x10 do testow). */
	static float Tempo;
	/** Tryb deweloperski (UI/PanelDeweloperski): kazda budowa od razu gotowa. */
	static bool bNatychmiast;

	// --- Towar ---
	/** Lezy w budynku (gotowym). */
	FSurowce Zapas;
	/** Czesc zapasu, po ktora ktos juz idzie. */
	FSurowce Zarezerwowane;
	/** Budowa: material przyniesiony. */
	FSurowce Dostarczone;
	/** Budowa / magazyn: niesione tu teraz (zajete miejsce). */
	FSurowce WDrodze;

	int32 Dostepne(ESurowiec S) const { return Zapas[S] - Zarezerwowane[S]; }
	/** Czy wolno stad brac S innym (magazyn: wszystko; budynek pracy: tylko wyrob). */
	bool Wydaje(ESurowiec S) const;
	/** Magazyn: ile jeszcze sztuk S sie zmiesci. */
	int32 Miejsce(ESurowiec S) const;
	/** Budowa: ile S jeszcze trzeba przyniesc (bez niesionego). */
	int32 Brakuje(ESurowiec S) const;
	/** Budowa: ulamek przyniesionego materialu 0..1 (1 = komplet albo bez kosztu). */
	float Material() const;
	/** Budowa stoi, bo postep doszedl do przyniesionego materialu. */
	bool CzekaNaMaterial() const;
	/** Zmienia zapas i odswieza stosy. */
	void ZmienZapas(ESurowiec S, int32 Ile);
	/** Tragarz odlozyl sztuke S na budowie. */
	void Dostarcz(ESurowiec S);
	/** Swiat (X, Y): punkt w ukladzie budynku. */
	FVector2D ZUkladu(FVector2D Lokalny) const;
	/** Swiat: gdzie stanac, zeby wziac / odlozyc S (przy stosie, od strony budynku). */
	FVector2D PrzyStosie(ESurowiec S) const;

	// --- Pole (etap 3) ---
	EUprawa Uprawa = EUprawa::Ugor;
	/** Czesc pola obsiana wiosna 0..1 (niedokonczony siew = mniejszy plon). */
	float Zasiano = 1.f;
	/** Wiosna: sekundy orki i siewu (polowa = zaorane, calosc = zasiane). */
	float PracaPola = 0.f;
	/** Jesien: snopy jeszcze na pniu. */
	int32 DoZebrania = 0;
	/** Lato: jak wysoko urosło zboze 0..1 (wyglad lanu, okno pola). */
	float Wzrost = 0.f;
	void UstawUprawe(EUprawa U);
	void UstawWzrost(float W);

	/** Przetworca: czesc ruchoma kreci sie jeszcze przez tyle sekund (skrzydla mlyna przy pracy). */
	void Napedzaj(float Sekundy) { CzasRuchu = FMath::Max(CzasRuchu, Sekundy); }

	// --- Owczarnia (etap 4) ---
	/** Owca z runem, ktorej nikt nie strzyze; staje w miejscu. -1 = brak. */
	int32 ZarezerwujOwce();
	void ZwolnijOwce(int32 K);
	/** Ostrzyzona: inny model, runo odrasta przez OdrostWelny sekund. */
	void OstrzyzOwce(int32 K);
	FVector2D GdzieOwca(int32 K) const;
	int32 OwiecZRunem() const;

	/** Pole i pastwisko: ustawia plot (wedlug postepu budowy) i kafle uprawy na terenie. bWszystko = tez kafle (po przesunieciu podgladu). */
	void OdswiezObszar(bool bWszystko);

	// --- Zniszczenia (23.11.2026, prosba uzytkownika: "etapy niszczenia budynkow podczas wojny, ogien, dym") ---
	// Wytrzymalosc z kosztu (kamien liczy sie mocniej). Ponizej 70%: model uszkodzony (SM_<Model>_Z1: dziury w dachu,
	// okopcone sciany). Na zero budynek sie wali: stary model zapada sie w chmurze kurzu, lecacy gruz, zostaje ruina
	// (SM_<Model>_Z2) i tli sie jeszcze chwile. Plac budowy po prostu znika. Ogien rosnie do Palnosc (drewno i strzecha
	// plona mocno, kamien slabo), trawi budynek i przeskakuje na sasiednie. AOsada zauwaza ruine (OdlaczRuine): rodziny
	// i praca przepadaja, ruine mozna odbudowac albo uprzatnac w oknie budynku.
	/** Namiot, ognisko i pole (bez modeli zniszczen) nie plona. */
	bool Zniszczalny() const;
	/** 0..1 (1 = caly). */
	float Wytrzymalosc() const;
	bool Ruina() const { return bRuina; }
	/** Plac budowy spalony do cna: AOsada go usuwa. */
	bool Zgliszcza() const { return bZgliszcza; }
	/** Sila ognia 0..1 (0 = nie pali sie). */
	float Ogien() const { return OgienB; }
	/** Jak mocno plonie: 0,25 (kamien) .. 1 (drewno ze strzecha). */
	float Palnosc() const;
	/** Obrazenia w punktach (pelna wytrzymalosc: 40 + 6 x koszt, kamien x 2,5). */
	void Uszkodz(float Punkty);
	/** Podpala z sila startowa 0..1 (juz plonacy: rozpala mocniej). */
	void Podpal(float Sila = 0.2f);
	/** Gaszenie: ogien slabnie o Ile (0..1). */
	void Gas(float Ile);
	/** Ruina wraca jako plac budowy od zera, ten sam rodzaj i wariant (AOsada::Odbuduj). */
	void Odbuduj();
	/** Wysokosc dachu nad podstawa (cm). */
	float Wysokosc() const;

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Powod) override;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Siatka;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMesh>> Modele;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialPodgladu;

	/** Stosy towaru (po jednym na surowiec z MaskaStosow), modele SM_Stos_<Surowiec>_1..4. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Stosy;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMesh>> ModeleStosow; // [surowiec * 4 + poziom - 1]

	/** Pole: wyglad wedlug uprawy (SM_Pole_<Uprawa>; Ugor = ostatnia faza budowy). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMesh>> ModeleUprawy;

	/** Skrzydla wiatraka / kolo wodne. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Ruchoma;

	/** Owce w zagrodzie (model SM_Owca / SM_Owca_Strzyzona). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Owce;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ModelOwcy;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ModelOwcyStrzyzonej;

	/** Plot pola / pastwiska: pelne segmenty (SM_Plot, skalowane do odcinka) i same slupki (SM_Plot_Palik). */
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Plot;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Paliki;
	/** Pole: lan uprawy, siatka lezaca na terenie (ZbudujLan). */
	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Lan;
	/** Pole: zboze w 3D, kepy SM_Zboze_<Zielone|Dojrzale|Scierniska> na grzbietach rzedow lanu. */
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> Zboze;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMesh>> ModeleZboza;
	/** Ubita ziemia pod budynkiem (FRodzajBudynku::Klepisko): siatka na terenie, material M_Droga (poszarpany brzeg). */
	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> PodlozeC;
	void Podloze();

private:
	/** Pole: siatka lanu od nowa, gdy zmienila sie uprawa albo stopien wzrostu (bSila = zawsze, np. po przesunieciu). */
	void ZbudujLan(bool bSila);
	EUprawa LanUprawy = EUprawa::Ile;
	int32 LanStopnia = -1;

	struct FOwca
	{
		FVector2D Poz = FVector2D::ZeroVector; // uklad budynku (cm)
		FVector2D Cel = FVector2D::ZeroVector;
		float Kierunek = 0.f;  // st., w ukladzie budynku
		float Postoj = 0.f;    // s do nastepnego kroku
		float Odrost = 0.f;    // s do odrosniecia runa (0 = ma runo)
		bool bRez = false;     // pasterz idzie ja strzyc
	};
	TArray<FOwca> Stado;
	void RuszOwce(float Dt);

	void OdswiezFaze();
	void OdswiezStosy();

	/** Plot: odcinki (uklad budynku) po kolei wokol prostokata; Konce = slupki na koncach odcinkow przy bramie. */
	TArray<TPair<FVector2D, FVector2D>> Odcinki;
	TArray<FVector2D> Konce;
	int32 PlotuPelnego = -1;

	FRodzajBudynku RodzajB;
	bool bPodgladB = false;
	float PostepBudowy = 0.f;
	int32 FazaTeraz = -1;
	float CzasRuchu = 0.f;

	/** Poziomy: wariant modelu (-1 = jeszcze nie wylosowany) i przebudowa w toku. */
	int32 WariantB = -1;
	bool bUlepszanie = false;
	/** Do anulowania przebudowy: rodzaj i modele sprzed Ulepsz. */
	FRodzajBudynku RodzajPrzed;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMesh>> ModelePrzed;
	/** Nazwa modelu z wariantem: SM_Chata -> SM_Chata_W2 (wariant 0 bez dopisku). */
	FString ModelWariantu(const FRodzajBudynku& R) const;
	/** Warianty 1 i 3 sa w modelu odbite w X: wejscie, sterta i miejsce zrzutu tez. */
	void OdbijWariant(FRodzajBudynku& R) const;

	// Zniszczenia.
	float MaxPunktow() const;
	float PunktyB = -1.f;          // -1 = pelna wytrzymalosc
	float OgienB = 0.f;
	int32 PozarId = 0;             // AEfekty::Pozar
	float DoSkoku = 2.f;           // s do proby przeskoku ognia na sasiadow
	float Tlenie = 0.f;            // ruina: s tlenia, potem ogien gasnie
	bool bRuina = false;
	bool bZgliszcza = false;
	bool bUszkodzony = false;      // widac model Z1
	float Zapada = -1.f;           // s od zawalenia (animacja), -1 = nie
	FRotator PochylenieZapadania = FRotator::ZeroRotator;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Zapadajacy;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ModelZ1;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ModelZ2;
	void WczytajZniszczenia();
	void Zawal();
	void TickZniszczen(float Dt);
	/** Srodek obrysu w swiecie i polowa obrysu (ogien, kurz). */
	FVector SrodekObrysu() const;
};
