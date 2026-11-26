// Plik: Source/grarts2d/UI/PasekBudowy.cpp
#include "UI/PasekBudowy.h"
#include "Budowy/Budowa.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// Paleta: ciemne drewno i przygaszone zloto (jak reszta interfejsu), karty jak pergamin.
	const FLinearColor Tlo(0.035f, 0.028f, 0.02f, 0.88f);
	const FLinearColor Ramka(0.42f, 0.32f, 0.18f, 1.f);
	const FLinearColor Przycisk(0.10f, 0.08f, 0.055f, 0.95f);
	const FLinearColor PrzyciskNad(0.20f, 0.15f, 0.09f, 1.f);
	const FLinearColor Zloto(0.85f, 0.66f, 0.32f, 1.f);
	const FLinearColor Napis(0.95f, 0.90f, 0.78f, 1.f);
	// Kolory liniowe (Slate): 0,4 to juz jasny pergamin na ekranie; napisy prawie czarny braz dla kontrastu.
	const FLinearColor Pergamin(0.40f, 0.31f, 0.19f, 1.f);
	const FLinearColor PergaminNad(0.50f, 0.40f, 0.25f, 1.f);
	const FLinearColor Brz(0.045f, 0.028f, 0.014f, 1.f);    // ciemny braz: ramka karty, napisy na pergaminie
	const FLinearColor Czerw(0.80f, 0.18f, 0.12f, 1.f);

	/** "8 kłód, 6 desek" (pusty = za darmo). */
	FString KosztTekst(const FRodzajBudynku& R)
	{
		TArray<FString> C;
		for (ESurowiec S : WszystkieSurowce)
		{
			if (R.Koszt[S] > 0)
			{
				C.Add(IleSurowca(S, R.Koszt[S]));
			}
		}
		return FString::Join(C, TEXT(", "));
	}

	FText PodpowiedzBudynku(const FRodzajBudynku& R)
	{
		FString T = R.Nazwa + TEXT("\n") + R.Opis;
		const FString K = KosztTekst(R);
		T += TEXT("\nKoszt: ") + (K.IsEmpty() ? FString(TEXT("nic")) : K);
		if (R.Miejsc > 0)
		{
			T += FString::Printf(TEXT("\nPracuje: %d %s (dodajesz w oknie budynku)"), R.Miejsc, R.Miejsc == 1 ? TEXT("rodzina") : TEXT("rodziny"));
		}
		T += FString::Printf(TEXT("\nBudują: %d os."), R.Budowniczych + R.Tragarzy);
		if (R.Rodzin > 0)
		{
			T += FString::Printf(TEXT("   Mieszka: %d %s"), R.Rodzin, R.Rodzin == 1 ? TEXT("rodzina") : TEXT("rodziny"));
		}
		return FText::FromString(T);
	}

	/** Znak kategorii (nazwa tekstury bez polskich liter; "Wojsko" jako kategoria budynkow to tarcza). */
	FString ZnakKategorii(const FString& K)
	{
		if (K == TEXT("Rzemiosło"))
		{
			return TEXT("Rzemioslo");
		}
		if (K == TEXT("Wojsko"))
		{
			return TEXT("Wojsko_Kat");
		}
		return K;
	}
}

const TArray<FString>& SPasekBudowy::NazwyZnakow()
{
	static const TArray<FString> N = { TEXT("Budowa"), TEXT("Wojsko"), TEXT("Rody"), TEXT("Mieszkalne"), TEXT("Surowce"), TEXT("Jedzenie"),
		TEXT("Rolnictwo"), TEXT("Rzemioslo"), TEXT("Wojsko_Kat"), TEXT("Handel"), TEXT("Religia"), TEXT("Mury") };
	return N;
}

