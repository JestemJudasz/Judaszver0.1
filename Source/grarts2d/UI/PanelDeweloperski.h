// Plik: Source/grarts2d/UI/PanelDeweloperski.h
// Tryb deweloperski / kreatywny (prosba uzytkownika 21.11.2026): panel w prawym dolnym rogu, F3 albo przycisk.
//   Budowa natychmiast: kazdy plac budowy od razu gotowy (ABudowa::bNatychmiast).
//   Czas gry: pauza, x1, x2, x5, x10 (globalne zwolnienie / przyspieszenie czasu swiata; kamera i sterowanie zostaja
//   w czasie rzeczywistym). W czasie bitwy czasem rzadzi ABitwa (pauza i zwolnienie przy zderzeniu), wiec tu x1.
//   Postaw wszystko: oboz, kazdy budynek gotowy, rodziny w kazdej pracy i +100 surowcow (ABudowniczy::PostawWszystko).
//   Surowce: +100 / +1000 kazdego towaru do obozu (AOsada::DodajSurowce) i tyle samo groszy.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"
#include "Styling/SlateBrush.h"

class AOsada;
class AActor;
class UWorld;

struct FTrybDeweloperski
{
	static bool bOtwarty;
	static bool bPauza;
	static float Predkosc;
	/** Na poczatek gry (BeginPlay ABudowniczy): wszystko wylaczone, czas x1. */
	static void Wyzeruj();
	/** Co klatke (ABudowniczy::Tick): ustawia czas swiata; Sterujacy (budowniczy) i kamera dostaja czas rzeczywisty. */
	static void Zastosuj(UWorld* W, AActor* Sterujacy, bool bBitwa);
	/** "x5", "pauza". */
	static FString NapisCzasu();
};

class SPanelDeweloperski : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPanelDeweloperski) {}
		SLATE_ARGUMENT(AOsada*, Osada)
		/** "Postaw wszystko" (ABudowniczy::PostawWszystko): zwraca podsumowanie do panelu. */
		SLATE_ARGUMENT(TFunction<FString()>, NaWszystko)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> Przycisk(const TAttribute<FText>& Napis, TFunction<void()> Akcja, const TAttribute<FSlateColor>& Kolor, const FString& Podpowiedz);
	TSharedRef<SWidget> PrzyciskCzasu(float P);
	TSharedRef<SWidget> Etykieta(const FString& T);
	void Dodaj(int32 Ile);

	TWeakObjectPtr<AOsada> Osada;
	TFunction<FString()> NaWszystko;
	FButtonStyle Styl;
	FSlateBrush Tlo;
	FString Wynik;
};
