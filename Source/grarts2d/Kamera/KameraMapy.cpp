// Plik: Source/grarts2d/Kamera/KameraMapy.cpp
#include "Kamera/KameraMapy.h"
#include "Swiat/SwiatWysokosci.h"
#include "Swiat/SwiatGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "HAL/IConsoleManager.h"

AKameraMapy::AKameraMapy()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Kamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Kamera"));
	Kamera->SetupAttachment(RootComponent);
	Kamera->FieldOfView = FovBlisko;
	SetCanBeDamaged(false);
}

void AKameraMapy::BeginPlay()
{
	Super::BeginPlay();
	const FVector2D S = FSwiat::Start();
	Focus = FVector(S.X, S.Y, FMath::Max(0.f, FSwiat::Wysokosc(S.X, S.Y)));
	FParse::Value(FCommandLine::Get(), TEXT("odl="), StartOdl); // np. -odl=20000 (zrzuty kontrolne)
	Odl = CelOdl = FMath::Clamp(StartOdl, MinOdl, MaxOdl);
	SetActorRotation(FRotator::ZeroRotator);

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI Tryb;
		Tryb.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Tryb);
	}
	UstawKamere();
}

void AKameraMapy::SetupPlayerInputComponent(UInputComponent* In)
{
	Super::SetupPlayerInputComponent(In);
	In->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AKameraMapy::KolkoW);
	In->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AKameraMapy::KolkoZ);
	In->BindKey(EKeys::Q, IE_Pressed, this, &AKameraMapy::ObrotL);
	In->BindKey(EKeys::E, IE_Pressed, this, &AKameraMapy::ObrotP);
}

void AKameraMapy::KolkoW() { Zoom(1.f); }
void AKameraMapy::KolkoZ() { Zoom(-1.f); }

void AKameraMapy::Zoom(float Kroki)
{
	ASwiatGameMode* GM = GetWorld()->GetAuthGameMode<ASwiatGameMode>();
	if (bRegion)
	{
		// Oddalenie na granicy zoomu regionu wraca do mapy Europy.
		if (Kroki < 0.f && CelOdl >= MaxOdlRegionu * 0.999f && GM)
		{
			GM->WyjdzZRegionu();
			return;
		}
		CelOdl = FMath::Clamp(CelOdl * FMath::Pow(KrokKolka, -Kroki), MinOdlRegionu, MaxOdlRegionu);
	}
	else
	{
		CelOdl = FMath::Clamp(CelOdl * FMath::Pow(KrokKolka, -Kroki), MinOdl, MaxOdl);
		if (Kroki > 0.f && CelOdl < ProgRegionu && GM)
		{
			FVector P;
			GM->WejdzDoRegionu(PunktPodKursorem(P) ? FVector2D(P) : FVector2D(Focus));
			return;
		}
	}
	// Jak w Google Maps: punkt pod kursorem zostaje pod kursorem.
	bZoomDoPunktu = PunktPodKursorem(PunktZoomu);
}

void AKameraMapy::UstawRegion(bool bWlacz, FVector2D Srodek)
{
	bRegion = bWlacz;
	bZoomDoPunktu = false;
	bPrzeciaganie = false;
	if (bWlacz)
	{
		Focus = FVector(Srodek, Focus.Z);
		Odl = CelOdl = FMath::Min(ProgRegionu, MaxOdlRegionu); // ta sama wysokosc co przed wejsciem, bez skoku
		Yaw = CelYaw = FMath::RoundToFloat(Yaw / 90.f) * 90.f;
	}
	else
	{
		Odl = CelOdl = FMath::Max(ProgRegionu * 1.6f, MaxOdlRegionu * 1.2f); // dalej niz region, bez skoku w przod
		CelYaw = Yaw;
	}
}

bool AKameraMapy::PunktPodKursorem(FVector& Out) const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	FVector Pocz, Kier;
	if (!PC || !PC->DeprojectMousePositionToWorld(Pocz, Kier) || Kier.Z > -0.01)
	{
		return false;
	}
	// shortcut: przeciecie z plaszczyzna na wysokosci punktu ogniska, nie z terenem; wystarcza do zoomu i przesuwu.
	const double T = (Focus.Z - Pocz.Z) / Kier.Z;
	Out = Pocz + Kier * T;
	return true;
}

float AKameraMapy::Przejscie(float A, float B) const
{
	const float X = FMath::Clamp((FMath::Loge(Odl) - FMath::Loge(A)) / (FMath::Loge(B) - FMath::Loge(A)), 0.f, 1.f);
	return X * X * (3.f - 2.f * X);
}

float AKameraMapy::StrefaMapy() const   { return bRegion ? 0.f : 1.f; }   // poza regionem zawsze papier
float AKameraMapy::SzerokoscWidoku() const { return 2.f * Odl * FMath::Tan(FMath::DegreesToRadians(Kamera->FieldOfView * 0.5f)); }
float AKameraMapy::StrefaSwiata() const { return Przejscie(1.0e6f, 3.0e6f); }   // 10-30 km

