// Plik: Source/grarts2d/UI/Minimapa.cpp
#include "UI/Minimapa.h"
#include "Kamera/KameraMapy.h"
#include "Swiat/SwiatWysokosci.h"
#include "Budowy/Budowa.h"
#include "Osada/Rody.h"
#include "EngineUtils.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	constexpr int32 PikseliMapy = 112; // pikseli tekstury na bok

	FLinearColor KolorPokrycia(FSwiat::EPokrycie P)
	{
		using E = FSwiat::EPokrycie;
		switch (P)
		{
		case E::Woda: return FLinearColor(0.20f, 0.33f, 0.40f);
		case E::Laka: return FLinearColor(0.42f, 0.50f, 0.27f);
		case E::Wrzos: return FLinearColor(0.45f, 0.42f, 0.32f);
		case E::Lisciasty: return FLinearColor(0.25f, 0.37f, 0.18f);
		case E::Iglasty: return FLinearColor(0.17f, 0.28f, 0.16f);
		case E::Mieszany: return FLinearColor(0.21f, 0.33f, 0.17f);
		case E::Rzadki: return FLinearColor(0.34f, 0.43f, 0.24f);
		case E::Mokradlo: return FLinearColor(0.30f, 0.38f, 0.30f);
		case E::Skaly: return FLinearColor(0.48f, 0.46f, 0.42f);
		case E::Snieg: return FLinearColor(0.85f, 0.86f, 0.88f);
		case E::Tundra: return FLinearColor(0.50f, 0.50f, 0.40f);
		default: return FLinearColor(0.4f, 0.45f, 0.3f);
		}
	}
}

void SMinimapa::Construct(const FArguments& InArgs)
{
	Swiat = InArgs._Swiat;
}

AKameraMapy* SMinimapa::Kamera() const
{
	UWorld* W = Swiat.Get();
	const APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	return PC ? Cast<AKameraMapy>(PC->GetPawn()) : nullptr;
}

void SMinimapa::Zakres(FVector2D& Srodek, double& BokSwiata) const
{
	const AKameraMapy* Kam = Kamera();
	if (Kam && Kam->WRegionie())
	{
		// Obraz liczony co jakis czas: srodek przeskakuje dopiero, gdy kamera zjedzie o 1/4 zakresu.
		const FVector2D F(Kam->Ognisko());
		Srodek = (bObrazRegionu && FVector2D::Distance(F, SrodekObrazu) < ZakresRegionu * 0.25) ? SrodekObrazu : F;
		BokSwiata = ZakresRegionu;
		return;
	}
	Srodek = FVector2D::ZeroVector;
	BokSwiata = FSwiat::Bok;
}

FVector2D SMinimapa::NaEkran(FVector2D P, FVector2D Srodek, double BokSwiata) const
{
	// Swiat: +X w gore ekranu, +Y w prawo (jak kamera przy obrocie 0).
	const double U = 0.5 + (P.Y - Srodek.Y) / BokSwiata;
	const double V = 0.5 - (P.X - Srodek.X) / BokSwiata;
	return FVector2D(U * Bok, V * Bok);
}

