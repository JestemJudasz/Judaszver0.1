// Plik: Source/grarts2d/Wojna/Bitwa.cpp
#include "Wojna/Bitwa.h"
#include "Ludzie/Ludzik.h"
#include "Dzwiek/Dzwieki.h"
#include "Osada/Osada.h"
#include "Budowy/Budowa.h"
#include "Osada/Rody.h"
#include "Osada/OsadaRodu.h"
#include "Efekty/Efekty.h"
#include "Swiat/SwiatWysokosci.h"
#include "Swiat/TerenKafle.h"
#include "Kamera/KameraMapy.h"
#include "Wojna/PanelBitwy.h"
#include "UI/StylUI.h"
#include "UI/Ustawienia.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "EngineUtils.h"
#include "Containers/Map.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// Odleglosci w cm (skala gry; ludzik ma ok. 2,4 m).
	constexpr float OdstepZwarty = 130.f;
	constexpr float OdstepLuzny = 230.f;
	constexpr float Marsz = 260.f;       // oddzial idzie
	constexpr float Szarza = 380.f;      // ostatnie metry ataku
	constexpr float Bieg = 420.f;        // ucieczka
	constexpr float Przestrzen = 95.f;   // woje swojej strony nie blizej niz to
	constexpr float PrzestrzenWroga = 120.f; // woj od wroga
	constexpr float Kontakt = 800.f;     // srodki oddzialow blizej = zwarcie, oddzial staje
	constexpr float ZasiegCiosu = 175.f; // wlocznia / topor
	constexpr float Szukanie = 520.f;    // woj szuka wroga w tym promieniu (od siebie)
	constexpr float Obchodzenie = 1100.f; // wolny woj walczacego oddzialu idzie do wolnego wroga najdalej tak daleko
	constexpr float SamDoWalki = 1000.f; // oddzial stojacy tak blisko wroga (srodki) sam rusza do walki
	constexpr float Daleko = 2600.f;     // uciekajacy tak daleko od wrogow = moze sie zebrac / opuszcza pole
	constexpr float Styk = 170.f;        // odstep czol dwoch oddzialow w zwarciu (linia frontu)
	constexpr float Blok = 0.3f;         // czesc ciosow z przodu, ktora przechodzi przez tarcze / zbicie

	float Dl(FVector2D A, FVector2D B) { return FVector2D::Distance(A, B); }
	FVector2D Poz(const FWojownik& W) { return W.L.IsValid() ? FVector2D(W.L->GetActorLocation()) : FVector2D::ZeroVector; }
	bool Zywy(const FWojownik& W) { return W.bZyje && !W.bPoza && W.L.IsValid(); }

	/** "12 kłód, 5 porcji jedzenia, 3 deski i 14 groszy" (najwiecej czego, do trzech rzeczy). */
	FString OpisTowaru(const FSurowce& Z, int32 Gr)
	{
		TArray<TPair<int32, ESurowiec>> L;
		for (ESurowiec S : WszystkieSurowce) { if (Z[S] > 0) { L.Add(TPair<int32, ESurowiec>(Z[S], S)); } }
		L.Sort([](const TPair<int32, ESurowiec>& A, const TPair<int32, ESurowiec>& B) { return A.Key > B.Key; });
		FString T;
		for (int32 k = 0; k < L.Num() && k < 3; ++k)
		{
			T += (k ? TEXT(", ") : TEXT("")) + IleSurowca(L[k].Value, L[k].Key);
		}
		if (Gr > 0)
		{
			T += FString::Printf(TEXT("%s%d groszy"), T.IsEmpty() ? TEXT("") : TEXT(", "), Gr);
		}
		return T.IsEmpty() ? FString(TEXT("nic")) : T;
	}

	const TCHAR* Rzymskie[] = { TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"), TEXT("VI"), TEXT("VII"), TEXT("VIII"),
		TEXT("IX"), TEXT("X"), TEXT("XI"), TEXT("XII") };

	/** Choragwie nad oddzialami: nazwa, ilu na polu, pasek morale; wybrane w zlotej ramce, kropki pod ich wojami. */
	class SZnakiBitwy : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SZnakiBitwy) {}
			SLATE_ARGUMENT(TWeakObjectPtr<ABitwa>, Bitwa)
		SLATE_END_ARGS()

		void Construct(const FArguments& A) { BitwaW = A._Bitwa; }
		virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Culling,
			FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Styl, bool bParentEnabled) const override
		{
			const ABitwa* B = BitwaW.Get();
			APlayerController* PC = B && B->GetWorld() ? B->GetWorld()->GetFirstPlayerController() : nullptr;
			if (!PC || !B->Trwa() || !B->WRegionie())
			{
				return Layer;
			}
			const float Skala = FMath::Max(0.01f, G.Scale);
			const FSlateBrush* Bialy = FCoreStyle::Get().GetBrush("GenericWhiteBox");
			const FSlateFontInfo Gruba = FCoreStyle::GetDefaultFontStyle("Bold", 10);
			auto Box = [&](int32 L, FVector2f P, FVector2f R, FLinearColor K)
			{
				FSlateDrawElement::MakeBox(Out, L, G.ToPaintGeometry(R, FSlateLayoutTransform(P)), Bialy, ESlateDrawEffect::None, K);
			};
			auto Rzut = [&](FVector2D Gdzie, float Wys, FVector2f& P)
			{
				FVector2D Px;
				if (!PC->ProjectWorldLocationToScreen(FVector(Gdzie, FSwiat::Wysokosc(Gdzie.X, Gdzie.Y) + Wys), Px, false))
				{
					return false;
				}
				P = FVector2f(Px / Skala);
				return true;
			};
			// Ramka zaznaczania: polprzezroczyste pole i zlote brzegi.
			FVector2D RA, RB;
			if (B->Ramka(RA, RB))
			{
				const FVector2f Min = FVector2f(FVector2D(FMath::Min(RA.X, RB.X), FMath::Min(RA.Y, RB.Y)) / Skala);
				const FVector2f Roz = FVector2f(FVector2D(FMath::Abs(RA.X - RB.X), FMath::Abs(RA.Y - RB.Y)) / Skala);
				Box(Layer, Min, Roz, FLinearColor(0.85f, 0.66f, 0.32f, 0.12f));
				Box(Layer + 1, Min, FVector2f(Roz.X, 1.5f), StylUI::Zloto);
				Box(Layer + 1, Min + FVector2f(0.f, Roz.Y), FVector2f(Roz.X, 1.5f), StylUI::Zloto);
				Box(Layer + 1, Min, FVector2f(1.5f, Roz.Y), StylUI::Zloto);
				Box(Layer + 1, Min + FVector2f(Roz.X, 0.f), FVector2f(1.5f, Roz.Y), StylUI::Zloto);
			}
			// Przeciaganie prawym: duszki miejsc nowego szyku i strzalki frontu.
			for (const FVector2D& M : B->PodgladSzyku())
			{
				FVector2f P;
				if (Rzut(M, 5.f, P)) { Box(Layer, P - FVector2f(3.f, 1.5f), FVector2f(6.f, 3.f), FLinearColor(1.f, 0.9f, 0.6f, 0.55f)); }
			}
			for (const FVector2D& M : B->PodgladStrzalek())
			{
				FVector2f P;
				if (Rzut(M, 5.f, P)) { Box(Layer, P - FVector2f(3.f, 3.f), FVector2f(6.f, 6.f), StylUI::Zloto); }
			}
			const TArray<FOddzial>& Odd = B->Oddzialy();
			// Cel ataku (23.11.2026, prosba uzytkownika "fajnie wygladajacy indykator, co jednostki atakuja"; wczesniej kropki):
			//  - od twojego atakujacego oddzialu do celu luk po ziemi z ciemnym obrysem, plyna po nim jasne grociki w strone wroga,
			//    na koncu grot strzaly (wybrane wyraznie, reszta blado);
			//  - cel: pulsujacy pierscien na ziemi wokol jego szyku (tez wrog pod kursorem: prawy klik zaatakuje jego);
			//  - wrog, ktory idzie na twoj oddzial: cienka przerywana ciemnoczerwona linia ze strzalka;
			//  - zwarcie: skrzyzowane miecze nad linia frontu.
			const float Czas = float(FPlatformTime::Seconds());
			auto Linia = [&](int32 L, const TArray<FVector2f>& Pkt, FLinearColor K, float Grub)
			{
				if (Pkt.Num() >= 2)
				{
					FSlateDrawElement::MakeLines(Out, L, G.ToPaintGeometry(), Pkt, ESlateDrawEffect::None, K, true, Grub);
				}
			};
			// Linia z obrysem (ciemny spod, kolor na wierzchu): widac ja na trawie, polu i sniegu.
			auto Kreska = [&](const TArray<FVector2f>& Pkt, FLinearColor K, float Grub)
			{
				Linia(Layer, Pkt, FLinearColor(0.f, 0.f, 0.f, 0.55f * K.A), Grub + 3.f);
				Linia(Layer + 1, Pkt, K, Grub);
			};
			// Grot / grocik w punkcie P ekranu, skierowany wzdluz D (znormalizowany).
			auto Grot = [&](FVector2f P, FVector2f D, float R, FLinearColor K, float Grub)
			{
				const FVector2f Bok(-D.Y, D.X);
				Kreska({ P - D * R + Bok * R * 0.75f, P, P - D * R - Bok * R * 0.75f }, K, Grub);
			};
			// Skrzyzowane miecze (zwarcie, cel pod kursorem): dwa ostrza, jelce i glowice.
			auto Miecze = [&](FVector2f P, float R, float A)
			{
				for (const float S : { -1.f, 1.f })
				{
					const FVector2f D = FVector2f(S, 1.f).GetSafeNormal(); // ostrze od rekojesci (dol) do sztychu (gora)
					const FVector2f Rek = P + FVector2f(-S * R, R) * 0.7f;
					const FVector2f Szt = P - FVector2f(-S * R, R) * 0.75f;
					const FVector2f Bok(-D.Y, D.X);
					const FVector2f Jelec = Rek + (Szt - Rek) * 0.22f;
					Kreska({ Jelec, Szt }, FLinearColor(0.92f, 0.9f, 0.85f, A), 3.f);
					Kreska({ Rek, Jelec }, FLinearColor(0.35f, 0.22f, 0.12f, A), 3.f);
					Kreska({ Jelec + Bok * R * 0.28f, Jelec - Bok * R * 0.28f }, FLinearColor(0.85f, 0.66f, 0.32f, A), 2.5f);
				}
			};
			// Punkty luku po ziemi (krzywa Beziera wygieta w bok, zeby kilka strzalek do jednego celu sie nie zlewalo).
			auto Luk = [&](FVector2D A, FVector2D Bk, float Wygiecie, TArray<FVector2D>& Swiat)
			{
				const FVector2D K = Bk - A;
				const FVector2D Kontrol = (A + Bk) * 0.5 + FVector2D(-K.Y, K.X).GetSafeNormal() * Wygiecie;
				const int32 N = FMath::Clamp(FMath::CeilToInt32(K.Size() / 120.0), 6, 60);
				Swiat.Reset();
				for (int32 k = 0; k <= N; ++k)
				{
					const double t = double(k) / N;
					Swiat.Add(FMath::Lerp(FMath::Lerp(A, Kontrol, t), FMath::Lerp(Kontrol, Bk, t), t));
				}
			};
			auto NaEkran = [&](const TArray<FVector2D>& Swiat, TArray<FVector2f>& Ekran)
			{
				Ekran.Reset();
				for (const FVector2D& S : Swiat)
				{
					FVector2f P;
					if (Rzut(S, 25.f, P)) { Ekran.Add(P); }
				}
			};
			// Pierscien wokol szyku oddzialu: elipsa w ukladzie jego frontu, z zapasem 1,8 m.
			auto Pierscien = [&](const FOddzial& Cel, float A, float Puls)
			{
				const FVector2D F = Cel.Front.GetSafeNormal();
				const FVector2D Pr(-F.Y, F.X);
				FBox2D Bx(ForceInit);
				for (const FWojownik& W : Cel.Ludzie)
				{
					if (Zywy(W))
					{
						const FVector2D D = Poz(W) - Cel.Srodek;
						Bx += FVector2D(FVector2D::DotProduct(D, F), FVector2D::DotProduct(D, Pr));
					}
				}
				if (!Bx.bIsValid)
				{
					return;
				}
				const FVector2D Sr = Bx.GetCenter();
				const FVector2D R = (Bx.GetExtent() + FVector2D(180.0, 180.0)) * Puls;
				TArray<FVector2D> Swiat;
				for (int32 k = 0; k <= 48; ++k)
				{
					const double Kat = 2.0 * PI * k / 48;
					const FVector2D L = Sr + FVector2D(FMath::Cos(Kat) * R.X, FMath::Sin(Kat) * R.Y);
					Swiat.Add(Cel.Srodek + F * L.X + Pr * L.Y);
				}
				TArray<FVector2f> Ekran;
				NaEkran(Swiat, Ekran);
				Kreska(Ekran, FLinearColor(0.95f, 0.22f, 0.12f, A), 2.5f);
			};
			TMap<int32, float> Cele;
			if (Odd.IsValidIndex(B->CelPodKursorem()))
			{
				Cele.Add(B->CelPodKursorem(), 0.9f);
			}
			TSet<uint32> Zwarcia;
			TArray<FVector2D> Swiat;
			TArray<FVector2f> Ekran;
			for (int32 i = 0; i < Odd.Num(); ++i)
			{
				const FOddzial& O = Odd[i];
				if (!O.Stoi() || O.Rozkaz != ERozkaz::Atakuj || !Odd.IsValidIndex(O.Cel) || !Odd[O.Cel].Stoi() || Odd[O.Cel].Strona == O.Strona)
				{
					continue;
				}
				const bool bMoj = O.Strona == 0;
				const float Jas = bMoj ? (B->Wybrany(i) ? 1.f : 0.6f) : 0.7f;
				if (bMoj)
				{
					float& C = Cele.FindOrAdd(O.Cel);
					C = FMath::Max(C, Jas);
				}
				const FOddzial& Cel = Odd[O.Cel];
				const FVector2D Od = O.Srodek + O.Front * 150.f;
				const FVector2D K = Cel.Srodek - Od;
				const float L = K.Size() - 320.f; // konczy sie przed wrogiem, nie pod jego wojami
				if (L < 250.f || FVector2D::Distance(O.Srodek, Cel.Srodek) < Kontakt)
				{
					// Zwarcie: miecze nad srodkiem linii frontu (jedne na pare oddzialow).
					const uint32 Para = uint32(FMath::Min(i, O.Cel)) * 1000u + uint32(FMath::Max(i, O.Cel));
					FVector2f P;
					if (!Zwarcia.Contains(Para) && Rzut((O.Srodek + Cel.Srodek) * 0.5, 380.f, P))
					{
						Zwarcia.Add(Para);
						Miecze(P, 11.f + 1.5f * FMath::Sin(Czas * 5.f), 0.95f);
					}
					continue;
				}
				const FVector2D Kn = K.GetSafeNormal();
				Luk(Od, Od + Kn * L, (bMoj ? 0.12f : -0.08f) * L * ((i % 2) ? 1.f : -1.f), Swiat);
				NaEkran(Swiat, Ekran);
				if (Ekran.Num() < 2)
				{
					continue;
				}
				if (bMoj)
				{
					// Pas z miekkim poczatkiem (kolory punktow), grociki plyna w strone celu, na koncu duzy grot.
					TArray<FLinearColor> Kol;
					for (int32 k = 0; k < Ekran.Num(); ++k)
					{
						Kol.Add(FLinearColor(0.92f, 0.2f, 0.12f, Jas * FMath::Clamp(float(k) / FMath::Max(1, Ekran.Num() / 5), 0.15f, 1.f)));
					}
					FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), Ekran, Kol, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 0.55f), true, 7.f);
					FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), Ekran, Kol, ESlateDrawEffect::None, FLinearColor::White, true, 4.f);
					const int32 N = Ekran.Num();
					for (float s = FMath::Fmod(Czas * 0.55f, 1.f) * 0.22f; s < 0.94f; s += 0.22f)
					{
						const float f = s * (N - 1);
						const int32 a = FMath::Clamp(FMath::FloorToInt32(f), 0, N - 2);
						const FVector2f P = FMath::Lerp(Ekran[a], Ekran[a + 1], f - a);
						const FVector2f D = (Ekran[a + 1] - Ekran[a]).GetSafeNormal();
						Linia(Layer + 2, { P - D * 6.f + FVector2f(-D.Y, D.X) * 5.f, P, P - D * 6.f - FVector2f(-D.Y, D.X) * 5.f },
							FLinearColor(1.f, 0.8f, 0.55f, Jas * FMath::Clamp(s * 6.f, 0.f, 1.f)), 2.f);
					}
					Grot(Ekran.Last(), (Ekran.Last() - Ekran[N - 2]).GetSafeNormal(), 15.f, FLinearColor(0.95f, 0.22f, 0.12f, Jas), 4.f);
				}
				else
				{
					// Wrog idzie na twoj oddzial: przerywana ciemniejsza linia.
					for (int32 k = 0; k + 1 < Ekran.Num(); k += 2)
					{
						Kreska({ Ekran[k], Ekran[k + 1] }, FLinearColor(0.6f, 0.08f, 0.05f, Jas), 2.f);
					}
					Grot(Ekran.Last(), (Ekran.Last() - Ekran[Ekran.Num() - 2]).GetSafeNormal(), 10.f, FLinearColor(0.6f, 0.08f, 0.05f, Jas), 2.5f);
				}
			}
			const float Puls = 1.f + 0.05f * FMath::Sin(Czas * 4.f);
			for (const TPair<int32, float>& C : Cele)
			{
				Pierscien(Odd[C.Key], C.Value, Puls);
				if (C.Key == B->CelPodKursorem())
				{
					FVector2f P;
					if (Rzut(Odd[C.Key].Srodek, 700.f, P))
					{
						Miecze(P - FVector2f(0.f, 30.f), 12.f, 0.95f); // prawy klik: atak
					}
				}
			}
			for (int32 i = 0; i < Odd.Num(); ++i)
			{
				const FOddzial& O = Odd[i];
				if (O.bPoza || O.NaPolu() == 0)
				{
					continue;
				}
				const bool bWyb = B->Wybrany(i);
				if (bWyb)
				{
					for (const FWojownik& W : O.Ludzie)
					{
						FVector2f P;
						if (Zywy(W) && Rzut(Poz(W), 5.f, P))
						{
							Box(Layer, P - FVector2f(3.f, 1.5f), FVector2f(6.f, 3.f), StylUI::Zloto);
						}
					}
				}
				FVector2f P;
				// Choragiew nad srodkiem glebokosci szyku (nie nad czolem).
				const FVector2D Nad = O.Srodek - O.Front * ((O.Szeregi - 1) * 0.5f * (O.bZwarty ? 130.f : 230.f));
				if (!Rzut(Nad, 560.f, P))
				{
					continue;
				}
				// Drzewce, plat choragwi w barwie oddzialu, obok nazwa i pasek morale.
				// Wrog ma wyzsze drzewce: w zwarciu jego znak jest nad twoim, nie na nim.
				const float Wys = O.Strona == 0 ? 0.f : 40.f;
				Box(Layer, P + FVector2f(-1.f, -30.f - Wys), FVector2f(2.f, 30.f + Wys), FLinearColor(0.05f, 0.04f, 0.03f, 1.f));
				P.Y -= Wys;
				if (bWyb)
				{
					Box(Layer, P + FVector2f(-1.f, -32.f), FVector2f(26.f, 18.f), StylUI::Zloto);
				}
				else if (const float* C = Cele.Find(i))
				{
					Box(Layer, P + FVector2f(-1.f, -32.f), FVector2f(26.f, 18.f), FLinearColor(0.92f, 0.2f, 0.12f, *C)); // cel ataku
				}
				Box(Layer + 1, P + FVector2f(1.f, -30.f), FVector2f(22.f, 14.f), O.Barwa);
				const FString Napis = FString::Printf(TEXT("%s %d"), *O.Nazwa, O.NaPolu());
				const float SzerNapisu = 7.5f * Napis.Len() + 12.f;
				const FVector2f T = P + FVector2f(28.f, -32.f);
				Box(Layer, T - FVector2f(4.f, 2.f), FVector2f(SzerNapisu, 26.f), FLinearColor(0.035f, 0.028f, 0.02f, 0.8f));
				FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(220.f, 14.f), FSlateLayoutTransform(T)), Napis, Gruba,
					ESlateDrawEffect::None, O.Strona == 0 ? StylUI::Napis : FLinearColor(1.f, 0.78f, 0.7f));
				const float M = FMath::Clamp(O.Morale / 100.f, 0.f, 1.f);
				Box(Layer + 1, T + FVector2f(0.f, 16.f), FVector2f(60.f, 5.f), FLinearColor(0.f, 0.f, 0.f, 0.9f));
				Box(Layer + 2, T + FVector2f(0.f, 16.f), FVector2f(60.f * M, 5.f), O.bZlamany ? StylUI::Czerwony : FMath::Lerp(StylUI::Czerwony, StylUI::Zielony, M));
				if (O.bZlamany)
				{
					FSlateDrawElement::MakeText(Out, Layer + 2, G.ToPaintGeometry(FVector2f(100.f, 14.f), FSlateLayoutTransform(T + FVector2f(66.f, 10.f))),
						FString(TEXT("ucieka!")), Gruba, ESlateDrawEffect::None, StylUI::Czerwony);
				}
			}
			return Layer + 3;
		}

	private:
		TWeakObjectPtr<ABitwa> BitwaW;
	};
}

