// Plik: Source/grarts2d/UI/Ustawienia.h
// Ustawienia gracza (22.11.2026): klawisze, glosnosci, kamera, interfejs, wygladzanie i skala renderu.
// Grafika silnika (rozdzielczosc, okno, VSync, FPS, presety, cienie) siedzi w UGameUserSettings; reszta w tym samym
// pliku GameUserSettings.ini w sekcjach [Ustawienia], [Klawisze], [Dzwiek]. Menu: UI/MenuGlowne.*.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

struct FUstawienia
{
	/** Czynnosc do przypisania klawisza: nazwa w ini, opis w menu, grupa (naglowek w menu), klawisz domyslny. */
	struct FAkcja { FName Nazwa; const TCHAR* Opis; const TCHAR* Grupa; FKey Domyslny; };
	static const TArray<FAkcja>& Akcje();
	/** Klawisz przypisany czynnosci (np. "Budowa"); bez wpisu = domyslny. */
	static const FKey& Klawisz(FName Akcja);
	static void UstawKlawisz(FName Akcja, const FKey& K);
	static void DomyslneKlawisze();

	/** Glosnosc ogolna razy glosnosc kategorii EDzwiek (indeks). */
	static float Glosnosc(int32 Kategoria);
	static float GlosOgolna;
	static float GlosKat[8];

	// Kamera (mnozniki 1 = jak dotad).
	static float PrzesuwMn, ObrotMn, MyszMn, ZoomMn;
	static bool bOdwrocZoom, bOdwrocObrot;

	/** Wygladzanie / upscaler: "TSR", "DLSS", "FSR", "XeSS", "TAA", "FXAA", "Brak". */
	static FString Wygladzanie;
	/** Skala renderu w % (100 = pelna rozdzielczosc z TSR, domyslnie od 22.11.2026). */
	static float SkalaRenderu;
	static float SkalaUI;
	static FString Jezyk;

	/** Dostepne metody wygladzania (DLSS / FSR / XeSS tylko, gdy ich wtyczka jest w grze). */
	static TArray<FString> MetodyWygladzania();

	/** Start gry: czyta ini i stosuje wszystko (bez zmiany rozdzielczosci przy -zrzut). */
	static void Wczytaj();
	static void Zapisz();
	static void ZastosujObraz();
	static void ZastosujUI();
};
