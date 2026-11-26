// Plik: Source/grarts2d/UI/OknoBudynku.cpp
#include "UI/OknoBudynku.h"
#include "UI/StylUI.h"
#include "Budowy/Budowa.h"
#include "Osada/Osada.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

using namespace StylUI;

namespace
{
	constexpr float Szerokosc = 380.f;

	FSlateFontInfo Czcionka(int32 Rozmiar, bool bGruba = false)
	{
		return FCoreStyle::GetDefaultFontStyle(bGruba ? "Bold" : "Regular", Rozmiar);
	}

	TSharedRef<SWidget> Etykieta(const FText& T, int32 Rozmiar, FLinearColor Kolor, bool bGruba = false)
	{
		return SNew(STextBlock).Text(T).Font(Czcionka(Rozmiar, bGruba)).ColorAndOpacity(Kolor);
	}

	/** Naglowek sekcji: "PRACOWNICY" zlotem. */
	TSharedRef<SWidget> Sekcja(const TCHAR* T)
	{
		return SNew(SBox).Padding(FMargin(0.f, 10.f, 0.f, 4.f))
		[
			Etykieta(FText::FromString(T), 10, Zloto, true)
		];
	}
}

void SOknoBudynku::Construct(const FArguments& InArgs)
{
	OsadaW = InArgs._Osada;
	BudynekW = InArgs._Budynek;
	NaZamknij = InArgs._NaZamknij;

	TloOkna = FSlateRoundedBoxBrush(Tlo, 6.f, Ramka, 1.f);
	TloKafla = FSlateRoundedBoxBrush(Kafel, 4.f, Ramka * 0.9f, 1.f);
	TloKaflaPuste = FSlateRoundedBoxBrush(Kafel * 0.6f, 4.f, Ramka * 0.45f, 1.f);
	Kreska = FSlateRoundedBoxBrush(Ramka * 0.7f, 0.f);
	IkonaB = Ikona(InArgs._Ikona, 56.f);
	IkonaRodzinyB = Ikona(InArgs._IkonaRodziny, 40.f);
	for (int32 i = 0; i < int32(ESurowiec::Ile); ++i)
	{
		IkonySurowcowB[i] = Ikona(InArgs._IkonySurowcow.IsValidIndex(i) ? InArgs._IkonySurowcow[i] : nullptr, 22.f);
	}
	StylPrzycisku = StylUI::StylPrzycisku();
	StylPaska = FProgressBarStyle()
		.SetBackgroundImage(FSlateRoundedBoxBrush(Kafel, 3.f, Ramka * 0.6f, 1.f))
		.SetFillImage(FSlateRoundedBoxBrush(Zloto * 0.8f, 3.f))
		.SetMarqueeImage(FSlateRoundedBoxBrush(Zloto * 0.8f, 3.f));
	StylPaskaStanu = FProgressBarStyle(StylPaska)
		.SetFillImage(FSlateRoundedBoxBrush(Czerwony * 0.85f, 3.f))
		.SetMarqueeImage(FSlateRoundedBoxBrush(Czerwony * 0.85f, 3.f));

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(Szerokosc)
		[
			SNew(SBorder)
			.BorderImage(&TloOkna)
			.Padding(FMargin(14.f, 12.f))
			// Klik w okno nie przechodzi do mapy (inaczej zamykalby okno jak klik w puste pole).
			.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[Naglowek()]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() { return BudynekW.IsValid() ? FText::FromString(BudynekW->Rodzaj().Opis) : FText::GetEmpty(); })
					.Font(Czcionka(10))
					.ColorAndOpacity(Napis * 0.85f)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
				[
					SNew(SBox).HeightOverride(1.f)[SNew(SImage).Image(&Kreska)]
				]
				+ SVerticalBox::Slot().AutoHeight()[CzescRuiny()]
				+ SVerticalBox::Slot().AutoHeight()[CzescStanu()]
				+ SVerticalBox::Slot().AutoHeight()[CzescBudowy()]
				+ SVerticalBox::Slot().AutoHeight()[CzescPracy()]
				+ SVerticalBox::Slot().AutoHeight()[CzescZapasow()]
				+ SVerticalBox::Slot().AutoHeight()[CzescUlepszenia()]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)[CzescRozbiorki()]
			]
		]
	];
}

TSharedRef<SWidget> SOknoBudynku::Naglowek()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
		[
			SNew(SBox).WidthOverride(56.f).HeightOverride(56.f)
			.Visibility(IkonaB.GetResourceObject() ? EVisibility::Visible : EVisibility::Collapsed)
			[
				SNew(SImage).Image(&IkonaB)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return BudynekW.IsValid() ? FText::FromString(BudynekW->Rodzaj().Nazwa) : FText::GetEmpty(); })
				.Font(Czcionka(16, true))
				.ColorAndOpacity(Zloto)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(this, &SOknoBudynku::TytulStanu)
				.Font(Czcionka(10))
				.ColorAndOpacity(Szary)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
		[
			SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(8.f, 2.f))
			.ToolTipText(FText::FromString(TEXT("Zamknij (Esc)")))
			.OnClicked_Lambda([this]() { NaZamknij.ExecuteIfBound(); return FReply::Handled(); })
			[
				Etykieta(FText::FromString(TEXT("X")), 12, Napis, true)
			]
		];
}

