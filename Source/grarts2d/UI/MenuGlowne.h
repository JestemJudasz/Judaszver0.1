// Plik: Source/grarts2d/UI/MenuGlowne.h
// Menu glowne (22.11.2026): przy starcie gry (Nowa gra, Wczytaj, Ustawienia, Wyjscie) i w grze pod Esc / F10
// (Kontynuuj, gra stoi w pauzie). Ustawienia w zakladkach: Grafika, Dzwiek, Sterowanie, Kamera, Interfejs.
// Wartosci i zapis: UI/Ustawienia.*. Otwarte menu zabiera mysz i klawiature grze (FInputModeUIOnly).
// -zrzut pomija menu startowe, chyba ze jest tez -menu (zrzut kontrolny samego menu).
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"

class SBox;
class UWorld;

class SMenuGlowne : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMenuGlowne) {}
		/** true = menu startowe (Nowa gra, bez pauzy), false = menu w grze (Kontynuuj, pauza). */
		SLATE_ARGUMENT(bool, Start)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	static void Otworz(UWorld* Swiat, bool bStart);
	static void Zamknij();
	static bool Otwarte();

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& G, const FKeyEvent& E) override;
	virtual FReply OnPreviewKeyDown(const FGeometry& G, const FKeyEvent& E) override;

private:
	void PokazGlowne();
	void PokazUstawienia(int32 Zakladka);
	/** Zapis gry (P4): lista slotow do zapisu (bZapis) albo wczytania. */
	void PokazSloty(bool bZapis);
	/** Gra w sieci (M1, 26.11.2026): imie, adresy tego komputera, Hostuj, Dolacz po adresie IP, Rozlacz. */
	void PokazSiec();
	FString StatusSieci;
	FString StatusSlotow;
	TSharedRef<SWidget> Grafika();
	TSharedRef<SWidget> Dzwiek();
	TSharedRef<SWidget> Sterowanie();
	TSharedRef<SWidget> Kamera();
	TSharedRef<SWidget> Interfejs();

	TSharedRef<SWidget> Guzik(const FString& Tekst, TFunction<void()> Akcja, bool bAktywny = true, const FString& Podp = FString());
	TSharedRef<SWidget> Naglowek(const FString& Tekst);
	TSharedRef<SWidget> Uwaga(const FString& Tekst);
	TSharedRef<SWidget> Wiersz(const FString& Nazwa, TSharedRef<SWidget> Wartosc, const FString& Podp = FString());
	/** Wybor z listy strzalkami < >; Indeks -1 = "Własne". */
	TSharedRef<SWidget> Wybor(const FString& Nazwa, TFunction<TArray<FString>()> Opcje, TFunction<int32()> Indeks,
		TFunction<void(int32)> Ustaw, const FString& Podp = FString());
	TSharedRef<SWidget> Suwak(const FString& Nazwa, float Min, float Max, float Krok, TFunction<float()> Wartosc,
		TFunction<void(float)> Ustaw, TFunction<FString(float)> Napis);

	bool bStart = true;
	int32 Zakladka = -1; // -1 = ekran glowny
	FName CzekaNa;       // czynnosc czekajaca na nowy klawisz
	TSharedPtr<SBox> Miejsce;
	FButtonStyle Styl;
	FButtonStyle StylWybranej;
};
