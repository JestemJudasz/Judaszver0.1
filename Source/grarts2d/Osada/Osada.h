// Plik: Source/grarts2d/Osada/Osada.h
// Osada gracza na mapie grywalnej: oboz startowy, rodziny osadnikow, budynki i przydzial pracy.
// Prosba uzytkownika (20.11.2026): gracz zaczyna w miejscu, ktore sam wybierze - powstaje tam oboz z 5 namiotow
// i 10 osadnikami (5 rodzin: mezczyzna i kobieta). Namioty to tylko 5 miejsc, wiec trzeba stawiac domy.
// Wolni osadnicy sami ida do najblizszej budowy (budowniczy / tragarz), a gdy dom jest gotowy, wprowadza sie
// do niego rodzina z namiotu i jej namiot znika.
// Ekonomia, etap 1 (drewno, decyzje uzytkownika 21.11.2026: plan "Ekonomia gry" zatwierdzony, rodziny przydziela gracz):
//   oboz startowy ma 20 klod (stos przy ognisku), budowa potrzebuje materialu, ktory przynosza tragarze budowy.
//   Gracz w oknie budynku dodaje / zabiera rodziny w chacie drwala, chacie borowego, pilarni (DodajRodzine / UsunRodzine).
//   Wolni osadnicy: najpierw budowy, potem rozwoza wyroby (klody, deski) na plac skladowy (Rozwoz).
//   Drzewa: drwale rezerwuja drzewo, zanim do niego pojda (dwoch nie idzie do jednego), borowi - miejsce na sadzonke.
// Etap 2 (kamien i jedzenie, 21.11.2026): kamieniarze, zbieracze (krzaki z mapy, odrastaja), mysliwi (las w poblizu).
//   Co CzasPosilku kazda rodzina zjada porcje jedzenia z najblizszego zapasu (spichlerz, oboz, stos przy chacie).
//   Zadowolenie 0-100% (dokument "Ekonomia gry"): jedzenie 50, dom zamiast namiotu 30, kosciol 20. Od 60% co jakis czas
//   przychodzi nowa rodzina (jesli jest wolne miejsce w domu), ponizej 30% rodziny odchodza (zostaja najmniej 2).
// Etap 3 (zboze i chleb, 21.11.2026): pory roku (FKalendarz, 4 min kazda; rok zaczyna sie wiosna w dniu zalozenia obozu).
//   Pola: wiosna orka i siew (PracujNaPolu), lato zboze rosnie, jesien dojrzale do zniw, zima niezebrane gnije.
//   Zima kazda rodzina przy posilku pali klode (cieplo); zadowolenie: jedzenie 40 + cieplo 10 + domy 30 + kosciol 20.
//   Kolor swiata wedlug pory: post process obozu (cieplejszy latem, zloty jesienia, chlodny i szary zima).
// Etap 4 (owce, sukno, ruda, bron, 21.11.2026): owczarnia (pasterz), tkacz, kopalnia rudy, kuznia (ruda + kloda -> wlocznia),
//   zbrojownia: magazyn broni i sukna; w przydzielonej rodzinie wojem zostaje maz (Wojow - licznik u gory).
// Etap 5 (inne rody, handel, 21.11.2026): skarbiec osady w groszach (Grosze, licznik u gory), targ (magazyn kazdego towaru,
//   na nim gracz handluje z rodami - ARody), liczniki do poziomu osady (Domow, Gotowych) i ogloszenia rodow.
// Spawnowana przez ABudowniczy (sterowanie i pasek budowy).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Osada/Surowce.h"
#include "Osada/Kalendarz.h"
#include "Osada.generated.h"

class ABudowa;
class ALudzik;
class AWol;
class ATerenKafle;
class APostProcessVolume;
struct FRodzajBudynku;

UCLASS()
class GRARTS2D_API AOsada : public AActor
{
	GENERATED_BODY()
	friend struct FZapisGry; // Gra/ZapisGry: zapis i wczytanie stanu

public:
	AOsada();
	virtual void Tick(float DeltaSeconds) override;

	/** Oboz: namioty po okregu wokol ogniska, wejsciem do srodka. Out: X, Y = srodek namiotu, Z = obrot (st.). */
	static void MiejscaObozu(FVector2D Srodek, TArray<FVector>& Out);
	static constexpr float PromienObozu = 1050.f; // do karczowania i odstepu od budynkow

	bool MaOboz() const { return bOboz; }
	FVector2D Oboz() const { return SrodekObozu; }

