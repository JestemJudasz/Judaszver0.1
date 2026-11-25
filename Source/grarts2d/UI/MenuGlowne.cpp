// Plik: Source/grarts2d/UI/MenuGlowne.cpp
#include "UI/MenuGlowne.h"
#include "UI/Ustawienia.h"
#include "UI/StylUI.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	TSharedPtr<SMenuGlowne> GMenu;
	TWeakObjectPtr<UWorld> GSwiat;

	FSlateFontInfo Czcionka(int32 R, bool bGruba = false) { return FCoreStyle::GetDefaultFontStyle(bGruba ? "Bold" : "Regular", R); }
	UGameUserSettings* GUS() { return GEngine ? GEngine->GetGameUserSettings() : nullptr; }
	void ZastosujGrafike()
	{
		if (UGameUserSettings* G = GUS()) { G->ApplySettings(false); }
		FUstawienia::ZastosujObraz(); // po ApplySettings: wlasne wygladzanie i skala renderu wygrywaja
	}
	FString Proc(float V) { return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(V * 100.f)); }
	const TArray<FString> Poziomy = { TEXT("Niska"), TEXT("Średnia"), TEXT("Wysoka"), TEXT("Epicka"), TEXT("Kinowa") };
	const TArray<FString> NieTak = { TEXT("Nie"), TEXT("Tak") };
}

void SMenuGlowne::Otworz(UWorld* Swiat, bool bStart)
{
	if (GMenu.IsValid() || !Swiat || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	GSwiat = Swiat;
	GMenu = SNew(SMenuGlowne).Start(bStart);
	GEngine->GameViewport->AddViewportWidgetContent(GMenu.ToSharedRef(), 300);
	if (APlayerController* PC = Swiat->GetFirstPlayerController())
	{
		FInputModeUIOnly Tryb;
		Tryb.SetWidgetToFocus(GMenu);
		PC->SetInputMode(Tryb);
		PC->bShowMouseCursor = true;
	}
	if (!bStart)
	{
		UGameplayStatics::SetGamePaused(Swiat, true);
	}
}

void SMenuGlowne::Zamknij()
{
	if (!GMenu.IsValid())
	{
		return;
	}
	FUstawienia::Zapisz();
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(GMenu.ToSharedRef());
	}
	GMenu.Reset();
	if (UWorld* W = GSwiat.Get())
	{
		if (APlayerController* PC = W->GetFirstPlayerController())
		{
			FInputModeGameAndUI Tryb; // jak w AKameraMapy::BeginPlay
			Tryb.SetHideCursorDuringCapture(false);
			PC->SetInputMode(Tryb);
		}
		UGameplayStatics::SetGamePaused(W, false);
	}
}

bool SMenuGlowne::Otwarte() { return GMenu.IsValid(); }

void SMenuGlowne::Construct(const FArguments& InArgs)
{
	bStart = InArgs._Start;
	Styl = StylUI::StylPrzycisku();
	StylWybranej = StylUI::StylPrzycisku();
	StylWybranej.SetNormal(FSlateRoundedBoxBrush(StylUI::PrzyciskNad, 4.f, StylUI::Zloto, 1.f));
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, bStart ? 0.35f : 0.55f))
		.Padding(0.f)
		[
			SAssignNew(Miejsce, SBox)
		]
	];
	int32 Z = -1;
	FParse::Value(FCommandLine::Get(), TEXT("zakladka="), Z); // zrzut kontrolny ustawien: -zrzut -menu -zakladka=0
	Z >= 0 ? PokazUstawienia(FMath::Min(Z, 4)) : PokazGlowne();
}

FReply SMenuGlowne::OnPreviewKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	if (CzekaNa.IsNone())
	{
		return FReply::Unhandled();
	}
	if (E.GetKey() != EKeys::Escape)
	{
		FUstawienia::UstawKlawisz(CzekaNa, E.GetKey());
	}
	CzekaNa = NAME_None;
	return FReply::Handled();
}

