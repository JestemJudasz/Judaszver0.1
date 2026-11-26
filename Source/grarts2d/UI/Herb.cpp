// Plik: Source/grarts2d/UI/Herb.cpp
#include "UI/Herb.h"
#include "UI/StylUI.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

FHerb FHerb::Rodu(const FString& Nazwa, FLinearColor Barwa)
{
	const uint32 H = GetTypeHash(Nazwa);
	FHerb R;
	R.Wzor = uint8(H % 8);
	R.Pole = Barwa;
	// Druga barwa (metal albo czern): zloto, srebro albo czern, kontrast z polem.
	const float Jas = Barwa.GetLuminance();
	static const FLinearColor Metale[] = { FLinearColor(0.86f, 0.68f, 0.22f), FLinearColor(0.88f, 0.87f, 0.82f), FLinearColor(0.06f, 0.05f, 0.05f) };
	const int32 k = int32((H >> 8) % 3);
	R.Godlo = Jas > 0.45f ? Metale[2] : Metale[k == 2 ? 0 : k];
	return R;
}

FHerb FHerb::Gracza()
{
	FHerb R;
	R.Wzor = 6; // krokiew
	R.Pole = FLinearColor(0.62f, 0.10f, 0.07f);
	R.Godlo = FLinearColor(0.88f, 0.70f, 0.25f);
	return R;
}

void FHerb::Rysuj(FSlateWindowElementList& Out, int32 L, const FGeometry& G, FVector2f P, float S, float Alfa) const
{
	const FSlateBrush* Bialy = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	const float Wys = S * 1.2f;
	const int32 Rzedow = FMath::Clamp(FMath::RoundToInt32(Wys / 1.5f), 8, 60);
	const float Krok = Wys / Rzedow;
	auto Pas = [&](int32 Lw, float X0, float X1, float Y, FLinearColor K)
	{
		if (X1 - X0 > 0.05f)
		{
			K.A *= Alfa;
			FSlateDrawElement::MakeBox(Out, Lw, G.ToPaintGeometry(FVector2f(X1 - X0, Krok + 0.35f), FSlateLayoutTransform(FVector2f(X0, Y))),
				Bialy, ESlateDrawEffect::None, K);
		}
	};
	// Ksztalt: prosta gora, od 55% wysokosci boki zbiegaja sie lukiem do ostrza.
	auto Szer = [&](float t)
	{
		if (t < 0.55f) { return 1.f; }
		const float u = (t - 0.55f) / 0.45f;
		return FMath::Max(0.f, FMath::Sqrt(FMath::Max(0.f, 1.f - u * u)));
	};
	const float Obr = FMath::Max(1.f, S * 0.07f);
	const FLinearColor Czern(0.05f, 0.04f, 0.03f, 0.95f);
	// Obrys: ta sama tarcza troche wieksza, pod spodem.
	for (int32 r = -1; r <= Rzedow; ++r)
	{
		const float t = FMath::Clamp((r + 0.5f) / Rzedow, 0.f, 1.f);
		const float Pol = 0.5f * S * Szer(t) + Obr;
		Pas(L, P.X + 0.5f * S - Pol, P.X + 0.5f * S + Pol, P.Y + r * Krok, Czern);
	}
	const float C = P.X + 0.5f * S;
	const float Band = S * 0.14f;
	for (int32 r = 0; r < Rzedow; ++r)
	{
		const float t = (r + 0.5f) / Rzedow;
		const float Pol = 0.5f * S * Szer(t);
		if (Pol <= 0.1f) { continue; }
		const float X0 = C - Pol, X1 = C + Pol, Y = P.Y + r * Krok;
		const float Yl = t * Wys; // od gory tarczy
		const FLinearColor A = Pole, B = Godlo;
		switch (Wzor)
		{
		case 0: Pas(L + 1, X0, C, Y, A); Pas(L + 1, C, X1, Y, B); break;
		case 1: Pas(L + 1, X0, X1, Y, Yl < Wys * 0.45f ? B : A); break;
		case 2:
			if (FMath::Abs(Yl - Wys * 0.4f) < Band) { Pas(L + 1, X0, X1, Y, B); }
			else { Pas(L + 1, X0, C - Band, Y, A); Pas(L + 1, C - Band, C + Band, Y, B); Pas(L + 1, C + Band, X1, Y, A); }
			break;
		case 3:
		{
			const float Sk = FMath::Clamp(P.X + Yl / Wys * S, X0, X1);
			Pas(L + 1, X0, Sk, Y, B); Pas(L + 1, Sk, X1, Y, A);
			break;
		}
		case 4: Pas(L + 1, X0, X1, Y, FMath::Abs(Yl - Wys * 0.42f) < Band * 1.3f ? B : A); break;
		case 5: Pas(L + 1, X0, C - Band * 1.3f, Y, A); Pas(L + 1, C - Band * 1.3f, C + Band * 1.3f, Y, B); Pas(L + 1, C + Band * 1.3f, X1, Y, A); break;
		case 6:
		{
			// Krokiew: odwrocone V od gory srodka w dol do bokow.
			const float D = (Yl - Wys * 0.18f) * 0.9f; // odleglosc ramion od srodka w tym rzedzie
			if (D < -Band)
			{
				Pas(L + 1, X0, X1, Y, A);
			}
			else
			{
				const float W0 = FMath::Max(0.f, D - Band), W1 = D + Band;
				Pas(L + 1, X0, FMath::Clamp(C - W1, X0, X1), Y, A);
				Pas(L + 1, FMath::Clamp(C - W1, X0, X1), FMath::Clamp(C - W0, X0, X1), Y, B);
				Pas(L + 1, FMath::Clamp(C - W0, X0, X1), FMath::Clamp(C + W0, X0, X1), Y, A);
				Pas(L + 1, FMath::Clamp(C + W0, X0, X1), FMath::Clamp(C + W1, X0, X1), Y, B);
				Pas(L + 1, FMath::Clamp(C + W1, X0, X1), X1, Y, A);
			}
			break;
		}
		default:
		{
			const bool bGora = Yl < Wys * 0.45f;
			Pas(L + 1, X0, C, Y, bGora ? A : B); Pas(L + 1, C, X1, Y, bGora ? B : A);
			break;
		}
		}
	}
	// Polysk w lewym gornym rogu (lekko wypukla tarcza).
	Pas(L + 2, P.X + S * 0.08f, P.X + S * 0.4f, P.Y + Krok, FLinearColor(1.f, 1.f, 1.f, 0.18f));
}