void SMinimapa::Odrysuj(FVector2D Srodek, double BokSwiata, bool bRegion) const
{
	if (!FSwiat::MaDane())
	{
		return;
	}
	if (!Obraz.IsValid())
	{
		UTexture2D* T = UTexture2D::CreateTransient(PikseliMapy, PikseliMapy, PF_B8G8R8A8);
		if (!T)
		{
			return;
		}
		T->SRGB = false;
		T->Filter = TF_Bilinear;
		T->AddressX = T->AddressY = TA_Clamp;
		Obraz.Reset(T);
		Pedzel.SetResourceObject(T);
		Pedzel.ImageSize = FVector2D(Bok, Bok);
		Pedzel.DrawAs = ESlateBrushDrawType::Image;
	}
	TArray<float> H;
	H.SetNumUninitialized(PikseliMapy * PikseliMapy);
	TArray<FLinearColor> K;
	K.SetNumUninitialized(PikseliMapy * PikseliMapy);
	const double Krok = BokSwiata / PikseliMapy;
	for (int32 R = 0; R < PikseliMapy; ++R)
	{
		for (int32 C = 0; C < PikseliMapy; ++C)
		{
			const double X = Srodek.X + (0.5 - (R + 0.5) / PikseliMapy) * BokSwiata;
			const double Y = Srodek.Y + ((C + 0.5) / PikseliMapy - 0.5) * BokSwiata;
			const int32 I = R * PikseliMapy + C;
			H[I] = FSwiat::Wysokosc(X, Y);
			const FSwiat::EWoda Wd = FSwiat::Woda(X, Y);
			K[I] = Wd != FSwiat::EWoda::Lad ? (Wd == FSwiat::EWoda::Morze ? FLinearColor(0.16f, 0.27f, 0.35f) : FLinearColor(0.22f, 0.36f, 0.44f))
				: KolorPokrycia(FSwiat::Pokrycie(X, Y));
		}
	}
	// Cieniowanie stokow (swiatlo z lewej gory): roznica wysokosci sasiadow.
	TArray<FColor> Piksele;
	Piksele.SetNumUninitialized(PikseliMapy * PikseliMapy);
	const float Skala = bRegion ? 4.f : 12.f;
	for (int32 R = 0; R < PikseliMapy; ++R)
	{
		for (int32 C = 0; C < PikseliMapy; ++C)
		{
			const int32 I = R * PikseliMapy + C;
			const float Sasiad = H[FMath::Max(R - 1, 0) * PikseliMapy + FMath::Max(C - 1, 0)];
			// Stok wznoszacy sie od swiatla jest jasniejszy (spadek wzgledem odleglosci miedzy pikselami).
			const float Cien = 1.f + FMath::Clamp(float((H[I] - Sasiad) / Krok) * Skala, -0.35f, 0.3f);
			FLinearColor L = K[I] * Cien;
			L.A = 1.f;
			Piksele[I] = L.ToFColor(true);
		}
	}
	FTexture2DMipMap& Mip = Obraz->GetPlatformData()->Mips[0];
	void* Dane = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Dane, Piksele.GetData(), Piksele.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();
	Obraz->UpdateResource();
	SrodekObrazu = Srodek;
	BokObrazu = BokSwiata;
	bObrazRegionu = bRegion;
}