	/** Zaklada oboz startowy (Z: wysokosci podlogi namiotow, w kolejnosci MiejscaObozu, ostatni = ognisko). */
	void ZalozOboz(FVector2D Srodek, const TArray<float>& Z);

	/** Stawia budynek (plac budowy) i karczuje drzewa; prace przydziela sam Tick. */
	ABudowa* Postaw(const FRodzajBudynku& R, FVector2D Gdzie, float Obrot, float Z);

	/** Czy budynek R w punkcie Gdzie nachodzi na inne budynki albo oboz (pole i pastwisko: prostokat obrysu obrocony o Obrot). */
	bool Koliduje(const FRodzajBudynku& R, FVector2D Gdzie, float Obrot = 0.f, const ABudowa* Pomin = nullptr) const;
	/** Poziomy (22.11.2026): czemu tego budynku nie da sie teraz ulepszyc (pusty napis = mozna). */
	FString CzemuNieUlepsz(const ABudowa* B) const;
	/** Rodzaj z bPierwszyDarmowy i w osadzie nie ma jeszcze budynku tej Funkcji (Postaw zdejmie koszt). */
	bool Darmowy(const FRodzajBudynku& R) const;
	/** Przebudowa gotowego domu / kosciola na nastepny poziom: karczuje wiekszy obrys, budowniczy i tragarze przychodza jak na budowe. */
	bool Ulepsz(ABudowa* B);
	/** 23.11.2026: cofa rozpoczeta przebudowe, przyniesiony material wraca do obozu / magazynu. */
	bool AnulujUlepszenie(ABudowa* B);
	/** 23.11.2026: rozbiorka budynku gracza (nie ogniska i namiotu). Zapasy i polowa kosztu (plac budowy: caly przyniesiony
	 *  material) wracaja do ogniska / magazynu; pracownicy sa wolni, mieszkancy bez domu czekaja przy obozie na nastepny dom. */
	bool Rozbierz(ABudowa* B);
	static bool MoznaRozebrac(const ABudowa* B);
	/** Wojna (23.11.2026): zawalony budynek wypada z osady i zostaje ruina (BudynekPod ja znajduje, okno budynku pokazuje
	 *  Odbuduj / Uprzatnij). Odbudowa = plac budowy od zera, caly koszt; uprzatniecie oddaje troche kamienia. */
	bool Odbuduj(ABudowa* Ruina);
	bool Uprzatnij(ABudowa* Ruina);
	int32 Ruin() const;
	/** Ile budynkow sie pali (panel osady, alarm). */
	int32 Plonacych() const;
	/** Budowa stoi bez materialu, nikt go nie niesie i nie ma skad go wziac: budowniczy ida gdzie indziej. */
	bool Zablokowana(const ABudowa* B) const;
	/** Ilu osadnikow ma te budowe za prace (budowniczy + tragarze). */
	int32 Budujacych(const ABudowa* B) const;

	/** Pokaz faz (-kosciol): osadnicy spoza rodzin, od razu przy pracy. */
	void WyslijLudzi(ABudowa* B, bool bNaMiejscu);

	/** Przydziel prace teraz; bNaMiejscu = osadnicy od razu przy budowie (zrzut kontrolny -osada). */
	void PrzydzielTeraz(bool bNaMiejscu);

	/** Poza mapa grywalna budynki i ludzie sa ukryci (mapa papierowa jest plaska). */
	void UstawWidocznosc(bool bWidoczne);
	void UstawTeren(ATerenKafle* T);

	// Liczniki do panelu osady.
	int32 Osadnikow() const;
	int32 Wolnych() const;
	int32 Rodzin() const { return Rodziny.Num(); }
	int32 WNamiotach() const;
	int32 Budow() const;

	// --- Towar ---
	/** Suma zapasow w gotowych budynkach (licznik u gory ekranu). */
	FSurowce Zapasy() const;
	/** Najblizszy gotowy budynek, z ktorego mozna wziac sztuke S (magazyn albo wyrob budynku pracy). */
	ABudowa* ZrodloSurowca(ESurowiec S, FVector2D Blisko, const ABudowa* Pomin) const;
	/** Zaczyna oboz z tyloma klodami (pokaz -osada daje tez deski). */
	static constexpr int32 KlodNaStart = 20;

	// --- Las ---
	bool ZarezerwujDrzewo(FVector2D Srodek, float Promien, FVector2D& Out);
	void ZwolnijDrzewo(FVector2D P);
	void ZetnijDrzewo(FVector2D P);
	/** Wolne miejsce na sadzonke w pierscieniu 6 m .. Promien (rezerwuje je). */
	bool MiejsceNaDrzewo(FVector2D Srodek, float Promien, FVector2D& Out);
	void Zasadz(FVector2D P);
	int32 Sadzonek(FVector2D Srodek, float Promien) const;

