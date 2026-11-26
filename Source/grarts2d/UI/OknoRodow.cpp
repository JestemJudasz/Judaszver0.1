// Plik: Source/grarts2d/UI/OknoRodow.cpp
#include "UI/OknoRodow.h"
#include "UI/StylUI.h"
#include "UI/Herb.h"
#include "Osada/Rody.h"
#include "Osada/Osada.h"
#include "Budowy/Budowa.h"
#include "Kamera/KameraMapy.h"
#include "Wojna/Bitwa.h"
#include "Swiat/TerenKafle.h" // Bitwa.h: UstawTeren potrzebuje pelnego typu
#include "EngineUtils.h"
#include "Brushes/SlateNoResource.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

using namespace StylUI;

namespace
{
	constexpr float SzerokoscListy = 310.f;
	constexpr float SzerokoscRodu = 430.f;
	constexpr int32 Partia = 5; // ile sztuk na jedno klikniecie kup / sprzedaj

	FSlateFontInfo Czcionka(int32 Rozmiar, bool bGruba = false)
	{
		return FCoreStyle::GetDefaultFontStyle(bGruba ? "Bold" : "Regular", Rozmiar);
	}

	TSharedRef<SWidget> Etykieta(const FText& T, int32 Rozmiar, FLinearColor Kolor, bool bGruba = false)
	{
		return SNew(STextBlock).Text(T).Font(Czcionka(Rozmiar, bGruba)).ColorAndOpacity(Kolor);
	}

	TSharedRef<SWidget> Sekcja(const TCHAR* T)
	{
		return SNew(SBox).Padding(FMargin(0.f, 10.f, 0.f, 4.f))
		[
			Etykieta(FText::FromString(T), 10, Zloto, true)
		];
	}

	FLinearColor KolorStosunku(float S)
	{
		return S <= -15.f ? Czerwony : (S >= 15.f ? Zielony : Napis);
	}

	FString Odmiana(int32 N, const TCHAR* Jeden, const TCHAR* Kilka, const TCHAR* Wiele)
	{
		const int32 D = N % 10, S = N % 100;
		return FString::Printf(TEXT("%d %s"), N, N == 1 ? Jeden : ((D >= 2 && D <= 4 && (S < 12 || S > 14)) ? Kilka : Wiele));
	}
}

void SOknoRodow::Construct(const FArguments& InArgs)
{
	RodyW = InArgs._Rody;
	OsadaW = InArgs._Osada;
	NaZamknij = InArgs._NaZamknij;

	TloOkna = FSlateRoundedBoxBrush(Tlo, 6.f, Ramka, 1.f);
	TloKafla = FSlateRoundedBoxBrush(Kafel, 4.f, Ramka * 0.5f, 1.f);
	TloWybranego = FSlateRoundedBoxBrush(PrzyciskNad, 4.f, Zloto, 1.f);
	Kreska = FSlateRoundedBoxBrush(Ramka * 0.7f, 0.f);
	Barwa = FSlateRoundedBoxBrush(FLinearColor::White, 3.f, FLinearColor(0.f, 0.f, 0.f, 0.8f), 1.f);
	IkonaGroszyB = Ikona(InArgs._IkonaGroszy, 20.f);
	for (int32 i = 0; i < int32(ESurowiec::Ile); ++i)
	{
		IkonySurowcowB[i] = Ikona(InArgs._IkonySurowcow.IsValidIndex(i) ? InArgs._IkonySurowcow[i] : nullptr, 20.f);
	}
	StylPrzycisku = StylUI::StylPrzycisku();
	StylWiersza = FButtonStyle()
		.SetNormal(FSlateNoResource())
		.SetHovered(FSlateRoundedBoxBrush(PrzyciskNad * 0.7f, 4.f))
		.SetPressed(FSlateRoundedBoxBrush(PrzyciskNad, 4.f))
		.SetNormalPadding(FMargin(0.f))
		.SetPressedPadding(FMargin(0.f));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(&TloOkna)
		.Padding(FMargin(14.f, 12.f))
		// Klik w okno nie przechodzi do mapy.
		.OnMouseButtonDown_Lambda([](const FGeometry&, const FPointerEvent&) { return FReply::Handled(); })
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[Naglowek()]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(SBox).HeightOverride(1.f)[SNew(SImage).Image(&Kreska)]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(SzerokoscListy)[ListaRodow()]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(12.f, 10.f, 12.f, 0.f)
				[
					SNew(SBox).WidthOverride(1.f)[SNew(SImage).Image(&Kreska)]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(SzerokoscRodu)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()[CzescRodu()]
						+ SVerticalBox::Slot().AutoHeight()[Dyplomacja()]
						+ SVerticalBox::Slot().AutoHeight()[Handel()]
					]
				]
			]
			// Wynik ostatniej sprawy (handel, podarunek, danina).
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Visibility_Lambda([this]() { return Status.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
				.Text_Lambda([this]() { return FText::FromString(Status); })
				.Font(Czcionka(11, true))
				.ColorAndOpacity_Lambda([this]() { return FSlateColor(bStatusDobry ? Zielony : Czerwony); })
				.AutoWrapText(true)
			]
		]
	];
}

