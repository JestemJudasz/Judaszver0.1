// Plik: Source/grarts2d/Testy/BitwaTesty.cpp
// Bitwa probna bez okna gry (24.11.2026): miara do poprawek walki ("ludziki sie blokuja, jedni czekali obok zamiast walczyc").
// Bitwa -bitwa (od razu atak) w tymczasowym swiecie, krok 0,05 s, najwyzej 240 s. Liczy:
//   - ile procent zywych wojow srednio walczy (ma przeciwnika albo zadaje cios),
//   - ilu srednio stoi bezczynnie tuz przy wrogu (< 2,5 m) i ilu najwiecej naraz,
//   - kiedy pierwsze zwarcie i kiedy koniec walki.
// Linie [BT-TEST] (i dziennik [BITWA]) w Saved\Testy.log (Tools\Testy.bat).
#include "Misc/AutomationTest.h"
#include "Wojna/Bitwa.h"
#include "Ludzie/Ludzik.h"
#include "Osada/Osada.h"
#include "Swiat/SwiatWysokosci.h"
#include "Swiat/TerenKafle.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBitwaTest, "grarts2d.Wojna.Bitwa", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FBitwaTest::RunTest(const FString&)
{
	FSwiat::Wczytaj();
	FMath::RandInit(4242);
	UWorld* W = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(W);
	const bool bBylDziennik = ABitwa::bDziennik;
	ABitwa::bDziennik = true;
	ABitwa* B = W->SpawnActor<ABitwa>();
	if (TestNotNull(TEXT("Bitwa"), B) && TestTrue(TEXT("Bitwa probna ruszyla"), B->ZacznijProbna(FSwiat::Start(), true)))
	{
		const float Dt = 0.05f;
		while (B->Czas() < 240.f && B->Miary().Koniec < 0.f)
		{
			B->Tick(Dt);
			for (TActorIterator<ALudzik> It(W); It; ++It)
			{
				It->Tick(Dt);
			}
		}
		const ABitwa::FMiary& M = B->Miary();
		const double Walczy = M.Zywych > 0 ? 100.0 * M.Walczacych / M.Zywych : 0.0;
		const double Bez = M.Probek ? M.Bezczynnych / M.Probek : 0.0;
		UE_LOG(LogTemp, Display, TEXT("[BT-TEST] zwarcie t=%.1f, koniec t=%.1f, srednio walczy %.0f%% zywych, bezczynnych przy wrogu srednio %.1f (najwiecej %d), probek %d"),
			M.Zwarcie, M.Koniec, Walczy, Bez, M.MaxBezczynnych, M.Probek);
		TestTrue(TEXT("Doszlo do zwarcia"), M.Zwarcie >= 0.f);
		TestTrue(TEXT("Bitwa sie skonczyla"), M.Koniec > 0.f);
		B->Destroy();
	}
	for (TActorIterator<ALudzik> It(W); It; ++It)
	{
		It->Destroy();
	}
	ABitwa::bDziennik = bBylDziennik;
	GEngine->DestroyWorldContext(W);
	W->DestroyWorld(false);
	return true;
}
#endif
