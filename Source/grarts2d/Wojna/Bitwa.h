// Plik: Source/grarts2d/Wojna/Bitwa.h
// Wojna, etap W1 (21.11.2026): bitwa probna blisko kamery, wedlug dokumentu "Wojna, bitwy i oblezenia: projekt".
// ABitwa prowadzi oddzialy prawdziwych ludzikow (ALudzik w trybie walki):
//   - Rozkazy na oddzial, nie na ludzika: stoj, idz (prawy klik w teren), atakuj (prawy klik we wroga albo A),
//     szyk zwarty / luzny (F), odwrot (R). Spacja = pauza (rozkazy mozna wydawac dalej). Lewy klik / 1-4 = wybor oddzialu.
//   - Szyk: 3 szeregi, frontem do celu. Zwarcie jako linia frontu: oddzialy staja czolem w czolo, walczy glownie
//     pierwszy szereg para na pare, tylni wchodza na miejsca poleglych. Z przodu wiekszosc ciosow zatrzymuje tarcza,
//     silniejszy oddzial powoli spycha linie, wrog w boku / plecach lamie ducha kilka razy szybciej.
//   - Morale zamiast punktow zycia oddzialu: spada od strat, flanki i zmeczenia, rosnie z dala od walki. Przy zerze oddzial
//     sie lamie i ucieka; raz moze sie zebrac, drugi raz ucieka z pola. Bitwa konczy sie, gdy jedna strona nie ma juz
//     oddzialu, ktory stoi.
// Nad oddzialami choragwie z nazwa, liczba ludzi i paskiem morale; w lewym dolnym rogu panel twoich oddzialow i rozkazow;
// po bitwie tablica wyniku (polegli, uciekli).
// Bitwa probna: klawisz N w regionie (pod kursorem), opcja -bitwa (zrzut kontrolny: od razu atak).
// W1 to proba "czucia" walki: wojownicy sa tworzeni na czas bitwy, nie z rodzin osady (to etap W3, obrona osady).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Osada/Surowce.h"
#include "Wojna/RodzajeWojsk.h"
#include "Bitwa.generated.h"

class ALudzik;
class AOsada;
class AOsadaRodu;
class ATerenKafle;
class APlayerController;
class SWidget;
class UInstancedStaticMeshComponent;

UENUM()
enum class ERozkaz : uint8 { Stoj, Idz, Atakuj, Odwrot };

struct FWojownik
{
	TWeakObjectPtr<ALudzik> L;
	float Zdrowie = 3.f;
	float Odnowa = 0.f;       // s do nastepnego ciosu
	int32 Doswiadczenie = 0;  // 0 rekrut, 1 wojak, 2 weteran
	int32 Wrog = -1;          // oddzial i woj, z ktorym walczy (-1 = nikt)
	int32 WrogW = -1;
	int32 Slot = 0;           // miejsce w szyku: szereg * Kol + kolumna
	FVector2D Luz = FVector2D::ZeroVector; // przestepowanie: maly odchyl od miejsca w szyku
	bool bZyje = true;
	bool bPoza = false;       // uciekl z pola
	bool bZOsady = false;     // pospolite ruszenie: po bitwie wraca do pracy w osadzie
	float Smierc = 0.f;       // CzasBitwy w chwili smierci (cialo lezy, potem znika poza kadrem)
	int32 Pociski = 0;        // strzaly / belty w kolczanie
	bool bFilm = false;       // filmowa para w zwarciu (blisko srodka ekranu): zywsza wymiana ciosow, kontry, iskry
	bool bObchodzi = false;   // wolny woj walczacego oddzialu poszedl do wolnego wroga (obejscie skrzydla, jak w Total War)
	int32 Podpala = -1;       // dzialka osady rodu, ktora idzie podpalic (palenie zdobytej osady)
};