void SOknoRodow::UstawStatus(const FString& T, bool bDobrze)
{
	Status = T;
	bStatusDobry = bDobrze;
}

void SOknoRodow::Wynik(bool bOk, const FString& Powod)
{
	if (!Powod.IsEmpty())
	{
		UstawStatus(Powod, bOk);
	}
}

const FRod* SOknoRodow::Rod(int32 I) const
{
	const ARody* A = RodyW.Get();
	return (A && A->Lista().IsValidIndex(I)) ? &A->Lista()[I] : nullptr;
}

bool SOknoRodow::JestTarg() const
{
	return OsadaW.IsValid() && OsadaW->Targ() != nullptr;
}

TSharedRef<SWidget> SOknoRodow::Naglowek()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				Etykieta(FText::FromString(TEXT("Rody i handel")), 16, Zloto, true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					const ARody* A = RodyW.Get();
					if (!A)
					{
						return FText::GetEmpty();
					}
					return FText::FromString(FString::Printf(TEXT("Wasza osada: %s, siła %d (woje x3 + rodziny)"),
						NazwaPoziomu(A->PoziomGracza()), A->SilaGracza()));
				})
				.Font(Czcionka(10))
				.ColorAndOpacity(Szary)
			]
		]
		// Skarbiec: grosze osady.
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
		[
			SNew(SBox).WidthOverride(20.f).HeightOverride(20.f)[SNew(SImage).Image(&IkonaGroszyB)]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 14.f, 0.f)
		[
			SNew(STextBlock)
			.Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%d groszy"), OsadaW.IsValid() ? OsadaW->Grosze : 0)); })
			.Font(Czcionka(13, true))
			.ColorAndOpacity(Napis)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
		[
			SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(8.f, 2.f))
			.ToolTipText(FText::FromString(TEXT("Zamknij (H / Esc)")))
			.OnClicked_Lambda([this]() { NaZamknij.ExecuteIfBound(); return FReply::Handled(); })
			[
				Etykieta(FText::FromString(TEXT("X")), 12, Napis, true)
			]
		];
}

TSharedRef<SWidget> SOknoRodow::ListaRodow()
{
	TSharedRef<SVerticalBox> V = SNew(SVerticalBox);
	V->AddSlot().AutoHeight()[Sekcja(TEXT("RODY W OKOLICY"))];
	const int32 N = RodyW.IsValid() ? RodyW->Lista().Num() : 0;
	for (int32 i = 0; i < N; ++i)
	{
		V->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 3.f)[WierszRodu(i)];
	}
	if (N == 0)
	{
		V->AddSlot().AutoHeight()[Etykieta(FText::FromString(TEXT("Nikt jeszcze nie osiadł w okolicy.")), 10, Szary)];
	}
	return V;
}

