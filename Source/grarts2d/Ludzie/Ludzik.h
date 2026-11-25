// Plik: Source/grarts2d/Ludzie/Ludzik.h
// Osadnik: postac z czesci (Tools/ludzie.py: tulow, glowa, rece, nogi, mlotek, kamien) ruszanych w kodzie
// (chod, stukanie mlotkiem, noszenie). Bez szkieletu - te same czesci posluza pozniej wojsku (wiele postaci naraz).
// Powiekszony x1,4 (decyzja uzytkownika 20.11.2026), kolor tuniki i wlosow z Custom Primitive Data (M_Ludzie).
// Praca przy budowie: Budowniczy chodzi wokol budynku i stuka mlotkiem, Tragarz nosi kamienie ze sterty pod mur.
// Oba dokladaja prace do ABudowa::DodajPrace - bez nich budowa stoi.
// Osadnicy z rodzin (AOsada): mezczyzna albo kobieta (suknia, chusta); bez pracy krecza sie przy swoim domu / namiocie.
// Ekonomia (etap 1): Tragarz najpierw przynosi material na budowe ze zrodel (plac skladowy, oboz, stosy przy chatach),
// a gdy jest komplet - nosi go ze sterty pod mur. Rodziny w budynkach pracy: Drwal (scina drzewa z mapy -> klody pod
// chata), Borowy (sadzi mlode drzewa), Pilarz (przynosi klody, piluje je na deski). Noszacy: wolny osadnik zanosi
// wyroby (klody, deski) na plac skladowy - jeden kurs, potem znow jest wolny.
// Etap 2 (21.11.2026): Kamieniarz (kuje skale kilofem -> kamien na stos), Zbieracz (obiera krzaki z mapy -> kosz jedzenia),
// Mysliwy (poluje z lukiem w lesie, nie zawsze trafi -> sarna = 3 porcje). Odchodzaca rodzina idzie za mape i znika.
// Etap 3 (21.11.2026): Rolnik (pole wedlug pory roku: wiosna orka motyka i siew, lato pielenie, jesien zniwa sierpem
// i snopy na stos, zima w domu), Rzemieslnik (mlyn, piekarnia, tartak wodny: przynosi Wsad, przerabia go na Wyrob).
// Zima zbieracz nie ma czego zbierac (krzaki bez owocow).
// Etap 4 (21.11.2026): Pasterz (strzyze owce ze stada owczarni nozycami -> welna na stos), gornik w kopalni rudy jak
// kamieniarz (Wyrob budynku), kowal jako rzemieslnik z dwoma wsadami (ruda + kloda), Woj (maz z rodziny w zbrojowni:
// bierze wlocznie i przeszywanice ze zbrojowni, cwiczy pchniecia na placu z manekinami; oddaje je, gdy dostanie inna prace).
// Wojna, etap W1 (21.11.2026): w bitwie ludzikiem steruje ABitwa (Wojna/Bitwa.h): WejdzDoBitwy, WalkaIdz / Stoj / Cios /
// Padnij. Wtedy nie szuka pracy, a po smierci lezy na polu bitwy.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Osada/Surowce.h"
#include "Ludzik.generated.h"

class ABudowa;
class AOsada;
class UStaticMesh;
class UStaticMeshComponent;

UENUM()
enum class ERolaLudzika : uint8 { Budowniczy, Tragarz, Drwal, Borowy, Pilarz, Noszacy, Kamieniarz, Zbieracz, Mysliwy, Rolnik, Rzemieslnik, Pasterz, Woj, Rybak };

UCLASS()
class GRARTS2D_API ALudzik : public AActor
{
	GENERATED_BODY()

public:
	ALudzik();
	virtual void Tick(float DeltaSeconds) override;

	/** Przydziela do budowy albo do budynku pracy (Drwal, Borowy, Pilarz). bNaMiejscu = od razu przy pracy (pokaz),
	 *  inaczej przychodzi z miejsca, gdzie stoi. nullptr = bez pracy (wraca pod dom). */
	void Przydziel(ABudowa* Budowa, ERolaLudzika Rola, bool bNaMiejscu);