struct FOddzial
{
	FString Nazwa;
	int32 Strona = 0;         // 0 = gracz, 1 = wrog
	FLinearColor Barwa = FLinearColor::White;
	FLinearColor Tunika = FLinearColor::Gray; // do karty oddzialu
	bool bTopor = false;      // bron: topor (inaczej wlocznia)
	// Rodzaj wojska (etap W2, Wojna/RodzajeWojsk.h): wartosci skopiowane z tabeli przy tworzeniu oddzialu.
	ERodzajWojska Rodzaj = ERodzajWojska::Wlocznicy;
	EIkonaBroni Ikona = EIkonaBroni::Wlocznia;
	float Blok = 0.3f;        // czesc ciosow z przodu, ktora przechodzi przez tarcze
	float Tempo = 1.f;        // mnoznik przerwy miedzy ciosami
	float Przebicie = 0.f;    // ile pancerza wroga sie nie liczy
	float Ciecie = 0.12f;     // szansa odciecia przy zabiciu
	TArray<FWojownik> Ludzie;
	FVector2D Srodek = FVector2D::ZeroVector;
	FVector2D Front = FVector2D(1.f, 0.f);
	FVector2D CelRuchu = FVector2D::ZeroVector;
	ERozkaz Rozkaz = ERozkaz::Stoj;
	int32 Cel = -1;           // oddzial atakowany
	float Morale = 100.f;     // 0..100
	float Zmeczenie = 0.f;    // 0..100
	float Trafienie = 0.42f;  // podstawowa szansa trafienia
	float Pancerz = 0.f;      // odejmowane od szansy trafienia przeciwnika
	float Zdrowie = 3.f;      // ciosy do smierci
	int32 Poczatkowo = 0;
	int32 Polegli = 0;
	int32 Uciekli = 0;
	bool bZwarty = true;
	int32 Kol = 1;            // kolumn w szyku (zweza sie po stratach)
	int32 Szeregi = 3;
	bool bUloz = false;       // ktos polegl: przelozyc szyk
	bool bOkrzyk = false;     // okrzyk przed szarza juz byl
	bool bZlamany = false;
	bool bZebral = false;     // raz juz sie zebral po ucieczce
	bool bPoza = false;       // caly oddzial poza polem bitwy
	bool bWalczy = false;     // ktos z oddzialu w zwarciu
	bool bZderzenie = false;  // pierwsze zderzenie juz bylo
	float Flanka = 0.f;       // 0..1: jak bardzo bity z boku / z tylu (do panelu)
	int32 Milicja = -1;       // oddzial pospolitego ruszenia (indeks w Milicje()), -1 = woje bitwy probnej / wrog
	float Start = 0.f;        // CzasBitwy, gdy oddzial stanal na polu
	bool bKoniec = false;     // po walce: schodzi z pola (bitwa probna, wrog)
	bool bNapad = false;      // bandyci napadu: bez obrony ida plądrowac osade
	bool bLup = false;        // bandyci juz zabrali lup i odchodza
	// Pladrowanie (26.11.2026): w osadzie bez obrony chodza od budynku do budynku, pala i morduja, potem lup i odwrot.
	float Pladruje = -1.f, DoMordu = 0.f, DoPozaru = 2.f;
	FVector2D Skad = FVector2D::ZeroVector; // skad przyszli bandyci (tam odchodza z lupem)
	// Szyk i marsz gracza (22.11.2026, przeciaganie prawym jak w Total War).
	int32 KolGracza = 0;      // szerokosc wybrana przeciagnieciem (0 = sama z liczby ludzi); straty zmniejszaja glebokosc
	FVector2D FrontCelu = FVector2D(1.f, 0.f); // front po dojsciu
	bool bDoFrontu = false;   // po dojsciu obrocic sie do FrontCelu
	TArray<FVector2D> Trasa;  // punkty marszu omijajace budynki, ostatni = CelRuchu (inny ostatni = trasa nieaktualna)
	bool bBiegiem = false;    // podwojny prawy klik: bieg (szybciej, rosnie zmeczenie)
	float TempoGrupy = 1.f;   // kilka oddzialow idzie razem tempem najwolniejszego
	bool bObronca = false;    // obroncy osady rodu (wyprawa): czekaja w murach, poki brama stoi