FText SOknoBudynku::TytulStanu() const
{
	const ABudowa* B = BudynekW.Get();
	if (!B)
	{
		return FText::GetEmpty();
	}
	const FRodzajBudynku& R = B->Rodzaj();
	if (B->Ruina())
	{
		return FText::FromString(B->Ogien() > 0.f ? TEXT("Ruina: zgliszcza jeszcze się tlą") : TEXT("Ruina: zawalił się"));
	}
	if (B->Ogien() > 0.f)
	{
		return FText::FromString(TEXT("Płonie!"));
	}
	if (!B->Gotowy())
	{
		if (OsadaW.IsValid() && OsadaW->Zablokowana(B))
		{
			return FText::FromString(TEXT("Czeka na materiał: budowniczowie poszli na inne budowy"));
		}
		return FText::FromString(B->CzekaNaMaterial() ? TEXT("Budowa stoi: brak materiału")
			: (B->Przebudowa() ? *FString::Printf(TEXT("Przebudowa na poziom %d"), R.Poziom) : TEXT("W budowie")));
	}
	switch (R.Funkcja)
	{
	case EFunkcja::Mieszkalny: return FText::FromString(TEXT("Dom mieszkalny"));
	case EFunkcja::Magazyn: return FText::FromString(R.Model == TEXT("SM_Ognisko") ? TEXT("Obóz startowy, zapas osadników") : TEXT("Magazyn osady"));
	case EFunkcja::Drwal:
	case EFunkcja::Borowy:
	case EFunkcja::Kamieniolom:
	case EFunkcja::Zbieracz:
	case EFunkcja::Mysliwy:
	case EFunkcja::Rybak:
	case EFunkcja::Pilarnia:
	case EFunkcja::Owczarnia:
	case EFunkcja::Przetworca: return FText::FromString(FString::Printf(TEXT("Budynek pracy, %s"), *R.Kategoria.ToLower()));
	case EFunkcja::Zbrojownia: return FText::FromString(TEXT("Zbrojownia: broń, sukno i woje"));
	case EFunkcja::Pole:
	{
		static const TCHAR* Stany[] = { TEXT("ugór"), TEXT("zaorane"), TEXT("obsiane"), TEXT("zboże rośnie"), TEXT("dojrzałe zboże"), TEXT("ściernisko") };
		return FText::FromString(FString::Printf(TEXT("Pole uprawne: %s"), Stany[FMath::Clamp(int32(B->Uprawa), 0, 5)]));
	}
	default: return FText::FromString(R.Rodzin > 0 ? TEXT("Miejsce dla rodziny") : TEXT(""));
	}
}

