// Plik: Source/grarts2d/Osada/Drogi.h
// Drogi jak w Manor Lords (prosba uzytkownika 21.11.2026): darmowe, rysowane na terenie odcinkami z lukami.
//   Klawisz G albo przycisk "Drogi" na dolnym pasku: lewy klik = poczatek, kolejne kliki = nastepne punkty (odcinek wygina sie
//   lukiem, zeby plynnie przejsc z poprzedniego), Shift = prosto (ostry zakret), prawy klik = koniec ciagu, drugi prawy klik /
//   Esc = wyjscie, Ctrl + klik = usun odcinek. Poczatek i koniec przyciagaja sie do drog obok (skrzyzowania).
//   Dopasowanie do drog (prosba 23.11.2026): luk wychodzi i wchodzi stycznie w koniec drogi obok, w srodek drogi pod katem
//   prostym; klik na innej drodze konczy ciag. W miejscu wejscia w droge wstega sie rozszerza (wyokraglony wjazd).
//   Droga nie wejdzie na wode, rzeke, zbyt stromy stok ani budynek; karczuje drzewa i trawe.
// Budynki przyciagaja sie do drogi frontem (ABudowniczy, Alt = bez przyciagania) i nie moga na niej stanac.
// Ludziki: graf drog (wezly co Probka cm, polaczone tez na skrzyzowaniach) - jesli droga skraca czas, ida nia (Trasa),
//   a na drodze chodza o 10% szybciej (Premia).
// Wyglad: ProceduralMesh nad terenem, material M_Droga (Tools/drogi.py + Tools/Zbuduj_Drogi.bat): ziemia z koleinami,
//   poszarpane pikselowe brzegi (maska z kanalu alfa wierzcholkow i szumu tekstury). Od 23.11.2026: dwie koleiny i pas
//   trawy posrodku, falisty brzeg, kepki trawy i kamyki wzdluz brzegow (modele z Tools/ue_natura.py).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Drogi.generated.h"

class UProceduralMeshComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ATerenKafle;
class AOsada;
class APlayerController;
struct FRodzajBudynku;

UCLASS()
class GRARTS2D_API ADrogi : public AActor
{
	GENERATED_BODY()

public:
	ADrogi();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Drogi osady gracza (jedne na swiat) - dla ludzikow. */
	static ADrogi* Obecne() { return Aktywne.Get(); }

	/** Polowa szerokosci drogi (cm): droga 3,2 m. */
	static constexpr float Polszerokosc = 160.f;
	/** Ludziki na drodze chodza o tyle szybciej (prosba: +5-10%). */
	static constexpr float Premia = 1.1f;
	/** Odstep punktow drogi i wezlow grafu (cm). */
	static constexpr float Probka = 250.f;

	void Ustaw(AOsada* O, ATerenKafle* T);

	/** Tryb rysowania drog (G). */
	bool Rysuje() const { return bRysuje; }
	void Wlacz(bool bWlacz);
	/** Klatka trybu rysowania: bJest/P = teren pod kursorem, bKlik = lewy klik bez przeciagania. */
	void Rysuj(APlayerController* PC, bool bJest, FVector2D P, bool bKlik);
	/** Napis pomocy w trybie rysowania. */
	FString Pomoc() const;

	/** Czy punkt lezy na drodze (Zapas poszerza droge). */
	bool NaDrodze(FVector2D P, float Zapas = 0.f) const;
	/** Budynek przy drodze: srodek i obrot tak, zeby front (+Y budynku) patrzyl na droge, a budynek stal tuz przy niej.
	 *  Glebokosc = polowa glebokosci budynku (cm). false = zadnej drogi w poblizu. */
	bool Przyciagnij(FVector2D P, float Glebokosc, FVector2D& Gdzie, float& ObrotSt) const;
	/** Czy obrys budynku (zwykly albo zaznaczony prostokat) nachodzi na droge. */
	bool PrzecinaBudynek(const FRodzajBudynku& R, FVector2D Gdzie, float ObrotSt) const;
	/** Trasa ludzika z Od do Do: punkty posrednie po drogach i Do na koncu. false = prosto jest szybciej. */
	bool Trasa(FVector2D Od, FVector2D Do, TArray<FVector2D>& Out) const;

	/** Pokaz / zrzut: droga po tych punktach (gesciej probkowana), bez sprawdzania terenu. */
	void DodajDroge(const TArray<FVector2D>& Punkty);
	int32 Odcinkow() const { return Odcinki.Num(); }

