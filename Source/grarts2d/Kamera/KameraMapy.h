// Plik: Source/grarts2d/Kamera/KameraMapy.h
// Kamera jak w Google Maps: kolko przybliza/oddala do punktu pod kursorem (od 25 m do calej Europy),
// przeciaganie lewym przyciskiem przesuwa mape, WASD/strzalki przesuwaja, Q/E obracaja plynnie o dowolny kat (21.11.2026:
// takze na mapie grywalnej, wczesniej skokami po 90 st.), przeciaganie srodkowym przyciskiem w bok obraca kamere
// (w bitwie srodkowy przesuwa mape, bo lewy zaznacza oddzialy).
// Blisko patrzy ukosnie jak mapa grywalna (38 st.), z daleka prostuje sie do widoku mapy z gory.
// Podstawa: TotalFornt/Camera/RTSCameraPawn (uproszczona, bez Enhanced Input).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "KameraMapy.generated.h"

class UCameraComponent;

UCLASS()
class GRARTS2D_API AKameraMapy : public APawn
{
	GENERATED_BODY()
	friend struct FZapisGry;

public:
	AKameraMapy();

	/** Bitwa: lewy przycisk zaznacza oddzialy ramka, mape przesuwa wtedy tylko srodkowy (ABitwa). */
	static bool bBezLewegoPrzeciagania;
	/** Bitwa na mapie grywalnej: kamera moze zjechac nizej (do ~80 m), zeby bylo widac wojow z bliska (prosba 21.11.2026). */
	static bool bBitwaBlisko;
	float MinRegionu() const { return bBitwaBlisko ? FMath::Min(MinOdlRegionu, 8000.f) : MinOdlRegionu; }
	/** Odleglosc kamery od razu (zrzuty kontrolne bitwy). */
	void UstawOdl(float O) { Odl = CelOdl = O; }
	/** Dla dzwieku (UDzwieki): punkt na ziemi, na ktory patrzy kamera, jej oddalenie (cm) i czy to mapa grywalna. */
	FVector Ognisko() const { return Focus; }
	float Oddalenie() const { return Odl; }
	bool WRegionie() const { return bRegion; }
	/** Obrot kamery wokol pionu (st.) - prostokat widoku na minimapie. */
	float Obrot() const { return Yaw; }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** 0 = teren (region), 1 = mapa strategiczna (papier) na kazdym oddaleniu poza regionem. Dla M_Teren i AMapaRzeki. */
	float StrefaMapy() const;

	/** Szerokosc widoku na wysokosci ogniska (cm) - do elementow mapy o stalej wielkosci na ekranie. */
	float SzerokoscWidoku() const;
	/** 0 = mapa terenu, 1 = caly swiat (od ~15 km). */
	float StrefaSwiata() const;

	/** Odleglosc kamery od punktu na ziemi (cm). */
	UPROPERTY(EditAnywhere, Category = "Kamera")
	float MinOdl = 2500.f;

	UPROPERTY(EditAnywhere, Category = "Kamera")
	float MaxOdl = 6.0e7f;

	UPROPERTY(EditAnywhere, Category = "Kamera")
	float StartOdl = 4.0e5f;

	/** Jeden klik kolka zmienia odleglosc o tyle razy. */
	UPROPERTY(EditAnywhere, Category = "Kamera")
	float KrokKolka = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Kamera")
	float Wygladzanie = 10.f;

	/** Przesuw klawiszami: ulamek widocznej szerokosci na sekunde. */
	UPROPERTY(EditAnywhere, Category = "Kamera")
	float PredkoscPrzesuwu = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Kamera")
	float PredkoscObrotu = 90.f;

	/** Nachylenie mapy papierowej z daleka (prawie z gory). Blisko, na progu regionu, = NachylenieRegionu. */
	UPROPERTY(EditAnywhere, Category = "Kamera")
	float NachylenieDaleko = 85.f;

	/** Pole widzenia mapy papierowej: od FovRegionu na progu regionu, przez FovBlisko (20 km), do FovDaleko (cala Europa). */
	UPROPERTY(EditAnywhere, Category = "Kamera")
	float FovBlisko = 25.f;

