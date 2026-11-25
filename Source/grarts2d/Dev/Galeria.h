// Plik: Source/grarts2d/Dev/Galeria.h
// Galerie testowe (24.11.2026, prosba uzytkownika: "po jakiejs komendzie stawia sie wszystko co mamy w grze ... a po
// jeszcze innym kazda jednostka i mamy ja w kazdej animacji"):
//   - F5: galeria budynkow - kazdy model z /Game/Swiat/Budynki w rzedach (etapy budowy F1.., warianty W, przebudowa U,
//     uszkodzony Z1, ruina Z2), mury, bramy, pola, natura; obok plonacy dom i co 12 s zawalenie (efekty ognia i gruzu);
//   - F6: galeria wojska - kazdy rodzaj wojska w rzedzie, w kolumnach kazda animacja walki (stoi, idzie, biegnie, ranny,
//     cios / strzal, blok, trafiony, napor, okrzyk, pada, sciety...), animacje zapetlone; rzad narzedzi ruszenia i kobieta.
// Ponowne nacisniecie zamyka galerie. Napisy nad modelami (Slate, polskie litery). -galeria / -galeriawojska: zrzut.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Galeria.generated.h"

class ALudzik;
class SWidget;

UCLASS()
class GRARTS2D_API AGaleria : public AActor
{
	GENERATED_BODY()

public:
	enum class ERodzaj : uint8 { Budynki, Wojsko };

	AGaleria();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Wlacza galerie R przy kamerze (inna galeria znika); druga taka sama = zamkniecie. Zwraca opis do panelu. */
	static FString Przelacz(UWorld* W, ERodzaj R);

	struct FNapis { FVector P; FString T; bool bGlowny; };
	const TArray<FNapis>& Napisy() const { return NapisyL; }

private:
	ERodzaj Rodzaj = ERodzaj::Budynki;
	FVector2D Start = FVector2D::ZeroVector; // lewy przedni rog galerii
	FVector2D Prawo = FVector2D(1.f, 0.f);   // kolumny (w prawo na ekranie)
	FVector2D Wglab = FVector2D(0.f, 1.f);   // rzedy (od kamery)
	TArray<FNapis> NapisyL;
	TSharedPtr<SWidget> Nakladka;

	FString PostawBudynki();
	FString PostawWojsko();

	// Budynki: efekty.
	FVector PozarS = FVector::ZeroVector, ZawalS = FVector::ZeroVector;
	FVector2D PozarPol = FVector2D(300.f, 300.f);
	float ZawalZa = 3.f;

	// Wojsko: jedna komorka = jeden woj w jednej animacji, zapetlonej.
	struct FKomorka
	{
		TWeakObjectPtr<ALudzik> L;
		int32 Rzad = 0, Kol = 0;
		FVector2D P = FVector2D::ZeroVector;
		float Za = 0.f;     // s do nastepnego powtorzenia
		bool bTam = false;  // chod tam i z powrotem
	};
	TArray<FKomorka> Komorki;
	void Ozyw(FKomorka& K);
	void Animuj(FKomorka& K, float Dt);
};