TSharedRef<SWidget> SOknoRodow::WierszRodu(int32 I)
{
	return SNew(SButton)
		.ButtonStyle(&StylWiersza)
		.OnClicked_Lambda([this, I]() { Wybrany = I; Status.Reset(); return FReply::Handled(); })
		[
			SNew(SBorder)
			.BorderImage_Lambda([this, I]() { return Wybrany == I ? &TloWybranego : &TloKafla; })
			.Padding(FMargin(8.f, 4.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SBox).WidthOverride(14.f).HeightOverride(18.f)
					[
						SNew(SImage)
						.Image(&Barwa)
						.ColorAndOpacity_Lambda([this, I]() { const FRod* R = Rod(I); return FSlateColor(R ? R->Barwa : FLinearColor::Gray); })
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, I]() { const FRod* R = Rod(I); return FText::FromString(R ? R->Nazwa : FString()); })
						.Font(Czcionka(11, true))
						.ColorAndOpacity(Napis)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this, I]()
						{
							const FRod* R = Rod(I);
							return R ? FText::FromString(FString::Printf(TEXT("%s, %s, %s, %.0f km"), NazwaCechy(R->Cecha), NazwaPoziomu(R->Poziom),
								*Odmiana(R->Rodzin, TEXT("rodzina"), TEXT("rodziny"), TEXT("rodzin")), R->OdlegloscKm)) : FText::GetEmpty();
						})
						.Font(Czcionka(9))
						.ColorAndOpacity(Szary)
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).HAlign(HAlign_Right)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text_Lambda([this, I]() { const FRod* R = Rod(I); return FText::FromString(R ? ARody::OpisStosunku(R->Stosunek) : TEXT("")); })
						.Font(Czcionka(10, true))
						.ColorAndOpacity_Lambda([this, I]() { const FRod* R = Rod(I); return FSlateColor(KolorStosunku(R ? R->Stosunek : 0.f)); })
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text_Lambda([this, I]()
						{
							const FRod* R = Rod(I);
							const TCHAR* T = !R ? TEXT("") : (R->bWojna ? TEXT("WOJNA z wami") : (R->ZadaDaniny > 0 ? TEXT("żąda daniny!") : (R->bWrogi ? TEXT("grozi wojną") : (R->bLennik ? TEXT("wasz lennik") : TEXT("")))));
							return FText::FromString(T);
						})
						.Font(Czcionka(9, true))
						.ColorAndOpacity_Lambda([this, I]() { const FRod* R = Rod(I); return FSlateColor((R && R->bLennik) ? Zloto : Czerwony); })
					]
				]
			]
		];
}

TSharedRef<SWidget> SOknoRodow::CzescRodu()
{
	auto Linia = [this](TFunction<FString(const FRod&)> F)
	{
		return SNew(STextBlock)
			.Text_Lambda([this, F]() { const FRod* R = Rod(); return R ? FText::FromString(F(*R)) : FText::GetEmpty(); })
			.Font(Czcionka(10))
			.ColorAndOpacity(Napis * 0.9f);
	};
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 10.f, 0.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SBox).WidthOverride(18.f).HeightOverride(24.f)
				[
					SNew(SImage)
					.Image(&Barwa)
					.ColorAndOpacity_Lambda([this]() { const FRod* R = Rod(); return FSlateColor(R ? R->Barwa : FLinearColor::Gray); })
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					const FRod* R = Rod();
					return R ? FText::FromString(FString::Printf(TEXT("%s  (ród %s, %s)"), *R->Nazwa, NazwaCechy(R->Cecha), NazwaPoziomu(R->Poziom)))
						: FText::FromString(TEXT("Wybierz ród z listy"));
				})
				.Font(Czcionka(14, true))
				.ColorAndOpacity(Zloto)
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Linia([this](const FRod& R)
			{
				const int32 SG = RodyW.IsValid() ? RodyW->SilaGracza() : 0;
				return FString::Printf(TEXT("%s, %s, siła %d (wasza: %d)"), *Odmiana(R.Rodzin, TEXT("rodzina"), TEXT("rodziny"), TEXT("rodzin")),
					*Odmiana(R.Woje, TEXT("woj"), TEXT("woje"), TEXT("wojów")), R.Sila(), SG);
			})
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Linia([](const FRod& R)
			{
				return FString::Printf(TEXT("Stosunki: %s (%+.0f), grosze rodu: %d, %.0f km stąd"), ARody::OpisStosunku(R.Stosunek), R.Stosunek, R.Grosze, R.OdlegloscKm);
			})
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Linia([](const FRod& R) { return FString::Printf(TEXT("Ostatnio: %s"), *R.Ostatnio); })
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			// AI0: co robi i dlaczego; ZASTOJ, gdy straznik nie widzi postepu od 2 lat gry.
			Linia([](const FRod& R) { return FString::Printf(TEXT("%sDlaczego: %s"), R.bZastoj ? TEXT("ZASTÓJ (2 lata bez postępu)! ") : TEXT(""), *R.Dlaczego); })
		];
}