FReply SMenuGlowne::OnKeyDown(const FGeometry& G, const FKeyEvent& E)
{
	if (E.GetKey() == EKeys::Escape || E.GetKey() == EKeys::F10)
	{
		if (Zakladka >= 0)
		{
			FUstawienia::Zapisz();
			PokazGlowne();
		}
		else if (!bStart)
		{
			Zamknij();
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

TSharedRef<SWidget> SMenuGlowne::Guzik(const FString& Tekst, TFunction<void()> Akcja, bool bAktywny, const FString& Podp)
{
	return SNew(SButton).ButtonStyle(&Styl).IsEnabled(bAktywny).HAlign(HAlign_Center)
		.ToolTipText(FText::FromString(Podp))
		.OnClicked_Lambda([Akcja]() { Akcja(); return FReply::Handled(); })
		[
			SNew(STextBlock).Text(FText::FromString(Tekst)).Font(Czcionka(13, true))
			.ColorAndOpacity(bAktywny ? StylUI::Zloto : StylUI::Szary)
		];
}

void SMenuGlowne::PokazGlowne()
{
	Zakladka = -1;
	CzekaNa = NAME_None;
	static FSlateRoundedBoxBrush Tlo(StylUI::Tlo, 6.f, StylUI::Ramka, 1.f);
	auto Przycisk = [this](const FString& T, TFunction<void()> A, bool bAkt = true, const FString& P = FString())
	{
		return SNew(SBox).WidthOverride(300.f).HeightOverride(46.f).Padding(FMargin(0.f, 0.f, 0.f, 8.f))[Guzik(T, A, bAkt, P)];
	};
	TWeakObjectPtr<UWorld> W = GSwiat;
	Miejsce->SetContent(
		SNew(SBox).HAlign(HAlign_Left).VAlign(VAlign_Center).Padding(FMargin(90.f, 0.f, 0.f, 0.f))
		[
			SNew(SBorder).BorderImage(&Tlo).Padding(FMargin(28.f, 24.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("GRARTS2D"))).Font(Czcionka(34, true)).ColorAndOpacity(StylUI::Zloto)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 22.f)
				[
					SNew(STextBlock).Text(FText::FromString(bStart ? TEXT("Osady, rody i wojny") : TEXT("Pauza"))).Font(Czcionka(12)).ColorAndOpacity(StylUI::Szary)
				]
				+ SVerticalBox::Slot().AutoHeight()[Przycisk(bStart ? TEXT("Nowa gra") : TEXT("Kontynuuj"), []() { Zamknij(); })]
				+ SVerticalBox::Slot().AutoHeight()[Przycisk(TEXT("Wczytaj grę"), []() {}, false, TEXT("Zapis gry jeszcze nie istnieje"))]
				+ SVerticalBox::Slot().AutoHeight()[Przycisk(TEXT("Ustawienia"), [this]() { PokazUstawienia(0); })]
				+ SVerticalBox::Slot().AutoHeight()[Przycisk(TEXT("Wyjście"), [W]()
				{
					if (UWorld* S = W.Get()) { UKismetSystemLibrary::QuitGame(S, S->GetFirstPlayerController(), EQuitPreference::Quit, false); }
				})]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(STextBlock).Text(FText::FromString(bStart ? TEXT("") : TEXT("Esc / F10 — wróć do gry"))).Font(Czcionka(9)).ColorAndOpacity(StylUI::Szary)
				]
			]
		]);
}

