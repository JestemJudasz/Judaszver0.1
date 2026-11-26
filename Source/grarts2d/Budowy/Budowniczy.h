// Plik: Source/grarts2d/Budowy/Budowniczy.h
// Sterowanie budowaniem na mapie grywalnej i interfejs osady:
//   Start gry: po wejsciu na mape grywalna gracz wybiera miejsce obozu (podglad 5 namiotow i ogniska, lewy klik).
//   Potem dolny pasek budowy (SPasekBudowy): zakladki Mieszkalne / Religia, klik w budynek = polprzezroczysty podglad
//   pod kursorem (zielony = mozna, czerwony = nie), R - obroc o 45 st., lewy klik (bez przeciagania) - postaw
//   (Shift - stawiaj dalej), prawy klik / Esc - anuluj, T - tempo budowy x1 / x10. U gory panel osady (ludzie, rodziny, budowy).
//   Klik w budynek (bez stawiania) otwiera okno budynku (SOknoBudynku): budowa i material, rodziny w pracy (+/-), zapasy.
//   U gory na srodku licznik surowcow osady (klody, deski, kamien, jedzenie, zboze, maka), zadowolenie (podpowiedz:
//   skladniki), rodziny bez pracy i pora roku; pod nim wiadomosci osady (przybyla / odeszla rodzina, brak jedzenia, pory).
//   Tartak wodny da sie postawic tylko nad rzeka (koniec koryta mlynowki do 6 m od jej srodka).
// Budynki, ludzie, rodziny i przydzial pracy: AOsada. Poza mapa grywalna wszystko jest ukryte (mapa papierowa jest plaska).
// Opcje zrzutow kontrolnych: -kosciol (5 kosciolow w kolejnych fazach), -osada (oboz, domy, drwal, pilarnia, plac skladowy,
// kamieniolom, chata zbieracza, spichlerz, pole przy zniwach, mlyn, piekarnia; okno pola otwarte; z -ui zrzut
// pokazuje interfejs); -osada -rody: targ z towarem, rody po 15 minutach, okno rodow otwarte.
// Etap 5 (21.11.2026): ARody (inne rody na mapie, liczone liczbami) zakladane razem z obozem; okno rodow (H, SOknoRodow):
// handel na targu, podarunki, danina; u gory licznik groszy; na mapie papierowej choragiewki rodow (SZnakiRodow).
// Wojna W1 (21.11.2026): ABitwa (Wojna/Bitwa.h) - klawisz N = bitwa probna pod kursorem; -bitwa = pokaz bitwy bez obozu.
// W czasie bitwy mysz i klawisze steruja oddzialami, budowanie stoi.
// Pole i pastwisko (21.11.2026): zaznaczane na terenie - pierwszy klik = rog, drugi = przeciwny rog, R obraca; nad paskiem
// panel z wielkoscia (m, ha), plonem / stadem, rodzinami i kosztem, Zbuduj (Enter) / Anuluj (Esc); prawy klik = krok wstecz.
// -osada -obszar: duze pole przy zniwach, duze pastwisko i zaznaczone nowe pole z otwartym panelem.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Osada/Surowce.h"
#include "Budowniczy.generated.h"

class ABudowa;
class AOsada;
class ATerenKafle;
class APlayerController;
class SWidget;
class STextBlock;
class SPasekBudowy;
class SOknoBudynku;
class SOknoRodow;
class SOknoWojska;
class ARody;
class ABitwa;
class ADrogi;
class SBox;
class UTexture2D;
struct FRodzajBudynku;