	// --- Jedzenie z mapy (etap 2) ---
	/** Najblizszy krzak (nie zerwany, nie zajety) w kole; rezerwuje go. */
	bool ZarezerwujKrzak(FVector2D Srodek, float Promien, FVector2D& Out);
	void ZwolnijKrzak(FVector2D P);
	/** Krzak obrany: odrasta po CzasOdrostu. */
	void ZerwijKrzak(FVector2D P);
	/** Mysliwy: miejsce w lesie (gestym jak sie da) w pierscieniu 12 m .. Promien; Las = ile drzew w 15 m. */
	bool MiejscePolowania(FVector2D Srodek, float Promien, FVector2D& Out, int32& Las) const;
	static constexpr float CzasOdrostu = 180.f;
	static constexpr int32 JedzeniaNaStart = 25;

	// --- Posilki i zadowolenie (etap 2) ---
	/** 0..100. */
	int32 Zadowolenie() const { return FMath::RoundToInt32(ZadowolenieF); }
	/** Do podpowiedzi licznika: skladniki i co dalej (nowa rodzina / odejscia). */
	FString OpisZadowolenia() const;
	/** Rodziny glodne po ostatnim posilku. */
	int32 Glodnych() const;
	/** Wolne miejsca w gotowych domach (dla nowych rodzin). */
	int32 WolnychMiejsc() const;
	/** Ostatnia wiadomosc osady (przyszla / odeszla rodzina, glod), pusta po 12 s. */
	FString Wiadomosc() const;
	static constexpr float CzasPosilku = 150.f; // balans 21.11.2026: bylo 90 s (rodziny jadly za czesto)
	static constexpr float CzasNowejRodziny = 75.f;
	static constexpr float CzasOdejscia = 180.f;

	// --- Paczka A (23.11.2026): ubranie z targu i poziom osady ---
	/** Co tyle posilkow rodzina zuzywa ubranie: bierze bele sukna z targu (tylko z targu, jak w Manor Lords). */
	static constexpr int32 PosilkowNaUbranie = 4;
	/** Rodziny ubrane (ostatnio dostaly sukno z targu albo jeszcze nosza stare). */
	int32 Ubranych() const;
	/** Poziom osady (EPoziomOsady: 0 oboz, 1 wies, 2 osada targowa, 3 miasto); tylko rosnie. */
	uint8 Poziom() const { return PoziomB; }
	/** "Wieś" itd. */
	FString NapisPoziomu() const;
	/** Podpowiedz: warunki nastepnego poziomu, co spelnione, co odblokowuje. */
	FString OpisPoziomu() const;
	/** P7: okno rozwoju osady - pilne braki, co dalej, warunki awansu. */
	FString RozwojOsady() const;
	/** Pusty napis = wolno stawiac R; inaczej "Wymaga: wieś". */
	FString CzemuZablokowany(const FRodzajBudynku& R) const;

	// --- Pory roku i pola (etap 3) ---
	EPora Pora() const { return Kalendarz.Pora(); }
	const FKalendarz& Czas() const { return Kalendarz; }
	/** "Jesień, rok 1". */
	FString NapisPory() const;
	/** Podpowiedz licznika pory: co sie dzieje i ile do nastepnej. */
	FString OpisPory() const;
	/** Pokaz (-osada): ustawia czas gry od zalozenia obozu (bez zmian na polach). */
	void UstawCzas(float Sekundy);
	/** Rolnik orze / sieje (sekundy pracy jednej osoby, razy Tempo); zmienia uprawe pola. */
	void PracujNaPolu(ABudowa* Pole, float Sekundy);
	static constexpr float CzasOrki = 60.f;  // sekundy pracy jednej osoby na cale pole
	static constexpr float CzasSiewu = 50.f;
	/** Orka i siew pola R (pole bazowe 20 x 14 m razy CzynnikObszaru). */
	static float OrkaPola(const FRodzajBudynku& R);
	static float SiewPola(const FRodzajBudynku& R);
	/** Okno pola: sekundy gry do najblizszych zniw (0 = zniwa trwaja) i ile snopow sie spodziewac. */
	float DoZniw(const ABudowa* Pole) const;
	int32 SpodziewanyPlon(const ABudowa* Pole) const;