int32 SMinimapa::OnPaint(const FPaintArgs& Args, const FGeometry& Geo, const FSlateRect& Clip, FSlateWindowElementList& Out,
	int32 Warstwa, const FWidgetStyle& Styl, bool bRodzic) const
{
	const AKameraMapy* Kam = Kamera();
	UWorld* W = Swiat.Get();
	if (!Kam || !W)
	{
		return Warstwa;
	}
	const bool bRegion = Kam->WRegionie();
	FVector2D Srodek;
	double BokSwiata;
	Zakres(Srodek, BokSwiata);
	const double Teraz = W->GetRealTimeSeconds();
	if (bRegion != bObrazRegionu || !Obraz.IsValid() || (bRegion && !SrodekObrazu.Equals(Srodek, 1.0) && Teraz - CzasObrazu > 0.4))
	{
		Odrysuj(Srodek, BokSwiata, bRegion);
		CzasObrazu = Teraz;
	}
	Srodek = SrodekObrazu;
	BokSwiata = BokObrazu > 0.0 ? BokObrazu : BokSwiata;

	const FSlateBrush* Bialy = FCoreStyle::Get().GetBrush("WhiteBrush");
	auto Kwadrat = [&](FVector2D Gdzie, float R, FLinearColor Kolor, int32 L)
	{
		FSlateDrawElement::MakeBox(Out, L, Geo.ToPaintGeometry(FVector2f(2.f * R, 2.f * R), FSlateLayoutTransform(FVector2f(Gdzie.X - R, Gdzie.Y - R))),
			Bialy, ESlateDrawEffect::None, Kolor);
	};
	// Ramka i obraz terenu.
	FSlateDrawElement::MakeBox(Out, Warstwa, Geo.ToPaintGeometry(FVector2f(Bok + 6.f, Bok + 6.f), FSlateLayoutTransform(FVector2f(-3.f, -3.f))),
		Bialy, ESlateDrawEffect::None, FLinearColor(0.12f, 0.09f, 0.05f, 0.95f));
	if (Obraz.IsValid())
	{
		FSlateDrawElement::MakeBox(Out, Warstwa + 1, Geo.ToPaintGeometry(FVector2f(Bok, Bok), FSlateLayoutTransform()), &Pedzel);
	}
	const auto Wewnatrz = [](FVector2D P) { return P.X >= 0.0 && P.Y >= 0.0 && P.X <= Bok && P.Y <= Bok; };
	// Budynki (gracza i rodow z ludzikami) - tylko na mapie grywalnej, gdzie sa wieksze niz piksel.
	FVector2D SumaOsady = FVector2D::ZeroVector;
	int32 IleOsady = 0;
	for (TActorIterator<ABudowa> It(W); It; ++It)
	{
		const FVector2D P(It->GetActorLocation());
		SumaOsady += P;
		++IleOsady;
		const FVector2D E = NaEkran(P, Srodek, BokSwiata);
		if (bRegion && Wewnatrz(E))
		{
			Kwadrat(E, 1.5f, FLinearColor(0.95f, 0.85f, 0.6f), Warstwa + 2);
		}
	}
	// Osada gracza na mapie papierowej: zloty znak w srodku budynkow.
	if (!bRegion && IleOsady > 0)
	{
		const FVector2D E = NaEkran(SumaOsady / IleOsady, Srodek, BokSwiata);
		Kwadrat(E, 4.f, FLinearColor(0.1f, 0.08f, 0.02f), Warstwa + 2);
		Kwadrat(E, 3.f, FLinearColor(1.f, 0.8f, 0.25f), Warstwa + 3);
	}
	// Osady rodow AI: kropki w barwie rodu (czerwona obwodka = wrogi).
	for (TActorIterator<ARody> It(W); It; ++It)
	{
		for (const FRod& R : It->Lista())
		{
			const FVector2D E = NaEkran(R.Gdzie, Srodek, BokSwiata);
			if (Wewnatrz(E))
			{
				Kwadrat(E, 4.f, R.bWrogi ? FLinearColor(0.8f, 0.1f, 0.05f) : FLinearColor(0.05f, 0.05f, 0.05f), Warstwa + 2);
				Kwadrat(E, 2.5f, R.Barwa, Warstwa + 3);
			}
		}
	}
	// Prostokat widoku (obrocony jak kamera).
	FVector2D Rozmiar(1.0, 1.0);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(Rozmiar);
	}
	const double Sz = Kam->SzerokoscWidoku();
	const double Wy = Sz * Rozmiar.Y / FMath::Max(Rozmiar.X, 1.0);
	const FRotator Obr(0.f, Kam->Obrot(), 0.f);
	const FVector F = Kam->Ognisko();
	TArray<FVector2f> Linia;
	const FVector2D Rogi[5] = { {1, -1}, {1, 1}, {-1, 1}, {-1, -1}, {1, -1} };
	for (const FVector2D& R : Rogi)
	{
		const FVector Lok = Obr.RotateVector(FVector(R.X * Wy * 0.5, R.Y * Sz * 0.5, 0.0));
		FVector2D E = NaEkran(FVector2D(F.X + Lok.X, F.Y + Lok.Y), Srodek, BokSwiata);
		E.X = FMath::Clamp(E.X, 0.0, double(Bok));
		E.Y = FMath::Clamp(E.Y, 0.0, double(Bok));
		Linia.Add(FVector2f(E));
	}
	FSlateDrawElement::MakeLines(Out, Warstwa + 4, Geo.ToPaintGeometry(), Linia, ESlateDrawEffect::None, FLinearColor(1.f, 0.95f, 0.75f), true, 1.5f);
	return Warstwa + 4;
}

void SMinimapa::Przenies(const FGeometry& Geo, const FPointerEvent& E)
{
	AKameraMapy* Kam = Kamera();
	if (!Kam)
	{
		return;
	}
	const FVector2D L = Geo.AbsoluteToLocal(E.GetScreenSpacePosition());
	const double BokSwiata = BokObrazu > 0.0 ? BokObrazu : FSwiat::Bok;
	const FVector2D Cel(SrodekObrazu.X + (0.5 - L.Y / Bok) * BokSwiata, SrodekObrazu.Y + (L.X / Bok - 0.5) * BokSwiata);
	const double Brzeg = FSwiat::Polowa * 0.98;
	Kam->UstawOgnisko(FVector2D(FMath::Clamp(Cel.X, -Brzeg, Brzeg), FMath::Clamp(Cel.Y, -Brzeg, Brzeg)));
}

FReply SMinimapa::OnMouseButtonDown(const FGeometry& Geo, const FPointerEvent& E)
{
	if (E.GetEffectingButton() != EKeys::LeftMouseButton || BokObrazu <= 0.0)
	{
		return FReply::Unhandled();
	}
	bCiagnie = true;
	Przenies(Geo, E);
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SMinimapa::OnMouseButtonUp(const FGeometry& Geo, const FPointerEvent& E)
{
	if (!bCiagnie)
	{
		return FReply::Unhandled();
	}
	bCiagnie = false;
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SMinimapa::OnMouseMove(const FGeometry& Geo, const FPointerEvent& E)
{
	if (bCiagnie && HasMouseCapture())
	{
		// W regionie przeciaganie dziala na zamrozonym obrazie (srodek nie skacze pod kursorem).
		Przenies(Geo, E);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}