	/** Jeden kurs: wez sztuke S z Zrodlo i zanies do Cel (magazyn). Rezerwuje towar i miejsce. */
	void Przewiez(ABudowa* Z, ABudowa* CelB, ESurowiec S);

	void UstawOsade(AOsada* O) { Osada = O; }

	/** Rodzina odchodzi z osady: rzuca prace, idzie do punktu i znika. */
	void Odejdz(FVector2D Gdzie);

	/** Co teraz robi (okno budynku), np. "ścina drzewo". */
	FString CoRobi() const;
	ABudowa* ZrodloKursu() const { return Zrodlo.Get(); }

	/** Kolor tuniki (u kobiety sukni) i wlosow (u kobiety chusty), liniowe. */
	void UstawKolory(FLinearColor Tunika, FLinearColor Wlosy);

	/** Kobieta: suknia i chusta (SM_Ludz_*K). Wolac przed Przydziel. */
	void UstawPlec(bool bKobietaNowa) { bKobieta = bKobietaNowa; }
	bool Kobieta() const { return bKobieta; }

	/** Dom (namiot / dom rodziny): bez pracy osadnik kreci sie w poblizu. */
	void UstawDom(FVector2D Gdzie);

	ABudowa* Praca() const { return Budowa.Get(); }
	ERolaLudzika RolaPracy() const { return Rola; }
	bool Wolny() const { return !Budowa.IsValid() && !bWalka; } // w bitwie osada go nie przydziela
	/** 23.11.2026 (prosba uzytkownika): pracownik, ktory od 20 s nie ma co robic w swoim budynku, moze pomoc przy budowie. */
	bool DoPomocy() const { return BezRoboty >= 20.f && !bWalka && Budowa.IsValid() && Rola != ERolaLudzika::Budowniczy
		&& Rola != ERolaLudzika::Tragarz && Rola != ERolaLudzika::Woj; }
	/** Idzie pomagac na budowie B; po jej skonczeniu (albo zablokowaniu) wraca do swojej pracy. */
	void Pomoz(ABudowa* B, ERolaLudzika R);
	/** Zdjety z budowy: pomocnik wraca do swojej pracy, reszta odpoczywa. */
	void Odwolaj();