	int32 Zywych() const;
	int32 NaPolu() const;
	bool Stoi() const { return !bZlamany && !bPoza && NaPolu() > 0; }
	/** Strzelcy, ktorym zostaly strzaly. */
	bool Strzela() const;
	int32 Strzal() const;
};

/** Oddzial pospolitego ruszenia (jak w Manor Lords): ustalony sklad mezczyzn z osady. W domu pracuja; Powolaj = rzucaja
 *  prace, biora wlocznie z zapasow (reszta narzedzia) i staja na polu; Rozwiaz = wracaja do pracy. */
struct FMilicja
{
	FString Nazwa;
	TArray<TWeakObjectPtr<ALudzik>> Ludzie;
	int32 Oddzial = -1;       // indeks w Oddzialy(), gdy powolana
	int32 Wlocznie = 0;       // bron wzieta z zapasow przy powolaniu (oddawana przy rozwiazaniu)
	// Szkolenie (P6, 26.11.2026, uwaga z gry: "caly system szkolenia wojska od nowa w zupelnie innym stylu"):
	// gracz wybiera, kim ma byc oddzial, i wysyla go na cwiczenia; wyszkolenie rosnie, gdy cwicza, i powoli spada bez cwiczen.
	ERodzajWojska Rodzaj = ERodzajWojska::Chlopi; // Chlopi = pospolite ruszenie z narzedziami
	float Wyszkolenie = 0.f;  // 0..100 (bez zbrojowni najwyzej 50)
	bool bCwiczy = false;     // cwiczy co dzien (zjada wiecej, mezczyzni pracuja krocej)
	float DoPosilku = 0.f;    // odliczanie jedzenia za cwiczenia
	bool Powolana() const { return Oddzial >= 0; }
	int32 Ilu() const;
};

UCLASS()
class GRARTS2D_API ABitwa : public AActor
{
	GENERATED_BODY()

public:
	ABitwa();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	void UstawTeren(ATerenKafle* T) { Teren = T; }
	/** Bitwa probna przy punkcie: 2 oddzialy wlocznikow gracza kontra 2 oddzialy bandytow. bPokaz = od razu atak (-bitwa). */
	bool ZacznijProbna(FVector2D Srodek, bool bPokaz);
	/** Napad bandytow na osade (klawisz P): bandyci nadchodza, gracz sam powoluje ruszenie (zakladka Wojsko). Bez obrony
	 *  bandyci plądruja zapasy i odchodza z lupem; rozbici oddaja lup. */
	bool ZacznijNapad();
	/** Na polu sa wrogowie, ktorzy stoja (walka). */
	bool Walka() const { return bWalkaTrwa; }
	/** Budowniczy stawia budynek: bitwa nie bierze myszy ani klawiszy. */
	static bool bMyszZajeta;

	// --- Pospolite ruszenie (okno Wojsko, Wojna/OknoWojska) ---
	const TArray<FMilicja>& Milicje() const { return Mil; }
	/** Zmienia sie przy kazdej zmianie oddzialow ruszenia (okno przebudowuje wiersze). */
	int32 Wersja() const { return WersjaN; }
	/** Nowy oddzial z wolnych mezczyzn (do 30). */
	int32 NowaMilicja();
	void Uzupelnij(int32 m);
	void UsunMilicje(int32 m);
	void Powolaj(int32 m);
	void Rozwiaz(int32 m, bool bOglos = true);
	/** Dorosli mezczyzni osady spoza oddzialow ruszenia. */
	TArray<ALudzik*> WolniMezczyzni();
	int32 MezczyznWOsadzie();
	static constexpr int32 MaxMilicji = 30;
	/** Szkolenie (P6): nastepny rodzaj oddzialu (przycisk w oknie Wojsko), cwiczenia wl./wyl. */
	void ZmienRodzaj(int32 m, int32 Ile = 1);
	void PrzelaczCwiczenia(int32 m);
	/** Ile broni z zapasow bierze jeden woj rodzaju R (wlocznia 1, luk 1, palka 0, topor z tarcza 2, kusza 2, miecz 3). */
	static int32 BroniNaWoja(ERodzajWojska R);
	/** Najwyzsze wyszkolenie: 50 bez zbrojowni, 100 ze zbrojownia. */
	float MaxWyszkolenia();
	/** Wyszkolenie na doswiadczenie woja w bitwie (0 rekrut, 1 wojak, 2 weteran). */
	static int32 Doswiadczenie(float Wysz) { return Wysz >= 70.f ? 2 : (Wysz >= 35.f ? 1 : 0); }
	/** P6: napad armii rodu R na osade gracza - bitwa jak napad bandytow, woje w barwie rodu, od strony jego osady. */
	bool NapadRodu(int32 Rod, int32 Woje, int32 Chlopi, FVector2D Skad);
	int32 RodNapadu() const { return NapadRoduI; }
	/** Do znakow bitwy: osada oblegana wyprawa, brama do rozbicia, budynki do podpalenia (oddzial -> dzialka). */
	AOsadaRodu* OsadaWyprawy() { return CelWyprawy(); }
	int32 BramaWyprawy() const { return BramaW; }
	const TMap<int32, int32>& CelePalenia() const { return Palenie; }
	/** -bitwa: szuka laki w okolicy startu, zaczyna bitwe i ustawia kamere. */
	void PokazBitwy();
	bool Trwa() const { return bTrwa; }
	/** Krew w bitwach (klawisz K w bitwie, zapisywane w ustawieniach): rozpryski, plamy, odcinane glowy i rece. */
	static bool bKrew;

