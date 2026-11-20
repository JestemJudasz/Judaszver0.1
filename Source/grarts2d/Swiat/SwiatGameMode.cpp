// Plik: Source/grarts2d/Swiat/SwiatGameMode.cpp
#include "Swiat/SwiatGameMode.h"
#include "Swiat/SwiatWysokosci.h"
#include "Swiat/TerenKafle.h"
#include "Swiat/MapaRzeki.h"
#include "Kamera/KameraMapy.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "TimerManager.h"

ASwiatGameMode::ASwiatGameMode()
{
	DefaultPawnClass = AKameraMapy::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void ASwiatGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	FSwiat::Wczytaj();
}

void ASwiatGameMode::StartPlay()
{
	UWorld* W = GetWorld();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Slonce: popoludnie, cienie ukosnie jak na zrzutach.
	Slonce = W->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-38.f, 35.f, 0.f), P);
	if (Slonce.IsValid())
	{
		UDirectionalLightComponent* L = CastChecked<UDirectionalLightComponent>(Slonce->GetLightComponent());
		L->SetMobility(EComponentMobility::Movable);
		L->SetIntensity(8.f);
		L->SetDynamicShadowDistanceMovableLight(3.0e5f); // gdyby cienie wirtualne byly wylaczone: zasieg 3 km (domyslnie 200 m)
		L->SetAtmosphereSunLight(true);
	}

	// Niebo z atmosfera (bez niej daleki widok jest czarny).
	if (AActor* Niebo = W->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, P))
	{
		USkyAtmosphereComponent* Atm = NewObject<USkyAtmosphereComponent>(Niebo);
		Niebo->SetRootComponent(Atm);
		// Slabsza mgielka powietrza: z 500 km Europa ma byc czytelna, nie mleczna.
		Atm->AerialPespectiveViewDistanceScale = 0.05f;
		Atm->RegisterComponent();
	}
	if (ASkyLight* Sky = W->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, P))
	{
		USkyLightComponent* S = Sky->GetLightComponent();
		S->SetMobility(EComponentMobility::Movable);
		// Setter (nie samo pole): komponent jest juz zarejestrowany, a bez przechwytywania nieba cienie byly czarne.
		S->SetRealTimeCapture(true);
		S->SetIntensity(1.2f);
		S->RecaptureSky();
		SwiatloNieba = Sky;
	}

	// Korekta obrazu tylko dla mapy grywalnej (wlaczana w UstawSwiatloRegionu): ciemniej i mniej nasycenia jak na zrzucie wzorcowym.
	Korekta = W->SpawnActor<APostProcessVolume>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	if (Korekta.IsValid())
	{
		Korekta->bUnbound = true;
		Korekta->bEnabled = false;
		Korekta->Settings.bOverride_AutoExposureBias = true;
		Korekta->Settings.AutoExposureBias = -0.7f;
		Korekta->Settings.bOverride_ColorSaturation = true;
		Korekta->Settings.ColorSaturation = FVector4(0.82, 0.82, 0.82, 1.0);
	}

	Teren = W->SpawnActor<ATerenKafle>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	W->SpawnActor<AMapaRzeki>(FVector::ZeroVector, FRotator::ZeroRotator, P);

	Super::StartPlay();
	PokazEkranLadowania();
	// -region: start od razu w mapie grywalnej wokol punktu startu (zrzuty kontrolne: -zrzut -region).
	if (FParse::Param(FCommandLine::Get(), TEXT("region")))
	{
		WejdzDoRegionu(FSwiat::Start());
	}

	// -zrzut bez wczytania w 3 min: wyjdz mimo to (zeby proces nie wisial).
	if (FParse::Param(FCommandLine::Get(), TEXT("zrzut")))
	{
		FTimerHandle T;
		W->GetTimerManager().SetTimer(T, []() { FPlatformMisc::RequestExit(false); }, 180.f, false);
	}
}

void ASwiatGameMode::PokazEkranLadowania()
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}
	const FSlateFontInfo Duza = FCoreStyle::GetDefaultFontStyle("Bold", 30);
	const FSlateFontInfo Mala = FCoreStyle::GetDefaultFontStyle("Bold", 16);
	Nakladka = SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot()
		[
			SAssignNew(Tlo, SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.025f, 0.035f, 0.025f))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SAssignNew(NapisDuzy, STextBlock)
				.Font(Duza)
				.ColorAndOpacity(FLinearColor(0.85f, 0.88f, 0.78f))
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(24.f)
		[
			SAssignNew(NapisMaly, STextBlock)
			.Font(Mala)
			.ColorAndOpacity(FLinearColor(1.f, 0.95f, 0.7f))
			.ShadowOffset(FVector2D(1.f, 1.f))
		];
	GEngine->GameViewport->AddViewportWidgetContent(Nakladka.ToSharedRef(), 100);
}

void ASwiatGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Teren.IsValid() || !NapisDuzy.IsValid())
	{
		return;
	}
	const bool bGotowe = Teren->Gotowe();
	const int32 Proc = FMath::RoundToInt32(Teren->Postep() * 100.f);
	if (!bWczytano)
	{
		NapisDuzy->SetText(FText::FromString(FString::Printf(TEXT("%s... %d%%"), *TekstLadowania, Proc)));
		// Pol sekundy zapasu: nowe drzewa i kafle zdaza sie narysowac, zanim zniknie zaslona.
		CzasGotowy = bGotowe ? CzasGotowy + DeltaSeconds : 0.f;
		if (CzasGotowy >= 0.5f)
		{
			PoWczytaniu();
		}
		return;
	}
	// Na mapie grywalnej kafle licza sie z wyprzedzeniem poza kadrem - napis migalby przy kazdym ruchu kamery.
	NapisMaly->SetText(bGotowe || Teren->WRegionie() ? FText::GetEmpty() : FText::FromString(FString::Printf(TEXT("Doczytywanie terenu... %d%%"), Proc)));
}

void ASwiatGameMode::PoWczytaniu()
{
	bWczytano = true;
	Tlo->SetVisibility(EVisibility::Collapsed);

	// Kontrola wygladu bez klikania: -zrzut => 2 s po wczytaniu zrzut do Saved/Screenshots i wyjscie (-odl=cm wybiera wysokosc).
	if (FParse::Param(FCommandLine::Get(), TEXT("zrzut")) && !bZrzutZaplanowany)
	{
		bZrzutZaplanowany = true;
		UWorld* W = GetWorld();
		FTimerHandle T1, T2;
		W->GetTimerManager().SetTimer(T1, [W]()
		{
			// Przez kontroler gracza: komenda trafia do okna gry (GEngine->Exec jej nie wykonywal).
			if (APlayerController* PC = W->GetFirstPlayerController())
			{
				PC->ConsoleCommand(TEXT("HighResShot 1"));
			}
		}, 2.f, false);
		W->GetTimerManager().SetTimer(T2, []() { FPlatformMisc::RequestExit(false); }, 5.f, false);
	}
}

void ASwiatGameMode::Zaslon(const TCHAR* Tekst)
{
	TekstLadowania = Tekst;
	bWczytano = false;
	CzasGotowy = 0.f;
	if (Tlo.IsValid())
	{
		Tlo->SetVisibility(EVisibility::HitTestInvisible);
		NapisMaly->SetText(FText::GetEmpty());
	}
}

void ASwiatGameMode::WejdzDoRegionu(FVector2D Gdzie)
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AKameraMapy* Kam = PC ? Cast<AKameraMapy>(PC->GetPawn()) : nullptr;
	if (!Teren.IsValid() || !Kam)
	{
		return;
	}
	Teren->WejdzDoRegionu();
	Kam->UstawRegion(true, Gdzie);
	UstawSwiatloRegionu(true);
	Zaslon(TEXT("Wczytywanie regionu"));
}

void ASwiatGameMode::WyjdzZRegionu()
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	AKameraMapy* Kam = PC ? Cast<AKameraMapy>(PC->GetPawn()) : nullptr;
	if (!Teren.IsValid() || !Kam)
	{
		return;
	}
	Teren->WyjdzZRegionu();
	Kam->UstawRegion(false, FVector2D::ZeroVector);
	UstawSwiatloRegionu(false);
	Zaslon(TEXT("Wczytywanie mapy"));
}

void ASwiatGameMode::UstawSwiatloRegionu(bool bRegion)
{
	// Region: slonce 45 st. z lewej gory ekranu - cienie krotsze niz przy 30 st. (czarne plamy pod drzewami), rzezba nadal widoczna.
	// Mapa Europy: oswietlenie jak bylo (uzytkownik je zatwierdil).
	if (Slonce.IsValid())
	{
		Slonce->GetLightComponent()->SetWorldRotation(bRegion ? FRotator(-45.f, 135.f, 0.f) : FRotator(-38.f, 35.f, 0.f));
	}
	if (SwiatloNieba.IsValid())
	{
		SwiatloNieba->GetLightComponent()->SetIntensity(bRegion ? 2.0f : 1.2f); // jasniejsze cienie w regionie
	}
	if (Korekta.IsValid())
	{
		Korekta->bEnabled = bRegion;
	}
}

void ASwiatGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Nakladka.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Nakladka.ToSharedRef());
	}
	Nakladka.Reset();
	Super::EndPlay(Reason);
}