	// --- Bitwa (etap W1) ---
	/** Wchodzi do bitwy: rzuca prace, w reku Bron (model czesci, np. SM_Ludz_Wlocznia), kolor tuniki. Dalej steruje nim ABitwa. */
	void WejdzDoBitwy(FLinearColor Tunika, FLinearColor Wlosy, const TCHAR* Bron);
	/** Idz (w bitwie) z predkoscia Szybkosc cm/s; po dojsciu stoi i czeka. */
	void WalkaIdz(FVector2D Gdzie, float Szybkosc);
	void WalkaStoj(FVector2D PatrzNa);
	/** Jeden cios (pchniecie, ok. 0,7 s); w tym czasie Uderza() = true. */
	void WalkaCios(FVector2D PatrzNa);
	/** Strzal z luku / kuszy w strone PatrzNa (ok. 0,8 s, kusza 1,1 s); w tym czasie Uderza() = true. */
	void WalkaStrzal(FVector2D PatrzNa);
	/** Ma tarcze w rece (strzaly z przodu czesto wbijaja sie w tarcze). */
	bool MaTarcze() const;
	/** Polegl od ciosu / strzaly lecacej w kierunku Kier: pada w te strone (prosto, na kolana i na twarz, z obrotem albo
	 *  odrzucony do tylu). bKona = jeszcze kilka sekund rusza sie na ziemi. */
	void WalkaPadnij(FVector2D Kier, bool bStrzala, bool bKona);
	/** Trafiony, ale zyje: zachwianie od ciosu z kierunku Kier; strzala: zgina sie, lapie za rane, czasem przyklęka. */
	void WalkaTrafiony(FVector2D Kier, bool bStrzala);
	/** Cios nadchodzi i zostanie zatrzymany: tarcza (albo bron) w gore na chwile. */
	void WalkaBlok() { Zaslona = 1.f; }
	/** Cios uderzyl w tarcze / bron: drgniecie reki (obroncy i napastnika). */
	void WalkaWstrzas() { Wstrzas = 1.f; }
	/** Czas od WalkaCios do chwili uderzenia (s): wtedy ABitwa liczy skutek ciosu. */
	static constexpr float DoUderzenia = 0.26f;
	/** Strzala / belt zostaje w ciele albo w tarczy (najwyzej 5 na woja). */
	void WbijStrzale(FVector2D Lot, bool bBelt, bool bWTarcze);
	/** Rany 0..1 (z utraconego zdrowia): kuleje, garbi sie, krew na tunice. */
	void UstawRane(float R, bool bKrewNaTunice);
	/** Odciete miejsce (szyja / bark) po Odetnij, do tryskania krwi; false = nic nie odciete. */
	bool Kikut(FVector& Gdzie) const;
	/** Filmowa para w zwarciu blisko srodka ekranu: pelna animacja co klatke. */
	void UstawFilm(bool b) { bFilm = b; }
	/** Tylny szereg w walce: stoi tuz za plecami woja przed soba i napiera na niego tarcza (ABitwa::DoSzyku). */
	void WalkaNapor(bool b) { bNapor = b; }
	/** Okrzyk: bron w gore, podskok (szarza, wrog ucieka, doping z tylnych szeregow). Opoznienie w s, zeby nie wszyscy naraz. */
	void WalkaOkrzyk(float Opoznienie) { OkrzykZa = FMath::Max(0.01f, Opoznienie); }
	/** Wyglad woja w bitwie (21.11.2026): Helm 0 brak, 1 szpiczasty helm, 2 futrzana czapka, 3 kaptur; broda; tarcza w barwie
	 *  rodu na lewym przedramieniu; bChoragiew = niesie choragiew oddzialu w lewej dloni (wtedy bez tarczy). */
	void UbierzDoBitwy(int32 Helm, bool bBroda, bool bTarcza, bool bChoragiew, FLinearColor Barwa);
	/** Polegly: odciecie glowy (0), lewej (1) albo prawej reki (2). Czesc odlatuje od ciosu z Od i zostaje na ziemi.
	 *  Zwraca miejsce ciecia (krew). */
	FVector Odetnij(int32 Co, FVector2D Od);
	bool NiesieChoragiew() const { return ChoragiewC != nullptr; }
	/** Pospolite ruszenie: osadnik rzuca prace i bierze do reki narzedzie swojej pracy (siekiera, kilof, motyka, sierp...). */
	void DoRuszenia(const TCHAR* Bron = nullptr);
	/** Po bitwie: wraca do pracy sprzed ruszenia (albo do tej, ktora osada przydzielila mu w czasie bitwy). */
	void WrocZBitwy();
	bool Lezy() const { return Stan == EStan::Lezy; }
	bool WBitwie() const { return bWalka; }
	bool Uderza() const { return bWalka && Stan == EStan::Pracuje; }
	bool WRuchu() const { return Stan == EStan::Idzie; }
	/** Omijanie budynkow dla trasy Pkt (ostatni = cel) z zapasem Zapas cm wokol obrysow (marsz oddzialu w bitwie). */
	static void OminBudynkiTrasa(UWorld* W, FVector2D Start, TArray<FVector2D>& Pkt, float Zapas);

	/** Powiekszenie wzgledem prawdziwego czlowieka (1,7 m), zeby byl widoczny z gory. */
	static constexpr float Skala = 1.4f;

	/** Osadnicy (poza bitwa) zyja tyle razy szybciej: chod, praca, przerwy i animacja (prosba uzytkownika 21.11.2026:
	 *  "za wolno to wszystko sie dzieje"). Bitwa (ABitwa) ma swoje predkosci i tego nie uzywa. */
	static constexpr float TempoOsadnikow = 1.4f;

	/** Predkosc chodu (cm/s, w skali gry). */
	UPROPERTY(EditAnywhere, Category = "Ludzik")
	float Predkosc = 190.f;