TSharedRef<SWidget> SOknoBudynku::CzescBudowy()
{
	TSharedRef<SVerticalBox> V = SNew(SVerticalBox);
	V->AddSlot().AutoHeight()[Sekcja(TEXT("BUDOWA"))];
	V->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SBox).HeightOverride(14.f)
			[
				SNew(SProgressBar)
				.Style(&StylPaska)
				.Percent_Lambda([this]() { return BudynekW.IsValid() ? TOptional<float>(BudynekW->Postep()) : TOptional<float>(0.f); })
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%d%%"), BudynekW.IsValid() ? FMath::FloorToInt32(BudynekW->Postep() * 100.f) : 0)); })
			.Font(Czcionka(11, true))
			.ColorAndOpacity(Napis)
		]
	];
	// 23.11.2026: priorytet budowy (- / +) i anulowanie przebudowy.
	auto ZmienPriorytet = [this](int32 O)
	{
		if (ABudowa* B = BudynekW.Get())
		{
			B->Priorytet = FMath::Clamp(B->Priorytet + O, 1, 5);
		}
		return FReply::Handled();
	};
	V->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				static const TCHAR* Nazwy[] = { TEXT("najniższy"), TEXT("niski"), TEXT("zwykły"), TEXT("wysoki"), TEXT("najwyższy") };
				const int32 P = BudynekW.IsValid() ? FMath::Clamp(BudynekW->Priorytet, 1, 5) : 3;
				return FText::FromString(FString::Printf(TEXT("Priorytet: %s"), Nazwy[P - 1]));
			})
			.Font(Czcionka(11, true))
			.ColorAndOpacity(Napis)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)
		[
			SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(10.f, 2.f))
			.ToolTipText(FText::FromString(TEXT("Niższy priorytet: budowniczowie przyjdą tu dopiero po pilniejszych budowach.")))
			.IsEnabled_Lambda([this]() { return BudynekW.IsValid() && BudynekW->Priorytet > 1; })
			.OnClicked_Lambda([ZmienPriorytet]() { return ZmienPriorytet(-1); })
			[
				Etykieta(FText::FromString(TEXT("-")), 12, Zloto, true)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f, 0.f, 0.f)
		[
			SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(10.f, 2.f))
			.ToolTipText(FText::FromString(TEXT("Wyższy priorytet: budowniczowie idą tu najpierw, nawet z innych budów.")))
			.IsEnabled_Lambda([this]() { return BudynekW.IsValid() && BudynekW->Priorytet < 5; })
			.OnClicked_Lambda([ZmienPriorytet]() { return ZmienPriorytet(1); })
			[
				Etykieta(FText::FromString(TEXT("+")), 12, Zloto, true)
			]
		]
	];
	V->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
	[
		SNew(SHorizontalBox)
		.Visibility_Lambda([this]() { return BudynekW.IsValid() && BudynekW->Przebudowa() ? EVisibility::Visible : EVisibility::Collapsed; })
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(14.f, 4.f))
			.ToolTipText(FText::FromString(TEXT("Przerwij przebudowę. Budynek wraca do poprzedniego poziomu, przyniesiony materiał wraca do składu.")))
			.OnClicked_Lambda([this]()
			{
				if (OsadaW.IsValid())
				{
					OsadaW->AnulujUlepszenie(BudynekW.Get());
				}
				return FReply::Handled();
			})
			[
				Etykieta(FText::FromString(TEXT("Anuluj przebudowę")), 12, Czerwony, true)
			]
		]
	];
	// Material: po wierszu na kazdy surowiec z kosztu.
	for (ESurowiec S : WszystkieSurowce)
	{
		V->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([this, S]() { return BudynekW.IsValid() && BudynekW->Rodzaj().Koszt[S] > 0 ? EVisibility::Visible : EVisibility::Collapsed; })
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SBox).WidthOverride(22.f).HeightOverride(22.f)[SNew(SImage).Image(&IkonySurowcowB[int32(S)])]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				Etykieta(FText::FromString(NazwaSurowca(S)), 11, Napis)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this, S]()
				{
					const ABudowa* B = BudynekW.Get();
					if (!B)
					{
						return FText::GetEmpty();
					}
					const int32 Jest = FMath::Min(B->Dostarczone[S], B->Rodzaj().Koszt[S]);
					FString T = FString::Printf(TEXT("%d / %d"), Jest, B->Rodzaj().Koszt[S]);
					if (B->WDrodze[S] > 0)
					{
						T += FString::Printf(TEXT("  (niosą %d)"), B->WDrodze[S]);
					}
					return FText::FromString(T);
				})
				.Font(Czcionka(11, true))
				.ColorAndOpacity_Lambda([this, S]()
				{
					const ABudowa* B = BudynekW.Get();
					const bool bBrak = B && B->Dostarczone[S] < B->Rodzaj().Koszt[S] && B->WDrodze[S] == 0
						&& !(OsadaW.IsValid() && OsadaW->ZrodloSurowca(S, FVector2D(B->GetActorLocation()), nullptr));
					return FSlateColor(bBrak ? Czerwony : Napis);
				})
			]
		];
	}
	V->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
	[
		SNew(STextBlock)
		.Text_Lambda([this]()
		{
			const ABudowa* B = BudynekW.Get();
			if (!B || !OsadaW.IsValid())
			{
				return FText::GetEmpty();
			}
			for (ESurowiec S : WszystkieSurowce)
			{
				if (B->Brakuje(S) > 0 && !OsadaW->ZrodloSurowca(S, FVector2D(B->GetActorLocation()), nullptr))
				{
					return FText::FromString(FString::Printf(TEXT("Brakuje: %s. Postaw drwala / pilarnię albo poczekaj na dostawę."), *IleSurowca(S, B->Brakuje(S))));
				}
			}
			if (OsadaW->Budujacych(B) == 0 && OsadaW->Wolnych() == 0)
			{
				return FText::FromString(TEXT("Nikt nie buduje: wszyscy osadnicy mają pracę. Zabierz rodzinę z budynku pracy albo podnieś priorytet."));
			}
			return FText::FromString(TEXT("Wolni osadnicy budują i noszą materiał sami."));
		})
		.Font(Czcionka(9))
		.ColorAndOpacity(Szary)
		.AutoWrapText(true)
	];

	return SNew(SBox)
		.Visibility_Lambda([this]() { return BudynekW.IsValid() && !BudynekW->Gotowy() && !BudynekW->Ruina() ? EVisibility::Visible : EVisibility::Collapsed; })
		[V];
}