TSharedRef<SWidget> SOknoRodow::Dyplomacja()
{
	auto Guzik = [this](const FText& Napis_, TFunction<bool()> Wlaczony, TFunction<FText()> Podpowiedz, TFunction<void()> Klik)
	{
		return SNew(SButton)
			.ButtonStyle(&StylPrzycisku)
			.ContentPadding(FMargin(10.f, 3.f))
			.IsEnabled_Lambda([Wlaczony]() { return Wlaczony(); })
			.ToolTipText_Lambda([Podpowiedz]() { return Podpowiedz(); })
			.OnClicked_Lambda([Klik]() { Klik(); return FReply::Handled(); })
			[
				Etykieta(Napis_, 10, Napis, true)
			];
	};
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Sekcja(TEXT("DYPLOMACJA"))]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
			[
				Guzik(FText::FromString(FString::Printf(TEXT("Podarunek (%d gr)"), ARody::KosztPodarunku)),
					[this]() { return Rod() && OsadaW.IsValid() && OsadaW->Grosze >= ARody::KosztPodarunku; },
					[]() { return FText::FromString(TEXT("Grosze w podarunku poprawiają stosunki (kupcy lubią je najbardziej).")); },
					[this]() { FString P; const bool b = RodyW.IsValid() && RodyW->Podarunek(Wybrany, P); Wynik(b, P); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				Guzik(FText::FromString(TEXT("Żądaj daniny")),
					[this]() { return RodyW.IsValid() && RodyW->MoznaZadacDaniny(Wybrany); },
					[this]()
					{
						const FRod* R = Rod();
						if (!R || !RodyW.IsValid())
						{
							return FText::GetEmpty();
						}
						return FText::FromString(R->bLennik ? FString::Printf(TEXT("%s już płacą wam daninę co rok."), *R->Nazwa)
							: FString::Printf(TEXT("Trzeba dwa razy większej siły: co najmniej %d, macie %d. Lennik płaci co rok, ale stosunki spadają."), 2 * R->Sila(), RodyW->SilaGracza()));
					},
					[this]() { FString P; const bool b = RodyW.IsValid() && RodyW->ZadajDaniny(Wybrany, P); Wynik(b, P); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
			[
				// AI4: kamera nad osada rodu w terenie (budynki i choragiew).
				Guzik(FText::FromString(TEXT("Pokaż osadę")),
					[this]() { return Rod() != nullptr; },
					[]() { return FText::FromString(TEXT("Kamera leci nad ich osadę (budynki w terenie, chorągiew w barwie rodu).")); },
					[this]()
					{
						const FRod* R = Rod();
						APlayerController* PC = OsadaW.IsValid() ? OsadaW->GetWorld()->GetFirstPlayerController() : nullptr;
						if (AKameraMapy* K = R && PC ? Cast<AKameraMapy>(PC->GetPawn()) : nullptr)
						{
							K->UstawOgnisko(R->Gdzie);
						}
					})
			]
		]
		// Podboj (23.11.2026): powolane ruszenie idzie pod brame osady rodu (ABitwa::Wyprawa).
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				Guzik(FText::FromString(TEXT("Wyprawa wojenna")),
					[this]() { return Rod() != nullptr; },
					[this]()
					{
						const FRod* R = Rod();
						return R ? FText::FromString(FString::Printf(TEXT("Powołane ruszenie (Wojsko, J) idzie pod bramę: obrońców ok. %d, %s. Bramę trzeba rozrąbać, a po zwycięstwie wybieracie: złupić, hołd z daniną albo spalić."),
							ABitwa::Obroncow(R->Woje, R->Rodzin), R->Mury >= 2 ? TEXT("mur kamienny") : (R->Mury == 1 ? TEXT("palisada") : TEXT("bez murów")))) : FText::GetEmpty();
					},
					[this]()
					{
						ABitwa* B = nullptr;
						if (OsadaW.IsValid())
						{
							for (TActorIterator<ABitwa> It(OsadaW->GetWorld()); It; ++It) { B = *It; break; }
						}
						FString P;
						const bool b = B && B->Wyprawa(Wybrany, P);
						Wynik(b, P.IsEmpty() ? FString(TEXT("Wojsko nie jest gotowe.")) : P);
						if (b)
						{
							NaZamknij.ExecuteIfBound(); // okno zamkniete: widac pole pod brama
						}
					})
			]
			// P6: pokoj za grosze, gdy rod jest w wojnie z wami.
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 0.f, 0.f)
			[
				Guzik(FText::FromString(TEXT("Zawrzyj pokój")),
					[this]() { const FRod* R = Rod(); return R && R->bWojna; },
					[this]()
					{
						const FRod* R = Rod();
						return R && R->bWojna ? FText::FromString(FString::Printf(TEXT("Kończy wojnę za %d groszy; ich armia idąca na was zawraca."), RodyW->CenaPokoju(Wybrany)))
							: FText::FromString(TEXT("Nie ma wojny z tym rodem."));
					},
					[this]() { FString P; const bool b = RodyW.IsValid() && RodyW->ZawrzyjPokoj(Wybrany, P); Wynik(b, P); })
			]
			// Przechwycenie (26.11.2026): ich armia idzie na was - powolane ruszenie wychodzi naprzeciw, bitwa w polu.
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(6.f, 0.f, 0.f, 0.f)
			[
				Guzik(FText::FromString(TEXT("Wyjdź naprzeciw")),
					[this]() { const FRod* R = Rod(); return R && R->bWojna && RodyW.IsValid() && RodyW->ArmiaNaGracza(Wybrany) >= 0; },
					[]() { return FText::FromString(TEXT("Ich armia idzie na waszą osadę: powołane ruszenie (Wojsko, J) wychodzi jej naprzeciw i bije się w polu, zanim dojdzie do oblężenia. Można też kliknąć chorągiew armii na mapie.")); },
					[this]()
					{
						FString P;
						const bool b = RodyW.IsValid() && RodyW->Przechwyc(RodyW->ArmiaNaGracza(Wybrany), P);
						Wynik(b, P);
						if (b)
						{
							NaZamknij.ExecuteIfBound();
						}
					})
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					const FRod* R = Rod();
					return R ? FText::FromString(FString::Printf(TEXT("obrońców ok. %d"), ABitwa::Obroncow(R->Woje, R->Rodzin))) : FText::GetEmpty();
				})
				.Font(Czcionka(9))
				.ColorAndOpacity(Szary)
			]
		]
		// Ich zadanie daniny: zaplac albo odmow (odmowa: wrogi rod, bez handlu, grozi wojna).
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			.Visibility_Lambda([this]() { const FRod* R = Rod(); return (R && R->ZadaDaniny > 0) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					const FRod* R = Rod();
					return R ? FText::FromString(FString::Printf(TEXT("Żądają %d groszy daniny (czas: %.0f s)"), R->ZadaDaniny, R->CzasZadania / FMath::Max(0.1f, ABudowa::Tempo)))
						: FText::GetEmpty();
				})
				.Font(Czcionka(10, true))
				.ColorAndOpacity(Czerwony)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
			[
				Guzik(FText::FromString(TEXT("Zapłać")),
					[this]() { const FRod* R = Rod(); return R && OsadaW.IsValid() && OsadaW->Grosze >= R->ZadaDaniny; },
					[]() { return FText::FromString(TEXT("Stosunki się poprawią.")); },
					[this]() { FString P; const bool b = RodyW.IsValid() && RodyW->ZaplacDanine(Wybrany, P); Wynik(b, P); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.f, 0.f, 0.f, 0.f)
			[
				Guzik(FText::FromString(TEXT("Odmów")),
					[]() { return true; },
					[]() { return FText::FromString(TEXT("Ród stanie się wrogi: zerwie handel i grozi wojną.")); },
					[this]()
					{
						if (RodyW.IsValid())
						{
							RodyW->OdmowDaniny(Wybrany);
							Wynik(false, TEXT("Odmówiono daniny. Ród jest teraz wrogi."));
						}
					})
			]
		];
}

