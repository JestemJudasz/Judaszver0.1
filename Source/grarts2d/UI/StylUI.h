// Plik: Source/grarts2d/UI/StylUI.h
// Wspolna paleta interfejsu (ciemne drewno, przygaszone zloto - jak pasek budowy) i style przyciskow.
#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/SlateTypes.h"

namespace StylUI
{
	inline const FLinearColor Tlo(0.035f, 0.028f, 0.02f, 0.88f);
	inline const FLinearColor Ramka(0.42f, 0.32f, 0.18f, 1.f);
	inline const FLinearColor Przycisk(0.10f, 0.08f, 0.055f, 0.95f);
	inline const FLinearColor PrzyciskNad(0.20f, 0.15f, 0.09f, 1.f);
	inline const FLinearColor Zloto(0.85f, 0.66f, 0.32f, 1.f);
	inline const FLinearColor Napis(0.95f, 0.90f, 0.78f, 1.f);
	inline const FLinearColor Szary(0.62f, 0.58f, 0.50f, 1.f);
	inline const FLinearColor Czerwony(0.90f, 0.42f, 0.32f, 1.f);
	inline const FLinearColor Zielony(0.58f, 0.78f, 0.42f, 1.f);
	inline const FLinearColor Kafel(0.07f, 0.055f, 0.04f, 0.95f);

	inline FButtonStyle StylPrzycisku()
	{
		return FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(Przycisk, 4.f, Ramka * 0.8f, 1.f))
			.SetHovered(FSlateRoundedBoxBrush(PrzyciskNad, 4.f, Zloto, 1.f))
			.SetPressed(FSlateRoundedBoxBrush(Przycisk * 0.7f, 4.f, Zloto, 1.f))
			.SetDisabled(FSlateRoundedBoxBrush(Przycisk * 0.6f, 4.f, Ramka * 0.4f, 1.f))
			.SetNormalPadding(FMargin(6.f, 5.f))
			.SetPressedPadding(FMargin(6.f, 6.f, 6.f, 4.f));
	}

	/** Pedzel ikony z tekstury (piksele ostre: tekstury UI maja filtr Nearest). */
	inline FSlateBrush Ikona(UObject* Tekstura, float Rozmiar)
	{
		FSlateBrush B;
		B.SetResourceObject(Tekstura);
		B.ImageSize = FVector2f(Rozmiar, Rozmiar);
		B.DrawAs = Tekstura ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
		return B;
	}
}
