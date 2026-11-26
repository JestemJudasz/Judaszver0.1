// Plik: Source/grarts2d/Siec/GraczZdalny.cpp
#include "Siec/GraczZdalny.h"
#include "Siec/Lacze.h"
#include "Osada/Osada.h"
#include "Osada/OsadaRodu.h"
#include "Budowy/Budowa.h"
#include "Kamera/KameraMapy.h"
#include "UI/StylUI.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	constexpr double CoIleStan = 2.0;   // s czasu rzeczywistego miedzy wiadomosciami stanu
	constexpr int32 LiniiCzatu = 6;
	constexpr int32 MaksCzatu = 200;     // znakow w jednej wiadomosci czatu

	FString DoJson(const TSharedRef<FJsonObject>& O)
	{
		FString S;
		TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> W = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&S);
		FJsonSerializer::Serialize(O, W);
		return S;
	}

	int32 Liczba(const TSharedPtr<FJsonObject>& O, const TCHAR* Pole, int32 Min, int32 Max, int32 Domyslna = 0)
	{
		double V = Domyslna;
		if (!O->TryGetNumberField(Pole, V) || !FMath::IsFinite(V))
		{
			return Domyslna;
		}
		return FMath::Clamp(int32(FMath::Clamp(V, -2.0e9, 2.0e9)), Min, Max);
	}

	FString Napis(const TSharedPtr<FJsonObject>& O, const TCHAR* Pole, int32 MaksZnakow)
	{
		FString S;
		O->TryGetStringField(Pole, S);
		S = S.Left(MaksZnakow);
		S.ReplaceInline(TEXT("\n"), TEXT(" "));
		S.ReplaceInline(TEXT("\r"), TEXT(" "));
		return S.TrimStartAndEnd();
	}

	/** Indeks rodzaju budynku w DoBudowania (z poziomami: dom pruski liczy sie jako chata, jak u rodow). */
	int32 IndeksRodzaju(const FRodzajBudynku& R)
	{
		const TArray<const FRodzajBudynku*>& L = FRodzajBudynku::DoBudowania();
		for (int32 i = 0; i < L.Num(); ++i)
		{
			const FRodzajBudynku* P = L[i];
			for (int32 Krok = 0; P && Krok < 8; ++Krok)
			{
				if (P == &R)
				{
					return i;
				}
				P = P->Ulepszenie ? &P->Ulepszenie() : nullptr;
			}
		}
		for (int32 i = 0; i < L.Num(); ++i)
		{
			if (L[i]->Nazwa == R.Nazwa) // pola i owczarnie dowolnym ksztaltem to kopie rodzaju z ta sama nazwa
			{
				return i;
			}
		}
		return -1;
	}

	const ESurowiec Dary[] = { ESurowiec::Jedzenie, ESurowiec::Kloda, ESurowiec::Deska, ESurowiec::Kamien };
}

// ---------------------------------------------------------------------------------------------------------------
// Panel "Gra w sieci" (lewa strona ekranu).