void SMenuGlowne::PokazUstawienia(int32 Nowa)
{
	Zakladka = Nowa;
	CzekaNa = NAME_None;
	static FSlateRoundedBoxBrush Tlo(StylUI::Tlo, 6.f, StylUI::Ramka, 1.f);
	const TCHAR* Nazwy[] = { TEXT("Grafika"), TEXT("Dźwięk"), TEXT("Sterowanie"), TEXT("Kamera"), TEXT("Interfejs") };
	TSharedRef<SHorizontalBox> Zakladki = SNew(SHorizontalBox);
	for (int32 i = 0; i < UE_ARRAY_COUNT(Nazwy); ++i)
	{
		Zakladki->AddSlot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SButton).ButtonStyle(i == Zakladka ? &StylWybranej : &Styl)
			.OnClicked_Lambda([this, i]() { PokazUstawienia(i); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(Nazwy[i])).Font(Czcionka(12, true))
				.ColorAndOpacity(i == Zakladka ? StylUI::Zloto : StylUI::Napis)
			]
		];
	}
	TSharedRef<SWidget> Tresc = Zakladka == 0 ? Grafika() : Zakladka == 1 ? Dzwiek() : Zakladka == 2 ? Sterowanie() : Zakladka == 3 ? Kamera() : Interfejs();
	Miejsce->SetContent(
		SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBorder).BorderImage(&Tlo).Padding(FMargin(22.f, 16.f))
			[
				SNew(SBox).WidthOverride(780.f).HeightOverride(620.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 10.f)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("USTAWIENIA"))).Font(Czcionka(18, true)).ColorAndOpacity(StylUI::Zloto)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 12.f)[Zakladki]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SScrollBox) + SScrollBox::Slot().Padding(0.f, 0.f, 10.f, 0.f)[Tresc]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 10.f, 0.f, 0.f)
					[
						SNew(SBox).WidthOverride(160.f)[Guzik(TEXT("Wróć"), [this]() { FUstawienia::Zapisz(); PokazGlowne(); })]
					]
				]
			]
		]);
}

TSharedRef<SWidget> SMenuGlowne::Naglowek(const FString& Tekst)
{
	return SNew(SBox).Padding(FMargin(0.f, 10.f, 0.f, 4.f))
		[SNew(STextBlock).Text(FText::FromString(Tekst)).Font(Czcionka(12, true)).ColorAndOpacity(StylUI::Zloto)];
}

TSharedRef<SWidget> SMenuGlowne::Uwaga(const FString& Tekst)
{
	return SNew(SBox).Padding(FMargin(0.f, 8.f, 0.f, 0.f))
		[SNew(STextBlock).Text(FText::FromString(Tekst)).Font(Czcionka(9)).ColorAndOpacity(StylUI::Szary).AutoWrapText(true)];
}

TSharedRef<SWidget> SMenuGlowne::Wiersz(const FString& Nazwa, TSharedRef<SWidget> Wartosc, const FString& Podp)
{
	return SNew(SBox).Padding(FMargin(0.f, 3.f)).ToolTipText(FText::FromString(Podp))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(Nazwa)).Font(Czcionka(11)).ColorAndOpacity(StylUI::Napis)
			]
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SBox).WidthOverride(340.f)[Wartosc]]
		];
}

TSharedRef<SWidget> SMenuGlowne::Wybor(const FString& Nazwa, TFunction<TArray<FString>()> Opcje, TFunction<int32()> Indeks,
	TFunction<void(int32)> Ustaw, const FString& Podp)
{
	auto Krok = [Opcje, Indeks, Ustaw](int32 D)
	{
		const int32 N = Opcje().Num();
		if (N > 0) { Ustaw((FMath::Max(Indeks(), 0) + D + N) % N); }
		return FReply::Handled();
	};
	auto Strzalka = [this, Krok](const TCHAR* T, int32 D)
	{
		return SNew(SBox).WidthOverride(36.f)
			[
				SNew(SButton).ButtonStyle(&Styl).HAlign(HAlign_Center).OnClicked_Lambda([Krok, D]() { return Krok(D); })
				[SNew(STextBlock).Text(FText::FromString(T)).Font(Czcionka(11, true)).ColorAndOpacity(StylUI::Zloto)]
			];
	};
	return Wiersz(Nazwa,
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[Strzalka(TEXT("<"), -1)]
		+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(Czcionka(11, true)).ColorAndOpacity(StylUI::Napis)
			.Text_Lambda([Opcje, Indeks]()
			{
				const TArray<FString> O = Opcje();
				const int32 i = Indeks();
				return FText::FromString(O.IsValidIndex(i) ? O[i] : FString(TEXT("Własne")));
			})
		]
		+ SHorizontalBox::Slot().AutoWidth()[Strzalka(TEXT(">"), 1)],
		Podp);
}

