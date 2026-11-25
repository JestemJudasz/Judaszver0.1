// Plik: Source/grarts2d/Dev/Galeria.cpp
// Galerie testowe (opis w Galeria.h).
#include "Dev/Galeria.h"
#include "Ludzie/Ludzik.h"
#include "Osada/Osada.h"
#include "Efekty/Efekty.h"
#include "Swiat/SwiatWysokosci.h"
#include "Swiat/TerenKafle.h"
#include "Kamera/KameraMapy.h"
#include "Wojna/RodzajeWojsk.h"
#include "UI/StylUI.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/SLeafWidget.h"

namespace
{
	class SNapisyGalerii : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SNapisyGalerii) {}
			SLATE_ARGUMENT(TWeakObjectPtr<AGaleria>, Galeria)
		SLATE_END_ARGS()

		void Construct(const FArguments& A)
		{
			Galeria = A._Galeria;
			Tlo = FSlateRoundedBoxBrush(FLinearColor(0.035f, 0.028f, 0.02f, 0.72f), 4.f);
			SetVisibility(EVisibility::HitTestInvisible);
		}
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L,
			const FWidgetStyle&, bool) const override
		{
			const AGaleria* Ga = Galeria.Get();
			APlayerController* PC = Ga ? Ga->GetWorld()->GetFirstPlayerController() : nullptr;
			if (!PC || !PC->PlayerCameraManager)
			{
				return L;
			}
			const FVector Kam = PC->PlayerCameraManager->GetCameraLocation();
			const float Skala = FMath::Max(0.01f, G.Scale);
			const FVector2f Rozm = FVector2f(G.GetLocalSize());
			const FSlateFontInfo Gruba = FCoreStyle::GetDefaultFontStyle("Bold", 12);
			const FSlateFontInfo Cienka = FCoreStyle::GetDefaultFontStyle("Regular", 9);
			for (const AGaleria::FNapis& N : Ga->Napisy())
			{
				// Z daleka tylko nazwy rzedow (inaczej ekran zlewa sie w napisy).
				const double Odl = FVector::Dist(Kam, N.P);
				if (Odl > (N.bGlowny ? 60000.0 : 14000.0))
				{
					continue;
				}
				FVector2D Px;
				if (!PC->ProjectWorldLocationToScreen(N.P, Px, false))
				{
					continue;
				}
				const FVector2f P = FVector2f(Px / Skala);
				if (P.X < -200.f || P.Y < -40.f || P.X > Rozm.X + 50.f || P.Y > Rozm.Y + 40.f)
				{
					continue;
				}
				const FSlateFontInfo& F = N.bGlowny ? Gruba : Cienka;
				const float Szer = (N.bGlowny ? 8.5f : 6.2f) * N.T.Len() + 10.f;
				const float Wys = N.bGlowny ? 20.f : 15.f;
				const FVector2f R = P - FVector2f(Szer * 0.5f, Wys);
				FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(FVector2f(Szer, Wys), FSlateLayoutTransform(R)), &Tlo);
				FSlateDrawElement::MakeText(Out, L + 1, G.ToPaintGeometry(FVector2f(Szer, Wys), FSlateLayoutTransform(R + FVector2f(5.f, 1.f))),
					N.T, F, ESlateDrawEffect::None, N.bGlowny ? StylUI::Zloto : StylUI::Napis);
			}
			return L + 2;
		}

	private:
		TWeakObjectPtr<AGaleria> Galeria;
		FSlateRoundedBoxBrush Tlo = FSlateRoundedBoxBrush(FLinearColor::Black, 4.f);
	};

	/** Czlon nazwy modelu, ktory opisuje stan, nie budynek: F3, Z1, W2, U. */
	bool CzlonStanu(const FString& C)
	{
		if (C == TEXT("U"))
		{
			return true;
		}
		return C.Len() >= 2 && (C[0] == 'F' || C[0] == 'Z' || C[0] == 'W') && C.Mid(1).IsNumeric();
	}

	/** "W2 etap 3", "uszkodzony", "ruina", "przebudowa" z czlonow stanu. */
	FString OpisStanu(const TArray<FString>& Czl)
	{
		TArray<FString> T;
		for (const FString& C : Czl)
		{
			if (C == TEXT("U")) { T.Add(TEXT("przebudowa")); }
			else if (C == TEXT("Z1")) { T.Add(TEXT("uszkodzony")); }
			else if (C == TEXT("Z2")) { T.Add(TEXT("ruina")); }
			else if (C[0] == 'F') { T.Add(TEXT("etap ") + C.Mid(1)); }
			else { T.Add(TEXT("wariant ") + C.Mid(1)); }
		}
		return T.Num() ? FString::Join(T, TEXT(", ")) : FString(TEXT("model"));
	}

	/** Klucz kolejnosci w rzedzie: wariant, potem etapy F, przebudowa, uszkodzony, ruina. */
	int32 Kolejnosc(const TArray<FString>& Czl)
	{
		int32 Wariant = 0, Typ = 0, Nr = 0;
		for (const FString& C : Czl)
		{
			if (C[0] == 'W') { Wariant = FCString::Atoi(*C.Mid(1)); }
			else if (C == TEXT("U")) { Typ = 1; }
			else if (C[0] == 'F') { Typ = 0; Nr = FCString::Atoi(*C.Mid(1)); }
			else if (C[0] == 'Z') { Typ = 2; Nr = FCString::Atoi(*C.Mid(1)); }
		}
		return Wariant * 1000 + Typ * 100 + Nr;
	}

	// Kolumny galerii wojska.
	enum EKol : int32 { Stoi, Idzie, Biegnie, Ranny, Cios, Blok, Trafiony, TrafionyStrzala, Napor, Okrzyk, PadaCios, PadaStrzala, Sciety, BezReki, Kolumn };
	const TCHAR* NazwyKolumn[Kolumn] = { TEXT("stoi"), TEXT("idzie"), TEXT("biegnie"), TEXT("ranny"), TEXT("cios / strzał"), TEXT("blok"),
		TEXT("trafiony"), TEXT("trafiony strzałą"), TEXT("napór"), TEXT("okrzyk"), TEXT("pada od ciosu"), TEXT("pada od strzały"),
		TEXT("ścięty"), TEXT("bez ręki") };
	// Rzad narzedzi ruszenia: kazda kolumna inne narzedzie (zadaje ciosy).
	const TCHAR* Narzedzia[] = { TEXT("SM_Ludz_Siekiera"), TEXT("SM_Ludz_Kilof"), TEXT("SM_Ludz_Motyka"), TEXT("SM_Ludz_Sierp"),
		TEXT("SM_Ludz_Pila"), TEXT("SM_Ludz_Mlotek"), TEXT("SM_Ludz_Nozyce"), TEXT("SM_Ludz_Wedka"), TEXT("SM_Ludz_Maczuga"),
		TEXT("SM_Ludz_Wlocznia"), TEXT("SM_Ludz_Miecz") };
	constexpr int32 Rodzajow = int32(ERodzajWojska::Ile);
	constexpr int32 RzadNarzedzi = Rodzajow;
	constexpr int32 RzadKobiety = Rodzajow + 1;
	constexpr float OdstepKol = 450.f, OdstepRzad = 560.f;
}

