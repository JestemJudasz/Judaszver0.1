// Plik: Source/grarts2d/UI/PanelDeweloperski.cpp
#include "UI/PanelDeweloperski.h"
#include "UI/StylUI.h"
#include "Budowy/Budowa.h"
#include "Osada/Osada.h"
#include "Osada/Rody.h"
#include "Dev/Galeria.h"
#include "Wojna/Bitwa.h"
#include "Swiat/TerenKafle.h" // Bitwa.h: UstawTeren potrzebuje pelnego typu
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

bool FTrybDeweloperski::bOtwarty = false;
bool FTrybDeweloperski::bPauza = false;
float FTrybDeweloperski::Predkosc = 1.f;

namespace
{
	float Ustawione = 1.f; // ostatnio ustawiony czas swiata (zeby w bitwie wrocic do x1 tylko raz)
}

void FTrybDeweloperski::Wyzeruj()
{
	bOtwarty = false;
	bPauza = false;
	Predkosc = 1.f;
	Ustawione = 1.f;
	ABudowa::bNatychmiast = FParse::Param(FCommandLine::Get(), TEXT("natychmiast")); // zrzut kontrolny: -natychmiast
}

void FTrybDeweloperski::Zastosuj(UWorld* W, AActor* Sterujacy, bool bBitwa)
{
	AWorldSettings* WS = W ? W->GetWorldSettings() : nullptr;
	if (!WS)
	{
		return;
	}
	// Budowa natychmiast: ABudowa ma Tick wylaczony (tylko czesci ruchome), wiec place budowy konczy tutaj.
	if (ABudowa::bNatychmiast)
	{
		for (TActorIterator<ABudowa> It(W); It; ++It)
		{
			if (!It->Podglad() && !It->Gotowy() && !It->Ruina())
			{
				It->UstawPostep(1.f);
				UE_LOG(LogTemp, Log, TEXT("Tryb deweloperski: budowa natychmiast - %s gotowy"), *It->Rodzaj().Nazwa);
			}
		}
	}
	APlayerController* PC = W->GetFirstPlayerController();
	APawn* Kamera = PC ? PC->GetPawn() : nullptr;
	float Odwrot = 1.f;
	if (bBitwa)
	{
		// Bitwa ma wlasna pauze i zwolnienie (ABitwa); przyspieszenie z panelu zdejmujemy raz.
		if (!FMath::IsNearlyEqual(Ustawione, 1.f))
		{
			UGameplayStatics::SetGlobalTimeDilation(W, 1.f);
			Ustawione = 1.f;
		}
	}
	else
	{
		const float Cel = bPauza ? WS->MinGlobalTimeDilation : FMath::Clamp(Predkosc, WS->MinGlobalTimeDilation, WS->MaxGlobalTimeDilation);
		if (!FMath::IsNearlyEqual(UGameplayStatics::GetGlobalTimeDilation(W), Cel))
		{
			UGameplayStatics::SetGlobalTimeDilation(W, Cel);
		}
		Ustawione = UGameplayStatics::GetGlobalTimeDilation(W);
		Odwrot = 1.f / FMath::Max(Ustawione, KINDA_SMALL_NUMBER);
	}
	// Kamera i budowniczy (klawisze, liczniki) chodza w czasie rzeczywistym.
	if (Kamera)
	{
		Kamera->CustomTimeDilation = Odwrot;
	}
	if (Sterujacy)
	{
		Sterujacy->CustomTimeDilation = Odwrot;
	}
}

FString FTrybDeweloperski::NapisCzasu()
{
	return bPauza ? FString(TEXT("pauza")) : FString::Printf(TEXT("x%d"), FMath::RoundToInt32(Predkosc));
}

TSharedRef<SWidget> SPanelDeweloperski::Etykieta(const FString& T)
{
	return SNew(STextBlock)
		.Text(FText::FromString(T))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
		.ColorAndOpacity(StylUI::Szary);
}

TSharedRef<SWidget> SPanelDeweloperski::Przycisk(const TAttribute<FText>& Napis, TFunction<void()> Akcja, const TAttribute<FSlateColor>& Kolor, const FString& Podpowiedz)
{
	return SNew(SButton)
		.ButtonStyle(&Styl)
		.ContentPadding(FMargin(8.f, 2.f))
		.ToolTipText(FText::FromString(Podpowiedz))
		.OnClicked_Lambda([Akcja]() { Akcja(); return FReply::Handled(); })
		[
			SNew(STextBlock)
			.Text(Napis)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(Kolor)
		];
}

