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
#include "UI/Ustawienia.h"

bool AKameraMapy::bBezLewegoPrzeciagania = false;
bool AKameraMapy::bBitwaBlisko = false;

AKameraMapy::AKameraMapy()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Kamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Kamera"));
	Kamera->SetupAttachment(RootComponent);
	Kamera->FieldOfView = FovRegionu;
	SetCanBeDamaged(false);
}

void AKameraMapy::BeginPlay()
{
	Super::BeginPlay();
	const FVector2D S = FSwiat::Start();
	Focus = FVector(S.X, S.Y, Ziemia(S.X, S.Y));
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

void AKameraMapy::KolkoW() { Zoom((FUstawienia::bOdwrocZoom ? -1.f : 1.f) * FUstawienia::ZoomMn); }
void AKameraMapy::KolkoZ() { Zoom((FUstawienia::bOdwrocZoom ? 1.f : -1.f) * FUstawienia::ZoomMn); }

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
		CelOdl = FMath::Clamp(CelOdl * FMath::Pow(KrokKolka, -Kroki), MinRegionu(), MaxOdlRegionu);
	}
	else
	{
		CelOdl = FMath::Clamp(CelOdl * FMath::Pow(KrokKolka, -Kroki), MinOdl, MaxOdl);
		if (Kroki > 0.f && CelOdl < ProgRegionu && GM)
		{
			// Wejscie do mapy grywalnej bez skoku: ten sam srodek kadru, ta sama odleglosc, kat i pole widzenia
			// (na progu mapa papierowa ma juz rzut regionu). Zoom do kursora trwa dalej, juz w regionie.
			GM->WejdzDoRegionu(FVector2D(Focus));
			bZoomDoPunktu = bRegion && PunktPodKursorem(PunktZoomu);
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
		// Ta sama odleglosc co na mapie papierowej tuz przed wejsciem (dawniej staly ProgRegionu = skok kadru).
		Odl = FMath::Clamp(Odl, MinRegionu(), MaxOdlRegionu);
		CelOdl = FMath::Clamp(CelOdl, MinRegionu(), MaxOdlRegionu);
		CelYaw = Yaw; // dowolny kat (obrot 360 st.)
	}
	else
	{
		// Jeden klik kolka dalej niz najdalszy widok regionu (i powyzej progu, zeby nie wejsc z powrotem).
		Odl = CelOdl = FMath::Max(ProgRegionu * KrokKolka, MaxOdlRegionu * KrokKolka);
		CelYaw = Yaw;
	}
	// Wysokosc terenu zmienia sie przy przelaczeniu (przesada rzezby w regionie): ognisko od razu na nowym terenie, bez dryfu kadru.
	Focus.Z = Ziemia(Focus.X, Focus.Y);
}

void AKameraMapy::UstawOgnisko(FVector2D Gdzie)
{
	bZoomDoPunktu = false;
	Focus = FVector(Gdzie, Ziemia(Gdzie.X, Gdzie.Y));
}

