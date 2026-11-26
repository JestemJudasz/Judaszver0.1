// Plik: Source/grarts2d/Swiat/SwiatGameMode.h
// Tryb gry: wczytuje dane swiata i stawia wszystko w pustej mapie (teren, slonce, niebo). Pawn = KameraMapy.
// Ekran ladowania zaslania swiat, dopoki teren z drzewami nie jest gotowy; potem w rogu napis "Doczytywanie..."
// zawsze, gdy cos sie jeszcze dociaga (zeby ogladac i oceniac tylko gotowy obraz).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SwiatGameMode.generated.h"

class ATerenKafle;
class ADirectionalLight;
class ASkyLight;
class APostProcessVolume;
class SWidget;
class SBorder;
class STextBlock;

UCLASS()
class GRARTS2D_API ASwiatGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASwiatGameMode();
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Mapa grywalna wokol punktu (wola kamera przy przyblizeniu; ekran ladowania do gotowosci). */
	void WejdzDoRegionu(FVector2D Gdzie);
	/** Powrot do mapy Europy (wola kamera przy oddaleniu). */
	void WyjdzZRegionu();

private:
	void PokazEkranLadowania();
	void Zaslon(const TCHAR* Tekst);
	void PoWczytaniu();
	void UstawSwiatloRegionu(bool bRegion);

	TWeakObjectPtr<ADirectionalLight> Slonce;
	TWeakObjectPtr<ASkyLight> SwiatloNieba;
	TWeakObjectPtr<APostProcessVolume> Korekta;

	FString TekstLadowania = TEXT("Wczytywanie świata");
	bool bZrzutZaplanowany = false;

	TWeakObjectPtr<ATerenKafle> Teren;
	TSharedPtr<SWidget> Nakladka;
	TSharedPtr<SWidget> Minimapa;
	bool bMinimapa = true;
	bool bBylWczytany = false;
	TSharedPtr<SBorder> Tlo;
	TSharedPtr<STextBlock> NapisDuzy;
	TSharedPtr<STextBlock> NapisMaly;
	bool bWczytano = false;
	float CzasGotowy = 0.f;
};
