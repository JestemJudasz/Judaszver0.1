// Plik: Source/grarts2d/UI/Minimapa.h
// Minimapa (P5, 26.11.2026): prawy dolny rog. Na mapie papierowej cala Europa, na mapie grywalnej ~6 km wokol kamery
// z budynkami i osadami rodow. Prostokat = to, co widac. Klik albo przeciagniecie przenosi kamere. M chowa/pokazuje.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Styling/SlateBrush.h"

class UTexture2D;
class AKameraMapy;

class SMinimapa : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SMinimapa) {}
		SLATE_ARGUMENT(UWorld*, Swiat)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Bok, Bok); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Clip, FSlateWindowElementList& Out,
		int32 Warstwa, const FWidgetStyle& Styl, bool bRodzic) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& E) override;
	virtual FReply OnMouseButtonUp(const FGeometry& Geo, const FPointerEvent& E) override;
	virtual FReply OnMouseMove(const FGeometry& Geo, const FPointerEvent& E) override;

	/** Bok minimapy na ekranie (px). */
	static constexpr float Bok = 210.f;
	/** Zakres mapy grywalnej (cm) - kwadrat wokol kamery. */
	static constexpr double ZakresRegionu = 6.0e5;

private:
	AKameraMapy* Kamera() const;
	/** Srodek i bok pokazanego kwadratu swiata (cm) dla obecnego trybu. */
	void Zakres(FVector2D& Srodek, double& BokSwiata) const;
	FVector2D NaEkran(FVector2D Swiat, FVector2D Srodek, double BokSwiata) const;
	void Przenies(const FGeometry& Geo, const FPointerEvent& E);
	/** Przelicza obraz terenu (Wysokosc + Woda + Pokrycie) do tekstury N x N. */
	void Odrysuj(FVector2D Srodek, double BokSwiata, bool bRegion) const;

	TWeakObjectPtr<UWorld> Swiat;
	mutable TStrongObjectPtr<UTexture2D> Obraz;
	mutable FSlateBrush Pedzel;
	mutable FVector2D SrodekObrazu = FVector2D(1e30, 1e30);
	mutable double BokObrazu = 0.0;
	mutable bool bObrazRegionu = false;
	mutable double CzasObrazu = -100.0;
	bool bCiagnie = false;
};