TSharedRef<SWidget> SOknoRodow::Handel()
{
	auto Kolumna = [](float W, const TCHAR* T)
	{
		return SNew(SBox).WidthOverride(W)[Etykieta(FText::FromString(T), 9, Szary, true)];
	};
	TSharedRef<SVerticalBox> Tabela = SNew(SVerticalBox)
		.Visibility_Lambda([this]() { return JestTarg() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; });
	Tabela->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 3.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[Kolumna(118.f, TEXT("TOWAR"))]
		+ SHorizontalBox::Slot().AutoWidth()[Kolumna(46.f, TEXT("MAJĄ"))]
		+ SHorizontalBox::Slot().AutoWidth()[Kolumna(108.f, TEXT("KUP 5"))]
		+ SHorizontalBox::Slot().AutoWidth()[Kolumna(46.f, TEXT("TARG"))]
		+ SHorizontalBox::Slot().AutoWidth()[Kolumna(108.f, TEXT("SPRZEDAJ 5"))]
	];
	for (ESurowiec S : WszystkieSurowce)
	{
		Tabela->AddSlot().AutoHeight().Padding(0.f, 1.f)[WierszTowaru(S)];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[Sekcja(TEXT("HANDEL NA WASZYM TARGU"))]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Visibility_Lambda([this]() { return JestTarg() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
			.Text(FText::FromString(TEXT("Kupcy przyjeżdżają tylko na targ. Postaw targ (zakładka Handel na pasku budowy): tragarze zniosą na niego towar, a tu kupicie i sprzedacie go za grosze.")))
			.Font(Czcionka(10))
			.ColorAndOpacity(Napis * 0.85f)
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Visibility_Lambda([this]() { const FRod* R = Rod(); return (JestTarg() && R && R->bWrogi) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
			.Text(FText::FromString(TEXT("Wrogi ród nie handluje z wami.")))
			.Font(Czcionka(10, true))
			.ColorAndOpacity(Czerwony)
		]
		+ SVerticalBox::Slot().AutoHeight()[Tabela];
}

TSharedRef<SWidget> SOknoRodow::WierszTowaru(ESurowiec S)
{
	auto Na = [this, S]() { return OsadaW.IsValid() && OsadaW->Targ() ? OsadaW->Targ()->Dostepne(S) : 0; };
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(118.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SBox).WidthOverride(20.f).HeightOverride(20.f)[SNew(SImage).Image(&IkonySurowcowB[int32(S)])]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Etykieta(FText::FromString(NazwaSurowca(S)), 10, Napis)
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(46.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this, S]() { return FText::AsNumber(RodyW.IsValid() ? RodyW->NaSprzedaz(Wybrany, S) : 0); })
				.Font(Czcionka(10, true))
				.ColorAndOpacity_Lambda([this, S]() { return FSlateColor((RodyW.IsValid() && RodyW->NaSprzedaz(Wybrany, S) > 0) ? Napis : Szary * 0.8f); })
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(108.f).Padding(FMargin(0.f, 0.f, 8.f, 0.f))
			[
				SNew(SButton)
				.ButtonStyle(&StylPrzycisku)
				.ContentPadding(FMargin(6.f, 1.f))
				.HAlign(HAlign_Center)
				.IsEnabled_Lambda([this, S]() { const FRod* R = Rod(); return R && !R->bWrogi && RodyW->NaSprzedaz(Wybrany, S) > 0; })
				.ToolTipText(FText::FromString(TEXT("Kupcy rodu przywiozą towar na wasz targ (cena za sztukę).")))
				.OnClicked_Lambda([this, S]() { FString P; const bool b = RodyW.IsValid() && RodyW->Kup(Wybrany, S, Partia, P); Wynik(b, P); return FReply::Handled(); })
				[
					SNew(STextBlock)
					.Text_Lambda([this, S]() { return FText::FromString(FString::Printf(TEXT("kup po %d"), RodyW.IsValid() ? RodyW->CenaKupna(Wybrany, S) : 0)); })
					.Font(Czcionka(9, true))
					.ColorAndOpacity(Napis)
				]
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(46.f)
			[
				SNew(STextBlock)
				.Text_Lambda([Na]() { return FText::AsNumber(Na()); })
				.Font(Czcionka(10, true))
				.ColorAndOpacity_Lambda([Na]() { return FSlateColor(Na() > 0 ? Napis : Szary * 0.8f); })
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(108.f)
			[
				SNew(SButton)
				.ButtonStyle(&StylPrzycisku)
				.ContentPadding(FMargin(6.f, 1.f))
				.HAlign(HAlign_Center)
				.IsEnabled_Lambda([this, S, Na]() { const FRod* R = Rod(); return R && !R->bWrogi && Na() > 0; })
				.ToolTipText(FText::FromString(TEXT("Kupcy rodu zabiorą towar z waszego targu (cena za sztukę).")))
				.OnClicked_Lambda([this, S]() { FString P; const bool b = RodyW.IsValid() && RodyW->Sprzedaj(Wybrany, S, Partia, P); Wynik(b, P); return FReply::Handled(); })
				[
					SNew(STextBlock)
					.Text_Lambda([this, S]() { return FText::FromString(FString::Printf(TEXT("sprzedaj po %d"), RodyW.IsValid() ? RodyW->CenaSprzedazy(Wybrany, S) : 0)); })
					.Font(Czcionka(9, true))
					.ColorAndOpacity(Napis)
				]
			]
		];
}

// --- Choragiewki rodow na mapie papierowej ---

void SZnakiRodow::Construct(const FArguments& InArgs)
{
	RodyW = InArgs._Rody;
	OsadaW = InArgs._Osada;
	Znak = FSlateRoundedBoxBrush(FLinearColor::White, 3.f, FLinearColor(0.08f, 0.06f, 0.04f, 1.f), 1.5f);
	Tlo = FSlateRoundedBoxBrush(FLinearColor(0.035f, 0.028f, 0.02f, 0.72f), 4.f);
	SetVisibility(EVisibility::HitTestInvisible);
}

int32 SZnakiRodow::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const ARody* A = RodyW.Get();
	UWorld* W = A ? A->GetWorld() : nullptr;
	APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return LayerId;
	}
	// Rzut na ekran w pikselach okna gry; geometria nakladki ma skale DPI.
	const float Skala = FMath::Max(0.01f, AllottedGeometry.Scale);
	const FSlateFontInfo Gruba = FCoreStyle::GetDefaultFontStyle("Bold", 11);
	const FSlateFontInfo Cienka = FCoreStyle::GetDefaultFontStyle("Regular", 9);
	auto Rysuj = [&](FVector2D Gdzie, FLinearColor Kolor, const FString& Nazwa, const FString& Opis, FLinearColor KolorOpisu, const FHerb& Herb)
	{
		FVector2D Px;
		if (!PC->ProjectWorldLocationToScreen(FVector(Gdzie.X, Gdzie.Y, 0.0), Px, false))
		{
			return;
		}
		const FVector2f P = FVector2f(Px / Skala);
		if (P.X < -100.f || P.Y < -100.f || P.X > AllottedGeometry.GetLocalSize().X + 100.f || P.Y > AllottedGeometry.GetLocalSize().Y + 100.f)
		{
			return;
		}
		// Drzewce i choragiew w barwie rodu, obok nazwa na ciemnym tle.
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(2.f, 22.f), FSlateLayoutTransform(P + FVector2f(-1.f, -22.f))),
			&Tlo, ESlateDrawEffect::None, FLinearColor(0.f, 0.f, 0.f, 1.f));
		Herb.Rysuj(OutDrawElements, LayerId + 1, AllottedGeometry, P + FVector2f(1.f, -23.f), 13.f); // P6: herb rodu zamiast plachty
		const FVector2f T = P + FVector2f(20.f, -26.f);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(8.f * Nazwa.Len() + 16.f, 32.f), FSlateLayoutTransform(T - FVector2f(4.f, 1.f))),
			&Tlo, ESlateDrawEffect::None, FLinearColor::White);
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2f(200.f, 16.f), FSlateLayoutTransform(T)),
			Nazwa, Gruba, ESlateDrawEffect::None, StylUI::Napis);
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(FVector2f(200.f, 14.f), FSlateLayoutTransform(T + FVector2f(0.f, 15.f))),
			Opis, Cienka, ESlateDrawEffect::None, KolorOpisu);
	};
	if (OsadaW.IsValid() && OsadaW->MaOboz())
	{
		Rysuj(OsadaW->Oboz(), StylUI::Zloto, TEXT("Wasza osada"), FString::Printf(TEXT("%s, siła %d"), NazwaPoziomu(A->PoziomGracza()), A->SilaGracza()), StylUI::Zloto, FHerb::Gracza());
	}
	for (const FRod& R : A->Lista())
	{
		const FLinearColor K = R.bWojna || R.ZadaDaniny > 0 || R.bWrogi ? StylUI::Czerwony : (R.bLennik ? StylUI::Zloto : StylUI::Szary);
		Rysuj(R.Gdzie, R.Barwa, R.Nazwa, R.bWojna ? FString::Printf(TEXT("%s, WOJNA z wami"), NazwaPoziomu(R.Poziom))
			: FString::Printf(TEXT("%s, %s"), NazwaPoziomu(R.Poziom), ARody::OpisStosunku(R.Stosunek)), K, FHerb::Rodu(R.Nazwa, R.Barwa));
	}
	// Wyprawy i armie w marszu rysuje SZnakiArmii (UI/ZnakiArmii, P6) na obu mapach.
	return LayerId + 2;
}