	/** Tragarz budowy i noszacy chodza o 20% szybciej (prosba uzytkownika 21.11.2026: "donoszenie o 20% szybciej"). */
	static constexpr float TempoNoszenia = 1.2f;

	/** Zbieracze (drwal, zbieracz, mysliwy) szukaja najpierw w zasiegu budynku, a gdy tam pusto - coraz dalej
	 *  (x2, x4), zamiast stac bezczynnie (prosba uzytkownika 21.11.2026: osada daleko od drzew). */
	static constexpr int32 IleZasiegow = 3;
	static constexpr float DalszeZasiegi[IleZasiegow] = { 1.f, 2.f, 4.f };

	/** Predkosc chodu z uwzglednieniem roli (noszenie szybciej). */
	float PredkoscChodu() const
	{
		return ((Rola == ERolaLudzika::Tragarz || Rola == ERolaLudzika::Noszacy) && !bWalka ? Predkosc * TempoNoszenia : Predkosc) * NaDrodze;
	}

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Korzen;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Tulow;
	// Szkielet z czesci (22.11.2026): Tulow = biodra, na nim Klatka (pas) z glowa i rekami; RekaL / RekaP to ramiona,
	// PrzedL / PrzedP przedramiona na lokciu (narzedzie, tarcza i choragiew w dloni przedramienia). Kobieta: suknia
	// w calosci na Tulow, Klatka bez modelu i bez ruchu.
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Klatka;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Glowa;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RekaL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> RekaP;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PrzedL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PrzedP;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> NogaL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> NogaP;
	// Noga z kolanem (21.11.2026): NogaL / NogaP to uda, lydki ze stopa wisza na kolanie (IK stawia stope na ziemi).
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> LydkaL;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> LydkaP;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mlotek;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Kamien;
	// Wyglad woja (tworzone dopiero w bitwie, osadnicy ich nie maja).
	UPROPERTY() TObjectPtr<UStaticMeshComponent> TarczaC;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> HelmC;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> BrodaC;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> ChoragiewC;

private:
	enum class EStan : uint8 { Idzie, Pracuje, Bierze, Odklada, Stoi, Kopie, Zbiera, Lezy };
	/** Krok pracy (co zrobic, gdy skonczy sie obecny stan). */
	enum class EKrok : uint8 { Start, PoTowar, Bierze, Niesie, Odklada, DoSterty, ZeSterty, PodMur, NaMur, DoCelu, Praca, PoSadzonke, Wraca };

	/** Punkt w ukladzie budynku (cm, bez skali) -> swiat (X, Y). */
	FVector2D ZBudynku(FVector2D Lokalny) const;
	/** Obwod prostokata wokol budynku (dla budowniczych): parametr S (cm) -> punkt lokalny i kierunek do budynku. */
	FVector2D NaObwodzie(float S, FVector2D* DoSrodka = nullptr) const;
	float ObwodDlugosc() const;
	float NajblizszeS(FVector2D Lokalny) const;

	void NastepneZadanie();
	void ZadanieBudowniczego();
	void ZadanieTragarza();
	void ZadanieDrwala();
	void ZadanieBorowego();
	void ZadaniePilarza();
	void ZadanieNoszacego();
	void ZadanieKamieniarza();
	void ZadanieZbieracza();
	void ZadanieMysliwego();
	void ZadanieRolnika();
	void ZadanieRzemieslnika();
	void ZadaniePasterza();
	void ZadanieWoja();
	void ZadanieRybaka(); // paczka A: brzeg rzeki za chata, wedka, ryby na stos
	/** Woj oddaje wlocznie i przeszywanice do swojej zbrojowni (kolor tuniki wraca). */
	void ZdejmijZbroje();
	/** Budowa skonczona / praca odebrana: odejdz kawalek (bOdejdz) i stan. */
	void KoniecPracy(bool bOdejdz);
	/** Oddaje rezerwacje (towar, miejsce, drzewo); niesiony towar wraca do zrodla. */
	void Zwolnij();