int32 FOddzial::Zywych() const
{
	int32 N = 0;
	for (const FWojownik& W : Ludzie) { N += W.bZyje ? 1 : 0; }
	return N;
}

int32 FOddzial::NaPolu() const
{
	int32 N = 0;
	for (const FWojownik& W : Ludzie) { N += Zywy(W) ? 1 : 0; }
	return N;
}

bool ABitwa::bKrew = true;

bool FOddzial::Strzela() const
{
	return RodzajWojska(Rodzaj).Strzela() && Strzal() > 0;
}

int32 FOddzial::Strzal() const
{
	int32 N = 0;
	for (const FWojownik& W : Ludzie) { N += Zywy(W) ? W.Pociski : 0; }
	return N;
}
bool ABitwa::bMyszZajeta = false;
bool ABitwa::bDziennik = false;

int32 FMilicja::Ilu() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ALudzik>& L : Ludzie) { N += L.IsValid() && !L->Lezy() ? 1 : 0; }
	return N;
}

ABitwa::ABitwa()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	for (TObjectPtr<UInstancedStaticMeshComponent>* C : { &KropleC, &PlamyC, &LeceC, &WbiteC, &IskryC })
	{
		*C = CreateDefaultSubobject<UInstancedStaticMeshComponent>(C == &KropleC ? TEXT("Krople") : C == &PlamyC ? TEXT("Plamy")
			: C == &LeceC ? TEXT("Strzaly") : C == &IskryC ? TEXT("Iskry") : TEXT("Wbite"));
		(*C)->SetupAttachment(RootComponent);
		(*C)->SetMobility(EComponentMobility::Movable);
		(*C)->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		(*C)->SetCastShadow(false);
	}
}

void ABitwa::Krew(FVector Gdzie, FVector2D Kier, int32 Ile, float Sila)
{
	if (!bKrew || !bWidac || Krople.Num() > 500)
	{
		return;
	}
	for (int32 k = 0; k < Ile; ++k)
	{
		const FVector2D R = (Kier + FVector2D(FMath::FRandRange(-0.8f, 0.8f), FMath::FRandRange(-0.8f, 0.8f))).GetSafeNormal();
		Krople.Add({ Gdzie + FVector(FMath::FRandRange(-12.f, 12.f), FMath::FRandRange(-12.f, 12.f), FMath::FRandRange(-15.f, 15.f)),
			FVector(R * FMath::FRandRange(60.f, 260.f) * Sila, FMath::FRandRange(60.f, 320.f) * Sila) });
	}
}

void ABitwa::Plama(FVector2D Gdzie, float Rozmiar)
{
	if (!bKrew || !PlamyC->GetStaticMesh())
	{
		return;
	}
	constexpr int32 MaxPlam = 500;
	// Kazda kolejna plama minimalnie wyzej: nachodzace na siebie nie migocza.
	const FTransform T(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f),
		FVector(Gdzie, FSwiat::Wysokosc(Gdzie.X, Gdzie.Y) + 3.f + 0.05f * (NastPlama % 40)), FVector(Rozmiar, Rozmiar * FMath::FRandRange(0.7f, 1.f), 1.f));
	if (PlamyC->GetInstanceCount() < MaxPlam)
	{
		PlamyC->AddInstance(T, true);
	}
	else
	{
		PlamyC->UpdateInstanceTransform(NastPlama % MaxPlam, T, true, true);
	}
	++NastPlama;
}

void ABitwa::KrewTick(float Dt)
{
	if (Krople.Num() == 0 && KropleC->GetInstanceCount() == 0)
	{
		return;
	}
	TArray<FTransform> T;
	for (int32 i = Krople.Num() - 1; i >= 0; --i)
	{
		FKropla& K = Krople[i];
		K.V.Z -= 1500.f * Dt;
		K.P += K.V * Dt;
		if (K.P.Z <= FSwiat::Wysokosc(K.P.X, K.P.Y) + 2.f)
		{
			if (FMath::FRand() < 0.35f)
			{
				Plama(FVector2D(K.P), FMath::FRandRange(0.15f, 0.4f));
			}
			Krople.RemoveAtSwap(i);
			continue;
		}
		T.Add(FTransform(FRotator::ZeroRotator, K.P, FVector(1.4f)));
	}
	KropleC->ClearInstances();
	if (T.Num() > 0)
	{
		KropleC->AddInstances(T, false, true);
	}
}

void ABitwa::EndPlay(const EEndPlayReason::Type Reason)
{
	UsunInterfejs();
	Super::EndPlay(Reason);
}

bool ABitwa::WRegionie() const
{
	return Teren.IsValid() && Teren->WRegionie();
}

FVector2D ABitwa::Miejsce(const FOddzial& O, int32 k)
{
	const int32 Kol = FMath::Max(1, O.Kol);
	const int32 R = k / Kol, C = k % Kol;
	const float Od = O.bZwarty ? OdstepZwarty : OdstepLuzny;
	const FVector2D Prawo(-O.Front.Y, O.Front.X);
	return O.Srodek + Prawo * ((C - (Kol - 1) * 0.5f) * Od) - O.Front * (R * Od);
}

int32 ABitwa::DodajOddzial(ERodzajWojska Rd, const FString& Nazwa, int32 Strona, int32 Ilu, FVector2D Srodek, FVector2D Front,
	int32 Doswiadczenie, FLinearColor Tunika)
{
	const FRodzajWojska& R = RodzajWojska(Rd);
	if (Tunika.A <= 0.f)
	{
		Tunika = R.Tunika;
	}
	Tunika.A = 1.f;
	const TCHAR* Bron = R.Bron;
	const float Zdrowie = R.Zdrowie;
	FOddzial O;
	O.Nazwa = Nazwa;
	O.Strona = Strona;
	O.Barwa = Strona == 0 ? FLinearColor(0.72f, 0.14f, 0.09f) : FLinearColor(0.06f, 0.06f, 0.08f);
	O.Srodek = O.CelRuchu = Srodek;
	O.Front = Front.GetSafeNormal();
	O.Rodzaj = Rd;
	O.Ikona = R.Ikona;
	O.Trafienie = R.Trafienie;
	O.Pancerz = R.Pancerz;
	O.Zdrowie = Zdrowie;
	O.Blok = R.Blok;
	O.Tempo = R.Tempo;
	O.Przebicie = R.Przebicie;
	O.Ciecie = R.Ciecie;
	O.Morale = R.Morale;
	O.Poczatkowo = Ilu;
	O.Start = CzasBitwy;
	bWrogByl |= Strona == 1;
	O.Tunika = Tunika;
	O.bTopor = R.Ikona != EIkonaBroni::Wlocznia;
	O.Szeregi = Ilu >= 6 ? 3 : 1;
	O.Kol = FMath::Max(1, FMath::DivideAndRoundUp(Ilu, O.Szeregi));
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(O.Front.Y, O.Front.X));
	for (int32 k = 0; k < Ilu; ++k)
	{
		const FVector2D G = Miejsce(O, k) + FVector2D(FMath::FRandRange(-25.f, 25.f), FMath::FRandRange(-25.f, 25.f));
		ALudzik* L = GetWorld()->SpawnActor<ALudzik>(FVector(G, FSwiat::Wysokosc(G.X, G.Y)), FRotator(0.f, Yaw, 0.f), P);
		if (!L)
		{
			continue;
		}
		L->UstawPlec(false);
		const FLinearColor Wlosy = FMath::Lerp(FLinearColor(0.05f, 0.03f, 0.02f), FLinearColor(0.45f, 0.32f, 0.15f), FMath::FRand());
		const FLinearColor T = Tunika * FMath::FRandRange(0.85f, 1.1f);
		L->WejdzDoBitwy(T, Wlosy, Bron);
		// Wyglad: kazdy woj troche inny (helm / czapka / kaptur, broda, tarcza). Choragiew niesie jeden woj ze srodka
		// drugiego szeregu w lepszych (doswiadczonych) oddzialach.
		const float Los = FMath::FRand();
		const int32 Helm = Los < R.Helm ? 1 : Los < R.Helm + R.Czapka ? 2 : Los < R.Helm + R.Czapka + R.Kaptur ? 3 : 0;
		const bool bChor = Doswiadczenie >= 1 && Ilu >= 6 && k == O.Kol + O.Kol / 2;
		const FLinearColor Lico = Strona == 0 ? O.Barwa : FMath::Lerp(FLinearColor(0.32f, 0.22f, 0.13f), O.Barwa, FMath::FRand() * 0.6f);
		L->UbierzDoBitwy(Helm, FMath::FRand() < (Strona == 0 ? 0.5f : 0.65f), FMath::FRand() < R.Tarcza, bChor, Lico);
		L->WalkaStoj(G + O.Front * 1000.f);
		FWojownik W;
		W.L = L;
		W.Zdrowie = Zdrowie;
		W.Slot = k;
		W.Doswiadczenie = Doswiadczenie;
		W.Odnowa = FMath::FRandRange(0.f, 1.f);
		W.Pociski = R.Pociski;
		O.Ludzie.Add(W);
	}
	return Oddz.Add(O);
}

bool ABitwa::ZacznijProbna(FVector2D Srodek, bool bPokaz)
{
	if (bTrwa)
	{
		return false;
	}
	Sprzatnij();
	if (Teren.IsValid())
	{
		Teren->Wykarczuj(Srodek, 3200.f);
	}
	Przygotuj(Srodek);
	// Bok do kamery (kamera regionu patrzy na start w strone +X): twoi woje po lewej, bandyci po prawej.
	const FVector2D Wsch(0.f, 1.f);
	// Etap W2: mieszane wojsko. Twoi: wlocznicy, miecznicy, za nimi lucznicy; wrog: tarczownicy z toporami, palkarze,
	// za nimi kusznicy.
	DodajOddzial(ERodzajWojska::Wlocznicy, TEXT("Włócznicy"), 0, 12, Srodek + FVector2D(-650.f, -1300.f), Wsch, 1);
	DodajOddzial(ERodzajWojska::Miecznicy, TEXT("Miecznicy"), 0, 10, Srodek + FVector2D(650.f, -1300.f), Wsch, 1);
	DodajOddzial(ERodzajWojska::Lucznicy, TEXT("Łucznicy"), 0, 12, Srodek + FVector2D(0.f, -2500.f), Wsch, 0);
	DodajOddzial(ERodzajWojska::Tarczownicy, TEXT("Tarczownicy"), 1, 12, Srodek + FVector2D(-650.f, 1300.f), -Wsch, 0, FLinearColor(0.22f, 0.24f, 0.17f));
	DodajOddzial(ERodzajWojska::Palkarze, TEXT("Pałkarze"), 1, 12, Srodek + FVector2D(650.f, 1300.f), -Wsch, 0, FLinearColor(0.30f, 0.22f, 0.16f));
	DodajOddzial(ERodzajWojska::Kusznicy, TEXT("Kusznicy"), 1, 10, Srodek + FVector2D(0.f, 2500.f), -Wsch, 0, FLinearColor(0.26f, 0.20f, 0.18f));
	Rozpocznij();
	if (bPokaz)
	{
		for (FOddzial& X : Oddz) { X.Start -= 6.f; } // pokaz: bandyci ruszaja od razu
	}
	Wybrane = { 0 };
	if (bPokaz)
	{
		Oddz[0].Rozkaz = ERozkaz::Atakuj;
		Oddz[0].Cel = 3;
		Oddz[1].Rozkaz = ERozkaz::Atakuj;
		Oddz[1].Cel = 4;
		Oddz[2].Rozkaz = ERozkaz::Atakuj;
		Oddz[2].Cel = 4; // lucznicy w palkarzy (bez tarcz)
	}
	UE_LOG(LogTemp, Log, TEXT("Bitwa: bitwa probna przy (%.0f, %.0f)"), Srodek.X, Srodek.Y);
	return true;
}

void ABitwa::Przygotuj(FVector2D Srodek)
{
	// Aktor na polu bitwy: krople i plamy (instancje) maja male wspolrzedne wzgledem niego.
	SetActorLocation(FVector(Srodek, FSwiat::Wysokosc(Srodek.X, Srodek.Y)));
	static bool bUstawienia = false;
	if (!bUstawienia)
	{
		bUstawienia = true;
		GConfig->GetBool(TEXT("Bitwa"), TEXT("Krew"), bKrew, GGameUserSettingsIni);
	}
	if (!KropleC->GetStaticMesh())
	{
		KropleC->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Swiat/Ludzie/SM_Ludz_Kropla.SM_Ludz_Kropla")));
		PlamyC->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Swiat/Ludzie/SM_Ludz_Plama.SM_Ludz_Plama")));
		UStaticMesh* Strzala = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Swiat/Ludzie/SM_Ludz_Strzala.SM_Ludz_Strzala"));
		LeceC->SetStaticMesh(Strzala);
		WbiteC->SetStaticMesh(Strzala);
	}
	if (!IskryC->GetStaticMesh())
	{
		IskryC->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Swiat/Ludzie/SM_Ludz_Iskra.SM_Ludz_Iskra")));
	}
}

void ABitwa::Rozpocznij()
{
	if (!bTrwa)
	{
		bTrwa = true;
		bWynik = false;
		bPauza = false;
		Zwyciezca = -1;
		Wybrane.Reset();
		MiaryB = FMiary();
		DziennikZa = 0.f;
		bDziennik |= FParse::Param(FCommandLine::Get(), TEXT("bitwadziennik"));
		if (bDziennik)
		{
			DoDziennika(FString::Printf(TEXT("=== %s: bitwa, %d oddzialow, t=%.1f ==="), *FDateTime::Now().ToString(), Oddz.Num(), CzasBitwy));
		}
	}
	Odswiez();
}

void ABitwa::Odswiez()
{
	// Panel bitwy ma karty oddzialow zrobione raz: nowy albo rozwiazany oddzial = panel od nowa (polozenie paneli zostaje).
	++WersjaN;
	UsunInterfejs();
	if (bTrwa)
	{
		ZrobInterfejs();
	}
}

void ABitwa::Odeslij(FWojownik& W)
{
	if (!W.L.IsValid())
	{
		return;
	}
	if (W.bZOsady && W.bZyje)
	{
		// Z pospolitego ruszenia: wraca do domu i do pracy (zywy, a takze ten, ktory uciekl z pola).
		if (RodW >= 0)
		{
			// z wyprawy: od razu pod oboz (nie idzie kilkudziesieciu km przez mape)
			const FVector2D G = Dom + FVector2D(FMath::FRandRange(-800.f, 800.f), FMath::FRandRange(-800.f, 800.f));
			W.L->SetActorLocation(FVector(G, FSwiat::Wysokosc(G.X, G.Y)));
		}
		W.L->WrocZBitwy();
		W.L->SetActorHiddenInGame(!WRegionie());
	}
	else
	{
		W.L->Destroy();
	}
	W.L.Reset();
}

int32 ABitwa::DodajRuszenie(const FString& Nazwa, const TArray<ALudzik*>& Kto, FVector2D Srodek, FVector2D Front, int32 Wloczni)
{
	FOddzial O;
	O.Nazwa = Nazwa;
	O.Strona = 0;
	O.Barwa = FLinearColor(0.72f, 0.14f, 0.09f);
	O.Tunika = FLinearColor(0.45f, 0.38f, 0.28f);
	O.Srodek = O.CelRuchu = Srodek;
	O.Front = Front.GetSafeNormal();
	O.Trafienie = 0.32f + 0.08f * Wloczni / FMath::Max(1, Kto.Num()); // chlopi: slabiej niz woje, z wlocznia troche lepiej
	O.Zdrowie = 2.f;
	O.Morale = 75.f;
	O.bTopor = Wloczni * 2 < Kto.Num();
	O.Rodzaj = ERodzajWojska::Chlopi;
	O.Ikona = O.bTopor ? EIkonaBroni::Topor : EIkonaBroni::Wlocznia;
	O.Ciecie = RodzajWojska(ERodzajWojska::Chlopi).Ciecie;
	O.Poczatkowo = Kto.Num();
	O.Start = CzasBitwy;
	O.Szeregi = Kto.Num() >= 6 ? 3 : 1;
	O.Kol = FMath::Max(1, FMath::DivideAndRoundUp(Kto.Num(), O.Szeregi));
	for (int32 k = 0; k < Kto.Num(); ++k)
	{
		ALudzik* L = Kto[k];
		L->DoRuszenia(k < Wloczni ? TEXT("SM_Ludz_Wlocznia") : nullptr);
		// Bez zbroi: czasem czapka, broda, rzadko jakas tarcza z deski.
		L->UbierzDoBitwy(FMath::FRand() < 0.3f ? 2 : 0, FMath::FRand() < 0.55f, FMath::FRand() < 0.15f, false, FLinearColor(0.38f, 0.27f, 0.16f));
		FWojownik W;
		W.L = L;
		W.Zdrowie = O.Zdrowie;
		W.Slot = k;
		W.bZOsady = true;
		W.Odnowa = FMath::FRandRange(0.f, 1.f);
		O.Ludzie.Add(W);
	}
	O.bUloz = true; // miejsca w szyku wedlug tego, kto skad przybiegl
	return Oddz.Add(O);
}

bool ABitwa::ZacznijNapad()
{
	AOsada* Wies = Osada();
	if (!Wies || !Wies->MaOboz())
	{
		return false;
	}
	for (const FOddzial& X : Oddz)
	{
		if (X.Strona == 1 && X.Stoi())
		{
			return false; // juz sa wrogowie na polu
		}
	}
	// Bandyci nadchodza z losowej strony z ok. 60 m (z ladu), chwile sie zbieraja: jest czas powolac ruszenie.
	const FVector2D C = Wies->Oboz();
	FVector2D Kier(1.f, 0.f);
	for (int32 Proba = 0; Proba < 16; ++Proba)
	{
		const float Kat = FMath::FRandRange(0.f, 2.f * PI);
		Kier = FVector2D(FMath::Cos(Kat), FMath::Sin(Kat));
		const FVector2D P = C + Kier * 6000.f;
		if (FSwiat::Woda(P.X, P.Y) == FSwiat::EWoda::Lad && FSwiat::OdRzeki(P.X, P.Y) > 600.f)
		{
			break;
		}
	}
	const FVector2D Prawo(-Kier.Y, Kier.X);
	if (Oddz.Num() == 0)
	{
		Przygotuj(C);
	}
	CelNapadu = C;
	Lup = FSurowce();
	LupGroszy = 0;
	bLupUciekl = false;
	// Bandyci: troche mniej niz mezczyzn w osadzie, w grupach do 14.
	const int32 NBand = FMath::Clamp(FMath::RoundToInt32(MezczyznWOsadzie() * 0.8f), 8, 60);
	const int32 NGrupB = FMath::DivideAndRoundUp(NBand, 14);
	static const TCHAR* NazwyB[] = { TEXT("Bandyci"), TEXT("Zbóje"), TEXT("Rozbójnicy"), TEXT("Łotrzykowie"), TEXT("Rabusie") };
	for (int32 g = 0; g < NGrupB; ++g)
	{
		const int32 Ile = NBand / NGrupB + (g < NBand % NGrupB ? 1 : 0);
		const FVector2D Gdzie = C + Kier * 6000.f + Prawo * ((g - (NGrupB - 1) * 0.5f) * 1500.f);
		const int32 i = DodajOddzial(ERodzajWojska::Bandyci, NazwyB[g % 5], 1, Ile, Gdzie, -Kier, 0, FLinearColor(0.24f + 0.03f * g, 0.23f, 0.17f));
		Oddz[i].bNapad = true;
		Oddz[i].Skad = Gdzie + Kier * 2500.f;
		Oddz[i].Start = CzasBitwy + 10.f;
	}
	Rozpocznij();
	Wies->Ogloszenie(FString::Printf(TEXT("Napad! %d bandytów nadchodzi. Powołaj pospolite ruszenie: Wojsko (J)."), NBand));
	UE_LOG(LogTemp, Log, TEXT("Bitwa: napad na osade, bandytow %d w %d grupach"), NBand, NGrupB);
	return true;
}

AOsada* ABitwa::Osada()
{
	if (!Os.IsValid())
	{
		for (TActorIterator<AOsada> It(GetWorld()); It; ++It)
		{
			Os = *It;
			break;
		}
	}
	return Os.Get();
}

TArray<ALudzik*> ABitwa::WolniMezczyzni()
{
	TArray<ALudzik*> Out;
	AOsada* Wies = Osada();
	if (!Wies)
	{
		return Out;
	}
	TSet<const ALudzik*> Zajeci;
	for (const FMilicja& M : Mil)
	{
		for (const TWeakObjectPtr<ALudzik>& L : M.Ludzie) { if (L.IsValid()) { Zajeci.Add(L.Get()); } }
	}
	for (ALudzik* L : Wies->Mezczyzni())
	{
		if (!Zajeci.Contains(L)) { Out.Add(L); }
	}
	return Out;
}

int32 ABitwa::MezczyznWOsadzie()
{
	int32 N = WolniMezczyzni().Num();
	for (const FMilicja& M : Mil) { N += M.Ilu(); }
	return N;
}

