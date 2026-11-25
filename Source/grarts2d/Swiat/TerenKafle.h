// Plik: Source/grarts2d/Swiat/TerenKafle.h
// Teren calego swiata jako kafle quadtree (jak Google Maps): blisko kamery male, gesto podzielone kafle (512 m),
// daleko duze i rzadkie (do jednego kafla na caly swiat). Siatki licza sie w tle na watkach roboczych.
// Stary kafel znika dopiero, gdy kafle, ktore go zastepuja, sa gotowe (brak dziur przy zoomie).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Async/Future.h"
#include "TerenKafle.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UInstancedStaticMeshComponent;

UCLASS()
class GRARTS2D_API ATerenKafle : public AActor
{
	GENERATED_BODY()

public:
	ATerenKafle();
	virtual void Tick(float DeltaSeconds) override;

	/** Ile z kafli potrzebnych dla obecnego widoku jest gotowych (0..1). Do ekranu ladowania. */
	float Postep() const;

	/** Wszystkie kafle potrzebne dla obecnego widoku sa gotowe (z drzewami). */
	bool Gotowe() const { return Chciane.Num() > 0 && Postep() >= 1.f; }

	/**
	 * Mapa grywalna: zamiast kafli quadtree pierscien kafli 512 m w pelnej jakosci (KwadyRegionu, drzewa, drobiazgi)
	 * wokol ogniska kamery. Kafle przed kamera licza sie w tle z zapasem, wiec przy jezdzie nic nie doczytuje sie
	 * na oczach gracza. Stare kafle znikaja od razu (pierwsze wczytanie zaslania ekran ladowania).
	 */
	void WejdzDoRegionu();

	/** Powrot do mapy Europy (kafle wg kamery, bez drzew). */
	void WyjdzZRegionu();

	bool WRegionie() const { return bRegion; }

	/** Mapa grywalna: usuwa drzewa, krzaki i drobiazgi w kole (cm) - pod budynkiem. Kafle liczone pozniej tez je pomijaja. */
	void Wykarczuj(FVector2D Srodek, float Promien);

	// --- Las dla ekonomii (chata drwala, chata borowego) ---
	/** Najblizsze dorosle drzewo (nie krzak, nie rosnaca sadzonka) w kole; pomija drzewa blizej niz 1 m od punktow Pomin. */
	bool ZnajdzDrzewo(FVector2D Srodek, float Promien, const TArray<FVector2D>& Pomin, FVector2D& Out) const;
	/** Scina drzewo stojace w punkcie (najblizsze w 1,5 m); nie odrasta po ponownym wczytaniu kafla. */
	bool Zetnij(FVector2D Gdzie);
	/** Czy w kole stoi drzewo, krzak albo sadzonka (borowy nie sadzi za gesto). */
	bool DrzewoBlisko(FVector2D Gdzie, float Promien) const;
	/** Mlode drzewo borowego: rosnie CzasWzrostu sekund, potem drwal moze je sciac. */
	void Zasadz(FVector2D Gdzie);
	/** Etap 2 (zbieracz): najblizszy krzak w kole, pomija krzaki blizej niz 1 m od punktow Pomin (zerwane, zajete). */
	bool ZnajdzKrzak(FVector2D Srodek, float Promien, const TArray<FVector2D>& Pomin, FVector2D& Out) const;
	/** Etap 2 (mysliwy): ile doroslych drzew stoi w kole (gestosc lasu). */
	int32 DrzewWKole(FVector2D Srodek, float Promien) const;
	/** Ile sadzonek jeszcze rosnie w kole. */
	int32 Sadzonek(FVector2D Srodek, float Promien) const;
	/** Wzrost sadzonek (sekundy gry, razy tempo budowy). Wola AOsada. */
	void Rosnij(float Sekundy);

	/** Sekundy od posadzenia do doroslego drzewa. */
	UPROPERTY(EditAnywhere, Category = "Natura")
	float CzasWzrostu = 240.f;

	/** Ile kafli 512 m w kazda strone od kafla pod ogniskiem kamery (5 = 11x11 kafli, >= 2,5 km; przy max oddaleniu rog kadru ~1,6 km). */
	UPROPERTY(EditAnywhere, Category = "Region")
	int32 PromienRegionu = 5;

	/** Kepki trawy i kamyki znikaja dalej niz tyle od kamery (cm) - z wysoka i tak sa mniejsze niz piksel. */
	UPROPERTY(EditAnywhere, Category = "Region")
	float ZasiegDrobnych = 2.5e5f;

	/** Ile gotowych kafli na klatke trafia na ekran podczas jazdy (tworzenie komponentow idzie na watku gry). */
	UPROPERTY(EditAnywhere, Category = "Region")
	int32 KafliNaKlatke = 2;

	/** Kwadraty na bok kafla w regionie (128 = co 4 m). */
	UPROPERTY(EditAnywhere, Category = "Region")
	int32 KwadyRegionu = 128;

	/** Skala drzew w regionie - korony ~6 m jak na zrzucie wzorcowym (domy ~12 m). */
	UPROPERTY(EditAnywhere, Category = "Region")
	float SkalaDrzewRegionu = 0.55f;

	/** Kwadraty na bok kafla (64 = co 8 m na najblizszym poziomie). */
	UPROPERTY(EditAnywhere, Category = "Teren")
	int32 Kwady = 64;

	/** Kafel dzieli sie na 4, gdy kamera jest blizej niz Podzial * bok kafla. Wiecej = ladniej i wolniej. */
	UPROPERTY(EditAnywhere, Category = "Teren")
	float Podzial = 2.0f;

