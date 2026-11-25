// Plik: Source/grarts2d/Osada/Rody.cpp
#include "Osada/Rody.h"
#include "Osada/Osada.h"
#include "Osada/OsadaRodu.h"
#include "Budowy/Budowa.h"
#include "Swiat/SwiatWysokosci.h"

namespace
{
	struct FWzorRodu { const TCHAR* Nazwa; FColor Barwa; ECechaRodu Cecha; };
	// Rody rycerskie (nazwy herbowe), barwy chorągwi.
	const FWzorRodu Wzory[] = {
		{ TEXT("Toporczycy"), FColor(178, 40, 36), ECechaRodu::Wojowniczy },
		{ TEXT("Gryfici"), FColor(214, 170, 52), ECechaRodu::Kupiecki },
		{ TEXT("Odrowążowie"), FColor(60, 92, 170), ECechaRodu::Pobozny },
		{ TEXT("Awdańcy"), FColor(70, 140, 70), ECechaRodu::Kupiecki },
		{ TEXT("Nałęcze"), FColor(220, 220, 210), ECechaRodu::Wojowniczy },
		{ TEXT("Pałucy"), FColor(140, 70, 150), ECechaRodu::Pobozny },
		{ TEXT("Leliwici"), FColor(40, 150, 170), ECechaRodu::Kupiecki },
		{ TEXT("Starykoniowie"), FColor(120, 80, 40), ECechaRodu::Wojowniczy },
	};

	/** Drugi wsad nie liczy sie tu; pilarnia (stara, bez Wsad) tnie klody. */
	ESurowiec WsadAI(const FRodzajBudynku& B)
	{
		return B.Wsad != ESurowiec::Ile ? B.Wsad : (B.Funkcja == EFunkcja::Pilarnia ? ESurowiec::Kloda : ESurowiec::Ile);
	}

	/** Ile sztuk rod chce miec w zapasie (ponizej - drozej kupuje, powyzej - tanio sprzedaje). */
	float Cel(const FRod& R, ESurowiec S)
	{
		return S == ESurowiec::Jedzenie ? 3.f * R.Rodzin + 6.f : 10.f + 2.f * R.Rodzin;
	}

	// Koszt murow rodu (klody na palisade; mur kamienny to kamien i klody na rusztowania).
	constexpr int32 KosztPalisady = 40;
	constexpr int32 KosztMuruKlody = 20;
	constexpr int32 KosztMuruKamienia = 60;
	/** Zapas, ktorego rod nie wydaje na budowy ani przerobke (towar na handel i na czarna godzine). */
	float Rezerwa(const FRod& R, ESurowiec S)
	{
		return (S == ESurowiec::Kloda && ARody::Trudnosc >= 2 ? 0.9f : 0.6f) * Cel(R, S); // AI5: trudny trzyma wiecej klod na zime
	}

	float Niedobor(const FRod& R, ESurowiec S)
	{
		const float C = Cel(R, S);
		return FMath::Clamp((C - R.Zapas[S]) / C, -0.6f, 1.f);
	}

	/** Mnoznik ceny: odleglosc (dalej drozej), stosunki (przyjazny taniej), kupiecki rod troche taniej. */
	float Mnoznik(const FRod& R)
	{
		return (1.f + R.OdlegloscKm / 150.f) * (1.f - R.Stosunek / 500.f) * (R.Cecha == ECechaRodu::Kupiecki ? 0.92f : 1.f);
	}
}

const TCHAR* NazwaPoziomu(EPoziomOsady P)
{
	switch (P)
	{
	case EPoziomOsady::Wies: return TEXT("wieś");
	case EPoziomOsady::OsadaTargowa: return TEXT("osada targowa");
	case EPoziomOsady::Miasto: return TEXT("miasto");
	default: return TEXT("obóz");
	}
}

const TCHAR* NazwaCechy(ECechaRodu C)
{
	switch (C)
	{
	case ECechaRodu::Pobozny: return TEXT("pobożny");
	case ECechaRodu::Wojowniczy: return TEXT("wojowniczy");
	default: return TEXT("kupiecki");
	}
}

EPoziomOsady LiczPoziom(int32 Rodzin, int32 Domow, bool bTarg, bool bKosciol)
{
	if (Rodzin >= 30 && bTarg && bKosciol) { return EPoziomOsady::Miasto; }
	if (Rodzin >= 15 && bTarg) { return EPoziomOsady::OsadaTargowa; }
	if (Rodzin >= 8 && Domow >= 4) { return EPoziomOsady::Wies; }
	return EPoziomOsady::Oboz;
}

int32 CenaBazowa(ESurowiec S)
{
	switch (S)
	{
	case ESurowiec::Kloda: return 2;
	case ESurowiec::Deska: return 4;
	case ESurowiec::Kamien: return 3;
	case ESurowiec::Jedzenie: return 2;
	case ESurowiec::Zboze: return 2;
	case ESurowiec::Maka: return 4;
	case ESurowiec::Welna: return 3;
	case ESurowiec::Sukno: return 8;
	case ESurowiec::Ruda: return 4;
	case ESurowiec::Bron: return 12;
	case ESurowiec::Opal: return 2;
	case ESurowiec::Ryby: return 2;
	case ESurowiec::Skory: return 3;
	case ESurowiec::Skora: return 5;
	case ESurowiec::Buty: return 9;
	case ESurowiec::Ubrania: return 10;
	case ESurowiec::Slod: return 4;
	case ESurowiec::Piwo: return 6;
	default: return 1;
	}
}

bool ARody::bDziennik = false;
int32 ARody::Trudnosc = 1;

ARody::ARody()
{
	Los.Initialize(FMath::Rand());
	bDziennik |= FParse::Param(FCommandLine::Get(), TEXT("aidziennik"));
	FParse::Value(FCommandLine::Get(), TEXT("trudnosc="), Trudnosc);
	Trudnosc = FMath::Clamp(Trudnosc, 0, 2);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Korzen"));
	SetCanBeDamaged(false);
}

float ARody::WydajnoscNaMinute(const FRodzajBudynku& B)
{
	// Sztuki (albo przerobki) na minute z jedna rodzina - mniej wiecej tyle, ile wychodzi ludzikom u gracza
	// (z chodzeniem po towar). Pole liczone osobno (plon jesienia).
	if (WsadAI(B) != ESurowiec::Ile)
	{
		const float T = B.CzasPracy > 0.f ? B.CzasPracy : 20.f;
		return 60.f / (T * 1.8f + 6.f);
	}
	switch (B.Funkcja)
	{
	case EFunkcja::Drwal: return 2.f;
	case EFunkcja::Kamieniolom: return 1.6f;
	case EFunkcja::Zbieracz: return 2.f;
	case EFunkcja::Mysliwy: return 1.4f;
	case EFunkcja::Rybak: return 1.5f;
	case EFunkcja::Owczarnia: return FMath::Min(2.f, B.Owiec * 60.f / FMath::Max(30.f, B.OdrostWelny));
	default: return 0.f;
	}
}

const TCHAR* ARody::OpisStosunku(float S)
{
	if (S <= -50.f) { return TEXT("wrogi"); }
	if (S <= -15.f) { return TEXT("chłodny"); }
	if (S < 15.f) { return TEXT("obojętny"); }
	if (S < 60.f) { return TEXT("przyjazny"); }
	return TEXT("sprzymierzony");
}

void ARody::Ogloszenie(const FString& T)
{
	if (Osada.IsValid())
	{
		Osada->Ogloszenie(T);
	}
	UE_LOG(LogTemp, Log, TEXT("Rody: %s"), *T);
}

int32 ARody::Indeks(const FRodzajBudynku& B) const
{
	const TArray<const FRodzajBudynku*>& L = FRodzajBudynku::DoBudowania();
	for (int32 i = 0; i < L.Num(); ++i)
	{
		if (L[i] == &B)
		{
			return i;
		}
	}
	return -1;
}

int32 ARody::Domowych(const FRod& R) const
{
	// Miejsca w domach (namioty obozu startowego to 5 miejsc ponad to).
	const TArray<const FRodzajBudynku*>& L = FRodzajBudynku::DoBudowania();
	int32 N = 0;
	for (int32 i = 0; i < L.Num(); ++i)
	{
		if (L[i]->Funkcja == EFunkcja::Mieszkalny)
		{
			N += R.Ile(i) * L[i]->Rodzin;
		}
	}
	return N;
}

