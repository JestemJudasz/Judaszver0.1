// Plik: Source/grarts2d/UI/PasekBudowy.h
// Dolny pasek (21.11.2026, uklad jak w Manor Lords, wyglad nasz): na samym dole rzad glownych przyciskow ze znakami
// (Budowa - B, Wojsko - J, Rody i handel - H); Budowa otwiera nad nim pasek kategorii (znaki: dom, siekiera, chleb, snop,
// kowadlo, tarcza, waga, krzyz), a nad paskiem karty budynkow wybranej kategorii: pergamin z obrazkiem budynku, nazwa,
// koszt i znak kategorii w kolku u dolu. Klik w karte -> ABudowniczy::Wybierz (podglad pod kursorem).
// Kategorie i budynki biora sie z FRodzajBudynku::DoBudowania(), wiec nowy budynek wystarczy tam dopisac.
// Znaki: Tools/ikony.py -> T_Ikona_Znak_<Nazwa> (lista: SPasekBudowy::NazwyZnakow).
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"

struct FRodzajBudynku;
class SHorizontalBox;
class SBox;
class UTexture2D;

DECLARE_DELEGATE_OneParam(FNaWyborBudynku, const FRodzajBudynku*);
/** Paczka A: czemu nie wolno stawiac (pusty = wolno), np. "Wymaga poziomu osady: wieś". */
DECLARE_DELEGATE_RetVal_OneParam(FString, FCzemuZablokowany, const FRodzajBudynku&);

class SPasekBudowy : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPasekBudowy) {}
		/** Klik w budynek (nullptr = odznacz). */
		SLATE_EVENT(FNaWyborBudynku, NaWybor)
		SLATE_EVENT(FCzemuZablokowany, Blokada)
		/** Aktualnie stawiany budynek (podswietlenie karty). */
		SLATE_ATTRIBUTE(const FRodzajBudynku*, Wybrany)
		/** Ikony w kolejnosci FRodzajBudynku::DoBudowania() (nullptr = bez ikony); trzymane przez wlasciciela (UPROPERTY). */
		SLATE_ARGUMENT(TArray<UTexture2D*>, Ikony)
		/** Znaki przyciskow i kategorii po nazwie z NazwyZnakow(); trzymane przez wlasciciela (UPROPERTY). */
		SLATE_ARGUMENT(TArray<UTexture2D*>, Znaki)
		/** Glowne przyciski: Wojsko (J) i Rody i handel (H) - otworz / zamknij okno. */
		SLATE_EVENT(FSimpleDelegate, NaWojsko)
		SLATE_EVENT(FSimpleDelegate, NaRody)
		/** Drogi (G, 21.11.2026): wlacz / wylacz rysowanie drog; znak T_Ikona_Znak_Droga (Tools/Zbuduj_Drogi.bat). */
		SLATE_EVENT(FSimpleDelegate, NaDrogi)
		SLATE_ATTRIBUTE(bool, DrogiOtwarte)
		SLATE_ARGUMENT(UTexture2D*, ZnakDrogi)
		SLATE_ATTRIBUTE(bool, WojskoOtwarte)
		SLATE_ATTRIBUTE(bool, RodyOtwarte)
		/** Niepusty = czerwony znak "!" na przycisku i podpowiedz (np. "bandyci!", "żądanie daniny!"). */
		SLATE_ATTRIBUTE(FString, AlarmWojska)
		SLATE_ATTRIBUTE(FString, AlarmRodow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Otwiera Budowe na tej kategorii (pusty = zamyka pasek kategorii i karty). */
	void OtworzKategorie(const FString& Kategoria);
	bool BudowaOtwarta() const { return bBudowa; }
	/** Klawisz B / przycisk Budowa: otworz (ostatnia albo pierwsza kategoria) albo zamknij. */
	void PrzelaczBudowe();

	/** Nazwy znakow w kolejnosci argumentu Znaki (tekstury T_Ikona_Znak_<Nazwa>). */
	static const TArray<FString>& NazwyZnakow();

private:
	void ZbudujRzad();
	FReply KlikKategorii(FString Kategoria);
	FReply KlikBudynku(const FRodzajBudynku* R);
	const FSlateBrush* Znak(const FString& Nazwa) const;
	TSharedRef<SWidget> GlownyPrzycisk(const FString& NazwaZnaku, const FString& Napis, const FString& Podpowiedz,
		TAttribute<bool> Otwarte, TAttribute<FString> Alarm, FSimpleDelegate Klik);

	FNaWyborBudynku NaWybor;
	FCzemuZablokowany Blokada;
	TAttribute<const FRodzajBudynku*> Wybrany;
	TArray<FString> Kategorie;
	TArray<TSharedPtr<FSlateBrush>> Ikony; // rownolegle do DoBudowania()
	TMap<FString, TSharedPtr<FSlateBrush>> Znaki;
	FString Otwarta;
	FString Ostatnia;
	bool bBudowa = false;

	FSimpleDelegate NaWojsko;
	FSimpleDelegate NaRody;
	FSimpleDelegate NaDrogi;
	TAttribute<bool> DrogiOtwarte;
	TAttribute<bool> WojskoOtwarte;
	TAttribute<bool> RodyOtwarte;
	TAttribute<FString> AlarmWojska;
	TAttribute<FString> AlarmRodow;

	TSharedPtr<SHorizontalBox> Rzad;

	FButtonStyle StylGlowny;
	FButtonStyle StylKategorii;
	FButtonStyle StylKategoriiWybranej;
	FButtonStyle StylKarty;
	FSlateBrush TloPaska;
	FSlateBrush RamkaWybranej;
	FSlateBrush Kolko;
	FSlateBrush TloAlarmu;
};
