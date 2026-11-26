// Plik: Source/grarts2d/Budowy/Budowniczy.cpp
#include "Budowy/Budowniczy.h"
#include "Dev/Galeria.h"
#include "Budowy/Budowa.h"
#include "Osada/Osada.h"
#include "Osada/Rody.h"
#include "Osada/Drogi.h"
#include "UI/PasekBudowy.h"
#include "UI/OknoBudynku.h"
#include "UI/OknoRodow.h"
#include "UI/StylUI.h"
#include "UI/Ustawienia.h"
#include "UI/MenuGlowne.h"
#include "UI/PanelDeweloperski.h"
#include "Swiat/SwiatWysokosci.h"
#include "Swiat/TerenKafle.h"
#include "Kamera/KameraMapy.h"
#include "Wojna/Bitwa.h"
#include "Wojna/OknoWojska.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Gra/ZapisGry.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// Plynne pojawienie sie okna (22.11.2026): 0,15 s od przezroczystego, zamiast nagle.
static void PokazPlynnie(const TSharedPtr<SBox>& Miejsce, TSharedRef<SWidget> Zawartosc)
{
	Miejsce->SetContent(Zawartosc);
	Miejsce->SetRenderOpacity(0.f);
	const double Start = FSlateApplication::Get().GetCurrentTime();
	TWeakPtr<SBox> Slaby = Miejsce;
	Miejsce->RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateLambda([Slaby, Start](double Teraz, float)
	{
		const float A = FMath::Clamp(float((Teraz - Start) / 0.15), 0.f, 1.f);
		if (TSharedPtr<SBox> S = Slaby.Pin())
		{
			S->SetRenderOpacity(A);
		}
		return A >= 1.f ? EActiveTimerReturnType::Stop : EActiveTimerReturnType::Continue;
	}));
}

ABudowniczy::ABudowniczy()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ABudowniczy::BeginPlay()
{
	Super::BeginPlay();
	FTrybDeweloperski::Wyzeruj();
	// Sprawdzenie przy przyspieszeniu (-tempo=10): gra idzie od razu z tym tempem (jak klawisz T).
	float TempoStart = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("tempo="), TempoStart) && TempoStart > 0.f)
	{
		ABudowa::Tempo = TempoStart;
		UE_LOG(LogTemp, Log, TEXT("Budowniczy: tempo x%.0f z linii polecen"), TempoStart);
	}
	bTrybPokazu = FParse::Param(FCommandLine::Get(), TEXT("kosciol"));
	bInterfejs = !FParse::Param(FCommandLine::Get(), TEXT("zrzut")) || FParse::Param(FCommandLine::Get(), TEXT("osada")) || FParse::Param(FCommandLine::Get(), TEXT("wszystko"));
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Osada = GetWorld()->SpawnActor<AOsada>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	Rody = GetWorld()->SpawnActor<ARody>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	Bitwa = GetWorld()->SpawnActor<ABitwa>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	Drogi = GetWorld()->SpawnActor<ADrogi>(FVector::ZeroVector, FRotator::ZeroRotator, P);
	bTrybBitwy = FParse::Param(FCommandLine::Get(), TEXT("bitwa"));
	for (const FRodzajBudynku* R : FRodzajBudynku::DoBudowania())
	{
		const FString Sciezka = FString::Printf(TEXT("/Game/Swiat/Budynki/%s.%s"), *R->Ikona, *R->Ikona);
		UTexture2D* T = LoadObject<UTexture2D>(nullptr, *Sciezka);
		UE_CLOG(!T, LogTemp, Warning, TEXT("Budowniczy: brak ikony %s (uruchom Tools/Zbuduj_Budynki.bat)"), *Sciezka);
		Ikony.Add(T);
	}
	auto Wczytaj = [](const TCHAR* Nazwa)
	{
		const FString Sciezka = FString::Printf(TEXT("/Game/Swiat/Budynki/%s.%s"), Nazwa, Nazwa);
		UTexture2D* T = LoadObject<UTexture2D>(nullptr, *Sciezka);
		UE_CLOG(!T, LogTemp, Warning, TEXT("Budowniczy: brak ikony %s (uruchom Tools/Zbuduj_Budynki.bat)"), *Sciezka);
		return T;
	};
	IkonySurowcow = { Wczytaj(TEXT("T_Ikona_Kloda")), Wczytaj(TEXT("T_Ikona_Deska")), Wczytaj(TEXT("T_Ikona_Kamien")), Wczytaj(TEXT("T_Ikona_Jedzenie")),
		Wczytaj(TEXT("T_Ikona_Zboze")), Wczytaj(TEXT("T_Ikona_Maka")), Wczytaj(TEXT("T_Ikona_Welna")), Wczytaj(TEXT("T_Ikona_Sukno")),
		Wczytaj(TEXT("T_Ikona_Ruda")), Wczytaj(TEXT("T_Ikona_Bron")), Wczytaj(TEXT("T_Ikona_Opal")), Wczytaj(TEXT("T_Ikona_Ryby")),
		Wczytaj(TEXT("T_Ikona_Skory")), Wczytaj(TEXT("T_Ikona_Skora")), Wczytaj(TEXT("T_Ikona_Buty")), Wczytaj(TEXT("T_Ikona_Ubrania")),
		Wczytaj(TEXT("T_Ikona_Slod")), Wczytaj(TEXT("T_Ikona_Piwo")) };
	check(IkonySurowcow.Num() == int32(ESurowiec::Ile));
	IkonaRodziny = Wczytaj(TEXT("T_Ikona_Rodzina"));
	IkonaZadowolenia = Wczytaj(TEXT("T_Ikona_Zadowolenie"));
	IkonaGroszy = Wczytaj(TEXT("T_Ikona_Grosze"));
	ZnakDrogi = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Swiat/Drogi/T_Ikona_Znak_Droga.T_Ikona_Znak_Droga")); // Tools/Zbuduj_Drogi.bat
	IkonaGroszyB = StylUI::Ikona(IkonaGroszy, 24.f);
	for (const FString& N : SPasekBudowy::NazwyZnakow())
	{
		Znaki.Add(Wczytaj(*(TEXT("T_Ikona_Znak_") + N)));
	}
	for (int32 i = 0; i < int32(ESurowiec::Ile); ++i)
	{
		IkonySurowcowB[i] = StylUI::Ikona(IkonySurowcow[i], 26.f);
	}
	IkonaRodzinyB = StylUI::Ikona(IkonaRodziny, 26.f);
	IkonaZadowoleniaB = StylUI::Ikona(IkonaZadowolenia, 22.f);
	ZrobInterfejs();
}

void ABudowniczy::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Nakladka.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Nakladka.ToSharedRef());
	}
	Nakladka.Reset();
	Pasek.Reset();
	Okno.Reset();
	MiejsceOkna.Reset();
	OknoRodow.Reset();
	MiejsceRodow.Reset();
	OknoWojska.Reset();
	MiejsceWojska.Reset();
	Super::EndPlay(Reason);
}