int32 ABitwa::NowaMilicja()
{
	AOsada* Wies = Osada();
	TArray<ALudzik*> Wolni = WolniMezczyzni();
	if (!Wies || Wolni.Num() == 0)
	{
		if (Wies) { Wies->Ogloszenie(TEXT("Nie ma wolnych mężczyzn na nowy oddział")); }
		return -1;
	}
	// Najnizszy wolny numer: Ruszenie I, II, ...
	int32 Nr = 0;
	for (; Nr < 11; ++Nr)
	{
		const FString N = FString::Printf(TEXT("Ruszenie %s"), Rzymskie[Nr]);
		if (!Mil.ContainsByPredicate([&N](const FMilicja& M) { return M.Nazwa == N; })) { break; }
	}
	FMilicja M;
	M.Nazwa = FString::Printf(TEXT("Ruszenie %s"), Rzymskie[Nr]);
	for (int32 k = 0; k < Wolni.Num() && k < MaxMilicji; ++k) { M.Ludzie.Add(Wolni[k]); }
	++WersjaN;
	return Mil.Add(M);
}

void ABitwa::Uzupelnij(int32 m)
{
	if (!Mil.IsValidIndex(m) || Mil[m].Powolana())
	{
		return;
	}
	FMilicja& M = Mil[m];
	M.Ludzie.RemoveAll([](const TWeakObjectPtr<ALudzik>& L) { return !L.IsValid() || L->Lezy(); });
	for (ALudzik* L : WolniMezczyzni())
	{
		if (M.Ludzie.Num() >= MaxMilicji) { break; }
		M.Ludzie.Add(L);
	}
	++WersjaN;
}

void ABitwa::UsunMilicje(int32 m)
{
	if (!Mil.IsValidIndex(m))
	{
		return;
	}
	if (Mil[m].Powolana())
	{
		Rozwiaz(m, false);
	}
	Mil.RemoveAt(m);
	for (FOddzial& O : Oddz)
	{
		O.Milicja = O.Milicja == m ? -1 : (O.Milicja > m ? O.Milicja - 1 : O.Milicja);
	}
	++WersjaN;
}

FVector2D ABitwa::MiejsceZbiorki(int32 m, FVector2D& Front)
{
	// Przed obozem, od strony najblizszych bandytow (bez nich: na wschod); kolejne oddzialy obok siebie.
	AOsada* Wies = Osada();
	const FVector2D C = Wies ? Wies->Oboz() : FVector2D(GetActorLocation());
	Front = FVector2D(1.f, 0.f);
	float NajD = TNumericLimits<float>::Max();
	for (const FOddzial& E : Oddz)
	{
		if (E.Strona == 1 && E.Stoi() && Dl(E.Srodek, C) < NajD)
		{
			NajD = Dl(E.Srodek, C);
			Front = (E.Srodek - C).GetSafeNormal();
		}
	}
	int32 k = 0, n = 0;
	for (int32 j = 0; j < Mil.Num(); ++j)
	{
		if (Mil[j].Powolana() || j == m)
		{
			k += j < m ? 1 : 0;
			++n;
		}
	}
	const FVector2D Prawo(-Front.Y, Front.X);
	return C + Front * 1600.f + Prawo * ((k - (n - 1) * 0.5f) * 1300.f);
}

void ABitwa::Powolaj(int32 m)
{
	AOsada* Wies = Osada();
	if (!Wies || !Mil.IsValidIndex(m) || Mil[m].Powolana())
	{
		return;
	}
	FMilicja& M = Mil[m];
	M.Ludzie.RemoveAll([](const TWeakObjectPtr<ALudzik>& L) { return !L.IsValid() || L->Lezy(); });
	TArray<ALudzik*> Kto;
	for (const TWeakObjectPtr<ALudzik>& L : M.Ludzie) { if (!L->WBitwie()) { Kto.Add(L.Get()); } }
	if (Kto.Num() == 0)
	{
		Wies->Ogloszenie(FString::Printf(TEXT("%s: nie ma kogo powołać"), *M.Nazwa));
		++WersjaN;
		return;
	}
	FVector2D Front;
	const FVector2D Gdzie = MiejsceZbiorki(m, Front);
	if (Oddz.Num() == 0)
	{
		Przygotuj(Gdzie);
	}
	// Wlocznie z zapasow (kuznia): ile jest, tyle bierze; reszta idzie z narzedziem z pracy.
	M.Wlocznie = Wies->Wez(ESurowiec::Bron, Kto.Num());
	const int32 i = DodajRuszenie(M.Nazwa, Kto, Gdzie, Front, M.Wlocznie);
	Oddz[i].Milicja = m;
	M.Oddzial = i;
	Rozpocznij();
	Wybrane.Reset();
	Wybrane.Add(i);
	Wies->Ogloszenie(M.Wlocznie > 0
		? FString::Printf(TEXT("%s: %d chłopa rzuca pracę, %d z włóczniami"), *M.Nazwa, Kto.Num(), M.Wlocznie)
		: FString::Printf(TEXT("%s: %d chłopa rzuca pracę i bierze narzędzia"), *M.Nazwa, Kto.Num()));
}

void ABitwa::Rozwiaz(int32 m, bool bOglos)
{
	if (!Mil.IsValidIndex(m) || !Mil[m].Powolana())
	{
		return;
	}
	FMilicja& M = Mil[m];
	int32 Wrocilo = 0;
	if (Oddz.IsValidIndex(M.Oddzial))
	{
		FOddzial& O = Oddz[M.Oddzial];
		for (FWojownik& W : O.Ludzie)
		{
			if (Zywy(W))
			{
				W.bPoza = true;
				Odeslij(W); // wraca do pracy
				++Wrocilo;
			}
		}
		O.bPoza = true;
		O.Rozkaz = ERozkaz::Stoj;
		Wybrane.Remove(M.Oddzial);
		// Wlocznie wracaja do zapasow (polegli i uciekajacy gubia swoje).
		if (AOsada* Wies = Osada(); Wies && M.Wlocznie > 0 && O.Poczatkowo > 0)
		{
			Wies->Zwroc(ESurowiec::Bron, FMath::RoundToInt32(float(M.Wlocznie) * Wrocilo / O.Poczatkowo));
		}
	}
	M.Ludzie.RemoveAll([](const TWeakObjectPtr<ALudzik>& L) { return !L.IsValid() || L->Lezy(); });
	M.Wlocznie = 0;
	M.Oddzial = -1;
	if (bOglos)
	{
		if (AOsada* Wies = Osada())
		{
			Wies->Ogloszenie(FString::Printf(TEXT("%s: %d chłopa wraca do pracy"), *M.Nazwa, Wrocilo));
		}
	}
	SprawdzKoniec();
	Odswiez();
}

void ABitwa::Grabiez(FOddzial& O)
{
	O.bLup = true;
	AOsada* Wies = Osada();
	if (!Wies)
	{
		return;
	}
	int32 Gr = 0;
	const FSurowce Z = Wies->Grabiez(0.35f, Gr);
	Lup += Z;
	LupGroszy += Gr;
	// Wojna (23.11.2026): odchodzac podpalaja najblizsze budynki (1 na 8 zbojow, najwyzej 5); ogien sam przeskakuje dalej.
	TArray<ABudowa*> Cele;
	for (TActorIterator<ABudowa> It(GetWorld()); It; ++It)
	{
		if (It->Zniszczalny() && !It->Ruina() && !It->Zgliszcza() && It->Ogien() <= 0.f && Dl(FVector2D(It->GetActorLocation()), O.Srodek) < 5000.f)
		{
			Cele.Add(*It);
		}
	}
	Cele.Sort([&O](const ABudowa& A, const ABudowa& B) { return Dl(FVector2D(A.GetActorLocation()), O.Srodek) < Dl(FVector2D(B.GetActorLocation()), O.Srodek); });
	const int32 Podpalonych = FMath::Min(Cele.Num(), FMath::Clamp(O.Zywych() / 8, 1, 5));
	for (int32 k = 0; k < Podpalonych; ++k)
	{
		Cele[k]->Podpal(FMath::FRandRange(0.25f, 0.4f));
	}
	Wies->Ogloszenie(Podpalonych > 0
		? FString::Printf(TEXT("%s plądrują osadę: zabrali %s i podpalili %d %s!"), *O.Nazwa, *OpisTowaru(Z, Gr), Podpalonych, Podpalonych == 1 ? TEXT("budynek") : TEXT("budynki"))
		: FString::Printf(TEXT("%s plądrują osadę: zabrali %s"), *O.Nazwa, *OpisTowaru(Z, Gr)));
	UE_LOG(LogTemp, Log, TEXT("Bitwa: %s plądruja osade (%s)"), *O.Nazwa, *OpisTowaru(Z, Gr));
}

void ABitwa::PokazBitwy()
{
	// Laka w okolicy startu: najmniejsza roznica wysokosci w kwadracie 50 m, bez wody.
	const FVector2D S = FSwiat::Start();
	FVector2D Najl = S;
	float NajlR = TNumericLimits<float>::Max();
	for (int32 Proba = 0; Proba < 120; ++Proba)
	{
		const int32 Pierscien = (Proba + 7) / 8;
		const float Kat = FMath::DegreesToRadians((Proba % 8) * 45.f);
		const FVector2D C = S + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * 4000.f * Pierscien;
		float Min = TNumericLimits<float>::Max(), Max = -Min;
		bool bWoda = false;
		for (float X = -2500.f; X <= 2500.f && !bWoda; X += 500.f)
		{
			for (float Y = -2500.f; Y <= 2500.f; Y += 500.f)
			{
				if (FSwiat::Woda(C.X + X, C.Y + Y) != FSwiat::EWoda::Lad || FSwiat::OdRzeki(C.X + X, C.Y + Y) < 800.f)
				{
					bWoda = true;
					break;
				}
				const float H = FSwiat::Wysokosc(C.X + X, C.Y + Y);
				Min = FMath::Min(Min, H);
				Max = FMath::Max(Max, H);
			}
		}
		if (!bWoda && Max - Min < NajlR)
		{
			NajlR = Max - Min;
			Najl = C;
		}
	}
	ZacznijProbna(Najl, true);
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AKameraMapy* Kam = Cast<AKameraMapy>(PC->GetPawn()))
		{
			Kam->UstawOgnisko(Najl);
			float Odl = 0.f;
			if (FParse::Value(FCommandLine::Get(), TEXT("odl="), Odl) && Odl < 25000.f)
			{
				AKameraMapy::bBitwaBlisko = true;
				Kam->UstawOdl(FMath::Max(Odl, 8000.f));
			}
		}
	}
}

int32 ABitwa::NajblizszyWrog(int32 i) const
{
	int32 Naj = -1;
	float D = TNumericLimits<float>::Max();
	for (int32 j = 0; j < Oddz.Num(); ++j)
	{
		if (Oddz[j].Strona != Oddz[i].Strona && Oddz[j].Stoi())
		{
			const float Dj = Dl(Oddz[j].Srodek, Oddz[i].Srodek);
			if (Dj < D) { D = Dj; Naj = j; }
		}
	}
	return Naj;
}

int32 ABitwa::OddzialPod(FVector2D P, int32 Strona, const APlayerController* PC, FVector2D Ekran) const
{
	int32 Naj = -1;
	if (PC)
	{
		// Wedlug obrazu: kamera patrzy z ukosa, wiec klik w postac woja trafia w ziemie za nim; liczy sie postac.
		float Px = 30.f;
		for (int32 j = 0; j < Oddz.Num(); ++j)
		{
			if (Oddz[j].Strona != Strona || Oddz[j].bPoza)
			{
				continue;
			}
			for (const FWojownik& W : Oddz[j].Ludzie)
			{
				FVector2D S;
				if (Zywy(W) && PC->ProjectWorldLocationToScreen(W.L->GetActorLocation() + FVector(0, 0, 110), S, false)
					&& FVector2D::Distance(S, Ekran) < Px)
				{
					Px = FVector2D::Distance(S, Ekran);
					Naj = j;
				}
			}
		}
		if (Naj >= 0)
		{
			return Naj;
		}
	}
	float D = 420.f;
	for (int32 j = 0; j < Oddz.Num(); ++j)
	{
		if (Oddz[j].Strona != Strona || Oddz[j].bPoza)
		{
			continue;
		}
		for (const FWojownik& W : Oddz[j].Ludzie)
		{
			if (Zywy(W) && Dl(Poz(W), P) < D)
			{
				D = Dl(Poz(W), P);
				Naj = j;
			}
		}
	}
	return Naj;
}

void ABitwa::DoSzyku(FOddzial& O, float Szybkosc)
{
	// Napor (22.11.2026): w walce tylne szeregi nie stoja na swoich miejscach, tylko trzymaja sie tuz za plecami tego,
	// kto stoi przed nimi w kolumnie, i napieraja na niego tarcza. Linia nie rozpada sie w tlum, a luki zamykaja sie same.
	TMap<int32, int32> NaMiejscu;
	if (O.bWalczy)
	{
		for (int32 w = 0; w < O.Ludzie.Num(); ++w)
		{
			if (Zywy(O.Ludzie[w])) { NaMiejscu.Add(O.Ludzie[w].Slot, w); }
		}
	}
	for (FWojownik& W : O.Ludzie)
	{
		if (!Zywy(W))
		{
			continue;
		}
		// Co kilkanascie sekund woj przestepuje troche w bok albo w przod: szereg zyje, nie stoi jak wbity w ziemie.
		if (FMath::FRand() < 0.008f)
		{
			W.Luz = FVector2D(FMath::FRandRange(-35.f, 35.f), FMath::FRandRange(-30.f, 30.f));
		}
		FVector2D M = Miejsce(O, W.Slot) + W.Luz;
		bool bNapor = false;
		if (const int32* P = W.Slot >= O.Kol ? NaMiejscu.Find(W.Slot - O.Kol) : nullptr)
		{
			const FWojownik& Przed = O.Ludzie[*P];
			const FVector2D PP = Poz(Przed);
			if (Dl(PP, Miejsce(O, Przed.Slot)) < 260.f)
			{
				M = PP - O.Front * (Przestrzen + 8.f); // tuz za plecami, blizej niz pozwala rozpychanie
				bNapor = true;
			}
		}
		W.L->WalkaNapor(bNapor);
		if (W.Wrog >= 0 || W.L->Uderza())
		{
			continue; // w zwarciu: prowadzi go Zwarcie()
		}
		if (Dl(Poz(W), M) > (bNapor ? 30.f : 45.f))
		{
			W.L->WalkaIdz(M, (Szybkosc + 60.f) * (W.Zdrowie < O.Zdrowie ? 0.85f : 1.f)); // ranny kuleje
		}
		else
		{
			W.L->WalkaStoj(M + O.Front * 1000.f);
		}
	}
}

void ABitwa::RuchOddzialow(float Dt)
{
	AOsadaRodu* V = RodW >= 0 ? CelWyprawy() : nullptr;
	const bool bBrama = V && BramaStoi();
	for (int32 i = 0; i < Oddz.Num(); ++i)
	{
		FOddzial& O = Oddz[i];
		if (O.bPoza)
		{
			continue;
		}
		if (O.NaPolu() == 0)
		{
			O.bPoza = true;
			continue;
		}
		if (MarszW.Etap >= 2 && MarszW.Oddzialy.Contains(i))
		{
			continue; // w drodze (ukryci)
		}
		if (Palenie.Contains(i))
		{
			continue; // woje biegaja z ogniem (PalenieTick)
		}
		if (O.bUloz)
		{
			Uloz(O);
		}
		if (O.bZlamany)
		{
			// Ucieczka: biegiem od najblizszych wrogow. Daleko od nich raz moze sie zebrac, za drugim razem opuszcza pole.
			FVector2D Sr = FVector2D::ZeroVector;
			int32 N = 0;
			for (const FWojownik& W : O.Ludzie) { if (Zywy(W)) { Sr += Poz(W); ++N; } }
			O.Srodek = Sr / FMath::Max(1, N);
			float NajD = TNumericLimits<float>::Max();
			FVector2D Od = -O.Front;
			for (const FOddzial& E : Oddz)
			{
				if (E.Strona != O.Strona && E.Stoi() && Dl(E.Srodek, O.Srodek) < NajD)
				{
					NajD = Dl(E.Srodek, O.Srodek);
					Od = (O.Srodek - E.Srodek).GetSafeNormal();
				}
			}
			if (Od.IsNearlyZero()) { Od = -O.Front; }
			for (FWojownik& W : O.Ludzie)
			{
				if (Zywy(W) && NajD > 1500.f && !W.L->WasRecentlyRendered(0.5f))
				{
					// Uciekl poza kadr: "wraca do swojej osady" (znika, gdy gracz nie patrzy).
					W.bPoza = true;
					Odeslij(W);
					++O.Uciekli;
					continue;
				}
				if (Zywy(W) && !W.L->WRuchu())
				{
					W.Wrog = -1;
					W.L->WalkaIdz(Poz(W) + Od * 1200.f + FVector2D(FMath::FRandRange(-300.f, 300.f), FMath::FRandRange(-300.f, 300.f)), Bieg);
				}
			}
			if (NajD > Daleko)
			{
				if (!O.bZebral && bTrwa && !O.bKoniec)
				{
					O.Morale += 8.f * Dt;
					if (O.Morale >= 35.f)
					{
						O.bZlamany = false;
						O.bZebral = true;
						O.Front = -Od;
						O.Rozkaz = ERozkaz::Stoj;
						O.CelRuchu = O.Srodek;
					}
				}
				else
				{
					for (FWojownik& W : O.Ludzie)
					{
						if (Zywy(W))
						{
							W.bPoza = true;
							Odeslij(W);
							++O.Uciekli;
						}
					}
					O.bPoza = true;
				}
			}
			continue;
		}
		if (!bTrwa)
		{
			DoSzyku(O, Marsz);
			continue;
		}
		// Wrog (prosta SI): po kilku sekundach rusza na najblizszy oddzial gracza. Bandyci napadu bez obrony w poblizu ida
		// do osady, zabieraja czesc zapasow i odchodza z lupem tam, skad przyszli.
		if (O.Strona == 1 && !O.bKoniec && CzasBitwy - O.Start > 6.f
			&& (O.Rozkaz != ERozkaz::Atakuj || !Oddz.IsValidIndex(O.Cel) || !Oddz[O.Cel].Stoi()))
		{
			const int32 Naj = NajblizszyWrog(i);
			if (Naj >= 0 && (!O.bNapad || Dl(Oddz[Naj].Srodek, O.Srodek) < 7000.f))
			{
				O.Rozkaz = ERozkaz::Atakuj;
				O.Cel = Naj;
			}
			else if (O.bNapad)
			{
				const FVector2D Dokad = O.bLup ? O.Skad : CelNapadu;
				if (Dl(O.Srodek, Dokad) < 450.f)
				{
					if (!O.bLup)
					{
						Grabiez(O);
					}
					else
					{
						for (FWojownik& Wj : O.Ludzie)
						{
							if (Zywy(Wj))
							{
								Wj.bPoza = true;
								Odeslij(Wj);
								++O.Uciekli;
							}
						}
						O.bPoza = true;
						bLupUciekl = true;
						continue;
					}
				}
				else
				{
					O.Rozkaz = ERozkaz::Idz;
					O.CelRuchu = Dokad;
					O.Front = (Dokad - O.Srodek).GetSafeNormal();
				}
			}
			else
			{
				O.Rozkaz = ERozkaz::Stoj;
			}
		}
		if (O.bObronca && bBrama)
		{
			// Oblezenie: poki brama stoi, obroncy czekaja w murach (lucznicy strzelaja zza palisady do kogo siegna).
			O.Rozkaz = ERozkaz::Stoj;
			O.CelRuchu = O.Srodek;
		}
		// Stojacy oddzial wrecz nie patrzy biernie, jak wrog bije sie tuz obok (24.11.2026: "jedni czekali obok zamiast walczyc").
		if (O.Rozkaz == ERozkaz::Stoj && !O.bKoniec && !O.Strzela() && !(O.bObronca && bBrama))
		{
			const int32 Naj = NajblizszyWrog(i);
			if (Naj >= 0 && Dl(Oddz[Naj].Srodek, O.Srodek) < SamDoWalki)
			{
				O.Rozkaz = ERozkaz::Atakuj;
				O.Cel = Naj;
			}
		}
		if (O.Rozkaz == ERozkaz::Atakuj && (!Oddz.IsValidIndex(O.Cel) || !Oddz[O.Cel].Stoi()))
		{
			O.Cel = NajblizszyWrog(i);
			if (O.Cel < 0)
			{
				O.Rozkaz = ERozkaz::Stoj;
			}
		}
		float Szybkosc = Marsz * (1.f - O.Zmeczenie * 0.004f);
		if (O.Rozkaz == ERozkaz::Atakuj)
		{
			O.bDoFrontu = false;
			O.bBiegiem &= !O.bWalczy;
			Szybkosc = O.bBiegiem ? Bieg : Szybkosc; // podwojny prawy klik we wroga: biegiem
		}
		const FVector2D Bylo = O.Srodek;
		if (bBrama && O.Strona == 0)
		{
			// Szturm: atak wrecz na obroncow prowadzi pod brame (tam woje ja rabia, Oblezenie); marsz w mury tez konczy sie
			// pod brama.
			const AOsadaRodu::FBrama& Br = V->Bramy()[BramaW];
			const FVector2D Pod = Br.P + Br.Na * 150.f;
			if (O.Rozkaz == ERozkaz::Idz && V->WMurach(O.CelRuchu))
			{
				O.CelRuchu = Pod;
				O.FrontCelu = -Br.Na;
				O.bDoFrontu = true;
				O.Trasa.Reset();
			}
			else if (O.Rozkaz == ERozkaz::Atakuj && !O.Strzela() && Oddz.IsValidIndex(O.Cel) && Oddz[O.Cel].bObronca)
			{
				const FVector2D D = Pod - O.Srodek;
				const float L = D.Size();
				if (!O.bOkrzyk && L < 1900.f)
				{
					O.bOkrzyk = true;
					Okrzyk(O, 0.85f);
				}
				O.Front = (O.Front - Br.Na * Dt * 3.f).GetSafeNormal();
				if (L > 20.f)
				{
					O.Srodek += D / L * FMath::Min(L, (L < 1600.f ? Szarza : Szybkosc) * Dt);
				}
				DoSzyku(O, Szybkosc);
				continue;
			}
		}
		if (O.Rozkaz == ERozkaz::Idz)
		{
			// Idzie do punktu; gdy wrog wejdzie mu w droge, staje i walczy.
			// Marsz w szyku (22.11.2026): po punktach trasy (omija budynki); na dlugim marszu szyk obraca sie plynnie w strone
			// marszu i idzie wolniej, dopoki nie stanie frontem; na krotkim (do 12 m) od razu ustawia sie do frontu celu.
			const int32 W = NajblizszyWrog(i);
			if (O.Trasa.Num() > 0 && !O.Trasa.Last().Equals(O.CelRuchu, 1.f))
			{
				O.Trasa.Reset(); // rozkaz z innego miejsca (SI, odwrot): prosto do celu
			}
			while (O.Trasa.Num() > 1 && Dl(O.Trasa[0], O.Srodek) < 150.f) { O.Trasa.RemoveAt(0); }
			const FVector2D D = (O.Trasa.Num() > 0 ? O.Trasa[0] : O.CelRuchu) - O.Srodek;
			// Rozkaz gracza (22.11.2026) idzie do konca, takze odwrot spod wroga; staje sam tylko oddzial SI.
			if (Dl(O.CelRuchu, O.Srodek) < 40.f || (O.Strona != 0 && W >= 0 && Dl(Oddz[W].Srodek, O.Srodek) < Kontakt))
			{
				O.Rozkaz = ERozkaz::Stoj;
				O.bBiegiem = false;
				O.TempoGrupy = 1.f;
				O.bDoFrontu &= W < 0 || Dl(Oddz[W].Srodek, O.Srodek) >= Kontakt;
			}
			else
			{
				const FVector2D K = D.GetSafeNormal();
				const bool bDlugi = Dl(O.CelRuchu, O.Srodek) > 1200.f || !O.bDoFrontu;
				Obroc(O, bDlugi ? K : O.FrontCelu, Dt);
				const float Zgodnosc = bDlugi ? FMath::Clamp(FVector2D::DotProduct(O.Front, K), 0.f, 1.f) : 1.f;
				Szybkosc = (O.bBiegiem ? Bieg : Szybkosc) * O.TempoGrupy;
				O.Srodek += K * FMath::Min(D.Size(), Szybkosc * (0.25f + 0.75f * Zgodnosc) * Dt);
			}
		}
		else if (O.Rozkaz == ERozkaz::Stoj && O.bDoFrontu && !O.bWalczy)
		{
			// Doszedl: obraca sie do frontu wskazanego przez gracza.
			Obroc(O, O.FrontCelu, Dt);
			O.bDoFrontu = FVector2D::DotProduct(O.Front, O.FrontCelu) < 0.9995f;
		}
		else if (O.Rozkaz == ERozkaz::Atakuj && O.Strzela() && !O.bWalczy)
		{
			// Strzelcy (W2): podchodza na zasieg (z zapasem), staja frontem do celu i strzelaja (Strzelanie). Bez strzal
			// albo w zwarciu walcza wrecz jak inni.
			const FVector2D Do = Oddz[O.Cel].Srodek - O.Srodek;
			const float L = Do.Size();
			const float Zas = RodzajWojska(O.Rodzaj).Zasieg * 0.85f;
			if (L > 1.f)
			{
				O.Front = (O.Front + Do / L * Dt * 3.f).GetSafeNormal();
			}
			if (L > Zas)
			{
				O.Srodek += Do / L * FMath::Min(L - Zas, Szybkosc * Dt);
			}
		}
		else if (O.Rozkaz == ERozkaz::Atakuj)
		{
			// Podejscie do najblizszego brzegu szyku wroga (czolo, bok albo tyl) i stanie czolem do niego w odstepie Styk.
			// W zwarciu oddzial juz nie podchodzi: linia przesuwa sie tylko przez spychanie (Nacisk).
			const FOddzial& E = Oddz[O.Cel];
			const float OdE = E.bZwarty ? OdstepZwarty : OdstepLuzny;
			const FVector2D PrawoE(-E.Front.Y, E.Front.X);
			const FVector2D Wzgl = O.Srodek - E.Srodek;
			const float X = FMath::Clamp(FVector2D::DotProduct(Wzgl, E.Front), -(E.Szeregi - 1) * OdE, 0.f);
			const float PolSzer = (E.Kol - 1) * 0.5f * OdE;
			const float Y = FMath::Clamp(FVector2D::DotProduct(Wzgl, PrawoE), -PolSzer, PolSzer);
			const FVector2D Brzeg = E.Srodek + E.Front * X + PrawoE * Y;
			FVector2D Kier = O.Srodek - Brzeg;
			Kier = Kier.SizeSquared() > 1.f ? Kier.GetSafeNormal() : E.Front;
			if (X > -1.f && FVector2D::DotProduct(Wzgl, E.Front) > 0.f)
			{
				Kier = (Kier + E.Front).GetSafeNormal(); // przed czolem: staje naprzeciw, nie na skos
			}
			const FVector2D Cel = Brzeg + Kier * Styk;
			const FVector2D D = Cel - O.Srodek;
			const float L = D.Size();
			if (!O.bOkrzyk && L < 1900.f)
			{
				O.bOkrzyk = true;
				Okrzyk(O, 0.85f); // okrzyk przed szarza
			}
			O.Front = (O.Front + (-Kier) * Dt * 3.f).GetSafeNormal();
			if (L > 110.f || !O.bWalczy)
			{
				Szybkosc = L < 1600.f ? Szarza : Szybkosc;
				O.Srodek += D.GetSafeNormal() * FMath::Min(L, Szybkosc * Dt);
			}
		}
		if (V && V->WMurach(O.Srodek) != V->WMurach(Bylo) && !V->PrzyRozbitejBramie(O.Srodek))
		{
			O.Srodek = Bylo; // przez mur nie przejdzie, tylko przez rozbita brame
		}
		DoSzyku(O, Szybkosc);
	}
}

