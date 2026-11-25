// Plik: Source/grarts2d/Wojna/PanelBitwy.cpp
#include "Wojna/PanelBitwy.h"
#include "Wojna/Bitwa.h"
#include "Ludzie/Ludzik.h"
#include "Osada/Osada.h"
#include "Swiat/TerenKafle.h"
#include "UI/StylUI.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	const FSlateBrush* Bialy() { return FCoreStyle::Get().GetBrush("GenericWhiteBox"); }

	void Pole(FSlateWindowElementList& Out, int32 L, const FGeometry& G, FVector2f P, FVector2f R, FLinearColor K)
	{
		FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(R, FSlateLayoutTransform(P)), Bialy(), ESlateDrawEffect::None, K);
	}

	void Ramka(FSlateWindowElementList& Out, int32 L, const FGeometry& G, FVector2f P, FVector2f R, float Gr, FLinearColor K)
	{
		Pole(Out, L, G, P, FVector2f(R.X, Gr), K);
		Pole(Out, L, G, P + FVector2f(0.f, R.Y - Gr), FVector2f(R.X, Gr), K);
		Pole(Out, L, G, P, FVector2f(Gr, R.Y), K);
		Pole(Out, L, G, P + FVector2f(R.X - Gr, 0.f), FVector2f(Gr, R.Y), K);
	}

	void Napis(FSlateWindowElementList& Out, int32 L, const FGeometry& G, FVector2f P, const FString& T, int32 Rozmiar, FLinearColor K, bool bGruby = true)
	{
		FSlateDrawElement::MakeText(Out, L, G.ToPaintGeometry(FVector2f(300.f, Rozmiar + 6.f), FSlateLayoutTransform(P)), T,
			FCoreStyle::GetDefaultFontStyle(bGruby ? "Bold" : "Regular", Rozmiar), ESlateDrawEffect::None, K);
	}

	/** Pikselowy woj (16 x 26 "pikseli" po Px punktow): nogi, tunika, rece, glowa, helm (gracz) / kaptur, wlocznia albo topor. */
	void RysujWoja(FSlateWindowElementList& Out, int32 L, const FGeometry& G, FVector2f O, float Px, FLinearColor Tunika, EIkonaBroni Bron, bool bHelm)
	{
		auto P = [&](float X, float Y, float W, float H, FLinearColor K) { Pole(Out, L, G, O + FVector2f(X, Y) * Px, FVector2f(W, H) * Px, K); };
		const FLinearColor Skora(0.86f, 0.66f, 0.50f), Ciemny(0.16f, 0.11f, 0.07f), Drewno(0.45f, 0.30f, 0.16f), Zelazo(0.70f, 0.72f, 0.74f);
		const FLinearColor Cien = Tunika * 0.7f;
		// Bron za postacia (rodzaj wojska, W2).
		switch (Bron)
		{
		case EIkonaBroni::Topor:
			P(12, 4, 1, 14, Drewno);
			P(12, 3, 3, 4, Zelazo);
			P(14, 4, 1, 2, Zelazo * 0.8f);
			break;
		case EIkonaBroni::Miecz:
			P(12.5f, 2, 1, 13, Zelazo * 1.1f); // glownia w gore z dloni
			P(11, 14, 4, 1, Drewno * 0.7f);    // jelec
			P(12.5f, 15, 1, 2, Ciemny);        // rekojesc
			break;
		case EIkonaBroni::Palka:
			P(12.5f, 6, 1, 11, Drewno);
			P(11.5f, 2, 3, 5, Drewno * 0.75f);
			P(11, 3, 1, 1, Zelazo);
			P(14.5f, 4, 1, 1, Zelazo);
			break;
		case EIkonaBroni::Luk:
			P(15, 3, 1, 18, Drewno);           // luczysko
			P(14, 2, 1, 2, Drewno);
			P(14, 20, 1, 2, Drewno);
			P(13.6f, 3, 0.35f, 18, FLinearColor(0.85f, 0.82f, 0.74f)); // cieciwa
			P(-1, 8, 3, 1, Drewno * 0.7f);     // kolczan na plecach
			P(-1, 6, 1, 3, FLinearColor(0.85f, 0.82f, 0.74f));
			break;
		case EIkonaBroni::Kusza:
			P(8, 14, 8, 1.5f, Drewno);         // loze
			P(15, 11.5f, 1, 7, Drewno * 0.75f); // luczysko
			P(14, 12, 0.4f, 6, FLinearColor(0.85f, 0.82f, 0.74f));
			break;
		default:
			P(13, 0, 1, 26, Drewno);
			P(12.5f, -3, 2, 4, Zelazo);
			break;
		}
		P(5, 19, 2, 6, Ciemny);          // nogi
		P(9, 19, 2, 6, Ciemny);
		P(4, 25, 3, 1, Ciemny * 0.6f);   // buty
		P(9, 25, 3, 1, Ciemny * 0.6f);
		P(4, 10, 8, 10, Tunika);         // tunika
		P(4, 17, 8, 3, Cien);
		P(4, 15, 8, 1, Drewno);          // pas
		P(2, 10, 2, 7, Cien);            // rece
		P(12, 10, 2, 6, Cien);
		P(2, 17, 2, 2, Skora);
		P(12, 15, 2, 2, Skora);
		P(5, 4, 6, 6, Skora);            // glowa
		P(6, 9, 4, 1, Ciemny * 1.3f);    // broda
		if (bHelm)
		{
			P(5, 3, 6, 2, Zelazo);       // szyszak
			P(7, 1, 2, 2, Zelazo);
			P(7.5f, 5, 1, 3, Zelazo * 0.8f); // nosal
		}
		else
		{
			P(4, 3, 8, 3, Cien * 0.8f);  // kaptur
			P(4, 6, 1, 3, Cien * 0.8f);
			P(11, 6, 1, 3, Cien * 0.8f);
		}
		P(6, 6, 1, 1, Ciemny);           // oczy
		P(9, 6, 1, 1, Ciemny);
	}

	FLinearColor KolorDucha(const FOddzial& O)
	{
		const float M = FMath::Clamp(O.Morale / 100.f, 0.f, 1.f);
		return O.bZlamany ? StylUI::Czerwony : FMath::Lerp(StylUI::Czerwony, StylUI::Zielony, M);
	}

	int32 Weterani(const FOddzial& O)
	{
		int32 S = 0, N = 0;
		for (const FWojownik& W : O.Ludzie) { if (W.bZyje) { S += W.Doswiadczenie; ++N; } }
		return N ? FMath::RoundToInt32(float(S) / N) : 0;
	}

	/** Karta oddzialu (jak w Total War): choragiewka, postac, liczba ludzi, pasek ducha, stan. */
	class SKartaOddzialu : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SKartaOddzialu) {}
			SLATE_ARGUMENT(TWeakObjectPtr<ABitwa>, Bitwa)
			SLATE_ARGUMENT(int32, Indeks)
			SLATE_ARGUMENT(int32, Numer)
		SLATE_END_ARGS()

		void Construct(const FArguments& A) { BitwaW = A._Bitwa; Indeks = A._Indeks; Numer = A._Numer; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(78.0, 126.0); }

		virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& E) override
		{
			if (ABitwa* B = BitwaW.Get(); B && E.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				B->Wybierz(Indeks, E.IsShiftDown());
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}
		virtual FReply OnMouseButtonDoubleClick(const FGeometry&, const FPointerEvent&) override
		{
			if (ABitwa* B = BitwaW.Get(); B && B->Oddzialy().IsValidIndex(Indeks))
			{
				B->PokazKamere(B->Oddzialy()[Indeks].Srodek);
			}
			return FReply::Handled();
		}

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L,
			const FWidgetStyle&, bool) const override
		{
			const ABitwa* B = BitwaW.Get();
			if (!B || !B->Oddzialy().IsValidIndex(Indeks))
			{
				return L;
			}
			const FOddzial& O = B->Oddzialy()[Indeks];
			const FVector2f R = FVector2f(G.GetLocalSize());
			const bool bWyb = B->Wybrany(Indeks);
			const bool bPoza = O.bPoza || O.NaPolu() == 0;
			Pole(Out, L, G, FVector2f(0.f), R, IsHovered() ? StylUI::PrzyciskNad : StylUI::Kafel);
			// Tlo postaci: niebo nad trawa (jak pole bitwy), choragiewka w barwie oddzialu.
			Pole(Out, L + 1, G, FVector2f(4.f, 12.f), FVector2f(R.X - 8.f, 54.f), FLinearColor(0.30f, 0.36f, 0.30f));
			Pole(Out, L + 1, G, FVector2f(4.f, 66.f), FVector2f(R.X - 8.f, 18.f), FLinearColor(0.16f, 0.26f, 0.12f));
			Pole(Out, L + 1, G, FVector2f(4.f, 4.f), FVector2f(R.X - 8.f, 7.f), O.Barwa);
			RysujWoja(Out, L + 2, G, FVector2f(R.X * 0.5f - 20.f, 18.f), 2.5f, O.Tunika, O.Ikona, O.Strona == 0);
			Napis(Out, L + 3, G, FVector2f(6.f, 12.f), FString::FromInt(Numer), 9, StylUI::Zloto);
			// Liczba ludzi i pasek ducha.
			Napis(Out, L + 2, G, FVector2f(6.f, 94.f), FString::Printf(TEXT("%d"), O.NaPolu()), 12, StylUI::Napis);
			Napis(Out, L + 2, G, FVector2f(30.f, 98.f), FString::Printf(TEXT("/%d"), O.Poczatkowo), 8, StylUI::Szary, false);
			for (int32 s = 0; s < Weterani(O); ++s)
			{
				Pole(Out, L + 2, G, FVector2f(R.X - 12.f - s * 7.f, 98.f), FVector2f(5.f, 5.f), StylUI::Zloto); // doswiadczenie
			}
			const float M = FMath::Clamp(O.Morale / 100.f, 0.f, 1.f);
			Pole(Out, L + 2, G, FVector2f(6.f, 114.f), FVector2f(R.X - 12.f, 5.f), FLinearColor(0.f, 0.f, 0.f, 0.9f));
			Pole(Out, L + 3, G, FVector2f(6.f, 114.f), FVector2f((R.X - 12.f) * M, 5.f), KolorDucha(O));
			Napis(Out, L + 3, G, FVector2f(6.f, 84.f), ABitwa::OpisStanu(O), 7, O.bZlamany ? StylUI::Czerwony : StylUI::Napis);
			if (O.Flanka > 0.4f && !O.bZlamany)
			{
				Ramka(Out, L + 3, G, FVector2f(4.f, 12.f), FVector2f(R.X - 8.f, 72.f), 2.f, StylUI::Czerwony); // bity z boku
			}
			if (bPoza)
			{
				Pole(Out, L + 4, G, FVector2f(0.f), R, FLinearColor(0.f, 0.f, 0.f, 0.55f));
			}
			Ramka(Out, L + 4, G, FVector2f(0.f), R, bWyb ? 3.f : 1.f, bWyb ? StylUI::Zloto : StylUI::Ramka);
			return L + 5;
		}

	private:
		TWeakObjectPtr<ABitwa> BitwaW;
		int32 Indeks = 0;
		int32 Numer = 1;
	};

	/** Portret wybranego oddzialu (po lewej): duza postac, nazwa i liczby. */
	class SPortret : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPortret) {}
			SLATE_ARGUMENT(TWeakObjectPtr<ABitwa>, Bitwa)
		SLATE_END_ARGS()

		void Construct(const FArguments& A) { BitwaW = A._Bitwa; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(178.0, 172.0); }

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L,
			const FWidgetStyle&, bool) const override
		{
			const ABitwa* B = BitwaW.Get();
			if (!B)
			{
				return L;
			}
			const FVector2f R = FVector2f(G.GetLocalSize());
			int32 Wyb = -1, Ile = 0;
			for (int32 i = 0; i < B->Oddzialy().Num(); ++i)
			{
				if (B->Wybrany(i)) { Wyb = Wyb < 0 ? i : Wyb; ++Ile; }
			}
			Napis(Out, L, G, FVector2f(4.f, 0.f), TEXT("DOWÓDZTWO"), 9, StylUI::Zloto);
			Pole(Out, L, G, FVector2f(4.f, 18.f), FVector2f(74.f, 96.f), FLinearColor(0.30f, 0.36f, 0.30f));
			Pole(Out, L, G, FVector2f(4.f, 94.f), FVector2f(74.f, 20.f), FLinearColor(0.16f, 0.26f, 0.12f));
			Ramka(Out, L + 1, G, FVector2f(4.f, 18.f), FVector2f(74.f, 96.f), 1.f, StylUI::Ramka);
			if (Wyb < 0)
			{
				Napis(Out, L + 1, G, FVector2f(86.f, 24.f), TEXT("Wybierz\noddział"), 9, StylUI::Szary, false);
				return L + 2;
			}
			const FOddzial& O = B->Oddzialy()[Wyb];
			RysujWoja(Out, L + 1, G, FVector2f(13.f, 24.f), 3.5f, O.Tunika, O.Ikona, true);
			Pole(Out, L + 1, G, FVector2f(4.f, 18.f), FVector2f(74.f, 5.f), O.Barwa);
			const FString Nazwa = Ile > 1 ? FString::Printf(TEXT("%d oddziały"), Ile) : O.Nazwa;
			Napis(Out, L + 1, G, FVector2f(4.f, 118.f), Nazwa, 10, StylUI::Napis);
			int32 Lud = 0, Pocz = 0;
			float Duch = 0.f, Zm = 0.f;
			for (int32 i = 0; i < B->Oddzialy().Num(); ++i)
			{
				if (B->Wybrany(i))
				{
					const FOddzial& X = B->Oddzialy()[i];
					Lud += X.NaPolu(); Pocz += X.Poczatkowo; Duch += X.Morale / Ile; Zm += X.Zmeczenie / Ile;
				}
			}
			const FString Staty = FString::Printf(TEXT("Ludzie %d/%d\nDuch %d%%\nZmęcz. %d%%\n%s"), Lud, Pocz, FMath::RoundToInt32(Duch),
				FMath::RoundToInt32(Zm), Ile > 1 ? TEXT("") : ABitwa::OpisStanu(O));
			Napis(Out, L + 1, G, FVector2f(86.f, 20.f), Staty, 9, StylUI::Napis, false);
			const TCHAR* Stopien[] = { TEXT("rekruci"), TEXT("wojacy"), TEXT("weterani") };
			Napis(Out, L + 1, G, FVector2f(86.f, 88.f), Stopien[FMath::Clamp(Weterani(O), 0, 2)], 9, StylUI::Zloto, false);
			Napis(Out, L + 1, G, FVector2f(4.f, 140.f), O.bZwarty ? TEXT("Szyk zwarty") : TEXT("Szyk luźny"), 8, StylUI::Szary, false);
			if (RodzajWojska(O.Rodzaj).Strzela())
			{
				Napis(Out, L + 1, G, FVector2f(4.f, 154.f), FString::Printf(TEXT("%s: %d"), O.Rodzaj == ERodzajWojska::Kusznicy ? TEXT("Bełty") : TEXT("Strzały"), O.Strzal()), 8, StylUI::Zloto, false);
			}
			return L + 2;
		}

	private:
		TWeakObjectPtr<ABitwa> BitwaW;
	};

	/** Mapka pola bitwy: kropka na woja; gora = polnoc (+X), jak kamera na starcie. Klik = kamera tam. */
	class SMinimapa : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SMinimapa) {}
			SLATE_ARGUMENT(TWeakObjectPtr<ABitwa>, Bitwa)
		SLATE_END_ARGS()

		void Construct(const FArguments& A) { BitwaW = A._Bitwa; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(172.0, 172.0); }

		bool Obszar(FVector2D& Srodek, float& Pol) const
		{
			const ABitwa* B = BitwaW.Get();
			FBox2D Box(ForceInit);
			if (B)
			{
				for (const FOddzial& O : B->Oddzialy())
				{
					for (const FWojownik& W : O.Ludzie)
					{
						if (W.L.IsValid() && !W.bPoza) { Box += FVector2D(W.L->GetActorLocation()); }
					}
				}
			}
			if (!Box.bIsValid)
			{
				return false;
			}
			Srodek = Box.GetCenter();
			Pol = FMath::Max(3000.f, float(Box.GetExtent().GetMax()) + 1500.f);
			return true;
		}

		virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override
		{
			FVector2D S;
			float P;
			ABitwa* B = BitwaW.Get();
			if (B && Obszar(S, P))
			{
				const FVector2D Lok = G.AbsoluteToLocal(E.GetScreenSpacePosition()) / G.GetLocalSize();
				B->PokazKamere(S + FVector2D((0.5 - Lok.Y) * 2.0 * P, (Lok.X - 0.5) * 2.0 * P));
			}
			return FReply::Handled();
		}

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L,
			const FWidgetStyle&, bool) const override
		{
			const FVector2f R = FVector2f(G.GetLocalSize());
			Pole(Out, L, G, FVector2f(0.f), R, FLinearColor(0.17f, 0.25f, 0.12f));
			// Lekka siatka (co 10 m) zeby bylo widac skale i ruch.
			FVector2D S;
			float P;
			const ABitwa* B = BitwaW.Get();
			if (B && Obszar(S, P))
			{
				const float Skala = R.X / (2.f * P);
				for (float K = -P + FMath::Fmod(P, 1000.f); K < P; K += 1000.f)
				{
					Pole(Out, L + 1, G, FVector2f((K + P) * Skala, 0.f), FVector2f(1.f, R.Y), FLinearColor(0.f, 0.f, 0.f, 0.12f));
					Pole(Out, L + 1, G, FVector2f(0.f, (K + P) * Skala), FVector2f(R.X, 1.f), FLinearColor(0.f, 0.f, 0.f, 0.12f));
				}
				for (int32 i = 0; i < B->Oddzialy().Num(); ++i)
				{
					const FOddzial& O = B->Oddzialy()[i];
					const bool bWyb = B->Wybrany(i);
					for (const FWojownik& W : O.Ludzie)
					{
						if (!W.L.IsValid() || W.bPoza)
						{
							continue;
						}
						const FVector2D Q = FVector2D(W.L->GetActorLocation()) - S;
						const FVector2f M((Q.Y + P) * Skala, (P - Q.X) * Skala);
						const FLinearColor K = !W.bZyje ? FLinearColor(0.f, 0.f, 0.f, 0.35f) : bWyb ? StylUI::Zloto
							: O.Strona == 0 ? FLinearColor(0.85f, 0.2f, 0.12f) : FLinearColor(0.05f, 0.05f, 0.06f);
						Pole(Out, L + 2, G, M - FVector2f(1.5f), FVector2f(3.f), K);
					}
				}
			}
			Ramka(Out, L + 3, G, FVector2f(0.f), R, 1.f, StylUI::Ramka);
			Napis(Out, L + 3, G, FVector2f(4.f, 2.f), TEXT("N"), 8, StylUI::Zloto);
			return L + 4;
		}

	private:
		TWeakObjectPtr<ABitwa> BitwaW;
	};

	/** Ikona rozkazu z linii (pikselowa, zlota): atak, stoj, szyk, odwrot, pauza. */
	class SIkonaRozkazu : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SIkonaRozkazu) {}
			SLATE_ARGUMENT(int32, Rodzaj)
		SLATE_END_ARGS()

		void Construct(const FArguments& A) { Rodzaj = A._Rodzaj; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(22.0, 22.0); }

		virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L,
			const FWidgetStyle&, bool) const override
		{
			const FLinearColor K = StylUI::Zloto;
			auto Linia = [&](TArray<FVector2f> Pkt, float Gr)
			{
				FSlateDrawElement::MakeLines(Out, L, G.ToPaintGeometry(), Pkt, ESlateDrawEffect::None, K, false, Gr);
			};
			switch (Rodzaj)
			{
			case 0: // atak: skrzyzowane wlocznie
				Linia({ FVector2f(3, 19), FVector2f(19, 3) }, 2.f);
				Linia({ FVector2f(3, 3), FVector2f(19, 19) }, 2.f);
				Linia({ FVector2f(14, 3), FVector2f(19, 3), FVector2f(19, 8) }, 2.f);
				Linia({ FVector2f(3, 8), FVector2f(3, 3), FVector2f(8, 3) }, 2.f);
				break;
			case 1: // stoj: tarcza
				Linia({ FVector2f(4, 3), FVector2f(18, 3), FVector2f(18, 11), FVector2f(11, 19), FVector2f(4, 11), FVector2f(4, 3) }, 2.f);
				Linia({ FVector2f(11, 4), FVector2f(11, 17) }, 1.5f);
				break;
			case 2: // szyk: trzy szeregi
				for (int32 r = 0; r < 3; ++r) { for (int32 c = 0; c < 3; ++c) { Pole(Out, L, G, FVector2f(4.f + c * 6.f, 4.f + r * 6.f), FVector2f(3.f), K); } }
				break;
			case 3: // odwrot: strzalka w tyl
				Linia({ FVector2f(19, 11), FVector2f(4, 11) }, 2.f);
				Linia({ FVector2f(10, 5), FVector2f(4, 11), FVector2f(10, 17) }, 2.f);
				break;
			default: // pauza
				Pole(Out, L, G, FVector2f(6.f, 4.f), FVector2f(3.f, 14.f), K);
				Pole(Out, L, G, FVector2f(13.f, 4.f), FVector2f(3.f, 14.f), K);
				break;
			}
			return L + 1;
		}

	private:
		int32 Rodzaj = 0;
	};
	/** Panel do przesuwania i skalowania (jak w Total War): ciagnij za puste miejsce panelu, rog w prawym dole zmienia
	 *  wielkosc, podwojny klik w puste miejsce przywraca. Polozenie i skala zapamietane w ustawieniach gracza. */
	class SPrzesuwne : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPrzesuwne) {}
			SLATE_ARGUMENT(FString, Nazwa)
			SLATE_DEFAULT_SLOT(FArguments, Tresc)
		SLATE_END_ARGS()

		void Construct(const FArguments& A)
		{
			Nazwa = A._Nazwa;
			GConfig->GetFloat(TEXT("PanelBitwy"), *(Nazwa + TEXT("X")), Przes.X, GGameUserSettingsIni);
			GConfig->GetFloat(TEXT("PanelBitwy"), *(Nazwa + TEXT("Y")), Przes.Y, GGameUserSettingsIni);
			GConfig->GetFloat(TEXT("PanelBitwy"), *(Nazwa + TEXT("Skala")), Skala, GGameUserSettingsIni);
			Skala = FMath::Clamp(Skala, 0.6f, 1.8f);
			SetRenderTransform(FSlateRenderTransform(Przes));
			ChildSlot
			[
				SNew(SDPIScaler).DPIScale_Lambda([this]() { return Skala; })
				[
					SNew(SOverlay)
					+ SOverlay::Slot()[A._Tresc.Widget]
					+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom)[SNew(SRog).Panel(this)]
				]
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry& G, const FPointerEvent& E) override
		{
			if (E.GetEffectingButton() != EKeys::LeftMouseButton)
			{
				return FReply::Unhandled();
			}
			Zacznij(E, false);
			return FReply::Handled().CaptureMouse(SharedThis(this));
		}
		virtual FReply OnMouseButtonDoubleClick(const FGeometry& G, const FPointerEvent& E) override
		{
			Przes = FVector2f::ZeroVector;
			Skala = 1.f;
			SetRenderTransform(FSlateRenderTransform(Przes));
			Zapisz();
			return FReply::Handled();
		}
		virtual FReply OnMouseMove(const FGeometry& G, const FPointerEvent& E) override
		{
			if (!HasMouseCapture())
			{
				return FReply::Unhandled();
			}
			Ruch(E);
			return FReply::Handled();
		}
		virtual FReply OnMouseButtonUp(const FGeometry& G, const FPointerEvent& E) override
		{
			if (!HasMouseCapture())
			{
				return FReply::Unhandled();
			}
			Zapisz();
			return FReply::Handled().ReleaseMouseCapture();
		}

		/** Poczatek ciagniecia (bSkala = rog: zmiana wielkosci). */
		void Zacznij(const FPointerEvent& E, bool bSkalaNowa)
		{
			bSkala = bSkalaNowa;
			MyszStart = FVector2f(E.GetScreenSpacePosition());
			PrzesStart = Przes;
			SkalaStart = Skala;
			// Piksele ekranu -> jednostki panelu (skala okna gry bez naszej skali).
			Jednostka = FMath::Max(0.01f, GetCachedGeometry().Scale);
		}
		void Ruch(const FPointerEvent& E)
		{
			const FVector2f D = (FVector2f(E.GetScreenSpacePosition()) - MyszStart) / Jednostka;
			if (bSkala)
			{
				Skala = FMath::Clamp(SkalaStart + (D.X + D.Y) / 500.f, 0.6f, 1.8f);
			}
			else
			{
				Przes = PrzesStart + D;
				SetRenderTransform(FSlateRenderTransform(Przes));
			}
		}
		void Zapisz() const
		{
			GConfig->SetFloat(TEXT("PanelBitwy"), *(Nazwa + TEXT("X")), Przes.X, GGameUserSettingsIni);
			GConfig->SetFloat(TEXT("PanelBitwy"), *(Nazwa + TEXT("Y")), Przes.Y, GGameUserSettingsIni);
			GConfig->SetFloat(TEXT("PanelBitwy"), *(Nazwa + TEXT("Skala")), Skala, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}

	private:
		/** Rog w prawym dole: ciagniecie zmienia wielkosc panelu. */
		class SRog : public SLeafWidget
		{
		public:
			SLATE_BEGIN_ARGS(SRog) {}
				SLATE_ARGUMENT(SPrzesuwne*, Panel)
			SLATE_END_ARGS()
			void Construct(const FArguments& A) { Panel = A._Panel; SetCursor(EMouseCursor::ResizeSouthEast); }
			virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(14.f, 14.f); }
			virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out, int32 L,
				const FWidgetStyle&, bool) const override
			{
				const FLinearColor K = IsHovered() || HasMouseCapture() ? StylUI::Zloto : StylUI::Ramka;
				for (int32 k = 0; k < 3; ++k)
				{
					const float O = 4.f * k;
					Pole(Out, L, G, FVector2f(11.f - O, 11.f), FVector2f(2.f, 2.f), K);
					Pole(Out, L, G, FVector2f(11.f, 11.f - O), FVector2f(2.f, 2.f), K);
				}
				Pole(Out, L, G, FVector2f(7.f, 7.f), FVector2f(2.f, 2.f), K);
				return L + 1;
			}
			virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& E) override
			{
				if (E.GetEffectingButton() != EKeys::LeftMouseButton || !Panel)
				{
					return FReply::Unhandled();
				}
				Panel->Zacznij(E, true);
				return FReply::Handled().CaptureMouse(SharedThis(this));
			}
			virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent& E) override
			{
				if (!HasMouseCapture() || !Panel)
				{
					return FReply::Unhandled();
				}
				Panel->Ruch(E);
				return FReply::Handled();
			}
			virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& E) override
			{
				if (!HasMouseCapture())
				{
					return FReply::Unhandled();
				}
				if (Panel) { Panel->Zapisz(); }
				return FReply::Handled().ReleaseMouseCapture();
			}
		private:
			SPrzesuwne* Panel = nullptr;
		};

		FString Nazwa;
		FVector2f Przes = FVector2f::ZeroVector;
		float Skala = 1.f;
		bool bSkala = false;
		FVector2f MyszStart = FVector2f::ZeroVector;
		FVector2f PrzesStart = FVector2f::ZeroVector;
		float SkalaStart = 1.f;
		float Jednostka = 1.f;
	};
}