void ARody::Zaloz(AOsada* O, FVector2D Gracz)
{
	Osada = O;
	Rody.Reset();
	const float Kat0 = Los.FRand() * 360.f;
	for (int32 k = 0; k < int32(UE_ARRAY_COUNT(Wzory)); ++k)
	{
		// Na ladzie, 12-60 km od gracza, kazdy w swoim kierunku (rozrzucone dookola). Z 40 prob najrowniejsze miejsce:
		// najmniejsza roznica wysokosci w kole 90 m, z dala od rzeki (zrzut 23.11.2026: osada na pagorkach wygladala dziwnie).
		FVector2D Gdzie = FVector2D::ZeroVector;
		bool bJest = false;
		float NajRoznica = TNumericLimits<float>::Max();
		for (int32 Proba = 0; Proba < 40; ++Proba)
		{
			const float Kat = FMath::DegreesToRadians(Kat0 + k * 45.f + Los.FRandRange(-18.f, 18.f));
			const float Odl = Los.FRandRange(1.2e6f, 6.0e6f);
			const FVector2D G = Gracz + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * Odl;
			if (FMath::Abs(G.X) >= FSwiat::Polowa - 1.0e5 || FMath::Abs(G.Y) >= FSwiat::Polowa - 1.0e5 || FSwiat::Woda(G.X, G.Y) != FSwiat::EWoda::Lad)
			{
				continue;
			}
			float Lo = TNumericLimits<float>::Max(), Hi = -TNumericLimits<float>::Max();
			bool bLad = true;
			for (int32 r = 0; r <= 3 && bLad; ++r)
			{
				for (int32 a = 0; a < (r == 0 ? 1 : 8) && bLad; ++a)
				{
					const FVector2D Q = G + FVector2D(FMath::Cos(a * PI / 4.f), FMath::Sin(a * PI / 4.f)) * (r * 3000.f);
					float OdRz = 1.0e5f;
					const float H = FSwiat::Wysokosc(Q.X, Q.Y, &OdRz);
					bLad = H > 100.f && OdRz > 1500.f;
					Lo = FMath::Min(Lo, H);
					Hi = FMath::Max(Hi, H);
				}
			}
			if (bLad && Hi - Lo < NajRoznica)
			{
				NajRoznica = Hi - Lo;
				Gdzie = G;
				bJest = true;
			}
		}
		if (!bJest)
		{
			continue;
		}
		FRod R = NowyRod(k);
		R.Gdzie = Gdzie;
		R.OdlegloscKm = float(FVector2D::Distance(Gdzie, Gracz) / 1.0e5);
		// Rody siedza tu dluzej niz gracz: od 3 do 20 minut gry przewagi (te same reguly, tylko wczesniejszy start).
		const float Przewaga = Los.FRandRange(180.f, 1200.f);
		for (float T = 0.f; T < Przewaga; T += Krok)
		{
			KrokRodu(R, Krok);
		}
		Rody.Add(R);
		UE_LOG(LogTemp, Log, TEXT("Rody: %s (%s) %.0f km, %d rodzin, teren nierowny o %.1f m"), *R.Nazwa, NazwaCechy(R.Cecha), R.OdlegloscKm, R.Rodzin, NajRoznica / 100.f);
	}
	UstawStosunki();
	DoRoku = 4.f * FKalendarz::DlugoscPory;
	OdswiezWidoki();
}

FRod ARody::NowyRod(int32 k) const
{
	const int32 NBud = FRodzajBudynku::DoBudowania().Num();
	FRod R;
	R.Nazwa = Wzory[k].Nazwa;
	R.Barwa = FLinearColor(Wzory[k].Barwa);
	R.Cecha = Wzory[k].Cecha;
	R.Rodzin = 5;
	R.Zapas[ESurowiec::Kloda] = AOsada::KlodNaStart;
	R.Zapas[ESurowiec::Jedzenie] = AOsada::JedzeniaNaStart;
	R.Grosze = AOsada::GroszyNaStart;
	R.Budynkow.Init(0, NBud);
	R.PostepBud.Init(0.f, NBud);
	R.Stosunek = R.Cecha == ECechaRodu::Kupiecki ? 10.f : (R.Cecha == ECechaRodu::Wojowniczy ? -10.f : 0.f);
	R.Ostatnio = TEXT("zakłada obóz");
	R.NajRodzin = R.Rodzin;
	return R;
}

TArray<FRod>& ARody::Testowe(int32 Ziarno)
{
	Osada.Reset();
	Los.Initialize(Ziarno);
	Kalendarz.Czas = 0.f;
	OstatniaPora = EPora::Wiosna;
	DoRoku = 4.f * FKalendarz::DlugoscPory;
	Rody.Reset();
	for (int32 k = 0; k < int32(UE_ARRAY_COUNT(Wzory)); ++k)
	{
		Rody.Add(NowyRod(k));
		Rody.Last().OdlegloscKm = 20.f + 5.f * k;
	}
	UstawStosunki();
	Napady.Reset();
	NapadowSlabszych = 0;
	Csv.Reset();
	return Rody;
}

EPora ARody::PoraRodow() const
{
	return Osada.IsValid() ? Osada->Pora() : Kalendarz.Pora();
}

void ARody::Dziennik(FRod& R, const FString& Co)
{
	++R.Wpisow;
	if (bDziennik)
	{
		UE_LOG(LogTemp, Log, TEXT("[AI] rok %d %s | %s (%s) | rodzin %d, woje %d, budynków %d | kłody %d deski %d kamień %d jedzenie %d | %s"),
			Kalendarz.Rok(), FKalendarz::Nazwa(PoraRodow()), *R.Nazwa, NazwaCechy(R.Cecha), R.Rodzin, R.Woje, R.WszystkichBudynkow(),
			R.Zapas[ESurowiec::Kloda], R.Zapas[ESurowiec::Deska], R.Zapas[ESurowiec::Kamien], R.Zapas[ESurowiec::Jedzenie], *Co);
	}
}

void ARody::Pilnuj(FRod& R, float Dt)
{
	// Straznik zastoju (AI0): postep = nowy rekord rodzin, budynkow albo wojow. Tylko mierzy; ratunek przyjdzie w AI1.
	const int32 Bud = R.WszystkichBudynkow();
	if (R.Rodzin > R.NajRodzin || Bud > R.NajBudynkow || R.Woje > R.NajWojow)
	{
		R.NajRodzin = FMath::Max(R.NajRodzin, R.Rodzin);
		R.NajBudynkow = FMath::Max(R.NajBudynkow, Bud);
		R.NajWojow = FMath::Max(R.NajWojow, R.Woje);
		R.BezPostepu = 0.f;
		R.bZastoj = false;
		return;
	}
	R.BezPostepu += Dt;
	if (!R.bZastoj && R.BezPostepu >= CzasZastoju)
	{
		R.bZastoj = true;
		++R.Zastojow;
		const bool bWas = bDziennik;
		bDziennik = true; // zastoj zawsze trafia do logu
		Dziennik(R, FString::Printf(TEXT("ZASTÓJ: 2 lata bez postępu | bo: %s"), *R.Dlaczego));
		bDziennik = bWas;
	}
}

void ARody::NowaPoraRodow()
{
	// Stan kazdego rodu raz na pore: linia w logu i wiersz CSV (wykres wzrostu w Excelu).
	if (Csv.IsEmpty())
	{
		Csv = TEXT("rok;pora;rod;cecha;rodzin;woje;budynkow;klody;deski;kamien;jedzenie;grosze;zadowolenie;zastoj;co\n");
	}
	for (int32 i = 0; i < Rody.Num(); ++i)
	{
		Wladca(i); // AI3
	}
	OdswiezWidoki();
	for (FRod& R : Rody)
	{
		Dziennik(R, FString::Printf(TEXT("%s | bo: %s"), *R.Ostatnio, *R.Dlaczego));
		Csv += FString::Printf(TEXT("%d;%s;%s;%s;%d;%d;%d;%d;%d;%d;%d;%d;%.0f;%d;%s\n"), Kalendarz.Rok(), FKalendarz::Nazwa(PoraRodow()), *R.Nazwa,
			NazwaCechy(R.Cecha), R.Rodzin, R.Woje, R.WszystkichBudynkow(), R.Zapas[ESurowiec::Kloda], R.Zapas[ESurowiec::Deska],
			R.Zapas[ESurowiec::Kamien], R.Zapas[ESurowiec::Jedzenie], R.Grosze, R.Zadowolenie, R.bZastoj ? 1 : 0, *R.Ostatnio);
	}
	if (bDziennik)
	{
		FFileHelper::SaveStringToFile(Csv, *(FPaths::ProjectSavedDir() / TEXT("AI_dziennik.csv")), FFileHelper::EEncodingOptions::ForceUTF8);
	}
}

void ARody::Przewin(float Sekundy)
{
	for (float T = 0.f; T < Sekundy; T += Krok)
	{
		Kalendarz.Czas += Krok;
		if (PoraRodow() != OstatniaPora)
		{
			OstatniaPora = PoraRodow();
			NowaPoraRodow();
		}
		for (FRod& R : Rody)
		{
			KrokRodu(R, Krok);
			Pilnuj(R, Krok);
		}
		DoRoku -= Krok;
		if (DoRoku <= 0.f)
		{
			DoRoku += 4.f * FKalendarz::DlugoscPory;
			Rok();
		}
	}
	OdswiezWidoki(); // w grze pory rodow ida za kalendarzem gracza, wiec po przewinieciu widoki trzeba odswiezyc tu
}