TSharedRef<SWidget> SMenuGlowne::Suwak(const FString& Nazwa, float Min, float Max, float Krok, TFunction<float()> Wartosc,
	TFunction<void(float)> Ustaw, TFunction<FString(float)> Napis)
{
	return Wiersz(Nazwa,
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SSlider).MinValue(Min).MaxValue(Max).StepSize(Krok)
			.SliderBarColor(StylUI::Ramka).SliderHandleColor(StylUI::Zloto)
			.Value_Lambda([Wartosc]() { return Wartosc(); })
			.OnValueChanged_Lambda([Ustaw](float V) { Ustaw(V); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
		[
			SNew(SBox).WidthOverride(80.f)
			[
				SNew(STextBlock).Font(Czcionka(11, true)).ColorAndOpacity(StylUI::Napis)
				.Text_Lambda([Wartosc, Napis]() { return FText::FromString(Napis(Wartosc())); })
			]
		]);
}

TSharedRef<SWidget> SMenuGlowne::Grafika()
{
	TSharedRef<SVerticalBox> B = SNew(SVerticalBox);
	auto Dodaj = [&B](TSharedRef<SWidget> W) { B->AddSlot().AutoHeight()[W]; };

	Dodaj(Naglowek(TEXT("Ekran")));
	TArray<FIntPoint> Rozdz;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Rozdz);
	Rozdz.Sort([](const FIntPoint& A, const FIntPoint& C) { return A.X * A.Y < C.X * C.Y; });
	TArray<FString> RozdzN;
	for (const FIntPoint& R : Rozdz) { RozdzN.Add(FString::Printf(TEXT("%d × %d"), R.X, R.Y)); }
	Dodaj(Wybor(TEXT("Rozdzielczość"), [RozdzN]() { return RozdzN; },
		[Rozdz]() { return GUS() ? Rozdz.IndexOfByKey(GUS()->GetScreenResolution()) : -1; },
		[Rozdz](int32 i) { if (GUS() && Rozdz.IsValidIndex(i)) { GUS()->SetScreenResolution(Rozdz[i]); ZastosujGrafike(); } }));
	const TArray<EWindowMode::Type> Tryby = { EWindowMode::Fullscreen, EWindowMode::WindowedFullscreen, EWindowMode::Windowed };
	Dodaj(Wybor(TEXT("Tryb okna"), []() { return TArray<FString>{ TEXT("Pełny ekran"), TEXT("Okno bez ramki"), TEXT("Okno") }; },
		[Tryby]() { return GUS() ? Tryby.IndexOfByKey(GUS()->GetFullscreenMode()) : -1; },
		[Tryby](int32 i) { if (GUS()) { GUS()->SetFullscreenMode(Tryby[i]); ZastosujGrafike(); } }));
	Dodaj(Wybor(TEXT("Synchronizacja pionowa (VSync)"), []() { return NieTak; },
		[]() { return GUS() && GUS()->IsVSyncEnabled() ? 1 : 0; },
		[](int32 i) { if (GUS()) { GUS()->SetVSyncEnabled(i == 1); ZastosujGrafike(); } }));
	const TArray<float> Fps = { 30.f, 60.f, 90.f, 120.f, 144.f, 165.f, 240.f, 0.f };
	Dodaj(Wybor(TEXT("Limit klatek na sekundę"),
		[Fps]() { TArray<FString> O; for (float F : Fps) { O.Add(F > 0.f ? FString::Printf(TEXT("%d"), int32(F)) : FString(TEXT("Bez limitu"))); } return O; },
		[Fps]() { return GUS() ? Fps.IndexOfByPredicate([](float F) { return FMath::IsNearlyEqual(F, GUS()->GetFrameRateLimit()); }) : -1; },
		[Fps](int32 i) { if (GUS()) { GUS()->SetFrameRateLimit(Fps[i]); ZastosujGrafike(); } }));

	Dodaj(Naglowek(TEXT("Jakość")));
	Dodaj(Wybor(TEXT("Jakość ogólna"), []() { return Poziomy; },
		[]() { return GUS() ? GUS()->GetOverallScalabilityLevel() : -1; },
		[](int32 i) { if (GUS()) { GUS()->SetOverallScalabilityLevel(i); ZastosujGrafike(); } },
		TEXT("Ustawia naraz wszystkie poziomy poniżej")));
	struct FGrupa { const TCHAR* Nazwa; int32 (UGameUserSettings::*Daj)() const; void (UGameUserSettings::*Ustaw)(int32); };
	const FGrupa Grupy[] = {
		{ TEXT("Cienie"), &UGameUserSettings::GetShadowQuality, &UGameUserSettings::SetShadowQuality },
		{ TEXT("Tekstury"), &UGameUserSettings::GetTextureQuality, &UGameUserSettings::SetTextureQuality },
		{ TEXT("Efekty"), &UGameUserSettings::GetVisualEffectQuality, &UGameUserSettings::SetVisualEffectQuality },
		{ TEXT("Roślinność"), &UGameUserSettings::GetFoliageQuality, &UGameUserSettings::SetFoliageQuality },
		{ TEXT("Zasięg widzenia"), &UGameUserSettings::GetViewDistanceQuality, &UGameUserSettings::SetViewDistanceQuality },
		{ TEXT("Obróbka obrazu"), &UGameUserSettings::GetPostProcessingQuality, &UGameUserSettings::SetPostProcessingQuality },
	};
	for (const FGrupa& G : Grupy)
	{
		Dodaj(Wybor(G.Nazwa, []() { return Poziomy; },
			[G]() { return GUS() ? (GUS()->*G.Daj)() : -1; },
			[G](int32 i) { if (GUS()) { (GUS()->*G.Ustaw)(i); ZastosujGrafike(); } }));
	}

	Dodaj(Naglowek(TEXT("Wygładzanie i skalowanie")));
	const TArray<FString> Metody = FUstawienia::MetodyWygladzania();
	Dodaj(Wybor(TEXT("Metoda wygładzania"), [Metody]() { return Metody; },
		[Metody]() { return Metody.IndexOfByKey(FUstawienia::Wygladzanie); },
		[Metody](int32 i) { FUstawienia::Wygladzanie = Metody[i]; FUstawienia::ZastosujObraz(); },
		TEXT("TSR: wbudowane w silnik, domyślne. DLSS / FSR / XeSS: po dodaniu ich wtyczek. TAA, FXAA: słabsze. Brak: ostre, ale migocze.")));
	Dodaj(Suwak(TEXT("Skala renderu"), 50.f, 100.f, 1.f, []() { return FUstawienia::SkalaRenderu; },
		[](float V) { FUstawienia::SkalaRenderu = V; FUstawienia::ZastosujObraz(); },
		[](float V)
		{
			const bool bDLAA = FUstawienia::Wygladzanie == TEXT("DLSS") && V >= 99.5f;
			return FString::Printf(TEXT("%d%%%s"), FMath::RoundToInt(V), bDLAA ? TEXT(" DLAA") : TEXT(""));
		}));
	FString Brak;
	for (const TCHAR* M : { TEXT("DLSS"), TEXT("FSR"), TEXT("XeSS") })
	{
		if (!Metody.Contains(M)) { Brak += Brak.IsEmpty() ? TEXT("") : TEXT(", "); Brak += M; }
	}
	Dodaj(Uwaga(TEXT("100% = pełna rozdzielczość z TSR (zalecane). Mniej = szybciej, ale bardziej miękko; upscaler dorysowuje brakujące piksele.")
		+ (Brak.IsEmpty() ? FString() : FString::Printf(TEXT(" Niedostępne, bo w grze nie ma ich wtyczek: %s."), *Brak))));
	return B;
}