void SPasekBudowy::Construct(const FArguments& InArgs)
{
	NaWybor = InArgs._NaWybor;
	Blokada = InArgs._Blokada;
	Wybrany = InArgs._Wybrany;
	NaWojsko = InArgs._NaWojsko;
	NaRody = InArgs._NaRody;
	NaDrogi = InArgs._NaDrogi;
	DrogiOtwarte = InArgs._DrogiOtwarte;
	WojskoOtwarte = InArgs._WojskoOtwarte;
	RodyOtwarte = InArgs._RodyOtwarte;
	AlarmWojska = InArgs._AlarmWojska;
	AlarmRodow = InArgs._AlarmRodow;

	TloPaska = FSlateRoundedBoxBrush(Tlo, 4.f, Ramka * 0.7f, 1.f);
	RamkaWybranej = FSlateRoundedBoxBrush(FLinearColor::Transparent, 7.f, Zloto, 3.f);
	Kolko = FSlateRoundedBoxBrush(Brz, 15.f, Zloto * 0.8f, 1.5f);
	TloAlarmu = FSlateRoundedBoxBrush(Czerw, 9.f, FLinearColor(0.1f, 0.03f, 0.02f, 1.f), 1.f);
	// Glowne przyciski: ciemne drewno ze zlota ramka (wlaczony: jasniejszy, gruba ramka).
	StylGlowny = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(Przycisk, 10.f, Ramka, 2.f))
		.SetHovered(FSlateRoundedBoxBrush(PrzyciskNad, 10.f, Zloto, 2.f))
		.SetPressed(FSlateRoundedBoxBrush(Przycisk * 0.7f, 10.f, Zloto, 2.f))
		.SetNormalPadding(FMargin(4.f))
		.SetPressedPadding(FMargin(4.f, 5.f, 4.f, 3.f));
	// Kategorie: bez tla, podswietlenie pod kursorem; wybrana z jasniejszym tlem i zlota ramka.
	StylKategorii = FButtonStyle()
		.SetNormal(FSlateNoResource())
		.SetHovered(FSlateRoundedBoxBrush(PrzyciskNad, 4.f, Ramka, 1.f))
		.SetPressed(FSlateRoundedBoxBrush(Przycisk, 4.f, Zloto, 1.f))
		.SetNormalPadding(FMargin(8.f, 3.f))
		.SetPressedPadding(FMargin(8.f, 4.f, 8.f, 2.f));
	StylKategoriiWybranej = FButtonStyle(StylKategorii)
		.SetNormal(FSlateRoundedBoxBrush(PrzyciskNad, 4.f, Zloto, 1.5f))
		.SetHovered(FSlateRoundedBoxBrush(PrzyciskNad * 1.2f, 4.f, Zloto, 1.5f));
	// Karty budynkow: pergamin w ciemnej ramce.
	StylKarty = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(Pergamin, 7.f, Brz, 2.f))
		.SetHovered(FSlateRoundedBoxBrush(PergaminNad, 7.f, Zloto, 2.f))
		.SetPressed(FSlateRoundedBoxBrush(Pergamin * 0.85f, 7.f, Zloto, 2.f))
		.SetNormalPadding(FMargin(5.f, 6.f, 5.f, 4.f))
		.SetPressedPadding(FMargin(5.f, 7.f, 5.f, 3.f));

	const TArray<const FRodzajBudynku*>& Lista = FRodzajBudynku::DoBudowania();
	for (int32 i = 0; i < Lista.Num(); ++i)
	{
		Kategorie.AddUnique(Lista[i]->Kategoria);
		UTexture2D* Tex = InArgs._Ikony.IsValidIndex(i) ? InArgs._Ikony[i] : nullptr;
		TSharedPtr<FSlateBrush> B;
		if (Tex)
		{
			B = MakeShared<FSlateBrush>();
			B->SetResourceObject(Tex);
			B->ImageSize = FVector2f(96.f, 96.f);
			B->DrawAs = ESlateBrushDrawType::Image;
		}
		Ikony.Add(B);
	}
	for (int32 i = 0; i < NazwyZnakow().Num(); ++i)
	{
		UTexture2D* Tex = InArgs._Znaki.IsValidIndex(i) ? InArgs._Znaki[i] : nullptr;
		if (Tex)
		{
			TSharedPtr<FSlateBrush> B = MakeShared<FSlateBrush>();
			B->SetResourceObject(Tex);
			B->ImageSize = FVector2f(32.f, 32.f);
			B->DrawAs = ESlateBrushDrawType::Image;
			Znaki.Add(NazwyZnakow()[i], B);
		}
	}
	if (InArgs._ZnakDrogi)
	{
		TSharedPtr<FSlateBrush> B = MakeShared<FSlateBrush>();
		B->SetResourceObject(InArgs._ZnakDrogi);
		B->ImageSize = FVector2f(32.f, 32.f);
		B->DrawAs = ESlateBrushDrawType::Image;
		Znaki.Add(TEXT("Droga"), B);
	}
	ZbudujRzad();
}

const FSlateBrush* SPasekBudowy::Znak(const FString& Nazwa) const
{
	const TSharedPtr<FSlateBrush>* B = Znaki.Find(Nazwa);
	return B ? B->Get() : FCoreStyle::Get().GetBrush("NoBrush");
}