void ARody::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Rody.Num() == 0)
	{
		return;
	}
	DoKroku -= DeltaSeconds * ABudowa::Tempo;
	while (DoKroku <= 0.f)
	{
		DoKroku += Krok;
		Przewin(Krok);
	}
}

void ARody::KrokRodu(FRod& R, float Dt)
{
	const TArray<const FRodzajBudynku*>& L = FRodzajBudynku::DoBudowania();
	Produkuj(R, Dt);

	// Posilek: te same odstepy co u gracza.
	R.DoPosilku -= Dt;
	if (R.DoPosilku <= 0.f)
	{
		R.DoPosilku += AOsada::CzasPosilku;
		Jedz(R);
	}

	// Budowa: placona z gory, konczy sie po czasie z kosztu (CzasBudowy, jak u gracza po balansie 21.11.2026).
	if (R.Buduje >= 0 || R.Buduje == FRod::BudujeMury)
	{
		R.DoKonca -= Dt;
		if (R.DoKonca <= 0.f && R.Buduje == FRod::BudujeMury)
		{
			++R.Mury;
			R.Ostatnio = R.Mury == 1 ? TEXT("zbudował: palisadę wokół osady") : TEXT("zbudował: mur kamienny wokół osady");
			R.Buduje = -1;
		}
		else if (R.DoKonca <= 0.f)
		{
			++R.Budynkow[R.Buduje];
			R.Ostatnio = FString::Printf(TEXT("zbudował: %s"), *L[R.Buduje]->Nazwa);
			R.Buduje = -1;
		}
	}
	else
	{
		if (Trudnosc > 0 || Los.FRand() < 0.4f) // AI5: latwy decyduje wolniej
		{
			Decyduj(R);
		}
	}

	// Rodziny: od 60% zadowolenia i z wolnym miejscem przychodzi nowa, ponizej 30% odchodzi (zostaja najmniej 2).
	R.DoRodziny -= Dt;
	if (R.DoRodziny <= 0.f)
	{
		const bool bMiejsce = R.Rodzin < 5 + Domowych(R);
		if (R.Zadowolenie >= 60.f && bMiejsce)
		{
			++R.Rodzin;
			R.DoRodziny = AOsada::CzasNowejRodziny;
		}
		else if (R.Zadowolenie < 30.f && R.Rodzin > 2)
		{
			--R.Rodzin;
			R.Woje = FMath::Min(R.Woje, R.Rodzin);
			R.DoRodziny = AOsada::CzasOdejscia;
		}
		else
		{
			R.DoRodziny = 10.f;
		}
	}

	// Woje: rodzina w zbrojowni bierze wlocznie (i sukno na przeszywanice, jesli jest).
	const int32 IZbr = Indeks(FRodzajBudynku::Zbrojownia());
	// Glodny rod rozpuszcza wojow (wojowniczy do 1/3 rodzin, inni 1/4; bron wraca do skladu): inaczej wojowniczy stal na 16 rodzinach, polowa w
	// wojsku, nikt do pola i zadnych wolnych rak na nowe budynki jedzenia (test Wojsko, 23.11.2026).
	const int32 MaxWojow = R.Cecha == ECechaRodu::Wojowniczy ? (R.bGlodny ? R.Rodzin / 3 : R.Rodzin / 2) : R.Rodzin / 4;
	if (R.bGlodny && R.Woje > MaxWojow)
	{
		R.Zapas[ESurowiec::Bron] += R.Woje - MaxWojow;
		R.Woje = MaxWojow;
	}
	while (R.Woje < R.Ile(IZbr) * FRodzajBudynku::Zbrojownia().Miejsc && R.Zapas[ESurowiec::Bron] > 0 && R.Woje < MaxWojow)
	{
		++R.Woje;
		R.Zapas[ESurowiec::Bron] -= 1;
		R.Zapas[ESurowiec::Sukno] = FMath::Max(0, R.Zapas[ESurowiec::Sukno] - 1);
	}

	// Nadwyzki rod sprzedaje kupcom z daleka (wolniej bez targu); tak zbiera grosze na zakupy u gracza.
	const bool bTarg = R.Ile(Indeks(FRodzajBudynku::Targ())) > 0;
	for (ESurowiec S : WszystkieSurowce)
	{
		if (R.Zapas[S] > 2.f * Cel(R, S) && Los.FRand() < Dt / (bTarg ? 6.f : 20.f))
		{
			R.Zapas[S] -= 1;
			R.Grosze += FMath::Max(1, CenaBazowa(S) / 2);
		}
	}

	// Stosunki wracaja powoli do tego, co wynika z cechy rodu (odmowa daniny zostaje dlugo w pamieci).
	const bool bKosciolGracza = Osada.IsValid() && (Osada->Gotowych(FRodzajBudynku::Kosciol()) + Osada->Gotowych(FRodzajBudynku::KosciolDrewniany())) > 0;
	float Baza = R.Cecha == ECechaRodu::Kupiecki ? 10.f : (R.Cecha == ECechaRodu::Wojowniczy ? -10.f : (bKosciolGracza ? 15.f : 0.f));
	Baza += R.bLennik ? -10.f : 0.f;
	Baza -= R.bWrogi ? 40.f : 0.f;
	R.Stosunek += FMath::Clamp(Baza - R.Stosunek, -Dt / 60.f, Dt / 60.f);

	// Zadanie daniny: czas na odpowiedz minal = odmowa.
	if (R.ZadaDaniny > 0)
	{
		R.CzasZadania -= Dt;
		if (R.CzasZadania <= 0.f)
		{
			OdmowDaniny(int32(&R - Rody.GetData()));
		}
	}

	const int32 IKos = Indeks(FRodzajBudynku::KosciolDrewniany());
	int32 Domow = 0;
	for (int32 i = 0; i < L.Num(); ++i)
	{
		Domow += L[i]->Funkcja == EFunkcja::Mieszkalny ? R.Ile(i) : 0;
	}
	const EPoziomOsady Nowy = LiczPoziom(R.Rodzin, Domow, bTarg, R.Ile(IKos) > 0);
	const bool bNaLiscie = Rody.Num() > 0 && &R >= Rody.GetData() && &R < Rody.GetData() + Rody.Num(); // nie przy przewadze startowej
	if (Nowy > R.Poziom && bNaLiscie)
	{
		Ogloszenie(FString::Printf(TEXT("%s: ich osada jest teraz: %s"), *R.Nazwa, NazwaPoziomu(Nowy)));
	}
	R.Poziom = Nowy;
}

void ARody::Produkuj(FRod& R, float Dt)
{
	// Rodziny obsadzaja miejsca pracy budynkow (Miejsc, jak u gracza) po kolei z listy, az braknie rodzin; woje nie pracuja.
	const TArray<const FRodzajBudynku*>& L = FRodzajBudynku::DoBudowania();
	const EPora P = PoraRodow();
	int32 Wolne = R.Rodzin - R.Woje;
	// AI2: wojowniczy obsadza najpierw kopalnie rudy i kuznie (inaczej wszystkie rece szly do budynkow wyzej na liscie i broni nie bylo).
	TArray<int32> Kolejnosc;
	if (R.Cecha == ECechaRodu::Wojowniczy && R.Zapas[ESurowiec::Bron] < 4 && R.Woje < R.Rodzin / 2) // tylko gdy broni brak
	{
		Kolejnosc = { Indeks(FRodzajBudynku::KopalniaRudy()), Indeks(FRodzajBudynku::Kuznia()) };
	}
	for (int32 i = 0; i < L.Num(); ++i)
	{
		Kolejnosc.AddUnique(i);
	}
	for (int32 Poz = 0; Poz < Kolejnosc.Num() && Wolne > 0; ++Poz)
	{
		const int32 i = Kolejnosc[Poz];
		const FRodzajBudynku& B = *L[i];
		if (B.Miejsc <= 0 || B.Funkcja == EFunkcja::Zbrojownia)
		{
			continue;
		}
		const int32 N = FMath::Min(R.Ile(i) * FMath::Max(1, B.Miejsc), Wolne);
		if (N <= 0)
		{
			continue;
		}
		Wolne -= N;
		if (B.Wyrob == ESurowiec::Ile)
		{
			continue; // borowy: las rodu sie nie konczy
		}
		float Praca = WydajnoscNaMinute(B) * N * Dt / 60.f;
		if (B.Funkcja == EFunkcja::Pole)
		{
			Praca = P == EPora::Jesien ? float(B.Plon) * FMath::Min(N, R.Ile(i)) * Dt / FKalendarz::DlugoscPory : 0.f; // zniwa jesienia, plon z pola
		}
		if (B.Funkcja == EFunkcja::Zbieracz && P == EPora::Zima)
		{
			Praca = 0.f;
		}
		const ESurowiec W = WsadAI(B);
		float& Post = R.PostepBud[i];
		Post += Praca;
		while (Post >= 1.f)
		{
			if (W != ESurowiec::Ile)
			{
				// Klody przerabia tylko ponad rezerwe (pilarnia i kuznia nie zabiora ostatnich klod na budowy i na zime).
				auto Ma = [&R](ESurowiec X) { return float(R.Zapas[X]) > (X == ESurowiec::Kloda ? Rezerwa(R, X) : 0.f); };
				const bool bJest = Ma(W) && (B.Wsad2 == ESurowiec::Ile || Ma(B.Wsad2));
				if (!bJest)
				{
					Post = 1.f; // czeka na wsad
					break;
				}
				R.Zapas[W] -= 1;
				if (B.Wsad2 != ESurowiec::Ile)
				{
					R.Zapas[B.Wsad2] -= 1;
				}
				R.Zapas[B.Wyrob] += FMath::Max(1, B.Uzysk);
			}
			else
			{
				R.Zapas[B.Wyrob] += 1;
			}
			Post -= 1.f;
		}
	}
}