TSharedRef<SWidget> SMenuGlowne::Dzwiek()
{
	TSharedRef<SVerticalBox> B = SNew(SVerticalBox);
	B->AddSlot().AutoHeight()[Naglowek(TEXT("Głośność"))];
	B->AddSlot().AutoHeight()[Suwak(TEXT("Ogólna"), 0.f, 1.f, 0.01f, []() { return FUstawienia::GlosOgolna; },
		[](float V) { FUstawienia::GlosOgolna = V; }, Proc)];
	// Kolejnosc jak EDzwiek (Dzwiek/Dzwieki.h).
	const TCHAR* Kat[] = { TEXT("Walka"), TEXT("Praca"), TEXT("Głosy ludzi"), TEXT("Osada i zwierzęta"), TEXT("Rozkazy oddziałów") };
	for (int32 i = 0; i < UE_ARRAY_COUNT(Kat); ++i)
	{
		B->AddSlot().AutoHeight()[Suwak(Kat[i], 0.f, 1.f, 0.01f, [i]() { return FUstawienia::GlosKat[i]; },
			[i](float V) { FUstawienia::GlosKat[i] = V; }, Proc)];
	}
	B->AddSlot().AutoHeight()[Uwaga(TEXT("Muzyki w grze jeszcze nie ma."))];
	return B;
}