UCLASS()
class GRARTS2D_API ABudowniczy : public AActor
{
	GENERATED_BODY()

public:
	ABudowniczy();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Najwieksza roznica wysokosci terenu pod budynkiem (cm). Fundament siega 3 m w dol. */
	UPROPERTY(EditAnywhere, Category = "Budowy")
	float MaxSpadek = 250.f;

private:
	/** Punkt terenu pod kursorem (promien z kamery do przeciecia z terenem). */
	bool TerenPodKursorem(APlayerController* PC, FVector& Out) const;
	/** Czy mozna tu postawic; Z = wysokosc, na ktorej stanie budynek. */
	bool MoznaPostawic(const FRodzajBudynku& R, FVector2D Gdzie, float Obrot, float& Z) const;
	/** Pole i pastwisko: teren w calym prostokacie (spadek rosnie z wielkoscia), szopa owczarni jak zwykly budynek. */
	bool MoznaObszar(const FRodzajBudynku& R, FVector2D Gdzie, float ObrotSt, float& Z) const;
	/** Zaznaczanie prostokata pola / pastwiska (zamiast Stawianie). */
	void StawianieObszaru(APlayerController* PC);
	/** Palisada i mur: przeciaganie linii odcinkow (FRodzajBudynku::bLinia). */
	void StawianieLinii(APlayerController* PC);
	void UsunPodgladLinii();
	TArray<TWeakObjectPtr<ABudowa>> PodgladLinii;
	bool bLiniaTrwa = false;
	/** Odcinki muru pod stawiana brama (rozbierane przy postawieniu, 26.11.2026). */
	TArray<TWeakObjectPtr<class ABudowa>> MurPodBrama;
	FVector2D LiniaA = FVector2D::ZeroVector;
	/** Panel nad paskiem budowy: Zbuduj -> stawia Szkic. */
	void ZbudujSzkic();
	FString OpisSzkicu() const;
	TSharedRef<SWidget> PanelObszaru();
	/** -osada -obszar: pokaz pola i pastwiska roznej wielkosci. */
	void PokazObszarow(FVector2D Srodek);
	/** Oboz w punkcie: wszystkie namioty i ognisko na dobrym terenie; Z = ich wysokosci (kolejnosc AOsada::MiejscaObozu). */
	bool MoznaOboz(FVector2D Srodek, TArray<float>& Z) const;
	void Wybierz(const FRodzajBudynku* R);
	void PokazPodgladObozu(bool bPokaz);
	void ZrobInterfejs();
	void UstawPomoc();
	void UstawInfo();
	void Pokaz();
	void PokazOsady();
	/** Zrzut kontrolny -poziomy (22.11.2026): domy 3 poziomy x 4 warianty, przebudowy w toku, trzy poziomy kosciola. */
	void PokazPoziomow();
	/** Tryb deweloperski: oboz (jesli go nie ma), kazdy budynek gotowy, rodziny w kazdej pracy, surowce. Zwraca podsumowanie. */
	FString PostawWszystko();
	void Stawianie(APlayerController* PC);
	void WyborObozu(APlayerController* PC);
	/** Lewy klik bez przeciagania (przeciaganie przesuwa mape). */
	bool Klik(APlayerController* PC);
	void OtworzOkno(ABudowa* B);
	void ZamknijOkno();
	UTexture2D* IkonaDla(const FRodzajBudynku& R) const;
	TSharedRef<SWidget> LicznikSurowcow();
	/** Okno rodow (H): otworz / zamknij; IndeksRodu >= 0 - od razu ten rod. */
	void OtworzRody(int32 IndeksRodu = -1);
	void ZamknijRody();
	/** Okno Wojsko (J): oddzialy pospolitego ruszenia, powolaj / rozwiaz (Wojna/OknoWojska). */
	void OtworzWojsko();
	void ZamknijWojsko();

