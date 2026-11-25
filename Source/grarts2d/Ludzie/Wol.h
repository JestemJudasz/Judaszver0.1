// Plik: Source/grarts2d/Ludzie/Wol.h
// Wol (23.11.2026, prosba uzytkownika: jak w Manor Lords woly pomagaja przy klodach, bez kupowania).
// Osada (AOsada::RozwozWolami) daje wolnemu wolowi kurs: klody ze zrodla (chata drwala, magazyn) na budowe albo
// do magazynu. Wol idzie po klody, czeka chwile przy zaladunku, ciagnie je po ziemi (SM_Ludz_WolKlody, do 6 sztuk)
// i zostawia na miejscu. Bez kursu pasie sie przy swojej zagrodzie (na starcie przy obozie).
// Czesci z Tools/ludzie.py (SM_Ludz_WolTulow, SM_Ludz_WolNoga, SM_Ludz_WolKlody), skala x1,4 jak osadnicy.
// Kon (23.11.2026): ta sama klasa z bKon - ze stajni, ciagnie woz (SM_Ludz_Woz) z deskami albo kamieniem (do 8 sztuk),
// szybciej niz wol. Czesci SM_Ludz_KonTulow, SM_Ludz_KonNoga, ladunek SM_Ludz_WozDeski / SM_Ludz_WozKamien.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Osada/Surowce.h"
#include "Wol.generated.h"

class ABudowa;
class UStaticMeshComponent;

UCLASS()
class GRARTS2D_API AWol : public AActor
{
	GENERATED_BODY()

public:
	AWol();
	virtual void Tick(float DeltaSeconds) override;

	/** Kon z wozem zamiast wolu (wolac przed UstawDom). */
	void UstawKonia() { bKon = true; }
	bool Kon() const { return bKon; }
	int32 Udzwig() const { return bKon ? 8 : 6; }
	/** Gdzie sie pasie, gdy nie ma kursu (wybieg zagrody albo oboz). */
	void UstawDom(FVector2D Gdzie, float Promien);
	bool Wolny() const { return Krok == EKrok::Pasie; }
	/** Kurs: Ile sztuk S z Z do CelB (budowa albo magazyn). Rezerwuje towar w Z i miejsce w CelB. */
	void Ciagnij(ABudowa* Z, ABudowa* CelB, int32 Ile, ESurowiec S);
	/** Zagroda, ktora dala tego wola (nullptr = jeden z dwoch wolow startowych). */
	TWeakObjectPtr<ABudowa> Zagroda;

private:
	enum class EKrok : uint8 { Pasie, DoZrodla, Laduje, Ciagnie, Zrzuca };

	UStaticMeshComponent* Czesc(const TCHAR* Nazwa, USceneComponent* Rodzic, FVector Staw);
	void Zaladuj();
	void IdzDo(FVector2D Gdzie);
	void Dotarl();
	void Porzuc(); // zrodlo albo cel zniknely: oddaje rezerwacje
	FVector2D MiejsceZrzutu() const;
	float Predkosc() const;

	UPROPERTY() TObjectPtr<USceneComponent> Korzen;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Tulow;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Klody; // ladunek: klody wolu albo towar na wozie
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Woz;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Nogi; // LP, PP, LT, PT

	TWeakObjectPtr<ABudowa> Zrodlo;
	TWeakObjectPtr<ABudowa> Cel;
	int32 Ladunek = 0;
	bool bMaKlody = false; // ciagnie / wiezie towar
	bool bKon = false;
	ESurowiec Towar = ESurowiec::Kloda;
	bool bZaladowany = false;
	EKrok Krok = EKrok::Pasie;
	TArray<FVector2D> Trasa;
	int32 TrasaI = 0;
	bool bIdzie = false;
	float Czekaj = 1.f;
	float Faza = 0.f;      // cykl chodu
	float WagaChodu = 0.f; // 0 stoi, 1 idzie
	float Kierunek = 0.f;  // yaw w stopniach
	FVector2D Dom = FVector2D::ZeroVector;
	float PromienDomu = 300.f;
};