	const TArray<FOddzial>& Oddzialy() const { return Oddz; }
	bool Wybrany(int32 i) const { return Wybrane.Contains(i); }
	bool Pauza() const { return bPauza; }
	bool WRegionie() const;
	/** Zaznaczanie ramka (lewy przycisk przeciagniety): rogi w pikselach okna. */
	bool Ramka(FVector2D& A, FVector2D& B) const { A = KlikStart; B = KlikTeraz; return bRamka; }
	/** Przeciaganie prawym: miejsca wojow w nowym szyku i kropki strzalek frontu (puste, gdy nie ciagnie). */
	const TArray<FVector2D>& PodgladSzyku() const { return Podglad; }
	const TArray<FVector2D>& PodgladStrzalek() const { return PodgladStrz; }
	/** Wrog pod kursorem, gdy sa wybrane oddzialy (prawy klik = atak na niego); -1 = brak. */
	int32 CelPodKursorem() const { return CelKursora; }

	// --- Panel bitwy (Wojna/PanelBitwy) ---
	void Wybierz(int32 i, bool bDodaj);
	void Rozkaz(ERozkaz R);
	void PrzelaczSzyk();
	void PrzelaczPauze() { bPauza = !bPauza; }
	bool SzykWybranychZwarty() const;
	/** Kamera regionu na punkt (podwojny klik w karte, klik w mapke). */
	void PokazKamere(FVector2D Gdzie) const;
	/** "stoi", "idzie", "atakuje", "w zwarciu", "UCIEKA", "poza polem". */
	static const TCHAR* OpisStanu(const FOddzial& O);

	/** Polozenie miejsca k w szyku oddzialu (3 szeregi, frontem do Front). */
	static FVector2D Miejsce(const FOddzial& O, int32 k);