void ARody::Jedz(FRod& R)
{
	// Porcja na rodzine; zima klode na rodzine. Zadowolenie jak u gracza: jedzenie 40 + cieplo 10 + domy 30 + kosciol 20.
	const int32 Zjedzone = FMath::Min(R.Rodzin, R.Zapas[ESurowiec::Jedzenie]);
	R.Zapas[ESurowiec::Jedzenie] -= Zjedzone;
	float Cieplo = 1.f;
	if (PoraRodow() == EPora::Zima)
	{
		const int32 Klod = FMath::Min(R.Rodzin, R.Zapas[ESurowiec::Kloda]);
		R.Zapas[ESurowiec::Kloda] -= Klod;
		Cieplo = float(Klod) / FMath::Max(1, R.Rodzin);
	}
	const float WDomach = FMath::Min(1.f, float(Domowych(R)) / FMath::Max(1, R.Rodzin));
	const bool bKosciol = R.Ile(Indeks(FRodzajBudynku::KosciolDrewniany())) > 0;
	const float CelZad = 40.f * Zjedzone / FMath::Max(1, R.Rodzin) + 10.f * Cieplo + 30.f * WDomach + (bKosciol ? 20.f : 0.f);
	// Zmienia sie powoli (1 pkt na s), tu skokiem co posilek o najwyzej tyle, ile przez ten czas.
	R.Zadowolenie += FMath::Clamp(CelZad - R.Zadowolenie, -AOsada::CzasPosilku, AOsada::CzasPosilku);
	R.bGlodny = Zjedzone < R.Rodzin;
	if (Zjedzone < R.Rodzin)
	{
		R.Ostatnio = TEXT("głoduje");
	}
}