TSharedRef<SWidget> SOknoBudynku::CzescStanu()
{
	// Wojna (23.11.2026): wytrzymalosc (czerwony pasek) i ogien; widac tylko, gdy budynek jest uszkodzony albo sie pali.
	TSharedRef<SVerticalBox> V = SNew(SVerticalBox);
	V->AddSlot().AutoHeight()[Sekcja(TEXT("STAN"))];
	V->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SBox).HeightOverride(14.f)
			[
				SNew(SProgressBar)
				.Style(&StylPaskaStanu)
				.Percent_Lambda([this]() { return BudynekW.IsValid() ? TOptional<float>(BudynekW->Wytrzymalosc()) : TOptional<float>(0.f); })
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%d%%"), BudynekW.IsValid() ? FMath::CeilToInt32(BudynekW->Wytrzymalosc() * 100.f) : 0)); })
			.Font(Czcionka(11, true))
			.ColorAndOpacity(Napis)
		]
	];
	V->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
	[
		SNew(STextBlock)
		.Text_Lambda([this]()
		{
			const ABudowa* B = BudynekW.Get();
			if (!B || B->Ogien() <= 0.f)
			{
				return FText::FromString(TEXT("Uszkodzony w walce. Przebudowa na wyższy poziom go naprawi."));
			}
			return FText::FromString(FString::Printf(TEXT("Płonie (ogień %d%%). %s"), FMath::RoundToInt32(B->Ogien() * 100.f),
				B->Palnosc() > 0.6f ? TEXT("Drewno i strzecha palą się szybko, ogień może przeskoczyć na sąsiadów.")
					: TEXT("Kamienne mury palą się wolno, ale dach i belki spłoną.")));
		})
		.Font(Czcionka(10))
		.ColorAndOpacity_Lambda([this]() { return FSlateColor(BudynekW.IsValid() && BudynekW->Ogien() > 0.f ? Czerwony : Napis * 0.85f); })
		.AutoWrapText(true)
	];
	return SNew(SBox)
		.Visibility_Lambda([this]()
		{
			const ABudowa* B = BudynekW.Get();
			return B && !B->Ruina() && (B->Ogien() > 0.f || B->Wytrzymalosc() < 0.999f) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[V];
}

TSharedRef<SWidget> SOknoBudynku::CzescRuiny()
{
	TSharedRef<SVerticalBox> V = SNew(SVerticalBox);
	V->AddSlot().AutoHeight()[Sekcja(TEXT("RUINA"))];
	V->AddSlot().AutoHeight()
	[
		SNew(STextBlock)
		.Text_Lambda([this]()
		{
			const ABudowa* B = BudynekW.Get();
			if (!B)
			{
				return FText::GetEmpty();
			}
			TArray<FString> K;
			for (ESurowiec S : WszystkieSurowce)
			{
				if (B->Rodzaj().Koszt[S] > 0)
				{
					K.Add(IleSurowca(S, B->Rodzaj().Koszt[S]));
				}
			}
			const int32 Kamien = B->Rodzaj().Koszt[ESurowiec::Kamien] / 3;
			return FText::FromString(FString::Printf(TEXT("Budynek zawalił się, rodziny i praca przepadły.\nOdbudowa od nowa: %s.\nUprzątnięcie: %s"),
				K.Num() ? *FString::Join(K, TEXT(", ")) : TEXT("bez materiału"),
				Kamien > 0 ? *FString::Printf(TEXT("z gruzu wróci %d kamienia."), Kamien) : TEXT("miejsce będzie wolne.")));
		})
		.Font(Czcionka(10))
		.ColorAndOpacity(Napis * 0.85f)
		.AutoWrapText(true)
	];
	V->AddSlot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 10.f, 0.f)
		[
			SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(14.f, 4.f))
			.ToolTipText(FText::FromString(TEXT("Plac budowy w tym samym miejscu: osadnicy uprzątną gruz, przyniosą materiał i postawią budynek od nowa.")))
			.OnClicked_Lambda([this]()
			{
				if (OsadaW.IsValid())
				{
					OsadaW->Odbuduj(BudynekW.Get());
				}
				return FReply::Handled();
			})
			[
				Etykieta(FText::FromString(TEXT("Odbuduj")), 12, Zloto, true)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(14.f, 4.f))
			.ToolTipText(FText::FromString(TEXT("Usuń ruinę. Część kamienia wraca do składu, miejsce jest wolne pod nowy budynek.")))
			.OnClicked_Lambda([this]()
			{
				if (OsadaW.IsValid() && OsadaW->Uprzatnij(BudynekW.Get()))
				{
					NaZamknij.ExecuteIfBound();
				}
				return FReply::Handled();
			})
			[
				Etykieta(FText::FromString(TEXT("Uprzątnij")), 12, Czerwony, true)
			]
		]
	];
	return SNew(SBox)
		.Visibility_Lambda([this]() { return BudynekW.IsValid() && BudynekW->Ruina() ? EVisibility::Visible : EVisibility::Collapsed; })
		[V];
}