void ABitwa::Zwarcie(float Dt)
{
	// Linia frontu (21.11.2026): oddzialy stoja w szyku naprzeciw siebie, a walcza ci, ktorzy maja wroga tuz przed soba,
	// zwykle pierwszy szereg, para na pare. Woj nie odchodzi od swojego miejsca w szyku dalej niz na krok, wiec walka nie
	// zbija sie w kule. Wiekszosc ciosow z przodu konczy sie na tarczy albo zbiciu, dlatego rowne oddzialy bija sie dlugo.
	TMap<int32, int32> Napastnicy;
	for (const FOddzial& O : Oddz)
	{
		for (const FWojownik& W : O.Ludzie)
		{
			if (Zywy(W) && W.Wrog >= 0) { Napastnicy.FindOrAdd(W.Wrog * 1000 + W.WrogW)++; }
		}
	}
	for (int32 i = 0; i < Oddz.Num(); ++i)
	{
		FOddzial& O = Oddz[i];
		const bool bWalczyl = O.bWalczy;
		O.bWalczy = false;
		if (!O.Stoi())
		{
			for (FWojownik& W : O.Ludzie) { W.Wrog = -1; }
			continue;
		}
		if (bWalczyl)
		{
			Zmiany(O);
		}
		if (Palenie.Contains(i))
		{
			continue;
		}
		const bool bIdzie = O.Rozkaz == ERozkaz::Idz;
		for (int32 w = 0; w < O.Ludzie.Num(); ++w)
		{
			FWojownik& A = O.Ludzie[w];
			if (!Zywy(A))
			{
				continue;
			}
			const FVector2D PA = Poz(A);
			const FVector2D M = Miejsce(O, A.Slot);
			const bool bPierwszy = A.Slot < O.Kol;
			if (A.Wrog >= 0 && (!Oddz.IsValidIndex(A.Wrog) || !Oddz[A.Wrog].Ludzie.IsValidIndex(A.WrogW)
				|| !Zywy(Oddz[A.Wrog].Ludzie[A.WrogW]) || Dl(Poz(Oddz[A.Wrog].Ludzie[A.WrogW]), M) > (bIdzie ? 260.f : A.bObchodzi ? Obchodzenie + 300.f : Szukanie)))
			{
				A.Wrog = A.WrogW = -1;
				A.bObchodzi = false;
			}
			if (A.Wrog < 0)
			{
				// Wrog blisko swojego miejsca w szyku: pierwszy szereg siega dalej, tylni tylko tego, kto wszedl miedzy nich.
				// Oddzial w marszu (rozkaz gracza) bije tylko tych, ktorzy sa tuz przy nim, i idzie dalej.
				float D = bPierwszy && !bIdzie ? 420.f : 260.f;
				for (int32 j = 0; j < Oddz.Num(); ++j)
				{
					const FOddzial& E = Oddz[j];
					if (E.Strona == O.Strona || E.bPoza || Dl(E.Srodek, O.Srodek) > 3500.f)
					{
						continue;
					}
					for (int32 v = 0; v < E.Ludzie.Num(); ++v)
					{
						if (Zywy(E.Ludzie[v]))
						{
							const int32* N = Napastnicy.Find(j * 1000 + v);
							const float Dv = Dl(Poz(E.Ludzie[v]), M) + (N ? 140.f * *N : 0.f);
							if (Dv < D) { D = Dv; A.Wrog = j; A.WrogW = v; }
						}
					}
				}
				// Obejscie (24.11.2026, prosba: "jak regimenty w Total War zachodza, zeby walczyc", "jedni czekali obok"):
				// gdy oddzial juz walczy, woj bez przeciwnika nie czeka w szyku, tylko idzie do najblizszego wroga, ktorego
				// nikt (albo tylko jeden) nie bije - nadmiar ludzi naturalnie owija skrzydla i bije w bok i plecy.
				if (A.Wrog < 0 && bWalczyl && !bIdzie && !O.Strzela())
				{
					float Naj = Obchodzenie;
					for (int32 j = 0; j < Oddz.Num(); ++j)
					{
						const FOddzial& E = Oddz[j];
						if (E.Strona == O.Strona || E.bPoza || Dl(E.Srodek, O.Srodek) > 3500.f)
						{
							continue;
						}
						for (int32 v = 0; v < E.Ludzie.Num(); ++v)
						{
							const int32* N = Napastnicy.Find(j * 1000 + v);
							if (Zywy(E.Ludzie[v]) && (!N || *N < 2))
							{
								const float Dv = Dl(Poz(E.Ludzie[v]), PA) + (N ? 300.f * *N : 0.f);
								if (Dv < Naj) { Naj = Dv; A.Wrog = j; A.WrogW = v; A.bObchodzi = true; }
							}
						}
					}
				}
				else if (A.Wrog >= 0)
				{
					A.bObchodzi = false;
				}
				if (A.Wrog >= 0)
				{
					Napastnicy.FindOrAdd(A.Wrog * 1000 + A.WrogW)++;
				}
			}
			if (A.Wrog < 0)
			{
				// Tylne szeregi dopinguja walczacych.
				if (bWalczyl && !bPierwszy && FMath::FRand() < 0.004f)
				{
					A.L->WalkaOkrzyk(0.01f);
				}
				continue;
			}
			O.bWalczy = true;
			O.bZderzenie = true;
			FOddzial& EO = Oddz[A.Wrog];
			FWojownik& E = EO.Ludzie[A.WrogW];
			const FVector2D PE = Poz(E);
			const float D = Dl(PA, PE);
			if (A.L->Uderza())
			{
				continue;
			}
			if (D > ZasiegCiosu)
			{
				// Krok do przeciwnika, ale nie dalej od swojego miejsca niz ok. 1,8 m: linia zostaje linia.
				const FVector2D Wypad = PE + (PA - PE).GetSafeNormal() * (ZasiegCiosu - 40.f);
				A.L->WalkaIdz(A.bObchodzi ? Wypad : Dl(Wypad, M) < 180.f ? Wypad : M, A.bObchodzi ? Szarza : Marsz);
				continue;
			}
			A.L->WalkaStoj(PE);
			A.Odnowa -= Dt;
			if (A.Odnowa > 0.f)
			{
				continue;
			}
			A.L->WalkaCios(PE);
			UDzwieki::Graj(this, "Zamach", A.L->GetActorLocation(), EDzwiek::Walka, 0.45f);
			// Filmowa para wymienia ciosy zywiej (skutek ten sam, tylko szybciej i z kontrami po bloku).
			A.Odnowa = (A.bFilm ? FMath::FRandRange(0.75f, 1.05f) : FMath::FRandRange(1.0f, 1.5f)) * O.Tempo + O.Zmeczenie * 0.006f;
			// Szansa trafienia: oddzial, doswiadczenie, zmeczenie, pancerz, rany. Z przodu wiekszosc ciosow zatrzymuje tarcza
			// albo zbicie; z boku i z tylu obrona jest slaba.
			float P = O.Trafienie + 0.06f * A.Doswiadczenie - 0.04f * E.Doswiadczenie - EO.Pancerz * (1.f - O.Przebicie) - O.Zmeczenie * 0.002f
				- (A.Zdrowie < O.Zdrowie ? 0.06f : 0.f);
			const float Kat = FVector2D::DotProduct(EO.Front, (PA - PE).GetSafeNormal());
			bool bCzolo = false;
			if (EO.bZlamany)
			{
				P += 0.3f; // cios w plecy uciekajacego
			}
			else if (Kat < -0.3f)
			{
				P += 0.2f;
				EO.Flanka = 1.f;
			}
			else if (Kat < 0.35f)
			{
				P = P * 0.8f + 0.05f;
				EO.Flanka = FMath::Max(EO.Flanka, 0.5f);
			}
			else
			{
				P *= EO.Blok + 0.25f * EO.Zmeczenie / 100.f; // zmeczony gorzej sie zaslania (tarczownicy najlepiej)
				bCzolo = true;
			}
			// Skutek widac dopiero w chwili uderzenia (CiosyTick): tarcza idzie w gore tuz przed nim.
			const bool bTraf = FMath::FRand() < P;
			Ciosy.Add({ i, w, A.Wrog, A.WrogW, ALudzik::DoUderzenia, bTraf, !bTraf && bCzolo && FMath::FRand() < 0.7f, false });
		}
	}
}

void ABitwa::CiosyTick(float Dt)
{
	for (int32 k = Ciosy.Num() - 1; k >= 0; --k)
	{
		FCios& C = Ciosy[k];
		C.Za -= Dt;
		if (!Oddz.IsValidIndex(C.OA) || !Oddz.IsValidIndex(C.OE) || !Oddz[C.OA].Ludzie.IsValidIndex(C.WA)
			|| !Oddz[C.OE].Ludzie.IsValidIndex(C.WE) || !Zywy(Oddz[C.OE].Ludzie[C.WE]))
		{
			Ciosy.RemoveAtSwap(k);
			continue;
		}
		FOddzial& O = Oddz[C.OA];
		FOddzial& EO = Oddz[C.OE];
		FWojownik& A = O.Ludzie[C.WA];
		FWojownik& E = EO.Ludzie[C.WE];
		if (C.bBlok && !C.bZaslona && C.Za <= 0.16f)
		{
			C.bZaslona = true;
			E.L->WalkaBlok();
		}
		if (C.Za > 0.f)
		{
			continue;
		}
		const FVector2D PE = Poz(E);
		const FVector2D PA = A.L.IsValid() ? Poz(A) : PE - EO.Front * 150.f;
		const FVector2D Kier = (PE - PA).GetSafeNormal();
		// Dzwiek uderzenia: tarcza (drewno), maczuga, wlocznia o drzewce, reszta stal o stal.
		const FVector Ucho(PE, FSwiat::Wysokosc(PE.X, PE.Y) + 150.f);
		const bool bTarcza = C.bBlok && E.L->MaTarcze();
		UDzwieki::Graj(this, bTarcza ? FName("Tarcza") : O.Ikona == EIkonaBroni::Palka ? FName("Maczuga")
			: O.Ikona == EIkonaBroni::Wlocznia && FMath::RandBool() ? FName("Drzewce") : FName("Szczek"), Ucho, EDzwiek::Walka,
			C.bTraf ? 0.6f : C.bBlok ? 1.f : 0.f);
		if (C.bTraf)
		{
			UDzwieki::Graj(this, "Trafienie", Ucho, EDzwiek::Walka);
			Rana(EO, E, 1.f, Kier, O.Ciecie, PA);
			if (!E.bZyje)
			{
				// Zwyciezca zyskuje ducha i czasem doswiadczenie.
				O.Morale = FMath::Min(100.f, O.Morale + 1.f);
				if (FMath::FRand() < 0.35f)
				{
					A.Doswiadczenie = FMath::Min(2, A.Doswiadczenie + 1);
				}
				A.Wrog = A.WrogW = -1;
			}
		}
		else if (C.bBlok)
		{
			// Cios na tarczy albo zbity bronia: obaj drgaja, leca iskry. Filmowa para: obronca od razu kontruje.
			E.L->WalkaWstrzas();
			if (A.L.IsValid())
			{
				A.L->WalkaWstrzas();
			}
			Iskry(FVector((PA + PE) * 0.5f + Kier * 25.f, FSwiat::Wysokosc(PE.X, PE.Y) + 160.f), -Kier, E.bFilm ? 7 : 3);
			if (E.bFilm)
			{
				E.Odnowa = FMath::Min(E.Odnowa, 0.3f);
			}
		}
		Ciosy.RemoveAtSwap(k);
	}
}

void ABitwa::Filmowe()
{
	// Do 4 par w zwarciu najblizej srodka ekranu (na ekranie): one walcza filmowo, reszta zwyczajnie.
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	for (FOddzial& O : Oddz)
	{
		for (FWojownik& W : O.Ludzie)
		{
			if (W.bFilm && W.L.IsValid()) { W.L->UstawFilm(false); }
			W.bFilm = false;
		}
	}
	if (!PC || !bWidac)
	{
		return;
	}
	int32 SX = 0, SY = 0;
	PC->GetViewportSize(SX, SY);
	const FVector2D Srodek(SX * 0.5f, SY * 0.5f);
	struct FKand { float D; int32 O, W; };
	TArray<FKand> K;
	for (int32 i = 0; i < Oddz.Num(); ++i)
	{
		for (int32 w = 0; w < Oddz[i].Ludzie.Num(); ++w)
		{
			const FWojownik& W = Oddz[i].Ludzie[w];
			FVector2D S;
			if (Zywy(W) && W.Wrog >= 0 && Oddz.IsValidIndex(W.Wrog) && Oddz[W.Wrog].Ludzie.IsValidIndex(W.WrogW)
				&& Zywy(Oddz[W.Wrog].Ludzie[W.WrogW]) && PC->ProjectWorldLocationToScreen(W.L->GetActorLocation(), S, false)
				&& S.X > 0 && S.Y > 0 && S.X < SX && S.Y < SY)
			{
				K.Add({ float(FVector2D::Distance(S, Srodek)), i, w });
			}
		}
	}
	K.Sort([](const FKand& A, const FKand& B) { return A.D < B.D; });
	int32 Par = 0;
	for (const FKand& C : K)
	{
		FWojownik& W = Oddz[C.O].Ludzie[C.W];
		FWojownik& E = Oddz[W.Wrog].Ludzie[W.WrogW];
		if (W.bFilm || E.bFilm)
		{
			continue;
		}
		W.bFilm = E.bFilm = true;
		W.L->UstawFilm(true);
		E.L->UstawFilm(true);
		if (++Par >= 4)
		{
			break;
		}
	}
}

void ABitwa::Iskry(FVector Gdzie, FVector2D Kier, int32 Ile)
{
	if (!bWidac || IskryL.Num() > 200)
	{
		return;
	}
	for (int32 k = 0; k < Ile; ++k)
	{
		const FVector2D R = (Kier + FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f))).GetSafeNormal();
		IskryL.Add({ Gdzie, FVector(R * FMath::FRandRange(150.f, 420.f), FMath::FRandRange(80.f, 320.f)), FMath::FRandRange(0.12f, 0.3f) });
	}
}

void ABitwa::IskryTick(float Dt)
{
	if (IskryL.Num() == 0 && IskryC->GetInstanceCount() == 0)
	{
		return;
	}
	TArray<FTransform> T;
	for (int32 i = IskryL.Num() - 1; i >= 0; --i)
	{
		FIskra& S = IskryL[i];
		S.T -= Dt;
		S.V.Z -= 900.f * Dt;
		S.P += S.V * Dt;
		if (S.T <= 0.f)
		{
			IskryL.RemoveAtSwap(i);
			continue;
		}
		// Iskra rozciagnieta wzdluz lotu (smuga), maleje pod koniec.
		T.Add(FTransform(S.V.Rotation(), S.P, FVector(1.4f + S.V.Size() * 0.004f, 1.4f, 1.4f) * FMath::Min(1.f, S.T * 6.f)));
	}
	IskryC->ClearInstances();
	if (T.Num() > 0)
	{
		IskryC->AddInstances(T, false, true);
	}
}