void ABudowniczy::ZrobInterfejs()
{
	if (!GEngine || !GEngine->GameViewport || !bInterfejs)
	{
		return;
	}
	TArray<UTexture2D*> Tex;
	for (const TObjectPtr<UTexture2D>& T : Ikony)
	{
		Tex.Add(T.Get());
	}
	TArray<UTexture2D*> ZnakiTex;
	for (const TObjectPtr<UTexture2D>& T : Znaki)
	{
		ZnakiTex.Add(T.Get());
	}
	TloInfo = FSlateRoundedBoxBrush(FLinearColor(0.035f, 0.028f, 0.02f, 0.82f), 6.f, FLinearColor(0.42f, 0.32f, 0.18f, 1.f), 1.f);
	StylPrzyciskuRodow = StylUI::StylPrzycisku();
	auto WidacOsade = [this]() { return (bWidoczne && Osada && Osada->MaOboz()) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; };
	Nakladka = SNew(SOverlay)
		.Visibility(EVisibility::SelfHitTestInvisible)
		// Mapa papierowa: choragiewki rodow i wasza osada (etap 5).
		+ SOverlay::Slot()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return (!bWidoczne && Rody && Rody->Zalozone()) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(SZnakiRodow).Rody(Rody.Get()).Osada(Osada.Get())
			]
		]
		// Podpowiedz klawiszy: lewy dolny rog.
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(20.f)
		[
			SAssignNew(Pomoc, STextBlock)
			.Visibility(EVisibility::HitTestInvisible)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
			.ColorAndOpacity(FLinearColor(1.f, 0.95f, 0.8f))
			.ShadowOffset(FVector2D(1.f, 1.f))
		]
		// Pasek budowy: dol, srodek.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Bottom)
		.Padding(0.f, 0.f, 0.f, 14.f)
		[
			SNew(SBox)
			.Visibility_Lambda([this, WidacOsade]() { return (Bitwa && bTrybBitwy) ? EVisibility::Collapsed : WidacOsade(); }) // budowanie trwa tez w czasie walki
			[
				SNew(SVerticalBox)
				// Pole i pastwisko: panel zaznaczonego prostokata (wielkosc, plon / stado, koszt; Zbuduj / Anuluj).
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 8.f)
				[
					PanelObszaru()
				]
				// Dolny pasek jak w Manor Lords: glowne przyciski (Budowa, Wojsko, Rody), kategorie i karty budynkow.
				+ SVerticalBox::Slot().AutoHeight()
				[
					SAssignNew(Pasek, SPasekBudowy)
					.Ikony(Tex)
					.Znaki(ZnakiTex)
					.Wybrany_Lambda([this]() { return Wybrany; })
					.Blokada_Lambda([this](const FRodzajBudynku& R) { return Osada ? Osada->CzemuZablokowany(R) : FString(); })
					.NaWybor_Lambda([this](const FRodzajBudynku* R) { Wybierz(R); })
					.NaWojsko_Lambda([this]() { OknoWojska.IsValid() ? ZamknijWojsko() : OtworzWojsko(); })
					.NaRody_Lambda([this]() { OknoRodow.IsValid() ? ZamknijRody() : OtworzRody(); })
					.ZnakDrogi(ZnakDrogi)
					.NaDrogi_Lambda([this]()
					{
						if (Drogi)
						{
							const bool bWlacz = !Drogi->Rysuje();
							Wybierz(nullptr);
							Drogi->Wlacz(bWlacz);
							UstawPomoc();
						}
					})
					.DrogiOtwarte_Lambda([this]() { return Drogi && Drogi->Rysuje(); })
					.WojskoOtwarte_Lambda([this]() { return OknoWojska.IsValid(); })
					.RodyOtwarte_Lambda([this]() { return OknoRodow.IsValid(); })
					.AlarmWojska_Lambda([this]() { return FString(Bitwa && Bitwa->Walka() ? TEXT("Bandyci napadają osadę!") : TEXT("")); })
					.AlarmRodow_Lambda([this]()
					{
						// Zadanie daniny czeka na odpowiedz: widac to na przycisku.
						bool bZada = false;
						if (Rody)
						{
							for (const FRod& R : Rody->Lista())
							{
								bZada |= R.ZadaDaniny > 0;
							}
						}
						return FString(bZada ? TEXT("Ród żąda daniny!") : TEXT(""));
					})
				]
			]
		]
		// Panel osady: lewy gorny rog; pod nim przycisk i okno rodow (etap 5).
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(16.f)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([this]() { return (bWidoczne && Osada && Osada->MaOboz()) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
			[
				SNew(SBorder)
				.BorderImage(&TloInfo)
				.Padding(FMargin(12.f, 6.f))
				.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&) { bRozwoj = !bRozwoj; DoRozwoju = 0.f; return FReply::Handled(); })
				.ToolTipText(FText::FromString(TEXT("Kliknij (albo O): okno rozwoju osady - co pilne, co dalej, warunki awansu.")))
				[
					SAssignNew(Info, STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
					.ColorAndOpacity(FLinearColor(0.95f, 0.90f, 0.78f))
				]
			]
			// P7: okno rozwoju osady.
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(&TloInfo)
				.Padding(FMargin(14.f, 10.f))
				.Visibility_Lambda([this]() { return bRozwoj ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				[
					SNew(SBox)
					.MaxDesiredWidth(560.f)
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Text_Lambda([this]() { return FText::FromString(TekstRozwoju); })
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
						.ColorAndOpacity(FLinearColor(0.95f, 0.90f, 0.78f))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, 6.f, 0.f, 0.f)
			[
				SAssignNew(MiejsceRodow, SBox)
				.Visibility_Lambda([this]() { return OknoRodow.IsValid() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
			]
			// Wojsko (jak zakladka wojska w Manor Lords): oddzialy pospolitego ruszenia. Przycisk okna - na dolnym pasku (J).
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0.f, 6.f, 0.f, 0.f)
			[
				SAssignNew(MiejsceWojska, SBox)
				.Visibility_Lambda([this]() { return OknoWojska.IsValid() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
			]
		]
		// Licznik surowcow: gora, srodek.
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		.Padding(0.f, 12.f, 0.f, 0.f)
		[
			SNew(SVerticalBox)
			.Visibility_Lambda([this]() { return (bWidoczne && Osada && Osada->MaOboz()) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
			[
				LicznikSurowcow()
			]
			// Wiadomosci osady (przybyla / odeszla rodzina, brak jedzenia): pod licznikiem, znikaja po 12 s.
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(&TloInfo)
				.Padding(FMargin(10.f, 3.f))
				.Visibility_Lambda([this]() { return (Osada && !Osada->Wiadomosc().IsEmpty()) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return FText::FromString(Osada ? Osada->Wiadomosc() : FString()); })
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
					.ColorAndOpacity(StylUI::Zloto)
				]
			]
		]
		// Tryb deweloperski: prawy dolny rog (UI/PanelDeweloperski).
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(16.f)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return bWidoczne ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
			[
				SNew(SPanelDeweloperski).Osada(Osada.Get()).NaWszystko([this]() { return PostawWszystko(); })
			]
		]
		// Okno budynku: prawy gorny rog.
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(16.f, 70.f, 16.f, 16.f)
		[
			SAssignNew(MiejsceOkna, SBox)
			.Visibility_Lambda([this]() { return (bWidoczne && Okno.IsValid()) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
		];
	GEngine->GameViewport->AddViewportWidgetContent(Nakladka.ToSharedRef(), 50);
	UstawPomoc();
}

TSharedRef<SWidget> ABudowniczy::LicznikSurowcow()
{
	TSharedRef<SHorizontalBox> H = SNew(SHorizontalBox);
	for (ESurowiec S : WszystkieSurowce)
	{
		H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 3.f, 0.f)
		[
			SNew(SBox).WidthOverride(26.f).HeightOverride(26.f)
			.ToolTipText(FText::FromString(NazwaSurowca(S)))
			[
				SNew(SImage).Image(&IkonySurowcowB[int32(S)])
			]
		];
		H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 7.f, 0.f)
		[
			SNew(STextBlock)
			.Text_Lambda([this, S]() { return FText::AsNumber(Osada ? Osada->Zapasy()[S] : 0); })
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
			.ColorAndOpacity_Lambda([this, S]()
			{
				// Brak: przygaszony; jedzenie na mniej niz jeden posilek (porcja na rodzine): czerwone.
				const int32 N = Osada ? Osada->Zapasy()[S] : 0;
				if (S == ESurowiec::Jedzenie && Osada && N < Osada->Rodzin())
				{
					return FSlateColor(StylUI::Czerwony);
				}
				return FSlateColor(N > 0 ? StylUI::Napis : StylUI::Szary * 0.8f);
			})
		];
	}
	// Zadowolenie: serce i procent (zielony od 60% - przychodza rodziny, czerwony ponizej 30% - odchodza).
	TSharedRef<SHorizontalBox> Zad = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
		[
			SNew(SBox).WidthOverride(22.f).HeightOverride(22.f)[SNew(SImage).Image(&IkonaZadowoleniaB)]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%d%%"), Osada ? Osada->Zadowolenie() : 0)); })
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
			.ColorAndOpacity_Lambda([this]()
			{
				const int32 Z = Osada ? Osada->Zadowolenie() : 0;
				return FSlateColor(Z >= 60 ? StylUI::Zielony : (Z < 30 ? StylUI::Czerwony : StylUI::Zloto));
			})
		];
	H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 10.f, 0.f)
	[
		SNew(SBox)
		.ToolTipText_Lambda([this]() { return FText::FromString(Osada ? Osada->OpisZadowolenia() : FString()); })
		[Zad]
	];
	H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f, 4.f, 0.f)
	[
		SNew(SBox).WidthOverride(26.f).HeightOverride(26.f)
		[
			SNew(SImage).Image(&IkonaRodzinyB)
		]
	];
	H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
	[
		SNew(STextBlock)
		.Text_Lambda([this]()
		{
			return FText::FromString(Osada ? FString::Printf(TEXT("%d / %d bez pracy"), Osada->WolnychRodzin(), Osada->Rodzin()) : FString());
		})
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		.ColorAndOpacity(StylUI::Napis)
	];
	// Woje (etap 4): mezowie z rodzin w zbrojowniach, z wlocznia; ikona: stos broni.
	H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f, 4.f, 0.f)
	[
		SNew(SBox).WidthOverride(26.f).HeightOverride(26.f)
		.ToolTipText(FText::FromString(TEXT("Woje: mężczyźni z rodzin przydzielonych do zbrojowni, z włócznią z kuźni")))
		[
			SNew(SImage).Image(&IkonySurowcowB[int32(ESurowiec::Bron)])
		]
	];
	H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
	[
		SNew(STextBlock)
		.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("Woje: %d"), Osada ? Osada->Wojow() : 0)); })
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		.ColorAndOpacity(StylUI::Napis)
	];
	// Grosze (etap 5): skarbiec osady - handel na targu, daniny, podarunki.
	H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f, 4.f, 0.f)
	[
		SNew(SBox).WidthOverride(24.f).HeightOverride(24.f)
		.ToolTipText(FText::FromString(TEXT("Grosze: skarbiec osady (handel z rodami na targu, daniny, podarunki)")))
		[
			SNew(SImage).Image(&IkonaGroszyB)
		]
	];
	H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
	[
		SNew(STextBlock)
		.Text_Lambda([this]() { return FText::AsNumber(Osada ? Osada->Grosze : 0); })
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
		.ColorAndOpacity(StylUI::Zloto)
	];
	// Podatek i dziesiecina (paczka A, 23.11.2026): klik zmienia stawke / wlacza dziesiecine.
	H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
	[
		SNew(SButton)
		.ButtonStyle(&StylPrzyciskuRodow)
		.ContentPadding(FMargin(6.f, 1.f))
		.ToolTipText_Lambda([this]() { return FText::FromString(Osada ? Osada->OpisPodatkow() : FString()); })
		.OnClicked_Lambda([this]() { if (Osada) { Osada->ZmienPodatek(); } return FReply::Handled(); })
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("Podatek: %s"), AOsada::NazwaPodatku(Osada ? Osada->Podatek : 1))); })
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(StylUI::Zloto)
		]
	];
	H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
	[
		SNew(SButton)
		.ButtonStyle(&StylPrzyciskuRodow)
		.ContentPadding(FMargin(6.f, 1.f))
		.ToolTipText_Lambda([this]() { return FText::FromString(Osada ? Osada->OpisPodatkow() : FString()); })
		.OnClicked_Lambda([this]() { if (Osada) { Osada->bDziesiecina = !Osada->bDziesiecina; } return FReply::Handled(); })
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return FText::FromString(Osada && Osada->bDziesiecina ? TEXT("Dziesięcina: tak") : TEXT("Dziesięcina: nie")); })
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
			.ColorAndOpacity(StylUI::Zloto)
		]
	];
	// Pora roku (etap 3): "Jesień, rok 1"; podpowiedz - co sie teraz dzieje i ile do nastepnej pory.
	H->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(10.f, 0.f, 8.f, 0.f)
	[
		SNew(SBox)
		.ToolTipText_Lambda([this]() { return FText::FromString(Osada ? Osada->OpisPory() : FString()); })
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return FText::FromString(Osada ? Osada->NapisPory() : FString()); })
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			.ColorAndOpacity_Lambda([this]()
			{
				static const FLinearColor Kolory[] = { FLinearColor(0.62f, 0.82f, 0.45f), FLinearColor(0.95f, 0.85f, 0.45f),
					FLinearColor(0.92f, 0.62f, 0.30f), FLinearColor(0.75f, 0.85f, 0.95f) };
				return FSlateColor(Kolory[int32(Osada ? Osada->Pora() : EPora::Wiosna)]);
			})
		]
	];
	return SNew(SBorder)
		.BorderImage(&TloInfo)
		.Padding(FMargin(8.f, 5.f))
		[H];
}

UTexture2D* ABudowniczy::IkonaDla(const FRodzajBudynku& R) const
{
	const TArray<const FRodzajBudynku*>& L = FRodzajBudynku::DoBudowania();
	for (int32 i = 0; i < L.Num(); ++i)
	{
		if (L[i]->Model == R.Model && Ikony.IsValidIndex(i))
		{
			return Ikony[i].Get();
		}
	}
	// Wyzsze poziomy domu i kosciola (22.11.2026) nie sa na pasku: ikona z wlasnej nazwy.
	return R.Ikona.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Swiat/Budynki/%s.%s"), *R.Ikona, *R.Ikona));
}

void ABudowniczy::OtworzOkno(ABudowa* B)
{
	if (!MiejsceOkna.IsValid() || !B)
	{
		return;
	}
	if (Okno.IsValid() && Okno->Budynek() == B)
	{
		return;
	}
	TArray<UTexture2D*> Sur;
	for (const TObjectPtr<UTexture2D>& T : IkonySurowcow)
	{
		Sur.Add(T.Get());
	}
	SAssignNew(Okno, SOknoBudynku)
		.Osada(Osada.Get())
		.Budynek(B)
		.Ikona(IkonaDla(B->Rodzaj()))
		.IkonySurowcow(Sur)
		.IkonaRodziny(IkonaRodziny.Get())
		.NaZamknij_Lambda([this]() { ZamknijOkno(); });
	PokazPlynnie(MiejsceOkna, Okno.ToSharedRef());
}

void ABudowniczy::ZamknijOkno()
{
	if (MiejsceOkna.IsValid())
	{
		MiejsceOkna->SetContent(SNullWidget::NullWidget);
	}
	Okno.Reset();
}

void ABudowniczy::OtworzRody(int32 IndeksRodu)
{
	if (!MiejsceRodow.IsValid() || !Rody)
	{
		return;
	}
	if (!OknoRodow.IsValid())
	{
		ZamknijWojsko();
		TArray<UTexture2D*> Sur;
		for (const TObjectPtr<UTexture2D>& T : IkonySurowcow)
		{
			Sur.Add(T.Get());
		}
		SAssignNew(OknoRodow, SOknoRodow)
			.Rody(Rody.Get())
			.Osada(Osada.Get())
			.IkonySurowcow(Sur)
			.IkonaGroszy(IkonaGroszy.Get())
			.NaZamknij_Lambda([this]() { ZamknijRody(); });
		PokazPlynnie(MiejsceRodow, OknoRodow.ToSharedRef());
		// Na poczatek rod, ktory zada daniny, a jesli zaden - najblizszy.
		int32 Naj = 0;
		for (int32 i = 0; i < Rody->Lista().Num(); ++i)
		{
			const FRod& R = Rody->Lista()[i];
			const FRod& N = Rody->Lista()[Naj];
			if ((R.ZadaDaniny > 0) != (N.ZadaDaniny > 0) ? R.ZadaDaniny > 0 : R.OdlegloscKm < N.OdlegloscKm)
			{
				Naj = i;
			}
		}
		OknoRodow->Wybierz(Naj);
	}
	if (IndeksRodu >= 0)
	{
		OknoRodow->Wybierz(IndeksRodu);
	}
}

void ABudowniczy::OtworzWojsko()
{
	if (!MiejsceWojska.IsValid() || !Bitwa || OknoWojska.IsValid())
	{
		return;
	}
	ZamknijRody(); // jedno okno pod panelem osady naraz
	SAssignNew(OknoWojska, SOknoWojska)
		.Bitwa(Bitwa.Get())
		.NaZamknij_Lambda([this]() { ZamknijWojsko(); });
	PokazPlynnie(MiejsceWojska, OknoWojska.ToSharedRef());
}

void ABudowniczy::ZamknijWojsko()
{
	if (MiejsceWojska.IsValid())
	{
		MiejsceWojska->SetContent(SNullWidget::NullWidget);
	}
	OknoWojska.Reset();
}

void ABudowniczy::ZamknijRody()
{
	if (MiejsceRodow.IsValid())
	{
		MiejsceRodow->SetContent(SNullWidget::NullWidget);
	}
	OknoRodow.Reset();
}

void ABudowniczy::UstawPomoc()
{
	if (!Pomoc.IsValid())
	{
		return;
	}
	FString T;
	if (!Teren.IsValid() || !Teren->WRegionie())
	{
		T = TEXT("Przybliż mapę, żeby budować");
	}
	else if (Bitwa && bTrybBitwy)
	{
		T = FString(); // rozkazy bitwy sa w panelu bitwy
	}
	else if (Osada && !Osada->MaOboz() && !bTrybPokazu)
	{
		T = TEXT("Wybierz miejsce na obóz osadników: lewy klik (zielony = można)");
	}
	else if (Wybrany && Wybrany->ZObszarem())
	{
		const int32 Od = FMath::RoundToInt32(Wybrany->ObszarMin.X * Wybrany->ObszarMin.Y / 1e4f), Do = FMath::RoundToInt32(Wybrany->ObszarMax.X * Wybrany->ObszarMax.Y / 1e4f);
		T = KrokObszaru == 2 ? FString::Printf(TEXT("%s: Enter albo Zbuduj - postaw, prawy klik - popraw, Esc - anuluj"), *Wybrany->Nazwa)
			: FString::Printf(TEXT("%s: klikaj kolejne rogi płotu (dowolny kształt, %d–%d m²), zamknij klikiem w pierwszy punkt albo Enter\nPrawy klik / Backspace - cofnij punkt, Esc - anuluj. Brama staje na najdłuższym boku%s"),
				*Wybrany->Nazwa, Od, Do, Wybrany->Funkcja == EFunkcja::Owczarnia ? TEXT(", szopa sama w środku") : TEXT(""));
		if (!BladObszaru.IsEmpty() && KrokObszaru != 2)
		{
			T += TEXT("\n") + BladObszaru;
		}
	}
	else if (Drogi && Drogi->Rysuje())
	{
		T = Drogi->Pomoc();
	}
	else if (Wybrany && Wybrany->bLinia)
	{
		T = FString::Printf(TEXT("%s: przytrzymaj lewy przycisk i przeciągnij linię, puść - budowa (zielone odcinki), R - obróć, prawy klik / Esc - koniec"), *Wybrany->Nazwa);
	}
	else if (Wybrany)
	{
		T = FString::Printf(TEXT("%s: lewy klik - postaw, R - obróć (przytrzymaj: płynnie, F - w drugą stronę), Shift - kilka, prawy klik / Esc - anuluj\nPrzy drodze budynek sam staje frontem do niej (Alt - bez przyciągania)"), *Wybrany->Nazwa);
	}
	else
	{
		T = FString::Printf(TEXT("Klik w budynek - okno budynku, w trawę - zamyka okna\nB - budowa     G - drogi     H - rody i handel     J - wojsko     T - tempo x%d\nN - bitwa próbna     P - napad bandytów     F3 - deweloper"), FMath::RoundToInt32(ABudowa::Tempo));
	}
	Pomoc->SetText(FText::FromString(T));
}

