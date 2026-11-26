// Plik: Source/grarts2d/UI/ZnakiArmii.cpp
#include "UI/ZnakiArmii.h"
#include "UI/Herb.h"
#include "UI/StylUI.h"
#include "Osada/Rody.h"
#include "Wojna/Bitwa.h"
#include "Swiat/TerenKafle.h" // Bitwa.h: UstawTeren potrzebuje pelnego typu
#include "Kamera/KameraMapy.h"
#include "Swiat/SwiatWysokosci.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Osada/Osada.h"
#include "InputCoreTypes.h"

int32 SZnakiArmii::ArmiaPod(ARody* A, APlayerController* PC, FVector2D Mysz)
{
	const AKameraMapy* Kam = PC ? Cast<AKameraMapy>(PC->GetPawn()) : nullptr;
	if (!A || !Kam)
	{
		return -1;
	}
	for (const ARody::FArmia& Ar : A->Armie())
	{
		if (Ar.Cel >= 0 || Ar.bCzeka || Ar.bPrzechwycona || !A->Lista().IsValidIndex(Ar.Rod) || !A->Lista()[Ar.Rod].bWojna)
		{
			continue;
		}
		const FVector2D P = Ar.Gdzie();
		FVector2D E;
		if (!PC->ProjectWorldLocationToScreen(FVector(P.X, P.Y, Kam->WRegionie() ? FSwiat::Wysokosc(P.X, P.Y) : 0.0), E, false))
		{
			continue;
		}
		// choragiew stoi nad punktem kolumny: drzewiec w gore, napisy w prawo (z zapasem na skale ekranu)
		const FVector2D D = Mysz - E;
		if (D.X > -30.0 && D.X < 220.0 && D.Y > -110.0 && D.Y < 25.0)
		{
			return Ar.Id;
		}
	}
	return -1;
}