	/** Luk od A do B (krzywa Beziera 3. stopnia), punkty co ok. Probka. KierA = kierunek wyjscia z A, KierB = kierunek
	 *  dojscia do B (zero = bez warunku; oba zero = prosto). Publiczne dla testu grarts2d.Drogi.Luk. */
	static void Krzywa(FVector2D A, FVector2D KierA, FVector2D B, FVector2D KierB, TArray<FVector2D>& Out);

private:
	static TWeakObjectPtr<ADrogi> Aktywne;

	struct FOdcinek
	{
		TArray<FVector2D> P; // punkty co ok. Probka cm, od poczatku do konca
	};
	TArray<FOdcinek> Odcinki;

	// Graf (Odbuduj): wezly = punkty odcinkow, Nast = nastepny punkt w tym samym odcinku (-1 na koncu).
	TArray<FVector2D> Wezly;
	TArray<int32> Nast;
	TArray<int32> OdcinekWezla;
	TArray<TArray<int32>> Sasiedzi;
	TMap<FIntPoint, TArray<int32>> Siatka; // komorki RozmiarKomorki cm -> wezly
	static constexpr float RozmiarKomorki = 1000.f;
	static FIntPoint Komorka(FVector2D P) { return FIntPoint(FMath::FloorToInt32(P.X / RozmiarKomorki), FMath::FloorToInt32(P.Y / RozmiarKomorki)); }
	/** Wezly w kole (przez komorki siatki). */
	void WezlyWKole(FVector2D P, float R, TArray<int32>& Out) const;
	int32 NajblizszyWezel(FVector2D P, float MaxOdl) const;
	/** Najblizszy punkt na odcinkach drog: punkt, kierunek drogi tam, odleglosc. */
	bool NajblizszyPunkt(FVector2D P, float MaxOdl, FVector2D& Q, FVector2D& Kier, float& Odl) const;

	void Odbuduj();
	void BudujSiatke();
	/** Wstega drogi (i okragle konce) do sekcji siatki; Baza = polozenie aktora. Nad = cm nad terenem (rozne dla odcinkow,
	 *  zeby nachodzace na siebie nie migaly), bWjazdA/B = koniec wchodzi w srodek innej drogi (wstega rozszerza sie lejkiem). */
	void Wstega(UProceduralMeshComponent* C, int32 Sekcja, const TArray<FVector2D>& P, bool bKonce, float Nad = 14.f, bool bWjazdA = false, bool bWjazdB = false) const;
	/** Teren pod droga: lad, nie rzeka, lagodny stok, bez budynkow. */
	bool MoznaDroge(const TArray<FVector2D>& P) const;
	/** Koniec drogi przy kursorze, przyciagniety do drogi obok. Zwraca 0 = wolny punkt, 1 = koniec drogi (Kier = kierunek
	 *  wyjscia z niej na zewnatrz), 2 = srodek drogi (Kier = kierunek drogi w tym miejscu). */
	int32 PrzyciagnijKoniec(FVector2D P, FVector2D& Gdzie, FVector2D& Kier) const;
	/** Czy koniec E odcinka Ktory lezy w srodku innej drogi (nie na jej koncu). */
	bool WchodziWDroge(int32 Ktory, FVector2D E) const;
	/** Odleglosc od Q do najblizszej innej drogi niz Ktory (najwyzej Max). */
	float OdInnejDrogi(int32 Ktory, FVector2D Q, float Max) const;
	/** Kepki trawy i kamyki wzdluz brzegow i miedzy koleinami (od nowa przy kazdej zmianie drog, polozenia z hasza punktu). */
	void PosadzKepki();
	void DodajOdcinek(const TArray<FVector2D>& P);
	/** Odcinek pod kursorem (-1 = brak). */
	int32 OdcinekPod(FVector2D P) const;
	void UsunOdcinek(int32 Ktory);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProceduralMeshComponent> Siatki;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProceduralMeshComponent> Szkic;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Kepki; // SM_Trawa_1, SM_Trawa_2, SM_Kamien_1
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> Material;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialSzkicu;

	TWeakObjectPtr<AOsada> Osada;
	TWeakObjectPtr<ATerenKafle> Teren;

	bool bRysuje = false;
	bool bStart = false;
	FVector2D Start = FVector2D::ZeroVector;
	FVector2D StartKier = FVector2D::ZeroVector;
	int32 StartTryb = 0; // jak PrzyciagnijKoniec: 2 = start w srodku drogi (StartKier = kierunek tamtej drogi)
	bool bSzkicDobry = false;
	mutable FString Uwaga; // czemu nie mozna (do pomocy)
};