class SPanelSieci : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPanelSieci) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AGraczZdalny>, Gracz)
	SLATE_END_ARGS()

	void Construct(const FArguments& A)
	{
		Gracz = A._Gracz;
		Styl = StylUI::StylPrzycisku();
		static FSlateRoundedBoxBrush Tlo(StylUI::Tlo, 6.f, StylUI::Ramka, 1.f);
		TWeakObjectPtr<AGraczZdalny> G = Gracz;
		auto Czc = [](int32 R, bool bGruba = false) { return FCoreStyle::GetDefaultFontStyle(bGruba ? "Bold" : "Regular", R); };
		auto Maly = [this, Czc](const FString& T, TFunction<void()> Akcja, const FString& Podp)
		{
			return SNew(SButton).ButtonStyle(&Styl).HAlign(HAlign_Center).ToolTipText(FText::FromString(Podp))
				.OnClicked_Lambda([Akcja]() { Akcja(); return FReply::Handled(); })
				[SNew(STextBlock).Text(FText::FromString(T)).Font(Czc(9, true)).ColorAndOpacity(StylUI::Zloto)];
		};
		TSharedRef<SHorizontalBox> DaryW = SNew(SHorizontalBox);
		DaryW->AddSlot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[Maly(TEXT("20 gr"), [G]() { if (G.IsValid()) { G->DajGrosze(20); } }, TEXT("Daj koledze 20 groszy ze skarbca"))];
		for (ESurowiec S : Dary)
		{
			DaryW->AddSlot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
				[Maly(FString::Printf(TEXT("10 %s"), NazwaSurowca(S)), [G, S]() { if (G.IsValid()) { G->DajTowar(S, 10); } },
					TEXT("Wyślij 10 sztuk z waszego targu na targ kolegi"))];
		}

		ChildSlot
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FMargin(14.f, 100.f, 0.f, 0.f)) // lewa strona pod licznikami obozu (prawa jest dla okna budynku)
		[
			SNew(SBox).WidthOverride(340.f)
			.Visibility_Lambda([G]()
			{
				const FPolaczenie& L = FLacze::Glowne();
				return G.IsValid() && (L.Stan() != FPolaczenie::EStan::Brak || !G->Komunikat().IsEmpty()) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
			})
			[
				SNew(SBorder).BorderImage(&Tlo).Padding(FMargin(12.f, 8.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("GRA W SIECI"))).Font(Czc(11, true)).ColorAndOpacity(StylUI::Zloto)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							Maly(TEXT("–"), [this]() { bZwiniety = !bZwiniety; }, TEXT("Zwiń / rozwiń panel"))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
					[
						SNew(STextBlock).Font(Czc(9)).ColorAndOpacity(StylUI::Szary).AutoWrapText(true)
						.Text_Lambda([]() { return FText::FromString(FLacze::Glowne().Opis()); })
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[
						SNew(SVerticalBox)
						.Visibility_Lambda([this]() { return !bZwiniety && FLacze::Glowne().Polaczony() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Font(Czc(10, true)).ColorAndOpacity(StylUI::Napis).AutoWrapText(true)
							.Text_Lambda([G]()
							{
								if (!G.IsValid() || !G->MaStan())
								{
									return FText::FromString(TEXT("Czekam na stan osady kolegi..."));
								}
								const FRod& K = G->Kolega();
								if (!G->KolegaMaOboz())
								{
									return FText::FromString(FString::Printf(TEXT("%s: jeszcze wybiera miejsce na obóz."), *K.Nazwa));
								}
								const float Km = G->OdlegloscKm();
								return FText::FromString(FString::Printf(TEXT("%s: %s, %d rodzin, %d wojów, %d gr%s"), *K.Nazwa,
									NazwaPoziomu(K.Poziom), K.Rodzin, K.Woje, K.Grosze,
									*(Km >= 0.f ? FString::Printf(TEXT(", %.1f km stąd"), Km) : FString())));
							})
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
							[
								Maly(TEXT("Pokaż osadę kolegi"), [G]() { if (G.IsValid()) { G->PokazKolege(); } }, TEXT("Kamera nad obozem kolegi"))
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
						[
							SNew(STextBlock).Text(FText::FromString(TEXT("Daj koledze:"))).Font(Czc(9)).ColorAndOpacity(StylUI::Szary)
						]
						+ SVerticalBox::Slot().AutoHeight()[DaryW]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 2.f)
						[
							SNew(STextBlock).Font(Czc(9)).ColorAndOpacity(StylUI::Napis).AutoWrapText(true)
							.Text_Lambda([G]() { return FText::FromString(G.IsValid() ? FString::Join(G->Czat(), TEXT("\n")) : FString()); })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
						[
							SAssignNew(Pole, SEditableTextBox)
							.HintText(FText::FromString(TEXT("Napisz do kolegi i wciśnij Enter")))
							.Font(Czc(9))
							.ClearKeyboardFocusOnCommit(true)
							.OnTextCommitted_Lambda([this, G](const FText& T, ETextCommit::Type Jak)
							{
								if (Jak == ETextCommit::OnEnter && G.IsValid() && !T.IsEmpty())
								{
									G->WyslijCzat(T.ToString());
									if (Pole.IsValid()) { Pole->SetText(FText::GetEmpty()); }
								}
							})
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[
						SNew(STextBlock).Font(Czc(9)).ColorAndOpacity(StylUI::Zloto).AutoWrapText(true)
						.Text_Lambda([G]() { return FText::FromString(G.IsValid() ? G->Komunikat() : FString()); })
					]
				]
			]
		];
	}

private:
	TWeakObjectPtr<AGraczZdalny> Gracz;
	FButtonStyle Styl;
	TSharedPtr<SEditableTextBox> Pole;
	bool bZwiniety = false;
};

// ---------------------------------------------------------------------------------------------------------------

AGraczZdalny::AGraczZdalny()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true; // lacze zyje tez w menu i w pauzie
	SetTickableWhenPaused(true);
}

void AGraczZdalny::UstawOsade(AOsada* O)
{
	Osada = O;
}

FString AGraczZdalny::MojeImie()
{
	FString I;
	if (GConfig)
	{
		GConfig->GetString(TEXT("Siec"), TEXT("Imie"), I, GGameUserSettingsIni);
	}
	I = I.TrimStartAndEnd().Left(24);
	return I.IsEmpty() ? FString(FPlatformProcess::UserName(false)).Left(24) : I;
}

void AGraczZdalny::UstawImie(const FString& Imie)
{
	if (GConfig)
	{
		GConfig->SetString(TEXT("Siec"), TEXT("Imie"), *Imie.TrimStartAndEnd().Left(24), GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void AGraczZdalny::BeginPlay()
{
	Super::BeginPlay();
	// Linia polecen (raz na uruchomienie gry, nie po kazdym wczytaniu zapisu): -host albo -dolacz=adres.
	static bool bLinia = false;
	if (!bLinia)
	{
		bLinia = true;
		FString Blad, Adres;
		if (FParse::Param(FCommandLine::Get(), TEXT("host")))
		{
			FLacze::Glowne().Hostuj(Blad);
		}
		else if (FParse::Value(FCommandLine::Get(), TEXT("dolacz="), Adres))
		{
			FLacze::Glowne().Dolacz(Adres, Blad);
		}
		UE_CLOG(!Blad.IsEmpty(), LogTemp, Warning, TEXT("[SIEC] %s"), *Blad);
	}
	if (GEngine && GEngine->GameViewport && (!FParse::Param(FCommandLine::Get(), TEXT("zrzut")) || FParse::Param(FCommandLine::Get(), TEXT("ui"))))
	{
		Panel = SNew(SPanelSieci).Gracz(this);
		GEngine->GameViewport->AddViewportWidgetContent(Panel.ToSharedRef(), 40);
	}
	// Po wczytaniu zapisu (przeladowanie poziomu) lacze dalej dziala: przywitanie od nowa, zeby kolega dostal stan.
	bPrzywitany = false;
}

void AGraczZdalny::EndPlay(const EEndPlayReason::Type Powod)
{
	if (Panel.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Panel.ToSharedRef());
	}
	Panel.Reset();
	if (Powod == EEndPlayReason::Quit || Powod == EEndPlayReason::EndPlayInEditor)
	{
		FLacze::Glowne().Rozlacz(); // pozegnanie: kolega od razu wie, ze wyszlismy
	}
	Super::EndPlay(Powod);
}

void AGraczZdalny::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FPolaczenie& L = FLacze::Glowne();
	L.Tick();
	if (!L.Polaczony())
	{
		if (bByloPolaczone)
		{
			bByloPolaczone = false;
			bPrzywitany = false;
			bMaStan = false;
			Info(FString::Printf(TEXT("Koniec gry w sieci: %s"), *L.Opis()));
			if (Widok)
			{
				Widok->Destroy();
				Widok = nullptr;
			}
		}
		return;
	}
	if (!bByloPolaczone)
	{
		bByloPolaczone = true;
		DopiszCzat(FString::Printf(TEXT("Połączono z %s."), *L.Adres()));
	}
	if (!bPrzywitany)
	{
		bPrzywitany = true;
		TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("t"), TEXT("witaj"));
		O->SetNumberField(TEXT("w"), Wersja);
		O->SetStringField(TEXT("n"), MojeImie());
		L.Wyslij(DoJson(O));
		DoStanu = 0.0;
	}
	FString Linia;
	while (L.Polaczony() && L.Odbierz(Linia))
	{
		Odbierz(Linia);
	}
	const double Teraz = FPlatformTime::Seconds();
	if (L.Polaczony() && Teraz >= DoStanu)
	{
		DoStanu = Teraz + CoIleStan;
		WyslijStan();
	}
}

void AGraczZdalny::WyslijStan()
{
	TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
	O->SetStringField(TEXT("t"), TEXT("stan"));
	O->SetStringField(TEXT("n"), MojeImie());
	// Barwa: host niebieski, dolaczajacy czerwony (choragiew i herb osady u kolegi).
	const FLinearColor B = FLacze::Glowne().Host() ? FLinearColor(0.12f, 0.26f, 0.70f) : FLinearColor(0.70f, 0.12f, 0.10f);
	O->SetNumberField(TEXT("r"), B.R);
	O->SetNumberField(TEXT("g"), B.G);
	O->SetNumberField(TEXT("b"), B.B);
	AOsada* Os = Osada.Get();
	const bool bOboz = Os && Os->MaOboz();
	O->SetBoolField(TEXT("o"), bOboz);
	if (bOboz)
	{
		O->SetNumberField(TEXT("x"), Os->Oboz().X);
		O->SetNumberField(TEXT("y"), Os->Oboz().Y);
		O->SetNumberField(TEXT("rod"), Os->Rodzin());
		O->SetNumberField(TEXT("woj"), Os->Wojow());
		O->SetNumberField(TEXT("gr"), Os->Grosze);
		O->SetNumberField(TEXT("zad"), Os->Zadowolenie());
		O->SetNumberField(TEXT("poz"), Os->Poziom());
		TArray<int32> Ile;
		Ile.Init(0, FRodzajBudynku::DoBudowania().Num());
		int32 Mury = 0;
		for (const TWeakObjectPtr<ABudowa>& W : Os->WszystkieBudynki())
		{
			const ABudowa* Bd = W.Get();
			if (!Bd || !Bd->Gotowy())
			{
				continue;
			}
			const FRodzajBudynku* R = &Bd->Rodzaj();
			if (R == &FRodzajBudynku::Mur() || R == &FRodzajBudynku::WiezaMurowana() || R == &FRodzajBudynku::BramaMurowana())
			{
				Mury = 2;
			}
			else if (R == &FRodzajBudynku::Palisada() || R == &FRodzajBudynku::WiezaPalisadowa() || R == &FRodzajBudynku::BramaPalisadowa())
			{
				Mury = FMath::Max(Mury, 1);
			}
			const int32 i = IndeksRodzaju(*R);
			if (Ile.IsValidIndex(i))
			{
				++Ile[i];
			}
		}
		O->SetNumberField(TEXT("mur"), Mury);
		TArray<TSharedPtr<FJsonValue>> Bud;
		for (int32 N : Ile) { Bud.Add(MakeShared<FJsonValueNumber>(N)); }
		O->SetArrayField(TEXT("bud"), Bud);
		const FSurowce Z = Os->Zapasy();
		TArray<TSharedPtr<FJsonValue>> Zap;
		for (int32 i = 0; i < int32(ESurowiec::Ile); ++i) { Zap.Add(MakeShared<FJsonValueNumber>(Z.Ile[i])); }
		O->SetArrayField(TEXT("zap"), Zap);
	}
	FLacze::Glowne().Wyslij(DoJson(O));
}

void AGraczZdalny::Odbierz(const FString& Linia)
{
	TSharedPtr<FJsonObject> O;
	TSharedRef<TJsonReader<TCHAR>> R = TJsonReaderFactory<TCHAR>::Create(Linia);
	if (!FJsonSerializer::Deserialize(R, O) || !O.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SIEC] Zla wiadomosc (pominieta)"));
		return;
	}
	const FString T = Napis(O, TEXT("t"), 16);
	if (T == TEXT("witaj"))
	{
		const int32 W = Liczba(O, TEXT("w"), 0, 1000000);
		if (W != Wersja)
		{
			FLacze::Glowne().Rozlacz(FString::Printf(TEXT("Kolega ma inną wersję gry (%d, a wy %d). Zagrajcie na tej samej wersji."), W, Wersja));
			return;
		}
		Zdalny.Nazwa = Napis(O, TEXT("n"), 24);
		DopiszCzat(FString::Printf(TEXT("%s dołączył do gry."), *Zdalny.Nazwa));
	}
	else if (T == TEXT("stan"))
	{
		const FString N = Napis(O, TEXT("n"), 24);
		Zdalny.Nazwa = N.IsEmpty() ? FString(TEXT("Kolega")) : N;
		double Rr = 0.5, Gg = 0.5, Bb = 0.5;
		O->TryGetNumberField(TEXT("r"), Rr);
		O->TryGetNumberField(TEXT("g"), Gg);
		O->TryGetNumberField(TEXT("b"), Bb);
		Zdalny.Barwa = FLinearColor(FMath::Clamp(float(Rr), 0.f, 1.f), FMath::Clamp(float(Gg), 0.f, 1.f), FMath::Clamp(float(Bb), 0.f, 1.f));
		bool bOboz = false;
		O->TryGetBoolField(TEXT("o"), bOboz);
		bKolegaOboz = bOboz;
		bMaStan = true;
		if (bOboz)
		{
			double X = 0.0, Y = 0.0;
			O->TryGetNumberField(TEXT("x"), X);
			O->TryGetNumberField(TEXT("y"), Y);
			if (FMath::IsFinite(X) && FMath::IsFinite(Y) && FMath::Abs(X) < 1.0e10 && FMath::Abs(Y) < 1.0e10)
			{
				Zdalny.Gdzie = FVector2D(X, Y);
			}
			Zdalny.Rodzin = Liczba(O, TEXT("rod"), 0, 2000);
			Zdalny.Woje = Liczba(O, TEXT("woj"), 0, 2000);
			Zdalny.Grosze = Liczba(O, TEXT("gr"), 0, 100000000);
			Zdalny.Zadowolenie = float(Liczba(O, TEXT("zad"), 0, 100, 50));
			Zdalny.Poziom = EPoziomOsady(Liczba(O, TEXT("poz"), 0, int32(EPoziomOsady::Miasto)));
			Zdalny.Mury = Liczba(O, TEXT("mur"), 0, 2);
			const int32 Rodzajow = FRodzajBudynku::DoBudowania().Num();
			Zdalny.Budynkow.Init(0, Rodzajow);
			Zdalny.PostepBud.Init(0.f, Rodzajow);
			const TArray<TSharedPtr<FJsonValue>>* Bud = nullptr;
			if (O->TryGetArrayField(TEXT("bud"), Bud))
			{
				for (int32 i = 0; i < FMath::Min(Bud->Num(), Rodzajow); ++i)
				{
					double V = 0.0;
					if ((*Bud)[i].IsValid() && (*Bud)[i]->TryGetNumber(V) && FMath::IsFinite(V))
					{
						Zdalny.Budynkow[i] = FMath::Clamp(int32(FMath::Clamp(V, 0.0, 1000.0)), 0, 60); // widok: najwyzej 60 jednego rodzaju
					}
				}
			}
			const TArray<TSharedPtr<FJsonValue>>* Zap = nullptr;
			Zdalny.Zapas = FSurowce();
			if (O->TryGetArrayField(TEXT("zap"), Zap))
			{
				for (int32 i = 0; i < FMath::Min(Zap->Num(), int32(ESurowiec::Ile)); ++i)
				{
					double V = 0.0;
					if ((*Zap)[i].IsValid() && (*Zap)[i]->TryGetNumber(V) && FMath::IsFinite(V))
					{
						Zdalny.Zapas.Ile[i] = FMath::Clamp(int32(FMath::Clamp(V, 0.0, 1.0e9)), 0, 1000000000);
					}
				}
			}
		}
		OdswiezWidok();
	}
	else if (T == TEXT("czat"))
	{
		const FString S = Napis(O, TEXT("s"), MaksCzatu);
		if (!S.IsEmpty())
		{
			const FString Kto = Zdalny.Nazwa.IsEmpty() ? FString(TEXT("Kolega")) : Zdalny.Nazwa;
			DopiszCzat(FString::Printf(TEXT("%s: %s"), *Kto, *S));
			if (AOsada* Os = Osada.Get()) { Os->Ogloszenie(FString::Printf(TEXT("%s: %s"), *Kto, *S)); }
		}
	}
	else if (T == TEXT("dar") || T == TEXT("zwrot"))
	{
		AOsada* Os = Osada.Get();
		if (!Os)
		{
			return;
		}
		const bool bZwrot = T == TEXT("zwrot");
		const FString Kto = Zdalny.Nazwa.IsEmpty() ? FString(TEXT("Kolega")) : Zdalny.Nazwa;
		const int32 Gr = Liczba(O, TEXT("gr"), 0, 1000);
		if (Gr > 0)
		{
			Os->Grosze += Gr;
			Info(FString::Printf(TEXT("%s przysłał %d groszy."), *Kto, Gr));
		}
		const int32 Si = Liczba(O, TEXT("s"), -1, int32(ESurowiec::Ile) - 1, -1);
		const int32 Ile = Liczba(O, TEXT("ile"), 0, 1000);
		if (Si >= 0 && Ile > 0)
		{
			const ESurowiec S = ESurowiec(Si);
			ABudowa* Targ = Os->Targ();
			const int32 Wchodzi = Targ ? FMath::Clamp(Targ->Miejsce(S), 0, Ile) : 0;
			if (Wchodzi > 0)
			{
				Targ->ZmienZapas(S, Wchodzi);
			}
			if (bZwrot)
			{
				Info(FString::Printf(TEXT("Wróciło na targ: %s (u kolegi nie było miejsca)."), *IleSurowca(S, Wchodzi)));
			}
			else
			{
				Info(Wchodzi > 0 ? FString::Printf(TEXT("%s przysłał na targ: %s."), *Kto, *IleSurowca(S, Wchodzi))
					: FString::Printf(TEXT("%s chciał przysłać %s, ale nie macie targu albo miejsca na nim."), *Kto, *IleSurowca(S, Ile)));
				if (Wchodzi < Ile)
				{
					// Reszta wraca do nadawcy (bez petli: zwrotu juz nie odsylamy).
					TSharedRef<FJsonObject> Z = MakeShared<FJsonObject>();
					Z->SetStringField(TEXT("t"), TEXT("zwrot"));
					Z->SetNumberField(TEXT("s"), Si);
					Z->SetNumberField(TEXT("ile"), Ile - Wchodzi);
					FLacze::Glowne().Wyslij(DoJson(Z));
				}
			}
		}
	}
}

void AGraczZdalny::OdswiezWidok()
{
	AOsada* Os = Osada.Get();
	const bool bDaleko = Os && Os->MaOboz() && FVector2D::Distance(Os->Oboz(), Zdalny.Gdzie) >= MinOdleglosc;
	if (!bKolegaOboz || !bDaleko || !GetWorld())
	{
		if (Widok)
		{
			Widok->Destroy();
			Widok = nullptr;
		}
		return;
	}
	if (Widok && FVector2D::Distance(FVector2D(Widok->GetActorLocation()), Zdalny.Gdzie) > 100.f)
	{
		Widok->Destroy(); // kolega wczytal inny zapis: oboz gdzie indziej
		Widok = nullptr;
	}
	if (!Widok)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Widok = GetWorld()->SpawnActor<AOsadaRodu>(FVector(Zdalny.Gdzie, 0.f), FRotator::ZeroRotator, P);
	}
	if (Widok)
	{
		Widok->Odswiez(Zdalny, Os->Pora());
	}
}