	// --- Rodziny w budynkach pracy (okno budynku) ---
	/** Dodaje najblizsza rodzine bez pracy; false = brak wolnych rodzin albo miejsc. */
	bool DodajRodzine(ABudowa* B, bool bNaMiejscu = false);
	/** Zapis gry: rodzina o indeksie I pracuje w B (bez szukania najblizszej). */
	void UstawPrace(int32 I, ABudowa* B);
	/** Zabiera ostatnio dodana rodzine (wraca pod dom i znow buduje). */
	bool UsunRodzine(ABudowa* B);
	int32 WolnychRodzin() const;
	int32 RodzinWPracy(const ABudowa* B) const;
	/** Woje (mezowie z rodzin w zbrojowniach). */
	int32 Wojow() const;
	/** Pokaz (-osada): N rodzin wiecej, przy ognisku, bez domu. */
	void RodzinyPokazu(int32 N);
	/** Do okna budynku: nazwy rodzin pracujacych w B i co robia (maz / zona). */
	void PracujacyW(const ABudowa* B, TArray<FString>& Nazwy, TArray<FString>& CoRobia) const;
	/** Rodziny mieszkajace w B (dom, namiot). */
	TArray<FString> MieszkancyW(const ABudowa* B) const;
	/** Budynek pod punktem terenu (klik). */
	ABudowa* BudynekPod(FVector2D P) const;

	// --- Handel i rody (etap 5) ---
	static constexpr int32 GroszyNaStart = 50;
	/** Skarbiec osady (grosze za handel, daniny, podarunki). */
	int32 Grosze = GroszyNaStart;
	/** Podatek (paczka A, 23.11.2026): 0 brak, 1 niski, 2 sredni, 3 wysoki. Co pore roku kazda rodzina w domu placi tyle groszy;
	 *  brak podatku +5 zadowolenia, sredni -6, wysoki -14. */
	int32 Podatek = 1;
	/** Dziesiecina: co pore roku 1/10 jedzenia z magazynow idzie na kosciol; gdy oddana, +5 zadowolenia (wymaga kosciola). */
	bool bDziesiecina = false;
	void ZmienPodatek() { Podatek = (Podatek + 1) % 4; }
	static const TCHAR* NazwaPodatku(int32 P);
	FString OpisPodatkow() const;
	float ZPodatkow() const; // wplyw podatku i dziesieciny na zadowolenie
	/** Pierwszy gotowy targ albo nullptr (bez targu nie ma handlu). */
	ABudowa* Targ() const;
	/** Ile gotowych budynkow rodzaju R (po modelu). */
	int32 Gotowych(const FRodzajBudynku& R) const;
	/** Gotowe domy (bez namiotow) - do poziomu osady. */
	int32 Domow() const;
	/** Ostatnia wiadomosc (panel osady); tez rody (ARody) oglaszaja tu swoje sprawy. */
	void Ogloszenie(const FString& Tekst);
	/** Dorosli mezczyzni osady, ktorzy nie sa w bitwie (pospolite ruszenie, Wojna/Bitwa). Kobiety pracuja dalej. */
	TArray<ALudzik*> Mezczyzni() const;
	/** Tryb deweloperski: Ile sztuk kazdego surowca do obozu (albo pierwszego magazynu) i Ile groszy. */
	void DodajSurowce(int32 Ile);
	/** Napad (Wojna/Bitwa): bandyci zabieraja Czesc kazdego towaru z budynkow i Czesc groszy (Gr). Zwraca, co zabrali. */
	FSurowce Grabiez(float Czesc, int32& Gr);
	/** Towar wraca do obozu albo pierwszego magazynu (odzyskany lup, wlocznie po rozwiazaniu ruszenia). */
	void Zwroc(const FSurowce& Z, int32 Gr = 0);
	void Zwroc(ESurowiec S, int32 Ile);
	/** Bierze do Ile sztuk S z budynkow (tylko nie zarezerwowane); zwraca, ile wzial (ruszenie bierze wlocznie). */
	int32 Wez(ESurowiec S, int32 Ile);

private:
	struct FRodzina
	{
		TWeakObjectPtr<ALudzik> Maz, Zona;
		TWeakObjectPtr<ABudowa> Namiot;
		TWeakObjectPtr<ABudowa> Dom;
		TWeakObjectPtr<ABudowa> Praca; // budynek pracy (drwal, borowy, pilarnia)
		FString Nazwa;                 // "Rodzina Mszczuja"
		bool bNajedzona = true;        // po ostatnim posilku
		bool bCieplo = true;           // zima: przy ostatnim posilku spalila klode
		bool bUbrana = true;           // paczka A: ma ubranie (nowa rodzina przychodzi ubrana)
		int32 DoUbrania = PosilkowNaUbranie; // posilkow do zuzycia ubrania
		int32 Rodzajow = 1;            // paczka A: ile rodzajow jedzenia zjadla przy ostatnim posilku (jedzenie, ryby)
		uint8 Ubranie = 2;             // paczka A krok 3: 0 obdarta, 1 sukno z targu, 2 ubranie od krawca
		bool bObuta = true;            // buty od szewca (z targu), wymieniane jak ubranie
		bool bPiwo = false;            // co drugi posilek baryłka piwa z karczmy
		int32 DoPiwa = 2;
	};
	/** Paczka A: warunki poziomu P (1..3); Out = linie "✓ / ✗ warunek". true = wszystkie spelnione. */
	bool WarunkiPoziomu(uint8 P, TArray<FString>* Out) const;
	void LiczPoziomOsady();
	uint8 PoziomB = 0;

