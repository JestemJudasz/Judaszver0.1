// Plik: Source/grarts2d/Testy/OsadaRoduTesty.cpp
// Uklad osad rodow AI (23.11.2026, wies z ulicami i murem po obrysie) bez uruchamiania gry: rody przewiniete 10 lat,
// kazda osada stawiana w tymczasowym swiecie (AOsadaRodu::Odswiez). Sprawdza: stoja wszystkie budynki, dzialki nie
// nachodza na siebie, zabudowa jest w murze, ufortyfikowane osady maja 2 bramy i baszty; po dwoch nowych domach stare
// budynki w srodku zostaja na swoich miejscach. Linie [AI-TEST] w Saved\Testy.log (Tools\Testy.bat).
#include "Misc/AutomationTest.h"
#include "Osada/Rody.h"
#include "Osada/OsadaRodu.h"
#include "Budowy/Budowa.h"
#include "Swiat/SwiatWysokosci.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UObject/Package.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOsadaRoduUkladTest, "grarts2d.AI.UkladOsady", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FOsadaRoduUkladTest::RunTest(const FString&)
{
	FSwiat::Wczytaj();
	ARody::bDziennik = false;
	ARody::Trudnosc = 1;
	ARody* A = NewObject<ARody>(GetTransientPackage());
	TArray<FRod>& Lista = A->Testowe(1234);
	A->Przewin(10.f * 4.f * FKalendarz::DlugoscPory);

	UWorld* W = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(W);
	for (const FRod& R : Lista)
	{
		AOsadaRodu* O = W->SpawnActor<AOsadaRodu>(FVector(R.Gdzie, 0.0), FRotator::ZeroRotator);
		if (!TestNotNull(TEXT("Osada rodu w swiecie testowym"), O)) { break; }
		const double T0 = FPlatformTime::Seconds();
		O->Odswiez(R, EPora::Lato);
		const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;
		const AOsadaRodu::FSprawdzenie S = O->Sprawdz();
		const FString Opis = FString::Printf(TEXT("%s: %d rodzin, mury %d, budynki %d/%d, nachodzi %d, poza murem %d, bramy %d, baszty %d, odcinki %d, ulice %d, %.0f ms"),
			*R.Nazwa, R.Rodzin, R.Mury, S.Budynkow, S.Chcianych, S.Nachodzi, S.PozaMurem, S.Bram, S.Baszt, S.Odcinkow, S.Ulic, Ms);
		UE_LOG(LogTemp, Display, TEXT("[AI-TEST] %s"), *Opis);
		O->ZapiszPlan(FPaths::ProjectSavedDir() / TEXT("Plany") / R.Nazwa + TEXT(".txt"));
		TestEqual(TEXT("Stoja wszystkie budynki: ") + Opis, S.Budynkow, S.Chcianych);
		TestEqual(TEXT("Dzialki nie nachodza: ") + Opis, S.Nachodzi, 0);
		TestEqual(TEXT("Zabudowa w murze: ") + Opis, S.PozaMurem, 0);
		if (R.Mury > 0)
		{
			TestEqual(TEXT("Dwie bramy: ") + Opis, S.Bram, 2);
			TestTrue(TEXT("Baszty i odcinki muru: ") + Opis, S.Baszt >= 4 && S.Odcinkow >= 20);
		}

		// Dwa nowe domy: stare budynki w srodku zostaja na miejscu (za murem moga sie przeniesc, gdy mur urosnie).
		TMap<FIntPoint, FVector2D> Bylo;
		for (const AOsadaRodu::FDzialka& D : O->ListaDzialek())
		{
			if (!D.bZewn) { Bylo.Add(FIntPoint(D.Typ, D.Nr), D.P); }
		}
		FRod R2 = R;
		const int32 Chata = FRodzajBudynku::DoBudowania().IndexOfByPredicate([](const FRodzajBudynku* B) { return B->Funkcja == EFunkcja::Mieszkalny && B->Fazy > 1; });
		if (R2.Budynkow.IsValidIndex(Chata)) { R2.Budynkow[Chata] += 2; }
		O->Odswiez(R2, EPora::Lato);
		int32 Przesuniete = 0;
		for (const AOsadaRodu::FDzialka& D : O->ListaDzialek())
		{
			const FVector2D* P = Bylo.Find(FIntPoint(D.Typ, D.Nr));
			Przesuniete += P && FVector2D::Distance(*P, D.P) > 1.0 ? 1 : 0;
		}
		TestEqual(TEXT("Stare budynki na miejscu: ") + R.Nazwa, Przesuniete, 0);
		TestEqual(TEXT("Nowe domy stoja: ") + R.Nazwa, O->Sprawdz().Budynkow, O->Sprawdz().Chcianych);

		// Mur rosnie na oczach (26.11.2026): palisada w budowie ma tyle odcinkow, ile postepu (od 0 do pelnego obwodu).
		FRod R3 = R;
		R3.Mury = 1;
		R3.Buduje = -1;
		O->Odswiez(R3, EPora::Lato);
		const int32 Pelny = O->Sprawdz().Odcinkow;
		R3.Mury = 0;
		R3.Buduje = FRod::BudujeMury;
		R3.DoKonca = FRod::CzasMuru(0) * 0.5f;
		O->Odswiez(R3, EPora::Lato);
		const int32 Polowa = O->Sprawdz().Odcinkow;
		TestTrue(FString::Printf(TEXT("Mur w budowie rosnie (%s: polowa postepu %d z %d odcinkow)"), *R.Nazwa, Polowa, Pelny), Polowa > Pelny * 0.3f && Polowa < Pelny * 0.7f);
		O->Destroy();
	}
	GEngine->DestroyWorldContext(W);
	W->DestroyWorld(false);
	return true;
}
#endif
