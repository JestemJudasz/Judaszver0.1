// Plik: Source/grarts2d/UI/Herb.h
// Herby rodow (P6, 26.11.2026, uwaga z gry: "ma byc widac na mapie herb wojska"): tarcza z podzialem w dwoch barwach,
// wzor i druga barwa z nazwy rodu (zawsze te same), pole w barwie rodu. Rysowana w Slate pasami (bez tekstur).
#pragma once

#include "CoreMinimal.h"

class FSlateWindowElementList;
struct FGeometry;

struct FHerb
{
	uint8 Wzor = 0;           // 0 w slup, 1 w pas, 2 krzyz, 3 skos, 4 pas, 5 slup, 6 krokiew, 7 w czwor
	FLinearColor Pole = FLinearColor::Red;
	FLinearColor Godlo = FLinearColor::White;

	static FHerb Rodu(const FString& Nazwa, FLinearColor Barwa);
	static FHerb Gracza();
	/** Tarcza: lewy gorny rog P, szerokosc S (wysokosc 1,2 S), z ciemnym obrysem. */
	void Rysuj(FSlateWindowElementList& Out, int32 Warstwa, const FGeometry& G, FVector2f P, float S, float Alfa = 1.f) const;
	/** Choragiew na drzewcu z herbem i napisem pod spodem (armia w marszu). Stopa = punkt na ziemi (ekran). */
	void Choragiew(FSlateWindowElementList& Out, int32 Warstwa, const FGeometry& G, FVector2f Stopa, float S, const FString& Napis,
		const FString& Podpis, FLinearColor KolorPodpisu) const;
};
