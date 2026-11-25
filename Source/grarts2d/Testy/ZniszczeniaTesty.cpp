// Plik: Source/grarts2d/Testy/ZniszczeniaTesty.cpp
// Zniszczenia budynkow (23.11.2026, wojna) bez uruchamiania gry, w tymczasowym swiecie:
//   - palnosc: chata z drewna pali sie mocno, kosciol kamienny slabo;
//   - ponizej 70% wytrzymalosci model uszkodzony (_Z1), pozar trawi chate az do ruiny (_Z2), efekty maja czastki;
//   - osada odlacza ruine (licznik ruin, klik ja znajduje), odbudowa robi z niej plac budowy;
//   - plac budowy spala sie do cna i znika; ogien przeskakuje na chate stojaca 2 m obok.
// Linie [ZN-TEST] w Saved\Testy.log (Tools\Testy.bat).
#include "Misc/AutomationTest.h"
#include "Budowy/Budowa.h"
#include "Osada/Osada.h"
#include "Efekty/Efekty.h"
#include "Swiat/SwiatWysokosci.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString ModelBudynku(const ABudowa* B)
	{
		const UStaticMeshComponent* C = B ? B->FindComponentByClass<UStaticMeshComponent>() : nullptr;
		return C && C->GetStaticMesh() ? C->GetStaticMesh()->GetName() : FString(TEXT("(brak)"));
	}

	/** Pali budynek do konca (sekundy gry = Krokow x 0,5 s x Tempo). Zwraca, po ilu krokach sie zawalil (-1 = nie). */
	int32 Spal(ABudowa* B, AEfekty* E, int32 Krokow)
	{
		for (int32 k = 0; k < Krokow; ++k)
		{
			B->Tick(0.5f);
			if (E)
			{
				E->Tick(0.1f);
			}
			if (B->Ruina() || B->Zgliszcza())
			{
				return k;
			}
		}
		return -1;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZniszczeniaTest, "grarts2d.Wojna.Zniszczenia", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZniszczeniaTest::RunTest(const FString&)
{
	FSwiat::Wczytaj();
	FMath::RandInit(4242);
	const float TempoBylo = ABudowa::Tempo;
	ABudowa::Tempo = 10.f;
	UWorld* W = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(W);

	AOsada* O = W->SpawnActor<AOsada>();
	if (TestNotNull(TEXT("Osada"), O))
	{
		O->UstawWidocznosc(false); // bez przydzialu pracy i wolow (w tescie nie ma ludzi)
		const FVector2D S = FSwiat::Start();
		auto Stoi = [&](const FRodzajBudynku& R, FVector2D Gdzie)
		{
			ABudowa* B = O->Postaw(R, Gdzie, 0.f, FSwiat::Wysokosc(Gdzie.X, Gdzie.Y));
			if (B)
			{
				B->UstawPostep(1.f);
			}
			return B;
		};
		ABudowa* Chata = Stoi(FRodzajBudynku::Chata(), S);
		ABudowa* Kosciol = Stoi(FRodzajBudynku::Kosciol(), S + FVector2D(6000.f, 0.f));
		AEfekty* E = AEfekty::W(W);
		if (TestNotNull(TEXT("Chata"), Chata) && TestNotNull(TEXT("Kosciol"), Kosciol) && TestNotNull(TEXT("Efekty"), E))
		{
			UE_LOG(LogTemp, Display, TEXT("[ZN-TEST] palnosc: chata %.2f, kosciol %.2f; wysokosc chaty %.0f cm"), Chata->Palnosc(), Kosciol->Palnosc(), Chata->Wysokosc());
			TestTrue(TEXT("Chata pali sie mocno"), Chata->Palnosc() > 0.6f);
			TestTrue(TEXT("Kosciol kamienny pali sie slabo"), Kosciol->Palnosc() < 0.5f);
			TestEqual(TEXT("Cala chata: wytrzymalosc 1"), Chata->Wytrzymalosc(), 1.f);

			// Uszkodzenie: model _Z1 dopiero ponizej 70%.
			Kosciol->Uszkodz(1.f);
			TestFalse(TEXT("Lekko uszkodzony kosciol bez zmiany modelu"), ModelBudynku(Kosciol).EndsWith(TEXT("_Z1")));
			int32 Uderzen = 1;
			for (; Uderzen < 1000 && Kosciol->Wytrzymalosc() >= 0.69f; ++Uderzen)
			{
				Kosciol->Uszkodz(5.f); // taran: po 5 punktow
			}
			UE_LOG(LogTemp, Display, TEXT("[ZN-TEST] kosciol: %d uderzen do 70%%, model %s"), Uderzen, *ModelBudynku(Kosciol));
			TestTrue(TEXT("Uszkodzony kosciol ma model _Z1"), !Kosciol->Ruina() && ModelBudynku(Kosciol).EndsWith(TEXT("_Z1")));

			// Pozar chaty az do ruiny.
			Chata->Podpal(0.3f);
			TestTrue(TEXT("Chata plonie"), Chata->Ogien() > 0.f);
			TestEqual(TEXT("Jeden pozar w efektach"), E->Pozarow(), 1);
			const int32 Kroki = Spal(Chata, E, 400);
			UE_LOG(LogTemp, Display, TEXT("[ZN-TEST] chata zawalila sie po %d s gry, model %s, czastek %d"), Kroki < 0 ? -1 : FMath::RoundToInt32(Kroki * 5.f), *ModelBudynku(Chata), E->Czastek());
			TestTrue(TEXT("Chata splonela"), Kroki > 0 && Chata->Ruina());
			TestTrue(TEXT("Ruina ma model _Z2"), ModelBudynku(Chata).EndsWith(TEXT("_Z2")));
			TestFalse(TEXT("Ruina nie jest gotowym budynkiem"), Chata->Gotowy());
			TestTrue(TEXT("Zawalenie: kurz, gruz, iskry"), E->Czastek() > 30);
			TestTrue(TEXT("Zgliszcza sie tla"), Chata->Ogien() > 0.f);

			// Osada odlacza ruine; odbudowa = plac budowy od zera.
			O->Tick(1.f);
			const int32 BudowBylo = O->Budow();
			TestEqual(TEXT("Osada widzi jedna ruine"), O->Ruin(), 1);
			TestTrue(TEXT("Klik w ruine ja znajduje"), O->BudynekPod(FVector2D(Chata->GetActorLocation())) == Chata);
			TestFalse(TEXT("Ruiny nie da sie rozebrac jak budynku"), AOsada::MoznaRozebrac(Chata));
			TestTrue(TEXT("Odbudowa"), O->Odbuduj(Chata));
			TestTrue(TEXT("Po odbudowie plac budowy"), !Chata->Ruina() && !Chata->Gotowy() && Chata->Postep() == 0.f && Chata->Ogien() == 0.f);
			TestEqual(TEXT("Ruin juz nie ma"), O->Ruin(), 0);
			TestEqual(TEXT("Odbudowa liczy sie jako budowa"), O->Budow(), BudowBylo + 1);
			UE_LOG(LogTemp, Display, TEXT("[ZN-TEST] odbudowa: model %s, budow %d"), *ModelBudynku(Chata), O->Budow());

			// Plac budowy spala sie do cna i znika z osady.
			ABudowa* Plac = O->Postaw(FRodzajBudynku::Chata(), S + FVector2D(0.f, 6000.f), 0.f, FSwiat::Wysokosc(S.X, S.Y + 6000.f));
			if (TestNotNull(TEXT("Plac budowy"), Plac))
			{
				Plac->UstawPostep(0.4f);
				Plac->Podpal(0.5f);
				const int32 KP = Spal(Plac, E, 400);
				TestTrue(TEXT("Plac budowy splonal"), KP > 0 && Plac->Zgliszcza());
				O->Tick(1.f);
				TestTrue(TEXT("Spalony plac budowy usuniety"), !IsValid(Plac) || Plac->IsActorBeingDestroyed());
			}

			// Ogien przeskakuje na chate 2 m obok (losowo, ale przez 2 minuty gry prawie na pewno).
			const FVector2D A = S + FVector2D(-6000.f, 0.f);
			ABudowa* Pierwsza = Stoi(FRodzajBudynku::Chata(), A);
			ABudowa* Druga = Stoi(FRodzajBudynku::Chata(), A + FVector2D(2.f * FRodzajBudynku::Chata().Polowa.X + 200.f, 0.f));
			if (TestNotNull(TEXT("Dwie chaty"), Pierwsza) && TestNotNull(TEXT("Druga chata"), Druga))
			{
				Pierwsza->Podpal(1.f);
				int32 K = 0;
				for (; K < 600 && Druga->Ogien() <= 0.f && !Pierwsza->Ruina(); ++K)
				{
					Pierwsza->Tick(0.1f); // 1 s gry
				}
				UE_LOG(LogTemp, Display, TEXT("[ZN-TEST] ogien przeskoczyl: %s po %d s gry"), Druga->Ogien() > 0.f ? TEXT("tak") : TEXT("nie"), K);
				TestTrue(TEXT("Ogien przeskoczyl na sasiada"), Druga->Ogien() > 0.f);
			}
		}
	}
	ABudowa::Tempo = TempoBylo;
	GEngine->DestroyWorldContext(W);
	W->DestroyWorld(false);
	return true;
}
#endif