void AKameraMapy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// Zoom wygladzony w skali logarytmicznej (rowne tempo od 25 m do 600 km).
	const float Poprzednia = Odl;
	Odl = FMath::Exp(FMath::FInterpTo(FMath::Loge(Odl), FMath::Loge(CelOdl), DeltaSeconds, Wygladzanie));
	if (bZoomDoPunktu && !FMath::IsNearlyEqual(Poprzednia, Odl))
	{
		const double Ratio = double(Odl) / Poprzednia;
		Focus.X = PunktZoomu.X + (Focus.X - PunktZoomu.X) * Ratio;
		Focus.Y = PunktZoomu.Y + (Focus.Y - PunktZoomu.Y) * Ratio;
	}

	// Przeciaganie mapy (lewy albo srodkowy przycisk): chwycony punkt jedzie za kursorem.
	const bool bTrzyma = PC->IsInputKeyDown(EKeys::LeftMouseButton) || PC->IsInputKeyDown(EKeys::MiddleMouseButton);
	if (bTrzyma)
	{
		FVector Teraz;
		if (PunktPodKursorem(Teraz))
		{
			if (!bPrzeciaganie)
			{
				PunktChwytu = Teraz;
				bPrzeciaganie = true;
			}
			else
			{
				Focus.X += PunktChwytu.X - Teraz.X;
				Focus.Y += PunktChwytu.Y - Teraz.Y;
			}
		}
		bZoomDoPunktu = false;
	}
	else
	{
		bPrzeciaganie = false;
	}

	// Klawisze.
	FVector2D Ruch = FVector2D::ZeroVector;
	if (PC->IsInputKeyDown(EKeys::W) || PC->IsInputKeyDown(EKeys::Up))    { Ruch.Y += 1.f; }
	if (PC->IsInputKeyDown(EKeys::S) || PC->IsInputKeyDown(EKeys::Down))  { Ruch.Y -= 1.f; }
	if (PC->IsInputKeyDown(EKeys::D) || PC->IsInputKeyDown(EKeys::Right)) { Ruch.X += 1.f; }
	if (PC->IsInputKeyDown(EKeys::A) || PC->IsInputKeyDown(EKeys::Left))  { Ruch.X -= 1.f; }
	if (bRegion)
	{
		Yaw = FMath::FInterpTo(Yaw, CelYaw, DeltaSeconds, 8.f); // obrot skokami po 90 st.
	}
	else
	{
		if (PC->IsInputKeyDown(EKeys::Q)) { Yaw -= PredkoscObrotu * DeltaSeconds; }
		if (PC->IsInputKeyDown(EKeys::E)) { Yaw += PredkoscObrotu * DeltaSeconds; }
		CelYaw = Yaw;
	}
	if (!Ruch.IsNearlyZero())
	{
		const float Szybko = PC->IsInputKeyDown(EKeys::LeftShift) ? 3.f : 1.f;
		const FRotator R(0.f, Yaw, 0.f);
		const FVector Krok = (R.RotateVector(FVector::ForwardVector) * Ruch.Y + R.RotateVector(FVector::RightVector) * Ruch.X).GetClampedToMaxSize(1.0);
		const float Tempo = Odl * PredkoscPrzesuwu * Szybko;
		Focus += Krok * (bRegion ? FMath::Min(Tempo, MaxPrzesuwRegionu) : Tempo) * DeltaSeconds;
		bZoomDoPunktu = false;
	}

	// Mapa grywalna: jazda po calej Europie, tylko z dala od krawedzi swiata (tam konczy sie teren).
	const FBox2D Granice = FBox2D(FVector2D(-FSwiat::Polowa), FVector2D(FSwiat::Polowa)).ExpandBy(bRegion ? -MarginesRegionu : 0.0);
	Focus.X = FMath::Clamp(Focus.X, Granice.Min.X, Granice.Max.X);
	Focus.Y = FMath::Clamp(Focus.Y, Granice.Min.Y, Granice.Max.Y);
	Focus.Z = FMath::FInterpTo(Focus.Z, FMath::Max(0.f, FSwiat::Wysokosc(Focus.X, Focus.Y)), DeltaSeconds, 6.f);
	if (bZoomDoPunktu)
	{
		PunktZoomu.Z = Focus.Z;
	}
	UstawPiksele();
	UstawKamere();
}

void AKameraMapy::UstawPiksele()
{
	// Poziom "budowa" (blisko): render w 50% i ostre piksele bez wygladzania. Dalej pelna rozdzielczosc z TSR.
	// Histereza, zeby obraz nie migal na granicy.
	const bool bChce = bRegion || (bPiksele ? Odl < PikseleDo * 1.2f : Odl < PikseleDo);
	if (bChce == bPiksele && bPikseleUstawione)
	{
		return;
	}
	bPiksele = bChce;
	bPikseleUstawione = true;
	IConsoleManager& CM = IConsoleManager::Get();
	if (IConsoleVariable* SP = CM.FindConsoleVariable(TEXT("r.ScreenPercentage")))
	{
		SP->Set(bPiksele ? 50.f : 100.f, ECVF_SetByCode);
	}
	if (IConsoleVariable* AA = CM.FindConsoleVariable(TEXT("r.AntiAliasingMethod")))
	{
		AA->Set(bPiksele ? 0 : 4, ECVF_SetByCode); // 0 = bez AA, 4 = TSR
	}
}

void AKameraMapy::UstawKamere()
{
	SetActorLocation(Focus);
	// Z daleka kamera prostuje sie do pionu (mapa), blisko ukos jak na zrzutach.
	const float Pitch = bRegion ? NachylenieRegionu : FMath::Lerp(NachylenieBlisko, NachylenieDaleko, Przejscie(2.0e5f, 4.0e6f));
	Kamera->SetFieldOfView(bRegion ? FovRegionu : FMath::Lerp(FovBlisko, FovDaleko, Przejscie(2.0e6f, 4.0e7f)));

	const FRotator Rot(-Pitch, Yaw, 0.f);
	FVector Offset = -Rot.Vector() * Odl;
	const FVector Cam = Focus + Offset;
	const float MinZ = FMath::Max(0.f, FSwiat::Wysokosc(Cam.X, Cam.Y)) + MinNadZiemia;
	if (Cam.Z < MinZ)
	{
		Offset.Z += MinZ - Cam.Z;
	}
	Kamera->SetRelativeLocationAndRotation(Offset, (-Offset).Rotation());
}