float AGraczZdalny::OdlegloscKm() const
{
	const AOsada* Os = Osada.Get();
	return Os && Os->MaOboz() && bKolegaOboz ? float(FVector2D::Distance(Os->Oboz(), Zdalny.Gdzie) / 1.0e5) : -1.f;
}

FString AGraczZdalny::Komunikat() const
{
	if (bKolegaOboz && Osada.IsValid() && Osada->MaOboz() && OdlegloscKm() * 1.0e5f < MinOdleglosc && FLacze::Glowne().Polaczony())
	{
		return FString::Printf(TEXT("Wasze obozy są za blisko (%.1f km): osady kolegi nie widać. Zacznijcie co najmniej %.0f km od siebie."),
			OdlegloscKm(), MinOdleglosc / 1.0e5f);
	}
	return FPlatformTime::Seconds() - CzasKomunikatu < 15.0 ? OstatniKomunikat : FString();
}

void AGraczZdalny::DopiszCzat(const FString& T)
{
	CzatL.Add(T);
	while (CzatL.Num() > LiniiCzatu)
	{
		CzatL.RemoveAt(0);
	}
}

void AGraczZdalny::Info(const FString& T)
{
	OstatniKomunikat = T;
	CzasKomunikatu = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Log, TEXT("[SIEC] %s"), *T);
}

