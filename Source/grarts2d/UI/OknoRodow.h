// Plik: Source/grarts2d/UI/OknoRodow.h
// Ekonomia, etap 5 (21.11.2026, plan "Ekonomia gry"): okno rodow (klawisz H albo przycisk pod panelem osady).
//   Lewa kolumna: rody w okolicy (barwa, nazwa, cecha, poziom osady, rodziny, odleglosc, stosunki, lennik / wrogi / zada daniny)
//   i wasza osada (poziom, sila). Klik w rod wybiera go.
//   Prawa kolumna: wybrany rod (sila, grosze, stosunki, co ostatnio robi), dyplomacja (podarunek, zadanie daniny,
//   odpowiedz na ich zadanie) i handel na targu: 10 towarow, kup / sprzedaj po 5 sztuk za grosze.
// SZnakiRodow: na mapie papierowej (poza mapa grywalna) choragiewki rodow w ich miejscach i wasza osada.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"
#include "Osada/Surowce.h"

class ARody;
class AOsada;
class UTexture2D;
struct FRod;

class SOknoRodow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOknoRodow) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ARody>, Rody)
		SLATE_ARGUMENT(TWeakObjectPtr<AOsada>, Osada)
		/** Ikony surowcow w kolejnosci ESurowiec. */
		SLATE_ARGUMENT(TArray<UTexture2D*>, IkonySurowcow)
		SLATE_ARGUMENT(UTexture2D*, IkonaGroszy)
		SLATE_EVENT(FSimpleDelegate, NaZamknij)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void Wybierz(int32 Indeks) { Wybrany = Indeks; }
	/** Napis na dole okna (wynik handlu, dyplomacji). */
	void UstawStatus(const FString& T, bool bDobrze);

private:
	TSharedRef<SWidget> Naglowek();
	TSharedRef<SWidget> ListaRodow();
	TSharedRef<SWidget> WierszRodu(int32 I);
	TSharedRef<SWidget> CzescRodu();
	TSharedRef<SWidget> Dyplomacja();
	TSharedRef<SWidget> Handel();
	TSharedRef<SWidget> WierszTowaru(ESurowiec S);

	const FRod* Rod(int32 I) const;
	const FRod* Rod() const { return Rod(Wybrany); }
	bool JestTarg() const;
	void Wynik(bool bOk, const FString& Powod);

	TWeakObjectPtr<ARody> RodyW;
	TWeakObjectPtr<AOsada> OsadaW;
	FSimpleDelegate NaZamknij;
	int32 Wybrany = 0;
	FString Status;
	bool bStatusDobry = true;

	FSlateBrush TloOkna;
	FSlateBrush TloKafla;
	FSlateBrush TloWybranego;
	FSlateBrush Kreska;
	FSlateBrush Barwa;
	FSlateBrush IkonaGroszyB;
	FSlateBrush IkonySurowcowB[int32(ESurowiec::Ile)];
	FButtonStyle StylPrzycisku;
	FButtonStyle StylWiersza;
};

/** Choragiewki rodow na mapie papierowej (rzut ich miejsca na ekran co klatke). */
class SZnakiRodow : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SZnakiRodow) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ARody>, Rody)
		SLATE_ARGUMENT(TWeakObjectPtr<AOsada>, Osada)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	TWeakObjectPtr<ARody> RodyW;
	TWeakObjectPtr<AOsada> OsadaW;
	FSlateBrush Znak;
	FSlateBrush Tlo;
};