	void IdzDo(FVector2D Cel);
	void Czekaj(float Sekundy, const TCHAR* Co);
	void Pracuj(float Sekundy, FVector2D PatrzNa, const TCHAR* Narzedzie, const TCHAR* Co);
	void Schyl(EStan S, float Sekundy, FVector2D PatrzNa);
	void Pokaz(const TCHAR* Niesione); // nullptr = nic w rekach
	/** Wez sztuke Towar ze Zrodlo (po Bierze). false = juz nie ma. */
	bool WezZeZrodla();
	void Animuj(float Dt);
	UStaticMeshComponent* Czesc(const TCHAR* Nazwa, USceneComponent* Rodzic, FVector Staw);

	void Zaladuj();

	TWeakObjectPtr<ABudowa> Budowa;
	TWeakObjectPtr<AOsada> Osada;
	TWeakObjectPtr<ABudowa> Zrodlo; // skad bierze towar
	ESurowiec Towar = ESurowiec::Kloda;
	bool bRezZrodla = false;  // Zrodlo->Zarezerwowane[Towar] +1
	bool bRezCelu = false;    // Budowa->WDrodze[Towar] +1
	bool bTowarWRekach = false;
	int32 Ladunek = 1;         // ile sztuk Towar niesie naraz (tragarz budowy do 3; rezerwacje i towar w rekach)
	bool bRezDrzewa = false;
	bool bRezKrzaka = false;  // Drzewo = krzak zbieracza
	bool bOdchodzi = false;
	float Szansa = 0.3f;      // mysliwy: szansa trafienia na tym polowaniu (gestosc lasu)
	/** Rolnik: co robi przy pracy (animacja): 0 = nic, 1 = siew, 2 = zniwa. */
	uint8 Robota = 0;
	float CzasRoboty = 0.f;   // rolnik: sekundy pracy do zaliczenia polu (orka, siew)
	bool bRez2 = false;       // rzemieslnik: Budowa->Zarezerwowane[Wsad2] +1 (kuznia: kloda)
	int32 Owca = -1;          // pasterz: zarezerwowana owca w stadzie
	bool bUzbrojony = false;  // woj: ma wlocznie ze zbrojowni
	bool bPrzeszywanica = false; // woj: ma przeszywanice (sukno ze zbrojowni)
	bool bWalka = false;      // w bitwie: steruje ABitwa
	float Opad = 0.f;         // polegly: 0..1 upadek
	bool bWTyl = false;       // polegly pada na plecy
	bool bTopor = false;      // w bitwie z toporem: ciecie znad glowy zamiast pchniecia
	bool bLuk = false;        // w bitwie z lukiem albo kusza (strzela; bron w dloni zawsze pionowo / poziomo)
	bool bKusza = false;
	float Odrzut = 0.f;       // 1 -> 0: zachwianie po trafieniu
	float Zaslona = 0.f;      // 1 -> 0: tarcza w gorze po zatrzymanym ciosie
	float Okrzyk = 0.f;       // 1 -> 0: okrzyk z bronia w gorze
	float OkrzykZa = 0.f;     // s do okrzyku (0 = brak)
	float Rytm = 1.f;         // wlasne tempo oddechu i rozgladania (kazdy woj troche inny)
	// Walka filmowa (22.11.2026).
	uint8 RodzajBroni = 0;    // 0 wlocznia, 1 topor / maczuga / narzedzie (znad glowy), 2 miecz (ciecia na zmiane)
	bool bZLewej = false;     // miecz: nastepne ciecie z lewej
	float Wstrzas = 0.f;      // 1 -> 0: cios na tarczy / broni
	float Zgiecie = 0.f;      // 1 -> 0: trafiony strzala zgina sie i lapie za rane
	float Przyklek = 0.f;     // 1 -> 0: przyklęka po strzale
	float BokCiosu = 0.f;     // -1..1: z ktorej strony przyszedl cios (skret przy zachwianiu)
	float Ranny = 0.f;        // 0..1
	uint8 Upadek = 0;         // 0 prosto, 1 na kolana i na twarz, 2 z obrotem, 3 odrzut do tylu
	float ObrotUpadku = 0.f;
	FVector2D KierUpadku = FVector2D::ZeroVector;
	FVector2D PozUpadku = FVector2D::ZeroVector;
	float Kona = 0.f;         // s: konajacy rusza sie jeszcze na ziemi
	FVector KikutLok = FVector::ZeroVector; // w ukladzie klatki
	bool bKikut = false;
	bool bFilm = false;
	bool bNapor = false;
	float NaporW = 0.f;       // 0..1: plynne wejscie w postawe naporu
	float DtAnim = 0.f;       // czas zebrany do rzadszej animacji (poza kadrem / daleko)
	uint32 KlatkaAnim = 0;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Strzaly;
	bool bZOsady = false;     // w bitwie z pospolitego ruszenia: po bitwie wraca do pracy
	TWeakObjectPtr<ABudowa> PracaPrzed;
	ERolaLudzika RolaPrzed = ERolaLudzika::Budowniczy;
	float BezRoboty = 0.f;            // ile sekund czeka w pracy bez zajecia (pelny stos, brak wsadu, zima...)
	TWeakObjectPtr<ABudowa> Stala;    // pomocnik na budowie: jego wlasna praca, do ktorej wroci
	ERolaLudzika RolaStala = ERolaLudzika::Budowniczy;
	void Bezrobotny(float Sekundy, const TCHAR* Co) { BezRoboty += Sekundy; Czekaj(Sekundy, Co); }
	/** Czesci, ktore odpadly (bron, tarcza, choragiew, odcieta glowa / reka): leca i zostaja na ziemi. */
	struct FLot { TWeakObjectPtr<USceneComponent> C; FVector V = FVector::ZeroVector; FRotator Wir = FRotator::ZeroRotator; FRotator NaZiemi = FRotator::ZeroRotator; bool bLezy = false; };
	TArray<FLot> Loty;
	void Rzuc(USceneComponent* C, FVector V, FRotator NaZiemi);
	void Leca(float Dt);
	UStaticMeshComponent* Dodatek(const TCHAR* Nazwa, USceneComponent* Rodzic, FVector Staw);
	FLinearColor KolorTuniki = FLinearColor::White;
	FLinearColor KolorWlosow = FLinearColor::White;
	FVector2D Drzewo = FVector2D::ZeroVector; // scinane drzewo / miejsce sadzenia
	EKrok Krok = EKrok::Start;
	FString Status;
	bool bZaladowany = false;
	bool bKobieta = false;
	bool bMaDom = false;
	FVector2D Dom = FVector2D::ZeroVector;
	float Przerwa = 5.f; // ile stoi przed przejsciem sie po obejsciu
	ERolaLudzika Rola = ERolaLudzika::Budowniczy;
	EStan Stan = EStan::Stoi;
	float CzasStanu = 0.f;
	float Faza = 0.f;   // cykl kroku / uderzenia
	float FazaChodu = 0.f;  // 0..1: pelny cykl chodu (dwa kroki), stopy stawiane na ziemi
	float WagaChodu = 0.f;  // 0 = postawa, 1 = chod (plynne przejscie przy ruszaniu i zatrzymaniu)
	/** Nogi: udo i lydka tak, zeby stopa doszla do punktu (X do przodu, Z nad ziemia) przy biodrze na wysokosci H. */
	void UstawNoge(UStaticMeshComponent* Udo, UStaticMeshComponent* Lydka, float Y, float H, FVector2D Stopa);
	bool bNiesie = false;

	// Ruch: albo prosto do celu, albo (budowniczy) wzdluz obwodu budynku do parametru SCel.
	FVector2D Cel = FVector2D::ZeroVector;
	bool bPoObwodzie = false;
	float S = 0.f, SCel = 0.f;
	float Kierunek = 0.f; // yaw (st.)
	/** Drogi (ADrogi, 21.11.2026): punkty posrednie po drogach i cel na koncu; Cel = Trasa[TrasaI]. */
	TArray<FVector2D> Trasa;
	int32 TrasaI = 0;
	/** ADrogi::Premia na drodze, inaczej 1. */
	float NaDrodze = 1.f;
	FVector2D Patrz = FVector2D(1.f, 0.f);
};
