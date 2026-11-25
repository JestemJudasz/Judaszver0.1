// Plik: Source/grarts2d/Osada/Rody.h
// Ekonomia, etap 5 (inne rody, handel, dyplomacja; 21.11.2026, wedlug planu "Ekonomia gry" zatwierdzonego przez uzytkownika).
// Inne rody na mapie Europy, kilkanascie-kilkadziesiat km od osady gracza. Zasada z planu: te same reguly co u gracza -
// rody stawiaja te same budynki (FRodzajBudynku::DoBudowania) za ten sam koszt, a budynki daja te same wyroby z tych samych
// wsadow. Daleko od kamery osada rodu to tylko liczby (rodziny, budynki, zapasy, grosze): co kilka sekund gry dostaje
// produkcje wedlug wydajnosci budynku (WydajnoscNaMinute), je co CzasPosilku, zima pali klody, rosnie albo traci rodziny
// wedlug zadowolenia (te same progi co u gracza). Decyzje: prosta lista priorytetow (jedzenie, drewno, domy, deski, kamien...),
// ktora zmienia cecha rodu: pobozny (wczesniej kosciol), kupiecki (targ, lepsze ceny), wojowniczy (owce, kuznia, zbrojownia, woje).
// Poziomy osady: oboz -> wies -> osada targowa -> miasto (tak samo liczone dla gracza).
// Kontakt z graczem: handel na targu gracza (towar za grosze; ceny wedlug zapasow rodu, odleglosci i stosunkow),
// podarunek (grosze za lepsze stosunki), danina: silniejszy (woje) moze zadac daniny od slabszego - rod zostaje lennikiem
// i placi co rok; wojowniczy silny rod sam zada daniny od gracza (zaplac albo odmow: stosunki spadaja, grozi wojna -
// sama wojna to nastepny etap, po wojsku).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Osada/Surowce.h"
#include "Osada/Kalendarz.h"
#include "Rody.generated.h"

class AOsada;
class AOsadaRodu;
struct FRodzajBudynku;

enum class ECechaRodu : uint8 { Pobozny, Kupiecki, Wojowniczy };
enum class EPoziomOsady : uint8 { Oboz, Wies, OsadaTargowa, Miasto };

/** "wieś", "osada targowa"... */
const TCHAR* NazwaPoziomu(EPoziomOsady P);
/** "pobożny", "kupiecki", "wojowniczy". */
const TCHAR* NazwaCechy(ECechaRodu C);
/** Poziom wedlug liczby rodzin, domow, targu i kosciola (ten sam dla gracza i rodow). */
EPoziomOsady LiczPoziom(int32 Rodzin, int32 Domow, bool bTarg, bool bKosciol);
/** Cena bazowa sztuki w groszach. */
int32 CenaBazowa(ESurowiec S);

