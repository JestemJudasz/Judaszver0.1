// Plik: Source/grarts2d/Wojna/OknoWojska.cpp
#include "Wojna/OknoWojska.h"
#include "Wojna/Bitwa.h"
#include "Ludzie/Ludzik.h"
#include "Osada/Osada.h"
#include "Swiat/TerenKafle.h"
#include "UI/StylUI.h"
#include "EngineUtils.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FSlateFontInfo Czcionka(int32 R, bool bGruba = false) { return FCoreStyle::GetDefaultFontStyle(bGruba ? "Bold" : "Regular", R); }

	int32 WloczniWZapasie(ABitwa* B)
	{
		if (!B || !B->GetWorld())
		{
			return 0;
		}
		for (TActorIterator<AOsada> It(B->GetWorld()); It; ++It)
		{
			return It->Zapasy()[ESurowiec::Bron];
		}
		return 0;
	}
}

void SOknoWojska::Construct(const FArguments& InArgs)
{
	Bitwa = InArgs._Bitwa;
	NaZamknij = InArgs._NaZamknij;
	Styl = StylUI::StylPrzycisku();
	static FSlateRoundedBoxBrush Tlo(StylUI::Tlo, 6.f, StylUI::Ramka, 1.f);
	const TWeakObjectPtr<ABitwa> Ja = Bitwa;
	auto Guzik = [this](const TCHAR* Tekst, const TCHAR* Podp, TFunction<void()> Akcja) -> TSharedRef<SWidget>
	{
		return SNew(SBox).Padding(FMargin(0.f, 0.f, 6.f, 0.f))
		[
			SNew(SButton).ButtonStyle(&Styl).ToolTipText(FText::FromString(Podp))
			.OnClicked_Lambda([Akcja]() { Akcja(); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(Tekst)).Font(Czcionka(10, true)).ColorAndOpacity(StylUI::Zloto)
			]
		];
	};
	ChildSlot
	[
		SNew(SBorder).BorderImage(&Tlo).Padding(FMargin(12.f, 8.f))
		[
			SNew(SBox).WidthOverride(470.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("WOJSKO — ODDZIAŁY I ĆWICZENIA"))).Font(Czcionka(12, true)).ColorAndOpacity(StylUI::Zloto)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton).ButtonStyle(&Styl).ToolTipText(FText::FromString(TEXT("Zamknij (J)")))
						.OnClicked_Lambda([this]() { NaZamknij.ExecuteIfBound(); return FReply::Handled(); })
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("X"))).Font(Czcionka(10, true)).ColorAndOpacity(StylUI::Napis)
						]
					]
				]
				// Liczniki: mezczyzni, wolni, wlocznie.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
				[
					SNew(STextBlock).Font(Czcionka(10)).ColorAndOpacity(StylUI::Napis)
					.Text_Lambda([Ja]()
					{
						ABitwa* B = Ja.Get();
						if (!B) { return FText::GetEmpty(); }
						return FText::FromString(FString::Printf(TEXT("Mężczyźni w osadzie: %d    wolni (poza oddziałami): %d    włócznie w zapasach: %d"),
							B->MezczyznWOsadzie(), B->WolniMezczyzni().Num(), WloczniWZapasie(B)));
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
				[
					SNew(STextBlock).Font(Czcionka(11, true)).ColorAndOpacity(StylUI::Czerwony)
					.Text(FText::FromString(TEXT("Bandyci w okolicy! Powołaj ruszenie, zanim splądrują zapasy.")))
					.Visibility_Lambda([Ja]() { return Ja.IsValid() && Ja->Walka() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					SAssignNew(Lista, SVerticalBox)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()[Guzik(TEXT("+ Nowy oddział"), TEXT("Oddział do 30 chłopa z wolnych mężczyzn osady"), [Ja]() { if (ABitwa* B = Ja.Get()) { B->NowaMilicja(); } })]
					+ SHorizontalBox::Slot().AutoWidth()[Guzik(TEXT("Powołaj wszystkie"), TEXT("Wszystkie oddziały rzucają pracę i stają przed osadą"), [Ja]()
					{
						if (ABitwa* B = Ja.Get()) { for (int32 m = 0; m < B->Milicje().Num(); ++m) { B->Powolaj(m); } }
					})]
					+ SHorizontalBox::Slot().AutoWidth()[Guzik(TEXT("Rozwiąż wszystkie"), TEXT("Wszyscy wracają do pracy"), [Ja]()
					{
						if (ABitwa* B = Ja.Get()) { for (int32 m = 0; m < B->Milicje().Num(); ++m) { B->Rozwiaz(m); } }
					})]
					// Punkt zbiorki (26.11.2026, jak w Manor Lords): klik w teren wyznacza, gdzie biegna powolani.
					+ SHorizontalBox::Slot().AutoWidth()[Guzik(TEXT("Punkt zbiórki"), TEXT("Kliknij miejsce na mapie: tam powołani biegną i się grupują (prawy klik - bez zmian)"), [this, Ja]()
					{
						if (ABitwa* B = Ja.Get()) { B->WybierzZbiorke(); }
						NaZamknij.ExecuteIfBound();
					})]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(STextBlock).Font(Czcionka(9)).ColorAndOpacity(StylUI::Szary).AutoWrapText(true)
					.Text(FText::FromString(TEXT("Każdemu oddziałowi wybierz rodzaj (włócznicy, łucznicy...) i włącz ćwiczenia - wyszkolenie rośnie, a wyszkoleni biją celniej i trudniej uciekają. "
						"Powołani rzucają pracę i biorą broń z zapasów (kuźnia); bez broni dla połowy idą jako ruszenie z narzędziami. "
						"Kobiety pracują dalej. Sterowanie jak w bitwie: lewy klik / ramka - wybór, prawy - idź / atakuj.")))
				]
			]
		]
	];
	Wiersze();
}

void SOknoWojska::Tick(const FGeometry& G, const double Czas, const float Dt)
{
	SCompoundWidget::Tick(G, Czas, Dt);
	if (Bitwa.IsValid() && Bitwa->Wersja() != Wersja)
	{
		Wiersze();
	}
}

void SOknoWojska::Wiersze()
{
	Lista->ClearChildren();
	ABitwa* B = Bitwa.Get();
	if (!B)
	{
		return;
	}
	Wersja = B->Wersja();
	const TWeakObjectPtr<ABitwa> Ja = Bitwa;
	if (B->Milicje().Num() == 0)
	{
		Lista->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Font(Czcionka(10)).ColorAndOpacity(StylUI::Szary)
			.Text(FText::FromString(TEXT("Brak oddziałów. Zrób nowy oddział z wolnych mężczyzn.")))
		];
		return;
	}
	auto Maly = [this](const TCHAR* Tekst, const TCHAR* Podp, TFunction<void()> Akcja, TFunction<bool()> Widac) -> TSharedRef<SWidget>
	{
		return SNew(SBox).Padding(FMargin(4.f, 0.f, 0.f, 0.f))
		[
			SNew(SButton).ButtonStyle(&Styl).ToolTipText(FText::FromString(Podp))
			.Visibility_Lambda([Widac]() { return Widac() ? EVisibility::Visible : EVisibility::Collapsed; })
			.OnClicked_Lambda([Akcja]() { Akcja(); return FReply::Handled(); })
			[
				SNew(STextBlock).Text(FText::FromString(Tekst)).Font(Czcionka(10, true)).ColorAndOpacity(StylUI::Zloto)
			]
		];
	};
	for (int32 m = 0; m < B->Milicje().Num(); ++m)
	{
		auto Powolana = [Ja, m]() { return Ja.IsValid() && Ja->Milicje().IsValidIndex(m) && Ja->Milicje()[m].Powolana(); };
		auto WDomu = [Powolana]() { return !Powolana(); };
		Lista->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox")).BorderBackgroundColor(StylUI::Kafel).Padding(FMargin(8.f, 4.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(Czcionka(11, true)).ColorAndOpacity(StylUI::Napis)
						.Text(FText::FromString(B->Milicje()[m].Nazwa))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(Czcionka(9)).ColorAndOpacity(StylUI::Szary)
						.Text_Lambda([Ja, m]()
						{
							ABitwa* X = Ja.Get();
							if (!X || !X->Milicje().IsValidIndex(m)) { return FText::GetEmpty(); }
							const FMilicja& M = X->Milicje()[m];
							if (!M.Powolana() || !X->Oddzialy().IsValidIndex(M.Oddzial))
							{
								return FText::FromString(FString::Printf(TEXT("%d / %d chłopa · w domu, przy pracy"), M.Ilu(), ABitwa::MaxMilicji));
							}
							const FOddzial& O = X->Oddzialy()[M.Oddzial];
							return FText::FromString(FString::Printf(TEXT("%d chłopa w polu · %s · duch %d%%%s"), O.NaPolu(), ABitwa::OpisStanu(O),
								FMath::RoundToInt32(O.Morale), M.Wlocznie > 0 ? *FString::Printf(TEXT(" · %d broni"), M.Wlocznie) : TEXT("")));
						})
					]
					// P6: szkolenie - kim jest oddzial, czy cwiczy, wyszkolenie.
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SButton).ButtonStyle(&Styl)
							.ToolTipText(FText::FromString(TEXT("Kim ma być oddział (klik: następny rodzaj). Przy powołaniu bierze broń z zapasów: włócznia 1, łuk 1, pałka 0, topór z tarczą 2, kusza 2, miecz 3 na chłopa. Zmiana rodzaju zabiera połowę wyszkolenia.")))
							.IsEnabled_Lambda([WDomu]() { return WDomu(); })
							.OnClicked_Lambda([Ja, m]() { if (ABitwa* X = Ja.Get()) { X->ZmienRodzaj(m); } return FReply::Handled(); })
							[
								SNew(STextBlock).Font(Czcionka(9, true)).ColorAndOpacity(StylUI::Zloto)
								.Text_Lambda([Ja, m]()
								{
									const ABitwa* X = Ja.Get();
									if (!X || !X->Milicje().IsValidIndex(m)) { return FText::GetEmpty(); }
									return FText::FromString(FString::Printf(TEXT("%s ▸"), RodzajWojska(X->Milicje()[m].Rodzaj).Nazwa));
								})
							]
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 0.f, 0.f)
						[
							SNew(SButton).ButtonStyle(&Styl)
							.ToolTipText(FText::FromString(TEXT("Ćwiczenia: wyszkolenie rośnie (do 50% w porę roku; powyżej 50% tylko ze zbrojownią, do 100%). Kosztują 1 jedzenie na 5 chłopa co pół pory. Bez ćwiczeń wyszkolenie powoli spada.")))
							.OnClicked_Lambda([Ja, m]() { if (ABitwa* X = Ja.Get()) { X->PrzelaczCwiczenia(m); } return FReply::Handled(); })
							[
								SNew(STextBlock).Font(Czcionka(9, true))
								.ColorAndOpacity_Lambda([Ja, m]()
								{
									const ABitwa* X = Ja.Get();
									return FSlateColor(X && X->Milicje().IsValidIndex(m) && X->Milicje()[m].bCwiczy ? StylUI::Zloto : StylUI::Szary);
								})
								.Text_Lambda([Ja, m]()
								{
									const ABitwa* X = Ja.Get();
									const bool b = X && X->Milicje().IsValidIndex(m) && X->Milicje()[m].bCwiczy;
									return FText::FromString(b ? TEXT("Ćwiczą ✓") : TEXT("Ćwiczenia"));
								})
							]
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
						[
							SNew(STextBlock).Font(Czcionka(9)).ColorAndOpacity(StylUI::Napis)
							.Text_Lambda([Ja, m]()
							{
								ABitwa* X = Ja.Get();
								if (!X || !X->Milicje().IsValidIndex(m)) { return FText::GetEmpty(); }
								const FMilicja& M = X->Milicje()[m];
								static const TCHAR* Stopnie[] = { TEXT("rekruci"), TEXT("wojacy"), TEXT("weterani") };
								return FText::FromString(FString::Printf(TEXT("wyszkolenie %d%% (%s, najwyżej %d%%) · broni: %d"),
									FMath::RoundToInt32(M.Wyszkolenie), Stopnie[ABitwa::Doswiadczenie(M.Wyszkolenie)], FMath::RoundToInt32(X->MaxWyszkolenia()),
									ABitwa::BroniNaWoja(M.Rodzaj) * M.Ilu()));
							})
						]
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Maly(TEXT("Powołaj"), TEXT("Rzucają pracę i stają przed osadą"), [Ja, m]() { if (ABitwa* X = Ja.Get()) { X->Powolaj(m); } }, WDomu)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Maly(TEXT("Rozwiąż"), TEXT("Wracają do pracy"), [Ja, m]() { if (ABitwa* X = Ja.Get()) { X->Rozwiaz(m); } }, Powolana)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Maly(TEXT("Uzupełnij"), TEXT("Dobierz wolnych mężczyzn do 30"), [Ja, m]() { if (ABitwa* X = Ja.Get()) { X->Uzupelnij(m); } }, WDomu)]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Maly(TEXT("Usuń"), TEXT("Rozwiąż oddział na stałe (ludzie zostają w osadzie)"), [Ja, m]() { if (ABitwa* X = Ja.Get()) { X->UsunMilicje(m); } }, WDomu)]
			]
		];
	}
}