void ABitwa::Rana(FOddzial& EO, FWojownik& E, float Ile, FVector2D Kier, float Ciecie, FVector2D Od, bool bStrzala, bool bBelt)
{
	E.Zdrowie -= Ile;
	EO.Morale -= 0.5f;
	Krew(E.L->GetActorLocation() + FVector(0.f, 0.f, 150.f), Kier, bStrzala ? 4 : 6, 1.f);
	if (bStrzala)
	{
		E.L->WbijStrzale(Kier, bBelt, false);
	}
	if (E.Zdrowie > 0.f)
	{
		E.L->WalkaTrafiony(Kier, bStrzala);
		E.L->UstawRane(1.f - E.Zdrowie / FMath::Max(0.1f, EO.Zdrowie), bKrew);
		UDzwieki::Graj(this, "Rana", E.L->GetActorLocation() + FVector(0.f, 0.f, 150.f), EDzwiek::Glosy, 0.8f);
		return;
	}
	Zabij(EO, E, Kier, Ciecie, Od, bStrzala);
}

void ABitwa::Zabij(FOddzial& EO, FWojownik& E, FVector2D Kier, float Ciecie, FVector2D Od, bool bStrzala)
{
	// Polegl: lezy na polu; jego oddzial traci ducha, sasiedzi troche. Tylni zajmuja jego miejsce.
	const FVector2D PE = Poz(E);
	E.bZyje = false;
	E.Smierc = CzasBitwy;
	E.Wrog = E.WrogW = -1;
	E.bFilm = false;
	const bool bCiety = bKrew && FMath::FRand() < Ciecie;
	E.L->WalkaPadnij(Kier, bStrzala, !bCiety && FMath::FRand() < 0.25f); // co czwarty kona jeszcze chwile
	UDzwieki::Graj(this, "Smierc", E.L->GetActorLocation() + FVector(0.f, 0.f, 150.f), EDzwiek::Glosy); // krzyk, jek
	if (bKrew)
	{
		// Kaluza pod cialem; mocny cios (czesciej toporem) odcina glowe albo reke, z kikuta krew tryska jeszcze chwile.
		Krew(E.L->GetActorLocation() + FVector(0.f, 0.f, 140.f), Kier, 10, 1.2f);
		Plama(PE + Kier * 70.f, FMath::FRandRange(1.f, 1.6f));
		if (bCiety)
		{
			const int32 Co = FMath::FRand() < 0.45f ? 0 : (FMath::RandBool() ? 1 : 2);
			const FVector Ciete = E.L->Odetnij(Co, Od);
			Krew(Ciete, Kier, 18, 1.5f);
			Plama(FVector2D(Ciete) + Kier * 130.f, FMath::FRandRange(0.7f, 1.2f));
			Tryski.Add({ E.L, FMath::FRandRange(1.8f, 3.f), 0.f });
		}
	}
	++EO.Polegli;
	EO.bUloz = true;
	EO.Morale -= 2.f + 30.f / FMath::Max(1, EO.Poczatkowo);
	for (FOddzial& S : Oddz)
	{
		if (&S != &EO && S.Strona == EO.Strona && Dl(S.Srodek, EO.Srodek) < 1500.f)
		{
			S.Morale -= 0.4f;
		}
	}
}

void ABitwa::Strzelanie(float Dt)
{
	// Strzelcy (W2): kazdy woj poza zwarciem, z pociskami, co Przeladowanie strzela w losowego woja celu. Cel: rozkazany
	// (atak), a bez rozkazu najblizszy wrog w zasiegu. W marszu nie strzelaja. Rozrzut rosnie z odlegloscia.
	for (int32 i = 0; i < Oddz.Num(); ++i)
	{
		FOddzial& O = Oddz[i];
		if (!O.Stoi() || !O.Strzela() || O.Rozkaz == ERozkaz::Idz)
		{
			continue;
		}
		const FRodzajWojska& R = RodzajWojska(O.Rodzaj);
		int32 C = O.Rozkaz == ERozkaz::Atakuj ? O.Cel : -1;
		if (!Oddz.IsValidIndex(C) || Oddz[C].bPoza || Oddz[C].NaPolu() == 0)
		{
			C = NajblizszyWrog(i);
		}
		if (C < 0 || Dl(Oddz[C].Srodek, O.Srodek) > R.Zasieg)
		{
			continue;
		}
		const FOddzial& E = Oddz[C];
		TArray<int32> Cele;
		for (int32 v = 0; v < E.Ludzie.Num(); ++v) { if (Zywy(E.Ludzie[v])) { Cele.Add(v); } }
		if (Cele.Num() == 0)
		{
			continue;
		}
		const bool bBelt = O.Rodzaj == ERodzajWojska::Kusznicy;
		for (FWojownik& W : O.Ludzie)
		{
			if (!Zywy(W) || W.Wrog >= 0 || W.Pociski <= 0 || W.L->Uderza())
			{
				continue;
			}
			W.Odnowa -= Dt;
			if (W.Odnowa > 0.f)
			{
				continue;
			}
			W.Odnowa = R.Przeladowanie * FMath::FRandRange(0.85f, 1.25f);
			--W.Pociski;
			const FVector Z = W.L->GetActorLocation() + FVector(0.f, 0.f, 200.f);
			FVector2D Do = Poz(E.Ludzie[Cele[FMath::RandRange(0, Cele.Num() - 1)]]);
			const float Odl = Dl(FVector2D(Z), Do);
			const float Kat = FMath::FRandRange(0.f, 2.f * PI);
			Do += FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * R.Rozrzut * Odl / 1000.f * FMath::Sqrt(FMath::FRand());
			FPocisk P;
			P.A = Z;
			P.B = FVector(Do, FSwiat::Wysokosc(Do.X, Do.Y) + 25.f);
			P.T = Odl / (bBelt ? 3800.f : 2300.f) + 0.25f;
			P.Luk = Odl * (bBelt ? 0.05f : 0.2f);
			P.Strona = O.Strona;
			P.Oddzial = i;
			P.Rana = R.Rana;
			P.Przebicie = O.Przebicie;
			P.bBelt = bBelt;
			Pociski.Add(P);
			W.L->WalkaStrzal(Do);
			UDzwieki::Graj(this, bBelt ? "Kusza" : "Luk", Z, EDzwiek::Walka, 0.7f);
		}
	}
}

void ABitwa::PociskiTick(float Dt)
{
	if (Pociski.Num() == 0 && LeceC->GetInstanceCount() == 0)
	{
		return;
	}
	auto Punkt = [](const FPocisk& P, float A) { return FMath::Lerp(P.A, P.B, A) + FVector(0.f, 0.f, 4.f * P.Luk * A * (1.f - A)); };
	TArray<FTransform> T;
	for (int32 i = Pociski.Num() - 1; i >= 0; --i)
	{
		FPocisk& P = Pociski[i];
		P.Czas += Dt;
		const float A = FMath::Min(1.f, P.Czas / P.T);
		const FVector X = Punkt(P, A);
		const FVector Kier = (Punkt(P, FMath::Min(1.f, A + 0.02f)) - Punkt(P, FMath::Max(0.f, A - 0.02f))).GetSafeNormal();
		const FVector Skala = P.bBelt ? FVector(0.9f, 1.8f, 1.8f) : FVector(1.4f);
		if (A < 1.f)
		{
			if (!P.bSwist && A > 0.75f)
			{
				P.bSwist = true; // swist nad celem, tuz przed upadkiem
				UDzwieki::Graj(this, "Swist", X, EDzwiek::Walka, 0.5f);
			}
			T.Add(FTransform(Kier.Rotation(), X, Skala));
			continue;
		}
		// Upadek: najblizszy zywy wrog przy tym miejscu moze oberwac. Z przodu oddzialu tarcza zatrzymuje wiekszosc strzal
		// (belt czesciej przechodzi), pancerz troche; samo swistanie strzal odbiera ducha.
		const FVector2D L(P.B);
		const FVector2D Lot = (L - FVector2D(P.A)).GetSafeNormal();
		int32 BO = -1, BW = -1;
		float BD = 75.f;
		for (int32 j = 0; j < Oddz.Num(); ++j)
		{
			FOddzial& EO = Oddz[j];
			if (EO.Strona == P.Strona || EO.bPoza || Dl(EO.Srodek, L) > 3000.f)
			{
				continue;
			}
			if (Dl(EO.Srodek, L) < 700.f)
			{
				EO.Morale -= 0.08f;
			}
			for (int32 v = 0; v < EO.Ludzie.Num(); ++v)
			{
				if (Zywy(EO.Ludzie[v]) && Dl(Poz(EO.Ludzie[v]), L) < BD)
				{
					BD = Dl(Poz(EO.Ludzie[v]), L);
					BO = j;
					BW = v;
				}
			}
		}
		bool bWbita = true;
		if (BO >= 0)
		{
			FOddzial& EO = Oddz[BO];
			FWojownik& E = EO.Ludzie[BW];
			const bool bZPrzodu = FVector2D::DotProduct(EO.Front, -Lot) > 0.3f && !EO.bZlamany;
			float Szansa = FMath::Max(0.2f, 1.f - 3.f * EO.Pancerz * (1.f - P.Przebicie));
			if (bZPrzodu && E.L->MaTarcze())
			{
				Szansa *= P.bBelt ? 0.6f : 0.3f;
			}
			if (FMath::FRand() < Szansa)
			{
				Rana(EO, E, P.Rana, Lot, 0.04f, FVector2D(P.A), true, P.bBelt);
				EO.Morale -= 0.4f;
				bWbita = false;
			}
			else if (bZPrzodu)
			{
				E.L->WalkaBlok();
				E.L->WalkaWstrzas();
				if (E.L->MaTarcze())
				{
					UDzwieki::Graj(this, "Tarcza", E.L->GetActorLocation() + FVector(0.f, 0.f, 150.f), EDzwiek::Walka, 0.6f);
				}
				if (E.L->MaTarcze() && FMath::FRand() < 0.6f)
				{
					E.L->WbijStrzale(Lot, P.bBelt, true); // strzala zostaje w tarczy
					bWbita = false;
				}
			}
		}
		if (bWbita && WbiteC->GetStaticMesh())
		{
			// Strzala wbita w ziemie (do 600 na bitwe, potem najstarsze znikaja).
			constexpr int32 MaxWbitych = 600;
			const FTransform TW(Kier.Rotation(), P.B - Kier * 22.f, Skala);
			if (WbiteC->GetInstanceCount() < MaxWbitych)
			{
				WbiteC->AddInstance(TW, true);
			}
			else
			{
				WbiteC->UpdateInstanceTransform(NastWbita % MaxWbitych, TW, true, true);
			}
			++NastWbita;
		}
		Pociski.RemoveAtSwap(i);
	}
	LeceC->ClearInstances();
	if (T.Num() > 0)
	{
		LeceC->AddInstances(T, false, true);
	}
}

void ABitwa::Uloz(FOddzial& O)
{
	// Szyk po stratach: zywi dostaja miejsca od pierwszego szeregu, od srodka na boki. Kazde miejsce bierze najblizszy
	// wolny woj, a odleglosc w bok liczy sie bardziej niz w glab, wiec na miejsce poleglego wchodzi ten, kto stal za nim.
	O.bUloz = false;
	TArray<int32> Zywi;
	for (int32 w = 0; w < O.Ludzie.Num(); ++w)
	{
		if (Zywy(O.Ludzie[w])) { Zywi.Add(w); }
	}
	const int32 N = Zywi.Num();
	if (N == 0)
	{
		return;
	}
	const int32 StaraKol = O.Kol;
	if (O.KolGracza > 0)
	{
		// Szerokosc wybrana przez gracza zostaje: straty zmniejszaja glebokosc szyku.
		O.Kol = FMath::Clamp(O.KolGracza, 1, N);
		O.Szeregi = FMath::DivideAndRoundUp(N, O.Kol);
	}
	else
	{
		O.Szeregi = N >= 6 ? 3 : 1;
		O.Kol = FMath::Max(1, FMath::DivideAndRoundUp(N, O.Szeregi));
	}
	TArray<int32> Miejsca;
	for (int32 k = 0; k < O.Szeregi * O.Kol; ++k) { Miejsca.Add(k); }
	const float Srodkowa = (O.Kol - 1) * 0.5f;
	Miejsca.Sort([&](int32 A, int32 B)
	{
		const int32 RA = A / O.Kol, RB = B / O.Kol;
		return RA != RB ? RA < RB : FMath::Abs(A % O.Kol - Srodkowa) < FMath::Abs(B % O.Kol - Srodkowa);
	});
	Miejsca.SetNum(N);
	const FVector2D Prawo(-O.Front.Y, O.Front.X);
	TArray<bool> Wziety;
	Wziety.Init(false, N);
	for (int32 k : Miejsca)
	{
		const FVector2D M = Miejsce(O, k);
		int32 Naj = -1;
		float Koszt = TNumericLimits<float>::Max();
		for (int32 z = 0; z < N; ++z)
		{
			if (Wziety[z])
			{
				continue;
			}
			const FVector2D D = Poz(O.Ludzie[Zywi[z]]) - M;
			const float Kz = FMath::Abs(FVector2D::DotProduct(D, Prawo)) * 1.6f + FMath::Abs(FVector2D::DotProduct(D, O.Front));
			if (Kz < Koszt) { Koszt = Kz; Naj = z; }
		}
		Wziety[Naj] = true;
		O.Ludzie[Zywi[Naj]].Slot = k;
	}
	if (StaraKol != O.Kol)
	{
		UE_LOG(LogTemp, Verbose, TEXT("Bitwa: %s zweza szyk do %d kolumn"), *O.Nazwa, O.Kol);
	}
}

void ABitwa::Okrzyk(FOddzial& O, float Szansa)
{
	// Jeden nagrany okrzyk grupy na caly oddzial (nie kazdy woj osobno).
	UDzwieki::Graj(O.Ludzie.Num() && O.Ludzie[0].L.IsValid() ? (UObject*)O.Ludzie[0].L.Get() : nullptr, "Okrzyk",
		FVector(O.Srodek, FSwiat::Wysokosc(O.Srodek.X, O.Srodek.Y) + 150.f), EDzwiek::Glosy);
	for (FWojownik& W : O.Ludzie)
	{
		if (Zywy(W) && FMath::FRand() < Szansa)
		{
			W.L->WalkaOkrzyk(FMath::FRandRange(0.f, 0.7f));
		}
	}
}

void ABitwa::Zmiany(FOddzial& O)
{
	if (O.Kol <= 0 || FMath::FRand() > 0.02f)
	{
		return; // srednio raz na 5 s w oddziale
	}
	int32 Ranny = -1;
	for (int32 w = 0; w < O.Ludzie.Num(); ++w)
	{
		const FWojownik& W = O.Ludzie[w];
		if (Zywy(W) && W.Slot < O.Kol && W.Zdrowie < O.Zdrowie && !W.L->Uderza()
			&& (Ranny < 0 || W.Zdrowie < O.Ludzie[Ranny].Zdrowie))
		{
			Ranny = w;
		}
	}
	if (Ranny < 0)
	{
		return;
	}
	for (FWojownik& Z : O.Ludzie)
	{
		if (Zywy(Z) && Z.Slot == O.Ludzie[Ranny].Slot + O.Kol && Z.Zdrowie > O.Ludzie[Ranny].Zdrowie)
		{
			Swap(Z.Slot, O.Ludzie[Ranny].Slot);
			Z.Wrog = Z.WrogW = -1;
			O.Ludzie[Ranny].Wrog = O.Ludzie[Ranny].WrogW = -1;
			return;
		}
	}
}

void ABitwa::Nacisk(float Dt)
{
	// Spychanie: dwa oddzialy stykajace sie czolami pchaja sie; silniejszy (duch, sily, liczba) powoli przesuwa linie,
	// slabszy cofa sie krok po kroku i traci troche ducha. Widac, kto wygrywa, zanim ktokolwiek ucieknie.
	auto Sila = [](const FOddzial& O)
	{
		return (0.3f + O.Morale / 100.f) * (1.2f - O.Zmeczenie / 100.f) * FMath::Sqrt(float(O.NaPolu()));
	};
	for (int32 i = 0; i < Oddz.Num(); ++i)
	{
		for (int32 j = i + 1; j < Oddz.Num(); ++j)
		{
			FOddzial& A = Oddz[i];
			FOddzial& B = Oddz[j];
			if (A.Strona == B.Strona || !A.Stoi() || !B.Stoi() || !A.bWalczy || !B.bWalczy
				|| FVector2D::DotProduct(A.Front, B.Front) > -0.6f)
			{
				continue;
			}
			const FVector2D D = B.Srodek - A.Srodek;
			const FVector2D Prawo(-A.Front.Y, A.Front.X);
			const float Szer = (A.Kol + B.Kol) * 0.5f * OdstepZwarty;
			if (FMath::Abs(FVector2D::DotProduct(D, A.Front)) > Styk + 120.f || FMath::Abs(FVector2D::DotProduct(D, Prawo)) > Szer)
			{
				continue;
			}
			const float SA = Sila(A), SB = Sila(B);
			const float R = (SA - SB) / FMath::Max(0.01f, SA + SB);
			const FVector2D Ruch = A.Front * (35.f * R * Dt);
			A.Srodek += Ruch;
			B.Srodek += Ruch;
			(R > 0.f ? B : A).Morale -= FMath::Abs(R) * 1.2f * Dt;
		}
	}
}

void ABitwa::MoraleOddzialow(float Dt)
{
	for (int32 i = 0; i < Oddz.Num(); ++i)
	{
		FOddzial& O = Oddz[i];
		if (!O.Stoi())
		{
			continue;
		}
		const bool bBiegnie = O.bBiegiem && (O.Rozkaz == ERozkaz::Idz || O.Rozkaz == ERozkaz::Atakuj);
		O.Zmeczenie = FMath::Clamp(O.Zmeczenie + (O.bWalczy ? 1.1f : bBiegnie ? 1.6f : -2.f) * Dt, 0.f, 100.f);
		// Wrog w boku albo w plecach lamie ducha kilka razy szybciej niz walka czolem.
		for (const FOddzial& E : Oddz)
		{
			if (E.Strona == O.Strona || !E.Stoi() || !E.bWalczy || E.Cel != i || Dl(E.Srodek, O.Srodek) > 1800.f)
			{
				continue;
			}
			const float Kat = FVector2D::DotProduct(O.Front, (E.Srodek - O.Srodek).GetSafeNormal());
			if (Kat < -0.3f)
			{
				O.Morale -= 6.f * Dt;
				O.Flanka = 1.f;
			}
			else if (Kat < 0.45f)
			{
				O.Morale -= 3.f * Dt;
				O.Flanka = FMath::Max(O.Flanka, 0.6f);
			}
		}
		if (O.Zmeczenie > 70.f)
		{
			O.Morale -= 0.6f * Dt;
		}
		if (!O.bWalczy)
		{
			// Z dala od walki duch wraca, ale nie ponad to, co zostawiaja straty.
			const float Sufit = 100.f - 40.f * O.Polegli / FMath::Max(1, O.Poczatkowo);
			if (O.Morale < Sufit)
			{
				O.Morale = FMath::Min(Sufit, O.Morale + 1.5f * Dt);
			}
		}
		O.Flanka = FMath::Max(0.f, O.Flanka - 0.5f * Dt);
		if (O.Morale > 0.f)
		{
			continue;
		}
		// Zlamany: ucieka. Sasiedzi traca ducha, wrogowie go nabieraja.
		O.Morale = 0.f;
		O.bZlamany = true;
		for (FWojownik& W : O.Ludzie) { W.Wrog = -1; }
		for (FOddzial& S : Oddz)
		{
			if (&S == &O || !S.Stoi())
			{
				continue;
			}
			if (S.Strona == O.Strona && Dl(S.Srodek, O.Srodek) < 2500.f)
			{
				S.Morale -= 12.f;
			}
			else if (S.Strona != O.Strona)
			{
				S.Morale = FMath::Min(100.f, S.Morale + 10.f);
				if (Dl(S.Srodek, O.Srodek) < 3000.f)
				{
					Okrzyk(S, 0.6f); // wrog ucieka!
				}
			}
		}
		UE_LOG(LogTemp, Log, TEXT("Bitwa: oddzial %s sie zlamal (polegli %d)"), *O.Nazwa, O.Polegli);
	}
}