AGaleria::AGaleria()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Korzen"));
}

FString AGaleria::Przelacz(UWorld* W, ERodzaj R)
{
	if (!W)
	{
		return FString();
	}
	bool bByla = false;
	for (TActorIterator<AGaleria> It(W); It; ++It)
	{
		bByla |= It->Rodzaj == R;
		It->Destroy();
	}
	if (bByla)
	{
		return TEXT("Galeria zamknięta");
	}
	APlayerController* PC = W->GetFirstPlayerController();
	AKameraMapy* Kam = PC ? Cast<AKameraMapy>(PC->GetPawn()) : nullptr;
	AGaleria* G = W->SpawnActor<AGaleria>();
	if (!G)
	{
		return TEXT("Nie udało się");
	}
	G->Rodzaj = R;
	// Uklad do ekranu: kolumny w prawo, rzedy od kamery w glab.
	if (PC && PC->PlayerCameraManager)
	{
		const FVector F = PC->PlayerCameraManager->GetCameraRotation().Vector();
		const FVector2D F2 = FVector2D(F.X, F.Y).GetSafeNormal();
		if (!F2.IsNearlyZero())
		{
			G->Wglab = F2;
			G->Prawo = FVector2D(-F2.Y, F2.X);
		}
	}
	G->Start = Kam ? FVector2D(Kam->GetActorLocation()) : FSwiat::Start();
	const FString T = R == ERodzaj::Budynki ? G->PostawBudynki() : G->PostawWojsko();
	if (GEngine && GEngine->GameViewport)
	{
		G->Nakladka = SNew(SNapisyGalerii).Galeria(G);
		GEngine->GameViewport->AddViewportWidgetContent(G->Nakladka.ToSharedRef(), 4);
	}
	if (Kam)
	{
		// Kamera na poczatek galerii (budynki z dalsza, ludzie z bliska).
		const FVector2D Cel = G->Start + G->Prawo * (R == ERodzaj::Budynki ? 9000.f : OdstepKol * float(Kolumn) * 0.5f)
			+ G->Wglab * (R == ERodzaj::Budynki ? 6000.f : OdstepRzad * 4.f);
		Kam->UstawOgnisko(Cel);
		float Odl = R == ERodzaj::Budynki ? 22000.f : 7000.f;
		FParse::Value(FCommandLine::Get(), TEXT("odl="), Odl);
		AKameraMapy::bBitwaBlisko = R == ERodzaj::Wojsko;
		Kam->UstawOdl(Odl);
	}
	UE_LOG(LogTemp, Log, TEXT("Galeria: %s"), *T);
	return T;
}

