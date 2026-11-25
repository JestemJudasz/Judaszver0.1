// Plik: Source/grarts2d/Testy/AITesty.cpp
// Faza AI0 planu "AI przeciwnikow" (23.11.2026): testy rodow AI bez uruchamiania gry. Rody to liczby, wiec 10 lat gry
// przewija sie w petli w kilka sekund (ARody::Testowe - 8 rodow od obozu, stale ziarno, wlasny kalendarz).
// Uruchomienie: Tools\Testy.bat; linie [AI-TEST] i ZASTOJ w Saved\Testy.log.
// Bramka AI0 (grarts2d.AI.Bramka) musi przechodzic. Scenariusze AI1 (grarts2d.AI.Scenariusze) na razie tylko raportuja
// ZALICZONY / BLAD - zachowanie AI naprawia faza AI1; wtedy bScenariuszeObowiazkowe = true.
#include "Misc/AutomationTest.h"
#include "Osada/Rody.h"
#include "Osada/Osada.h"
#include "Budowy/Budowa.h"
#include "HAL/PlatformTime.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags Flagi = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
	constexpr bool bScenariuszeObowiazkowe = true; // AI1 (23.11.2026): wszystkie zaliczone
	constexpr float Rok = 4.f * FKalendarz::DlugoscPory;
	constexpr int32 Ziarno = 1234;

	int32 IndeksB(const FRodzajBudynku& B)
	{
		return FRodzajBudynku::DoBudowania().IndexOfByKey(&B);
	}

	ARody* NoweRody()
	{
		ARody::bDziennik = true;
		ARody::Trudnosc = 1;
		return NewObject<ARody>(GetTransientPackage());
	}

	/** Linia raportu; w scenariuszach AI1 BLAD tylko ostrzega, dopoki bScenariuszeObowiazkowe = false. */
	void Wynik(FAutomationTestBase& T, bool bOk, const FString& Nazwa, const FString& Opis, bool bObowiazkowy)
	{
		const FString L = FString::Printf(TEXT("[AI-TEST] %s %s: %s"), bOk ? TEXT("ZALICZONY") : TEXT("BŁĄD"), *Nazwa, *Opis);
		UE_LOG(LogTemp, Display, TEXT("%s"), *L);
		if (!bOk && bObowiazkowy)
		{
			T.AddError(L);
		}
		else
		{
			T.AddInfo(L);
		}
	}

	FString Najslabszy(const TArray<FRod>& L, TFunctionRef<int32(const FRod&)> Miara, int32& Min)
	{
		Min = MAX_int32;
		FString N;
		for (const FRod& R : L)
		{
			if (Miara(R) < Min)
			{
				Min = Miara(R);
				N = R.Nazwa;
			}
		}
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIBramkaTest, "grarts2d.AI.Bramka", Flagi)
bool FAIBramkaTest::RunTest(const FString&)
{
	ARody* A = NoweRody();
	TArray<FRod>& L = A->Testowe(Ziarno);
	TestEqual(TEXT("8 rodów"), L.Num(), 8);
	const double T0 = FPlatformTime::Seconds();
	A->Przewin(10.f * Rok);
	const double Sek = FPlatformTime::Seconds() - T0;
	TestTrue(FString::Printf(TEXT("10 lat gry 8 rodów w < 10 s (trwało %.2f s)"), Sek), Sek < 10.0);
	TestEqual(TEXT("kalendarz rodów: rok 11 po 10 latach"), A->KalendarzRodow().Rok(), 11);

	// Dziennik: kazdy rod ma wpisy. Raport zastojow (tabela "Gdzie dzisiejsze AI moze stanac").
	int32 Zastojow = 0;
	TArray<int32> Rodzin;
	for (const FRod& R : L)
	{
		TestTrue(FString::Printf(TEXT("%s ma wpisy w dzienniku"), *R.Nazwa), R.Wpisow > 0);
		Zastojow += R.Zastojow;
		Rodzin.Add(R.Rodzin);
		Wynik(*this, R.Zastojow == 0, FString::Printf(TEXT("Raport %s (%s)"), *R.Nazwa, NazwaCechy(R.Cecha)),
			FString::Printf(TEXT("po 10 latach rodzin %d, woje %d, budynków %d, zastojów %d (kopalnia %d, kuźnia %d, zbrojownia %d, ruda %d, broń %d), teraz: %s"), R.Rodzin, R.Woje,
				R.WszystkichBudynkow(), R.Zastojow, R.Ile(IndeksB(FRodzajBudynku::KopalniaRudy())), R.Ile(IndeksB(FRodzajBudynku::Kuznia())),
				R.Ile(IndeksB(FRodzajBudynku::Zbrojownia())), R.Zapas[ESurowiec::Ruda], R.Zapas[ESurowiec::Bron], *R.Dlaczego), false);
	}
	UE_LOG(LogTemp, Display, TEXT("[AI-TEST] RAPORT: zastojów razem %d w 10 latach (8 rodów), symulacja %.2f s"), Zastojow, Sek);

	// Stale ziarno: drugi przebieg daje to samo.
	ARody* B = NoweRody();
	TArray<FRod>& L2 = B->Testowe(Ziarno);
	B->Przewin(10.f * Rok);
	TArray<int32> Rodzin2;
	for (const FRod& R : L2)
	{
		Rodzin2.Add(R.Rodzin);
	}
	TestTrue(TEXT("to samo ziarno = ten sam wynik"), Rodzin == Rodzin2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIScenariuszeTest, "grarts2d.AI.Scenariusze", Flagi)
bool FAIScenariuszeTest::RunTest(const FString&)
{
	const bool bOb = bScenariuszeObowiazkowe;
	int32 Zastojow = 0;
	auto Zastoje = [&Zastojow](const TArray<FRod>& L) { for (const FRod& R : L) { Zastojow += R.Zastojow; } };
	int32 Min = 0;

	// Wzrost: 6 lat od obozu, kazdy >= 15 rodzin i >= 12 budynkow.
	{
		ARody* A = NoweRody();
		TArray<FRod>& L = A->Testowe(Ziarno);
		A->Przewin(6.f * Rok);
		const FString N1 = Najslabszy(L, [](const FRod& R) { return R.Rodzin; }, Min);
		const int32 MinRodzin = Min;
		const FString N2 = Najslabszy(L, [](const FRod& R) { return R.WszystkichBudynkow(); }, Min);
		Wynik(*this, MinRodzin >= 15 && Min >= 12, TEXT("Wzrost"),
			FString::Printf(TEXT("po 6 latach najmniej rodzin %d (%s), najmniej budynków %d (%s); wymagane 15 i 12"), MinRodzin, *N1, Min, *N2), bOb);
		Zastoje(L);
	}
	// Od zera: bez klod i bez drwala; w 1 rok ma drwala i nie traci rodzin.
	{
		ARody* A = NoweRody();
		TArray<FRod>& L = A->Testowe(Ziarno);
		for (FRod& R : L) { R.Zapas[ESurowiec::Kloda] = 0; }
		A->Przewin(Rok);
		const int32 ID = IndeksB(FRodzajBudynku::ChataDrwala());
		int32 BezDrwala = 0;
		for (const FRod& R : L) { BezDrwala += R.Ile(ID) == 0 || R.Rodzin < 5 ? 1 : 0; }
		Wynik(*this, BezDrwala == 0, TEXT("Od zera"), FString::Printf(TEXT("po roku bez drwala albo z mniejszą liczbą rodzin: %d z 8 rodów"), BezDrwala), bOb);
		Zastoje(L);
	}
	// Strata: po 3 latach zabieramy 80% zapasow i 3 budynki; w 2 lata wraca do liczby rodzin sprzed straty.
	{
		ARody* A = NoweRody();
		TArray<FRod>& L = A->Testowe(Ziarno);
		A->Przewin(3.f * Rok);
		TArray<int32> Przed;
		for (FRod& R : L)
		{
			Przed.Add(R.Rodzin);
			for (ESurowiec S : WszystkieSurowce) { R.Zapas[S] = R.Zapas[S] / 5; }
			for (int32 Zabrane = 0, i = R.Budynkow.Num() - 1; i >= 0 && Zabrane < 3; --i)
			{
				if (R.Budynkow[i] > 0) { --R.Budynkow[i]; ++Zabrane; } // najpozniejsze z listy (zwykle najdrozsze)
			}
		}
		A->Przewin(2.f * Rok);
		int32 Nie = 0;
		for (int32 k = 0; k < L.Num(); ++k) { Nie += L[k].Rodzin < Przed[k] ? 1 : 0; }
		Wynik(*this, Nie == 0, TEXT("Strata"), FString::Printf(TEXT("po 2 latach nie wróciło do liczby rodzin sprzed straty: %d z 8 rodów"), Nie), bOb);
		Zastoje(L);
	}
	// Glod: jesien, jedzenie 0 (zima za 1 pore); nie spada ponizej 3 rodzin i wraca do wzrostu.
	{
		ARody* A = NoweRody();
		TArray<FRod>& L = A->Testowe(Ziarno);
		A->Przewin(2.f * FKalendarz::DlugoscPory);
		for (FRod& R : L) { R.Zapas[ESurowiec::Jedzenie] = 0; }
		TArray<int32> Najmniej;
		for (const FRod& R : L) { Najmniej.Add(R.Rodzin); }
		for (float T = 0.f; T < 2.f * Rok; T += 20.f)
		{
			A->Przewin(20.f);
			for (int32 k = 0; k < L.Num(); ++k) { Najmniej[k] = FMath::Min(Najmniej[k], L[k].Rodzin); }
		}
		int32 Zle = 0;
		for (int32 k = 0; k < L.Num(); ++k) { Zle += Najmniej[k] < 3 || L[k].Rodzin <= Najmniej[k] ? 1 : 0; }
		Wynik(*this, Zle == 0, TEXT("Głód"), FString::Printf(TEXT("spadły poniżej 3 rodzin albo nie wróciły do wzrostu: %d z 8 rodów"), Zle), bOb);
		Zastoje(L);
	}
	// Zima: rok zaczyna sie bez klod; pod koniec jesieni ma klody na zime (posilki zimy x rodziny).
	{
		ARody* A = NoweRody();
		TArray<FRod>& L = A->Testowe(Ziarno);
		for (FRod& R : L) { R.Zapas[ESurowiec::Kloda] = 0; }
		A->Przewin(3.f * FKalendarz::DlugoscPory - 4.f);
		const int32 Posilkow = FMath::CeilToInt32(FKalendarz::DlugoscPory / AOsada::CzasPosilku);
		int32 Bez = 0;
		for (const FRod& R : L) { Bez += R.Zapas[ESurowiec::Kloda] < Posilkow * R.Rodzin ? 1 : 0; }
		Wynik(*this, Bez == 0, TEXT("Zima"), FString::Printf(TEXT("pod koniec jesieni bez kłód na zimę (%d na rodzinę): %d z 8 rodów"), Posilkow, Bez), bOb);
		Zastoje(L);
	}
	Wynik(*this, Zastojow == 0, TEXT("Bez zastoju"), FString::Printf(TEXT("strażnik zgłosił zastojów: %d"), Zastojow), bOb);
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS
// Faza AI2 (wojsko): na razie raport; obowiazkowy, gdy AI2 bedzie skonczone.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIWojskoTest, "grarts2d.AI.Wojsko", Flagi)
bool FAIWojskoTest::RunTest(const FString&)
{
	ARody* A = NoweRody();
	TArray<FRod>& L = A->Testowe(Ziarno);
	A->Przewin(4.f * Rok);
	FString Opis;
	bool bOk = true;
	for (const FRod& R : L)
	{
		if (R.Cecha == ECechaRodu::Wojowniczy)
		{
			bOk &= R.Woje >= 7 && R.Woje * 2 >= R.Rodzin - 1; // 23.11.2026: z 8 na 7 - palisada kosztuje rodzine wzrostu; uzbrojony do pelna (1/2 rodzin)
			Opis += FString::Printf(TEXT("%s %d wojów (rodzin %d, zbrojowni %d, kuźni %d, kopalni %d, broni %d); "), *R.Nazwa, R.Woje, R.Rodzin, R.Ile(IndeksB(FRodzajBudynku::Zbrojownia())), R.Ile(IndeksB(FRodzajBudynku::Kuznia())), R.Ile(IndeksB(FRodzajBudynku::KopalniaRudy())), R.Zapas[ESurowiec::Bron]);
		}
		else
		{
			bOk &= R.Woje * 4 <= R.Rodzin;
		}
	}
	Wynik(*this, bOk, TEXT("Wojowniczy ród"), Opis + TEXT("wymagane >= 7 i polowa rodzin po 4 latach, pokojowe <= 1/4 rodzin"), true);

	// Utrata armii: po zabiciu wszystkich wojow odbudowuje polowe w 1,5 roku.
	TArray<int32> Przed;
	for (FRod& R : L) { Przed.Add(R.Woje); R.Woje = 0; }
	A->Przewin(1.5f * Rok);
	FString Opis2;
	bool bOk2 = true;
	for (int32 k = 0; k < L.Num(); ++k)
	{
		bOk2 &= L[k].Woje * 2 >= Przed[k];
		Opis2 += Przed[k] > 0 ? FString::Printf(TEXT("%s %d -> %d; "), *L[k].Nazwa, Przed[k], L[k].Woje) : FString();
	}
	Wynik(*this, bOk2, TEXT("Utrata armii"), Opis2 + TEXT("wymagana połowa w 1,5 roku"), true);
	return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
// Faza AI3 (decyzje wladcy, napady miedzy rodami).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIWladcaTest, "grarts2d.AI.Wladca", Flagi)
bool FAIWladcaTest::RunTest(const FString&)
{
	// Silny na slabego: Toporczycy (0) 2x silniejsi i wrodzy Gryfitom (1) napadaja w ciagu roku.
	{
		ARody* A = NoweRody();
		TArray<FRod>& L = A->Testowe(Ziarno);
		A->Przewin(2.f * Rok);
		L[0].Woje = FMath::Max(L[0].Woje, (2 * L[1].Sila() - L[0].Rodzin) / 3 + 1);
		L[0].Zapas[ESurowiec::Jedzenie] += 100;
		L[0].Wobec[1] = L[1].Wobec[0] = -80.f;
		const int32 Przed = A->Napady.Num();
		bool bNapadl = false;
		for (int32 k = 0; k < 4 && !bNapadl; ++k)
		{
			A->Przewin(FKalendarz::DlugoscPory);
			for (int32 n = Przed; n < A->Napady.Num(); ++n) { bNapadl |= A->Napady[n].X == 0; }
		}
		Wynik(*this, bNapadl, TEXT("Silny na słabego"), FString::Printf(TEXT("Toporczycy (siła %d) %s w ciągu roku"), L[0].Sila(), bNapadl ? TEXT("napadli") : TEXT("nie napadli")), true);
	}
	// 10 lat: bez samobojstwa, po przegranej rod zyje i rosnie, przyjaciele nie walcza, kazdy >= 5 roznych decyzji.
	ARody* A = NoweRody();
	TArray<FRod>& L = A->Testowe(Ziarno);
	// Pokoj: Gryfici (1) i Awdancy (3), obaj kupieccy, przyjazni.
	L[1].Wobec[3] = L[3].Wobec[1] = 80.f;
	TArray<int32> PoNapadzie;
	PoNapadzie.Init(-1, L.Num());
	TArray<float> KiedyNapad;
	KiedyNapad.Init(-1.f, L.Num());
	for (float T = 0.f; T < 10.f * Rok; T += FKalendarz::DlugoscPory)
	{
		const int32 Przed = A->Napady.Num();
		A->Przewin(FKalendarz::DlugoscPory);
		for (int32 n = Przed; n < A->Napady.Num(); ++n)
		{
			const int32 Ofiara = A->Napady[n].Y;
			if (KiedyNapad[Ofiara] < 0.f && T < 6.f * Rok)
			{
				KiedyNapad[Ofiara] = T;
				PoNapadzie[Ofiara] = L[Ofiara].Rodzin;
			}
		}
	}
	Wynik(*this, A->NapadowSlabszych == 0, TEXT("Bez samobójstwa"), FString::Printf(TEXT("napadów razem %d, słabszy na silniejszego: %d"), A->Napady.Num(), A->NapadowSlabszych), true);
	FString Opis;
	bool bOk = true;
	for (int32 k = 0; k < L.Num(); ++k)
	{
		if (PoNapadzie[k] >= 0)
		{
			bOk &= L[k].Rodzin >= 2 && L[k].Rodzin > PoNapadzie[k];
			Opis += FString::Printf(TEXT("%s %d -> %d; "), *L[k].Nazwa, PoNapadzie[k], L[k].Rodzin);
		}
	}
	Wynik(*this, bOk, TEXT("Po przegranej"), Opis.IsEmpty() ? FString(TEXT("nikt nie został napadnięty przed 6. rokiem")) : Opis, true);
	int32 Miedzy = 0;
	for (const FIntPoint& N : A->Napady) { Miedzy += (N.X == 1 && N.Y == 3) || (N.X == 3 && N.Y == 1) ? 1 : 0; }
	Wynik(*this, Miedzy == 0, TEXT("Pokój"), FString::Printf(TEXT("Gryfici i Awdańcy (przyjaźni) walczyli %d razy w 10 lat"), Miedzy), true);
	FString Decyzje;
	int32 MinRoznych = 99;
	for (const FRod& R : L)
	{
		const int32 N = FMath::CountBits(R.Decyzje);
		MinRoznych = FMath::Min(MinRoznych, N);
		Decyzje += FString::Printf(TEXT("%s %d; "), *R.Nazwa, N);
	}
	Wynik(*this, MinRoznych >= 4, TEXT("Bez zawieszenia"), Decyzje + TEXT("wymagane >= 4 różnych decyzji"), true);
	return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
// Faza AI5: raport balansu - 50 symulacji (rozne ziarno) na kazdy poziom, 5 lat. Zastojow ma byc 0.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIBalansTest, "grarts2d.AI.Balans", Flagi)
bool FAIBalansTest::RunTest(const FString&)
{
	constexpr int32 Symulacji = 50;
	int32 ZastojowRazem = 0;
	for (int32 Poz = 0; Poz < 3; ++Poz)
	{
		double Rodzin = 0.0, Wojow = 0.0;
		int32 Napadow = 0, Zastojow = 0;
		for (int32 Z = 0; Z < Symulacji; ++Z)
		{
			ARody* A = NoweRody();
			ARody::bDziennik = false; // 150 przebiegow: bez linii w logu (zastoj i tak sie zapisze)
			ARody::Trudnosc = Poz;
			TArray<FRod>& L = A->Testowe(1000 + Z);
			A->Przewin(5.f * Rok);
			for (const FRod& R : L)
			{
				Rodzin += R.Rodzin;
				Wojow += R.Woje;
				Zastojow += R.Zastojow;
			}
			Napadow += A->Napady.Num();
		}
		const double N = double(Symulacji) * 8.0;
		ZastojowRazem += Zastojow;
		Wynik(*this, Zastojow == 0, FString::Printf(TEXT("Balans %s"), ARody::NazwaTrudnosci(Poz)),
			FString::Printf(TEXT("po 5 latach średnio %.1f rodzin i %.1f wojów na ród, napadów %.1f na grę, zastojów %d"),
				Rodzin / N, Wojow / N, double(Napadow) / Symulacji, Zastojow), true);
	}
	ARody::Trudnosc = 1;
	ARody::bDziennik = true;
	return ZastojowRazem == 0;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
// Mury i szybsze zbrojenie (23.11.2026, po sprawdzeniu na x10): wojowniczy maja woje po 4 latach, rody stawiaja mury,
// a mury chronia przed napadem (obronca za palisada x1,3, za murem x1,6).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIMuryTest, "grarts2d.AI.Mury", Flagi)
bool FAIMuryTest::RunTest(const FString&)
{
	ARody* A = NoweRody();
	TArray<FRod>& L = A->Testowe(Ziarno);
	A->Przewin(4.f * Rok);
	FString Opis;
	bool bWoje = true;
	for (const FRod& R : L)
	{
		if (R.Cecha == ECechaRodu::Wojowniczy)
		{
			bWoje &= R.Woje >= 6;
			Opis += FString::Printf(TEXT("%s %d wojów; "), *R.Nazwa, R.Woje);
		}
	}
	Wynik(*this, bWoje, TEXT("Woje po 4 latach"), Opis + TEXT("wymagane >= 6 u wojowniczych"), true);

	A->Przewin(2.f * Rok);
	int32 Palisad = 0, Murow = 0;
	FString Opis2;
	for (const FRod& R : L)
	{
		Palisad += R.Mury >= 1 ? 1 : 0;
		Murow += R.Mury >= 2 ? 1 : 0;
		Opis2 += FString::Printf(TEXT("%s %d (rodzin %d); "), *R.Nazwa, R.Mury, R.Rodzin);
	}
	Wynik(*this, Palisad >= 6 && Murow >= 1, TEXT("Mury po 6 latach"), Opis2 + FString::Printf(TEXT("palisad %d (wymagane >= 6), murów kamiennych %d (>= 1)"), Palisad, Murow), true);

	FRod X;
	X.Woje = 10; X.Rodzin = 20;
	const int32 Bez = X.SilaObrony();
	X.Mury = 2;
	Wynik(*this, X.SilaObrony() > Bez * 3 / 2, TEXT("Mury bronią"), FString::Printf(TEXT("siła obrony bez murów %d, za murem %d"), Bez, X.SilaObrony()), true);
	return true;
}
#endif