void AGraczZdalny::WyslijCzat(const FString& Tekst)
{
	FString S = Tekst.Left(MaksCzatu);
	S.ReplaceInline(TEXT("\n"), TEXT(" "));
	S = S.TrimStartAndEnd();
	if (S.IsEmpty() || !FLacze::Glowne().Polaczony())
	{
		return;
	}
	TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
	O->SetStringField(TEXT("t"), TEXT("czat"));
	O->SetStringField(TEXT("s"), S);
	FLacze::Glowne().Wyslij(DoJson(O));
	DopiszCzat(FString::Printf(TEXT("Wy: %s"), *S));
}

void AGraczZdalny::DajGrosze(int32 Ile)
{
	AOsada* Os = Osada.Get();
	if (!Os || !FLacze::Glowne().Polaczony())
	{
		return;
	}
	if (Os->Grosze < Ile)
	{
		Info(TEXT("Za mało groszy w skarbcu."));
		return;
	}
	Os->Grosze -= Ile;
	TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
	O->SetStringField(TEXT("t"), TEXT("dar"));
	O->SetNumberField(TEXT("gr"), Ile);
	FLacze::Glowne().Wyslij(DoJson(O));
	Info(FString::Printf(TEXT("Wysłano koledze %d groszy."), Ile));
}