void AGaleria::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Nakladka.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Nakladka.ToSharedRef());
	}
	Nakladka.Reset();
	for (FKomorka& K : Komorki)
	{
		if (K.L.IsValid()) { K.L->Destroy(); }
	}
	Komorki.Reset();
	Super::EndPlay(Reason);
}

FString AGaleria::PostawBudynki()
{
	// Modele zebrane z rejestru zasobow (nowe modele pojawiaja sie same): rzad na budynek, w rzedzie jego stany.
	struct FModel { UStaticMesh* M; FString Opis; int32 Klucz; };
	TMap<FString, TArray<FModel>> Rzedy;
	IAssetRegistry& Rej = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	for (const TCHAR* Folder : { TEXT("/Game/Swiat/Budynki"), TEXT("/Game/Swiat/Natura"), TEXT("/Game/Swiat/Drogi") })
	{
		TArray<FAssetData> Zasoby;
		Rej.GetAssetsByPath(FName(Folder), Zasoby, true);
		const bool bBudynki = FCString::Strstr(Folder, TEXT("Budynki")) != nullptr;
		for (const FAssetData& A : Zasoby)
		{
			if (A.AssetClassPath != UStaticMesh::StaticClass()->GetClassPathName())
			{
				continue;
			}
			FString Nazwa = A.AssetName.ToString();
			Nazwa.RemoveFromStart(TEXT("SM_"));
			TArray<FString> Czl;
			Nazwa.ParseIntoArray(Czl, TEXT("_"));
			TArray<FString> Stan;
			while (bBudynki && Czl.Num() > 1 && CzlonStanu(Czl.Last()))
			{
				Stan.Insert(Czl.Pop(), 0);
			}
			const FString Rzad = bBudynki ? FString::Join(Czl, TEXT("_")) : FString(FPaths::GetCleanFilename(Folder));
			UStaticMesh* M = Cast<UStaticMesh>(A.GetAsset());
			if (M)
			{
				Rzedy.FindOrAdd(Rzad).Add({ M, bBudynki ? OpisStanu(Stan) : Nazwa, bBudynki ? Kolejnosc(Stan) : 0 });
			}
		}
	}
	Rzedy.KeySort([](const FString& A, const FString& B) { return A < B; });

	// Uklad: rzedy jeden za drugim; co ok. 260 m glebi nowy blok obok (zeby galeria nie ciagnela sie kilometrami w glab).
	struct FStaw { UStaticMesh* M; FVector2D UV; FString Opis; float Wys; };
	TArray<FStaw> Staw;
	TArray<TPair<FVector2D, FString>> NazwyRzedow;
	float U0 = 0.f, V = 0.f, SzerBloku = 0.f, MaxU = 0.f;
	int32 Modeli = 0;
	for (TPair<FString, TArray<FModel>>& R : Rzedy)
	{
		R.Value.Sort([](const FModel& A, const FModel& B) { return A.Klucz != B.Klucz ? A.Klucz < B.Klucz : A.Opis < B.Opis; });
		float Glab = 0.f;
		for (const FModel& M : R.Value)
		{
			Glab = FMath::Max(Glab, float(M.M->GetBounds().BoxExtent.Y) * 2.f);
		}
		if (V > 0.f && V + Glab > 26000.f)
		{
			U0 += SzerBloku + 4000.f;
			V = SzerBloku = 0.f;
		}
		float U = U0;
		NazwyRzedow.Add({ FVector2D(U0 - 600.f, V + Glab * 0.5f), R.Key });
		for (const FModel& M : R.Value)
		{
			const FBoxSphereBounds B = M.M->GetBounds();
			const float Szer = float(B.BoxExtent.X) * 2.f;
			// Srodek obrysu (nie poczatek ukladu modelu) w srodku komorki.
			Staw.Add({ M.M, FVector2D(U + Szer * 0.5f - B.Origin.X, V + Glab * 0.5f - B.Origin.Y), M.Opis, float(B.Origin.Z + B.BoxExtent.Z) });
			U += Szer + 500.f;
			++Modeli;
		}
		SzerBloku = FMath::Max(SzerBloku, U - U0);
		MaxU = FMath::Max(MaxU, U);
		V += Glab + 900.f;
	}
	// Rzad efektow przed galeria: plonacy dom i miejsce zawalen.
	const float VEf = -3000.f;

	// Miejsce: sucha laka przy kamerze (sprawdzane w siatce punktow galerii).
	const float GlabCala = Rzedy.Num() ? 26000.f : 2000.f;
	auto Swiat = [&](FVector2D Org, FVector2D UV) { return Org + Prawo * UV.X + Wglab * UV.Y; };
	auto Sucho = [&](FVector2D Org)
	{
		for (int32 i = 0; i <= 10; ++i)
		{
			for (int32 j = -1; j <= 8; ++j)
			{
				const FVector2D P = Swiat(Org, FVector2D(MaxU * i / 10.f, GlabCala * j / 8.f));
				if (FSwiat::Woda(P.X, P.Y) != FSwiat::EWoda::Lad || FSwiat::Wysokosc(P.X, P.Y) <= 0.f)
				{
					return false;
				}
			}
		}
		return true;
	};
	const FVector2D Kamera = Start;
	Start = Kamera - Prawo * 9000.f - Wglab * 6000.f;
	for (int32 Proba = 0; Proba < 120; ++Proba)
	{
		const float Kat = Proba * 0.7f, Odl = 5000.f * FMath::Sqrt(float(Proba));
		const FVector2D O = Kamera - Prawo * 9000.f - Wglab * 6000.f + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * Odl;
		if (Sucho(O))
		{
			Start = O;
			break;
		}
	}
	// shortcut: gdy nigdzie w poblizu nie ma tak duzej suchej laki, galeria stoi czesciowo w wodzie - wystarczy do ogladania.

	ATerenKafle* Teren = nullptr;
	for (TActorIterator<ATerenKafle> It(GetWorld()); It; ++It) { Teren = *It; break; }
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Prawo.Y, Prawo.X));
	auto Postaw = [&](UStaticMesh* M, FVector2D G)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(M);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetupAttachment(RootComponent);
		C->RegisterComponent();
		C->SetWorldLocationAndRotation(FVector(G, FSwiat::Wysokosc(G.X, G.Y)), FRotator(0.f, Yaw, 0.f));
		if (Teren)
		{
			Teren->Wykarczuj(G, float(M->GetBounds().SphereRadius) + 200.f);
		}
		return C;
	};
	SetActorLocation(FVector(Start, FSwiat::Wysokosc(Start.X, Start.Y)));
	for (const FStaw& S : Staw)
	{
		const FVector2D G = Swiat(Start, S.UV);
		UStaticMeshComponent* C = Postaw(S.M, G);
		NapisyL.Add({ C->GetComponentLocation() + FVector(0.f, 0.f, S.Wys + 150.f), S.Opis, false });
	}
	for (const TPair<FVector2D, FString>& N : NazwyRzedow)
	{
		const FVector2D G = Swiat(Start, N.Key);
		NapisyL.Add({ FVector(G, FSwiat::Wysokosc(G.X, G.Y) + 300.f), N.Value, true });
	}
	// Efekty: dom w ogniu (caly czas) i co 12 s zawalenie ruiny.
	UStaticMesh* Dom = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Swiat/Budynki/SM_ChataDrwala_Z1.SM_ChataDrwala_Z1"));
	UStaticMesh* Ruina = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Swiat/Budynki/SM_ChataDrwala_Z2.SM_ChataDrwala_Z2"));
	if (Dom && Ruina)
	{
		const FVector2D A = Swiat(Start, FVector2D(2000.f, VEf)), B = Swiat(Start, FVector2D(6000.f, VEf));
		Postaw(Dom, A);
		Postaw(Ruina, B);
		const FBoxSphereBounds Bd = Dom->GetBounds();
		PozarPol = FVector2D(Bd.BoxExtent.X, Bd.BoxExtent.Y);
		PozarS = FVector(A, FSwiat::Wysokosc(A.X, A.Y));
		ZawalS = FVector(B, FSwiat::Wysokosc(B.X, B.Y));
		if (AEfekty* E = AEfekty::W(GetWorld()))
		{
			E->UstawPozar(E->Pozar(PozarS, PozarPol, Yaw, float(Bd.Origin.Z + Bd.BoxExtent.Z)), 1.f);
		}
		NapisyL.Add({ PozarS + FVector(0.f, 0.f, 900.f), TEXT("Pożar"), true });
		NapisyL.Add({ ZawalS + FVector(0.f, 0.f, 900.f), TEXT("Zawalenie (co 12 s)"), true });
	}
	return FString::Printf(TEXT("Galeria budynków: %d modeli w %d rzędach (F5 zamyka)"), Modeli, Rzedy.Num());
}

