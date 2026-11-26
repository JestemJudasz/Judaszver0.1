// Plik: Source/grarts2d/Testy/ZapisTesty.cpp
// Zapis gry (P4, 26.11.2026) bez uruchamiania gry: osada z obozem, gotowa chata, plac budowy, pole z uprawa, rodzina w pracy
// i droga trafia do slotu 9, potem nowa osada wczytuje go (FZapisGry::Zastosuj) i ma to samo. Linie [ZAPIS-TEST] w Saved\Testy.log.
#include "Misc/AutomationTest.h"
#include "Gra/ZapisGry.h"
#include "Budowy/Budowa.h"
#include "Osada/Osada.h"
#include "Osada/Drogi.h"
#include "Swiat/SwiatWysokosci.h"
#include "HAL/FileManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FZapisGryTest, "grarts2d.Gra.Zapis", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FZapisGryTest::RunTest(const FString&)
{
	FSwiat::Wczytaj();
	FMath::RandInit(77);
	UWorld* W = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
	Ctx.SetCurrentWorld(W);
	constexpr int32 Slot = 9;

	AOsada* O = W->SpawnActor<AOsada>();
	ADrogi* D = W->SpawnActor<ADrogi>();
	if (TestNotNull(TEXT("Osada"), O) && TestNotNull(TEXT("Drogi"), D))
	{
		O->UstawWidocznosc(false);
		const FVector2D S = FSwiat::Start();
		TArray<FVector> Miejsca;
		AOsada::MiejscaObozu(S, Miejsca);
		TArray<float> Z;
		for (const FVector& M : Miejsca)
		{
			Z.Add(FSwiat::Wysokosc(M.X, M.Y));
		}
		O->ZalozOboz(S, Z);
		auto Stoi = [&](const FRodzajBudynku& R, FVector2D Gdzie, float Postep)
		{
			ABudowa* B = O->Postaw(R, Gdzie, 30.f, FSwiat::Wysokosc(Gdzie.X, Gdzie.Y));
			if (B)
			{
				B->UstawPostep(Postep);
			}
			return B;
		};
		ABudowa* Drwal = Stoi(FRodzajBudynku::ChataDrwala(), S + FVector2D(4000.f, 0.f), 1.f);
		ABudowa* Plac = Stoi(FRodzajBudynku::Chata(), S + FVector2D(0.f, 4000.f), 0.4f);
		ABudowa* Pole = Stoi(FRodzajBudynku::NaObszar(FRodzajBudynku::Pole(), 3000.f, 2000.f), S + FVector2D(-5000.f, 0.f), 1.f);
		if (Drwal) { Drwal->ZmienZapas(ESurowiec::Kloda, 3); O->DodajRodzine(Drwal, true); }
		if (Pole) { Pole->UstawUprawe(EUprawa::Rosnie); Pole->UstawWzrost(0.5f); }
		O->Grosze = 123;
		O->UstawCzas(700.f);
		D->DodajDroge({ S + FVector2D(1500.f, 1500.f), S + FVector2D(1500.f, 3500.f) });

		FString Blad;
		const bool bZapis = FZapisGry::Zapisz(W, Slot, Blad);
		TestTrue(FString::Printf(TEXT("Zapis udany (%s)"), *Blad), bZapis);
		const int32 Rodzin = O->Rodzin(), Budow = O->Budow(), WPracy = Drwal ? O->RodzinWPracy(Drwal) : -1;
		const FSurowce Zapasy = O->Zapasy();
		UE_LOG(LogTemp, Display, TEXT("[ZAPIS-TEST] przed: %d rodzin, %d budow, %d w pracy u drwala, zapas %d, opis: %s"), Rodzin, Budow, WPracy, Zapasy.Suma(), *FZapisGry::Opis(Slot));

		// Nowa osada w tym samym swiecie (stara znika, jej budynki zostaja jako zwykle aktory) wczytuje slot.
		O->Destroy();
		D->Destroy();
		AOsada* O2 = W->SpawnActor<AOsada>();
		ADrogi* D2 = W->SpawnActor<ADrogi>();
		if (bZapis && TestNotNull(TEXT("Nowa osada"), O2))
		{
			O2->UstawWidocznosc(false);
			FZapisGry::DoWczytania = Slot;
			FZapisGry::Zastosuj(W);
			UE_LOG(LogTemp, Display, TEXT("[ZAPIS-TEST] po: %d rodzin, %d budow, zapas %d, grosze %d, %s, drog %d"), O2->Rodzin(), O2->Budow(), O2->Zapasy().Suma(), O2->Grosze, *O2->NapisPory(), D2 ? D2->Odcinkow() : -1);
			TestTrue(TEXT("Oboz po wczytaniu"), O2->MaOboz());
			TestEqual(TEXT("Te same rodziny"), O2->Rodzin(), Rodzin);
			TestEqual(TEXT("Te same budowy"), O2->Budow(), Budow);
			TestEqual(TEXT("Te same zapasy"), O2->Zapasy().Suma(), Zapasy.Suma());
			TestEqual(TEXT("Te same grosze"), O2->Grosze, 123);
			TestEqual(TEXT("Ten sam czas"), O2->Czas().Czas, 700.f);
			TestEqual(TEXT("Droga wczytana"), D2 ? D2->Odcinkow() : 0, 1);
			ABudowa* Drwal2 = O2->BudynekPod(S + FVector2D(4000.f, 0.f));
			ABudowa* Pole2 = O2->BudynekPod(S + FVector2D(-5000.f, 0.f));
			ABudowa* Plac2 = O2->BudynekPod(S + FVector2D(0.f, 4000.f));
			TestTrue(TEXT("Chata drwala z rodzina"), Drwal2 && Drwal2->Gotowy() && O2->RodzinWPracy(Drwal2) == WPracy);
			TestTrue(TEXT("Pole z obszarem i uprawa"), Pole2 && FMath::IsNearlyEqual(Pole2->Rodzaj().Obszar.X, 3000.f) && Pole2->Uprawa == EUprawa::Rosnie);
			TestTrue(TEXT("Plac budowy z postepem"), Plac2 && FMath::IsNearlyEqual(Plac2->Postep(), 0.4f, 0.01f));
			TestTrue(TEXT("Obrot zachowany"), Drwal2 && FMath::IsNearlyEqual(float(Drwal2->GetActorRotation().Yaw), 30.f, 0.5f));
		}
	}
	IFileManager::Get().Delete(*FZapisGry::Plik(Slot));
	GEngine->DestroyWorldContext(W);
	W->DestroyWorld(false);
	return true;
}
#endif
