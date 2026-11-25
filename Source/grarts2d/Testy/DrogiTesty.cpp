// Plik: Source/grarts2d/Testy/DrogiTesty.cpp
// Luk drogi gracza (ADrogi::Krzywa, 23.11.2026: dopasowanie do drog obok) bez uruchamiania gry: droga wychodzi z A
// w zadanym kierunku, wchodzi w B w zadanym kierunku (stycznie w koniec drogi, prostopadle w srodek), punkty co ok. Probka.
#include "Misc/AutomationTest.h"
#include "Osada/Drogi.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrogiLukTest, "grarts2d.Drogi.Luk", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FDrogiLukTest::RunTest(const FString&)
{
	auto Kier = [](const TArray<FVector2D>& P, bool bKoniec)
	{
		return bKoniec ? (P.Last() - P[P.Num() - 2]).GetSafeNormal() : (P[1] - P[0]).GetSafeNormal();
	};
	auto MaxKrok = [](const TArray<FVector2D>& P)
	{
		double M = 0.0;
		for (int32 i = 1; i < P.Num(); ++i) { M = FMath::Max(M, FVector2D::Distance(P[i - 1], P[i])); }
		return M;
	};
	TArray<FVector2D> P;

	// Wyjscie na wschod, wejscie w B jadac na polnoc (np. prostopadle w srodek drogi biegnacej wschod-zachod).
	const FVector2D A(0.0, 0.0), B(3000.0, 3000.0);
	ADrogi::Krzywa(A, FVector2D(1.0, 0.0), B, FVector2D(0.0, 1.0), P);
	TestTrue(TEXT("Poczatek i koniec na miejscu"), P[0].Equals(A, 1.0) && P.Last().Equals(B, 1.0));
	TestTrue(TEXT("Wychodzi w zadanym kierunku"), (Kier(P, false) | FVector2D(1.0, 0.0)) > 0.97);
	TestTrue(TEXT("Wchodzi w zadanym kierunku"), (Kier(P, true) | FVector2D(0.0, 1.0)) > 0.97);
	TestTrue(TEXT("Punkty co ok. Probka"), MaxKrok(P) <= ADrogi::Probka * 1.35);

	// Bez warunkow: prosto.
	ADrogi::Krzywa(A, FVector2D::ZeroVector, FVector2D(2000.0, 0.0), FVector2D::ZeroVector, P);
	double Odchyl = 0.0;
	for (const FVector2D& Q : P) { Odchyl = FMath::Max(Odchyl, FMath::Abs(Q.Y)); }
	TestTrue(TEXT("Bez kierunkow prosto"), Odchyl < 1.0);

	// Kierunek do tylu nie robi petli: punkty ida od A do B.
	ADrogi::Krzywa(A, FVector2D(-1.0, 0.0), FVector2D(2000.0, 0.0), FVector2D::ZeroVector, P);
	bool bNaprzod = true;
	for (int32 i = 1; i < P.Num(); ++i) { bNaprzod &= P[i].X >= P[i - 1].X - 1.0; }
	TestTrue(TEXT("Kierunek do tylu - bez petli"), bNaprzod);
	return true;
}
#endif