TSharedRef<SWidget> SMenuGlowne::Sterowanie()
{
	TSharedRef<SVerticalBox> B = SNew(SVerticalBox);
	FString Grupa;
	for (const FUstawienia::FAkcja& A : FUstawienia::Akcje())
	{
		if (Grupa != A.Grupa)
		{
			Grupa = A.Grupa;
			B->AddSlot().AutoHeight()[Naglowek(Grupa)];
		}
		const FName N = A.Nazwa;
		const FString G = A.Grupa;
		B->AddSlot().AutoHeight()[Wiersz(A.Opis,
			SNew(SButton).ButtonStyle(&Styl).HAlign(HAlign_Center)
			.OnClicked_Lambda([this, N]() { CzekaNa = N; return FReply::Handled(); })
			[
				SNew(STextBlock).Font(Czcionka(11, true))
				.Text_Lambda([this, N]()
				{
					return CzekaNa == N ? FText::FromString(TEXT("Naciśnij klawisz… (Esc anuluje)")) : FUstawienia::Klawisz(N).GetDisplayName();
				})
				// Czerwony: ten sam klawisz ma inna czynnosc z tej samej grupy.
				.ColorAndOpacity_Lambda([N, G]()
				{
					for (const FUstawienia::FAkcja& X : FUstawienia::Akcje())
					{
						if (X.Nazwa != N && G == X.Grupa && FUstawienia::Klawisz(X.Nazwa) == FUstawienia::Klawisz(N)) { return FSlateColor(StylUI::Czerwony); }
					}
					return FSlateColor(StylUI::Zloto);
				})
			])];
	}
	B->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, 12.f, 0.f, 0.f)
	[
		SNew(SBox).WidthOverride(220.f)[Guzik(TEXT("Przywróć domyślne"), []() { FUstawienia::DomyslneKlawisze(); })]
	];
	B->AddSlot().AutoHeight()[Uwaga(TEXT("Strzałki zawsze przesuwają kamerę. Mysz: kółko przybliża, lewy przycisk przesuwa mapę, "
		"środkowy obraca kamerę, prawy wydaje rozkazy. Czerwony klawisz jest zajęty przez inną czynność z tej samej grupy."))];
	return B;
}