TSharedRef<SWidget> SPanelDeweloperski::PrzyciskCzasu(float P)
{
	const bool bPauzaP = P <= 0.f;
	const FString N = bPauzaP ? FString(TEXT("Pauza")) : FString::Printf(TEXT("x%d"), FMath::RoundToInt32(P));
	return Przycisk(FText::FromString(N),
		[P, bPauzaP]()
		{
			FTrybDeweloperski::bPauza = bPauzaP;
			if (!bPauzaP)
			{
				FTrybDeweloperski::Predkosc = P;
			}
		},
		TAttribute<FSlateColor>::CreateLambda([P, bPauzaP]()
		{
			const bool bWybrany = bPauzaP ? FTrybDeweloperski::bPauza : (!FTrybDeweloperski::bPauza && FMath::IsNearlyEqual(FTrybDeweloperski::Predkosc, P));
			return FSlateColor(bWybrany ? StylUI::Zloto : StylUI::Napis);
		}),
		bPauzaP ? TEXT("Zatrzymaj czas gry (kamera dalej działa)") : TEXT("Szybkość upływu czasu gry"));
}

void SPanelDeweloperski::Dodaj(int32 Ile)
{
	AOsada* O = Osada.Get();
	if (!O || !O->MaOboz())
	{
		Wynik = TEXT("Najpierw załóż obóz");
		return;
	}
	O->DodajSurowce(Ile);
	Wynik = FString::Printf(TEXT("Dodano po %d"), Ile);
}