void ABudowniczy::UstawInfo()
{
	if (!Info.IsValid() || !Osada)
	{
		return;
	}
	// W trzech wierszach: licznik surowcow u gory (etap 4: 10 surowcow i woje) zachodzil na jeden dlugi wiersz.
	// Paczka A: poziom osady na gorze (podpowiedz: warunki awansu).
	Info->SetText(FText::FromString(FString::Printf(TEXT("%s\nOsadnicy: %d (wolni: %d)\nRodziny: %d (w namiotach: %d)\nBudowy: %d"),
		*Osada->NapisPoziomu(), Osada->Osadnikow(), Osada->Wolnych(), Osada->Rodzin(), Osada->WNamiotach(), Osada->Budow())));
}

bool ABudowniczy::TerenPodKursorem(APlayerController* PC, FVector& Out) const
{
	FVector Pocz, Kier;
	if (!PC->DeprojectMousePositionToWorld(Pocz, Kier) || Kier.Z > -0.01)
	{
		return false;
	}
	// Krokami wzdluz promienia az pod teren, potem polowienie. Kamera regionu jest najwyzej ~1,5 km od ogniska.
	const double Krok = 1500.0;
	double T0 = 0.0;
	for (int32 i = 0; i < 400; ++i)
	{
		const double T1 = T0 + Krok;
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

bool ABudowniczy::MoznaPostawic(const FRodzajBudynku& R, FVector2D Gdzie, float ObrotSt, float& Z) const
{
	if (R.ZObszarem())
	{
		return MoznaObszar(R, Gdzie, ObrotSt, Z);
	}
	// Teren w 15 punktach obrysu: bez wody i rzek, roznica wysokosci najwyzej MaxSpadek.
	const float S = FMath::Sin(FMath::DegreesToRadians(ObrotSt)), C = FMath::Cos(FMath::DegreesToRadians(ObrotSt));
	float Min = TNumericLimits<float>::Max(), Max = -TNumericLimits<float>::Max();
	bool bOk = true;
	for (int32 i = -2; i <= 2; ++i)
	{
		for (int32 j = -1; j <= 1; ++j)
		{
			const float Lx = R.Polowa.X * i / 2.f, Ly = R.Polowa.Y * j;
			const double X = Gdzie.X + Lx * C - Ly * S, Y = Gdzie.Y + Lx * S + Ly * C;
			float OdRzeki = 1e5f;
			const float H = FSwiat::Wysokosc(X, Y, &OdRzeki);
			Min = FMath::Min(Min, H);
			Max = FMath::Max(Max, H);
			if (H <= 0.f || OdRzeki < 400.f || FSwiat::Woda(X, Y) != FSwiat::EWoda::Lad)
			{
				bOk = false;
			}
		}
	}
	// Tartak wodny: koniec koryta mlynowki (R.Woda) musi siegac rzeki (do 6 m od jej srodka).
	if (R.bNadRzeka)
	{
		const double X = Gdzie.X + R.Woda.X * C - R.Woda.Y * S, Y = Gdzie.Y + R.Woda.X * S + R.Woda.Y * C;
		float OdRzeki = 1e5f;
		FSwiat::Wysokosc(X, Y, &OdRzeki);
		if (OdRzeki > 600.f)
		{
			bOk = false;
		}
	}
	// Budynek stoi na najwyzszym punkcie (podloga nad trawa), cokol schodzi w dol po nizszej stronie.
	Z = Max - 20.f;
	if (Max - Min > MaxSpadek)
	{
		bOk = false;
	}
	if (Osada && Osada->Koliduje(R, Gdzie, ObrotSt))
	{
		bOk = false;
	}
	if (Drogi && Drogi->PrzecinaBudynek(R.NajwyzszyPoziom(), Gdzie, ObrotSt))
	{
		bOk = false; // budynek nie staje na drodze (26.11.2026: obrys po pelnym ulepszeniu)
	}
	return bOk;
}

bool ABudowniczy::MoznaObszar(const FRodzajBudynku& R, FVector2D Gdzie, float ObrotSt, float& Z) const
{
	// Teren w siatce punktow co ok. 4 m: bez wody i rzek; spadek do MaxSpadek na kazde 20 m dluzszego boku
	// (plot i kafle uprawy ida za terenem). Szopa owczarni (BudynekMin..Max) jak zwykly budynek.
	const double Rad = FMath::DegreesToRadians(double(ObrotSt));
	const double Sn = FMath::Sin(Rad), Cs = FMath::Cos(Rad);
	bool bOk = true;
	auto Zakres = [&](FVector2D A, FVector2D B, int32 N, float& Mn, float& Mx)
	{
		Mn = TNumericLimits<float>::Max();
		Mx = -TNumericLimits<float>::Max();
		for (int32 i = 0; i <= N; ++i)
		{
			for (int32 j = 0; j <= N; ++j)
			{
				const double Lx = FMath::Lerp(A.X, B.X, double(i) / N), Ly = FMath::Lerp(A.Y, B.Y, double(j) / N);
				const double X = Gdzie.X + Lx * Cs - Ly * Sn, Y = Gdzie.Y + Lx * Sn + Ly * Cs;
				float OdRzeki = 1e5f;
				const float H = FSwiat::Wysokosc(X, Y, &OdRzeki);
				Mn = FMath::Min(Mn, H);
				Mx = FMath::Max(Mx, H);
				if (H <= 0.f || OdRzeki < 400.f || FSwiat::Woda(X, Y) != FSwiat::EWoda::Lad)
				{
					bOk = false;
				}
			}
		}
	};
	const FBox2D O = R.Obrys();
	const double Bok = FMath::Max(O.GetSize().X, O.GetSize().Y);
	float Mn = 0.f, Mx = 0.f;
	Zakres(O.Min, O.Max, FMath::Clamp(FMath::CeilToInt32(Bok / 400.0), 2, 16), Mn, Mx);
	if (Mx - Mn > MaxSpadek * FMath::Max(1.0, Bok / 2000.0))
	{
		bOk = false;
	}
	Z = FSwiat::Wysokosc(Gdzie.X, Gdzie.Y);
	if (R.BudynekMax.X > R.BudynekMin.X)
	{
		Zakres(R.BudynekMin, R.BudynekMax, 2, Mn, Mx);
		Z = Mx - 20.f; // szopa na najwyzszym punkcie, jak zwykly budynek
		if (Mx - Mn > MaxSpadek)
		{
			bOk = false;
		}
	}
	if (Osada && Osada->Koliduje(R, Gdzie, ObrotSt))
	{
		bOk = false;
	}
	if (Drogi && Drogi->PrzecinaBudynek(R, Gdzie, ObrotSt))
	{
		bOk = false;
	}
	return bOk;
}

bool ABudowniczy::MoznaOboz(FVector2D Srodek, TArray<float>& Z) const
{
	TArray<FVector> Miejsca;
	AOsada::MiejscaObozu(Srodek, Miejsca);
	Z.SetNum(Miejsca.Num());
	bool bOk = true;
	for (int32 k = 0; k < Miejsca.Num(); ++k)
	{
		const FRodzajBudynku& R = k + 1 < Miejsca.Num() ? FRodzajBudynku::Namiot() : FRodzajBudynku::Ognisko();
		bOk &= MoznaPostawic(R, FVector2D(Miejsca[k]), Miejsca[k].Z, Z[k]);
	}
	return bOk;
}

void ABudowniczy::Wybierz(const FRodzajBudynku* R)
{
	Wybrany = R;
	UsunPodgladLinii();
	KrokObszaru = 0;
	PunktyObszaru.Reset();
	BladObszaru.Empty();
	OstatniKursor = FVector2D(1e9);
	if (LiniaObszaru)
	{
		LiniaObszaru->ClearAllMeshSections();
	}
	Szkic.Reset();
	SzkicPoz = FVector(1e9);
	if (Podglad.IsValid())
	{
		Podglad->Destroy();
		Podglad.Reset();
	}
	if (R)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Podglad = GetWorld()->SpawnActor<ABudowa>(FVector::ZeroVector, FRotator::ZeroRotator, P);
		if (Podglad.IsValid())
		{
			Podglad->Ustaw(*R, true);
			Podglad->SetActorHiddenInGame(true);
		}
	}
	UstawPomoc();
}

void ABudowniczy::PokazPodgladObozu(bool bPokaz)
{
	if (!bPokaz)
	{
		for (const TWeakObjectPtr<ABudowa>& B : PodgladObozu)
		{
			if (B.IsValid())
			{
				B->Destroy();
			}
		}
		PodgladObozu.Reset();
		return;
	}
	if (PodgladObozu.Num())
	{
		return;
	}
	for (int32 k = 0; k < 6; ++k)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ABudowa* B = GetWorld()->SpawnActor<ABudowa>(FVector::ZeroVector, FRotator::ZeroRotator, P);
		if (B)
		{
			B->Ustaw(k < 5 ? FRodzajBudynku::Namiot() : FRodzajBudynku::Ognisko(), true);
			B->SetActorHiddenInGame(true);
		}
		PodgladObozu.Add(B);
	}
}

void ABudowniczy::Pokaz()
{
	// -kosciol: 5 kosciolow w kolejnych fazach w rzedzie poziomo na ekranie (wzdluz +Y = wschod), nawa wzdluz rzedu,
	// plac z materialem od strony kamery. Szukamy wolnego miejsca po spirali od punktu startu i przenosimy tam kamere.
	const FRodzajBudynku& R = FRodzajBudynku::Kosciol();
	const float ObrotPokazu = 90.f; // wieza z lewej, plac z materialem od strony kamery (przy -90 bylo odwrotnie)
	const float Odstep = 3400.f;
	const FVector2D S = FSwiat::Start();
	for (int32 Proba = 0; Proba < 600; ++Proba)
	{
		const int32 Pierscien = (Proba + 7) / 8;
		const float Kat = (Proba % 8) * 45.f;
		const FVector2D Srodek = S + FVector2D(FMath::Cos(FMath::DegreesToRadians(Kat)), FMath::Sin(FMath::DegreesToRadians(Kat))) * 8000.f * Pierscien;
		TArray<FVector> Miejsca;
		for (int32 k = 0; k < 5; ++k)
		{
			const FVector2D G = Srodek + FVector2D(0.f, (k - 2) * Odstep);
			float Z;
			if (!MoznaPostawic(R, G, ObrotPokazu, Z))
			{
				break;
			}
			Miejsca.Add(FVector(G, Z));
		}
		if (Miejsca.Num() < 5)
		{
			continue;
		}
		for (int32 k = 0; k < 5; ++k)
		{
			if (ABudowa* B = Osada->Postaw(R, FVector2D(Miejsca[k]), ObrotPokazu, Miejsca[k].Z))
			{
				B->UstawPostep(k == 4 ? 1.f : (k + 0.5f) / 4.f);
				B->Dostarczone = R.Koszt; // pokaz bez gospodarki: material na miejscu
				if (k < 4)
				{
					Osada->WyslijLudzi(B, true); // od razu przy pracy
				}
			}
		}
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AKameraMapy* Kam = Cast<AKameraMapy>(PC->GetPawn()))
			{
				Kam->UstawOgnisko(Srodek);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("Budowniczy: pokaz faz kosciola przy (%.0f, %.0f)"), Srodek.X, Srodek.Y);
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Budowniczy: nie znaleziono miejsca na pokaz faz kosciola"));
}