	// --- Podboj osad rodow (23.11.2026, prosba: "mechanika zeby podbic te miasta") ---
	/** Wyprawa powolanego ruszenia na osade rodu R (indeks w ARody): woje staja 60 m przed brama, za murem obroncy
	 *  (woje rodu i chlopi, przy 10+ wojach lucznicy). Rozkaz ataku na obroncow prowadzi pod brame: woje rabia ja
	 *  toporami, az padnie; wtedy obroncy wychodza do walki. Po zwyciestwie gracz wybiera los osady (PoZdobyciu). */
	bool Wyprawa(int32 Rod, FString& Powod);
	/** Wyjscie naprzeciw armii rodu (ARody::Przechwyc, 26.11.2026): powolane ruszenie maszeruje do Miejsca jak wyprawa,
	 *  tam bitwa w polu z armia Armia (ludzie z ARody, duze armie pomniejszone jak przy napadzie). */
	bool Przechwyc(int32 Rod, FVector2D Miejsce, int32 Armia, FString& Powod);
	/** Rod, ktorego armie wasze wojsko idzie przechwycic albo z ktora walczy w polu (-1 = brak). */
	int32 RodPrzechwycenia() const { return PoleRod; }
	// --- Punkt zbiorki ruszenia (26.11.2026, jak w Manor Lords) ---
	/** Nastepny klik w teren (ABudowniczy) ustawia punkt zbiorki. */
	void WybierzZbiorke();
	bool WybieraZbiorke() const { return bWybieraZbiorke; }
	void AnulujZbiorke() { bWybieraZbiorke = false; }
	/** Powolani biegna tu i staja w szyku (takze juz powolani, jesli nie walcza). */
	void UstawZbiorke(FVector2D P);
	bool MaZbiorke() const { return bZbiorka; }
	FVector2D Zbiorka() const { return ZbiorkaP; }
	/** Dokad idzie kolumna wyprawy albo przechwycenia (pod brame / miejsce spotkania). */
	FVector2D CelMarszu() const { return MarszW.Wejscie + MarszW.Na * 6000.f; }
	/** Ilu obroncow wystawi rod (woje + co trzecia rodzina, 6..60). */
	static int32 Obroncow(int32 Woje, int32 Rodzin) { return FMath::Clamp(Woje + Rodzin / 3, 6, 60); }
	int32 RodWyprawy() const { return RodW; }
	/** Osada zdobyta: czeka wybor gracza. */
	bool CzekaWybor() const { return bZdobyta; }
	/** 0 zlupic, 1 hold i danina, 2 spalic (ARody::PoWyprawie); ocalali wracaja do domu. */
	void PoZdobyciu(int32 Wybor);
	/** Wyprawa w drodze (tam albo z powrotem): miejsce kolumny na mapie i postep 0..1 (znak na mapie rodow). */
	bool MarszWyprawy(FVector2D& Gdzie, float& Postep, bool& bPowrot) const;
	/** Sekundy (czasu bitwy) marszu wyprawy na odleglosc Odl cm: krotko, zeby nie nudzilo (10..30 s). */
	static float CzasMarszu(float Odl) { return FMath::Clamp(8.f + Odl / 100000.f, 10.f, 30.f); }