	/** Inne rody (etap 5): zakladane razem z obozem gracza. */
	UPROPERTY(Transient)
	TObjectPtr<ARody> Rody;
	/** Drogi (G, 21.11.2026): rysowane na terenie, budynki przyciagaja sie do nich, ludziki chodza nimi szybciej. */
	UPROPERTY(Transient)
	TObjectPtr<ADrogi> Drogi;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> ZnakDrogi;
	/** R: krotko = nastepne 45 st., przytrzymane = plynny obrot (F w druga strone), dowolny kat 0-360. */
	void ObrocKlawiszem(APlayerController* PC);
	float CzasObrotu = 0.f;
	/** -drogi: pokaz drog z budynkami przyciagnietymi do nich (zrzut kontrolny). */
	void PokazDrog(FVector2D Srodek);
	/** Bitwa probna (wojna, etap W1). */
	UPROPERTY(Transient)
	TObjectPtr<ABitwa> Bitwa;
	bool bTrybBitwy = false;    // -bitwa: pokaz bitwy, bez obozu
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> IkonaGroszy;

	UPROPERTY(Transient)
	TObjectPtr<AOsada> Osada;

	/** Ikony paska (trzymane tu, zeby GC ich nie zwolnil; Slate ma tylko wskaznik). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> Ikony;

	/** Znaki dolnego paska (T_Ikona_Znak_<Nazwa>, kolejnosc SPasekBudowy::NazwyZnakow). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> Znaki;

	/** Ikony surowcow (T_Ikona_Kloda / Deska / Kamien) i rodziny (Tools/ikony.py). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> IkonySurowcow;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> IkonaRodziny;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> IkonaZadowolenia;

	TWeakObjectPtr<ATerenKafle> Teren;
	TWeakObjectPtr<ABudowa> Podglad;
	TArray<TWeakObjectPtr<ABudowa>> PodgladObozu;
	const FRodzajBudynku* Wybrany = nullptr;
	float Obrot = 90.f; // front domu (drzwi, plac) do kamery w poczatkowym ustawieniu
	bool bMozna = false;
	FVector Punkt = FVector::ZeroVector;
	float PunktZ = 0.f;
	TArray<float> ZObozu;

	/** Pole i pastwisko: 0 = czeka na pierwszy rog, 1 = na drugi, 2 = zaznaczone (panel: Zbuduj / Anuluj). */
	int32 KrokObszaru = 0;
	FVector2D RogA = FVector2D::ZeroVector;
	/** Rodzaj na zaznaczony prostokat (FRodzajBudynku::NaObszar) i gdzie stanie. */
	TSharedPtr<FRodzajBudynku> Szkic;
	FVector2D SzkicGdzie = FVector2D::ZeroVector;
	/** Ostatnie polozenie podgladu (X, Y, obrot): po zmianie plot i kafle liczone od nowa na terenie. */
	FVector SzkicPoz = FVector(1e9);

	FVector2D KlikStart = FVector2D::ZeroVector;
	bool bKlikTrwa = false;

	bool bWidoczne = true;
	bool bPokazZrobiony = false;
	bool bTrybPokazu = false;   // -kosciol: bez obozu
	bool bInterfejs = true;     // -zrzut bez -osada: czysty obraz
	float DoInfo = 0.f;
	float DoAutozapisu = 300.f; // FZapisGry::CoIleAutozapis

	TSharedPtr<SWidget> Nakladka;
	TSharedPtr<STextBlock> Pomoc;
	TSharedPtr<STextBlock> Info;
	TSharedPtr<SPasekBudowy> Pasek;
	TSharedPtr<SBox> MiejsceOkna;
	TSharedPtr<SOknoBudynku> Okno;
	TSharedPtr<SBox> MiejsceRodow;
	TSharedPtr<SOknoRodow> OknoRodow;
	TSharedPtr<SBox> MiejsceWojska;
	TSharedPtr<SOknoWojska> OknoWojska;
	FSlateBrush IkonaGroszyB;
	FButtonStyle StylPrzyciskuRodow;
	FSlateBrush TloInfo;
	FSlateBrush IkonySurowcowB[int32(ESurowiec::Ile)];
	FSlateBrush IkonaRodzinyB;
	FSlateBrush IkonaZadowoleniaB;
};