void ARody::Decyduj(FRod& R)
{
	// Lista priorytetow (plan "Ekonomia gry": brak drewna -> drwal, ludzie w namiotach -> dom, nadwyzka -> rozbudowa
	// albo wojsko); cecha rodu zmienia kolejnosc. Rod stawia pierwszy budynek z listy, na ktory ma material
	// (patrzy na trzy pierwsze, zeby nie odkladac w nieskonczonosc na drogi budynek).
	const TArray<const FRodzajBudynku*>& L = FRodzajBudynku::DoBudowania();
	auto Ile = [&R, this](const FRodzajBudynku& B) { return R.Ile(Indeks(B)); };
	int32 Zatrudnionych = R.Woje;
	float JedzenieNaMin = 0.f;
	for (int32 i = 0; i < L.Num(); ++i)
	{
		if (L[i]->Miejsc > 0 && L[i]->Funkcja != EFunkcja::Zbrojownia)
		{
			Zatrudnionych += R.Ile(i) * L[i]->Miejsc;
		}
		if (L[i]->Wyrob == ESurowiec::Jedzenie)
		{
			JedzenieNaMin += WydajnoscNaMinute(*L[i]) * FMath::Max(1, L[i]->Uzysk) * R.Ile(i) * FMath::Max(1, L[i]->Miejsc);
		}
	}
	const int32 Bez = R.Rodzin - Zatrudnionych;
	const int32 WNamiotach = FMath::Max(0, R.Rodzin - Domowych(R));
	// Mury (23.11.2026): zagrozony rod (ma silniejszego wroga albo byl napadniety) stawia palisade od 10 rodzin
	// (trudny od 8), spokojny od 16; mur kamienny od 20 rodzin z kamieniolomem. Tylko gdy stac go z zapasem i nie glodzi.
	{
		const bool bZagrozony = R.bObrona || R.Napadniety > 0; // wojowniczy najpierw zbroi sie, mury potem
		const int32 Od = (Trudnosc >= 2 ? 8 : 10) + (bZagrozony ? 0 : 6);
		const bool bSyty = R.Zapas[ESurowiec::Jedzenie] >= R.Rodzin; // ma na posilek
		const int32 Klod = R.Mury == 0 ? KosztPalisady : KosztMuruKlody;
		const int32 Kam = R.Mury == 0 ? 0 : KosztMuruKamienia;
		const bool bChce = (R.Mury == 0 && R.Rodzin >= Od) || (R.Mury == 1 && R.Rodzin >= Od + 10 && Ile(FRodzajBudynku::Kamieniolom()) > 0);
		if (bChce && bSyty && R.Zapas[ESurowiec::Kloda] >= Klod + FMath::FloorToInt32(Rezerwa(R, ESurowiec::Kloda) * 0.5f) && R.Zapas[ESurowiec::Kamien] >= Kam)
		{
			R.Zapas[ESurowiec::Kloda] -= Klod;
			R.Zapas[ESurowiec::Kamien] -= Kam;
			R.Buduje = FRod::BudujeMury;
			R.DoKonca = R.Mury == 0 ? 150.f : 300.f;
			R.Ostatnio = R.Mury == 0 ? TEXT("buduje: palisadę") : TEXT("buduje: mur kamienny");
			R.Dlaczego = bZagrozony ? TEXT("obrona: wrogowie w okolicy") : TEXT("duża osada potrzebuje murów");
			Dziennik(R, FString::Printf(TEXT("%s | bo: %s"), *R.Ostatnio, *R.Dlaczego));
			return;
		}
	}
	TArray<const FRodzajBudynku*> Lista;
	TArray<FString> Powody; // AI0: dlaczego dana pozycja jest na liscie (dziennik)
	const TCHAR* Czemu = TEXT("");
	auto Dodaj = [&Lista, &Powody, &Czemu](const FRodzajBudynku& B) { if (!Lista.Contains(&B)) { Lista.Add(&B); Powody.Add(Czemu); } };

	Czemu = TEXT("pobożny ród chce kościoła");
	if (R.Cecha == ECechaRodu::Pobozny && R.Rodzin >= 10 && Ile(FRodzajBudynku::KosciolDrewniany()) == 0)
	{
		Dodaj(FRodzajBudynku::KosciolDrewniany());
	}
	// AI2 (23.11.2026): wojowniczy rod od 8 rodzin stawia lancuch broni przed domami i drwalami (inaczej lista z 3 pozycji
	// zawsze byla pelna domow i nigdy nie dochodzila do kopalni - po 10 latach 0 wojow). Brakujace zrodla dobierze fallback.
	Czemu = TEXT("wojowniczy ród zbroi się");
	const int32 Zbrojowni = Ile(FRodzajBudynku::Zbrojownia());
	if ((R.Cecha == ECechaRodu::Wojowniczy || R.bObrona) && Zbrojowni > 0 && R.Woje >= Zbrojowni * FRodzajBudynku::Zbrojownia().Miejsc && R.Woje < (R.Cecha == ECechaRodu::Wojowniczy ? R.Rodzin / 2 : R.Rodzin / 4))
	{
		Dodaj(FRodzajBudynku::Zbrojownia()); // kazda zbrojownia uzbraja tylko Miejsc wojow
	}
	if ((R.Cecha == ECechaRodu::Wojowniczy || R.bObrona) && R.Rodzin >= (Trudnosc >= 2 || R.Cecha == ECechaRodu::Wojowniczy ? 6 : 8) && Zbrojowni == 0) // AI3: obrona przed silniejszym wrogiem
	{
		// Sprawdzenie x10: wojowniczy czekal na kamien do kuzni latami - najpierw drugi kamieniolom.
		const bool bBrakKamienia = R.Zapas[ESurowiec::Kamien] < FRodzajBudynku::Kuznia().Koszt[ESurowiec::Kamien];
		if (Ile(FRodzajBudynku::Kuznia()) == 0 && bBrakKamienia && Ile(FRodzajBudynku::Kamieniolom()) < 2) { Dodaj(FRodzajBudynku::Kamieniolom()); }
		if (Ile(FRodzajBudynku::Kuznia()) == 0) { Dodaj(FRodzajBudynku::Kuznia()); }
		else if (Ile(FRodzajBudynku::KopalniaRudy()) == 0) { Dodaj(FRodzajBudynku::KopalniaRudy()); }
		else { Dodaj(FRodzajBudynku::Zbrojownia()); }
	}
	Czemu = TEXT("za mało jedzenia na rodziny");
	if (Bez > 0 && JedzenieNaMin < R.Rodzin / 1.4f)
	{
		if (Ile(FRodzajBudynku::ChataZbieracza()) < 2) { Dodaj(FRodzajBudynku::ChataZbieracza()); }
		if (Ile(FRodzajBudynku::ChataMysliwego()) < 2) { Dodaj(FRodzajBudynku::ChataMysliwego()); }
		if (R.Rodzin >= 8 && Ile(FRodzajBudynku::Pole()) < 1 + R.Rodzin / 10) { Dodaj(FRodzajBudynku::Pole()); }
		if (Ile(FRodzajBudynku::Pole()) > 0 && Ile(FRodzajBudynku::Mlyn()) == 0) { Dodaj(FRodzajBudynku::Mlyn()); }
		if (Ile(FRodzajBudynku::Mlyn()) > 0 && Ile(FRodzajBudynku::Piekarnia()) == 0) { Dodaj(FRodzajBudynku::Piekarnia()); }
	}
	Czemu = TEXT("za mało drwali");
	if (Bez > 0 && Ile(FRodzajBudynku::ChataDrwala()) < 1 + R.Rodzin / 8)
	{
		Dodaj(FRodzajBudynku::ChataDrwala());
	}
	Czemu = TEXT("rodziny w namiotach");
	if (WNamiotach > 0 || R.Rodzin >= 5 + Domowych(R))
	{
		if (R.Rodzin >= 12) { Dodaj(FRodzajBudynku::DomDuzy()); }
		if (Ile(FRodzajBudynku::Kamieniolom()) > 0) { Dodaj(FRodzajBudynku::DomPruski()); }
		Dodaj(FRodzajBudynku::Chata());
	}
	Czemu = TEXT("rozwój: deski, kamień, magazyny");
	if (Bez > 0 && Ile(FRodzajBudynku::Pilarnia()) == 0 && Ile(FRodzajBudynku::ChataDrwala()) > 0) { Dodaj(FRodzajBudynku::Pilarnia()); }
	if (Bez > 0 && Ile(FRodzajBudynku::Kamieniolom()) == 0 && R.Rodzin >= 7) { Dodaj(FRodzajBudynku::Kamieniolom()); }
	if (Ile(FRodzajBudynku::PlacSkladowy()) == 0 && R.Rodzin >= 6) { Dodaj(FRodzajBudynku::PlacSkladowy()); }
	if (Ile(FRodzajBudynku::Spichlerz()) == 0 && R.Rodzin >= 9) { Dodaj(FRodzajBudynku::Spichlerz()); }

	// Cechy: kupiecki - targ i sukno na sprzedaz, wojowniczy - owce, ruda, kuznia, zbrojownia (inni pozniej).
	Czemu = TEXT("cecha rodu: handel albo wojsko");
	const int32 OdWojska = R.Cecha == ECechaRodu::Wojowniczy ? 8 : 14;
	if (R.Rodzin >= (R.Cecha == ECechaRodu::Kupiecki ? 8 : 14) && Ile(FRodzajBudynku::Targ()) == 0) { Dodaj(FRodzajBudynku::Targ()); }
	if ((Bez > 0 || R.Cecha == ECechaRodu::Wojowniczy) && R.Rodzin >= OdWojska) // AI1: wojowniczy nie czeka na wolne rece
	{
		if (Ile(FRodzajBudynku::Owczarnia()) == 0) { Dodaj(FRodzajBudynku::Owczarnia()); }
		if (Ile(FRodzajBudynku::KopalniaRudy()) == 0) { Dodaj(FRodzajBudynku::KopalniaRudy()); }
		if (Ile(FRodzajBudynku::KopalniaRudy()) > 0 && Ile(FRodzajBudynku::Kuznia()) == 0) { Dodaj(FRodzajBudynku::Kuznia()); }
		if (Ile(FRodzajBudynku::Owczarnia()) > 0 && Ile(FRodzajBudynku::ChataTkacza()) == 0) { Dodaj(FRodzajBudynku::ChataTkacza()); }
	}
	if (R.Rodzin >= OdWojska && Ile(FRodzajBudynku::Kuznia()) > 0 && Ile(FRodzajBudynku::Zbrojownia()) == 0)
	{
		Dodaj(FRodzajBudynku::Zbrojownia());
	}
	Czemu = TEXT("duża osada potrzebuje kościoła");
	if (R.Rodzin >= 18 && Ile(FRodzajBudynku::KosciolDrewniany()) == 0) { Dodaj(FRodzajBudynku::KosciolDrewniany()); }
	// Nadwyzka rak do pracy: wiecej drwali i kamieniarzy (towar na budowy i na sprzedaz).
	Czemu = TEXT("wolne ręce do pracy");
	if (Bez >= 2)
	{
		Dodaj(Ile(FRodzajBudynku::Kamieniolom()) < Ile(FRodzajBudynku::ChataDrwala()) ? FRodzajBudynku::Kamieniolom() : FRodzajBudynku::ChataDrwala());
	}

	for (int32 k = 0; k < FMath::Min(3, Lista.Num()); ++k)
	{
		const FRodzajBudynku& B = *Lista[k];
		// Pierwsza pilarnia za darmo (jak u gracza, AOsada::Darmowy).
		const int32 IB = Indeks(B);
		const bool bDarmo = B.bPierwszyDarmowy && IB >= 0 && R.Ile(IB) == 0;
		// Rezerwe trzyma tylko z tego, co sam wytwarza (inaczej na poczatku nie postawilby nic).
		bool bStac = true;
		for (ESurowiec S : WszystkieSurowce)
		{
			bool bWytwarza = false;
			for (int32 j = 0; j < L.Num() && !bWytwarza; ++j)
			{
				bWytwarza = R.Ile(j) > 0 && L[j]->Wyrob == S;
			}
			bStac &= R.Zapas[S] >= B.Koszt[S] + (B.Koszt[S] > 0 && bWytwarza ? FMath::FloorToInt32(Rezerwa(R, S)) : 0);
		}
		bStac |= bDarmo;
		const int32 I = IB;
		if (!bStac || I < 0 || B.bNadRzeka)
		{
			continue;
		}
		for (ESurowiec S : WszystkieSurowce)
		{
			R.Zapas[S] -= bDarmo ? 0 : B.Koszt[S];
		}
		R.Buduje = I;
		R.DoKonca = B.CzasBudowy; // jak u gracza: CzasBudowy z praca liczona 1,5 raza, budowniczy i tragarze naraz
		R.Ostatnio = FString::Printf(TEXT("buduje: %s"), *B.Nazwa);
		R.Dlaczego = Powody[k];
		Dziennik(R, FString::Printf(TEXT("buduje: %s | bo: %s"), *B.Nazwa, *R.Dlaczego));
		return;
	}
	// AI1: nic nie stac - dla czego brakuje, a nikt w rodzie tego nie wytwarza, postaw wytworce (lancuchem po wsadzie:
	// bron -> kuznia -> ruda -> kopalnia). Bez tego rod czekal w nieskonczonosc (np. kosciol bez pilarni i kamieniolomu).
	auto Wytwarza = [&](ESurowiec S) { for (int32 j = 0; j < L.Num(); ++j) { if (R.Ile(j) > 0 && L[j]->Wyrob == S) { return true; } } return S == ESurowiec::Kloda && R.Ile(Indeks(FRodzajBudynku::ChataDrwala())) > 0; };
	auto Wytworca = [&](ESurowiec S) -> const FRodzajBudynku*
	{
		for (const FRodzajBudynku* W : L) { if ((W->Wyrob == S || (S == ESurowiec::Deska && W->Funkcja == EFunkcja::Pilarnia)) && !W->bNadRzeka) { return W; } }
		return nullptr;
	};
	for (int32 k = 0; k < FMath::Min(3, Lista.Num()); ++k)
	{
		for (ESurowiec S : WszystkieSurowce)
		{
			const FRodzajBudynku* W = Lista[k]->Koszt[S] > R.Zapas[S] && !Wytwarza(S) ? Wytworca(S) : nullptr;
			for (int32 Glebia = 0; W && Glebia < 3; ++Glebia)
			{
				const ESurowiec Wsad = WsadAI(*W);
				if (Wsad != ESurowiec::Ile && !Wytwarza(Wsad) && Wytworca(Wsad))
				{
					W = Wytworca(Wsad); // najpierw zrodlo wsadu
					continue;
				}
				break;
			}
			const int32 IW = W ? Indeks(*W) : -1;
			bool bStac = W && IW >= 0;
			for (ESurowiec X : WszystkieSurowce) { bStac = bStac && R.Zapas[X] >= W->Koszt[X]; }
			if (bStac)
			{
				for (ESurowiec X : WszystkieSurowce) { R.Zapas[X] -= W->Koszt[X]; }
				R.Buduje = IW;
				R.DoKonca = W->CzasBudowy;
				R.Ostatnio = FString::Printf(TEXT("buduje: %s"), *W->Nazwa);
				R.Dlaczego = FString::Printf(TEXT("brak: %s na %s"), NazwaSurowca(S), *Lista[k]->Nazwa);
				Dziennik(R, FString::Printf(TEXT("buduje: %s | bo: %s"), *W->Nazwa, *R.Dlaczego));
				return;
			}
		}
	}
	// AI0: nic nie postawil - zapisz, na co czeka (zmiana powodu = wpis w dzienniku).
	FString Czeka = TEXT("nic do budowy");
	if (Lista.Num() > 0)
	{
		const FRodzajBudynku& B = *Lista[0];
		FString Brak;
		for (ESurowiec S : WszystkieSurowce)
		{
			if (R.Zapas[S] < B.Koszt[S])
			{
				Brak += FString::Printf(TEXT(" %s %d/%d"), NazwaSurowca(S), R.Zapas[S], B.Koszt[S]);
			}
		}
		Czeka = FString::Printf(TEXT("czeka na materiał: %s (%s; brak:%s)"), *B.Nazwa, *Powody[0], Brak.IsEmpty() ? TEXT(" rezerwa") : *Brak);
	}
	if (Czeka != R.Dlaczego)
	{
		R.Dlaczego = Czeka;
		Dziennik(R, Czeka);
	}
}