void ABitwa::SprawdzKoniec()
{
	// Oddzialy ruszenia, ktore cale zeszly z pola (uciekli, polegli): rozwiazane, ocaleni sa juz przy pracy.
	for (int32 m = 0; m < Mil.Num(); ++m)
	{
		if (Mil[m].Powolana() && (!Oddz.IsValidIndex(Mil[m].Oddzial) || Oddz[Mil[m].Oddzial].bPoza))
		{
			Rozwiaz(m, false);
			return; // Rozwiaz wywoluje SprawdzKoniec jeszcze raz
		}
	}
	bool bStoi[2] = { false, false };
	bool bNapadu = false;
	for (const FOddzial& O : Oddz)
	{
		bStoi[O.Strona] |= O.Stoi();
		bNapadu |= O.Strona == 1 && O.bNapad && O.Stoi();
	}
	bWalkaTrwa = bStoi[1];
	// Koniec walki: wrog rozbity albo odszedl z lupem; w bitwie probnej tez przegrana gracza. Gra toczy sie dalej, krotka
	// wiadomosc u gory. Powolane ruszenie zostaje na polu, az gracz je rozwiaze.
	if (bWrogByl && (!bStoi[1] || (!bStoi[0] && !bNapadu)))
	{
		bWrogByl = false;
		Zwyciezca = (!bStoi[1] && !bLupUciekl) ? 0 : 1;
		int32 Pol[2] = { 0, 0 };
		for (const FOddzial& O : Oddz) { Pol[O.Strona] += O.Polegli; }
		const bool bBylLup = !Lup.Puste() || LupGroszy > 0;
		FString T = bLupUciekl ? FString::Printf(TEXT("Bandyci odeszli z łupem: %s"), *OpisTowaru(Lup, LupGroszy))
			: Zwyciezca == 1 ? FString(TEXT("Bitwa przegrana, woje uciekli"))
			: bBylLup ? FString::Printf(TEXT("Bandyci rozbici, łup odzyskany: %s"), *OpisTowaru(Lup, LupGroszy))
			: FString(TEXT("Bandyci rozbici"));
		const FString Nazwa = RodW >= 0 && Rody() && Rody()->Lista().IsValidIndex(RodW) ? Rody()->Lista()[RodW].Nazwa : FString();
		if (RodW >= 0)
		{
			T = Zwyciezca == 0 ? FString::Printf(TEXT("Osada %s zdobyta! Zdecyduj, co z nią zrobić"), *Nazwa)
				: FString::Printf(TEXT("Wyprawa na %s odparta, ocaleni wracają do domu"), *Nazwa);
		}
		T += FString::Printf(TEXT(" (polegli: twoi %d, wrogowie %d)"), Pol[0], Pol[1]);
		if (RodW >= 0 && Zwyciezca == 0)
		{
			bZdobyta = true; // panel wyboru (ZrobInterfejs)
		}
		else if (RodW >= 0)
		{
			if (Rody()) { Rody()->PoWyprawie(RodW, -1, Pol[1]); }
			KoniecWyprawy();
		}
		if (Zwyciezca == 0 && bBylLup)
		{
			if (AOsada* Wies = Osada()) { Wies->Zwroc(Lup, LupGroszy); }
		}
		Lup = FSurowce();
		LupGroszy = 0;
		bLupUciekl = false;
		bPauza = false;
		CzasKonca = CzasBitwy;
		for (FOddzial& O : Oddz)
		{
			O.bKoniec |= O.Milicja < 0; // woje bitwy probnej i wrogowie schodza z pola
			if (O.Rozkaz == ERozkaz::Atakuj)
			{
				O.Rozkaz = ERozkaz::Stoj;
				O.CelRuchu = O.Srodek;
			}
			for (FWojownik& W : O.Ludzie) { W.Wrog = -1; }
		}
		if (AOsada* Wies = Osada()) { Wies->Ogloszenie(T); }
		UE_LOG(LogTemp, Log, TEXT("Bitwa: koniec walki po %.0f s: %s"), CzasBitwy, *T);
		MiaryB.Koniec = CzasBitwy;
		if (bDziennik)
		{
			const FMiary& M = MiaryB;
			DoDziennika(FString::Printf(TEXT("t=%.1f KONIEC: %s | srednio walczy %.0f%% zywych, bezczynnych przy wrogu %.1f (najwiecej %d), pierwsze zwarcie t=%.1f"),
				CzasBitwy, *T, M.Zywych > 0 ? 100.0 * M.Walczacych / M.Zywych : 0.0, M.Probek ? M.Bezczynnych / M.Probek : 0.0, M.MaxBezczynnych, M.Zwarcie));
		}
	}
	// Pole czynne, poki jakis oddzial na nim stoi albo jeszcze z niego schodzi.
	bool bCzynne = false;
	for (const FOddzial& O : Oddz) { bCzynne |= !O.bPoza && !O.bKoniec && O.NaPolu() > 0; }
	if (bTrwa && !bCzynne)
	{
		Wybrane.Reset();
		bPauza = false;
	}
	bTrwa = bCzynne;
}

void ABitwa::Symuluj(float Dt)
{
	CzasBitwy += Dt;
	MarszTick(Dt);
	RuchOddzialow(Dt);
	PalenieTick(Dt);
	if (bTrwa)
	{
		if (RodW >= 0)
		{
			Oblezenie(Dt);
		}
		Zwarcie(Dt);
		Strzelanie(Dt);
		Nacisk(Dt);
		MoraleOddzialow(Dt);
	}
	SprawdzKoniec();
	DziennikZa -= Dt;
	if (DziennikZa <= 0.f && bTrwa)
	{
		DziennikZa = 1.f;
		Dziennik();
	}
}

void ABitwa::Rozkaz(ERozkaz R)
{
	for (int32 i : Wybrane)
	{
		if (!Oddz.IsValidIndex(i) || !Oddz[i].Stoi())
		{
			continue;
		}
		FOddzial& O = Oddz[i];
		Palenie.Remove(i);
		switch (R)
		{
		case ERozkaz::Atakuj:
			O.Rozkaz = ERozkaz::Atakuj;
			O.Cel = NajblizszyWrog(i);
			break;
		case ERozkaz::Stoj:
			O.Rozkaz = ERozkaz::Stoj;
			O.bDoFrontu = O.bBiegiem = false;
			O.CelRuchu = O.Srodek;
			break;
		case ERozkaz::Odwrot:
			// Odwrot: oddzial zrywa walke i cofa sie 25 m; to kosztuje troche ducha.
			O.Rozkaz = ERozkaz::Idz;
			O.CelRuchu = O.Srodek - O.Front * 2500.f;
			Obroc(O, -O.Front, 0.f); // w tyl zwrot: tylny szereg prowadzi
			O.bDoFrontu = false;
			O.Morale -= 8.f;
			for (FWojownik& W : O.Ludzie) { W.Wrog = -1; }
			break;
		default:
			break;
		}
	}
}

bool ABitwa::TerenPodKursorem(APlayerController* PC, FVector& Out) const
{
	FVector Pocz, Kier;
	if (!PC->DeprojectMousePositionToWorld(Pocz, Kier) || Kier.Z > -0.01)
	{
		return false;
	}
	double T0 = 0.0;
	for (int32 i = 0; i < 400; ++i)
	{
		const double T1 = T0 + 1500.0;
		const FVector P = Pocz + Kier * T1;
		if (P.Z <= FSwiat::Wysokosc(P.X, P.Y))
		{
			double A = T0, B = T1;
			for (int32 k = 0; k < 14; ++k)
			{
				const double M = (A + B) * 0.5;
				const FVector Q = Pocz + Kier * M;
				(Q.Z <= FSwiat::Wysokosc(Q.X, Q.Y) ? B : A) = M;
			}
			Out = Pocz + Kier * B;
			return true;
		}
		T0 = T1;
	}
	return false;
}

void ABitwa::Sterowanie(APlayerController* PC)
{
	CelKursora = -1;
	if (bWynik && (PC->WasInputKeyJustPressed(EKeys::Enter) || PC->WasInputKeyJustPressed(EKeys::Escape)))
	{
		Sprzatnij();
		return;
	}
	if (!bTrwa)
	{
		return;
	}
	if (bMyszZajeta)
	{
		// Gracz stawia budynek: mysz i klawisze (R = obrot) naleza do budowania.
		bKlikTrwa = bRamka = bPrawyTrwa = false;
		Podglad.Reset();
		PodgladStrz.Reset();
		return;
	}
	const bool bShift = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
	if (bWalkaTrwa && PC->WasInputKeyJustPressed(FUstawienia::Klawisz("Pauza")))
	{
		bPauza = !bPauza;
	}
	if (PC->WasInputKeyJustPressed(FUstawienia::Klawisz("Krew")))
	{
		// Krew wlacz / wylacz (zapisane w ustawieniach gracza).
		bKrew = !bKrew;
		GConfig->SetBool(TEXT("Bitwa"), TEXT("Krew"), bKrew, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
		if (!bKrew)
		{
			Krople.Reset();
			KropleC->ClearInstances();
			PlamyC->ClearInstances();
		}
		for (TActorIterator<AOsada> It(GetWorld()); It; ++It)
		{
			It->Ogloszenie(bKrew ? TEXT("Krew w bitwach: włączona (K)") : TEXT("Krew w bitwach: wyłączona (K)"));
			break;
		}
	}
	// 1-9: twoje oddzialy po kolei, Ctrl+A: wszystkie (24.11.2026: lepsze sterowanie).
	const FKey Cyfry[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	TArray<int32> Moje;
	for (int32 i = 0; i < Oddz.Num(); ++i) { if (Oddz[i].Strona == 0 && !Oddz[i].bPoza && !Oddz[i].bKoniec) { Moje.Add(i); } }
	if (PC->WasInputKeyJustPressed(EKeys::A) && (PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl)))
	{
		Wybrane = Moje;
	}
	for (int32 k = 0; k < UE_ARRAY_COUNT(Cyfry) && k < Moje.Num(); ++k)
	{
		if (PC->WasInputKeyJustPressed(Cyfry[k]))
		{
			if (!bShift) { Wybrane.Reset(); }
			Wybrane.AddUnique(Moje[k]);
		}
	}
	// A i S rusza kamera (WASD): atak i stoj sa w panelu i pod prawym klikiem.
	if (Wybrane.Num() > 0 && PC->WasInputKeyJustPressed(FUstawienia::Klawisz("Odwrot"))) { Rozkaz(ERozkaz::Odwrot); }
	if (PC->WasInputKeyJustPressed(FUstawienia::Klawisz("Szyk")))
	{
		for (int32 i : Wybrane) { if (Oddz.IsValidIndex(i)) { Oddz[i].bZwarty = !Oddz[i].bZwarty; } }
	}
	// Lewy klik: wybor oddzialu; przeciagniecie: ramka - wszystkie twoje oddzialy, ktorych woje sa w ramce (Shift - dodaj).
	float MX = 0.f, MY = 0.f;
	PC->GetMousePosition(MX, MY);
	KlikTeraz = FVector2D(MX, MY);
	FVector P;
	if (Wybrane.Num() > 0 && !bLinia && !bRamka && TerenPodKursorem(PC, P))
	{
		CelKursora = OddzialPod(FVector2D(P), 1, PC, KlikTeraz); // podswietlony wrog: prawy klik zaatakuje wlasnie jego
	}
	if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		KlikStart = KlikTeraz;
		bKlikTrwa = true;
		bRamka = false;
	}
	if (bKlikTrwa && FVector2D::Distance(KlikStart, KlikTeraz) > 8.f)
	{
		bRamka = true;
	}
	if (bKlikTrwa && PC->WasInputKeyJustReleased(EKeys::LeftMouseButton))
	{
		bKlikTrwa = false;
		if (!bShift) { Wybrane.Reset(); }
		if (bRamka)
		{
			const FVector2D Min(FMath::Min(KlikStart.X, KlikTeraz.X), FMath::Min(KlikStart.Y, KlikTeraz.Y));
			const FVector2D Max(FMath::Max(KlikStart.X, KlikTeraz.X), FMath::Max(KlikStart.Y, KlikTeraz.Y));
			for (int32 i = 0; i < Oddz.Num(); ++i)
			{
				if (Oddz[i].Strona != 0 || !Oddz[i].Stoi())
				{
					continue;
				}
				for (const FWojownik& W : Oddz[i].Ludzie)
				{
					FVector2D S;
					if (Zywy(W) && PC->ProjectWorldLocationToScreen(W.L->GetActorLocation() + FVector(0, 0, 120), S, false)
						&& S.X >= Min.X && S.X <= Max.X && S.Y >= Min.Y && S.Y <= Max.Y)
					{
						Wybrane.AddUnique(i);
						break;
					}
				}
			}
		}
		else if (TerenPodKursorem(PC, P))
		{
			const int32 i = OddzialPod(FVector2D(P), 0, PC, KlikTeraz);
			if (i >= 0) { Wybrane.AddUnique(i); }
		}
		bRamka = false;
	}
	// Prawy klik: we wroga = atakuj go, w teren = idz tam (kilka oddzialow obok siebie, frontem w strone marszu).
	// Przeciagniecie prawym: linia nowego szyku (dlugosc = szerokosc, front prostopadle), na ziemi podglad miejsc.
	// Podwojny prawy klik = biegiem.
	if (PC->WasInputKeyJustPressed(EKeys::RightMouseButton) && Wybrane.Num() > 0 && TerenPodKursorem(PC, P))
	{
		const double Teraz = GetWorld()->GetRealTimeSeconds();
		bPrawyTrwa = true;
		bLinia = false;
		bBiegPrawy = Teraz - OstatniPrawy < 0.35;
		OstatniPrawy = Teraz;
		PrawyStart = PrawyKoniec = FVector2D(P);
		PrawyStartEkran = KlikTeraz;
	}
	if (!bPrawyTrwa)
	{
		return;
	}
	FVector Q;
	if (TerenPodKursorem(PC, Q))
	{
		PrawyKoniec = FVector2D(Q);
	}
	if (FVector2D::Distance(PrawyStartEkran, KlikTeraz) > 12.f && FVector2D::Distance(PrawyStart, PrawyKoniec) > 300.f)
	{
		bLinia = true;
	}
	TArray<FUstaw> U;
	Podglad.Reset();
	PodgladStrz.Reset();
	if (bLinia)
	{
		RozstawNaLinii(PrawyStart, PrawyKoniec, false, U);
		for (const FUstaw& S : U)
		{
			FOddzial Tmp;
			Tmp.Srodek = S.Srodek;
			Tmp.Front = S.Front;
			Tmp.Kol = S.Kol;
			Tmp.bZwarty = Oddz[S.Oddzial].bZwarty;
			const int32 N = Oddz[S.Oddzial].NaPolu();
			for (int32 k = 0; k < N; ++k) { Podglad.Add(Miejsce(Tmp, k)); }
			for (int32 k = 1; k <= 4; ++k) { PodgladStrz.Add(S.Srodek + S.Front * (120.f + 70.f * k)); }
			const FVector2D Prawo(-S.Front.Y, S.Front.X);
			PodgladStrz.Add(S.Srodek + S.Front * 330.f - Prawo * 60.f); // grot strzalki
			PodgladStrz.Add(S.Srodek + S.Front * 330.f + Prawo * 60.f);
		}
	}
	if (!PC->WasInputKeyJustReleased(EKeys::RightMouseButton))
	{
		return;
	}
	bPrawyTrwa = false;
	Podglad.Reset();
	PodgladStrz.Reset();
	if (!bLinia)
	{
		// Klik: we wroga = atakuj go, w teren = idz tam w obecnych szerokosciach, frontem w strone marszu.
		const FVector2D Gdzie = PrawyStart;
		const int32 Wrog = OddzialPod(Gdzie, 1, PC, PrawyStartEkran);
		if (Wrog >= 0)
		{
			for (int32 i : Wybrane)
			{
				if (Oddz.IsValidIndex(i) && Oddz[i].Stoi())
				{
					Oddz[i].Rozkaz = ERozkaz::Atakuj;
					Oddz[i].Cel = Wrog;
					Oddz[i].bDoFrontu = false;
					Oddz[i].bBiegiem = bBiegPrawy;
					Palenie.Remove(i);
				}
			}
			return;
		}
		// Prawy klik w budynek osady rodu (gdy da sie do niego dojsc: brama rozbita albo poza murem) = podpalic go.
		if (AOsadaRodu* V = CelWyprawy())
		{
			const int32 Dz = V->DoSpalenia(Gdzie, 250.f);
			if (Dz >= 0 && (!BramaStoi() || !V->WMurach(Gdzie)))
			{
				for (int32 i : Wybrane)
				{
					if (Oddz.IsValidIndex(i) && Oddz[i].Stoi()) { Palenie.Add(i, Dz); }
				}
				if (AOsada* Wies = Osada()) { Wies->Ogloszenie(TEXT("Woje biegną podpalić budynek")); }
				return;
			}
		}
		FVector2D Sr = FVector2D::ZeroVector;
		for (int32 i : Wybrane) { Sr += Oddz[i].Srodek; }
		Sr /= Wybrane.Num();
		const FVector2D Kier = (Gdzie - Sr).GetSafeNormal().IsNearlyZero() ? FVector2D(1.f, 0.f) : (Gdzie - Sr).GetSafeNormal();
		const FVector2D Prawo(-Kier.Y, Kier.X);
		RozstawNaLinii(Gdzie - Prawo * 100.f, Gdzie + Prawo * 100.f, true, U);
	}
	// Kilka oddzialow idzie razem tempem najwolniejszego (najbardziej zmeczonego).
	float Najwolniej = 1.f;
	for (const FUstaw& S : U) { Najwolniej = FMath::Min(Najwolniej, 1.f - Oddz[S.Oddzial].Zmeczenie * 0.004f); }
	for (const FUstaw& S : U)
	{
		FOddzial& O = Oddz[S.Oddzial];
		Palenie.Remove(S.Oddzial);
		WydajMarsz(O, S.Srodek, S.Front, bLinia ? S.Kol : 0);
		O.bBiegiem = bBiegPrawy;
		O.TempoGrupy = U.Num() > 1 ? Najwolniej / FMath::Max(0.1f, 1.f - O.Zmeczenie * 0.004f) : 1.f;
	}
}

void ABitwa::RozstawNaLinii(FVector2D A, FVector2D B, bool bZachowaj, TArray<FUstaw>& Out) const
{
	Out.Reset();
	const float L = FVector2D::Distance(A, B);
	const FVector2D D = L > 1.f ? (B - A) / L : FVector2D(0.f, 1.f);
	const FVector2D Front(D.Y, -D.X); // ciagniecie w prawo ekranu = front od kamery (Prawo = (-Front.Y, Front.X) = D)
	TArray<int32> Kto;
	for (int32 i : Wybrane)
	{
		if (Oddz.IsValidIndex(i) && Oddz[i].Strona == 0 && Oddz[i].Stoi())
		{
			Kto.Add(i);
		}
	}
	if (Kto.Num() == 0)
	{
		return;
	}
	// Po kolei wzdluz linii, jak stoja teraz: oddzialy sie nie krzyzuja.
	Kto.Sort([&](int32 X, int32 Y) { return FVector2D::DotProduct(Oddz[X].Srodek - A, D) < FVector2D::DotProduct(Oddz[Y].Srodek - A, D); });
	constexpr float Przerwa = 250.f; // miedzy oddzialami
	TArray<float> Waga;
	float Suma = 0.f;
	for (int32 i : Kto)
	{
		const FOddzial& O = Oddz[i];
		const float Od = O.bZwarty ? OdstepZwarty : OdstepLuzny;
		Waga.Add(bZachowaj ? FMath::Max(1, O.Kol) * Od : float(O.NaPolu()));
		Suma += Waga.Last();
	}
	// Zwykly klik: linia tak dluga jak oddzialy obok siebie, wysrodkowana na A-B.
	const FVector2D Sr = (A + B) * 0.5f;
	const float Szer = bZachowaj ? Suma : FMath::Max(L - Przerwa * (Kto.Num() - 1), 100.f * Kto.Num());
	const float Calosc = Szer + Przerwa * (Kto.Num() - 1);
	const FVector2D Pocz = bZachowaj ? Sr - D * (Calosc * 0.5f) : A;
	float S = 0.f;
	for (int32 n = 0; n < Kto.Num(); ++n)
	{
		const FOddzial& O = Oddz[Kto[n]];
		const float Od = O.bZwarty ? OdstepZwarty : OdstepLuzny;
		const float Czesc = Szer * Waga[n] / FMath::Max(1.f, Suma);
		const int32 N = FMath::Max(1, O.NaPolu());
		// Najwyzej 12 szeregow, najmniej 1.
		const int32 Kol = bZachowaj ? O.Kol : FMath::Clamp(FMath::RoundToInt(Czesc / Od), FMath::DivideAndRoundUp(N, 12), N);
		Out.Add({ Kto[n], Pocz + D * (S + Czesc * 0.5f), Front, Kol });
		S += Czesc + Przerwa;
	}
}

void ABitwa::WydajMarsz(FOddzial& O, FVector2D Cel, FVector2D Front, int32 Kol)
{
	O.Rozkaz = ERozkaz::Idz;
	O.Cel = -1;
	O.CelRuchu = Cel;
	O.FrontCelu = Front.GetSafeNormal();
	O.bDoFrontu = true;
	if (Kol > 0 && Kol != O.Kol)
	{
		O.KolGracza = Kol;
		O.bUloz = true;
	}
	// Srodek oddzialu omija budynki z zapasem na pol szerokosci szyku.
	const float PolSzer = (FMath::Max(1, O.Kol) - 1) * 0.5f * (O.bZwarty ? OdstepZwarty : OdstepLuzny);
	O.Trasa.Reset();
	O.Trasa.Add(Cel);
	ALudzik::OminBudynkiTrasa(GetWorld(), O.Srodek, O.Trasa, FMath::Clamp(PolSzer + 100.f, 100.f, 600.f));
	for (FWojownik& W : O.Ludzie) { W.Wrog = -1; }
}

void ABitwa::Obroc(FOddzial& O, FVector2D Chce, float Dt)
{
	Chce = Chce.GetSafeNormal();
	if (Chce.IsNearlyZero())
	{
		return;
	}
	const float Od = O.bZwarty ? OdstepZwarty : OdstepLuzny;
	const float Cos = FVector2D::DotProduct(O.Front, Chce);
	if (Cos < -0.5f)
	{
		O.Srodek -= O.Front * ((O.Szeregi - 1) * Od);
		O.Front = -O.Front;
		O.bUloz = true;
		return;
	}
	const float Kat = FMath::Atan2(FVector2D::CrossProduct(O.Front, Chce), Cos);
	const float PolSzer = FMath::Max(200.f, (O.Kol - 1) * 0.5f * Od);
	const float Max = Marsz * 1.1f / PolSzer * Dt;
	O.Front = O.Front.GetRotated(FMath::RadiansToDegrees(FMath::Clamp(Kat, -Max, Max))).GetSafeNormal();
}

