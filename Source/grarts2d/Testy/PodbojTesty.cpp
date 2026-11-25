// Plik: Source/grarts2d/Testy/PodbojTesty.cpp
// Podboj osad rodow (23.11.2026) bez uruchamiania gry: osada rodu z murem w tymczasowym swiecie.
//   - dwie bramy z pelna wytrzymaloscia, srodek osady w murach, pole za nimi nie;
//   - ciosy w brame: ponizej 60% model _Z1, przy zerze _Z2 i przejscie tylko przy rozbitej bramie;
//   - spalenie: budynki plona, po ~80 s gry zostaja zgliszcza (_Z2), pozary gasna;
//   - nowa pora roku: bramy naprawione, dwa budynki odbudowane;
//   - skutki wyprawy w liczbach rodu: hold (lennik, grosze), lup (zapasy), spalenie (rodziny), odparta (wrogi).
// Linie [PB-TEST] w Saved\Testy.log (Tools\Testy.bat).
#include "Misc/AutomationTest.h"
#include "Osada/Rody.h"
#include "Osada/OsadaRodu.h"
#include "Budowy/Budowa.h"
#include "Efekty/Efekty.h"
#include "Swiat/SwiatWysokosci.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Ile instancji maja modele, ktorych nazwa zawiera Co. */
	int32 Instancji(const AActor* A, const TCHAR* Co)
	{
		int32 N = 0;
		TArray<UInstancedStaticMeshComponent*> C;
		A->GetComponents(C);
		for (const UInstancedStaticMeshComponent* I : C)
		{
			if (I->GetStaticMesh() && I->GetStaticMesh()->GetName().Contains(Co)) { N += I->GetInstanceCount(); }
		}
		return N;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPodbojTest, "grarts2d.Wojna.Podboj", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FPodbojTest::RunTest(const FString&)
{
	FSwiat::Wczytaj();
	FMath::RandInit(777);
	ARody::bDziennik = false;
	ARody::Trudnosc = 1;
	ARody* A = NewObject<ARody>(GetTransientPackage());
	TArray<FRod>& Lista = A->Testowe(1234);
	A->Przewin(10.f * 4.f * FKalendarz::DlugoscPory);
	const int32 RI = Lista.IndexOfByPredicate([](const FRod& R) { return R.Mury > 0; });
	if (!TestTrue(TEXT("Po 10 latach jakis rod ma mury"), RI >= 0))
	{
		return false;
	}
	const float TempoBylo = ABudowa::Tempo;
	ABudowa::Tempo = 10.f;
	UWorld* W = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(W);
	const FRod R = Lista[RI];
	AOsadaRodu* O = W->SpawnActor<AOsadaRodu>(FVector(R.Gdzie, 0.0), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("Osada rodu"), O))
	{
		O->Odswiez(R, EPora::Lato);
		UE_LOG(LogTemp, Display, TEXT("[PB-TEST] %s: mury %d, rodzin %d, wojow %d, bram %d"), *R.Nazwa, R.Mury, R.Rodzin, R.Woje, O->Bramy().Num());
		if (TestEqual(TEXT("Dwie bramy"), O->Bramy().Num(), 2))
		{
			const AOsadaRodu::FBrama B0 = O->Bramy()[0];
			TestEqual(TEXT("Brama cala"), B0.Punkty, B0.Max);
			TestTrue(TEXT("Plac w murach"), O->WMurach(R.Gdzie));
			TestFalse(TEXT("Pole przed brama poza murami"), O->WMurach(B0.P + B0.Na * 3000.f));
			TestFalse(TEXT("Cala brama: nie ma przejscia"), O->PrzyRozbitejBramie(B0.P));

			// Topory: po 4 punkty, az brama padnie.
			int32 Ciosow = 0, Pekla = -1;
			bool bPadla = false;
			for (; Ciosow < 2000 && !bPadla; ++Ciosow)
			{
				bPadla = O->UderzBrame(0, 4.f);
				if (Pekla < 0 && Instancji(O, TEXT("Brama_Z1")) > 0) { Pekla = Ciosow + 1; }
			}
			UE_LOG(LogTemp, Display, TEXT("[PB-TEST] brama (max %.0f): pekla po %d ciosach, padla po %d"), B0.Max, Pekla, Ciosow);
			TestTrue(TEXT("Uszkodzona brama ma model _Z1"), Pekla > 0 && Pekla < Ciosow);
			TestTrue(TEXT("Brama padla"), bPadla && O->BramaLezy(0));
			TestTrue(TEXT("Rozbita brama ma model _Z2"), Instancji(O, TEXT("Brama_Z2")) == 1);
			TestTrue(TEXT("Przy rozbitej bramie jest przejscie"), O->PrzyRozbitejBramie(O->Bramy()[0].P));
			TestFalse(TEXT("Przy drugiej bramie nie ma"), O->PrzyRozbitejBramie(O->Bramy()[1].P));
			TestFalse(TEXT("Lezacej bramy nie da sie bic"), O->UderzBrame(0, 4.f));
		}

		// Spalenie zdobytej osady.
		O->Spal(6);
		AEfekty* E = AEfekty::W(W);
		TestEqual(TEXT("Plonie szesc budynkow"), O->Plonacych(), 6);
		TestEqual(TEXT("Szesc pozarow w efektach"), E ? E->Pozarow() : -1, 6);
		const int32 Z2Bylo = Instancji(O, TEXT("_Z2"));
		for (int32 k = 0; k < 40; ++k)
		{
			O->Tick(0.5f); // 5 s gry
			if (E) { E->Tick(0.1f); }
		}
		UE_LOG(LogTemp, Display, TEXT("[PB-TEST] po 200 s gry: plonie %d, zniszczonych %d, zgliszcz w modelach %d (bylo %d)"),
			O->Plonacych(), O->Zniszczonych(), Instancji(O, TEXT("_Z2")), Z2Bylo);
		TestEqual(TEXT("Pozary zgasly"), O->Plonacych(), 0);
		TestEqual(TEXT("Szesc zgliszcz"), O->Zniszczonych(), 6);
		TestEqual(TEXT("Zgliszcza widac"), Instancji(O, TEXT("_Z2")), Z2Bylo + 6);

		// Nowa pora roku: brama naprawiona, dwa budynki odbudowane.
		O->Odswiez(R, EPora::Jesien);
		TestFalse(TEXT("Brama naprawiona"), O->BramaLezy(0));
		TestEqual(TEXT("Dwa budynki odbudowane"), O->Zniszczonych(), 4);

		// Rozkaz "podpal ten budynek" (24.11.2026): najblizszy caly budynek, drugi raz nie zapala sie.
		const int32 Dz = O->DoSpalenia(R.Gdzie, 8000.f);
		if (TestTrue(TEXT("Jest budynek do podpalenia"), Dz >= 0))
		{
			float Pol = 0.f;
			const FVector2D M = O->MiejsceDzialki(Dz, Pol);
			TestEqual(TEXT("Klik w budynek go wskazuje"), O->DoSpalenia(M, 250.f), Dz);
			const int32 Plonie = O->Plonacych();
			TestTrue(TEXT("Woj podpala"), O->Podpal(Dz));
			TestFalse(TEXT("Plonacego nie podpala drugi raz"), O->Podpal(Dz));
			TestEqual(TEXT("Jeden pozar wiecej"), O->Plonacych(), Plonie + 1);
			TestNotEqual(TEXT("Plonacy nie jest juz do podpalenia"), O->DoSpalenia(M, 250.f), Dz);
		}
		O->Destroy();
	}

	// Skutki wyprawy w liczbach rodu.
	{
		FRod& H = Lista[RI];
		const int32 Grosze = H.Grosze;
		A->PoWyprawie(RI, 1, 3);
		TestTrue(TEXT("Hold: lennik, nie wrogi"), H.bLennik && !H.bWrogi);
		TestEqual(TEXT("Hold: 30% groszy"), H.Grosze, Grosze - Grosze * 3 / 10);
		const int32 Inny = (RI + 1) % Lista.Num();
		FRod& L = Lista[Inny];
		L.Zapas[ESurowiec::Jedzenie] = 40;
		A->PoWyprawie(Inny, 0, 0);
		TestEqual(TEXT("Lup: polowa jedzenia"), L.Zapas[ESurowiec::Jedzenie], 20);
		TestTrue(TEXT("Lup: rod wrogi"), L.bWrogi && !L.bLennik);
		const int32 Trzeci = (RI + 2) % Lista.Num();
		FRod& S = Lista[Trzeci];
		S.Rodzin = 30;
		A->PoWyprawie(Trzeci, 2, 0);
		TestEqual(TEXT("Spalenie: traci trzecia czesc rodzin"), S.Rodzin, 20);
		const int32 Czwarty = (RI + 3) % Lista.Num();
		Lista[Czwarty].bWrogi = false;
		const FString T = A->PoWyprawie(Czwarty, -1, 5);
		TestTrue(TEXT("Odparta wyprawa: rod wrogi"), Lista[Czwarty].bWrogi);
		UE_LOG(LogTemp, Display, TEXT("[PB-TEST] odparta: %s"), *T);
	}
	ABudowa::Tempo = TempoBylo;
	GEngine->DestroyWorldContext(W);
	W->DestroyWorld(false);
	return true;
}
#endif