void ABudowniczy::PokazPoziomow()
{
	// -poziomy: domy w trzech rzedach (z tylu poziom 3), w kazdym 4 warianty; obok przebudowy w toku (poziom 2 i 3);
	// z prawej kosciol drewniany i kamienny, za nimi kosciol z placem. Wyzsze poziomy powstaja przez AOsada::Ulepsz
	// (ta sama droga co przycisk "Ulepsz"). Budynki frontem do kamery (obrot 90 st.: front = -X swiata).
	if (!Osada)
	{
		return;
	}
	// Typ: 0 dom, 1 kosciol, 2 chata drwala, 3 chata borowego (drwale i borowi w dwoch rzedach za koscolami).
	struct FMiejsce { int32 Poziom; int32 Typ; int32 Wariant; FVector2D Off; float Postep; };
	auto Pierwszy = [](int32 T) -> const FRodzajBudynku& {
		return T == 1 ? FRodzajBudynku::KosciolDrewniany() : T == 2 ? FRodzajBudynku::ChataDrwala() : T == 3 ? FRodzajBudynku::ChataBorowego() : FRodzajBudynku::Chata(); };
	auto Najwiekszy = [](int32 T, int32 P) -> const FRodzajBudynku& {
		return T == 1 ? (P == 3 ? FRodzajBudynku::KosciolWielki() : FRodzajBudynku::Kosciol()) : T == 2 ? FRodzajBudynku::ChataDrwala3()
			: T == 3 ? FRodzajBudynku::ChataBorowego3() : FRodzajBudynku::DomDuzy(); };
	TArray<FMiejsce> M;
	for (int32 P = 1; P <= 3; ++P)
	{
		M.Add({ P, 2, 0, FVector2D(-1800.f + (P - 1) * 1700.f, 7400.f), 1.f });
		M.Add({ P, 3, 0, FVector2D(-1800.f + (P - 1) * 1700.f, 9000.f), 1.f });
	}
	for (int32 P = 1; P <= 3; ++P)
	{
		for (int32 W = 0; W < 4; ++W)
		{
			M.Add({ P, false, W, FVector2D(-1800.f + (P - 1) * 1700.f, -7500.f + W * 1500.f), 1.f });
		}
	}
	M.Add({ 2, false, 1, FVector2D(-100.f, -1300.f), 0.25f });  // przebudowa chaty na dom pruski (stara chata w rusztowaniu)
	M.Add({ 3, false, 2, FVector2D(1600.f, -1300.f), 0.6f });   // przebudowa domu pruskiego na duzy (prawie gotowy)
	M.Add({ 1, true, 0, FVector2D(-1800.f, 1900.f), 1.f });
	M.Add({ 2, true, 0, FVector2D(-1800.f, 4900.f), 1.f });
	M.Add({ 3, true, 0, FVector2D(1500.f, 3500.f), 1.f });
	const float ObrotPokazu = 90.f;
	const FVector2D S = FSwiat::Start();
	FVector2D Srodek = S;
	for (int32 Proba = 0; Proba < 400; ++Proba)
	{
		const int32 Pierscien = (Proba + 7) / 8;
		const float Kat = (Proba % 8) * 45.f;
		const FVector2D C = S + FVector2D(FMath::Cos(FMath::DegreesToRadians(Kat)), FMath::Sin(FMath::DegreesToRadians(Kat))) * 6000.f * Pierscien;
		int32 Dobre = 0;
		for (const FMiejsce& D : M)
		{
			const FRodzajBudynku& R = Najwiekszy(D.Typ, D.Poziom);
			float Zd;
			Dobre += MoznaPostawic(R, C + D.Off, ObrotPokazu, Zd) ? 1 : 0;
		}
		if (Dobre == M.Num())
		{
			Srodek = C;
			break;
		}
	}
	int32 Ile = 0;
	for (const FMiejsce& D : M)
	{
		const FRodzajBudynku& R1 = Pierwszy(D.Typ);
		float Z = 0.f;
		MoznaPostawic(Najwiekszy(D.Typ, 3), Srodek + D.Off, ObrotPokazu, Z);
		ABudowa::NastepnyWariant = D.Wariant;
		ABudowa* B = Osada->Postaw(R1, Srodek + D.Off, ObrotPokazu, Z);
		if (!B)
		{
			continue;
		}
		B->UstawPostep(1.f);
		for (int32 P = 1; P < D.Poziom; ++P)
		{
			if (!Osada->Ulepsz(B))
			{
				UE_LOG(LogTemp, Warning, TEXT("Budowniczy: pokaz poziomow - nie ulepszono %s: %s"), *B->Rodzaj().Nazwa, *Osada->CzemuNieUlepsz(B));
				break;
			}
			B->UstawPostep(P + 1 == D.Poziom ? D.Postep : 1.f);
		}
		++Ile;
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AKameraMapy* Kam = Cast<AKameraMapy>(PC->GetPawn()))
		{
			Kam->UstawOgnisko(Srodek + FVector2D(0.f, FParse::Param(FCommandLine::Get(), TEXT("pilarnie")) ? 8200.f : -1200.f));
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Budowniczy: pokaz poziomow, %d budynkow przy (%.0f, %.0f)"), Ile, Srodek.X, Srodek.Y);
}

void ABudowniczy::PokazOsady()
{
	// -osada: oboz, za nim (dalej od kamery) domy frontem do kamery: chata gotowa (rodzina juz w niej, jej namiot
	// zniknal), dom pruski w budowie; po prawej chata drwala i pilarnia z rodzinami przy pracy, po lewej plac skladowy
	// z zapasem. Etap 2: dalej za domami kamieniolom (2 rodziny) i spichlerz z jedzeniem, z przodu po lewej chata
	// zbieracza. Etap 3: wczesna jesien, za kamieniolomem pole z dojrzalym zbozem (rodzina przy zniwach), z boku mlyn
	// (rodzina miele), z przodu piekarnia. Etap 4: przed obozem (od strony kamery) zbrojownia z trzema wojami przy
	// manekinach, kuznia, owczarnia ze stadem, dalej chata tkacza i kopalnia rudy (siedem rodzin wiecej, bez domow);
	// kamera na nich, okno zbrojowni otwarte, pasek budowy na zakladce Wojsko.
	struct FDom { const FRodzajBudynku* R; FVector2D Off; float Postep; };
	const FDom Domy[] = {
		{ &FRodzajBudynku::Chata(), FVector2D(2300.f, -1900.f), 1.f },
		{ &FRodzajBudynku::DomPruski(), FVector2D(2300.f, 0.f), 0.4f },
		{ &FRodzajBudynku::ChataDrwala(), FVector2D(2400.f, 2000.f), 1.f },
		{ &FRodzajBudynku::Pilarnia(), FVector2D(300.f, 2700.f), 1.f },
		{ &FRodzajBudynku::PlacSkladowy(), FVector2D(300.f, -2800.f), 1.f },
		{ &FRodzajBudynku::Kamieniolom(), FVector2D(4300.f, -1300.f), 1.f },
		{ &FRodzajBudynku::Spichlerz(), FVector2D(4300.f, 1300.f), 1.f },
		{ &FRodzajBudynku::ChataZbieracza(), FVector2D(-1900.f, -2700.f), 1.f },
	};
	const float ObrotDomu = 90.f;
	const FVector2D S = FSwiat::Start();
	for (int32 Proba = 0; Proba < 600; ++Proba)
	{
		const int32 Pierscien = (Proba + 7) / 8;
		const float Kat = (Proba % 8) * 45.f;
		const FVector2D Srodek = S + FVector2D(FMath::Cos(FMath::DegreesToRadians(Kat)), FMath::Sin(FMath::DegreesToRadians(Kat))) * 6000.f * Pierscien;
		TArray<float> Z;
		if (!MoznaOboz(Srodek, Z))
		{
			continue;
		}
		TArray<float> ZDomow;
		for (const FDom& D : Domy)
		{
			float Zd;
			if (!MoznaPostawic(*D.R, Srodek + D.Off, ObrotDomu, Zd))
			{
				break;
			}
			ZDomow.Add(Zd);
		}
		if (ZDomow.Num() < int32(UE_ARRAY_COUNT(Domy)))
		{
			continue;
		}
		Osada->ZalozOboz(Srodek, Z);
		TArray<ABudowa*> B;
		for (int32 k = 0; k < int32(UE_ARRAY_COUNT(Domy)); ++k)
		{
			B.Add(Osada->Postaw(*Domy[k].R, Srodek + Domy[k].Off, ObrotDomu, ZDomow[k]));
			if (B.Last())
			{
				B.Last()->UstawPostep(Domy[k].Postep);
			}
		}
		if (B[2]) { B[2]->ZmienZapas(ESurowiec::Kloda, 4); Osada->DodajRodzine(B[2], true); }
		if (B[3]) { B[3]->ZmienZapas(ESurowiec::Kloda, 2); B[3]->ZmienZapas(ESurowiec::Deska, 4); Osada->DodajRodzine(B[3], true); }
		if (B[4]) { B[4]->ZmienZapas(ESurowiec::Kloda, 12); B[4]->ZmienZapas(ESurowiec::Deska, 6); B[4]->ZmienZapas(ESurowiec::Kamien, 4); }
		if (B[5]) { B[5]->ZmienZapas(ESurowiec::Kamien, 5); Osada->DodajRodzine(B[5], true); }
		if (B[6]) { B[6]->ZmienZapas(ESurowiec::Jedzenie, 18); }
		if (B[7]) { B[7]->ZmienZapas(ESurowiec::Jedzenie, 3); }
		// Etap 3: pole przy zniwach (wczesna jesien), mlyn i piekarnia; kazde w jednym z dwoch miejsc, jesli teren pozwala.
		Osada->UstawCzas(2.f * FKalendarz::DlugoscPory + 25.f);
		struct FDodatek { const FRodzajBudynku* R; FVector2D Off[2]; };
		const FDodatek Dodatki[] = {
			{ &FRodzajBudynku::Pole(), { FVector2D(6500.f, 0.f), FVector2D(-2600.f, 300.f) } },
			{ &FRodzajBudynku::Mlyn(), { FVector2D(6400.f, 2900.f), FVector2D(6400.f, -2900.f) } },
			{ &FRodzajBudynku::Piekarnia(), { FVector2D(-1900.f, 2600.f), FVector2D(2300.f, 3800.f) } },
		};
		ABudowa* E[3] = { nullptr, nullptr, nullptr };
		for (int32 k = 0; k < 3; ++k)
		{
			for (const FVector2D& Off : Dodatki[k].Off)
			{
				float Zd;
				if (MoznaPostawic(*Dodatki[k].R, Srodek + Off, ObrotDomu, Zd))
				{
					E[k] = Osada->Postaw(*Dodatki[k].R, Srodek + Off, ObrotDomu, Zd);
					break;
				}
			}
			UE_CLOG(!E[k], LogTemp, Warning, TEXT("Budowniczy: pokaz osady bez: %s (teren)"), *Dodatki[k].R->Nazwa);
		}
		if (E[0])
		{
			E[0]->UstawPostep(1.f);
			E[0]->DoZebrania = 13;
			E[0]->UstawUprawe(EUprawa::Dojrzale);
			E[0]->ZmienZapas(ESurowiec::Zboze, 5);
			Osada->DodajRodzine(E[0], true);
		}
		if (E[1])
		{
			E[1]->UstawPostep(1.f);
			E[1]->ZmienZapas(ESurowiec::Zboze, 3);
			E[1]->ZmienZapas(ESurowiec::Maka, 4);
			Osada->DodajRodzine(E[1], true);
		}
		if (E[2])
		{
			E[2]->UstawPostep(1.f);
			E[2]->ZmienZapas(ESurowiec::Maka, 2);
			E[2]->ZmienZapas(ESurowiec::Jedzenie, 8);
		}
		if (B[6]) { B[6]->ZmienZapas(ESurowiec::Zboze, 8); B[6]->ZmienZapas(ESurowiec::Maka, 3); }
		// Etap 4 (pozycje dobrane tak, zeby kola budynkow, Promien, nie nachodzily na siebie ani na oboz).
		const FDodatek Dodatki4[] = {
			{ &FRodzajBudynku::Zbrojownia(), { FVector2D(-3300.f, 0.f), FVector2D(-3300.f, 500.f) } },
			{ &FRodzajBudynku::Kuznia(), { FVector2D(-3500.f, 2200.f), FVector2D(-3700.f, 2500.f) } },
			{ &FRodzajBudynku::Owczarnia(), { FVector2D(-3700.f, -2300.f), FVector2D(-3900.f, -2600.f) } },
			{ &FRodzajBudynku::ChataTkacza(), { FVector2D(-5400.f, 1000.f), FVector2D(-5600.f, 1400.f) } },
			{ &FRodzajBudynku::KopalniaRudy(), { FVector2D(-5400.f, -1100.f), FVector2D(-5700.f, -1400.f) } },
		};
		ABudowa* E4[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
		for (int32 k = 0; k < 5; ++k)
		{
			for (const FVector2D& Off : Dodatki4[k].Off)
			{
				float Zd;
				if (MoznaPostawic(*Dodatki4[k].R, Srodek + Off, ObrotDomu, Zd))
				{
					E4[k] = Osada->Postaw(*Dodatki4[k].R, Srodek + Off, ObrotDomu, Zd);
					break;
				}
			}
			UE_CLOG(!E4[k], LogTemp, Warning, TEXT("Budowniczy: pokaz osady bez: %s (teren)"), *Dodatki4[k].R->Nazwa);
			if (E4[k])
			{
				E4[k]->UstawPostep(1.f);
			}
		}
		Osada->RodzinyPokazu(7);
		if (E4[0])
		{
			E4[0]->ZmienZapas(ESurowiec::Bron, 6);
			E4[0]->ZmienZapas(ESurowiec::Sukno, 5);
			for (int32 k = 0; k < 3; ++k)
			{
				Osada->DodajRodzine(E4[0], true);
			}
		}
		if (E4[1])
		{
			E4[1]->ZmienZapas(ESurowiec::Ruda, 3);
			E4[1]->ZmienZapas(ESurowiec::Kloda, 3);
			E4[1]->ZmienZapas(ESurowiec::Bron, 2);
			Osada->DodajRodzine(E4[1], true);
		}
		if (E4[2]) { E4[2]->ZmienZapas(ESurowiec::Welna, 4); Osada->DodajRodzine(E4[2], true); }
		if (E4[3]) { E4[3]->ZmienZapas(ESurowiec::Welna, 3); E4[3]->ZmienZapas(ESurowiec::Sukno, 3); Osada->DodajRodzine(E4[3], true); }
		if (E4[4]) { E4[4]->ZmienZapas(ESurowiec::Ruda, 5); Osada->DodajRodzine(E4[4], true); }
		Osada->PrzydzielTeraz(true);
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AKameraMapy* Kam = Cast<AKameraMapy>(PC->GetPawn()))
			{
				// Etap 4: kamera na zbrojowni i warsztatach przed obozem (bez nich jak w etapie 3: pole za kamieniolomem).
				Kam->UstawOgnisko(Srodek + (E4[0] ? FVector2D(-3900.f, 0.f) : FVector2D(E[0] ? 2600.f : 1100.f, 0.f)));
			}
		}
		if (Pasek.IsValid())
		{
			Pasek->OtworzKategorie(E4[0] ? TEXT("Wojsko") : TEXT("Rolnictwo"));
		}
		OtworzOkno(E4[0] ? E4[0] : (E[0] ? E[0] : B[5]));
		// Etap 5 (-rody): targ z towarem w pierscieniu za osada, rody po 15 minutach gry, jeden zakup u najblizszego rodu;
		// kamera na targu (z prawej od okna rodow), okno rodow i okno targu otwarte, pasek na zakladce Handel.
		if (FParse::Param(FCommandLine::Get(), TEXT("rody")) && Rody)
		{
			ABudowa* TargP = nullptr;
			for (int32 ProbaT = 0; ProbaT < 90 && !TargP; ++ProbaT)
			{
				const float OdlT = 4800.f + 900.f * float(ProbaT / 18);
				const float KatT = FMath::DegreesToRadians(10.f + 20.f * float(ProbaT % 18));
				const FVector2D G = Srodek + FVector2D(FMath::Cos(KatT), FMath::Sin(KatT)) * OdlT;
				float Zd;
				if (MoznaPostawic(FRodzajBudynku::Targ(), G, ObrotDomu, Zd))
				{
					TargP = Osada->Postaw(FRodzajBudynku::Targ(), G, ObrotDomu, Zd);
				}
			}
			UE_CLOG(!TargP, LogTemp, Warning, TEXT("Budowniczy: pokaz rodow bez targu (teren)"));
			if (TargP)
			{
				TargP->UstawPostep(1.f);
				TargP->ZmienZapas(ESurowiec::Kloda, 8);
				TargP->ZmienZapas(ESurowiec::Deska, 6);
				TargP->ZmienZapas(ESurowiec::Jedzenie, 10);
				TargP->ZmienZapas(ESurowiec::Welna, 4);
				TargP->ZmienZapas(ESurowiec::Sukno, 3);
				TargP->ZmienZapas(ESurowiec::Bron, 2);
			}
			if (!Rody->Zalozone())
			{
				Rody->Zaloz(Osada, Srodek);
			}
			float LatRodow = 0.f; // -latarodow=6: rody zyja dluzej (zrzut murow i duzych osad)
			FParse::Value(FCommandLine::Get(), TEXT("latarodow="), LatRodow);
			Rody->Przewin(LatRodow > 0.f ? LatRodow * 4.f * FKalendarz::DlugoscPory : 900.f);
			// Najblizszy rod: kupno 5 sztuk kamienia (albo desek, albo klod - co ma na sprzedaz).
			int32 Naj = -1;
			for (int32 i = 0; i < Rody->Lista().Num(); ++i)
			{
				if (Naj < 0 || Rody->Lista()[i].OdlegloscKm < Rody->Lista()[Naj].OdlegloscKm)
				{
					Naj = i;
				}
			}
			FString Powod;
			bool bOk = false;
			if (Naj >= 0)
			{
				bOk = Rody->Kup(Naj, ESurowiec::Kamien, 5, Powod) || Rody->Kup(Naj, ESurowiec::Deska, 5, Powod) || Rody->Kup(Naj, ESurowiec::Kloda, 5, Powod);
			}
			OtworzRody(Naj);
			if (OknoRodow.IsValid())
			{
				OknoRodow->UstawStatus(Powod, bOk);
			}
			if (TargP)
			{
				OtworzOkno(TargP);
				if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
				{
					if (AKameraMapy* Kam = Cast<AKameraMapy>(PC->GetPawn()))
					{
						// Ekran w prawo to +Y swiata (kamera patrzy w +X): srodek ekranu na lewo od targu.
						Kam->UstawOgnisko(FVector2D(TargP->GetActorLocation()) - FVector2D(0.f, 2000.f));
					}
				}
			}
			if (Pasek.IsValid())
			{
				Pasek->OtworzKategorie(TEXT("Handel"));
			}
			UE_LOG(LogTemp, Log, TEXT("Budowniczy: pokaz rodow: %d rodow, zakup: %s"), Rody->Lista().Num(), *Powod);
			// AI4 (-widokrodu): kamera nad osada najblizszego rodu w terenie.
			if (FParse::Param(FCommandLine::Get(), TEXT("widokrodu")) && Naj >= 0)
			{
				if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
				{
					if (AKameraMapy* Kam = Cast<AKameraMapy>(PC->GetPawn()))
					{
						Kam->UstawOgnisko(Rody->Lista()[Naj].Gdzie);
					}
				}
				ZamknijOkno();
			}
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("obszar")))
		{
			PokazObszarow(Srodek);
		}
		// Zrzut paska: -pasek=Surowce otwiera Budowe na tej kategorii (karty budynkow).
		FString KategoriaPaska;
		if (FParse::Value(FCommandLine::Get(), TEXT("pasek="), KategoriaPaska) && Pasek.IsValid())
		{
			ZamknijOkno();
			Pasek->OtworzKategorie(KategoriaPaska);
		}
		UE_LOG(LogTemp, Log, TEXT("Budowniczy: pokaz osady przy (%.0f, %.0f)"), Srodek.X, Srodek.Y);
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Budowniczy: nie znaleziono miejsca na pokaz osady"));
}

