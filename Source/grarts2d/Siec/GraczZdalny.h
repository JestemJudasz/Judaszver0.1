// Plik: Source/grarts2d/Siec/GraczZdalny.h
// Gra w sieci, krok M1 (26.11.2026): drugi gracz na tej samej mapie Europy. Kazdy gra swoja osada na swoim komputerze;
// co 2 s gry wysylamy przez FLacze stan osady (miejsce obozu, rodziny, woje, grosze, poziom, mury, ile budynkow kazdego
// rodzaju, zapasy), a osade drugiego gracza pokazujemy w terenie tak jak osade rodu AI (AOsadaRodu: te same modele,
// ludziki przy kamerze, choragiew w jego barwie). Do tego czat i dary: grosze ze skarbca albo towar z targu
// (towar laduje na targu kolegi; bez targu albo miejsca wraca do nadawcy).
// shortcut: osada kolegi to widok z liczb (jak rody AI), nie jego prawdziwe ludziki; wojny i handel cenami miedzy graczami
// to nastepne kroki (M2). Czas gry kazdy ma swoj (pauza i tempo nie sa wspolne).
// Panel "Gra w sieci" (lewa strona ekranu) jest widoczny, gdy lacze dziala; hostowanie i dolaczanie w menu (Esc / F10).
// Linia polecen: -host albo -dolacz=26.10.20.5 (bez menu, do sprawdzania na jednym komputerze: -dolacz=127.0.0.1).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Osada/Rody.h"
#include "GraczZdalny.generated.h"

class AOsada;
class AOsadaRodu;
class SWidget;

UCLASS()
class GRARTS2D_API AGraczZdalny : public AActor
{
	GENERATED_BODY()

public:
	AGraczZdalny();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Powod) override;
	virtual void Tick(float DeltaSeconds) override;

	void UstawOsade(AOsada* O);

	/** Wersja wiadomosci: inna u kolegi = rozlaczenie z komunikatem. */
	static constexpr int32 Wersja = 1;
	/** Osady blizej niz tyle cm nie pokazuja widoku kolegi (nachodzilby na nasza osade). */
	static constexpr float MinOdleglosc = 3.0e5f;

	/** Dla panelu: czy kolega przyslal juz stan, jego dane, odleglosc w km (-1 = nie wiadomo). */
	bool MaStan() const { return bMaStan; }
	bool KolegaMaOboz() const { return bKolegaOboz; }
	const FRod& Kolega() const { return Zdalny; }
	float OdlegloscKm() const;
	const TArray<FString>& Czat() const { return CzatL; }
	FString Komunikat() const;

	// --- Czynnosci z panelu ---
	void WyslijCzat(const FString& Tekst);
	void DajGrosze(int32 Ile);
	void DajTowar(ESurowiec S, int32 Ile);
	void PokazKolege();

	/** Imie gracza widoczne u kolegi (ustawienia gry, domyslnie nazwa uzytkownika Windows). */
	static FString MojeImie();
	static void UstawImie(const FString& Imie);

private:
	void Odbierz(const FString& Linia);
	void WyslijStan();
	void DopiszCzat(const FString& T);
	void Info(const FString& T);
	void OdswiezWidok();

	TWeakObjectPtr<AOsada> Osada;
	UPROPERTY() TObjectPtr<AOsadaRodu> Widok;
	FRod Zdalny;
	bool bMaStan = false;
	bool bKolegaOboz = false;
	bool bPrzywitany = false;
	bool bByloPolaczone = false;
	double DoStanu = 0.0;
	TArray<FString> CzatL;
	FString OstatniKomunikat;
	double CzasKomunikatu = -100.0;
	TSharedPtr<SWidget> Panel;
};