void SPanelDeweloperski::Construct(const FArguments& InArgs)
{
	Osada = InArgs._Osada;
	NaWszystko = InArgs._NaWszystko;
	Styl = StylUI::StylPrzycisku();
	Tlo = FSlateRoundedBoxBrush(StylUI::Tlo, 6.f, StylUI::Ramka, 1.f);
	const FMargin Odstep(0.f, 0.f, 4.f, 0.f);

	ChildSlot
	[
		SNew(SVerticalBox)
		// Panel (otwarty).
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
		[
			SNew(SBorder)
			.BorderImage(&Tlo)
			.Padding(FMargin(12.f, 8.f))
			.Visibility_Lambda([]() { return FTrybDeweloperski::bOtwarty ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Tryb deweloperski")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					.ColorAndOpacity(StylUI::Zloto)
				]
				// Wszystko naraz do sprawdzenia.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					Przycisk(FText::FromString(TEXT("Postaw wszystko")),
						[this]() { if (NaWszystko) { Wynik = NaWszystko(); } },
						FSlateColor(StylUI::Zloto),
						TEXT("Obóz (jeśli go nie ma), każdy budynek gotowy, rodziny w każdej pracy, +100 surowców"))
				]
				// Galerie testowe i dziennik bitwy (24.11.2026).
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
					[
						Przycisk(FText::FromString(TEXT("Galeria budynków (F5)")),
							[this]() { if (Osada.IsValid()) { Wynik = AGaleria::Przelacz(Osada->GetWorld(), AGaleria::ERodzaj::Budynki); } },
							FSlateColor(StylUI::Napis),
							TEXT("Każdy model w każdym etapie budowy, wariancie i stanie zniszczenia, do tego pożar i zawalenie"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
					[
						Przycisk(FText::FromString(TEXT("Galeria wojska (F6)")),
							[this]() { if (Osada.IsValid()) { Wynik = AGaleria::Przelacz(Osada->GetWorld(), AGaleria::ERodzaj::Wojsko); } },
							FSlateColor(StylUI::Napis),
							TEXT("Każdy rodzaj wojska w każdej animacji walki (powtarzane), narzędzia ruszenia, kobieta"))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 8.f, 0.f)
					[
						Etykieta(TEXT("Dziennik bitwy"))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						Przycisk(TAttribute<FText>::CreateLambda([]() { return FText::FromString(ABitwa::bDziennik ? TEXT("WŁĄCZONY") : TEXT("wyłączony")); }),
							[]() { ABitwa::bDziennik = !ABitwa::bDziennik; },
							TAttribute<FSlateColor>::CreateLambda([]() { return FSlateColor(ABitwa::bDziennik ? StylUI::Zielony : StylUI::Napis); }),
							TEXT("Co sekundę stan każdego oddziału (rozkaz, ilu walczy, ilu stoi bezczynnie przy wrogu, duch) w logu [BITWA] i w Saved/Logs/Bitwa_dziennik.txt. N = bitwa próbna pod kursorem, P = napad"))
					]
				]
				// Budowa natychmiast.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
					[
						Etykieta(TEXT("Budowa natychmiast"))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						Przycisk(TAttribute<FText>::CreateLambda([]() { return FText::FromString(ABudowa::bNatychmiast ? TEXT("WŁĄCZONA") : TEXT("wyłączona")); }),
							[]() { ABudowa::bNatychmiast = !ABudowa::bNatychmiast; },
							TAttribute<FSlateColor>::CreateLambda([]() { return FSlateColor(ABudowa::bNatychmiast ? StylUI::Zielony : StylUI::Napis); }),
							TEXT("Każda budowa (też już zaczęta) od razu gotowa, bez materiału i budowniczych"))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 8.f, 0.f)
					[
						Etykieta(TEXT("Dziennik AI"))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						Przycisk(TAttribute<FText>::CreateLambda([]() { return FText::FromString(ARody::bDziennik ? TEXT("WŁĄCZONY") : TEXT("wyłączony")); }),
							[]() { ARody::bDziennik = !ARody::bDziennik; },
							TAttribute<FSlateColor>::CreateLambda([]() { return FSlateColor(ARody::bDziennik ? StylUI::Zielony : StylUI::Napis); }),
							TEXT("Linie [AI] w logu (co postanowił każdy ród i dlaczego) i plik Saved/AI_dziennik.csv"))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12.f, 0.f, 8.f, 0.f)
					[
						Etykieta(TEXT("Trudność AI"))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						Przycisk(TAttribute<FText>::CreateLambda([]() { return FText::FromString(ARody::NazwaTrudnosci(ARody::Trudnosc)); }),
							[]() { ARody::Trudnosc = (ARody::Trudnosc + 1) % 3; },
							FSlateColor(StylUI::Napis),
							TEXT("Jak mądrze decydują rody AI: łatwy, średni, trudny (bez darmowych surowców)"))
					]
				]
				// Czas gry.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
				[
					Etykieta(TEXT("Czas gry"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(Odstep) [ PrzyciskCzasu(0.f) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(Odstep) [ PrzyciskCzasu(1.f) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(Odstep) [ PrzyciskCzasu(2.f) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(Odstep) [ PrzyciskCzasu(5.f) ]
					+ SHorizontalBox::Slot().AutoWidth() [ PrzyciskCzasu(10.f) ]
				]
				// Surowce.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
				[
					Etykieta(TEXT("Wszystkie surowce i grosze"))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(Odstep)
					[
						Przycisk(FText::FromString(TEXT("+100")), [this]() { Dodaj(100); }, FSlateColor(StylUI::Napis),
							TEXT("Po 100 sztuk każdego towaru do obozu i 100 groszy"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(Odstep)
					[
						Przycisk(FText::FromString(TEXT("+1000")), [this]() { Dodaj(1000); }, FSlateColor(StylUI::Napis),
							TEXT("Po 1000 sztuk każdego towaru do obozu i 1000 groszy"))
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Visibility_Lambda([this]() { return Wynik.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
					.Text_Lambda([this]() { return FText::FromString(Wynik); })
					.WrapTextAt(300.f)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
					.ColorAndOpacity(StylUI::Zielony)
				]
			]
		]
		// Przycisk otwierajacy (zawsze widac, ze czas jest zmieniony).
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.f, 6.f, 0.f, 0.f)
		[
			Przycisk(TAttribute<FText>::CreateLambda([]()
				{
					FString T = TEXT("Tryb deweloperski (F3)");
					if (FTrybDeweloperski::bPauza || !FMath::IsNearlyEqual(FTrybDeweloperski::Predkosc, 1.f))
					{
						T += TEXT("  czas ") + FTrybDeweloperski::NapisCzasu();
					}
					if (ABudowa::bNatychmiast)
					{
						T += TEXT("  budowa natychmiast");
					}
					return FText::FromString(T);
				}),
				[]() { FTrybDeweloperski::bOtwarty = !FTrybDeweloperski::bOtwarty; },
				FSlateColor(StylUI::Zloto),
				TEXT("Kreatywny tryb do testów: czas gry, budowa natychmiast, surowce"))
		]
	];
}