void SPasekBudowy::OtworzKategorie(const FString& Kategoria)
{
	Otwarta = Kategoria;
	bBudowa = !Kategoria.IsEmpty();
	if (bBudowa)
	{
		Ostatnia = Kategoria;
	}
	ZbudujRzad();
}

void SPasekBudowy::PrzelaczBudowe()
{
	if (bBudowa)
	{
		NaWybor.ExecuteIfBound(nullptr);
		OtworzKategorie(FString());
	}
	else
	{
		OtworzKategorie(Ostatnia.IsEmpty() && Kategorie.Num() ? Kategorie[0] : Ostatnia);
	}
}

FReply SPasekBudowy::KlikKategorii(FString Kategoria)
{
	OtworzKategorie(Kategoria);
	return FReply::Handled();
}

FReply SPasekBudowy::KlikBudynku(const FRodzajBudynku* R)
{
	NaWybor.ExecuteIfBound(Wybrany.Get() == R ? nullptr : R);
	return FReply::Handled();
}

TSharedRef<SWidget> SPasekBudowy::GlownyPrzycisk(const FString& NazwaZnaku, const FString& NapisP, const FString& Podpowiedz,
	TAttribute<bool> Otwarte, TAttribute<FString> AlarmP, FSimpleDelegate Klik)
{
	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SButton)
			.ButtonStyle(&StylGlowny)
			.ToolTipText_Lambda([Podpowiedz, AlarmP]()
			{
				const FString A = AlarmP.Get();
				return FText::FromString(A.IsEmpty() ? Podpowiedz : Podpowiedz + TEXT("\n") + A);
			})
			.OnClicked_Lambda([Klik]() { Klik.ExecuteIfBound(); return FReply::Handled(); })
			[
				SNew(SBox)
				.WidthOverride(74.f)
				.HeightOverride(66.f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 0.f)
					[
						SNew(SBox).WidthOverride(40.f).HeightOverride(40.f)
						[
							SNew(SImage).Image(Znak(NazwaZnaku))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 2.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(NapisP))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity_Lambda([Otwarte]() { return FSlateColor(Otwarte.Get() ? Zloto : Napis * 0.9f); })
					]
				]
			]
		]
		// Otwarte okno: zlota ramka.
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(&RamkaWybranej)
			.Visibility_Lambda([Otwarte]() { return Otwarte.Get() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
		]
		// Alarm: czerwone kolko z "!" w prawym gornym rogu.
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(0.f, -6.f, -6.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(&TloAlarmu)
			.Padding(FMargin(6.f, 0.f))
			.Visibility_Lambda([AlarmP]() { return AlarmP.Get().IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("!")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				.ColorAndOpacity(FLinearColor::White)
			]
		];
}

void SPasekBudowy::ZbudujRzad()
{
	// Karty budynkow otwartej kategorii.
	Rzad = SNew(SHorizontalBox);
	const TArray<const FRodzajBudynku*>& Lista = FRodzajBudynku::DoBudowania();
	for (int32 i = 0; i < Lista.Num(); ++i)
	{
		const FRodzajBudynku* R = Lista[i];
		if (R->Kategoria != Otwarta)
		{
			continue;
		}
		TSharedPtr<SWidget> Obraz;
		if (Ikony[i].IsValid())
		{
			TSharedRef<SImage> I = SNew(SImage).Image(Ikony[i].Get());
			Obraz = I;
		}
		else
		{
			TSharedRef<SBorder> Puste = SNew(SBorder).BorderImage(&TloPaska); // brak ikony (nie zaimportowana)
			Obraz = Puste;
		}
		const FString Koszt = KosztTekst(*R);
		Rzad->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Bottom)
		.Padding(5.f, 0.f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.Padding(0.f, 0.f, 0.f, 14.f)
			[
				SNew(SButton)
				.ButtonStyle(&StylKarty)
				.OnClicked(this, &SPasekBudowy::KlikBudynku, R)
				.IsEnabled_Lambda([this, R]() { return !Blokada.IsBound() || Blokada.Execute(*R).IsEmpty(); })
				.ToolTipText_Lambda([this, R]()
				{
					const FString B = Blokada.IsBound() ? Blokada.Execute(*R) : FString();
					return B.IsEmpty() ? PodpowiedzBudynku(*R) : FText::FromString(B + TEXT("\n\n") + PodpowiedzBudynku(*R).ToString());
				})
				[
					SNew(SBox)
					.WidthOverride(112.f)
					.HeightOverride(156.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
						[
							SNew(SBox).WidthOverride(96.f).HeightOverride(96.f)[Obraz.ToSharedRef()]
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 1.f, 0.f, 0.f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(R->Nazwa))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							.ColorAndOpacity(Brz)
							.Justification(ETextJustify::Center)
							.AutoWrapText(true)
						]
						+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 1.f, 0.f, 0.f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(Koszt.IsEmpty() ? FString(TEXT("za darmo")) : Koszt))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
							.ColorAndOpacity(FLinearColor(0.09f, 0.06f, 0.03f, 1.f))
							.Justification(ETextJustify::Center)
							.AutoWrapText(true)
						]
					]
				]
			]
			// Ramka wybranej (stawianej) karty.
			+ SOverlay::Slot()
			.Padding(0.f, 0.f, 0.f, 14.f)
			[
				SNew(SBorder)
				.BorderImage(&RamkaWybranej)
				.Visibility_Lambda([this, R]() { return Wybrany.Get() == R ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			]
			// Znak kategorii w kolku na dolnej krawedzi karty.
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Bottom)
			[
				SNew(SBorder)
				.BorderImage(&Kolko)
				.Padding(FMargin(3.f))
				.Visibility(EVisibility::HitTestInvisible)
				[
					SNew(SBox).WidthOverride(24.f).HeightOverride(24.f)
					[
						SNew(SImage).Image(Znak(ZnakKategorii(R->Kategoria)))
					]
				]
			]
		];
	}

	// Pasek kategorii: znaki, podpowiedz z nazwa; z lewej nazwa otwartej kategorii.
	TSharedRef<SHorizontalBox> Pasek = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(10.f, 0.f, 14.f, 0.f)
		[
			SNew(SBox)
			.MinDesiredWidth(96.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Otwarta.ToUpper()))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				.ColorAndOpacity(Zloto)
			]
		];
	for (const FString& K : Kategorie)
	{
		Pasek->AddSlot()
		.AutoWidth()
		.Padding(4.f, 0.f)
		[
			SNew(SButton)
			.ButtonStyle(K == Otwarta ? &StylKategoriiWybranej : &StylKategorii)
			.OnClicked(this, &SPasekBudowy::KlikKategorii, K)
			.ToolTipText(FText::FromString(K))
			[
				SNew(SBox).WidthOverride(30.f).HeightOverride(30.f)
				[
					SNew(SImage)
					.Image(Znak(ZnakKategorii(K)))
					.ColorAndOpacity(K == Otwarta ? FLinearColor::White : FLinearColor(0.85f, 0.85f, 0.85f, 0.9f))
				]
			]
		];
	}
	Pasek->AddSlot().AutoWidth().Padding(14.f, 0.f, 0.f, 0.f)[SNew(SBox).WidthOverride(96.f)];

	const EVisibility WidacBudowe = bBudowa ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
	ChildSlot
	[
		SNew(SVerticalBox)
		// Karty budynkow.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(SBox)
			.Visibility(WidacBudowe)
			[
				Rzad.ToSharedRef()
			]
		]
		// Pasek kategorii.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
			.Visibility(WidacBudowe)
			.BorderImage(&TloPaska)
			.Padding(FMargin(6.f, 4.f))
			[
				Pasek
			]
		]
		// Glowne przyciski.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0.f, 8.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(5.f, 0.f)
			[
				GlownyPrzycisk(TEXT("Budowa"), TEXT("Budowa (B)"), TEXT("Budowa: domy, warsztaty, pola, magazyny"),
					TAttribute<bool>::CreateLambda([this]() { return bBudowa; }), TAttribute<FString>(FString()),
					FSimpleDelegate::CreateLambda([this]() { PrzelaczBudowe(); }))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(5.f, 0.f)
			[
				GlownyPrzycisk(TEXT("Droga"), TEXT("Drogi (G)"), TEXT("Drogi: darmowe, rysowane łukami. Ludzie chodzą nimi o 10% szybciej,\nbudynki przyciągają się do nich frontem"),
					DrogiOtwarte, TAttribute<FString>(FString()), NaDrogi)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(5.f, 0.f)
			[
				GlownyPrzycisk(TEXT("Wojsko"), TEXT("Wojsko (J)"), TEXT("Pospolite ruszenie: oddziały z mężczyzn osady, powołaj / rozwiąż"),
					WojskoOtwarte, AlarmWojska, NaWojsko)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(5.f, 0.f)
			[
				GlownyPrzycisk(TEXT("Rody"), TEXT("Rody (H)"), TEXT("Inne rody w okolicy: handel na targu, podarunki, danina"),
					RodyOtwarte, AlarmRodow, NaRody)
			]
		]
	];
}