	// --- Dziennik bitwy (24.11.2026, prosba: "logi testowe bardziej dogłębne") ---
	/** Co sekunde bitwy stan kazdego oddzialu w logu ([BITWA]) i w Saved/Logs/Bitwa_dziennik.txt (F3 albo -bitwadziennik). */
	static bool bDziennik;
	/** Miary walki liczone zawsze (co 1 s): ilu wojow bije sie, a ilu stoi bezczynnie tuz przy wrogu. */
	struct FMiary
	{
		int32 Probek = 0;
		double Zywych = 0.0, Walczacych = 0.0, Bezczynnych = 0.0; // sumy z probek
		int32 MaxBezczynnych = 0;
		float Zwarcie = -1.f;     // CzasBitwy pierwszego zwarcia
		float Koniec = -1.f;      // CzasBitwy konca walki
	};
	const FMiary& Miary() const { return MiaryB; }
	float Czas() const { return CzasBitwy; }

private:
	FMiary MiaryB;
	float DziennikZa = 0.f;
	/** Probka miar i (przy bDziennik) wiersze dziennika. */
	void Dziennik();
	static void DoDziennika(const FString& Wiersz);
	int32 RodW = -1;          // rod, na ktory idzie wyprawa
	int32 NapadRoduI = -1;    // P6: rod, ktory napadl na osade gracza (bitwa trwa)
	float SkalaNapadu = 1.f;  // ilu ludzi rodu na jednego na polu (duze armie sa pomniejszone)
	void SzkolenieTick(float Dt);
	/** Napastnicy w osadzie: od budynku do budynku, morduja ludzi w poblizu, podpalaja budynki (Pladruje s). */
	void Pladrowanie(float Dt);
	static constexpr float CzasPladrowania = 30.f;
	int32 Zamordowani = 0;
	bool bOglosilPladrowanie = false;
	int32 BramaW = -1;        // brama do rozbicia (indeks w AOsadaRodu::Bramy)
	bool bZdobyta = false;
	FVector2D Dom = FVector2D::ZeroVector; // oboz gracza (tu wracaja woje wyprawy)
	TWeakObjectPtr<class ARody> RodyW;
	class ARody* Rody();
	class AOsadaRodu* CelWyprawy();
	/** Brama wyprawy jeszcze stoi (obroncy czekaja, mur zatrzymuje). */
	bool BramaStoi();
	/** Woje przy bramie rabia ja (co krok symulacji). */
	void Oblezenie(float Dt);
	/** Oddzial w nowe miejsce od razu (marsz wyprawy bez pokazywania drogi). */
	void Przenies(FOddzial& O, FVector2D Gdzie, FVector2D Front);
	/** Ocaleni wracaja pod oboz, osada rodu przestaje byc oblezona. */
	void KoniecWyprawy(bool bOdRazu = false);
	// Marsz wyprawy (24.11.2026, prosba: "zeby jednostki rzeczywiscie szly na wyprawe, a nie sie teleportowaly, ale zeby to
	// nie trwalo dlugo"): 1 = wymarsz z obozu (widac, jak ida, znikaja za kadrem), 2 = w drodze (znak na mapie rodow),
	// po dojsciu ostatnie 30 m marszem pod brame; 3 = powrot (w drodze, potem wchodza do obozu).
	struct FMarsz
	{
		int32 Etap = 0;
		float Czas = 0.f, Caly = 10.f;
		TArray<int32> Oddzialy;
		FVector2D Wejscie = FVector2D::ZeroVector, Na = FVector2D(1.f, 0.f);
		bool bPole = false; // przechwycenie: bitwa w polu w Wejscie + Na * 6000 zamiast pod brama
	};
	FMarsz MarszW;
	void MarszTick(float Dt);
	/** Wyprawa doszla: oddzialy pod brama, obroncy w murach, kamera. */
	void Dotarli();
	/** Przechwycenie doszlo: oddzialy gracza i armia rodu naprzeciw siebie w polu. */
	void DotarliPole();
	int32 PoleRod = -1, PoleArmia = -1;
	FVector2D ZbiorkaP = FVector2D::ZeroVector;
	bool bZbiorka = false, bWybieraZbiorke = false;
	void UkryjMarsz(bool bUkryj);
	// Palenie (24.11.2026, prosba: "zaznaczyc spal ten budynek", "po wyborze Spalic jednostki biegaja, morduja, pala"):
	// oddzial -> dzialka osady rodu do podpalenia (prawy klik w budynek), -1 = pali, co popadnie (po wyborze Spalic).
	TMap<int32, int32> Palenie;
	float PalenieDo = 0.f;    // CzasBitwy konca palenia zdobytej osady (0 = nie trwa)
	int32 PalenieIle = 0, Spalono = 0;
	void PalenieTick(float Dt);