TSharedRef<SWidget> SOknoBudynku::Kafelek(int32 Miejsce)
{
	auto Zajete = [this, Miejsce]()
	{
		TArray<FString> N, C;
		if (OsadaW.IsValid())
		{
			OsadaW->PracujacyW(BudynekW.Get(), N, C);
		}
		return N.IsValidIndex(Miejsce);
	};
	auto Tekst = [this, Miejsce](bool bNazwa)
	{
		TArray<FString> N, C;
		if (OsadaW.IsValid())
		{
			OsadaW->PracujacyW(BudynekW.Get(), N, C);
		}
		if (!N.IsValidIndex(Miejsce))
		{
			return FText::FromString(bNazwa ? TEXT("Wolne miejsce pracy") : TEXT("Kliknij „+ rodzina”, żeby kogoś tu posłać"));
		}
		return FText::FromString(bNazwa ? N[Miejsce] : C[Miejsce]);
	};
	return SNew(SBorder)
		.BorderImage_Lambda([this, Zajete]() -> const FSlateBrush* { return Zajete() ? &TloKafla : &TloKaflaPuste; })
		.Padding(FMargin(8.f, 6.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 10.f, 0.f)
			[
				SNew(SBox).WidthOverride(40.f).HeightOverride(40.f)
				[
					SNew(SImage)
					.Image(&IkonaRodzinyB)
					.ColorAndOpacity_Lambda([Zajete]() { return FSlateColor(Zajete() ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.18f)); })
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text_Lambda([Tekst]() { return Tekst(true); })
					.Font(Czcionka(12, true))
					.ColorAndOpacity_Lambda([Zajete]() { return FSlateColor(Zajete() ? Napis : Szary); })
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text_Lambda([Tekst]() { return Tekst(false); })
					.Font(Czcionka(10))
					.ColorAndOpacity_Lambda([Zajete]() { return FSlateColor(Zajete() ? Zielony : Szary * 0.85f); })
					.AutoWrapText(true)
				]
			]
		];
}