TSharedRef<SWidget> ZrobPanelBitwy(TWeakObjectPtr<ABitwa> Ja)
{
	static FSlateRoundedBoxBrush Tlo(StylUI::Tlo, 6.f, StylUI::Ramka, 1.f);
	static FButtonStyle Przycisk = StylUI::StylPrzycisku();

	// Karty twoich oddzialow.
	TSharedRef<SHorizontalBox> Karty = SNew(SHorizontalBox);
	if (ABitwa* B = Ja.Get())
	{
		int32 Numer = 1;
		for (int32 i = 0; i < B->Oddzialy().Num(); ++i)
		{
			if (B->Oddzialy()[i].Strona == 0 && !B->Oddzialy()[i].bPoza && !B->Oddzialy()[i].bKoniec) // rozwiazane ruszenie: bez karty
			{
				Karty->AddSlot().AutoWidth().Padding(3.f, 0.f)[SNew(SKartaOddzialu).Bitwa(Ja).Indeks(i).Numer(Numer++)];
			}
		}
	}

	auto Guzik = [Ja](int32 Ikona, TFunction<FString()> Tekst, const TCHAR* Podpowiedz, TFunction<void(ABitwa*)> Akcja) -> TSharedRef<SWidget>
	{
		return SNew(SBox).Padding(FMargin(3.f, 0.f))
		[
			SNew(SButton)
			.ButtonStyle(&Przycisk)
			.ToolTipText(FText::FromString(Podpowiedz))
			.OnClicked_Lambda([Ja, Akcja]() { if (ABitwa* B = Ja.Get()) { Akcja(B); } return FReply::Handled(); })
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SIkonaRozkazu).Rodzaj(Ikona)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5.f, 0.f, 2.f, 0.f)
				[
					SNew(STextBlock).Text_Lambda([Tekst]() { return FText::FromString(Tekst()); })
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10)).ColorAndOpacity(StylUI::Napis)
				]
			]
		];
	};
	auto Staly = [](const TCHAR* T) { const FString S(T); return TFunction<FString()>([S]() { return S; }); };

	return SNew(SHorizontalBox)
		// Dowodztwo: portret wybranego oddzialu.
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(0.f, 0.f, 8.f, 0.f)
		[
			SNew(SPrzesuwne).Nazwa(TEXT("Dowodztwo"))[SNew(SBorder).BorderImage(&Tlo).Padding(8.f)[SNew(SPortret).Bitwa(Ja)]]
		]
		// Karty oddzialow i rozkazy.
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
		[
			SNew(SPrzesuwne).Nazwa(TEXT("Druzyna"))
			[
			SNew(SBorder).BorderImage(&Tlo).Padding(8.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(STextBlock)
					.Text_Lambda([Ja]() { return FText::FromString(Ja.IsValid() && Ja->Pauza() ? TEXT("DRUŻYNA — PAUZA") : TEXT("DRUŻYNA")); })
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11)).ColorAndOpacity(StylUI::Zloto)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Karty]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[Guzik(0, Staly(TEXT("Atak")), TEXT("Wybrane oddziały ruszają na najbliższego wroga"), [](ABitwa* B) { B->Rozkaz(ERozkaz::Atakuj); })]
					+ SHorizontalBox::Slot().AutoWidth()[Guzik(1, Staly(TEXT("Stój")), TEXT("Stań i trzymaj miejsce"), [](ABitwa* B) { B->Rozkaz(ERozkaz::Stoj); })]
					+ SHorizontalBox::Slot().AutoWidth()[Guzik(2, [Ja]() { return FString(Ja.IsValid() && Ja->SzykWybranychZwarty() ? TEXT("Zwarty") : TEXT("Luźny")); },
						TEXT("F - szyk zwarty (trzyma front) / luźny (szerzej)"), [](ABitwa* B) { B->PrzelaczSzyk(); })]
					+ SHorizontalBox::Slot().AutoWidth()[Guzik(3, Staly(TEXT("Odwrót")), TEXT("R - zerwij walkę i cofnij się 25 m (kosztuje ducha)"), [](ABitwa* B) { B->Rozkaz(ERozkaz::Odwrot); })]
					+ SHorizontalBox::Slot().AutoWidth()[Guzik(4, Staly(TEXT("Pauza")), TEXT("Spacja - pauza w czasie walki, rozkazy działają dalej"), [](ABitwa* B) { B->PrzelaczPauze(); })]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Lewy klik / ramka / 1-4 - wybór (Shift - kilka)   prawy klik - idź / atakuj (2× - biegiem, przeciągnij - szyk)   środkowy - przesuń mapę   K - krew   J - wojsko (rozwiąż)   panele: przeciągnij, róg = wielkość")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9)).ColorAndOpacity(StylUI::Szary)
				]
			]
			]
		]
		// Mapka pola bitwy.
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(8.f, 0.f, 0.f, 0.f)
		[
			SNew(SPrzesuwne).Nazwa(TEXT("Mapka"))[SNew(SBorder).BorderImage(&Tlo).Padding(8.f)[SNew(SMinimapa).Bitwa(Ja)]]
		];
}