	/** Oddzial rodzaju R (bron, sila, wyglad z tabeli). Tunika z alfa 0 = kolor z tabeli. */
	int32 DodajOddzial(ERodzajWojska R, const FString& Nazwa, int32 Strona, int32 Ilu, FVector2D Srodek, FVector2D Front,
		int32 Doswiadczenie, FLinearColor Tunika = FLinearColor::Transparent);
	/** Strzelcy (W2): kazdy woj poza zwarciem strzela do celu oddzialu w zasiegu; strzaly leca lukiem (Pociski). */
	void Strzelanie(float Dt);
	void PociskiTick(float Dt);
	/** Cios / strzala trafila: rana, krew; przy zerze zdrowia Zabij. */
	void Rana(FOddzial& EO, FWojownik& E, float Ile, FVector2D Kier, float Ciecie, FVector2D Od, bool bStrzala = false, bool bBelt = false);
	/** Polegl: lezy na polu, krew i odciecia, duch jego oddzialu i sasiadow. */
	void Zabij(FOddzial& EO, FWojownik& E, FVector2D Kier, float Ciecie, FVector2D Od, bool bStrzala = false);
	/** Cios w drodze (22.11.2026): skutek liczony przy WalkaCios, widoczny w chwili uderzenia (ALudzik::DoUderzenia),
	 *  obronca podnosi tarcze tuz przed nim. */
	struct FCios { int32 OA, WA, OE, WE; float Za; bool bTraf, bBlok, bZaslona; };
	TArray<FCios> Ciosy;
	void CiosyTick(float Dt);
	/** Co kilka sekund: do 4 par w zwarciu najblizej srodka ekranu dostaje filmowa walke. */
	void Filmowe();
	float FilmZa = 0.f;
	/** Iskry z tarczy / broni przy zatrzymanym ciosie. */
	void Iskry(FVector Gdzie, FVector2D Kier, int32 Ile);
	void IskryTick(float Dt);
	struct FIskra { FVector P; FVector V; float T; };
	TArray<FIskra> IskryL;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> IskryC;
	/** Krew tryska z kikuta przez chwile po odcieciu. */
	struct FTrysk { TWeakObjectPtr<ALudzik> L; float Zostalo; float Co; };
	TArray<FTrysk> Tryski;
	struct FPocisk
	{
		FVector A, B;             // skad, dokad (miejsce celu w chwili strzalu + rozrzut)
		float Czas = 0.f, T = 1.f, Luk = 0.f;
		int32 Strona = 0, Oddzial = -1;
		float Rana = 1.f, Przebicie = 0.f;
		bool bBelt = false;
		bool bSwist = false;      // dzwiek swistu juz zagral
	};
	TArray<FPocisk> Pociski;
	int32 NastWbita = 0;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> LeceC;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> WbiteC;
	void Symuluj(float Dt);
	void RuchOddzialow(float Dt);
	void Zwarcie(float Dt);
	/** Spychanie linii: silniejszy oddzial powoli przesuwa front. */
	void Nacisk(float Dt);
	/** Zywi dostaja miejsca w szyku od pierwszego szeregu; tylni zajmuja miejsca poleglych. */
	void Uloz(FOddzial& O);
	/** Pospolite ruszenie: oddzial z osadnikow (nie tworzy nowych ludzikow). */
	int32 DodajRuszenie(const FString& Nazwa, const TArray<ALudzik*>& Kto, FVector2D Srodek, FVector2D Front, int32 Wloczni,
		ERodzajWojska Rodzaj = ERodzajWojska::Chlopi, float Wyszkolenie = 0.f);
	AOsada* Osada();
	/** Gdzie staje oddzial ruszenia m: przy obozie, a gdy sa bandyci - od ich strony. */
	FVector2D MiejsceZbiorki(int32 m, FVector2D& Front);
	/** Bandyci doszli do osady bez obrony: zabieraja czesc zapasow. */
	void Grabiez(FOddzial& O);
	/** Przebudowa panelu bitwy (nowy / rozwiazany oddzial). */
	void Odswiez();
	TArray<FMilicja> Mil;
	int32 WersjaN = 0;
	int32 NumerMilicji = 0;
	TWeakObjectPtr<AOsada> Os;
	bool bWalkaTrwa = false;
	bool bWrogByl = false;    // od ostatniego konca walki pojawil sie wrog
	FVector2D CelNapadu = FVector2D::ZeroVector;
	FSurowce Lup;
	int32 LupGroszy = 0;
	bool bLupUciekl = false;
	/** Woj schodzi z pola: z osady wraca do pracy, reszta znika. */
	void Odeslij(FWojownik& W);
	/** Wspolne przed kazda bitwa: aktor na polu, ustawienia krwi, modele kropli i plam. */
	void Przygotuj(FVector2D Srodek);
	/** Wspolne po rozstawieniu oddzialow: bitwa trwa, interfejs. */
	void Rozpocznij();
	/** Zmiany w pierwszym szeregu: ranny cofa sie, jego miejsce zajmuje zdrowszy zza plecow. */
	void Zmiany(FOddzial& O);
	/** Okrzyk calego oddzialu (kazdy z innym opoznieniem). */
	static void Okrzyk(FOddzial& O, float Szansa);
	void MoraleOddzialow(float Dt);
	/** Woje nie wchodza w siebie: rozpychanie par blizszych niz osobista przestrzen (co klatke). */
	void Rozepchnij(float Dt);
	void SprawdzKoniec();
	void Sterowanie(APlayerController* PC);
	void ZrobInterfejs();
	void UsunInterfejs();
	void Sprzatnij();
	int32 NajblizszyWrog(int32 i) const;
	/** Oddzial strony pod punktem terenu; z PC i Ekran najpierw ten, w ktorego woja (postac, nie ziemie) celuje kursor. */
	int32 OddzialPod(FVector2D P, int32 Strona, const APlayerController* PC = nullptr, FVector2D Ekran = FVector2D::ZeroVector) const;
	bool TerenPodKursorem(APlayerController* PC, FVector& Out) const;
	/** Rozkazy dla wojow oddzialu poza zwarciem: do miejsc w szyku. */
	void DoSzyku(FOddzial& O, float Szybkosc);
	FString TekstWyniku() const;
	/** Krew (21.11.2026): krople leca i spadaja, na ziemi zostaja plamy (do konca bitwy). */
	void Krew(FVector Gdzie, FVector2D Kier, int32 Ile, float Sila);
	void Plama(FVector2D Gdzie, float Rozmiar);
	void KrewTick(float Dt);
	struct FKropla { FVector P; FVector V; };
	TArray<FKropla> Krople;
	int32 NastPlama = 0;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> KropleC;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> PlamyC;