void AGraczZdalny::DajTowar(ESurowiec S, int32 Ile)
{
	AOsada* Os = Osada.Get();
	if (!Os || !FLacze::Glowne().Polaczony())
	{
		return;
	}
	ABudowa* Targ = Os->Targ();
	if (!Targ)
	{
		Info(TEXT("Dary towarów idą z targu: najpierw postawcie targ."));
		return;
	}
	const int32 N = FMath::Min(Ile, Targ->Dostepne(S));
	if (N <= 0)
	{
		Info(FString::Printf(TEXT("Na targu nie ma: %s. Zanieście towar na targ (zakładka Handel)."), NazwaSurowca(S)));
		return;
	}
	Targ->ZmienZapas(S, -N);
	TSharedRef<FJsonObject> O = MakeShared<FJsonObject>();
	O->SetStringField(TEXT("t"), TEXT("dar"));
	O->SetNumberField(TEXT("s"), int32(S));
	O->SetNumberField(TEXT("ile"), N);
	FLacze::Glowne().Wyslij(DoJson(O));
	Info(FString::Printf(TEXT("Wysłano koledze: %s."), *IleSurowca(S, N)));
}

void AGraczZdalny::PokazKolege()
{
	if (!bKolegaOboz)
	{
		Info(TEXT("Kolega nie ma jeszcze obozu."));
		return;
	}
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (AKameraMapy* Kam = PC ? Cast<AKameraMapy>(PC->GetPawn()) : nullptr)
	{
		Kam->UstawOgnisko(Zdalny.Gdzie);
	}
}