void ABitwa::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Oddz.Num() == 0)
	{
		return;
	}
	// Szum bitwy (wrzawa tlumu): rosnie z liczba walczacych oddzialow i z oddaleniem kamery, gdzie pojedyncze ciosy gasna.
	if (UDzwieki* D = UDzwieki::Z(this))
	{
		int32 Walcza = 0;
		for (const FOddzial& O : Oddz) { Walcza += O.bWalczy && O.Stoi() ? 1 : 0; }
		UDzwieki::Petla(this, "Petla_Bitwa", EDzwiek::Glosy, FMath::Min(1.f, Walcza / 4.f) * (0.35f + 0.65f * (1.f - D->Blisko())));
	}
	// Mapa papierowa: bitwa ukryta (jak osada).
	const bool bRegion = WRegionie();
	if (bRegion != bWidac)
	{
		bWidac = bRegion;
		for (FOddzial& O : Oddz)
		{
			for (FWojownik& W : O.Ludzie)
			{
				if (W.L.IsValid()) { W.L->SetActorHiddenInGame(!bRegion); }
			}
		}
		KropleC->SetVisibility(bRegion);
		PlamyC->SetVisibility(bRegion);
		LeceC->SetVisibility(bRegion);
		WbiteC->SetVisibility(bRegion);
	}
	AKameraMapy::bBezLewegoPrzeciagania = bRegion && bTrwa && !bMyszZajeta; // lewy przycisk = ramka, mapa: srodkowy
	AKameraMapy::bBitwaBlisko = bRegion; // w bitwie (i chwile po niej) kamera moze zjechac nizej
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (bRegion)
		{
			Sterowanie(PC);
		}
	}
	if (Oddz.Num() == 0)
	{
		return; // Sprzatnij (Enter po bitwie)
	}
	// Zwolnienie czasu przy zderzeniu oddzialow (liczone w czasie rzeczywistym).
	if (Zwolnienie > 0.f)
	{
		Zwolnienie -= FApp::GetDeltaTime();
		UGameplayStatics::SetGlobalTimeDilation(this, Zwolnienie > 0.f ? 0.3f : 1.f);
	}
	// Pauza: ludziki zamrozone, symulacja stoi; rozkazy mozna wydawac.
	for (FOddzial& O : Oddz)
	{
		for (FWojownik& W : O.Ludzie)
		{
			if (W.L.IsValid()) { W.L->CustomTimeDilation = bPauza ? 0.f : 1.f; }
		}
	}
	if (bPauza)
	{
		return;
	}
	// Staly krok symulacji 0,1 s (zwarcie i morale nie zaleza od liczby klatek).
	Krok += DeltaSeconds;
	while (Krok >= 0.1f)
	{
		Krok -= 0.1f;
		Symuluj(0.1f);
	}
	// Skutki ciosow co klatke (dokladnie w chwili uderzenia), filmowe pary co 3 s.
	CiosyTick(DeltaSeconds);
	FilmZa -= DeltaSeconds;
	if (FilmZa <= 0.f && bWalkaTrwa)
	{
		FilmZa = 3.f;
		Filmowe();
	}
	PoBitwie();
	if (Oddz.Num() == 0)
	{
		return;
	}
	KrewTick(DeltaSeconds);
	IskryTick(DeltaSeconds);
	// Krew tryska z kikuta (co 0,08 s, slabnie).
	for (int32 i = Tryski.Num() - 1; i >= 0; --i)
	{
		FTrysk& T = Tryski[i];
		T.Zostalo -= DeltaSeconds;
		T.Co -= DeltaSeconds;
		FVector Gdzie;
		if (T.Zostalo <= 0.f || !T.L.IsValid() || !T.L->Kikut(Gdzie))
		{
			Tryski.RemoveAtSwap(i);
			continue;
		}
		if (T.Co <= 0.f)
		{
			T.Co = 0.08f;
			Krew(Gdzie, FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)), 2, FMath::Min(1.f, 0.4f + T.Zostalo * 0.3f));
		}
	}
	PociskiTick(DeltaSeconds);
	Rozepchnij(DeltaSeconds);
}

const TCHAR* ABitwa::OpisStanu(const FOddzial& O)
{
	return O.bPoza || O.NaPolu() == 0 ? TEXT("poza polem") : O.bZlamany ? TEXT("UCIEKA") : O.bWalczy ? TEXT("w zwarciu")
		: O.Rozkaz == ERozkaz::Atakuj ? TEXT("atakuje") : O.Rozkaz == ERozkaz::Idz ? TEXT("idzie") : TEXT("stoi");
}

void ABitwa::Wybierz(int32 i, bool bDodaj)
{
	if (!bDodaj)
	{
		Wybrane.Reset();
	}
	if (Oddz.IsValidIndex(i) && Oddz[i].Strona == 0)
	{
		Wybrane.AddUnique(i);
	}
}

void ABitwa::PrzelaczSzyk()
{
	const bool bZwarty = !SzykWybranychZwarty();
	for (int32 i : Wybrane) { if (Oddz.IsValidIndex(i)) { Oddz[i].bZwarty = bZwarty; } }
}

bool ABitwa::SzykWybranychZwarty() const
{
	for (int32 i : Wybrane) { if (Oddz.IsValidIndex(i)) { return Oddz[i].bZwarty; } }
	return true;
}

void ABitwa::PokazKamere(FVector2D Gdzie) const
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AKameraMapy* Kam = Cast<AKameraMapy>(PC->GetPawn()))
		{
			Kam->UstawOgnisko(Gdzie);
		}
	}
}

FString ABitwa::TekstWyniku() const
{
	int32 Pol[2] = { 0, 0 };
	for (const FOddzial& O : Oddz) { Pol[O.Strona] += O.Polegli; }
	const TCHAR* Co = Zwyciezca == 0 ? TEXT("Bitwa wygrana, bandyci rozbici") : Zwyciezca == 1 ? TEXT("Bitwa przegrana, woje uciekli") : TEXT("Obie strony uciekły z pola");
	return FString::Printf(TEXT("%s (polegli: twoi %d, wrogowie %d)"), Co, Pol[0], Pol[1]);
}

namespace
{
	/** Po zdobyciu osady rodu: trzy wybory (ARody::PoWyprawie). */
	TSharedRef<SWidget> ZrobWyborPodboju(TWeakObjectPtr<ABitwa> Ja, const FSlateBrush* Tlo)
	{
		static const FButtonStyle Styl = StylUI::StylPrzycisku();
		auto Napis = [](const TCHAR* T, int32 Rozmiar, FLinearColor K, bool bGruba)
		{
			return SNew(STextBlock).Text(FText::FromString(T)).Font(FCoreStyle::GetDefaultFontStyle(bGruba ? "Bold" : "Regular", Rozmiar))
				.ColorAndOpacity(K).AutoWrapText(true);
		};
		auto Guzik = [Ja](const TCHAR* Co, const TCHAR* Opis, int32 Wybor)
		{
			return SNew(SButton)
				.ButtonStyle(&Styl)
				.ContentPadding(FMargin(14.f, 6.f))
				.ToolTipText(FText::FromString(Opis))
				.OnClicked_Lambda([Ja, Wybor]() { if (Ja.IsValid()) { Ja->PoZdobyciu(Wybor); } return FReply::Handled(); })
				[
					SNew(STextBlock).Text(FText::FromString(Co)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 12)).ColorAndOpacity(StylUI::Napis)
				];
		};
		return SNew(SBorder)
			.Visibility_Lambda([Ja]() { return Ja.IsValid() && Ja->CzekaWybor() ? EVisibility::Visible : EVisibility::Collapsed; })
			.BorderImage(Tlo)
			.Padding(FMargin(20.f, 14.f))
			[
				SNew(SBox).WidthOverride(500.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[Napis(TEXT("Osada zdobyta!"), 16, StylUI::Zloto, true)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 12.f)
					[
						Napis(TEXT("Obrońcy rozbici, brama leży. Co zrobić z osadą? Najedź na przycisk, żeby zobaczyć skutki."), 11, StylUI::Napis, false)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
						[
							Guzik(TEXT("Złupić"), TEXT("Zabieracie połowę ich zapasów i 60% groszy. Ród zostaje wrogi i będzie szukał zemsty."), 0)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
						[
							Guzik(TEXT("Hołd i danina"), TEXT("Ród zostaje waszym lennikiem: 30% groszy teraz i danina co rok. Osada stoi dalej."), 1)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							Guzik(TEXT("Spalić"), TEXT("Osada płonie: ród traci trzecią część rodzin i większość zapasów, na długo zostaje słaby i wrogi."), 2)
						]
					]
				]
			];
	}
}

