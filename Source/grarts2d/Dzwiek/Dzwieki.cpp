// Plik: Source/grarts2d/Dzwiek/Dzwieki.cpp
#include "Dzwiek/Dzwieki.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kamera/KameraMapy.h"
#include "UI/Ustawienia.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"

namespace
{
	// Na kategorie: glosnosc, limit glosow naraz, czy gasnie z oddaleniem kamery.
	struct FKat { float Glosnosc; int32 Limit; bool bOdZoomu; };
	const FKat Kat[(int32)EDzwiek::Ile] = {
		// 23.11.2026 (uzytkownik: walka za glosna i trzeszczy): mniej glosow naraz i ciszej, zeby suma nie przesterowala.
		{ 0.45f, 8, true },  // Walka: szczek, tarcze, trafienia
		{ 0.6f,  8, true },  // Praca: siekiery, kilofy, pily, kowal
		{ 0.7f,  6, true },  // Glosy: krzyki, okrzyki, jeki
		{ 0.7f,  6, true },  // Osada: rozmowy, zwierzeta
		{ 1.0f,  1, false }, // Rozkazy: kwestie oddzialow (nowa przerywa poprzednia)
	};
	constexpr float PelnaDo = 1500.f;     // cm od ogniska: pelna glosnosc
	constexpr float Wygasanie = 25000.f;  // cm dalej: do ciszy
	constexpr float BliskoOd = 40000.f;   // oddalenie kamery (cm): ponizej wszystko slychac
	constexpr float BliskoDo = 140000.f;  // powyzej pojedyncze dzwieki milkna (max regionu 150000)
}

UDzwieki* UDzwieki::Z(const UObject* Kontekst)
{
	UWorld* W = Kontekst ? Kontekst->GetWorld() : nullptr;
	return W ? W->GetSubsystem<UDzwieki>() : nullptr;
}

void UDzwieki::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	if (!FPlatformProperties::RequiresCookedData())
		AR.ScanPathsSynchronous({ TEXT("/Game/Dzwieki") }); // -game z edytora: rejestr moze jeszcze skanowac
	TArray<FAssetData> Pliki;
	AR.GetAssetsByPath(FName(TEXT("/Game/Dzwieki")), Pliki, true);
	for (const FAssetData& A : Pliki)
	{
		if (!A.IsInstanceOf(USoundBase::StaticClass())) continue;
		// "Miecz_Tarcza_03" -> grupa "Miecz_Tarcza"
		FString N = A.AssetName.ToString();
		int32 i = N.Len();
		while (i > 0 && FChar::IsDigit(N[i - 1])) --i;
		if (i < N.Len() && i > 1 && N[i - 1] == '_') N.LeftInline(i - 1);
		FGrupa& G = Grupy.FindOrAdd(FName(*N));
		G.Pliki.Add(A.GetSoftObjectPath());
		G.Snd.Add(nullptr);
	}
	UE_LOG(LogTemp, Log, TEXT("Dzwieki: %d plikow w %d grupach"), Pliki.Num(), Grupy.Num());

	Tlumienie = NewObject<USoundAttenuation>(this);
	FSoundAttenuationSettings& S = Tlumienie->Attenuation;
	S.bAttenuate = true;
	S.bSpatialize = true;
	S.AttenuationShape = EAttenuationShape::Sphere;
	S.AttenuationShapeExtents = FVector(PelnaDo, 0.f, 0.f);
	S.FalloffDistance = Wygasanie;
	S.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	S.dBAttenuationAtMax = -40.f;
	// 23.11.2026 (uzytkownik: dzwiek "na jednej sluchawce"): blisko ogniska w obu uszach, strona narasta dopiero
	// z odlegloscia (pelna dopiero od 80 m, krawedz kadru z bliska ~44 m = ok. polowa).
	S.NonSpatializedRadiusStart = 500.f;
	S.NonSpatializedRadiusEnd = 8000.f;

	for (const FKat& K : Kat)
	{
		USoundConcurrency* C = NewObject<USoundConcurrency>(this);
		C->Concurrency.MaxCount = K.Limit;
		C->Concurrency.bLimitToOwner = false;
		C->Concurrency.ResolutionRule = K.bOdZoomu ? EMaxConcurrentResolutionRule::StopQuietest : EMaxConcurrentResolutionRule::StopOldest;
		Limity.Add(C);
	}
}

TStatId UDzwieki::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDzwieki, STATGROUP_Tickables);
}

float UDzwieki::Gain(EDzwiek K) const
{
	const FKat& C = Kat[(int32)K];
	const float U = FUstawienia::Glosnosc((int32)K); // suwaki z menu ustawien
	if (!C.bOdZoomu) return C.Glosnosc * U;
	return bSwiat ? C.Glosnosc * U * BliskoN : 0.f;
}