bool ABudowniczy::Klik(APlayerController* PC)
{
	float MX = 0.f, MY = 0.f;
	PC->GetMousePosition(MX, MY);
	if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		KlikStart = FVector2D(MX, MY);
		bKlikTrwa = true;
	}
	if (bKlikTrwa && PC->WasInputKeyJustReleased(EKeys::LeftMouseButton))
	{
		bKlikTrwa = false;
		return FVector2D::Distance(KlikStart, FVector2D(MX, MY)) < 6.f;
	}
	return false;
}

void ABudowniczy::WyborObozu(APlayerController* PC)
{
	PokazPodgladObozu(true);
	FVector P;
	const bool bJest = TerenPodKursorem(PC, P);
	bMozna = bJest && MoznaOboz(FVector2D(P), ZObozu);
	TArray<FVector> Miejsca;
	if (bJest)
	{
		AOsada::MiejscaObozu(FVector2D(P), Miejsca);
		Punkt = P;
	}
	for (int32 k = 0; k < PodgladObozu.Num(); ++k)
	{
		ABudowa* B = PodgladObozu[k].Get();
		if (!B)
		{
			continue;
		}
		B->SetActorHiddenInGame(!bJest);
		if (bJest)
		{
			const FVector2D G(Miejsca[k]);
			B->SetActorLocationAndRotation(FVector(G, ZObozu.IsValidIndex(k) ? ZObozu[k] : FSwiat::Wysokosc(G.X, G.Y)), FRotator(0.f, Miejsca[k].Z, 0.f));
			B->UstawKolorPodgladu(bMozna);
		}
	}
	if (Klik(PC) && bMozna)
	{
		PokazPodgladObozu(false);
		Osada->ZalozOboz(FVector2D(Punkt), ZObozu);
		if (Pasek.IsValid())
		{
			Pasek->OtworzKategorie(TEXT("Mieszkalne"));
		}
		UstawPomoc();
	}
}

// Mury, wieze i bramy laczone same (26.11.2026, prosba: "brama i mury powinno dac sie laczyc, przeciagac pod roznymi katami,
// automatycznie wykrywac przy budowaniu, czy sie polaczyc"). Wezly = konce odcinkow, boki bram i srodki wiez osady gracza.
namespace
{
	struct FLiniaMuru
	{
		FVector2D A, B;
		ABudowa* Bud;
	};

	void ZbierzUmocnienia(UWorld* W, TArray<FVector2D>& Wezly, TArray<FLiniaMuru>& Linie)
	{
		for (TActorIterator<ABudowa> It(W); It; ++It)
		{
			ABudowa* B = *It;
			const FRodzajBudynku& R = B->Rodzaj();
			if (B->Podglad() || !R.bUmocnienie || B->Ruina() || B->Zgliszcza())
			{
				continue;
			}
			const FVector2D P(B->GetActorLocation());
			const double Rad = FMath::DegreesToRadians(double(B->GetActorRotation().Yaw));
			const FVector2D K(FMath::Cos(Rad), FMath::Sin(Rad));
			if (R.bLinia)
			{
				Wezly.Add(P);
				Wezly.Add(P + K * 400.0);
				Linie.Add({ P, P + K * 400.0, B });
			}
			else if (R.Polowa.X > R.Polowa.Y + 50.f)
			{
				// Brama: mur dochodzi do bokow wiezy bramnej (wzdluz X), przejazd w poprzek (Y).
				Wezly.Add(P - K * double(R.Polowa.X));
				Wezly.Add(P + K * double(R.Polowa.X));
			}
			else
			{
				Wezly.Add(P); // wieza: mur konczy sie w jej srodku
			}
		}
	}

	bool Zaczep(const TArray<FVector2D>& Wezly, FVector2D P, float Promien, FVector2D& Out)
	{
		double Naj = double(Promien) * Promien;
		bool bJest = false;
		for (const FVector2D& Q : Wezly)
		{
			const double D = FVector2D::DistSquared(Q, P);
			if (D < Naj)
			{
				Naj = D;
				Out = Q;
				bJest = true;
			}
		}
		return bJest;
	}

	/** Najblizszy punkt na odcinku muru (do Promien); Yaw = kierunek muru. */
	bool NaMurze(const TArray<FLiniaMuru>& Linie, FVector2D P, float Promien, FVector2D& Out, float& Yaw, int32& Ktory)
	{
		double Naj = double(Promien) * Promien;
		Ktory = -1;
		for (int32 i = 0; i < Linie.Num(); ++i)
		{
			const FVector2D AB = Linie[i].B - Linie[i].A;
			const double T = FMath::Clamp(((P - Linie[i].A) | AB) / FMath::Max(1.0, AB.SizeSquared()), 0.0, 1.0);
			const FVector2D Q = Linie[i].A + AB * T;
			const double D = FVector2D::DistSquared(Q, P);
			if (D < Naj)
			{
				Naj = D;
				Out = Q;
				Yaw = float(FMath::RadiansToDegrees(FMath::Atan2(AB.Y, AB.X)));
				Ktory = i;
			}
		}
		return Ktory >= 0;
	}
}

void ABudowniczy::Stawianie(APlayerController* PC)
{
	if (Wybrany->ZObszarem())
	{
		StawianieObszaru(PC);
		return;
	}
	if (Wybrany->bLinia)
	{
		StawianieLinii(PC);
		return;
	}
	if (PC->WasInputKeyJustPressed(EKeys::Escape) || PC->WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		Wybierz(nullptr);
		return;
	}
	ObrocKlawiszem(PC);

	// Podglad pod kursorem. Przy drodze budynek przyciaga sie do niej frontem (Alt - bez przyciagania).
	FVector P;
	if (TerenPodKursorem(PC, P))
	{
		FVector2D G;
		float ObrotDrogi = Obrot;
		if (Drogi && !PC->IsInputKeyDown(EKeys::LeftAlt) && !PC->IsInputKeyDown(EKeys::RightAlt)
			&& Drogi->Przyciagnij(FVector2D(P), Wybrany->Polowa.Y, G, ObrotDrogi))
		{
			P = FVector(G, P.Z);
			Obrot = ObrotDrogi;
		}
		// Wieza i brama (26.11.2026): wieza staje na koncu muru albo na murze, brama wchodzi w mur (obrot jak mur);
		// odcinki pod brama sa rozbierane przy postawieniu. Alt wylacza.
		MurPodBrama.Reset();
		if (Wybrany->bUmocnienie && !PC->IsInputKeyDown(EKeys::LeftAlt) && !PC->IsInputKeyDown(EKeys::RightAlt))
		{
			TArray<FVector2D> Wezly;
			TArray<FLiniaMuru> Linie;
			ZbierzUmocnienia(GetWorld(), Wezly, Linie);
			const bool bBrama = Wybrany->Polowa.X > Wybrany->Polowa.Y + 50.f;
			FVector2D Q;
			float YawM = Obrot;
			int32 Ktory = -1;
			if (!bBrama && Zaczep(Wezly, FVector2D(P), 400.f, Q))
			{
				P = FVector(Q, P.Z);
			}
			else if (NaMurze(Linie, FVector2D(P), 400.f, Q, YawM, Ktory))
			{
				P = FVector(Q, P.Z);
				if (bBrama)
				{
					// Zostaje ta z dwoch stron muru, ktora gracz wybral obrotem (front bramy +Y).
					Obrot = FMath::Abs(FMath::FindDeltaAngleDegrees(YawM, Obrot)) <= 90.f ? YawM : YawM + 180.f;
					const FVector2D Os(FMath::Cos(FMath::DegreesToRadians(Obrot)), FMath::Sin(FMath::DegreesToRadians(Obrot)));
					for (const FLiniaMuru& L : Linie)
					{
						const FVector2D Sr = (L.A + L.B) * 0.5 - Q;
						if (FMath::Abs(Sr | Os) < Wybrany->Polowa.X + 100.0 && FMath::Abs(Sr ^ Os) < 200.0)
						{
							MurPodBrama.Add(L.Bud);
						}
					}
				}
			}
		}
		float Z;
		bMozna = MoznaPostawic(*Wybrany, FVector2D(P), Obrot, Z);
		Punkt = P;
		PunktZ = Z;
		if (Podglad.IsValid())
		{
			Podglad->SetActorHiddenInGame(false);
			Podglad->SetActorLocationAndRotation(FVector(P.X, P.Y, Z), FRotator(0.f, Obrot, 0.f));
			Podglad->UstawKolorPodgladu(bMozna);
		}
	}
	else if (Podglad.IsValid())
	{
		Podglad->SetActorHiddenInGame(true);
		bMozna = false;
	}

	if (Klik(PC) && bMozna)
	{
		// Budowe podejmuja wolni osadnicy (AOsada::Tick).
		Osada->Postaw(*Wybrany, FVector2D(Punkt), Obrot, PunktZ);
		for (const TWeakObjectPtr<ABudowa>& M : MurPodBrama)
		{
			if (M.IsValid())
			{
				Osada->Rozbierz(M.Get()); // przejazd bramy zamiast odcinkow muru (material wraca do skladu)
			}
		}
		MurPodBrama.Reset();
		if (!PC->IsInputKeyDown(EKeys::LeftShift))
		{
			Wybierz(nullptr);
		}
	}
}