void ABitwa::ZrobInterfejs()
{
	if (Nakladka.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	static FSlateRoundedBoxBrush Tlo(StylUI::Tlo, 6.f, StylUI::Ramka, 1.f);
	const TWeakObjectPtr<ABitwa> Ja(this);
	Nakladka = SNew(SOverlay)
		.Visibility_Lambda([Ja]() { return Ja.IsValid() && Ja->Trwa() && Ja->WRegionie() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
		+ SOverlay::Slot()
		[
			SNew(SZnakiBitwy).Bitwa(Ja).Visibility(EVisibility::HitTestInvisible)
		]
		// Panel bitwy na dole (Wojna/PanelBitwy): dowodztwo, karty oddzialow z rozkazami, mapka.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(16.f, 0.f, 16.f, 150.f)) // nad paskiem budowy (budowanie trwa dalej)
		[
			ZrobPanelBitwy(Ja)
		]
		// Podboj: osada rodu zdobyta, gracz wybiera jej los.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			ZrobWyborPodboju(Ja, &Tlo)
		];
	GEngine->GameViewport->AddViewportWidgetContent(Nakladka.ToSharedRef(), 6);
}

void ABitwa::UsunInterfejs()
{
	if (Nakladka.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Nakladka.ToSharedRef());
	}
	Nakladka.Reset();
}

void ABitwa::Sprzatnij()
{
	if (RodW >= 0 || MarszW.Etap != 0)
	{
		KoniecWyprawy(true);
	}
	for (FOddzial& O : Oddz)
	{
		for (FWojownik& W : O.Ludzie)
		{
			Odeslij(W);
		}
	}
	Oddz.Reset();
	Wybrane.Reset();
	for (FMilicja& M : Mil) { M.Oddzial = -1; }
	bWalkaTrwa = bWrogByl = bLupUciekl = false;
	CzasBitwy = CzasKonca = 0.f;
	Lup = FSurowce();
	LupGroszy = 0;
	++WersjaN;
	Krople.Reset();
	KropleC->ClearInstances();
	PlamyC->ClearInstances();
	NastPlama = 0;
	Ciosy.Reset();
	IskryL.Reset();
	IskryC->ClearInstances();
	Tryski.Reset();
	Pociski.Reset();
	LeceC->ClearInstances();
	WbiteC->ClearInstances();
	NastWbita = 0;
	bTrwa = bWynik = bPauza = false;
	Zwolnienie = 0.f;
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	AKameraMapy::bBezLewegoPrzeciagania = false;
	AKameraMapy::bBitwaBlisko = false;
	bKlikTrwa = bRamka = false;
	UsunInterfejs();
}

void ABitwa::Rozepchnij(float Dt)
{
	// Pary wojow blizej niz osobista przestrzen rozsuwaja sie (polegli nie). Swoi: 0,95 m, wrog: 1,2 m.
	struct FP { ALudzik* L; FVector2D P; FVector2D Ruch; int32 Strona; };
	TArray<FP> Wszyscy;
	for (const FOddzial& O : Oddz)
	{
		for (const FWojownik& W : O.Ludzie)
		{
			if (Zywy(W)) { Wszyscy.Add({ W.L.Get(), Poz(W), FVector2D::ZeroVector, O.Strona }); }
		}
	}
	for (int32 a = 0; a < Wszyscy.Num(); ++a)
	{
		for (int32 b = a + 1; b < Wszyscy.Num(); ++b)
		{
			const float Min = Wszyscy[a].Strona == Wszyscy[b].Strona ? Przestrzen : PrzestrzenWroga;
			FVector2D D = Wszyscy[a].P - Wszyscy[b].P;
			const float L = D.Size();
			if (L >= Min)
			{
				continue;
			}
			D = L > 1.f ? D / L : FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal();
			const FVector2D Pchn = D * (Min - L) * 0.5f;
			Wszyscy[a].Ruch += Pchn;
			Wszyscy[b].Ruch -= Pchn;
		}
	}
	const float K = FMath::Min(1.f, Dt * 10.f);
	for (const FP& W : Wszyscy)
	{
		if (!W.Ruch.IsNearlyZero(0.5f))
		{
			const FVector2D N = W.P + W.Ruch * K;
			W.L->SetActorLocation(FVector(N, FSwiat::Wysokosc(N.X, N.Y)));
		}
	}
}

void ABitwa::PoBitwie()
{
	// Pobojowisko: ciala leza dwie minuty, potem znikaja, gdy kamera na nie nie patrzy. Po walce woje bitwy probnej i
	// resztki wroga schodza (8 s, poza kadrem). Gdy na polu nie ma juz nikogo (albo 90 s po walce), pole jest sprzatane.
	const float Po = CzasBitwy - CzasKonca;
	int32 Zostalo = 0;
	for (FOddzial& O : Oddz)
	{
		for (FWojownik& W : O.Ludzie)
		{
			if (!W.L.IsValid() || (W.bPoza && W.bZyje))
			{
				continue;
			}
			const bool bCialo = !W.bZyje && CzasBitwy - W.Smierc > 120.f;
			if ((bCialo || (O.bKoniec && Po > 8.f)) && !W.L->WasRecentlyRendered(1.f))
			{
				W.bPoza = true;
				Odeslij(W);
				continue;
			}
			++Zostalo;
		}
	}
	if (!bTrwa && (Zostalo == 0 || Po > 90.f))
	{
		Sprzatnij();
	}
}

// ---------------------------------------------------------------------------------------------------------------
// Podboj osad rodow (23.11.2026). Wyprawa: powolane ruszenie od razu pod brame osady rodu (marsz bez pokazywania drogi),
// za murem obroncy. Poki brama stoi, obroncy czekaja, a mur zatrzymuje oba wojska (Srodek oddzialu nie przechodzi przez
// obrys muru); atak na obroncow prowadzi pod brame, gdzie woje rabia ja toporami. Po zwyciestwie panel wyboru.
// shortcut: pojedynczy woje w szyku moga przenikac przez rog muru (mur blokuje srodek oddzialu, nie kazdego woja).

ARody* ABitwa::Rody()
{
	if (!RodyW.IsValid())
	{
		for (TActorIterator<ARody> It(GetWorld()); It; ++It)
		{
			RodyW = *It;
			break;
		}
	}
	return RodyW.Get();
}

AOsadaRodu* ABitwa::CelWyprawy()
{
	ARody* R = Rody();
	return R ? R->Widok(RodW) : nullptr;
}

bool ABitwa::BramaStoi()
{
	AOsadaRodu* V = CelWyprawy();
	return V && V->Bramy().IsValidIndex(BramaW) && !V->BramaLezy(BramaW);
}

void ABitwa::Przenies(FOddzial& O, FVector2D Gdzie, FVector2D Front)
{
	O.Srodek = O.CelRuchu = Gdzie;
	O.Front = O.FrontCelu = Front.GetSafeNormal();
	O.Rozkaz = ERozkaz::Stoj;
	O.Cel = -1;
	O.Trasa.Reset();
	O.bDoFrontu = O.bBiegiem = O.bWalczy = O.bZlamany = O.bOkrzyk = false;
	O.Morale = FMath::Max(O.Morale, 40.f);
	for (FWojownik& W : O.Ludzie)
	{
		if (!Zywy(W))
		{
			continue;
		}
		W.Wrog = -1;
		const FVector2D G = Miejsce(O, W.Slot);
		W.L->SetActorLocation(FVector(G, FSwiat::Wysokosc(G.X, G.Y)));
		W.L->WalkaStoj(G + O.Front * 1000.f);
	}
}

bool ABitwa::Wyprawa(int32 RI, FString& Powod)
{
	ARody* Ro = Rody();
	AOsadaRodu* V = Ro ? Ro->Widok(RI) : nullptr;
	if (!V || !Ro->Lista().IsValidIndex(RI))
	{
		Powod = TEXT("Nie wiadomo, dokąd iść.");
		return false;
	}
	if (RodW >= 0 || MarszW.Etap != 0)
	{
		Powod = TEXT("Jedna wyprawa już trwa.");
		return false;
	}
	for (const FOddzial& X : Oddz)
	{
		if (X.Strona == 1 && X.Stoi())
		{
			Powod = TEXT("Najpierw rozbijcie wrogów, którzy są na polu.");
			return false;
		}
	}
	TArray<int32> Moi;
	for (int32 i = 0; i < Oddz.Num(); ++i)
	{
		if (Oddz[i].Strona == 0 && Oddz[i].Milicja >= 0 && Oddz[i].Stoi()) { Moi.Add(i); }
	}
	if (Moi.Num() == 0)
	{
		Powod = TEXT("Najpierw powołajcie pospolite ruszenie (Wojsko, J): na wyprawę idą powołani.");
		return false;
	}
	const FRod& R = Ro->Lista()[RI];
	AOsada* Wies = Osada();
	Dom = Wies ? Wies->Oboz() : Oddz[Moi[0]].Srodek;
	// Brama najblizej osady gracza; bez murow wejscie od strony gracza, 40 m od placu.
	BramaW = -1;
	const TArray<AOsadaRodu::FBrama>& Br = V->Bramy();
	for (int32 k = 0; k < Br.Num(); ++k)
	{
		if (BramaW < 0 || Dl(Br[k].P, Dom) < Dl(Br[BramaW].P, Dom)) { BramaW = k; }
	}
	FVector2D Wejscie = R.Gdzie, Na = (Dom - R.Gdzie).GetSafeNormal();
	if (BramaW >= 0)
	{
		Wejscie = Br[BramaW].P;
		Na = Br[BramaW].Na;
	}
	else
	{
		Wejscie = R.Gdzie + Na * 4000.f;
	}
	RodW = RI;
	bZdobyta = false;
	// Marsz: woje wychodza z obozu w strone osady rodu i znikaja za kadrem, potem kolumna idzie po mapie, na koniec
	// ostatnie 30 m pod brame (Dotarli).
	MarszW = FMarsz();
	MarszW.Etap = 1;
	MarszW.Oddzialy = Moi;
	MarszW.Wejscie = Wejscie;
	MarszW.Na = Na;
	MarszW.Caly = CzasMarszu(Dl(Dom, Wejscie));
	const FVector2D Kier = (Wejscie - Dom).GetSafeNormal();
	for (int32 n = 0; n < Moi.Num(); ++n)
	{
		FOddzial& O = Oddz[Moi[n]];
		O.Rozkaz = ERozkaz::Idz;
		O.CelRuchu = O.Srodek + Kier * 6000.f;
		O.Trasa.Reset();
		O.bDoFrontu = O.bBiegiem = false;
	}
	Wybrane.Reset();
	Powod = FString::Printf(TEXT("Wyprawa wyrusza na %s, dojdzie za ok. %.0f s"), *R.Nazwa, MarszW.Caly + 8.f);
	if (Wies) { Wies->Ogloszenie(Powod); }
	UE_LOG(LogTemp, Log, TEXT("Bitwa: wyprawa na %s wyrusza (%d oddzialow, marsz %.0f s)"), *R.Nazwa, Moi.Num(), MarszW.Caly);
	return true;
}

void ABitwa::Dotarli()
{
	ARody* Ro = Rody();
	AOsadaRodu* V = CelWyprawy();
	if (!Ro || !V || !Ro->Lista().IsValidIndex(RodW))
	{
		MarszW.Etap = 0;
		UkryjMarsz(false);
		KoniecWyprawy();
		return;
	}
	const FRod& R = Ro->Lista()[RodW];
	AOsada* Wies = Osada();
	const FVector2D Wejscie = MarszW.Wejscie, Na = MarszW.Na;
	const FVector2D Prawo(-Na.Y, Na.X);
	const TArray<int32> Moi = MarszW.Oddzialy.FilterByPredicate([this](int32 i) { return Oddz.IsValidIndex(i) && Oddz[i].Stoi(); });
	MarszW.Etap = 0;
	V->UstawOblezenie(true);
	// Pole bitwy przy osadzie rodu (krew i strzaly z poprzedniej bitwy znikaja).
	Krople.Reset();
	KropleC->ClearInstances();
	PlamyC->ClearInstances();
	NastPlama = 0;
	WbiteC->ClearInstances();
	NastWbita = 0;
	Przygotuj(Wejscie);
	for (int32 n = 0; n < Moi.Num(); ++n)
	{
		// Wychodza z lasu 90 m przed brama i marszem staja 60 m przed nia.
		FOddzial& O = Oddz[Moi[n]];
		const FVector2D Miejsce = Wejscie + Na * 6000.f + Prawo * ((n - (Moi.Num() - 1) * 0.5f) * 1500.f);
		Przenies(O, Miejsce + Na * 3000.f, -Na);
		O.Rozkaz = ERozkaz::Idz;
		O.CelRuchu = Miejsce;
		O.FrontCelu = -Na;
	}
	UkryjMarsz(false);
	// Obroncy: woje rodu w jego barwie (przy 10+ wojach czesc z lukami przy murze), reszta to chlopi z rodzin.
	const int32 N = Obroncow(R.Woje, R.Rodzin);
	const int32 Wojow = FMath::Min(R.Woje, N);
	const int32 Lucznikow = Wojow >= 10 ? FMath::Min(12, Wojow / 4) : 0;
	struct FGrupa { ERodzajWojska Rd; int32 Ilu; const TCHAR* Kto; FLinearColor T; };
	TArray<FGrupa> Grupy;
	const ERodzajWojska Bron = R.Cecha == ECechaRodu::Wojowniczy ? ERodzajWojska::Tarczownicy : ERodzajWojska::Wlocznicy;
	for (int32 Zost = Wojow - Lucznikow; Zost > 0; Zost -= 14) { Grupy.Add({ Bron, FMath::Min(14, Zost), TEXT("Woje"), R.Barwa * 0.7f }); }
	for (int32 Zost = N - Wojow; Zost > 0; Zost -= 14) { Grupy.Add({ ERodzajWojska::Chlopi, FMath::Min(14, Zost), TEXT("Chłopi"), FLinearColor(0.36f, 0.29f, 0.2f) }); }
	auto WSrodku = [&](FVector2D P)
	{
		for (int32 k = 0; k < 6 && BramaW >= 0 && !V->WMurach(P); ++k) { P = (P + R.Gdzie) * 0.5f; }
		return P;
	};
	if (Lucznikow > 0)
	{
		for (int32 s = 0; s < 2; ++s)
		{
			const int32 Ilu = Lucznikow / 2 + (s == 0 ? Lucznikow % 2 : 0);
			const FVector2D P = WSrodku(Wejscie - Na * 900.f + Prawo * (s == 0 ? -1700.f : 1700.f));
			const int32 i = DodajOddzial(ERodzajWojska::Lucznicy, FString::Printf(TEXT("Łucznicy (%s)"), *R.Nazwa), 1, Ilu, P, Na, 1, R.Barwa * 0.7f);
			Oddz[i].bObronca = true;
			Oddz[i].Barwa = R.Barwa; // choragiew w barwie rodu
		}
	}
	for (int32 g = 0; g < Grupy.Num(); ++g)
	{
		const FVector2D P = WSrodku(Wejscie - Na * (1800.f + (g / 3) * 1500.f) + Prawo * ((g % 3 - 1) * 1600.f));
		const int32 i = DodajOddzial(Grupy[g].Rd, FString::Printf(TEXT("%s (%s)"), Grupy[g].Kto, *R.Nazwa), 1, Grupy[g].Ilu, P, Na, Grupy[g].Rd == ERodzajWojska::Chlopi ? 0 : 1, Grupy[g].T);
		Oddz[i].bObronca = true;
		Oddz[i].Barwa = R.Barwa; // choragiew w barwie rodu
	}
	Rozpocznij();
	Wybrane = Moi;
	PokazKamere(Wejscie + Na * 3500.f);
	const TCHAR* Mur = R.Mury >= 2 ? TEXT("murem kamiennym") : (R.Mury == 1 ? TEXT("palisadą") : TEXT("płotami"));
	const FString Powod = FString::Printf(TEXT("Wyprawa doszła pod %s: %d obrońców za %s. Rozkaz ataku na obrońców prowadzi pod bramę, woje ją rozrąbią."), *R.Nazwa, N, Mur);
	if (Wies) { Wies->Ogloszenie(Powod); }
	UE_LOG(LogTemp, Log, TEXT("Bitwa: wyprawa doszla pod %s (%d oddzialow gracza, %d obroncow, brama %d)"), *R.Nazwa, Moi.Num(), N, BramaW);
}

void ABitwa::Oblezenie(float Dt)
{
	AOsadaRodu* V = CelWyprawy();
	if (!V || !BramaStoi())
	{
		return;
	}
	const AOsadaRodu::FBrama B = V->Bramy()[BramaW];
	const FVector Gdzie(B.P, FSwiat::Wysokosc(B.P.X, B.P.Y) + 160.f);
	for (FOddzial& O : Oddz)
	{
		if (O.Strona != 0 || !O.Stoi() || O.Strzela())
		{
			continue;
		}
		const float Sila = O.bTopor ? 4.f : 2.5f; // topor rabie lepiej niz wlocznia
		for (FWojownik& W : O.Ludzie)
		{
			if (!Zywy(W) || W.Slot >= O.Kol || W.Wrog >= 0 || W.L->Uderza() || Dl(Poz(W), B.P) > 480.f)
			{
				continue; // rabie tylko pierwszy szereg na szerokosc wrot
			}
			W.Odnowa -= Dt;
			if (W.Odnowa > 0.f)
			{
				continue;
			}
			W.Odnowa = FMath::FRandRange(0.9f, 1.4f) * O.Tempo;
			W.L->WalkaCios(B.P);
			if (FMath::FRand() < 0.35f) { UDzwieki::Graj(this, "Brama_Uderzenie", Gdzie, EDzwiek::Walka, 0.8f); }
			if (FMath::FRand() < 0.2f)
			{
				if (AEfekty* E = AEfekty::W(GetWorld())) { E->Kurz(Gdzie + FVector(B.Na * 60.f, 0.f), 120.f, 2); }
			}
			if (V->UderzBrame(BramaW, Sila))
			{
				// Brama padla: obroncy ruszaja od razu, twoi wznosza okrzyk.
				for (FOddzial& X : Oddz)
				{
					if (X.bObronca) { X.Start = FMath::Min(X.Start, CzasBitwy - 6.f); }
					if (X.Strona == 0 && X.Stoi()) { Okrzyk(X, 0.8f); }
				}
				if (AOsada* Wies = Osada()) { Wies->Ogloszenie(TEXT("Brama rozbita! Obrońcy wychodzą do walki.")); }
				UE_LOG(LogTemp, Log, TEXT("Bitwa: brama rozbita po %.0f s"), CzasBitwy);
				return;
			}
		}
	}
}

void ABitwa::KoniecWyprawy(bool bOdRazu)
{
	if (AOsadaRodu* V = CelWyprawy())
	{
		V->UstawOblezenie(false);
	}
	// Ocaleni wracaja: znikaja spod osady rodu, kolumna idzie po mapie i po chwili wchodzi do obozu (MarszTick).
	// bOdRazu (sprzatanie pola): od razu pod obozem.
	UkryjMarsz(false);
	const FMarsz Byl = MarszW;
	MarszW = FMarsz();
	MarszW.Wejscie = Byl.Wejscie;
	MarszW.Na = Byl.Na;
	FVector2D Front;
	for (int32 m = 0; m < Mil.Num(); ++m)
	{
		if (Mil[m].Powolana() && Oddz.IsValidIndex(Mil[m].Oddzial) && !Oddz[Mil[m].Oddzial].bPoza)
		{
			FOddzial& O = Oddz[Mil[m].Oddzial];
			if (bOdRazu || Byl.Wejscie.IsZero())
			{
				const FVector2D Gdzie = MiejsceZbiorki(m, Front);
				Przenies(O, Gdzie, Front);
				continue;
			}
			MarszW.Oddzialy.Add(Mil[m].Oddzial);
			O.Rozkaz = ERozkaz::Stoj;
			O.CelRuchu = O.Srodek;
			O.Cel = -1;
			for (FWojownik& W : O.Ludzie) { W.Wrog = -1; }
		}
	}
	if (MarszW.Oddzialy.Num() > 0)
	{
		MarszW.Etap = 3;
		MarszW.Caly = CzasMarszu(Dl(Dom, MarszW.Wejscie));
		UkryjMarsz(true);
		if (AOsada* Wies = Osada()) { Wies->Ogloszenie(FString::Printf(TEXT("Wyprawa wraca do domu, będzie za ok. %.0f s"), MarszW.Caly)); }
	}
	RodW = -1;
	BramaW = -1;
	bZdobyta = false;
}

void ABitwa::UkryjMarsz(bool bUkryj)
{
	for (int32 i : MarszW.Oddzialy)
	{
		if (!Oddz.IsValidIndex(i))
		{
			continue;
		}
		for (FWojownik& W : Oddz[i].Ludzie)
		{
			if (Zywy(W)) { W.L->SetActorHiddenInGame(bUkryj || !bWidac); }
		}
	}
}

bool ABitwa::MarszWyprawy(FVector2D& Gdzie, float& Postep, bool& bPowrot) const
{
	if (MarszW.Etap == 0)
	{
		return false;
	}
	const FVector2D Pod = MarszW.Wejscie + MarszW.Na * 6000.f;
	Postep = MarszW.Etap == 1 ? 0.f : FMath::Clamp(MarszW.Czas / FMath::Max(1.f, MarszW.Caly), 0.f, 1.f);
	bPowrot = MarszW.Etap == 3;
	Gdzie = bPowrot ? FMath::Lerp(Pod, Dom, Postep) : FMath::Lerp(Dom, Pod, Postep);
	return true;
}

void ABitwa::MarszTick(float Dt)
{
	if (MarszW.Etap == 0)
	{
		return;
	}
	MarszW.Czas += Dt;
	if (MarszW.Etap == 1)
	{
		// Wymarsz: kto wyszedl za kadr (25 m od obozu), znika; po 20 s albo gdy wszyscy znikna - kolumna w drodze.
		bool bWszyscy = true;
		for (int32 i : MarszW.Oddzialy)
		{
			if (!Oddz.IsValidIndex(i)) { continue; }
			for (FWojownik& W : Oddz[i].Ludzie)
			{
				if (!Zywy(W) || W.L->IsHidden()) { continue; }
				if (Dl(Poz(W), Dom) > 2500.f && !W.L->WasRecentlyRendered(0.3f)) { W.L->SetActorHiddenInGame(true); }
				else { bWszyscy = false; }
			}
		}
		if (bWszyscy || MarszW.Czas > 20.f)
		{
			MarszW.Etap = 2;
			MarszW.Czas = 0.f;
			for (int32 i : MarszW.Oddzialy)
			{
				if (Oddz.IsValidIndex(i)) { Oddz[i].Rozkaz = ERozkaz::Stoj; Oddz[i].CelRuchu = Oddz[i].Srodek; }
			}
		}
		return;
	}
	UkryjMarsz(true);
	if (MarszW.Czas < MarszW.Caly)
	{
		return;
	}
	if (MarszW.Etap == 2)
	{
		Dotarli();
		return;
	}
	// Powrot: wchodza do obozu od strony osady rodu i staja na miejscu zbiorki.
	const FVector2D Skad = (MarszW.Wejscie - Dom).GetSafeNormal();
	const TArray<int32> Wracaja = MarszW.Oddzialy;
	UkryjMarsz(false);
	MarszW = FMarsz();
	for (int32 m = 0; m < Mil.Num(); ++m)
	{
		const int32 i = Mil[m].Oddzial;
		if (!Wracaja.Contains(i) || !Oddz.IsValidIndex(i))
		{
			continue;
		}
		FVector2D Front;
		const FVector2D Gdzie = MiejsceZbiorki(m, Front);
		FOddzial& O = Oddz[i];
		Przenies(O, Gdzie + Skad * 3000.f, -Skad);
		O.Rozkaz = ERozkaz::Idz;
		O.CelRuchu = Gdzie;
		O.FrontCelu = Front;
		O.bDoFrontu = true;
	}
	if (AOsada* Wies = Osada()) { Wies->Ogloszenie(TEXT("Wyprawa wróciła do domu")); }
}
void ABitwa::PoZdobyciu(int32 Wybor)
{
	ARody* Ro = Rody();
	if (!bZdobyta || !Ro)
	{
		return;
	}
	int32 Poleglo = 0;
	for (const FOddzial& O : Oddz) { Poleglo += O.Strona == 1 ? O.Polegli : 0; }
	const FRod R = Ro->Lista()[RodW];
	const FString T = Ro->PoWyprawie(RodW, Wybor, Poleglo);
	if (AOsada* Wies = Osada()) { Wies->Ogloszenie(T); }
	if (Wybor == 2 && CelWyprawy())
	{
		// Spalenie: woje rozbiegaja sie po osadzie i podkladaja ogien, mieszkancy uciekaja (kogo dopadna, tego bija).
		// Po ok. 45 s albo po spaleniu PalenieIle budynkow wyprawa wraca (PalenieTick).
		bZdobyta = false;
		PalenieIle = FMath::RandRange(6, 10);
		Spalono = 0;
		PalenieDo = CzasBitwy + 45.f;
		for (int32 i = 0; i < Oddz.Num(); ++i)
		{
			if (Oddz[i].Strona == 0 && Oddz[i].Stoi()) { Palenie.Add(i, -1); }
		}
		const int32 j = DodajOddzial(ERodzajWojska::Chlopi, TEXT("Mieszkańcy"), 1, FMath::Clamp(R.Rodzin / 2, 6, 16), R.Gdzie,
			MarszW.Na.IsNearlyZero() ? FVector2D(1.f, 0.f) : -MarszW.Na, 0, FLinearColor(0.42f, 0.34f, 0.25f));
		Oddz[j].bZlamany = Oddz[j].bZebral = Oddz[j].bKoniec = true;
		Oddz[j].Morale = 0.f;
		bWrogByl = false; // uciekajacy mieszkancy to nie nowa bitwa
		Odswiez();
		return;
	}
	KoniecWyprawy();
	if (Wybor != 2)
	{
		PokazKamere(Dom); // przy spaleniu kamera zostaje: widac pozar
	}
}

void ABitwa::DoDziennika(const FString& Wiersz)
{
	UE_LOG(LogTemp, Display, TEXT("[BITWA] %s"), *Wiersz);
	FFileHelper::SaveStringToFile(Wiersz + TEXT("\n"), *(FPaths::ProjectLogDir() / TEXT("Bitwa_dziennik.txt")),
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
}

void ABitwa::Dziennik()
{
	// Bezczynny = zywy woj walczacego oddzialu, ktory nie ma przeciwnika ani nie zadaje ciosu, a najblizszy zywy wrog stoi
	// blizej niz 2,5 m (czyli moglby sie bic). Strzelcy z pociskami sie nie licza.
	static const TCHAR* Rozkazy[] = { TEXT("stoj"), TEXT("idz"), TEXT("atakuj"), TEXT("odwrot") };
	int32 Zywi = 0, Walcza = 0, Bezczynni = 0, Wrecz = 0;
	for (const FOddzial& O : Oddz)
	{
		int32 Z = 0, Wal = 0, Bez = 0;
		const bool bStrzelcy = O.Strzela();
		for (const FWojownik& W : O.Ludzie)
		{
			if (!Zywy(W))
			{
				continue;
			}
			++Z;
			const bool bWalczy = W.Wrog >= 0 || W.L->Uderza();
			Wal += bWalczy ? 1 : 0;
			Wrecz += W.Wrog >= 0 ? 1 : 0;
			if (bWalczy || bStrzelcy || !O.Stoi())
			{
				continue;
			}
			const FVector2D P = Poz(W);
			bool bBlisko = false;
			for (const FOddzial& E : Oddz)
			{
				if (E.Strona == O.Strona || !E.Stoi()) { continue; }
				for (const FWojownik& V : E.Ludzie)
				{
					if (Zywy(V) && Dl(P, Poz(V)) < 250.f) { bBlisko = true; break; }
				}
				if (bBlisko) { break; }
			}
			Bez += bBlisko ? 1 : 0;
		}
		Zywi += Z;
		Walcza += Wal;
		Bezczynni += Bez;
		if (bDziennik && O.NaPolu() > 0)
		{
			DoDziennika(FString::Printf(TEXT("t=%5.1f %-12s s%d %-7s cel=%-2d zywi=%2d/%2d walczy=%2d bezczynni=%2d duch=%3.0f zmecz=%3.0f flanka=%.2f %s%s poz=(%.0f,%.0f)"),
				CzasBitwy, *O.Nazwa, O.Strona, Rozkazy[FMath::Clamp(int32(O.Rozkaz), 0, 3)], O.Cel, Z, O.Poczatkowo, Wal, Bez, O.Morale,
				O.Zmeczenie, O.Flanka, *FString(OpisStanu(O)), O.bZlamany ? TEXT(" ZLAMANY") : TEXT(""), O.Srodek.X, O.Srodek.Y));
		}
	}
	if (bWalkaTrwa && Zywi > 0)
	{
		FMiary& M = MiaryB;
		++M.Probek;
		M.Zywych += Zywi;
		M.Walczacych += Walcza;
		M.Bezczynnych += Bezczynni;
		M.MaxBezczynnych = FMath::Max(M.MaxBezczynnych, Bezczynni);
		if (M.Zwarcie < 0.f && Wrecz > 0) { M.Zwarcie = CzasBitwy; }
	}
}
void ABitwa::PalenieTick(float Dt)
{
	if (Palenie.Num() == 0 && PalenieDo <= 0.f)
	{
		return;
	}
	AOsadaRodu* V = CelWyprawy();
	if (!V)
	{
		Palenie.Reset();
		PalenieDo = 0.f;
		return;
	}
	const bool bSpalenie = PalenieDo > 0.f;
	// Oddzial bity przez wroga przerywa podpalanie i walczy (Zwarcie).
	TSet<int32> Bici;
	for (const FOddzial& E : Oddz)
	{
		for (const FWojownik& W : E.Ludzie) { if (Zywy(W) && W.Wrog >= 0) { Bici.Add(W.Wrog); } }
	}
	for (auto It = Palenie.CreateIterator(); It; ++It)
	{
		const int32 i = It.Key();
		if (!Oddz.IsValidIndex(i) || !Oddz[i].Stoi() || Bici.Contains(i))
		{
			It.RemoveCurrent();
			continue;
		}
		FOddzial& O = Oddz[i];
		const int32 Cel = It.Value();
		bool bZrobione = false;
		FVector2D Sr = FVector2D::ZeroVector;
		int32 N = 0;
		for (int32 w = 0; w < O.Ludzie.Num(); ++w)
		{
			FWojownik& W = O.Ludzie[w];
			if (!Zywy(W))
			{
				continue;
			}
			const FVector2D P = Poz(W);
			Sr += P;
			++N;
			W.Odnowa -= Dt;
			if (W.L->Uderza())
			{
				continue;
			}
			if (bSpalenie)
			{
				// Uciekajacy mieszkancy w poblizu: dogonic i uderzyc.
				int32 Oj = -1, Vj = -1;
				float NajD = 700.f;
				for (int32 j = 0; j < Oddz.Num(); ++j)
				{
					if (Oddz[j].Strona == O.Strona || Oddz[j].bPoza) { continue; }
					for (int32 v = 0; v < Oddz[j].Ludzie.Num(); ++v)
					{
						const FWojownik& E = Oddz[j].Ludzie[v];
						if (Zywy(E) && Dl(Poz(E), P) < NajD) { NajD = Dl(Poz(E), P); Oj = j; Vj = v; }
					}
				}
				if (Oj >= 0)
				{
					const FVector2D PE = Poz(Oddz[Oj].Ludzie[Vj]);
					if (NajD > ZasiegCiosu)
					{
						W.L->WalkaIdz(PE, Bieg);
					}
					else if (W.Odnowa <= 0.f)
					{
						W.Odnowa = FMath::FRandRange(0.9f, 1.3f);
						W.L->WalkaCios(PE);
						Ciosy.Add({ i, w, Oj, Vj, ALudzik::DoUderzenia, FMath::FRand() < 0.7f, false, false });
					}
					continue;
				}
			}
			// Budynek: rozkazany (Cel) albo najblizszy jeszcze caly (kazdy woj swoj).
			float R = 0.f;
			int32 Dz = Cel >= 0 ? Cel : W.Podpala;
			if (Dz < 0 || V->DoSpalenia(V->MiejsceDzialki(Dz, R), 1.f) != Dz)
			{
				if (Cel >= 0)
				{
					bZrobione = true; // juz plonie
					continue;
				}
				Dz = W.Podpala = V->DoSpalenia(P, 8000.f);
				if (Dz < 0)
				{
					W.L->WalkaStoj(P + O.Front * 1000.f);
					continue;
				}
			}
			const FVector2D M = V->MiejsceDzialki(Dz, R);
			// Kazdy podchodzi z innej strony (nie tloczy sie w jednym miejscu).
			const FVector2D Od = (P - M).IsNearlyZero() ? O.Front : (P - M).GetSafeNormal();
			const FVector2D Przy = M + Od * (R + 70.f);
			if (Dl(P, Przy) > 130.f)
			{
				W.L->WalkaIdz(Przy, Bieg);
			}
			else if (W.Odnowa <= 0.f)
			{
				W.Odnowa = 1.f;
				W.L->WalkaCios(M); // przyklada pochodnie / wybija okiennice
				if (V->Podpal(Dz))
				{
					++Spalono;
					UDzwieki::Graj(this, "Okrzyk", W.L->GetActorLocation(), EDzwiek::Glosy, 0.5f);
				}
				W.Podpala = -1;
				bZrobione |= Cel >= 0;
			}
		}
		if (N > 0)
		{
			O.Srodek = O.CelRuchu = Sr / N; // choragiew i karta oddzialu ida za ludzmi
		}
		if (bZrobione)
		{
			O.Rozkaz = ERozkaz::Stoj;
			O.bUloz = true;
			It.RemoveCurrent();
		}
	}
	if (bSpalenie && (CzasBitwy >= PalenieDo || Spalono >= PalenieIle || Palenie.Num() == 0))
	{
		if (Spalono < PalenieIle)
		{
			V->Spal(PalenieIle - Spalono); // reszte ogien bierze sam (przeskakuje z dachow)
		}
		PalenieDo = 0.f;
		Palenie.Reset();
		UE_LOG(LogTemp, Log, TEXT("Bitwa: woje podpalili %d budynkow, wyprawa wraca"), Spalono);
		KoniecWyprawy();
	}
}