float AKameraMapy::Ziemia(double X, double Y) const
{
	// Mapa papierowa jest plaska kartka na wysokosci 0 (ATerenKafle), mapa grywalna ma rzezbe.
	return bRegion ? FMath::Max(0.f, FSwiat::Wysokosc(X, Y)) : 0.f;
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
	const bool bTrzyma = (PC->IsInputKeyDown(EKeys::LeftMouseButton) && !bBezLewegoPrzeciagania)
		|| (PC->IsInputKeyDown(EKeys::MiddleMouseButton) && bBezLewegoPrzeciagania);
	// Srodkowy przycisk poza bitwa: przeciaganie w bok obraca kamere wokol punktu ogniska (360 st.).
	float MX = 0.f, MY = 0.f;
	PC->GetMousePosition(MX, MY);
	if (PC->IsInputKeyDown(EKeys::MiddleMouseButton) && !bBezLewegoPrzeciagania)
	{
		if (bObracaMysza)
		{
			CelYaw += (MX - MyszX) * ObrotMyszy * FUstawienia::MyszMn * (FUstawienia::bOdwrocObrot ? -1.f : 1.f);
		}
		bObracaMysza = true;
		bZoomDoPunktu = false;
	}
	else
	{
		bObracaMysza = false;
	}
	MyszX = MX;
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
	if (PC->IsInputKeyDown(FUstawienia::Klawisz("Przod")) || PC->IsInputKeyDown(EKeys::Up))    { Ruch.Y += 1.f; }
	if (PC->IsInputKeyDown(FUstawienia::Klawisz("Tyl")) || PC->IsInputKeyDown(EKeys::Down))  { Ruch.Y -= 1.f; }
	if (PC->IsInputKeyDown(FUstawienia::Klawisz("Prawo")) || PC->IsInputKeyDown(EKeys::Right)) { Ruch.X += 1.f; }
	if (PC->IsInputKeyDown(FUstawienia::Klawisz("Lewo")) || PC->IsInputKeyDown(EKeys::Left))  { Ruch.X -= 1.f; }
	// Q/E: plynny obrot o dowolny kat (wszedzie); mysz srodkowym przyciskiem dodaje do CelYaw, wygladzane.
	if (PC->IsInputKeyDown(FUstawienia::Klawisz("ObrotL"))) { CelYaw -= PredkoscObrotu * FUstawienia::ObrotMn * DeltaSeconds; }
	if (PC->IsInputKeyDown(FUstawienia::Klawisz("ObrotP"))) { CelYaw += PredkoscObrotu * FUstawienia::ObrotMn * DeltaSeconds; }
	Yaw = FMath::FInterpTo(Yaw, CelYaw, DeltaSeconds, 12.f);
	// Lagodny start i hamowanie (ok. 0,15 s) zamiast szarpniecia przy wcisnieciu i puszczeniu klawisza.
	const float Szybko = PC->IsInputKeyDown(FUstawienia::Klawisz("Szybciej")) ? 3.f : 1.f;
	RuchGladki = FMath::Vector2DInterpTo(RuchGladki, Ruch.GetClampedToMaxSize(1.0) * Szybko, DeltaSeconds, 8.f);
	if (!Ruch.IsNearlyZero())
	{
		bZoomDoPunktu = false;
	}
	if (!RuchGladki.IsNearlyZero(0.01))
	{
		const FRotator R(0.f, Yaw, 0.f);
		const float Sila = RuchGladki.Size();
		const FVector Kier = (R.RotateVector(FVector::ForwardVector) * RuchGladki.Y + R.RotateVector(FVector::RightVector) * RuchGladki.X) / Sila;
		const float Tempo = Odl * PredkoscPrzesuwu * FUstawienia::PrzesuwMn * Sila;
		Focus += Kier * (bRegion ? FMath::Min(Tempo, MaxPrzesuwRegionu * FMath::Min(Sila, 1.f)) : Tempo) * DeltaSeconds;
	}

	// Mapa grywalna: jazda po calej Europie, tylko z dala od krawedzi swiata (tam konczy sie teren).
	const FBox2D Granice = FBox2D(FVector2D(-FSwiat::Polowa), FVector2D(FSwiat::Polowa)).ExpandBy(bRegion ? -MarginesRegionu : 0.0);
	Focus.X = FMath::Clamp(Focus.X, Granice.Min.X, Granice.Max.X);
	Focus.Y = FMath::Clamp(Focus.Y, Granice.Min.Y, Granice.Max.Y);
	Focus.Z = FMath::FInterpTo(Focus.Z, Ziemia(Focus.X, Focus.Y), DeltaSeconds, 6.f);
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
	// Bez migotania (22.11.2026): TSR zawsze. Piksele robi drugie skalowanie: TSR wygladza obraz w 50%
	// rozdzielczosci, a potem najblizszy sasiad (r.Upscale.Quality=0) powieksza go x2. Wczesniej render 50% bez AA
	// migotal na drzewach, trawie i krawedziach przy kazdym ruchu kamery.
	IConsoleManager& CM = IConsoleManager::Get();
	// Skala renderu i metoda wygladzania z ustawien gracza (domyslnie 100% i TSR).
	FUstawienia::ZastosujObraz();
	if (IConsoleVariable* SP2 = CM.FindConsoleVariable(TEXT("r.SecondaryScreenPercentage.GameViewport")))
	{
		// Uzytkownik (22.11.2026): piksele 50% z TSR "za bardzo rozmazane" - pelna rozdzielczosc, ostrosc z r.Tonemapper.Sharpen.
		// Powrot do pikseli: bPiksele ? 50.f : 0.f.
		SP2->Set(0.f, ECVF_SetByCode); // 0 = bez drugiego skalowania
	}
}

void AKameraMapy::UstawKamere()
{
	SetActorLocation(Focus);
	// Z daleka kamera prostuje sie do pionu (mapa). Na progu wejscia (ProgRegionu) mapa papierowa ma dokladnie
	// kat i pole widzenia mapy grywalnej, wiec przejscie miedzy nimi nie zmienia skali ani kadru.
	float Pitch = NachylenieRegionu;
	float Fov = FovRegionu;
	if (!bRegion)
	{
		Pitch = FMath::Lerp(NachylenieRegionu, NachylenieDaleko, Przejscie(ProgRegionu, 4.0e6f));
		Fov = Odl < 2.0e6f
			? FMath::Lerp(FovRegionu, FovBlisko, Przejscie(ProgRegionu, 2.0e6f))
			: FMath::Lerp(FovBlisko, FovDaleko, Przejscie(2.0e6f, 4.0e7f));
	}
	Kamera->SetFieldOfView(Fov);

	const FRotator Rot(-Pitch, Yaw, 0.f);
	FVector Offset = -Rot.Vector() * Odl;
	const FVector Cam = Focus + Offset;
	const float MinZ = Ziemia(Cam.X, Cam.Y) + MinNadZiemia;
	if (Cam.Z < MinZ)
	{
		Offset.Z += MinZ - Cam.Z;
	}
	Kamera->SetRelativeLocationAndRotation(Offset, (-Offset).Rotation());
}