void ABudowniczy::UsunPodgladLinii()
{
	for (const TWeakObjectPtr<ABudowa>& B : PodgladLinii)
	{
		if (B.IsValid()) { B->Destroy(); }
	}
	PodgladLinii.Reset();
	bLiniaTrwa = false;
}

void ABudowniczy::StawianieLinii(APlayerController* PC)
{
	// Palisada i mur (26.11.2026): przytrzymaj lewy przycisk i przeciagnij - powstaje szereg odcinkow po 4 m od punktu A do kursora
	// (zielone = mozna, czerwone = nie). Puszczenie stawia zielone odcinki; wybor zostaje, zeby ciagnac mur dalej (Esc / prawy klik konczy).
	constexpr float Dl = 400.f;
	constexpr int32 Max = 40;
	if (PC->WasInputKeyJustPressed(EKeys::Escape) || PC->WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		if (bLiniaTrwa)
		{
			UsunPodgladLinii();
		}
		else
		{
			Wybierz(nullptr);
		}
		return;
	}
	FVector P;
	if (!TerenPodKursorem(PC, P))
	{
		if (bLiniaTrwa && PC->WasInputKeyJustReleased(EKeys::LeftMouseButton))
		{
			UsunPodgladLinii(); // puszczone poza mapa: linia przepada, nic sie nie buduje
			return;
		}
		for (const TWeakObjectPtr<ABudowa>& B : PodgladLinii) { if (B.IsValid()) { B->SetActorHiddenInGame(true); } }
		return;
	}
	// Przyciaganie (Alt wylacza): poczatek i koniec linii do konca muru, boku bramy albo wiezy w promieniu 3,5 m.
	TArray<FVector2D> Wezly;
	TArray<FLiniaMuru> Linie;
	const bool bPrzyciagaj = !PC->IsInputKeyDown(EKeys::LeftAlt) && !PC->IsInputKeyDown(EKeys::RightAlt);
	if (bPrzyciagaj)
	{
		ZbierzUmocnienia(GetWorld(), Wezly, Linie);
	}
	FVector2D K(P);
	FVector2D Zacz;
	const bool bZaczep = bPrzyciagaj && Zaczep(Wezly, K, 350.f, Zacz) && !(bLiniaTrwa && Zacz.Equals(LiniaA, 1.0));
	if (bZaczep)
	{
		K = Zacz;
	}
	if (!bLiniaTrwa)
	{
		ObrocKlawiszem(PC);
		if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
		{
			bLiniaTrwa = true;
			LiniaA = K;
		}
	}
	const FVector2D A = bLiniaTrwa ? LiniaA : K;
	const FVector2D Wektor = K - A;
	const float Dist = float(Wektor.Size());
	// Dowolny kat i dlugosc: liczba odcinkow w gore, rowne kroki (odcinki lekko na siebie zachodza), wiec linia konczy sie
	// dokladnie na kursorze albo na przyciagnietym wezle, bez szpary w rogu.
	const int32 N = bLiniaTrwa && Dist > 150.f ? FMath::Clamp(FMath::CeilToInt32(Dist / Dl - 0.05f), 1, Max) : 1;
	const float Krok = N > 0 && bLiniaTrwa && Dist > 150.f ? FMath::Min(Dl, Dist / N) : Dl;
	const float Yaw = bLiniaTrwa && Dist > 150.f ? FMath::RadiansToDegrees(FMath::Atan2(Wektor.Y, Wektor.X)) : Obrot;
	if (bLiniaTrwa && Dist > 150.f) { Obrot = Yaw; }
	const FVector2D Kier(FMath::Cos(FMath::DegreesToRadians(Yaw)), FMath::Sin(FMath::DegreesToRadians(Yaw)));
	while (PodgladLinii.Num() < N)
	{
		FActorSpawnParameters Sp;
		Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ABudowa* B = GetWorld()->SpawnActor<ABudowa>(FVector::ZeroVector, FRotator::ZeroRotator, Sp);
		if (!B) { break; }
		B->Ustaw(*Wybrany, true);
		PodgladLinii.Add(B);
	}
	while (PodgladLinii.Num() > N)
	{
		if (ABudowa* B = PodgladLinii.Pop().Get()) { B->Destroy(); }
	}
	if (Podglad.IsValid()) { Podglad->SetActorHiddenInGame(true); }
	TArray<FVector> Miejsca; // X, Y, Z odcinka; Z = NaN gdy nie wolno
	for (int32 i = 0; i < PodgladLinii.Num(); ++i)
	{
		const FVector2D P0 = A + Kier * (Krok * i);
		float Z = 0.f;
		const bool bOk = MoznaPostawic(*Wybrany, P0, Yaw, Z); // ten sam punkt, w ktorym odcinek jest zapisany (Polowa.X = 400 obejmuje caly odcinek)
		Miejsca.Add(FVector(P0, bOk ? Z : TNumericLimits<float>::Max()));
		if (ABudowa* B = PodgladLinii[i].Get())
		{
			B->SetActorHiddenInGame(false);
			B->SetActorLocationAndRotation(FVector(P0, bOk ? Z : FSwiat::Wysokosc(P0.X, P0.Y)), FRotator(0.f, Yaw, 0.f));
			B->UstawKolorPodgladu(bOk);
		}
	}
	if (bLiniaTrwa && PC->WasInputKeyJustReleased(EKeys::LeftMouseButton))
	{
		int32 Postawiono = 0;
		for (const FVector& M : Miejsca)
		{
			if (M.Z < TNumericLimits<float>::Max())
			{
				Osada->Postaw(*Wybrany, FVector2D(M), Yaw, M.Z);
				++Postawiono;
			}
		}
		UsunPodgladLinii();
		if (Postawiono == 0 && Osada) { Osada->Ogloszenie(TEXT("Tu nie można budować (teren, rzeka albo inny budynek)")); }
	}
}

void ABudowniczy::StawianieObszaru(APlayerController* PC)
{
	// Pole i pastwisko dowolnym ksztaltem (26.11.2026, uwaga z gry: "rysowane punktami jak w Manor Lords"): lewy klik - kolejny
	// rog plotu, zamkniecie klikiem w pierwszy punkt albo Enter, prawy klik / Backspace - cofnij punkt (bez punktow: anuluj),
	// Esc - anuluj. Przod z brama to najdluzszy bok, szopa owczarni staje sama w srodku. Potem panel: Zbuduj (Enter) / Anuluj.
	const bool bPrawy = PC->WasInputKeyJustPressed(EKeys::RightMouseButton) || PC->WasInputKeyJustPressed(EKeys::BackSpace);
	const bool bKlik = Klik(PC);
	if (PC->WasInputKeyJustPressed(EKeys::Escape) || (bPrawy && PunktyObszaru.Num() == 0 && KrokObszaru == 0))
	{
		Wybierz(nullptr);
		return;
	}
	if (bPrawy)
	{
		if (KrokObszaru == 2)
		{
			KrokObszaru = 0;
		}
		else
		{
			PunktyObszaru.Pop();
		}
		OstatniKursor = FVector2D(1e9);
		UstawPomoc();
		return;
	}
	if (KrokObszaru == 2)
	{
		RysujLinie(PunktyObszaru, true, bMozna);
		if (PC->WasInputKeyJustPressed(EKeys::Enter))
		{
			ZbudujSzkic();
		}
		return;
	}
	FVector P;
	const bool bJest = TerenPodKursorem(PC, P);
	const FVector2D Kursor = bJest ? FVector2D(P) : FVector2D::ZeroVector;
	const bool bDoPierwszego = bJest && PunktyObszaru.Num() >= 3 && FVector2D::Distance(Kursor, PunktyObszaru[0]) < 250.0;
	const bool bZamknij = PunktyObszaru.Num() >= 3 && (PC->WasInputKeyJustPressed(EKeys::Enter) || (bKlik && bDoPierwszego));
	TArray<FVector2D> Pkt = PunktyObszaru;
	if (bJest && !bDoPierwszego && !bZamknij)
	{
		Pkt.Add(Kursor);
	}
	if (Pkt.Num() >= 3)
	{
		if (bZamknij || !Kursor.Equals(OstatniKursor, 50.0))
		{
			OstatniKursor = Kursor;
			FRodzajBudynku Nowy;
			FVector2D G;
			float O = 0.f;
			BladObszaru = FRodzajBudynku::NaWielokat(*Wybrany, Pkt, Nowy, G, O);
			if (BladObszaru.IsEmpty())
			{
				Szkic = MakeShared<FRodzajBudynku>(Nowy);
				SzkicGdzie = G;
				SzkicObrot = O;
				float Z = 0.f;
				bMozna = MoznaPostawic(*Szkic, G, O, Z);
				PunktZ = Z;
				if (!bMozna)
				{
					BladObszaru = TEXT("Tu nie można: woda, za stromo albo zajęte przez budynek lub drogę");
				}
				if (Podglad.IsValid())
				{
					Podglad->Ustaw(*Szkic, true);
					Podglad->SetActorHiddenInGame(false);
					Podglad->SetActorLocationAndRotation(FVector(G, Z), FRotator(0.f, O, 0.f));
					Podglad->OdswiezObszar(true);
					Podglad->UstawKolorPodgladu(bMozna);
				}
			}
			else
			{
				bMozna = false;
				if (Podglad.IsValid())
				{
					Podglad->SetActorHiddenInGame(true);
				}
			}
			UstawPomoc();
		}
	}
	else
	{
		bMozna = false;
		BladObszaru.Empty();
		Szkic.Reset();
		if (Podglad.IsValid())
		{
			Podglad->SetActorHiddenInGame(true);
		}
	}
	RysujLinie(Pkt, Pkt.Num() >= 3, Pkt.Num() < 3 || bMozna);
	if (bZamknij)
	{
		if (bMozna && Szkic.IsValid())
		{
			KrokObszaru = 2;
		}
		UstawPomoc();
		return;
	}
	if (bJest && bKlik && !bDoPierwszego)
	{
		PunktyObszaru.Add(Kursor);
		UstawPomoc();
	}
}

void ABudowniczy::RysujLinie(const TArray<FVector2D>& P, bool bZamknieta, bool bDobra)
{
	// Obrys rysowanego plotu: pas 40 cm nad terenem i kwadraty w rogach (zielony = mozna, czerwony = nie).
	if (!LiniaObszaru)
	{
		LiniaObszaru = NewObject<UProceduralMeshComponent>(this);
		LiniaObszaru->SetupAttachment(RootComponent);
		LiniaObszaru->SetUsingAbsoluteLocation(true);
		LiniaObszaru->SetUsingAbsoluteRotation(true);
		LiniaObszaru->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LiniaObszaru->SetCastShadow(false);
		LiniaObszaru->RegisterComponent();
		LiniaObszaru->SetWorldLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Swiat/Budynki/M_Bud_Podglad.M_Bud_Podglad")))
		{
			MaterialLinii = UMaterialInstanceDynamic::Create(M, this);
			LiniaObszaru->SetMaterial(0, MaterialLinii);
		}
	}
	LiniaObszaru->ClearAllMeshSections();
	if (P.Num() == 0)
	{
		return;
	}
	if (MaterialLinii)
	{
		MaterialLinii->SetVectorParameterValue(TEXT("Kolor"), bDobra ? FLinearColor(0.2f, 0.9f, 0.3f) : FLinearColor(1.f, 0.15f, 0.1f));
	}
	TArray<FVector> V;
	TArray<int32> T;
	auto Quad = [&](FVector2D A, FVector2D B, float Szer)
	{
		const FVector2D D = (B - A).GetSafeNormal();
		const FVector2D N(-D.Y * Szer * 0.5, D.X * Szer * 0.5);
		const int32 I = V.Num();
		for (const FVector2D& Q : { A - N, A + N, B + N, B - N })
		{
			V.Add(FVector(Q, FSwiat::WysokoscKafla(Q.X, Q.Y) + 40.0));
		}
		T.Append({ I, I + 1, I + 2, I, I + 2, I + 3, I, I + 2, I + 1, I, I + 3, I + 2 }); // z obu stron
	};
	const int32 N = P.Num();
	for (int32 i = 0; i < (bZamknieta ? N : N - 1); ++i)
	{
		const FVector2D A = P[i], B = P[(i + 1) % N];
		const int32 Kawalki = FMath::Max(1, FMath::CeilToInt32(FVector2D::Distance(A, B) / 200.0));
		for (int32 k = 0; k < Kawalki; ++k)
		{
			Quad(FMath::Lerp(A, B, double(k) / Kawalki), FMath::Lerp(A, B, double(k + 1) / Kawalki), 40.f);
		}
	}
	for (int32 i = 0; i < N; ++i)
	{
		Quad(P[i] - FVector2D(60.0, 0.0), P[i] + FVector2D(60.0, 0.0), i == 0 ? 160.f : 120.f);
	}
	LiniaObszaru->CreateMeshSection(0, V, T, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>(), false);
}