	/** Najglebszy poziom: 10 = kafle 512 m (mapa grywalna). */
	UPROPERTY(EditAnywhere, Category = "Teren")
	int32 MaxPoziom = 10;

	/** Najglebszy poziom mapy papierowej: plaska kartka, wiec wystarcza kafle 4 km (7) - mniej liczenia przy zoomie. */
	UPROPERTY(EditAnywhere, Category = "Teren")
	int32 MaxPoziomMapy = 7;

	/** Znaki mapy papierowej (M_Teren, Tools/mapa_znaki.py): ile oczek siatki znakow na szerokosc widoku. Wiecej = mniejsze i gestsze. */
	UPROPERTY(EditAnywhere, Category = "Teren")
	float ZnakiGestosc = 16.f;

	/** Ile kafli naraz liczy sie w tle. */
	UPROPERTY(EditAnywhere, Category = "Teren")
	int32 MaxZadan = 12;

	UPROPERTY(EditAnywhere, Category = "Teren")
	TObjectPtr<UMaterialInterface> Material;

	/** Drzewa na mapie Europy od tego poziomu kafli (99 = wcale: drzewa tylko w regionie, mapa doczytuje sie szybko). */
	UPROPERTY(EditAnywhere, Category = "Natura")
	int32 MinPoziomDrzew = 99;

	/** Oczko siatki sadzenia (cm); w kazdym oczku najwyzej jedno drzewo lub krzak. */
	UPROPERTY(EditAnywhere, Category = "Natura")
	float KomorkaDrzew = 900.f;

	/** Modele z Tools/ue_natura.py: dab, buk, brzoza, sosna, swierk, krzak, kamien, trawa - po 2 warianty (kolejnosc wazna dla rozsiewu). */
	UPROPERTY(EditAnywhere, Category = "Natura")
	TArray<TObjectPtr<UStaticMesh>> Modele;

	/** Gotowe siatki jednego kafla (liczone poza watkiem gry). */
	struct FDane
	{
		TArray<FVector> V;
		TArray<int32> T;
		TArray<FVector> N;
		TArray<FVector2D> UV0; // detal tekstur (1 = 4 m)
		TArray<FVector2D> UV1; // pozycja w kaflu 0..1 (M_Teren: + rog i bok kafla z Custom Primitive Data)
		TArray<FVector2D> UV2; // X = odleglosc od brzegu rzeki w metrach (ujemna w wodzie), dla M_Teren na mapie grywalnej
		TArray<TArray<FTransform>> Drzewa; // na kazdy model, wzgledem rogu kafla
	};

private:
	struct FKafel
	{
		TObjectPtr<UProceduralMeshComponent> Siatka = nullptr;
		TArray<TObjectPtr<UInstancedStaticMeshComponent>> Drzewa;
		TFuture<TSharedPtr<FDane>> Zadanie;
		bool bGotowy = false;
	};

	/** Klucz kafla: (X, Y, Poziom). */
	void Zbierz(int32 Poziom, int32 X, int32 Y, const FVector& Kamera, float NadZiemia, TArray<FIntVector>& Out) const;
	static FBox2D Obszar(const FIntVector& K);
	bool PokrytyGotowymi(const FIntVector& K, const TArray<FIntVector>& Lista) const;
	void Odswiez();
	void Wyczysc();

	bool bRegion = false;
	FIntPoint SrodekRegionu = FIntPoint::ZeroValue; // kafel pod ogniskiem kamery (z ostatniego Odswiez)

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialRegionu;

	/** Kafle poza regionem: M_Teren z parametrami mapy strategicznej (Mapa, SkalaMapy) ustawianymi co klatke. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialMapy;

	/** Wykarczowane kola: X, Y, promien (cm). */
	TArray<FVector> Polany;
	/** Polany w komorkach 50 m (23.11.2026: osady rodow karczuja setki kol, NaPolanie sprawdza tylko swoja komorke). */
	TMap<FIntPoint, TArray<int32>> PolanyKomorki;
	static constexpr double KomorkaPolan = 5000.0;
	/** Dopisuje polane do listy i komorek; false = lezy cala w juz wykarczowanej (nic nowego). */
	bool DodajPolane(FVector2D Srodek, float Promien);

	/** Drzewa posadzone przez borowych (nie naleza do kafli, wiec przetrwaja przeladowanie regionu). */
	struct FSadzonka
	{
		FVector2D P;
		float Wiek = 0.f;
		float Skala = 1.f;  // docelowa
		float Obrot = 0.f;
		int32 Model = 0;
	};
	TArray<FSadzonka> Sadzonki;

	/** Instancje sadzonek, po jednym komponencie na model (indeks jak Modele). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Mlode;

	FTransform TransformSadzonki(const FSadzonka& S) const;
	void OdbudujMlode(int32 Model);
	/** Szuka instancji drzewa (bKrzaki: krzaka) z kafli najblizszej punktu (do Zetnij / ZnajdzDrzewo / ZnajdzKrzak); Model < 0 = brak. */
	bool NajblizszeZKafli(FVector2D Srodek, float Promien, const TArray<FVector2D>* Pomin, UInstancedStaticMeshComponent*& Komp, int32& Indeks, FVector2D& Gdzie, bool bKrzaki = false) const;
	bool NaPolanie(double X, double Y) const;

	TMap<FIntVector, FKafel> Kafle;
	TArray<FIntVector> Chciane; // kafle dla obecnego widoku (z ostatniego Odswiez)
	bool bByloGotowe = false;   // do pierwszego wczytania liczymy wiecej kafli naraz
	double NastepneOdswiezenie = 0.0;
	int32 Zadania = 0;
};