TSharedRef<SWidget> SOknoBudynku::CzescPracy()
{
	const ABudowa* B = BudynekW.Get();
	const int32 Miejsc = B ? B->Rodzaj().Miejsc : 0;
	TSharedRef<SVerticalBox> V = SNew(SVerticalBox);
	V->AddSlot().AutoHeight()[Sekcja(TEXT("PRACOWNICY"))];
	for (int32 i = 0; i < Miejsc; ++i)
	{
		V->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)[Kafelek(i)];
	}
	V->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(12.f, 4.f))
			.ToolTipText(FText::FromString(TEXT("Zabierz rodzinę z tej pracy (wróci do budowania)")))
			.IsEnabled_Lambda([this]() { return OsadaW.IsValid() && OsadaW->RodzinWPracy(BudynekW.Get()) > 0; })
			.OnClicked(this, &SOknoBudynku::Usun)
			[
				Etykieta(FText::FromString(TEXT("- rodzina")), 12, Napis, true)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 12.f, 0.f)
		[
			SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(12.f, 4.f))
			.ToolTipText(FText::FromString(TEXT("Poślij tu najbliższą rodzinę bez pracy")))
			.IsEnabled_Lambda([this]()
			{
				return OsadaW.IsValid() && BudynekW.IsValid() && OsadaW->WolnychRodzin() > 0
					&& OsadaW->RodzinWPracy(BudynekW.Get()) < BudynekW->Rodzaj().Miejsc;
			})
			.OnClicked(this, &SOknoBudynku::Dodaj)
			[
				Etykieta(FText::FromString(TEXT("+ rodzina")), 12, Zloto, true)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(HAlign_Right)
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				return FText::FromString(FString::Printf(TEXT("Rodziny bez pracy: %d"), OsadaW.IsValid() ? OsadaW->WolnychRodzin() : 0));
			})
			.Font(Czcionka(10))
			.ColorAndOpacity(Szary)
		]
	];
	// Borowy: ile mlodych drzew rosnie w jego lesie.
	V->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
	[
		SNew(STextBlock)
		.Visibility(B && B->Rodzaj().Funkcja == EFunkcja::Borowy ? EVisibility::Visible : EVisibility::Collapsed)
		.Text_Lambda([this]()
		{
			const ABudowa* Bb = BudynekW.Get();
			const int32 N = (Bb && OsadaW.IsValid()) ? OsadaW->Sadzonek(FVector2D(Bb->GetActorLocation()), Bb->Rodzaj().Zasieg) : 0;
			return FText::FromString(FString::Printf(TEXT("Młode drzewa rosnące w zasięgu: %d (najwyżej 16)"), N));
		})
		.Font(Czcionka(10))
		.ColorAndOpacity(Napis * 0.85f)
	];

	// Pole (22.11.2026): pasek wzrostu zboza, czas do zniw i spodziewany zbior.
	V->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
	[
		SNew(SVerticalBox)
		.Visibility(B && B->Rodzaj().Funkcja == EFunkcja::Pole ? EVisibility::Visible : EVisibility::Collapsed)
		+ SVerticalBox::Slot().AutoHeight()
		[
			Etykieta(FText::FromString(TEXT("Wzrost zboża")), 10, Szary)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SBox).HeightOverride(12.f)
				[
					SNew(SProgressBar)
					.Style(&StylPaska)
					.Percent_Lambda([this]()
					{
						const ABudowa* Bb = BudynekW.Get();
						if (!Bb) { return TOptional<float>(0.f); }
						return TOptional<float>(Bb->Uprawa == EUprawa::Dojrzale ? 1.f : Bb->Uprawa == EUprawa::Rosnie ? Bb->Wzrost : 0.f);
					})
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					const ABudowa* Bb = BudynekW.Get();
					const float W = !Bb ? 0.f : Bb->Uprawa == EUprawa::Dojrzale ? 1.f : Bb->Uprawa == EUprawa::Rosnie ? Bb->Wzrost : 0.f;
					return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::FloorToInt32(W * 100.f)));
				})
				.Font(Czcionka(11, true))
				.ColorAndOpacity(Napis)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				const ABudowa* Bb = BudynekW.Get();
				if (!Bb || !OsadaW.IsValid())
				{
					return FText::GetEmpty();
				}
				auto Czas = [](float Sekundy)
				{
					const int32 S = FMath::CeilToInt32(Sekundy / FMath::Max(1.f, ABudowa::Tempo));
					return S >= 60 ? FString::Printf(TEXT("%d min %02d s"), S / 60, S % 60) : FString::Printf(TEXT("%d s"), S);
				};
				const float T = OsadaW->DoZniw(Bb);
				const FString Zniwa = T <= 0.f
					? FString::Printf(TEXT("Żniwa trwają: zostało %s do zimy"), *Czas(OsadaW->Czas().DoKoncaPory()))
					: FString::Printf(TEXT("Żniwa za: %s"), *Czas(T));
				const int32 P = OsadaW->SpodziewanyPlon(Bb);
				return FText::FromString(FString::Printf(TEXT("%s\nSpodziewany zbiór: %s"), *Zniwa,
					P > 0 ? *IleSurowca(ESurowiec::Zboze, P) : TEXT("brak (pole nieobsiane)")));
			})
			.Font(Czcionka(10, true))
			.ColorAndOpacity(Napis)
		]
	];

	// Etap 3: pole - co rosnie i co dalej; przetworca - z czego co robi.
	V->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
	[
		SNew(STextBlock)
		.Visibility(B && (B->Rodzaj().Funkcja == EFunkcja::Pole || B->Rodzaj().Funkcja == EFunkcja::Przetworca
			|| B->Rodzaj().Funkcja == EFunkcja::Owczarnia || B->Rodzaj().Funkcja == EFunkcja::Zbrojownia) ? EVisibility::Visible : EVisibility::Collapsed)
		.Text_Lambda([this]()
		{
			const ABudowa* Bb = BudynekW.Get();
			if (!Bb)
			{
				return FText::GetEmpty();
			}
			const FRodzajBudynku& R = Bb->Rodzaj();
			if (R.Funkcja == EFunkcja::Przetworca)
			{
				const FString Wsady = R.Wsad2 == ESurowiec::Ile ? IleSurowca(R.Wsad, 1)
					: FString::Printf(TEXT("%s i %s"), *IleSurowca(R.Wsad, 1), *IleSurowca(R.Wsad2, 1));
				return FText::FromString(FString::Printf(TEXT("Przerabia %s na %s (ok. %.0f s pracy)"),
					*Wsady, *IleSurowca(R.Wyrob, R.Uzysk), R.CzasPracy));
			}
			if (R.Funkcja == EFunkcja::Owczarnia)
			{
				return FText::FromString(FString::Printf(TEXT("Owce w stadzie: %d, z runem do strzyżenia: %d.\nRuno odrasta po ok. %.0f s.\nPastwisko: %s"),
					R.Owiec, Bb->OwiecZRunem(), R.OdrostWelny, *R.OpisObszaru()));
			}
			if (R.Funkcja == EFunkcja::Zbrojownia)
			{
				const int32 Woje = OsadaW.IsValid() ? OsadaW->RodzinWPracy(Bb) : 0;
				return FText::FromString(FString::Printf(TEXT("Woje tej zbrojowni: %d z %d (w każdej rodzinie mąż).\nKażdy bierze włócznię i przeszywanicę z sukna.\nWszystkich wojów w osadzie: %d"),
					Woje, R.Miejsc, OsadaW.IsValid() ? OsadaW->Wojow() : 0));
			}
			const bool bWiosna = OsadaW.IsValid() && OsadaW->Pora() == EPora::Wiosna;
			FString T;
			switch (Bb->Uprawa)
			{
			case EUprawa::Ugor:
				T = bWiosna ? FString::Printf(TEXT("Orka: %d%%"), FMath::Min(100, FMath::FloorToInt32(100.f * Bb->PracaPola / AOsada::OrkaPola(R))))
					: FString(TEXT("Ugór. Wiosną rolnicy zaorzą i obsieją pole."));
				break;
			case EUprawa::Zaorane:
				T = bWiosna ? FString::Printf(TEXT("Zaorane. Siew: %d%%"), FMath::Min(100, FMath::FloorToInt32(100.f * (Bb->PracaPola - AOsada::OrkaPola(R)) / AOsada::SiewPola(R))))
					: (Bb->Zasiano >= 0.05f ? FString::Printf(TEXT("Obsiane w %d%%: plon tylko z tej części."), FMath::RoundToInt32(100.f * Bb->Zasiano))
						: FString(TEXT("Zaorane, ale nieobsiane: plonu nie będzie.")));
				break;
			case EUprawa::Zasiane: T = TEXT("Obsiane. Latem zboże wzejdzie."); break;
			case EUprawa::Rosnie: T = TEXT("Zboże rośnie. Żniwa jesienią."); break;
			case EUprawa::Dojrzale: T = FString::Printf(TEXT("Do zebrania: %s. Zbierz przed zimą!"), *IleSurowca(ESurowiec::Zboze, Bb->DoZebrania)); break;
			default: T = TEXT("Ściernisko: zboże zebrane, pole czeka na wiosnę."); break;
			}
			return FText::FromString(T + FString::Printf(TEXT("\nPełny plon: %s z jednych żniw.\nPowierzchnia: %s"), *IleSurowca(ESurowiec::Zboze, R.Plon), *R.OpisObszaru()));
		})
		.Font(Czcionka(10))
		.ColorAndOpacity(Napis * 0.9f)
		.AutoWrapText(true)
	];

	return SNew(SBox)
		.Visibility_Lambda([this, Miejsc]() { return Miejsc > 0 && BudynekW.IsValid() && BudynekW->Gotowy() ? EVisibility::Visible : EVisibility::Collapsed; })
		[V];
}