FString AGaleria::PostawWojsko()
{
	ATerenKafle* Teren = nullptr;
	for (TActorIterator<ATerenKafle> It(GetWorld()); It; ++It) { Teren = *It; break; }
	const int32 Rzedow = Rodzajow + 2;
	const FVector2D Kamera = Start;
	auto Swiat = [&](FVector2D Org, float U, float V) { return Org + Prawo * U + Wglab * V; };
	// Sucha laka przy kamerze.
	Start = Kamera - Prawo * (OdstepKol * float(Kolumn) * 0.5f);
	for (int32 Proba = 0; Proba < 120; ++Proba)
	{
		const float Kat = Proba * 0.7f, Odl = 2000.f * FMath::Sqrt(float(Proba));
		const FVector2D O = Start + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * Odl;
		bool bSucho = true;
		for (int32 i = 0; i <= Kolumn && bSucho; i += 2)
		{
			for (int32 j = -1; j <= Rzedow && bSucho; ++j)
			{
				const FVector2D P = Swiat(O, i * OdstepKol, j * OdstepRzad);
				bSucho = FSwiat::Woda(P.X, P.Y) == FSwiat::EWoda::Lad && FSwiat::Wysokosc(P.X, P.Y) > 0.f;
			}
		}
		if (bSucho)
		{
			Start = O;
			break;
		}
	}
	SetActorLocation(FVector(Start, FSwiat::Wysokosc(Start.X, Start.Y)));
	if (Teren)
	{
		Teren->Wykarczuj(Swiat(Start, OdstepKol * float(Kolumn) * 0.5f, OdstepRzad * Rzedow * 0.5f), FMath::Max(OdstepKol * float(Kolumn), OdstepRzad * Rzedow) * 0.75f);
	}
	for (int32 r = 0; r < Rzedow; ++r)
	{
		const int32 Kol = r == RzadNarzedzi ? UE_ARRAY_COUNT(Narzedzia) : Kolumn;
		for (int32 k = 0; k < Kol; ++k)
		{
			FKomorka K;
			K.Rzad = r;
			K.Kol = k;
			K.P = Swiat(Start, (k + 0.5f) * OdstepKol, (r + 0.5f) * OdstepRzad);
			Komorki.Add(K);
		}
		const FString Nazwa = r < Rodzajow ? FString(RodzajWojska(ERodzajWojska(r)).Nazwa) : r == RzadNarzedzi ? FString(TEXT("Ruszenie: narzędzia")) : FString(TEXT("Kobieta"));
		const FVector2D G = Swiat(Start, -350.f, (r + 0.5f) * OdstepRzad);
		NapisyL.Add({ FVector(G, FSwiat::Wysokosc(G.X, G.Y) + 260.f), Nazwa, true });
	}
	for (int32 k = 0; k < Kolumn; ++k)
	{
		const FVector2D G = Swiat(Start, (k + 0.5f) * OdstepKol, -250.f);
		NapisyL.Add({ FVector(G, FSwiat::Wysokosc(G.X, G.Y) + 120.f), NazwyKolumn[k], false });
	}
	for (int32 k = 0; k < UE_ARRAY_COUNT(Narzedzia); ++k)
	{
		const FVector2D G = Swiat(Start, (k + 0.5f) * OdstepKol, (RzadNarzedzi + 0.5f) * OdstepRzad);
		FString N = Narzedzia[k];
		N.RemoveFromStart(TEXT("SM_Ludz_"));
		NapisyL.Add({ FVector(G, FSwiat::Wysokosc(G.X, G.Y) + 330.f), N.ToLower(), false });
	}
	for (FKomorka& K : Komorki)
	{
		Ozyw(K);
	}
	return FString::Printf(TEXT("Galeria wojska: %d rodzajów, %d animacji, %d ludzi (F6 zamyka)"), Rodzajow, int32(Kolumn), Komorki.Num());
}

