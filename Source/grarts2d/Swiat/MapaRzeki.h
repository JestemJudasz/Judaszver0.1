// Plik: Source/grarts2d/Swiat/MapaRzeki.h
// Rzeki mapy strategicznej (pergamin): wstegi z linii Data/rzeki.bin, blekitne z ciemnym obrysem, zawsze ostre
// i o stalej szerokosci na ekranie (M_MapaRzeki rozsuwa wierzcholki osi o parametr Szerokosc). Widoczne tylko poza
// regionem, krycie = AKameraMapy::StrefaMapy().
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapaRzeki.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class GRARTS2D_API AMapaRzeki : public AActor
{
	GENERATED_BODY()

public:
	AMapaRzeki();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Najszersza rzeka na ekranie: ulamek szerokosci widoku (0,0025 = ~5 px przy 1920 px). Wezsze wg rangi. */
	UPROPERTY(EditAnywhere, Category = "Mapa")
	float SzerokoscNaEkranie = 0.0025f;

	/** Co ile punktow linii (punkty co 20 m) brac wierzcholek wstegi: 5 = co 100 m. */
	UPROPERTY(EditAnywhere, Category = "Mapa")
	int32 CoIle = 5;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProceduralMeshComponent> Siatka;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> MaterialBazowy;
};
