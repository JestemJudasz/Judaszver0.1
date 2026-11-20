// Plik: Source/grarts2d/Swiat/MapaRzeki.cpp
#include "Swiat/MapaRzeki.h"
#include "Swiat/SwiatWysokosci.h"
#include "Kamera/KameraMapy.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AMapaRzeki::AMapaRzeki()
{
	PrimaryActorTick.bCanEverTick = true;
	Siatka = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Siatka"));
	RootComponent = Siatka;
	Siatka->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Siatka->SetCastShadow(false);
	// Material budowany skryptem Tools/ue_teren.py (Tools/Zbuduj_Teren.bat).
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Game/Swiat/Teren/M_MapaRzeki.M_MapaRzeki"));
	MaterialBazowy = Mat.Object;
}

void AMapaRzeki::BeginPlay()
{
	Super::BeginPlay();
	const TArray<FVector2D>* P = nullptr;
	const TArray<float>* Polowa = nullptr;
	const TArray<uint8>* Koniec = nullptr;
	FSwiat::Rzeki(P, Polowa, Koniec);

	// Wierzcholki parami na osi rzeki; material rozsuwa je na boki (normalna = bok, UV0 = (ranga, -1/+1)).
	TArray<FVector> V, N;
	TArray<FVector2D> UV;
	TArray<int32> T;
	const int32 Krok = FMath::Max(1, CoIle);
	for (int32 Od = 0; Od < P->Num();)
	{
		int32 Do = Od;
		while (Do < P->Num() - 1 && !(*Koniec)[Do])
		{
			++Do;
		}
		TArray<int32> Wybrane;
		for (int32 i = Od; i < Do; i += Krok)
		{
			Wybrane.Add(i);
		}
		Wybrane.Add(Do);

		int32 Poprzedni = INDEX_NONE;
		for (int32 k = 0; k < Wybrane.Num(); ++k)
		{
			const FVector2D Q = (*P)[Wybrane[k]];
			const FSwiat::EWoda W = FSwiat::Woda(Q.X, Q.Y);
			if (W == FSwiat::EWoda::Morze || W == FSwiat::EWoda::Jezioro)
			{
				Poprzedni = INDEX_NONE; // przez jeziora i morze bez wstegi (linie Natural Earth biegna tez srodkiem jezior)
				continue;
			}
			const FVector2D Kier = ((*P)[Wybrane[FMath::Min(k + 1, Wybrane.Num() - 1)]] - (*P)[Wybrane[FMath::Max(k - 1, 0)]]).GetSafeNormal();
			const FVector Bok(-Kier.Y, Kier.X, 0.0);
			const FVector Pkt(Q, FMath::Max(0.f, FSwiat::Wysokosc(Q.X, Q.Y)));
			const float Ranga = FMath::Clamp((*Polowa)[Wybrane[k]] / 2000.f, 0.3f, 1.f); // Tools/rzeki.py: 2000 cm = najwieksze
			const int32 I = V.Num();
			V.Append({ Pkt, Pkt });
			N.Append({ Bok, Bok });
			UV.Append({ FVector2D(Ranga, -1.0), FVector2D(Ranga, 1.0) });
			if (Poprzedni != INDEX_NONE)
			{
				T.Append({ Poprzedni, I, Poprzedni + 1, Poprzedni + 1, I, I + 1 });
			}
			Poprzedni = I;
		}
		Od = Do + 1;
	}
	Siatka->CreateMeshSection(0, V, T, N, UV, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	if (MaterialBazowy)
	{
		Material = UMaterialInstanceDynamic::Create(MaterialBazowy, this);
		Siatka->SetMaterial(0, Material);
	}
	Siatka->SetVisibility(false);
	UE_LOG(LogTemp, Log, TEXT("MapaRzeki: %d wierzcholkow, %d trojkatow."), V.Num(), T.Num() / 3);
}

void AMapaRzeki::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const AKameraMapy* Kam = PC ? Cast<AKameraMapy>(PC->GetPawn()) : nullptr;
	const float Mapa = Kam ? Kam->StrefaMapy() : 0.f;
	Siatka->SetVisibility(Mapa > 0.01f && Material);
	if (Mapa > 0.01f && Material)
	{
		Material->SetScalarParameterValue(TEXT("Mapa"), Mapa);
		Material->SetScalarParameterValue(TEXT("Szerokosc"), Kam->SzerokoscWidoku() * SzerokoscNaEkranie * 0.5f);
	}
}