struct FRod
{
	FString Nazwa;               // "Toporczycy"
	FLinearColor Barwa = FLinearColor::White;
	ECechaRodu Cecha = ECechaRodu::Kupiecki;
	FVector2D Gdzie = FVector2D::ZeroVector;
	float OdlegloscKm = 0.f;
	int32 Rodzin = 5;
	FSurowce Zapas;
	int32 Grosze = 40;
	TArray<int32> Budynkow;      // ile gotowych, indeks jak w FRodzajBudynku::DoBudowania
	int32 Buduje = -1;           // indeks budowanego budynku albo -1
	float DoKonca = 0.f;         // s do konca budowy
	int32 Woje = 0;
	EPoziomOsady Poziom = EPoziomOsady::Oboz;
	float Zadowolenie = 60.f;
	float Stosunek = 0.f;        // -100 (wrogi) .. 100 (sprzymierzony)
	bool bLennik = false;        // placi graczowi danine co rok
	bool bWrogi = false;         // odmowiono mu daniny: grozi wojna
	int32 ZadaDaniny = 0;        // > 0: zada od gracza tylu groszy
	float CzasZadania = 0.f;     // s do konca czasu na odpowiedz
	TArray<float> PostepBud;     // ulamki pracy kazdego rodzaju budynku (indeks jak Budynkow)
	float DoPosilku = 90.f;
	float DoRodziny = 75.f;
	FString Ostatnio;            // co ostatnio zrobil (okno rodow)
	// Faza AI0 (23.11.2026): dziennik i straznik zastoju.
	FString Dlaczego;            // powod ostatniej decyzji albo czekania (okno rodow, dziennik)
	int32 Wpisow = 0;            // linii w dzienniku AI
	float BezPostepu = 0.f;      // s gry bez nowego budynku, rodziny ani woja
	int32 NajRodzin = 0, NajBudynkow = 0, NajWojow = 0;
	bool bZastoj = false;        // straznik: CzasZastoju bez postepu
	int32 Zastojow = 0;
	// Faza AI3 (23.11.2026): decyzje wladcy raz na pore roku (ARody::Wladca).
	TArray<float> Wobec;         // stosunki z innymi rodami (indeks jak lista rodow), -100..100
	uint32 Decyzje = 0;          // bity EDecyzja, ktore juz kiedys podjal (test "bez zawieszenia")
	int32 Napadow = 0;           // ile razy napadl
	int32 Napadniety = 0;        // ile razy go napadnieto
	int32 Napastnik = -1;        // kto napadl ostatnio (do odbudowy i proby pokoju)
	bool bObrona = false;        // silniejszy wrogi sasiad: zbroi sie do 1/4 rodzin
	int32 GroziGraczowi = 0;     // 1 = zapowiedzial napad na gracza, przyjdzie za pore
	// Mury (23.11.2026, prosba uzytkownika: rody buduja osady z murami): 0 brak, 1 palisada, 2 mur kamienny.
	int32 Mury = 0;
	bool bGlodny = false;         // przy ostatnim posilku zabraklo jedzenia (rozpuszcza wojow do 1/4 rodzin)
	static constexpr int32 BudujeMury = -2; // Buduje: w budowie kolejny pierscien murow
	float Obrona() const { return 1.f + 0.3f * Mury; } // mnoznik sily obroncy za murami

	int32 Sila() const { return Woje * 3 + Rodzin; }
	/** Sila przy obronie osady (za murami). */
	int32 SilaObrony() const { return FMath::RoundToInt32(Sila() * Obrona()); }
	int32 Ile(int32 Indeks) const { return Budynkow.IsValidIndex(Indeks) ? Budynkow[Indeks] : 0; }
	int32 WszystkichBudynkow() const { int32 N = 0; for (int32 B : Budynkow) { N += B; } return N; }
};

UCLASS()
class GRARTS2D_API ARody : public AActor
{
	GENERATED_BODY()

public:
	ARody();
	virtual void Tick(float DeltaSeconds) override;

	/** Rody wokol osady gracza (12-60 km, na ladzie); wolane raz, po zalozeniu obozu. */
	void Zaloz(AOsada* O, FVector2D Gracz);
	bool Zalozone() const { return Rody.Num() > 0; }
	const TArray<FRod>& Lista() const { return Rody; }

	/** Pokaz (-rody): rody zyja juz tyle sekund gry. */
	void Przewin(float Sekundy);

	// --- Handel (na targu gracza) ---
	/** Ile groszy gracz placi rodowi R za sztuke S (0 = rod nie sprzedaje). */
	int32 CenaKupna(int32 R, ESurowiec S) const;
	/** Ile groszy gracz dostaje od rodu R za sztuke S. */
	int32 CenaSprzedazy(int32 R, ESurowiec S) const;
	/** Ile rod R moze sprzedac (zostawia sobie zapas). */
	int32 NaSprzedaz(int32 R, ESurowiec S) const;
	/** false + Powod, gdy nie mozna (brak targu, groszy, towaru, miejsca). */
	bool Kup(int32 R, ESurowiec S, int32 N, FString& Powod);
	bool Sprzedaj(int32 R, ESurowiec S, int32 N, FString& Powod);

	// --- Dyplomacja ---
	static constexpr int32 KosztPodarunku = 20;
	bool Podarunek(int32 R, FString& Powod);
	/** Gracz zada daniny: wystarczy dwa razy wieksza sila (woje x3 + rodziny). */
	bool MoznaZadacDaniny(int32 R) const;
	bool ZadajDaniny(int32 R, FString& Powod);
	/** Odpowiedz na zadanie rodu. */
	bool ZaplacDanine(int32 R, FString& Powod);
	void OdmowDaniny(int32 R);

	/** Sila gracza (woje x3 + rodziny) i jego poziom osady. */
	int32 SilaGracza() const;
	EPoziomOsady PoziomGracza() const;