int32 SZnakiArmii::OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Clip, FSlateWindowElementList& Out,
	int32 L, const FWidgetStyle& Styl, bool bRodzic) const
{
	ARody* A = RodyW.Get();
	UWorld* W = A ? A->GetWorld() : nullptr;
	APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	const AKameraMapy* Kam = PC ? Cast<AKameraMapy>(PC->GetPawn()) : nullptr;
	if (!Kam)
	{
		return L;
	}
	const bool bRegion = Kam->WRegionie();
	const float Skala = FMath::Max(0.01f, G.Scale);
	const FVector2f Rozm = G.GetLocalSize();
	auto Rzut = [&](FVector2D P, FVector2f& E)
	{
		FVector2D Px;
		const double Z = bRegion ? FSwiat::Wysokosc(P.X, P.Y) : 0.0; // mapa papierowa jest plaska
		if (!PC->ProjectWorldLocationToScreen(FVector(P.X, P.Y, Z), Px, false))
		{
			return false;
		}
		E = FVector2f(Px / Skala);
		return E.X > -200.f && E.Y > -200.f && E.X < Rozm.X + 200.f && E.Y < Rozm.Y + 200.f;
	};
	// Przerywana linia od kolumny do celu (po ziemi, kilkanascie punktow).
	auto Droga = [&](FVector2D Od, FVector2D Do, FLinearColor K)
	{
		const int32 N = 24;
		TArray<FVector2f> Pkt;
		for (int32 k = 0; k <= N; ++k)
		{
			FVector2f E;
			if (Rzut(FMath::Lerp(Od, Do, double(k) / N), E)) { Pkt.Add(E); }
			else { Pkt.Reset(); }
			if (Pkt.Num() == 2)
			{
				if (k % 2 == 1)
				{
					FSlateDrawElement::MakeLines(Out, L, G.ToPaintGeometry(), Pkt, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.5f * K.A), true, 4.f);
					FSlateDrawElement::MakeLines(Out, L + 1, G.ToPaintGeometry(), Pkt, ESlateDrawEffect::None, K, true, 2.f);
				}
				Pkt.RemoveAt(0);
			}
		}
	};
	const float S = bRegion ? 24.f : 20.f;
	for (const ARody::FArmia& Ar : A->Armie())
	{
		if (!A->Lista().IsValidIndex(Ar.Rod))
		{
			continue;
		}
		const FRod& R = A->Lista()[Ar.Rod];
		const bool bNaNas = Ar.Cel < 0;
		const FVector2D P = Ar.Gdzie();
		Droga(P, Ar.Dokad, bNaNas ? FLinearColor(0.85f, 0.15f, 0.1f, 0.9f) : FLinearColor(0.9f, 0.85f, 0.7f, 0.6f));
		FVector2f E;
		if (!Rzut(P, E))
		{
			continue;
		}
		const FString Dokad = bNaNas ? FString(TEXT("na waszą osadę")) : (A->Lista().IsValidIndex(Ar.Cel) ? TEXT("na ") + A->Lista()[Ar.Cel].Nazwa : FString());
		FString Podpis = Ar.bCzeka ? FString::Printf(TEXT("pod waszą osadą! %d ludzi"), Ar.Woje + Ar.Chlopi)
			: Ar.bPrzechwycona ? FString::Printf(TEXT("czekają na bitwę w polu · %d ludzi"), Ar.Woje + Ar.Chlopi)
			: FString::Printf(TEXT("%s · %d ludzi · %.0f s"), *Dokad, Ar.Woje + Ar.Chlopi, FMath::Max(0.f, Ar.Caly - Ar.Czas));
		if (bNaNas && !Ar.bCzeka && !Ar.bPrzechwycona && R.bWojna)
		{
			Podpis += TEXT(" · kliknij: wyjdź naprzeciw");
		}
		FHerb::Rodu(R.Nazwa, R.Barwa).Choragiew(Out, L + 2, G, E, S, R.Nazwa, Podpis, bNaNas ? StylUI::Czerwony : StylUI::Szary);
	}
	// Wasza wyprawa w drodze.
	for (TActorIterator<ABitwa> It(W); It; ++It)
	{
		FVector2D P;
		float Postep;
		bool bPowrot;
		if (!It->MarszWyprawy(P, Postep, bPowrot))
		{
			continue;
		}
		const int32 RP = It->RodPrzechwycenia();
		const int32 RW = RP >= 0 ? RP : It->RodWyprawy();
		const FString Cel = A->Lista().IsValidIndex(RW) ? A->Lista()[RW].Nazwa : FString();
		int32 Ludzi = 0;
		for (const FOddzial& O : It->Oddzialy()) { Ludzi += O.Strona == 0 && O.Milicja >= 0 ? O.NaPolu() : 0; }
		if (A->Lista().IsValidIndex(RW) && !bPowrot)
		{
			Droga(P, RP >= 0 ? It->CelMarszu() : A->Lista()[RW].Gdzie, FLinearColor(0.9f, 0.7f, 0.3f, 0.8f));
		}
		FVector2f E;
		if (Rzut(P, E))
		{
			const TCHAR* Kto = bPowrot ? TEXT("Wasza wyprawa wraca") : (RP >= 0 ? TEXT("Wasze wojsko naprzeciw") : TEXT("Wasza wyprawa"));
			FHerb::Gracza().Choragiew(Out, L + 2, G, E, S, Kto,
				FString::Printf(TEXT("%s%s · %d%% drogi"), bPowrot ? TEXT("z ") : TEXT("na "), *Cel, FMath::RoundToInt32(Postep * 100.f)), StylUI::Zloto);
		}
	}
	// Punkt zbiorki ruszenia (mapa grywalna).
	if (bRegion)
	{
		for (TActorIterator<ABitwa> It(W); It; ++It)
		{
			FVector2f E;
			if (It->MaZbiorke() && Rzut(It->Zbiorka(), E))
			{
				FHerb::Gracza().Choragiew(Out, L + 2, G, E, 16.f, TEXT("Zbiórka ruszenia"), FString(), StylUI::Zloto);
			}
		}
	}
	// Baner wojny / pokoju: duzy napis u gory ekranu, znika po kilku sekundach.
	const double Teraz = W->GetRealTimeSeconds();
	if (!A->Baner.IsEmpty() && Teraz < A->BanerDo)
	{
		const float Alfa = FMath::Clamp(float(A->BanerDo - Teraz) / 1.5f, 0.f, 1.f);
		const bool bPokoj = A->Baner.StartsWith(TEXT("Pokój"));
		const FSlateFontInfo Duza = FCoreStyle::GetDefaultFontStyle("Bold", 26);
		const TSharedRef<FSlateFontMeasure> Miara = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const FVector2f Wym = FVector2f(Miara->Measure(A->Baner, Duza));
		const FVector2f P((Rozm.X - Wym.X) * 0.5f, Rozm.Y * 0.2f);
		const FSlateBrush* Bialy = FCoreStyle::Get().GetBrush("GenericWhiteBox");
		FSlateDrawElement::MakeBox(Out, L + 5, G.ToPaintGeometry(Wym + FVector2f(80.f, 22.f), FSlateLayoutTransform(P - FVector2f(40.f, 11.f))),
			Bialy, ESlateDrawEffect::None, FLinearColor(0.08f, 0.03f, 0.02f, 0.82f * Alfa));
		FSlateDrawElement::MakeBox(Out, L + 6, G.ToPaintGeometry(FVector2f(Wym.X + 80.f, 2.f), FSlateLayoutTransform(P - FVector2f(40.f, 11.f))),
			Bialy, ESlateDrawEffect::None, FLinearColor(0.85f, 0.66f, 0.32f, Alfa));
		FSlateDrawElement::MakeBox(Out, L + 6, G.ToPaintGeometry(FVector2f(Wym.X + 80.f, 2.f), FSlateLayoutTransform(P + FVector2f(-40.f, Wym.Y + 9.f))),
			Bialy, ESlateDrawEffect::None, FLinearColor(0.85f, 0.66f, 0.32f, Alfa));
		FLinearColor K = bPokoj ? StylUI::Zloto : FLinearColor(0.95f, 0.25f, 0.18f);
		K.A = Alfa;
		FSlateDrawElement::MakeText(Out, L + 7, G.ToPaintGeometry(Wym, FSlateLayoutTransform(P)), A->Baner, Duza, ESlateDrawEffect::None, K);
	}
	return L + 8;
}