USoundBase* UDzwieki::Wybierz(FName Nazwa)
{
	FGrupa* G = Grupy.Find(Nazwa);
	if (!G || G->Pliki.IsEmpty()) return nullptr;
	const double Teraz = GetWorld()->GetRealTimeSeconds();
	if (Teraz - G->Kiedy < 0.03) return nullptr; // ten sam dzwiek naraz z wielu miejsc brzmi jak jeden, glosniejszy
	int32 i = FMath::RandRange(0, G->Pliki.Num() - 1);
	if (G->Pliki.Num() > 1 && i == G->Ostatni) i = (i + 1) % G->Pliki.Num();
	if (!G->Snd[i])
	{
		G->Snd[i] = Cast<USoundBase>(G->Pliki[i].TryLoad());
		if (!G->Snd[i]) return nullptr;
		Wczytane.Add(G->Snd[i]);
	}
	G->Ostatni = i;
	G->Kiedy = Teraz;
	return G->Snd[i];
}

void UDzwieki::Graj(const UObject* Kontekst, FName Nazwa, FVector Gdzie, EDzwiek K, float Glosnosc)
{
	UDzwieki* D = Z(Kontekst);
	if (!D) return;
	const float V = Glosnosc * D->Gain(K);
	if (V < 0.02f || FVector::DistSquared2D(Gdzie, D->Ognisko) > FMath::Square(PelnaDo + Wygasanie)) return;
	if (USoundBase* S = D->Wybierz(Nazwa))
		UGameplayStatics::PlaySoundAtLocation(D, S, Gdzie, FRotator::ZeroRotator, V * FMath::FRandRange(0.85f, 1.f),
			FMath::FRandRange(0.93f, 1.07f), 0.f, D->Tlumienie, D->Limity[(int32)K]);
}

void UDzwieki::Graj2D(const UObject* Kontekst, FName Nazwa, EDzwiek K, float Glosnosc)
{
	UDzwieki* D = Z(Kontekst);
	if (!D) return;
	const float V = Glosnosc * D->Gain(K);
	if (V < 0.02f) return;
	if (USoundBase* S = D->Wybierz(Nazwa))
		UGameplayStatics::PlaySound2D(D, S, V, FMath::FRandRange(0.97f, 1.03f), 0.f, D->Limity[(int32)K]);
}

void UDzwieki::Petla(const UObject* Kontekst, FName Nazwa, EDzwiek K, float Glosnosc)
{
	UDzwieki* D = Z(Kontekst);
	if (!D || !D->Grupy.Contains(Nazwa)) return;
	FPetla& P = D->Petle.FindOrAdd(Nazwa);
	P.Cel = FMath::Max(P.Cel, FMath::Clamp(Glosnosc, 0.f, 1.f));
	P.K = K;
}

void UDzwieki::Tick(float Dt)
{
	// Sluchacz w ognisku kamery na ziemi, obrocony jak kamera (lewo/prawo ekranu = lewy/prawy glosnik).
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AKameraMapy* Kam = PC ? Cast<AKameraMapy>(PC->GetPawn()) : nullptr;
	bSwiat = Kam && Kam->WRegionie();
	if (Kam)
	{
		Ognisko = Kam->Ognisko();
		const float Yaw = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation().Yaw : 0.f;
		PC->SetAudioListenerOverride(nullptr, Ognisko + FVector(0.f, 0.f, 300.f), FRotator(0.f, Yaw, 0.f));
		const float T = FMath::Clamp(FMath::Loge(FMath::Max(Kam->Oddalenie(), 1.f) / BliskoOd) / FMath::Loge(BliskoDo / BliskoOd), 0.f, 1.f);
		BliskoN = bSwiat ? 1.f - FMath::SmoothStep(0.f, 1.f, T) : 0.f;
	}

	// Petle tla: plynnie do glosnosci, ktora gra podala w tej klatce; bez podania cichna.
	for (TPair<FName, FPetla>& It : Petle)
	{
		FPetla& P = It.Value;
		const FKat& C = Kat[(int32)P.K];
		const float Cel = P.Cel * C.Glosnosc * FUstawienia::Glosnosc((int32)P.K) * (bSwiat || !C.bOdZoomu ? 1.f : 0.f);
		P.Teraz = FMath::FInterpTo(P.Teraz, Cel, Dt, 1.5f);
		P.Cel = 0.f;
		TObjectPtr<UAudioComponent>& AC = PetleC.FindOrAdd(It.Key);
		if (P.Teraz < 0.005f)
		{
			if (AC && AC->IsPlaying()) AC->Stop();
			continue;
		}
		if (!AC)
		{
			USoundBase* S = Wybierz(It.Key);
			if (!S) continue;
			AC = UGameplayStatics::CreateSound2D(this, S, P.Teraz, 1.f, 0.f, nullptr, false, false);
			if (!AC) continue;
		}
		AC->SetVolumeMultiplier(P.Teraz);
		if (!AC->IsPlaying()) AC->Play(); // pliki Petla_* maja bLooping (Zbuduj_Dzwieki)
	}
}

void UDzwieki::GrajPrace(const UObject* Kontekst, FName Nazwa, FVector Gdzie)
{
	Graj(Kontekst, Nazwa, Gdzie, EDzwiek::Praca, Nazwa == "Pila" ? 0.18f : Nazwa == "Kilof" ? 0.22f : 0.7f /* 26.11.2026: ciszej (bylo 0,3 / 0,4 / 0,8) */);
}