void ABudowniczy::ZbudujSzkic()
{
	if (!Wybrany || !Szkic.IsValid() || KrokObszaru != 2 || !bMozna || !Osada)
	{
		return;
	}
	// Budowe podejmuja wolni osadnicy (AOsada::Tick). Shift: zaznaczaj dalej.
	Osada->Postaw(*Szkic, SzkicGdzie, SzkicObrot, PunktZ);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->IsInputKeyDown(EKeys::LeftShift))
	{
		KrokObszaru = 0;
		PunktyObszaru.Reset();
		Szkic.Reset();
		RysujLinie({}, false, true);
		UstawPomoc();
	}
	else
	{
		Wybierz(nullptr);
	}
}

FString ABudowniczy::OpisSzkicu() const
{
	if (!Szkic.IsValid())
	{
		return FString();
	}
	const FRodzajBudynku& R = *Szkic;
	FString T = FString::Printf(TEXT("%s: %s"), *R.Nazwa, *R.OpisObszaru());
	if (R.Funkcja == EFunkcja::Pole)
	{
		T += FString::Printf(TEXT("\nPlon: %s z jednych żniw   ·   rodzin do pracy: %d   ·   orka i siew: %.0f s pracy jednej osoby"),
			*IleSurowca(ESurowiec::Zboze, R.Plon), R.Miejsc, AOsada::OrkaPola(R) + AOsada::SiewPola(R));
	}
	else
	{
		T += FString::Printf(TEXT("\nOwiec w stadzie: %d   ·   rodzin pasterzy: %d"), R.Owiec, R.Miejsc);
	}
	FString Koszt;
	for (ESurowiec S : WszystkieSurowce)
	{
		if (R.Koszt[S] > 0)
		{
			if (!Koszt.IsEmpty())
			{
				Koszt += TEXT(", ");
			}
			Koszt += IleSurowca(S, R.Koszt[S]);
		}
	}
	T += FString::Printf(TEXT("\nKoszt: %s   ·   płot: %d m"), *Koszt, FMath::RoundToInt32(R.ZWielokatem() ? R.ObwodWielokata / 100.f : 2.f * float(R.Obszar.X + R.Obszar.Y) / 100.f));
	return T;
}

TSharedRef<SWidget> ABudowniczy::PanelObszaru()
{
	return SNew(SBorder)
		.BorderImage(&TloInfo)
		.Padding(FMargin(16.f, 8.f))
		.Visibility_Lambda([this]()
		{
			return (Wybrany && Wybrany->ZObszarem() && Szkic.IsValid() && (KrokObszaru == 2 || PunktyObszaru.Num() >= 2)) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
		})
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::FromString(OpisSzkicu()); })
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				.ColorAndOpacity(FLinearColor(0.95f, 0.90f, 0.78f))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Visibility_Lambda([this]() { return bMozna && BladObszaru.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
				.Text_Lambda([this]() { return FText::FromString(BladObszaru.IsEmpty() ? FString(TEXT("Tu nie można: woda, za stromo albo zajęte przez inny budynek")) : BladObszaru); })
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				.ColorAndOpacity(FLinearColor(1.f, 0.42f, 0.32f))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(&StylPrzyciskuRodow)
					.ContentPadding(FMargin(16.f, 4.f))
					.ToolTipText(FText::FromString(TEXT("Postaw tu plac budowy; osadnicy przyniosą żerdzie i postawią płot")))
					.IsEnabled_Lambda([this]() { return KrokObszaru == 2 && bMozna; })
					.OnClicked_Lambda([this]() { ZbudujSzkic(); return FReply::Handled(); })
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Zbuduj (Enter)")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						.ColorAndOpacity(StylUI::Zloto)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(&StylPrzyciskuRodow)
					.ContentPadding(FMargin(16.f, 4.f))
					.OnClicked_Lambda([this]() { Wybierz(nullptr); return FReply::Handled(); })
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Anuluj (Esc)")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						.ColorAndOpacity(StylUI::Zloto)
					]
				]
			]
		];
}

void ABudowniczy::PokazObszarow(FVector2D Srodek)
{
	// -osada -obszar: w pierscieniu za osada duze pole przy zniwach (32 x 20 m), z prawej pastwisko 24 x 18 m ze stadem,
	// z lewej zaznaczone nowe pole 24 x 16 m (podglad, panel z hektarami); kamera na nich.
	const float Ob = 90.f;
	const double Rad = FMath::DegreesToRadians(double(Ob));
	const FVector2D U(FMath::Cos(Rad), FMath::Sin(Rad)), V(-U.Y, U.X);
	const FRodzajBudynku PoleD = FRodzajBudynku::NaObszar(FRodzajBudynku::Pole(), 3200.f, 2000.f);
	const FRodzajBudynku OwczD = FRodzajBudynku::NaObszar(FRodzajBudynku::Owczarnia(), 2400.f, 1800.f);
	const FRodzajBudynku Nowe = FRodzajBudynku::NaObszar(FRodzajBudynku::Pole(), 2400.f, 1600.f);
	auto Gdzie = [&](const FRodzajBudynku& R, FVector2D SrodekProst) { return SrodekProst - U * R.SrodekObszaru.X - V * R.SrodekObszaru.Y; };
	for (int32 Proba = 0; Proba < 120; ++Proba)
	{
		const float Odl = 9000.f + 1500.f * float(Proba / 24);
		const float Kat = FMath::DegreesToRadians(7.5f + 15.f * float(Proba % 24));
		const FVector2D G = Srodek + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * Odl;
		const FVector2D GP = Gdzie(PoleD, G), GO = Gdzie(OwczD, G + FVector2D(0.f, 3400.f)), GN = Gdzie(Nowe, G - FVector2D(0.f, 3500.f));
		float ZP, ZO, ZN;
		if (!MoznaPostawic(PoleD, GP, Ob, ZP) || !MoznaPostawic(OwczD, GO, Ob, ZO) || !MoznaPostawic(Nowe, GN, Ob, ZN))
		{
			continue;
		}
		if (ABudowa* P = Osada->Postaw(PoleD, GP, Ob, ZP))
		{
			P->UstawPostep(1.f);
			P->DoZebrania = PoleD.Plon;
			P->UstawUprawe(EUprawa::Dojrzale);
			P->ZmienZapas(ESurowiec::Zboze, 6);
			Osada->DodajRodzine(P, true);
			Osada->DodajRodzine(P, true);
			OtworzOkno(P);
		}
		if (ABudowa* O = Osada->Postaw(OwczD, GO, Ob, ZO))
		{
			O->UstawPostep(1.f);
			O->ZmienZapas(ESurowiec::Welna, 5);
			Osada->DodajRodzine(O, true);
		}
		Osada->PrzydzielTeraz(true);
		// Nowe pole zaznaczone, czeka na Zbuduj.
		Obrot = Ob;
		Wybierz(&FRodzajBudynku::Pole());
		Szkic = MakeShared<FRodzajBudynku>(Nowe);
		KrokObszaru = 2;
		SzkicGdzie = GN;
		SzkicObrot = Ob;
		PunktZ = ZN;
		bMozna = true;
		if (Podglad.IsValid())
		{
			Podglad->Ustaw(*Szkic, true);
			Podglad->SetActorHiddenInGame(false);
			Podglad->SetActorLocationAndRotation(FVector(GN, ZN), FRotator(0.f, Ob, 0.f));
			Podglad->OdswiezObszar(true);
			Podglad->UstawKolorPodgladu(true);
		}
		if (Pasek.IsValid())
		{
			Pasek->OtworzKategorie(TEXT("Rolnictwo"));
		}
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (AKameraMapy* Kam = Cast<AKameraMapy>(PC->GetPawn()))
			{
				Kam->UstawOgnisko(G + FVector2D(0.f, 700.f)); // okno budynku zaslania prawa strone ekranu
			}
		}
		UstawPomoc();
		UE_LOG(LogTemp, Log, TEXT("Budowniczy: pokaz pola i pastwiska przy (%.0f, %.0f)"), G.X, G.Y);
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Budowniczy: pokaz pola i pastwiska - brak miejsca"));
}