void ARody::Rok()
{
	// Co rok: lennicy placa danine; silny wojowniczy rod zada daniny od gracza.
	const int32 SilaG = SilaGracza();
	for (int32 k = 0; k < Rody.Num(); ++k)
	{
		FRod& R = Rody[k];
		if (R.bLennik)
		{
			if (R.Sila() > SilaG)
			{
				R.bLennik = false;
				R.Stosunek -= 20.f;
				Ogloszenie(FString::Printf(TEXT("%s są już silniejsi i nie płacą daniny"), *R.Nazwa));
				continue;
			}
			const int32 Danina = FMath::Min(R.Grosze, 10 + 2 * R.Rodzin);
			R.Grosze -= Danina;
			if (Osada.IsValid())
			{
				Osada->Grosze += Danina;
			}
			Ogloszenie(FString::Printf(TEXT("%s płacą daninę: %d groszy"), *R.Nazwa, Danina));
		}
		else if (R.Cecha == ECechaRodu::Wojowniczy && R.ZadaDaniny == 0 && R.Sila() >= 2 * SilaG && R.Stosunek < 30.f)
		{
			R.ZadaDaniny = 20 + 10 * int32(R.Poziom);
			R.CzasZadania = FKalendarz::DlugoscPory;
			Ogloszenie(FString::Printf(TEXT("%s żądają daniny: %d groszy (okno rodów, klawisz H)"), *R.Nazwa, R.ZadaDaniny));
		}
	}
}

int32 ARody::SilaGracza() const
{
	return Osada.IsValid() ? Osada->Wojow() * 3 + Osada->Rodzin() : 0;
}

EPoziomOsady ARody::PoziomGracza() const
{
	if (!Osada.IsValid())
	{
		return EPoziomOsady::Oboz;
	}
	return EPoziomOsady(Osada->Poziom()); // paczka A: poziom gracza liczy osada (warunki z potrzebami)
}

int32 ARody::NaSprzedaz(int32 RI, ESurowiec S) const
{
	if (!Rody.IsValidIndex(RI))
	{
		return 0;
	}
	const FRod& R = Rody[RI];
	return FMath::Max(0, R.Zapas[S] - FMath::CeilToInt32(Cel(R, S) * 0.3f));
}

int32 ARody::CenaKupna(int32 RI, ESurowiec S) const
{
	if (!Rody.IsValidIndex(RI))
	{
		return 0;
	}
	const FRod& R = Rody[RI];
	return FMath::Max(1, FMath::RoundToInt32(CenaBazowa(S) * (1.f + 0.6f * Niedobor(R, S)) * 1.2f * Mnoznik(R)));
}

int32 ARody::CenaSprzedazy(int32 RI, ESurowiec S) const
{
	if (!Rody.IsValidIndex(RI))
	{
		return 0;
	}
	const FRod& R = Rody[RI];
	return FMath::Max(1, FMath::RoundToInt32(CenaBazowa(S) * (1.f + 0.6f * Niedobor(R, S)) * 0.8f / Mnoznik(R)));
}

bool ARody::Kup(int32 RI, ESurowiec S, int32 N, FString& Powod)
{
	ABudowa* Targ = Osada.IsValid() ? Osada->Targ() : nullptr;
	if (!Rody.IsValidIndex(RI) || !Targ)
	{
		Powod = TEXT("Najpierw postaw targ (zakładka Handel).");
		return false;
	}
	FRod& R = Rody[RI];
	if (R.bWrogi)
	{
		Powod = FString::Printf(TEXT("%s nie handlują z wami (odmówiono im daniny)."), *R.Nazwa);
		return false;
	}
	const int32 Cena = CenaKupna(RI, S);
	N = FMath::Min3(N, NaSprzedaz(RI, S), Targ->Miejsce(S));
	N = FMath::Min(N, Osada->Grosze / Cena);
	if (N <= 0)
	{
		Powod = NaSprzedaz(RI, S) <= 0 ? FString::Printf(TEXT("%s nie mają tego na sprzedaż."), *R.Nazwa)
			: (Targ->Miejsce(S) <= 0 ? FString(TEXT("Na targu nie ma już miejsca na ten towar.")) : FString(TEXT("Za mało groszy.")));
		return false;
	}
	Osada->Grosze -= Cena * N;
	R.Grosze += Cena * N;
	R.Zapas[S] -= N;
	Targ->ZmienZapas(S, N);
	R.Stosunek = FMath::Min(100.f, R.Stosunek + 1.f);
	Powod = FString::Printf(TEXT("Kupiono %s za %d groszy. Kupcy przywieźli towar na targ."), *IleSurowca(S, N), Cena * N);
	return true;
}

bool ARody::Sprzedaj(int32 RI, ESurowiec S, int32 N, FString& Powod)
{
	ABudowa* Targ = Osada.IsValid() ? Osada->Targ() : nullptr;
	if (!Rody.IsValidIndex(RI) || !Targ)
	{
		Powod = TEXT("Najpierw postaw targ (zakładka Handel).");
		return false;
	}
	FRod& R = Rody[RI];
	if (R.bWrogi)
	{
		Powod = FString::Printf(TEXT("%s nie handlują z wami (odmówiono im daniny)."), *R.Nazwa);
		return false;
	}
	const int32 Cena = CenaSprzedazy(RI, S);
	N = FMath::Min3(N, Targ->Dostepne(S), R.Grosze / Cena);
	if (N <= 0)
	{
		Powod = Targ->Dostepne(S) <= 0 ? FString(TEXT("Na targu nie ma tego towaru: tragarze znoszą tu wyroby jak na plac składowy."))
			: FString::Printf(TEXT("%s nie mają tylu groszy."), *R.Nazwa);
		return false;
	}
	Targ->ZmienZapas(S, -N);
	R.Zapas[S] += N;
	R.Grosze -= Cena * N;
	Osada->Grosze += Cena * N;
	R.Stosunek = FMath::Min(100.f, R.Stosunek + 1.f);
	Powod = FString::Printf(TEXT("Sprzedano %s za %d groszy."), *IleSurowca(S, N), Cena * N);
	return true;
}

bool ARody::Podarunek(int32 RI, FString& Powod)
{
	if (!Rody.IsValidIndex(RI) || !Osada.IsValid() || Osada->Grosze < KosztPodarunku)
	{
		Powod = FString::Printf(TEXT("Podarunek kosztuje %d groszy."), KosztPodarunku);
		return false;
	}
	FRod& R = Rody[RI];
	Osada->Grosze -= KosztPodarunku;
	R.Grosze += KosztPodarunku;
	// Im lepsze stosunki, tym mniej daje kolejny podarunek; kupcy lubia grosze najbardziej.
	const float Zysk = 12.f * (1.f - FMath::Max(0.f, R.Stosunek) / 120.f) + (R.Cecha == ECechaRodu::Kupiecki ? 3.f : 0.f);
	R.Stosunek = FMath::Min(100.f, R.Stosunek + Zysk);
	Powod = FString::Printf(TEXT("%s przyjęli podarunek (stosunki +%.0f)."), *R.Nazwa, Zysk);
	return true;
}

bool ARody::MoznaZadacDaniny(int32 RI) const
{
	return Rody.IsValidIndex(RI) && !Rody[RI].bLennik && SilaGracza() >= 2 * Rody[RI].SilaObrony();
}