	ALudzik* NowyLudzik(FVector2D Gdzie, bool bKobieta);
	ABudowa* Spawnuj(const FRodzajBudynku& R, FVector2D Gdzie, float Obrot, float Z);
	FVector2D WejscieDo(const ABudowa* B) const;
	void Przydzielaj(bool bNaMiejscu);
	void Wprowadzaj();
	void Rozwoz();
	/** Woly (23.11.2026): dostawia brakujace (2 na start + Wolow gotowych zagrod) i daje wolnym kursy z klodami. */
	void RozwozWolami();
	FVector2D DomRodziny(const FRodzina& F) const;
	bool Zarezerwowane(FVector2D P, float R) const;
	void Posilek();
	void LiczZadowolenie(float Dt);
	void NowaRodzina();
	void OdejdzRodzina();
	float CelZadowolenia(float* Jedz = nullptr, float* Domy = nullptr, float* Wiara = nullptr, float* Cieplo = nullptr, float* Ubranie = nullptr, float* Wygody = nullptr) const;
	ABudowa* Karczma() const;
	void NowaPora(EPora P);
	void KolorPory();
	FString NastepnaNazwa();

	/** Rodziny tracą dom / prace w B, ludzie przy nim przerywaja zajecie (rozbiorka, zawalenie). */
	void OdlaczBudynek(ABudowa* B);
	/** Przeglad: zawalone budynki -> Ruiny, spalone place budowy znikaja. */
	void SprawdzZniszczenia();

	TArray<FRodzina> Rodziny;
	TArray<TWeakObjectPtr<ABudowa>> Budynki;
	TArray<TWeakObjectPtr<ABudowa>> Ruiny;
	TArray<TWeakObjectPtr<ALudzik>> Ludzie;
	TArray<TWeakObjectPtr<AWol>> Woly;
	static constexpr int32 WolowNaStart = 2;
	TWeakObjectPtr<ATerenKafle> Teren;
	TArray<FVector2D> RezerwacjeDrzew; // drzewa, do ktorych ida drwale, i miejsca, gdzie borowi sadza
	TArray<FVector2D> RezerwacjeKrzakow; // krzaki, do ktorych ida zbieracze
	TArray<FVector> Zerwane;            // X, Y = krzak, Z = ile sekund do odrostu
	float DoPosilku = CzasPosilku;
	float DoNowej = CzasNowejRodziny;
	float DoOdejscia = CzasOdejscia;
	float ZadowolenieF = 50.f;
	void PobierzPodatki(); // przy kazdej nowej porze roku
	bool bDziesiecinaOddana = false;
	int32 OstatniPodatek = 0, OstatniaDziesiecina = 0;
	bool bPierwszyPosilek = false;
	int32 NumerNazwy = 0;
	FString OstatniaWiadomosc;
	double CzasWiadomosci = -100.0;
	FVector2D SrodekObozu = FVector2D::ZeroVector;
	bool bOboz = false;
	bool bWidoczne = true;
	float DoPrzegladu = 0.f;
	FKalendarz Kalendarz;
	EPora PoraTeraz = EPora::Wiosna;
	/** Post process obozu (SwiatGameMode): kolor pory roku; wartosci bazowe zapamietane przy pierwszym uzyciu. */
	TWeakObjectPtr<APostProcessVolume> Korekta;
	bool bKorektaSzukana = false;
	FVector4 BazaNasycenia = FVector4(1.f, 1.f, 1.f, 1.f);
	float BazaEkspozycji = 0.f;
};