void AGaleria::Ozyw(FKomorka& K)
{
	if (K.L.IsValid())
	{
		K.L->Destroy();
	}
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector2D Do = -Wglab; // twarza do kamery
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Do.Y, Do.X));
	ALudzik* L = GetWorld()->SpawnActor<ALudzik>(FVector(K.P, FSwiat::Wysokosc(K.P.X, K.P.Y)), FRotator(0.f, Yaw, 0.f), P);
	if (!L)
	{
		return;
	}
	const bool bKobieta = K.Rzad == RzadKobiety;
	const ERodzajWojska Rd = K.Rzad < Rodzajow ? ERodzajWojska(K.Rzad) : ERodzajWojska::Chlopi;
	const FRodzajWojska& R = RodzajWojska(Rd);
	L->UstawPlec(bKobieta);
	const TCHAR* Bron = bKobieta ? TEXT("SM_Ludz_Kosz") : K.Rzad == RzadNarzedzi ? Narzedzia[K.Kol] : R.Bron;
	L->WejdzDoBitwy(bKobieta ? FLinearColor(0.55f, 0.42f, 0.3f) : R.Tunika, FLinearColor(0.2f, 0.12f, 0.06f), Bron);
	if (!bKobieta)
	{
		// Wyglad jak w bitwie: nakrycie glowy po kolei przez kolumny, tarcza wedlug rodzaju.
		const int32 Helm = R.Helm > 0.5f ? 1 : R.Czapka > 0.2f ? 2 : R.Kaptur > 0.2f ? 3 : 0;
		L->UbierzDoBitwy(Helm, K.Kol % 2 == 0, R.Tarcza > 0.5f, K.Rzad == 0 && K.Kol == Stoi, FLinearColor(0.72f, 0.14f, 0.09f));
	}
	if (K.Rzad == RzadNarzedzi || K.Kol == Ranny)
	{
		L->UstawRane(K.Kol == Ranny && K.Rzad != RzadNarzedzi ? 0.8f : 0.f, true);
	}
	L->WalkaStoj(K.P + Do * 1000.f);
	if (K.Kol == Napor && K.Rzad != RzadNarzedzi)
	{
		L->WalkaNapor(true);
	}
	K.L = L;
	K.Za = K.Kol >= PadaCios && K.Rzad != RzadNarzedzi ? 1.f : FMath::FRandRange(0.f, 0.5f);
	K.bTam = false;
}