bool ARody::ZadajDaniny(int32 RI, FString& Powod)
{
	if (!Rody.IsValidIndex(RI))
	{
		return false;
	}
	FRod& R = Rody[RI];
	if (!MoznaZadacDaniny(RI))
	{
		Powod = R.bLennik ? FString::Printf(TEXT("%s już płacą daninę."), *R.Nazwa)
			: FString::Printf(TEXT("Za mało sił: trzeba co najmniej %d (woje x3 + rodziny, za murami więcej), macie %d."), 2 * R.SilaObrony(), SilaGracza());
		return false;
	}
	R.bLennik = true;
	R.Stosunek = FMath::Max(-100.f, R.Stosunek - 30.f);
	const int32 Danina = FMath::Min(R.Grosze, 10 + 2 * R.Rodzin);
	R.Grosze -= Danina;
	Osada->Grosze += Danina;
	Powod = FString::Printf(TEXT("%s ugięli się: płacą %d groszy teraz i co rok (stosunki -30)."), *R.Nazwa, Danina);
	Ogloszenie(Powod);
	return true;
}

bool ARody::ZaplacDanine(int32 RI, FString& Powod)
{
	if (!Rody.IsValidIndex(RI) || Rody[RI].ZadaDaniny <= 0)
	{
		return false;
	}
	FRod& R = Rody[RI];
	if (!Osada.IsValid() || Osada->Grosze < R.ZadaDaniny)
	{
		Powod = TEXT("Za mało groszy na daninę.");
		return false;
	}
	Osada->Grosze -= R.ZadaDaniny;
	R.Grosze += R.ZadaDaniny;
	R.Stosunek = FMath::Min(100.f, R.Stosunek + 10.f);
	R.bWrogi = false;
	Powod = FString::Printf(TEXT("Zapłacono daninę rodowi %s: %d groszy."), *R.Nazwa, R.ZadaDaniny);
	R.ZadaDaniny = 0;
	return true;
}

void ARody::OdmowDaniny(int32 RI)
{
	if (!Rody.IsValidIndex(RI))
	{
		return;
	}
	FRod& R = Rody[RI];
	R.ZadaDaniny = 0;
	R.bWrogi = true;
	R.Stosunek = FMath::Max(-100.f, R.Stosunek - 30.f);
	Ogloszenie(FString::Printf(TEXT("%s: odmowa daniny. Grożą wojną i zrywają handel."), *R.Nazwa));
}

// ---------------------------------------------------------------------------------------------------------------
// Faza AI3 (23.11.2026): decyzje wladcy. Raz na pore roku kazdy rod wazy: odbudowa po napadzie, napad na slabszego
// wrogiego sasiada (co najmniej 1,5 raza slabszego), napad na gracza (od 2. roku, zapowiedziany pore wczesniej),
// danina od dwa razy slabszego, obrona przed silniejszym wrogiem, pokoj z rownym wrogiem, handel z przyjacielem,
// rozwoj. Bitwy miedzy rodami liczone z sily (woje x3 + rodziny).
// shortcut: napad na gracza tez liczony z sily (bez ABitwa przy kamerze), do zrobienia przy AI4 / wojnie W3.
const TCHAR* ARody::NazwaDecyzji(EDecyzja D)
{
	switch (D)
	{
	case EDecyzja::Rozwoj: return TEXT("rozwój");
	case EDecyzja::Handel: return TEXT("handel");
	case EDecyzja::Pokoj: return TEXT("pokój");
	case EDecyzja::Napad: return TEXT("napad");
	case EDecyzja::Danina: return TEXT("danina");
	case EDecyzja::Obrona: return TEXT("obrona");
	case EDecyzja::Odbudowa: return TEXT("odbudowa");
	default: return TEXT("napad na was");
	}
}

float ARody::BazaStosunku(int32 A, int32 B) const
{
	const ECechaRodu CA = Rody[A].Cecha, CB = Rody[B].Cecha;
	if (CA == ECechaRodu::Wojowniczy && CB == ECechaRodu::Wojowniczy) { return -40.f; } // rywale
	if (CA == ECechaRodu::Wojowniczy || CB == ECechaRodu::Wojowniczy) { return -30.f; }
	return CA == CB ? 20.f : 0.f;
}

void ARody::UstawStosunki()
{
	for (int32 i = 0; i < Rody.Num(); ++i)
	{
		Rody[i].Wobec.Init(0.f, Rody.Num());
		for (int32 j = 0; j < Rody.Num(); ++j)
		{
			Rody[i].Wobec[j] = i == j ? 100.f : BazaStosunku(i, j);
		}
	}
}

void ARody::Bitwa(int32 IA, int32 IB)
{
	FRod& A = Rody[IA];
	FRod& B = Rody[IB];
	Napady.Add(FIntPoint(IA, IB));
	NapadowSlabszych += A.Sila() < B.Sila() ? 1 : 0;
	++A.Napadow;
	++B.Napadniety;
	B.Napastnik = IA;
	const float SA = A.Sila() * Los.FRandRange(0.85f, 1.15f);
	const float SB = B.Sila() * Los.FRandRange(0.85f, 1.15f) * 1.2f * B.Obrona(); // obronca ma przewage, wieksza za murami
	FString Wynik;
	if (SA > SB)
	{
		for (ESurowiec S : WszystkieSurowce)
		{
			const int32 N = B.Zapas[S] * 3 / 10;
			B.Zapas[S] -= N;
			A.Zapas[S] += N;
		}
		const int32 Gr = B.Grosze * 3 / 10;
		B.Grosze -= Gr;
		A.Grosze += Gr;
		B.Woje /= 2;
		B.Rodzin = FMath::Max(2, B.Rodzin - FMath::Max(1, B.Rodzin / 7));
		B.Woje = FMath::Min(B.Woje, B.Rodzin);
		A.Woje -= A.Woje / 5;
		Wynik = FString::Printf(TEXT("%s złupili %s"), *A.Nazwa, *B.Nazwa);
	}
	else
	{
		A.Woje -= A.Woje / 2;
		B.Woje -= B.Woje / 5;
		Wynik = FString::Printf(TEXT("%s odparli napad %s"), *B.Nazwa, *A.Nazwa);
	}
	A.Wobec[IB] = FMath::Max(-100.f, A.Wobec[IB] - 30.f);
	B.Wobec[IA] = FMath::Max(-100.f, B.Wobec[IA] - 30.f);
	Dziennik(A, FString::Printf(TEXT("BITWA: %s (siła %.0f na %.0f)"), *Wynik, SA, SB));
	if (Osada.IsValid())
	{
		Ogloszenie(Wynik);
	}
}