	UPROPERTY(EditAnywhere, Category = "Kamera")
	float FovDaleko = 45.f;

	UPROPERTY(EditAnywhere, Category = "Kamera")
	float MinNadZiemia = 800.f;

	/** Do tej odleglosci (cm) poziom "budowa": efekt pikseli. Dalej obraz gladki. */
	UPROPERTY(EditAnywhere, Category = "Kamera")
	float PikseleDo = 1.5e5f;

	/** Przyblizenie mapy Europy ponizej tej odleglosci (cm) wchodzi do mapy grywalnej (region). */
	UPROPERTY(EditAnywhere, Category = "Region")
	float ProgRegionu = 1.0e5f;

	/** Rzut w regionie jak na zrzucie wzorcowym: staly kat, waskie pole widzenia (prawie bez perspektywy). */
	UPROPERTY(EditAnywhere, Category = "Region")
	float NachylenieRegionu = 38.f;

	UPROPERTY(EditAnywhere, Category = "Region")
	float FovRegionu = 20.f;

	/** Zoom w regionie: od kilku domow (~90 m szerokosci widoku) do ~0,5 km; wyzej jest juz papierowa mapa. */
	UPROPERTY(EditAnywhere, Category = "Region")
	float MinOdlRegionu = 2.5e4f;

	UPROPERTY(EditAnywhere, Category = "Region")
	float MaxOdlRegionu = 2.2e5f; // P5 26.11.2026: bylo 1.5e5, uzytkownik chcial wieksze oddalenie

	/** Najwieksza predkosc jazdy w regionie (cm/s, z Shiftem) - kafle 512 m musza nadazyc sie policzyc. */
	UPROPERTY(EditAnywhere, Category = "Region")
	float MaxPrzesuwRegionu = 1.5e5f;

	/** Na mapie grywalnej ognisko kamery trzyma sie tyle (cm) od krawedzi swiata, zeby nie bylo widac jego konca. */
	UPROPERTY(EditAnywhere, Category = "Region")
	float MarginesRegionu = 2.0e5f;

	/** Wlacza / wylacza tryb mapy grywalnej (wola ASwiatGameMode); Srodek = gdzie stanac po wejsciu. */
	void UstawRegion(bool bWlacz, FVector2D Srodek);

	/** Przenosi kamere nad punkt (bez zmiany oddalenia), np. do pokazu budynkow na zrzucie kontrolnym. */
	void UstawOgnisko(FVector2D Gdzie);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Kamera")
	TObjectPtr<UCameraComponent> Kamera;

private:
	void KolkoW();
	void KolkoZ();
	void ObrotL() {}
	void ObrotP() {}
	/** Obrot srodkowym przyciskiem: st. na piksel ruchu myszy. */
	float ObrotMyszy = 0.35f;
	bool bObracaMysza = false;
	float MyszX = 0.f;
	/** WASD wygladzone (22.11.2026): kamera rusza i hamuje lagodnie (x = prawo, y = przod, z Shiftem do 3). */
	FVector2D RuchGladki = FVector2D::ZeroVector;

	bool bRegion = false;
	float CelYaw = 0.f;
	/** Wysokosc ziemi pod punktem: 0 na mapie papierowej (plaska), teren na mapie grywalnej. */
	float Ziemia(double X, double Y) const;
	void Zoom(float Kroki);
	bool PunktPodKursorem(FVector& Out) const;
	void UstawKamere();
	void UstawPiksele();
	bool bPiksele = false;
	bool bPikseleUstawione = false;
	/** 0..1 w skali logarytmicznej odleglosci miedzy A i B (cm), wygladzone. */
	float Przejscie(float A, float B) const;

	FVector Focus = FVector::ZeroVector;
	float Yaw = 0.f;
	float Odl = 4.0e5f;
	float CelOdl = 4.0e5f;

	bool bZoomDoPunktu = false;
	FVector PunktZoomu = FVector::ZeroVector;

	bool bPrzeciaganie = false;
	FVector PunktChwytu = FVector::ZeroVector;
};