	/** Ile sztuk na minute daje budynek z jedna rodzina (wedlug jego wyrobu, czasu pracy, owiec, plonu). */
	static float WydajnoscNaMinute(const FRodzajBudynku& B);

	// --- Faza AI0: pomiar (zachowanie AI bez zmian) ---
	/** Dziennik AI: linie [AI] w logu (decyzje z powodem, stan co pore roku) i Saved/AI_dziennik.csv. Wlaczany
	 *  w panelu F3 albo -aidziennik; w testach zawsze. */
	static bool bDziennik;
	/** AI5: poziom trudnosci rodow 0 latwy (wolniej decyduje, ostrozniej napada), 1 sredni, 2 trudny (wiekszy zapas klod
	 *  na zime, wczesniej sie zbroi, smielej napada). Zadnych darmowych surowcow. Panel F3 albo -trudnosc=0..2. */
	static int32 Trudnosc;
	static const TCHAR* NazwaTrudnosci(int32 T) { return T <= 0 ? TEXT("łatwy") : (T == 1 ? TEXT("średni") : TEXT("trudny")); }
	/** Straznik: tyle s gry bez postepu (budynek, rodzina, woj) = ZASTOJ (decyzja: 2 lata gry). */
	static constexpr float CzasZastoju = 8.f * FKalendarz::DlugoscPory;
	/** Testy: 8 rodow od obozu, bez swiata i osady gracza, ze stalym ziarnem losowania. */
	TArray<FRod>& Testowe(int32 Ziarno);
	/** Pora roku rodow: z osady gracza, a bez niej z wlasnego kalendarza (testy). */
	EPora PoraRodow() const;
	const FKalendarz& KalendarzRodow() const { return Kalendarz; }

	// --- Faza AI3: decyzje wladcy ---
	enum class EDecyzja : uint8 { Rozwoj, Handel, Pokoj, Napad, Danina, Obrona, Odbudowa, NapadNaGracza };
	static const TCHAR* NazwaDecyzji(EDecyzja D);
	/** Wszystkie napady miedzy rodami: (napastnik, ofiara) i ile napadow slabszego na silniejszego (ma byc 0). */
	TArray<FIntPoint> Napady;
	/** AI4: osady rodow w terenie (budynki i choragiew); wolane co pore roku i po przewinieciu. */
	void OdswiezWidoki();
	int32 NapadowSlabszych = 0;
	/** Osada rodu R w terenie (nullptr w testach bez swiata). */
	AOsadaRodu* Widok(int32 R) const { return Widoki.IsValidIndex(R) ? Widoki[R].Get() : nullptr; }
	/** Podboj (23.11.2026): skutki wyprawy gracza na rod R. Wybor: -1 odparta, 0 zlupic (polowa zapasow i 60% groszy),
	 *  1 hold (lennik, 30% groszy teraz, danina co rok), 2 spalic (rod traci 1/3 rodzin i 60% zapasow, osada plonie).
	 *  Poleglo = ilu obroncow zginelo. Zwraca ogloszenie. */
	FString PoWyprawie(int32 R, int32 Wybor, int32 Poleglo);

	/** Stosunek slowami: "wrogi", "chlodny", "obojetny", "przyjazny", "sprzymierzony". */
	static const TCHAR* OpisStosunku(float S);

private:
	void KrokRodu(FRod& R, float Dt);
	void Produkuj(FRod& R, float Dt);
	void Jedz(FRod& R);
	void Decyduj(FRod& R);
	void Rok();
	int32 Indeks(const FRodzajBudynku& B) const;
	int32 Domowych(const FRod& R) const;
	void Ogloszenie(const FString& T);
	FRod NowyRod(int32 Wzor) const;
	void Dziennik(FRod& R, const FString& Co);
	void Pilnuj(FRod& R, float Dt);
	void NowaPoraRodow();
	void Wladca(int32 I);
	void Bitwa(int32 A, int32 B);
	void UstawStosunki();
	float BazaStosunku(int32 A, int32 B) const;
	FRandomStream Los;
	FKalendarz Kalendarz;
	EPora OstatniaPora = EPora::Wiosna;
	FString Csv;

	TArray<FRod> Rody;
	UPROPERTY() TArray<TObjectPtr<AOsadaRodu>> Widoki;
	TWeakObjectPtr<AOsada> Osada;
	float DoKroku = 0.f;
	float DoRoku = 0.f;
	static constexpr float Krok = 2.f; // s gry miedzy krokami symulacji
};