TSharedRef<SWidget> SMenuGlowne::Kamera()
{
	TSharedRef<SVerticalBox> B = SNew(SVerticalBox);
	auto Mn = [](float V) { return FString::Printf(TEXT("× %.2f"), V); };
	B->AddSlot().AutoHeight()[Naglowek(TEXT("Kamera"))];
	B->AddSlot().AutoHeight()[Suwak(TEXT("Prędkość przesuwu"), 0.25f, 3.f, 0.05f, []() { return FUstawienia::PrzesuwMn; }, [](float V) { FUstawienia::PrzesuwMn = V; }, Mn)];
	B->AddSlot().AutoHeight()[Suwak(TEXT("Prędkość obrotu (klawisze)"), 0.25f, 3.f, 0.05f, []() { return FUstawienia::ObrotMn; }, [](float V) { FUstawienia::ObrotMn = V; }, Mn)];
	B->AddSlot().AutoHeight()[Suwak(TEXT("Czułość obrotu myszą"), 0.25f, 3.f, 0.05f, []() { return FUstawienia::MyszMn; }, [](float V) { FUstawienia::MyszMn = V; }, Mn)];
	B->AddSlot().AutoHeight()[Suwak(TEXT("Szybkość przybliżania"), 0.25f, 3.f, 0.05f, []() { return FUstawienia::ZoomMn; }, [](float V) { FUstawienia::ZoomMn = V; }, Mn)];
	B->AddSlot().AutoHeight()[Wybor(TEXT("Odwróć kółko myszy"), []() { return NieTak; }, []() { return FUstawienia::bOdwrocZoom ? 1 : 0; }, [](int32 i) { FUstawienia::bOdwrocZoom = i == 1; })];
	B->AddSlot().AutoHeight()[Wybor(TEXT("Odwróć obrót myszą"), []() { return NieTak; }, []() { return FUstawienia::bOdwrocObrot ? 1 : 0; }, [](int32 i) { FUstawienia::bOdwrocObrot = i == 1; })];
	B->AddSlot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, 12.f, 0.f, 0.f)
	[
		SNew(SBox).WidthOverride(220.f)[Guzik(TEXT("Przywróć domyślne"), []()
		{
			FUstawienia::PrzesuwMn = FUstawienia::ObrotMn = FUstawienia::MyszMn = FUstawienia::ZoomMn = 1.f;
			FUstawienia::bOdwrocZoom = FUstawienia::bOdwrocObrot = false;
		})]
	];
	return B;
}

TSharedRef<SWidget> SMenuGlowne::Interfejs()
{
	TSharedRef<SVerticalBox> B = SNew(SVerticalBox);
	const TArray<float> Skale = { 0.75f, 0.9f, 1.f, 1.1f, 1.25f, 1.5f };
	B->AddSlot().AutoHeight()[Naglowek(TEXT("Interfejs"))];
	B->AddSlot().AutoHeight()[Wybor(TEXT("Skala interfejsu"),
		[Skale]() { TArray<FString> O; for (float S : Skale) { O.Add(Proc(S)); } return O; },
		[Skale]() { return Skale.IndexOfByPredicate([](float S) { return FMath::IsNearlyEqual(S, FUstawienia::SkalaUI, 0.01f); }); },
		[Skale](int32 i) { FUstawienia::SkalaUI = Skale[i]; FUstawienia::ZastosujUI(); })];
	const TArray<FString> Jezyki = { TEXT("pl"), TEXT("en") };
	B->AddSlot().AutoHeight()[Wybor(TEXT("Język"), []() { return TArray<FString>{ TEXT("Polski"), TEXT("English") }; },
		[Jezyki]() { return Jezyki.IndexOfByKey(FUstawienia::Jezyk); },
		[Jezyki](int32 i) { FUstawienia::Jezyk = Jezyki[i]; FUstawienia::ZastosujUI(); })];
	B->AddSlot().AutoHeight()[Uwaga(TEXT("Tłumaczenia angielskiego jeszcze nie ma: wybór się zapisuje, ale napisy zostają po polsku."))];
	return B;
}