void ARody::Wladca(int32 I)
{
	FRod& R = Rody[I];
	if (R.Wobec.Num() != Rody.Num())
	{
		return;
	}
	// Stosunki wracaja powoli do tego, co wynika z cech (pamiec napadow i pokojow slabnie).
	for (int32 j = 0; j < Rody.Num(); ++j)
	{
		if (j != I)
		{
			R.Wobec[j] += FMath::Clamp(BazaStosunku(I, j) - R.Wobec[j], -5.f, 5.f);
		}
	}
	const bool bSyty = R.Zapas[ESurowiec::Jedzenie] >= R.Rodzin;
	int32 Cel = -1, Grozny = -1, Rowny = -1, Przyjaciel = -1, Lennik = -1;
	float Najlepszy = 0.f;
	for (int32 j = 0; j < Rody.Num(); ++j)
	{
		if (j == I)
		{
			continue;
		}
		const FRod& J = Rody[j];
		const float Stos = R.Wobec[j];
		const float Przewaga = float(R.Sila()) / FMath::Max(1, J.SilaObrony()); // mury odstraszaja napastnika
		if (Stos < -20.f && Przewaga >= (Trudnosc <= 0 ? 2.f : (Trudnosc == 1 ? 1.5f : 1.3f)) && R.Woje >= 3 && bSyty && Przewaga > Najlepszy)
		{
			Najlepszy = Przewaga;
			Cel = j;
		}
		if (Stos < -20.f && J.Sila() > R.Sila()) { Grozny = j; }
		if (Stos < -20.f && Przewaga > 0.67f && Przewaga < 1.5f) { Rowny = j; }
		if (Stos > 15.f) { Przyjaciel = j; }
		if (Stos < 0.f && Przewaga >= 2.f && J.Grosze > 0) { Lennik = j; }
	}
	R.bObrona = Grozny >= 0;
	const int32 SG = SilaGracza();
	const bool bNaGracza = Osada.IsValid() && Kalendarz.Rok() >= (Trudnosc <= 0 ? 3 : 2) && R.Cecha == ECechaRodu::Wojowniczy && R.Stosunek < -20.f
		&& SG > 0 && R.Sila() * 2 >= SG * 3 && bSyty;

	EDecyzja D = EDecyzja::Rozwoj;
	FString Bo = TEXT("nic pilnego, buduje dalej");
	if (R.Napastnik >= 0)
	{
		D = EDecyzja::Odbudowa;
		Bo = FString::Printf(TEXT("napadli nas %s; szuka zgody"), *Rody[R.Napastnik].Nazwa);
		R.Wobec[R.Napastnik] = FMath::Min(100.f, R.Wobec[R.Napastnik] + 10.f);
		R.Napastnik = -1;
	}
	else if (Cel >= 0)
	{
		D = EDecyzja::Napad;
		Bo = FString::Printf(TEXT("%s wrodzy i %.1f raza słabsi"), *Rody[Cel].Nazwa, Najlepszy);
	}
	else if (bNaGracza)
	{
		D = EDecyzja::NapadNaGracza;
		Bo = FString::Printf(TEXT("wasza siła %d, ich %d"), SG, R.Sila());
	}
	else if (Lennik >= 0)
	{
		D = EDecyzja::Danina;
		Bo = FString::Printf(TEXT("%s dwa razy słabsi"), *Rody[Lennik].Nazwa);
	}
	else if (Grozny >= 0 && R.Woje * 4 >= R.Rodzin)
	{
		D = EDecyzja::Pokoj; // juz uzbrojony: probuje zgody z silniejszym wrogiem
		Rowny = Grozny;
		Bo = FString::Printf(TEXT("silniejsi %s, a my już uzbrojeni: prosimy o zgodę"), *Rody[Grozny].Nazwa);
	}
	else if (Grozny >= 0)
	{
		D = EDecyzja::Obrona;
		Bo = FString::Printf(TEXT("silniejsi i wrodzy %s"), *Rody[Grozny].Nazwa);
	}
	else if (Rowny >= 0)
	{
		D = EDecyzja::Pokoj;
		Bo = FString::Printf(TEXT("%s wrodzy, ale równie silni"), *Rody[Rowny].Nazwa);
	}
	else if (Przyjaciel >= 0 && (PoraRodow() == EPora::Lato || PoraRodow() == EPora::Jesien)) // kupcy jezdza latem i jesienia
	{
		D = EDecyzja::Handel;
		Bo = FString::Printf(TEXT("przyjaźń z %s"), *Rody[Przyjaciel].Nazwa);
	}

	switch (D)
	{
	case EDecyzja::Napad:
		Bitwa(I, Cel);
		break;
	case EDecyzja::NapadNaGracza:
		if (R.GroziGraczowi == 0)
		{
			R.GroziGraczowi = 1; // zapowiedz: gracz ma pore na woje albo podarunek
			Ogloszenie(FString::Printf(TEXT("%s zapowiadają napad za porę roku! Wystaw woje albo poślij podarunek (okno rodów, H)."), *R.Nazwa));
			Bo += TEXT(", zapowiedź");
		}
		else
		{
			R.GroziGraczowi = 0;
			int32 Gr = 0;
			if (R.Sila() * Los.FRandRange(0.85f, 1.15f) > SG * 1.2f)
			{
				const FSurowce Lup = Osada->Grabiez(0.25f, Gr);
				for (ESurowiec S : WszystkieSurowce) { R.Zapas[S] += Lup[S]; }
				R.Grosze += Gr;
				Ogloszenie(FString::Printf(TEXT("%s napadli na osadę i zabrali łup (%d groszy)"), *R.Nazwa, Gr));
			}
			else
			{
				R.Woje -= R.Woje / 2;
				Ogloszenie(FString::Printf(TEXT("%s napadli na osadę, ale wasza siła ich odparła"), *R.Nazwa));
			}
			R.Stosunek = FMath::Max(-100.f, R.Stosunek - 20.f);
		}
		break;
	case EDecyzja::Danina:
	{
		FRod& J = Rody[Lennik];
		const int32 Ile = FMath::Min(J.Grosze, 10 + 2 * J.Rodzin);
		J.Grosze -= Ile;
		R.Grosze += Ile;
		J.Wobec[I] = FMath::Max(-100.f, J.Wobec[I] - 10.f);
		Bo += FString::Printf(TEXT(", płacą %d groszy"), Ile);
		break;
	}
	case EDecyzja::Pokoj:
		// Rozejm: stosunki do zera, potem powoli wracaja do cech (ok. 1,5 roku spokoju).
		R.Wobec[Rowny] = FMath::Max(R.Wobec[Rowny], 0.f);
		Rody[Rowny].Wobec[I] = FMath::Max(Rody[Rowny].Wobec[I], 0.f);
		break;
	case EDecyzja::Handel:
	{
		// Nadwyzka za grosze przyjaciela (po cenie bazowej).
		FRod& J = Rody[Przyjaciel];
		ESurowiec Naj = ESurowiec::Kloda;
		for (ESurowiec S : WszystkieSurowce) { Naj = R.Zapas[S] > R.Zapas[Naj] ? S : Naj; }
		const int32 N = FMath::Min3(R.Zapas[Naj] / 5, 10, J.Grosze / FMath::Max(1, CenaBazowa(Naj)));
		R.Zapas[Naj] -= N;
		J.Zapas[Naj] += N;
		J.Grosze -= N * CenaBazowa(Naj);
		R.Grosze += N * CenaBazowa(Naj);
		R.Wobec[Przyjaciel] = FMath::Min(100.f, R.Wobec[Przyjaciel] + 3.f);
		J.Wobec[I] = FMath::Min(100.f, J.Wobec[I] + 3.f);
		break;
	}
	default:
		break;
	}
	R.Decyzje |= 1u << uint32(D);
	Dziennik(R, FString::Printf(TEXT("WŁADCA: %s | bo: %s"), NazwaDecyzji(D), *Bo));
}

FString ARody::PoWyprawie(int32 RI, int32 Wybor, int32 Poleglo)
{
	if (!Rody.IsValidIndex(RI))
	{
		return FString();
	}
	FRod& R = Rody[RI];
	// Straty obroncow: najpierw woje, reszta to chlopi z rodzin.
	const int32 Wojow = FMath::Min(R.Woje, Poleglo);
	R.Woje -= Wojow;
	R.Rodzin = FMath::Max(2, R.Rodzin - (Poleglo - Wojow) / 2);
	++R.Napadniety;
	AOsada* O = Osada.Get();
	FString T;
	switch (Wybor)
	{
	case 0:
	{
		FSurowce Lup;
		int32 Sztuk = 0;
		for (ESurowiec S : WszystkieSurowce)
		{
			const int32 N = R.Zapas[S] / 2;
			R.Zapas[S] -= N;
			Lup[S] = N;
			Sztuk += N;
		}
		const int32 Gr = R.Grosze * 6 / 10;
		R.Grosze -= Gr;
		if (O) { O->Zwroc(Lup, Gr); }
		R.bWrogi = true;
		R.bLennik = false;
		R.Stosunek = FMath::Max(-100.f, R.Stosunek - 50.f);
		T = FString::Printf(TEXT("%s złupieni: %d sztuk towaru i %d groszy jedzie do waszej osady. Ród poprzysiągł zemstę."), *R.Nazwa, Sztuk, Gr);
		break;
	}
	case 1:
	{
		const int32 Gr = R.Grosze * 3 / 10;
		R.Grosze -= Gr;
		if (O) { O->Zwroc(FSurowce(), Gr); }
		R.bLennik = true;
		R.bWrogi = false;
		R.ZadaDaniny = 0;
		R.GroziGraczowi = 0;
		R.Stosunek = FMath::Max(-100.f, FMath::Min(R.Stosunek, 0.f) - 10.f);
		T = FString::Printf(TEXT("%s składają hołd: %d groszy teraz i danina co rok."), *R.Nazwa, Gr);
		break;
	}
	case 2:
	{
		for (ESurowiec S : WszystkieSurowce) { R.Zapas[S] = R.Zapas[S] * 4 / 10; }
		R.Rodzin = FMath::Max(2, R.Rodzin - R.Rodzin / 3);
		R.Woje = FMath::Min(R.Woje, R.Rodzin / 4);
		R.bWrogi = true;
		R.bLennik = false;
		R.Stosunek = -100.f;
		// Ogien podkladaja woje wyprawy (ABitwa::PoZdobyciu, 24.11.2026); bez nich pali AOsadaRodu::Spal.
		T = FString::Printf(TEXT("Osada %s płonie. Ród stracił trzecią część rodzin i większość zapasów."), *R.Nazwa);
		break;
	}
	default:
		R.bWrogi = true;
		R.Stosunek = FMath::Max(-100.f, R.Stosunek - 20.f);
		T = FString::Printf(TEXT("%s obronili osadę przed waszą wyprawą (poległo ich %d)."), *R.Nazwa, Poleglo);
		break;
	}
	R.Ostatnio = T;
	Dziennik(R, FString::Printf(TEXT("WYPRAWA GRACZA: %s"), *T));
	return T;
}

void ARody::OdswiezWidoki()
{
	// AI4 (czesc widoczna): kazdy rod ma w terenie swoja osade z budynkami; tylko w swiecie gry (testy nie maja swiata).
	UWorld* W = GetWorld();
	if (!W || !Osada.IsValid())
	{
		return;
	}
	for (int32 i = 0; i < Rody.Num(); ++i)
	{
		if (!Widoki.IsValidIndex(i) || !Widoki[i])
		{
			Widoki.SetNum(Rody.Num());
			Widoki[i] = W->SpawnActor<AOsadaRodu>(FVector(Rody[i].Gdzie, 0.f), FRotator::ZeroRotator);
		}
		if (Widoki[i])
		{
			Widoki[i]->Odswiez(Rody[i], PoraRodow());
		}
	}
}
