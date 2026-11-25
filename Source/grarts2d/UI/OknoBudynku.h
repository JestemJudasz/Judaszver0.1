// Plik: Source/grarts2d/UI/OknoBudynku.h
// Okno budynku (prosba uzytkownika 21.11.2026: "ladne gui do przypisywania ludzi pracujacych w tych domach / pracach").
// Otwiera sie po kliknieciu budynku (ABudowniczy). Pokazuje:
//   - nazwe, ikone i opis,
//   - budowe: postep i material (ile przyniesiono z kosztu),
//   - budynek pracy: kafelki miejsc pracy (rodzina + co teraz robi, albo puste miejsce), przyciski "+ rodzina" / "- rodzina"
//     i licznik rodzin bez pracy,
//   - zapasy przy budynku (stosy), mieszkancow domu / namiotu, mlode drzewa borowego,
//   - etap 3: stan pola (orka / siew w procentach, snopy do zebrania) i przerobke mlyna, piekarni, tartaku wodnego.
// Wszystko odswiezane na biezaco (lambdy), wiec okno moze zostac otwarte, gdy budowa sie konczy.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"
#include "Osada/Surowce.h"

class ABudowa;
class AOsada;
class UTexture2D;

class SOknoBudynku : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOknoBudynku) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AOsada>, Osada)
		SLATE_ARGUMENT(TWeakObjectPtr<ABudowa>, Budynek)
		/** Ikona budynku z paska (moze byc nullptr: namiot, ognisko). */
		SLATE_ARGUMENT(UTexture2D*, Ikona)
		/** Ikony surowcow w kolejnosci ESurowiec. */
		SLATE_ARGUMENT(TArray<UTexture2D*>, IkonySurowcow)
		SLATE_ARGUMENT(UTexture2D*, IkonaRodziny)
		SLATE_EVENT(FSimpleDelegate, NaZamknij)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	ABudowa* Budynek() const { return BudynekW.Get(); }

private:
	TSharedRef<SWidget> Naglowek();
	TSharedRef<SWidget> CzescBudowy();
	TSharedRef<SWidget> CzescPracy();
	TSharedRef<SWidget> CzescZapasow();
	TSharedRef<SWidget> Kafelek(int32 Miejsce);
	/** Poziomy (22.11.2026): poziom domu / kosciola, koszt nastepnego i przycisk "Ulepsz". */
	TSharedRef<SWidget> CzescUlepszenia();
	/** 23.11.2026: przycisk "Rozbierz" (drugi klik potwierdza). */
	TSharedRef<SWidget> CzescRozbiorki();
	bool bPotwierdzRozbiorke = false;
	/** Wojna (23.11.2026): wytrzymalosc i ogien; ruina: Odbuduj / Uprzatnij. */
	TSharedRef<SWidget> CzescStanu();
	TSharedRef<SWidget> CzescRuiny();
	FProgressBarStyle StylPaskaStanu;

	FText TytulStanu() const;
	FReply Dodaj();
	FReply Usun();
	FReply Ulepsz();

	TWeakObjectPtr<AOsada> OsadaW;
	TWeakObjectPtr<ABudowa> BudynekW;
	FSimpleDelegate NaZamknij;

	FSlateBrush TloOkna;
	FSlateBrush TloKafla;
	FSlateBrush TloKaflaPuste;
	FSlateBrush Kreska;
	FSlateBrush IkonaB;
	FSlateBrush IkonaRodzinyB;
	FSlateBrush IkonySurowcowB[int32(ESurowiec::Ile)];
	FButtonStyle StylPrzycisku;
	FProgressBarStyle StylPaska;
};