	TWeakObjectPtr<ATerenKafle> Teren;
	TArray<FOddzial> Oddz;
	TArray<int32> Wybrane;
	bool bTrwa = false;
	bool bWynik = false;      // tablica wyniku na ekranie
	bool bPauza = false;
	bool bWidac = true;
	int32 Zwyciezca = -1;
	float CzasBitwy = 0.f;
	float Krok = 0.f;         // akumulator kroku symulacji
	float CzasKonca = 0.f;    // CzasBitwy w chwili konca
	/** Po bitwie: woje znikaja, gdy kamera na nich nie patrzy; potem pole jest sprzatane. */
	void PoBitwie();
	float Zwolnienie = 0.f;   // s czasu rzeczywistego zwolnienia po zderzeniu
	FVector2D KlikStart = FVector2D::ZeroVector;
	bool bKlikTrwa = false;
	bool bRamka = false;
	FVector2D KlikTeraz = FVector2D::ZeroVector;
	TSharedPtr<SWidget> Nakladka;

	// Prawy przycisk: klik = idz / atakuj, przeciagniecie = linia nowego szyku, podwojny klik = biegiem.
	struct FUstaw { int32 Oddzial; FVector2D Srodek; FVector2D Front; int32 Kol; };
	/** Wybrane oddzialy na linii A-B: kolejno wzdluz linii, front prostopadle (ciagniecie w prawo = front od kamery).
	 *  bZachowaj = obecne szerokosci (zwykly klik), inaczej szerokosc linii dzielona wedlug liczby ludzi. */
	void RozstawNaLinii(FVector2D A, FVector2D B, bool bZachowaj, TArray<FUstaw>& Out) const;
	void WydajMarsz(FOddzial& O, FVector2D Cel, FVector2D Front, int32 Kol);
	/** Plynny obrot szyku do Chce (skrzydla nie szybciej niz marsz); zwrot o ponad 120 st. = w tyl zwrot: tylny szereg
	 *  staje sie przednim, nikt nie biegnie przez caly oddzial. */
	void Obroc(FOddzial& O, FVector2D Chce, float Dt);
	bool bPrawyTrwa = false;
	bool bLinia = false;
	bool bBiegPrawy = false;
	FVector2D PrawyStart = FVector2D::ZeroVector;
	FVector2D PrawyKoniec = FVector2D::ZeroVector;
	FVector2D PrawyStartEkran = FVector2D::ZeroVector;
	double OstatniPrawy = -10.0;
	int32 CelKursora = -1;
	TArray<FVector2D> Podglad;
	TArray<FVector2D> PodgladStrz;
};