void FHerb::Choragiew(FSlateWindowElementList& Out, int32 L, const FGeometry& G, FVector2f Stopa, float S, const FString& Napis,
	const FString& Podpis, FLinearColor KolorPodpisu) const
{
	const FSlateBrush* Bialy = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	const float Drzewce = S * 2.4f;
	// Cien pod stopa, drzewce, poprzeczka, tarcza zawieszona na poprzeczce.
	FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(FVector2f(S * 0.9f, 4.f), FSlateLayoutTransform(Stopa - FVector2f(S * 0.45f, 2.f))),
		Bialy, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	FSlateDrawElement::MakeBox(Out, L + 1, G.ToPaintGeometry(FVector2f(3.f, Drzewce), FSlateLayoutTransform(Stopa - FVector2f(1.5f, Drzewce))),
		Bialy, ESlateDrawEffect::None, FLinearColor(0.22f, 0.14f, 0.07f));
	FSlateDrawElement::MakeBox(Out, L + 1, G.ToPaintGeometry(FVector2f(S + 6.f, 3.f), FSlateLayoutTransform(Stopa + FVector2f(-S * 0.5f - 3.f, -Drzewce + 2.f))),
		Bialy, ESlateDrawEffect::None, FLinearColor(0.22f, 0.14f, 0.07f));
	Rysuj(Out, L + 2, G, Stopa + FVector2f(-S * 0.5f, -Drzewce + 6.f), S);
	// Napisy obok drzewca na ciemnym tle.
	const FSlateFontInfo Gruba = FCoreStyle::GetDefaultFontStyle("Bold", 10);
	const FSlateFontInfo Cienka = FCoreStyle::GetDefaultFontStyle("Regular", 9);
	const TSharedRef<FSlateFontMeasure> Miara = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const float Szer = FMath::Max(Miara->Measure(Napis, Gruba).X, Miara->Measure(Podpis, Cienka).X) + 12.f;
	const FVector2f T = Stopa + FVector2f(S * 0.5f + 8.f, -Drzewce + 4.f);
	FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(FVector2f(Szer, Podpis.IsEmpty() ? 18.f : 32.f), FSlateLayoutTransform(T - FVector2f(4.f, 1.f))),
		Bialy, ESlateDrawEffect::None, FLinearColor(0.06f, 0.05f, 0.04f, 0.78f));
	FSlateDrawElement::MakeText(Out, L + 1, G.ToPaintGeometry(FVector2f(Szer, 16.f), FSlateLayoutTransform(T)), Napis, Gruba, ESlateDrawEffect::None, StylUI::Napis);
	if (!Podpis.IsEmpty())
	{
		FSlateDrawElement::MakeText(Out, L + 1, G.ToPaintGeometry(FVector2f(Szer, 14.f), FSlateLayoutTransform(T + FVector2f(0.f, 15.f))), Podpis, Cienka,
			ESlateDrawEffect::None, KolorPodpisu);
	}
}