TSharedRef<SWidget> SOknoBudynku::CzescZapasow()
{
	const ABudowa* B = BudynekW.Get();
	TSharedRef<SVerticalBox> V = SNew(SVerticalBox);
	V->AddSlot().AutoHeight()[Sekcja(TEXT("ZAPAS"))];
	for (ESurowiec S : WszystkieSurowce)
	{
		if (!B || !B->Rodzaj().MaStos(S))
		{
			continue;
		}
		V->AddSlot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SBox).WidthOverride(22.f).HeightOverride(22.f)[SNew(SImage).Image(&IkonySurowcowB[int32(S)])]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				Etykieta(FText::FromString(NazwaSurowca(S)), 11, Napis)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this, S]()
				{
					const ABudowa* Bb = BudynekW.Get();
					if (!Bb)
					{
						return FText::GetEmpty();
					}
					const FRodzajBudynku& R = Bb->Rodzaj();
					const int32 Poj = (R.Magazynuje() || R.Wyrob == S) ? R.Pojemnosc : 4;
					return FText::FromString(FString::Printf(TEXT("%d / %d"), Bb->Zapas[S], Poj));
				})
				.Font(Czcionka(11, true))
				.ColorAndOpacity(Napis)
			]
		];
	}
	// Mieszkancy (dom, namiot).
	V->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
	[
		SNew(STextBlock)
		.Visibility_Lambda([this]()
		{
			return OsadaW.IsValid() && BudynekW.IsValid() && BudynekW->Rodzaj().Rodzin > 0 && BudynekW->Gotowy() ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.Text_Lambda([this]()
		{
			const TArray<FString> M = OsadaW.IsValid() ? OsadaW->MieszkancyW(BudynekW.Get()) : TArray<FString>();
			if (M.Num())
			{
				return FText::FromString(FString::Printf(TEXT("Mieszka: %s"), *FString::Join(M, TEXT(", "))));
			}
			return FText::FromString(OsadaW.IsValid() && OsadaW->WNamiotach() > 0 ? TEXT("Pusty: zaraz wprowadzi się rodzina z namiotu")
				: TEXT("Pusty: nowa rodzina przyjdzie, gdy zadowolenie osady dojdzie do 60%"));
		})
		.Font(Czcionka(11))
		.ColorAndOpacity(Napis)
		.AutoWrapText(true)
	];

	const bool bCos = B && (B->Rodzaj().MaskaStosow != 0 || B->Rodzaj().Rodzin > 0);
	return SNew(SBox)
		.Visibility_Lambda([this, bCos]() { return bCos && BudynekW.IsValid() && BudynekW->Gotowy() ? EVisibility::Visible : EVisibility::Collapsed; })
		[V];
}

FReply SOknoBudynku::Dodaj()
{
	if (OsadaW.IsValid())
	{
		OsadaW->DodajRodzine(BudynekW.Get());
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SOknoBudynku::CzescRozbiorki()
{
	return SNew(SHorizontalBox)
		.Visibility_Lambda([this]() { return AOsada::MoznaRozebrac(BudynekW.Get()) ? EVisibility::Visible : EVisibility::Collapsed; })
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(14.f, 4.f))
			.ToolTipText_Lambda([this]()
			{
				const ABudowa* B = BudynekW.Get();
				return FText::FromString(B && !B->Gotowy() && !B->Przebudowa()
					? TEXT("Usuń plac budowy. Przyniesiony materiał wraca do składu.")
					: TEXT("Rozbierz budynek. Połowa materiału i zapasy wracają do składu, pracownicy są wolni, mieszkańcy czekają przy obozie na inny dom."));
			})
			.OnClicked_Lambda([this]()
			{
				if (!bPotwierdzRozbiorke)
				{
					bPotwierdzRozbiorke = true; // pierwszy klik: pytanie
					return FReply::Handled();
				}
				if (OsadaW.IsValid() && OsadaW->Rozbierz(BudynekW.Get()))
				{
					NaZamknij.ExecuteIfBound();
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text_Lambda([this]() { return FText::FromString(bPotwierdzRozbiorke ? TEXT("Na pewno? Kliknij jeszcze raz") : TEXT("Rozbierz")); })
				.Font(Czcionka(12, true))
				.ColorAndOpacity(Czerwony)
			]
		];
}

TSharedRef<SWidget> SOknoBudynku::CzescUlepszenia()
{
	auto Nastepny = [this]() -> const FRodzajBudynku*
	{
		const ABudowa* B = BudynekW.Get();
		return B && B->Rodzaj().Ulepszenie ? &B->Rodzaj().Ulepszenie() : nullptr;
	};
	TSharedRef<SVerticalBox> V = SNew(SVerticalBox);
	V->AddSlot().AutoHeight()[Sekcja(TEXT("POZIOM"))];
	V->AddSlot().AutoHeight()
	[
		SNew(STextBlock)
		.Text_Lambda([this, Nastepny]()
		{
			const ABudowa* B = BudynekW.Get();
			if (!B)
			{
				return FText::GetEmpty();
			}
			const FRodzajBudynku* N = Nastepny();
			const int32 Max = B->Rodzaj().NajwyzszyPoziom().Poziom;
			return FText::FromString(N ? FString::Printf(TEXT("Poziom %d z %d. Następny: %s"), B->Rodzaj().Poziom, Max, *N->Nazwa)
				: FString::Printf(TEXT("Poziom %d z %d: najwyższy."), B->Rodzaj().Poziom, Max));
		})
		.Font(Czcionka(11, true))
		.ColorAndOpacity(Napis)
		.AutoWrapText(true)
	];
	V->AddSlot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
	[
		SNew(STextBlock)
		.Visibility_Lambda([Nastepny]() { return Nastepny() ? EVisibility::Visible : EVisibility::Collapsed; })
		.Text_Lambda([Nastepny]()
		{
			const FRodzajBudynku* N = Nastepny();
			if (!N)
			{
				return FText::GetEmpty();
			}
			TArray<FString> K;
			for (ESurowiec S : WszystkieSurowce)
			{
				if (N->Koszt[S] > 0)
				{
					K.Add(IleSurowca(S, N->Koszt[S]));
				}
			}
			FString T = FString::Printf(TEXT("Koszt: %s"), K.Num() ? *FString::Join(K, TEXT(", ")) : TEXT("nic"));
			if (N->Rodzin > 0)
			{
				T += FString::Printf(TEXT("\nMieszka: %d %s"), N->Rodzin, N->Rodzin == 1 ? TEXT("rodzina") : TEXT("rodziny"));
			}
			else if (N->Religia())
			{
				T += FString::Printf(TEXT("\nZadowolenie z kościoła: %d z 20"), N->Poziom >= 3 ? 20 : 16);
			}
			return FText::FromString(T);
		})
		.Font(Czcionka(10))
		.ColorAndOpacity(Napis * 0.85f)
		.AutoWrapText(true)
	];
	V->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
	[
		SNew(SHorizontalBox)
		.Visibility_Lambda([Nastepny]() { return Nastepny() ? EVisibility::Visible : EVisibility::Collapsed; })
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 10.f, 0.f)
		[
			SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(14.f, 4.f))
			.ToolTipText(FText::FromString(TEXT("Przebuduj na wyższy poziom. Rodzina mieszka dalej, osadnicy przyniosą materiał i przebudują.")))
			.IsEnabled_Lambda([this]() { return OsadaW.IsValid() && OsadaW->CzemuNieUlepsz(BudynekW.Get()).IsEmpty(); })
			.OnClicked(this, &SOknoBudynku::Ulepsz)
			[
				Etykieta(FText::FromString(TEXT("Ulepsz")), 12, Zloto, true)
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return FText::FromString(OsadaW.IsValid() ? OsadaW->CzemuNieUlepsz(BudynekW.Get()) : FString()); })
			.Font(Czcionka(9))
			.ColorAndOpacity(Czerwony)
			.AutoWrapText(true)
		]
	];
	return SNew(SBox)
		.Visibility_Lambda([this]()
		{
			return BudynekW.IsValid() && BudynekW->Gotowy() && BudynekW->Rodzaj().Poziom > 0 ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[V];
}

FReply SOknoBudynku::Ulepsz()
{
	if (OsadaW.IsValid())
	{
		OsadaW->Ulepsz(BudynekW.Get());
	}
	return FReply::Handled();
}

FReply SOknoBudynku::Usun()
{
	if (OsadaW.IsValid())
	{
		OsadaW->UsunRodzine(BudynekW.Get());
	}
	return FReply::Handled();
}