void AGaleria::Animuj(FKomorka& K, float Dt)
{
	ALudzik* L = K.L.Get();
	if (!L)
	{
		Ozyw(K);
		return;
	}
	const FVector2D Do = -Wglab;
	const FVector2D Patrz = K.P + Do * 1000.f;
	const FRodzajWojska& R = RodzajWojska(K.Rzad < Rodzajow ? ERodzajWojska(K.Rzad) : ERodzajWojska::Chlopi);
	K.Za -= Dt;
	const int32 Kol = K.Rzad == RzadNarzedzi ? int32(Cios) : K.Kol;
	switch (Kol)
	{
	case Idzie:
	case Biegnie:
	case Ranny:
		if (!L->WRuchu())
		{
			K.bTam = !K.bTam;
			L->WalkaIdz(K.P + Prawo * (K.bTam ? 160.f : -160.f), Kol == Biegnie ? 420.f : Kol == Idzie ? 260.f : 140.f);
		}
		break;
	case Cios:
		if (K.Za <= 0.f)
		{
			K.Za = R.Strzela() && K.Rzad != RzadNarzedzi ? 2.4f : 1.5f;
			R.Strzela() && K.Rzad != RzadNarzedzi ? L->WalkaStrzal(K.P + Do * 3000.f) : L->WalkaCios(Patrz);
		}
		break;
	case Blok:
		if (K.Za <= 0.f)
		{
			K.Za = 1.3f;
			L->WalkaBlok();
			L->WalkaWstrzas();
		}
		break;
	case Trafiony:
	case TrafionyStrzala:
		if (K.Za <= 0.f)
		{
			K.Za = 2.f;
			L->WalkaTrafiony(Wglab, Kol == TrafionyStrzala);
			if (Kol == TrafionyStrzala)
			{
				L->WbijStrzale(Wglab, false, false);
			}
		}
		break;
	case Okrzyk:
		if (K.Za <= 0.f)
		{
			K.Za = 3.f;
			L->WalkaOkrzyk(0.01f);
		}
		break;
	case PadaCios:
	case PadaStrzala:
	case Sciety:
	case BezReki:
		if (K.Za <= 0.f)
		{
			if (L->Lezy())
			{
				Ozyw(K); // po 6 s wstaje nowy i pada znowu
				return;
			}
			L->WalkaPadnij(Wglab, Kol == PadaStrzala, Kol == PadaCios);
			if (Kol == Sciety || Kol == BezReki)
			{
				L->Odetnij(Kol == Sciety ? 0 : 2, K.P - Wglab * 200.f);
			}
			K.Za = 6.f;
		}
		break;
	default:
		break;
	}
}

void AGaleria::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	for (FKomorka& K : Komorki)
	{
		Animuj(K, DeltaSeconds);
	}
	if (!ZawalS.IsZero())
	{
		ZawalZa -= DeltaSeconds;
		if (ZawalZa <= 0.f)
		{
			ZawalZa = 12.f;
			if (AEfekty* E = AEfekty::W(GetWorld()))
			{
				E->Zawal(ZawalS, PozarPol, FMath::RadiansToDegrees(FMath::Atan2(Prawo.Y, Prawo.X)), 450.f, false);
			}
		}
	}
}
