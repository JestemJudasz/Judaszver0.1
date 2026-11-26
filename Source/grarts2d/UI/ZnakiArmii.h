// Plik: Source/grarts2d/UI/ZnakiArmii.h
// Armie w marszu i baner wojny (P6, 26.11.2026, uwagi z gry: "ma byc widac na mapie herb wojska, kiedy i gdzie sie
// porusza", "mamy widziec, czy ktos wypowiedzial nam wojne"). Na mapie papierowej i grywalnej: choragiew z herbem
// rodu (albo wasza) w miejscu kolumny, przerywana linia do celu, ilu ludzi i dokad. Baner "X wypowiadaja wam wojne!"
// na gorze ekranu przez kilka sekund.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class ARody;
class APlayerController;

class SZnakiArmii : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SZnakiArmii) {}
		SLATE_ARGUMENT(TWeakObjectPtr<ARody>, Rody)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs) { RodyW = InArgs._Rody; }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Clip, FSlateWindowElementList& Out,
		int32 Warstwa, const FWidgetStyle& Styl, bool bRodzic) const override;

	/** Id armii idacej na gracza (w czasie wojny), w ktorej choragiew kliknieto (Mysz = piksele ekranu); -1 = zadna. */
	static int32 ArmiaPod(ARody* A, APlayerController* PC, FVector2D Mysz);

private:
	TWeakObjectPtr<ARody> RodyW;
};