void ABudowniczy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Teren.IsValid())
	{
		for (TActorIterator<ATerenKafle> It(GetWorld()); It; ++It)
		{
			Teren = *It;
			if (Osada)
			{
				Osada->UstawTeren(*It);
			}
			if (Bitwa)
			{
				Bitwa->UstawTeren(*It);
			}
			break;
		}
	}
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !Teren.IsValid() || !Osada)
	{
		return;
	}
	// Tryb deweloperski (F3 albo przycisk w prawym dolnym rogu): czas gry, budowa natychmiast, surowce.
	if (PC->WasInputKeyJustPressed(EKeys::F3))
	{
		FTrybDeweloperski::bOtwarty = !FTrybDeweloperski::bOtwarty;
	}
	FTrybDeweloperski::Zastosuj(GetWorld(), this, Bitwa && (Bitwa->Walka() || bTrybBitwy));

	// Poza mapa grywalna: osada ukryta, budowanie wylaczone.
	const bool bRegion = Teren->WRegionie();
	if (bRegion != bWidoczne)
	{
		bWidoczne = bRegion;
		Osada->UstawWidocznosc(bRegion);
		if (Drogi)
		{
			Drogi->SetActorHiddenInGame(!bRegion);
			Drogi->Wlacz(false);
		}
		if (!bRegion)
		{
			Wybierz(nullptr);
			PokazPodgladObozu(false);
		}
		UstawPomoc();
	}
	DoInfo -= DeltaSeconds;
	if (DoInfo <= 0.f)
	{
		DoInfo = 0.5f;
		UstawInfo();
		UstawPomoc();
	}
	if (!bRegion)
	{
		return;
	}
	if (!bPokazZrobiony && Teren->Gotowe())
	{
		bPokazZrobiony = true;
		FZapisGry::Zastosuj(GetWorld()); // wczytanie zapisu (P4, 26.11.2026): czeka, az teren regionu jest gotowy
		if (bTrybPokazu)
		{
			Pokaz();
		}
		else if (bTrybBitwy && Bitwa)
		{
			Bitwa->PokazBitwy();
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("wszystko")))
		{
			// Zrzut kontrolny trybu deweloperskiego: przycisk "Postaw wszystko", panel otwarty.
			FTrybDeweloperski::bOtwarty = true;
			PostawWszystko();
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("osada")))
		{
			PokazOsady();
			if (FParse::Param(FCommandLine::Get(), TEXT("drogi")) && Osada->MaOboz())
			{
				PokazDrog(Osada->Oboz());
			}
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("galeria")))
		{
			AGaleria::Przelacz(GetWorld(), AGaleria::ERodzaj::Budynki);
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("galeriawojska")))
		{
			AGaleria::Przelacz(GetWorld(), AGaleria::ERodzaj::Wojsko);
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("poziomy")))
		{
			PokazPoziomow();
		}
		UstawPomoc();
	}

	// Zrzut kontrolny pospolitego ruszenia: -osada -napad = kilka sekund po pokazie osady napad bandytow, jeden oddzial
	// ruszenia powolany (jak klik gracza w oknie Wojsko), okno Wojsko otwarte.
	static float NapadZa = 6.f;
	if (bPokazZrobiony && Bitwa && NapadZa > 0.f && FParse::Param(FCommandLine::Get(), TEXT("napad")))
	{
		NapadZa -= DeltaSeconds;
		if (NapadZa <= 0.f && Bitwa->ZacznijNapad())
		{
			if (Bitwa->Milicje().Num() == 0)
			{
				Bitwa->NowaMilicja();
			}
			Bitwa->Powolaj(0);
			OtworzWojsko();
			// Zrzut: -odl= ponizej zwyklej granicy regionu (w czasie walki kamera moze zjechac nizej).
			float Odl = 0.f;
			APlayerController* PCZ = GetWorld()->GetFirstPlayerController();
			AKameraMapy* Kam = PCZ ? Cast<AKameraMapy>(PCZ->GetPawn()) : nullptr;
			if (Kam && FParse::Value(FCommandLine::Get(), TEXT("odl="), Odl) && Odl < 25000.f)
			{
				AKameraMapy::bBitwaBlisko = true;
				Kam->UstawOdl(FMath::Max(Odl, 8000.f));
			}
		}
	}

	// Wojsko w polu: oddzialami steruje ABitwa (lewy klik / ramka, prawy klik), a budowanie trwa dalej. Gdy gracz stawia
	// budynek, mysz nalezy do budowania. Pokaz -bitwa: samo pole bitwy. N - bitwa probna pod kursorem.
	if (Bitwa)
	{
		ABitwa::bMyszZajeta = Wybrany != nullptr || (Drogi && Drogi->Rysuje());
	}
	// Galerie testowe (24.11.2026): F5 wszystkie budynki i ich stany, F6 kazdy rodzaj wojska w kazdej animacji.
	if (PC->WasInputKeyJustPressed(EKeys::F5)) { AGaleria::Przelacz(GetWorld(), AGaleria::ERodzaj::Budynki); }
	if (PC->WasInputKeyJustPressed(EKeys::F6)) { AGaleria::Przelacz(GetWorld(), AGaleria::ERodzaj::Wojsko); }
	if (Bitwa && bTrybBitwy)
	{
		return;
	}
	if (PC->WasInputKeyJustPressed(FUstawienia::Klawisz("Rozwoj")) && Osada->MaOboz())
	{
		bRozwoj = !bRozwoj;
		DoRozwoju = 0.f;
	}
	if (bRozwoj && (DoRozwoju -= DeltaSeconds) <= 0.f)
	{
		DoRozwoju = 0.5f;
		TekstRozwoju = Osada->RozwojOsady();
	}
	if (PC->WasInputKeyJustPressed(FUstawienia::Klawisz("Wojsko")) && Osada->MaOboz())
	{
		OknoWojska.IsValid() ? ZamknijWojsko() : OtworzWojsko();
	}
	if (Bitwa && !Wybrany && !Bitwa->Trwa() && PC->WasInputKeyJustPressed(EKeys::N))
	{
		FVector P;
		if (TerenPodKursorem(PC, P) && Bitwa->ZacznijProbna(FVector2D(P), false))
		{
			ZamknijOkno();
			UstawPomoc();
			return;
		}
	}
	if (Bitwa && !Wybrany && PC->WasInputKeyJustPressed(EKeys::P) && Bitwa->ZacznijNapad())
	{
		// Napad bandytow na osade: gracz sam powoluje pospolite ruszenie (okno Wojsko).
		OtworzWojsko();
	}
	if (PC->WasInputKeyJustPressed(FUstawienia::Klawisz("Tempo")))
	{
		ABudowa::Tempo = ABudowa::Tempo > 1.5f ? 1.f : 10.f;
		UstawPomoc();
	}
	if (!Osada->MaOboz())
	{
		if (!bTrybPokazu && Teren->Gotowe())
		{
			WyborObozu(PC);
		}
		return;
	}
	if (Rody && !Rody->Zalozone())
	{
		Rody->Zaloz(Osada, Osada->Oboz()); // etap 5: inne rody osiadaja w okolicy razem z obozem gracza
	}
	// Autozapis co 5 min gry (slot 0), nie w trakcie bitwy.
	DoAutozapisu -= DeltaSeconds;
	if (DoAutozapisu <= 0.f && !(Bitwa && Bitwa->Walka()))
	{
		DoAutozapisu = FZapisGry::CoIleAutozapis;
		FString Blad;
		if (FZapisGry::Zapisz(GetWorld(), 0, Blad))
		{
			Osada->Ogloszenie(TEXT("Autozapis gry"));
		}
	}
	if (PC->WasInputKeyJustPressed(FUstawienia::Klawisz("Rody")))
	{
		OknoRodow.IsValid() ? ZamknijRody() : OtworzRody();
	}
	if (PC->WasInputKeyJustPressed(FUstawienia::Klawisz("Budowa")) && Pasek.IsValid())
	{
		Pasek->PrzelaczBudowe(); // Budowa: kategorie i karty budynkow
	}
	if (Okno.IsValid() && !Okno->Budynek())
	{
		ZamknijOkno(); // budynek zniknal (namiot po przeprowadzce)
	}
	// Drogi (G albo przycisk na pasku): rysowanie zamiast stawiania budynkow.
	if (Drogi)
	{
		Drogi->Ustaw(Osada, Teren.Get());
		if (PC->WasInputKeyJustPressed(FUstawienia::Klawisz("Drogi")))
		{
			const bool bWlacz = !Drogi->Rysuje();
			Wybierz(nullptr);
			Drogi->Wlacz(bWlacz);
			UstawPomoc();
		}
		if (Drogi->Rysuje() && Wybrany)
		{
			Drogi->Wlacz(false); // wybrany budynek z paska konczy rysowanie drog
		}
		if (Drogi->Rysuje())
		{
			FVector P;
			const bool bJest = TerenPodKursorem(PC, P);
			Drogi->Rysuj(PC, bJest, FVector2D(P), Klik(PC));
			if (!Drogi->Rysuje())
			{
				UstawPomoc();
			}
			return;
		}
	}
	if (Wybrany)
	{
		Stawianie(PC);
	}
	else
	{
		if (Klik(PC))
		{
			// Klik w budynek: okno budynku; w trawe (puste pole): zamyka wszystkie okna i karty budowy (jak w Manor Lords).
			// Klik idzie dalej do ABitwa (zaznaczanie oddzialow w polu).
			FVector P;
			ABudowa* B = TerenPodKursorem(PC, P) ? Osada->BudynekPod(FVector2D(P)) : nullptr;
			if (B)
			{
				OtworzOkno(B);
			}
			else
			{
				ZamknijOkno();
				ZamknijRody();
				ZamknijWojsko();
				if (Pasek.IsValid())
				{
					Pasek->OtworzKategorie(FString());
				}
			}
		}
		if (PC->WasInputKeyJustPressed(EKeys::Escape))
		{
			if (Okno.IsValid())
			{
				ZamknijOkno();
			}
			else if (OknoRodow.IsValid())
			{
				ZamknijRody();
			}
			else if (OknoWojska.IsValid())
			{
				ZamknijWojsko();
			}
			else if (Pasek.IsValid() && Pasek->BudowaOtwarta())
			{
				Pasek->OtworzKategorie(FString());
			}
			else
			{
				SMenuGlowne::Otworz(GetWorld(), false); // nic do zamkniecia: menu gry (pauza)
			}
		}
	}
}

void ABudowniczy::ObrocKlawiszem(APlayerController* PC)
{
	// Prosba uzytkownika 21.11.2026: budynki stawiane w dowolnym kierunku 0-360 st. Czas rzeczywisty (dziala tez w pauzie).
	const float Dt = float(FApp::GetDeltaTime());
	const FKey& KR = FUstawienia::Klawisz("ObrotBud");
	const FKey& KF = FUstawienia::Klawisz("ObrotBudL");
	const bool bR = PC->IsInputKeyDown(KR), bF = PC->IsInputKeyDown(KF);
	if (bR || bF)
	{
		CzasObrotu += Dt;
		if (CzasObrotu > 0.2f)
		{
			Obrot += (bR ? 1.f : -1.f) * 120.f * Dt; // przytrzymany: plynnie, 120 st./s
		}
	}
	else if (CzasObrotu > 0.f)
	{
		if (CzasObrotu <= 0.2f)
		{
			// Krotkie nacisniecie: do nastepnej wielokrotnosci 45 st. (R w prawo, F w lewo).
			const bool bWPrawo = PC->WasInputKeyJustReleased(KR) || !PC->WasInputKeyJustReleased(KF);
			Obrot = bWPrawo ? (FMath::FloorToFloat(Obrot / 45.f + 0.001f) + 1.f) * 45.f : (FMath::CeilToFloat(Obrot / 45.f - 0.001f) - 1.f) * 45.f;
		}
		CzasObrotu = 0.f;
	}
	Obrot = FMath::Fmod(Obrot + 720.f, 360.f);
}

void ABudowniczy::PokazDrog(FVector2D Srodek)
{
	// Zrzut kontrolny -osada -drogi: kreta droga 60 m obok obozu, przy niej domy przyciagniete frontem do drogi.
	if (!Drogi || !Osada)
	{
		return;
	}
	const FVector2D C = Srodek + FVector2D(0.f, -6500.f);
	TArray<FVector2D> Pkt;
	for (int32 t = 0; t <= 12; ++t)
	{
		Pkt.Add(C + FVector2D((t - 6) * 600.f, 700.f * FMath::Sin(t / 12.f * PI * 1.5f)));
	}
	Drogi->DodajDroge(Pkt);
	const FRodzajBudynku* Domy[] = { &FRodzajBudynku::Chata(), &FRodzajBudynku::DomPruski(), &FRodzajBudynku::Chata(), &FRodzajBudynku::DomDuzy() };
	int32 Ile = 0;
	for (int32 k = 0; k < 8 && Ile < 6; ++k)
	{
		const FRodzajBudynku& R = *Domy[k % 4];
		const FVector2D Przy = Pkt[1 + (k / 2) * 3] + FVector2D(0.f, (k % 2 ? 1.f : -1.f) * 1500.f);
		FVector2D G;
		float Ob = 0.f, Z = 0.f;
		if (Drogi->Przyciagnij(Przy, R.Polowa.Y, G, Ob) && MoznaPostawic(R, G, Ob, Z))
		{
			if (ABudowa* B = Osada->Postaw(R, G, Ob, Z))
			{
				B->UstawPostep(1.f);
				++Ile;
			}
		}
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AKameraMapy* Kam = Cast<AKameraMapy>(PC->GetPawn()))
		{
			Kam->UstawOgnisko(C);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Budowniczy: pokaz drog, %d domow przy drodze"), Ile);
}

FString ABudowniczy::PostawWszystko()
{
	// Tryb deweloperski (prosba uzytkownika 21.11.2026): jednym przyciskiem cala osada do sprawdzenia - oboz przy kamerze,
	// kazdy rodzaj budynku z paska (gotowy) w pierscieniach wokol obozu, rodziny w kazdym miejscu pracy, +100 surowcow.
	if (!Osada || !Teren.IsValid() || !bWidoczne)
	{
		return TEXT("Tylko na mapie grywalnej");
	}
	if (Bitwa && Bitwa->Trwa())
	{
		return TEXT("Najpierw skończ bitwę");
	}
	Wybierz(nullptr);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Kam = PC ? PC->GetPawn() : nullptr;
	const FVector2D Widok = Kam ? FVector2D(Kam->GetActorLocation()) : FSwiat::Start();
	if (!Osada->MaOboz())
	{
		PokazPodgladObozu(false);
		for (int32 Proba = 0; Proba < 400 && !Osada->MaOboz(); ++Proba)
		{
			const int32 Pierscien = (Proba + 7) / 8;
			const float Kat = FMath::DegreesToRadians((Proba % 8) * 45.f);
			const FVector2D P = Widok + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * 3000.f * Pierscien;
			TArray<float> Z;
			if (MoznaOboz(P, Z))
			{
				Osada->ZalozOboz(P, Z);
			}
		}
		if (!Osada->MaOboz())
		{
			return TEXT("Brak miejsca na obóz w okolicy");
		}
	}
	const FVector2D Srodek = Osada->Oboz();
	TArray<ABudowa*> Nowe;
	TArray<FString> Brak;
	for (const FRodzajBudynku* R : FRodzajBudynku::DoBudowania())
	{
		if (Osada->Gotowych(*R) > 0)
		{
			continue; // drugi klik nie dubluje budynkow
		}
		ABudowa* B = nullptr;
		// Pierscienie co 7 m od obozu, 24 kierunki; najdalej ok. 230 m (tartak wodny szuka rzeki).
		for (int32 Proba = 0; Proba < 720 && !B; ++Proba)
		{
			const float Odl = AOsada::PromienObozu + 1500.f + 700.f * float(Proba / 24);
			const float Kat = FMath::DegreesToRadians(15.f * float(Proba % 24) + 7.f * float(Proba / 24));
			const FVector2D G = Srodek + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * Odl;
			float Z;
			if (MoznaPostawic(*R, G, Obrot, Z))
			{
				B = Osada->Postaw(*R, G, Obrot, Z);
			}
		}
		if (B)
		{
			B->UstawPostep(1.f);
			Nowe.Add(B);
		}
		else
		{
			Brak.Add(R->Nazwa);
		}
	}
	// Rodziny: tyle, ile miejsc pracy w nowych budynkach (nowe przy ognisku, bez domu), od razu przy pracy.
	int32 Miejsc = 0;
	for (ABudowa* B : Nowe)
	{
		Miejsc += B->Rodzaj().Miejsc;
	}
	const int32 Dodac = FMath::Max(0, Miejsc - Osada->WolnychRodzin());
	Osada->RodzinyPokazu(Dodac);
	for (ABudowa* B : Nowe)
	{
		for (int32 k = 0; k < B->Rodzaj().Miejsc; ++k)
		{
			if (!Osada->DodajRodzine(B, true))
			{
				break;
			}
		}
	}
	Osada->DodajSurowce(100);
	Osada->PrzydzielTeraz(true);
	if (AKameraMapy* K = Cast<AKameraMapy>(Kam))
	{
		K->UstawOgnisko(Srodek);
	}
	UstawPomoc();
	FString T = FString::Printf(TEXT("Postawiono %d budynków, nowych rodzin: %d"), Nowe.Num(), Dodac);
	if (Brak.Num())
	{
		T += TEXT("\nBez miejsca: ") + FString::Join(Brak, TEXT(", "));
	}
	UE_LOG(LogTemp, Log, TEXT("Budowniczy: tryb deweloperski - %s"), *T.Replace(TEXT("\n"), TEXT(" | ")));
	return T;
}
