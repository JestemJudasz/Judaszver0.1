// Plik: Source/grarts2d/Testy/SwiatTesty.cpp
// Testy automatyczne swiata (bez uruchamiania gry). Uruchomienie: Tools\Testy.bat (wynik w Saved\Testy.log).
// Punkty kontrolne w ukladzie gry (cm, X = polnoc, Y = wschod) policzone z Tools/dem_europa.py (do_xy * 12,5).
#include "Misc/AutomationTest.h"
#include "Swiat/SwiatWysokosci.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector2D Baltyk(5214540.0, 6285982.0);
	const FVector2D Sniezka(-1558465.0, 5043541.0);
	const FVector2D Sniardwy(3213443.0, 9592863.0);
	constexpr EAutomationTestFlags Flagi = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSwiatDaneTest, "grarts2d.Swiat.Dane", Flagi)
bool FSwiatDaneTest::RunTest(const FString&)
{
	FSwiat::Wczytaj();
	FSwiat::UstawRegion(false);
	if (!TestTrue(TEXT("Dane DEM wczytane"), FSwiat::MaDane()))
	{
		return false;
	}
	const FVector2D Krakow = FSwiat::Start();
	const float Kr = FSwiat::WysokoscMetry(Krakow.X, Krakow.Y);
	TestTrue(FString::Printf(TEXT("Krakow 150-300 m (jest %.0f)"), Kr), Kr > 150.f && Kr < 300.f);
	const float Sn = FSwiat::WysokoscMetry(Sniezka.X, Sniezka.Y);
	TestTrue(FString::Printf(TEXT("Sniezka 900-1700 m (jest %.0f)"), Sn), Sn > 900.f && Sn < 1700.f);
	TestTrue(TEXT("Baltyk to morze"), FSwiat::Woda(Baltyk.X, Baltyk.Y) == FSwiat::EWoda::Morze);
	TestTrue(TEXT("Baltyk ponizej 0"), FSwiat::Wysokosc(Baltyk.X, Baltyk.Y) < 0.f);
	TestTrue(TEXT("Sniardwy to jezioro"), FSwiat::Woda(Sniardwy.X, Sniardwy.Y) == FSwiat::EWoda::Jezioro);
	TestTrue(TEXT("Pokrycie na Baltyku = woda"), FSwiat::Pokrycie(Baltyk.X, Baltyk.Y) == FSwiat::EPokrycie::Woda);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSwiatRegionTest, "grarts2d.Swiat.MapaGrywalna", Flagi)
bool FSwiatRegionTest::RunTest(const FString&)
{
	FSwiat::Wczytaj();
	const FVector2D Krakow = FSwiat::Start();
	FSwiat::UstawRegion(false);
	const float PrzedEuropa = FSwiat::Wysokosc(Krakow.X, Krakow.Y);
	FSwiat::UstawRegion(true);

	// 1) Teren ciagly: linia 6 km przez Krakow co 4 m (przecina ~12 granic kafli 512 m) - bez uskokow.
	float MaxSkok = 0.f;
	float Poprzednia = FSwiat::Wysokosc(Krakow.X - 3.0e5, Krakow.Y + 1234.0);
	// Gladkosc: zmiana spadku miedzy kolejnymi krokami 4 m (druga roznica), z dala od rzek (tam celowo skarpy).
	float Spadek = 0.f, MaxZalom = 0.f;
	for (int32 k = 1; k <= 1500; ++k)
	{
		const double X = Krakow.X - 3.0e5 + k * 400.0, Y = Krakow.Y + 1234.0;
		const float Z = FSwiat::Wysokosc(X, Y);
		MaxSkok = FMath::Max(MaxSkok, FMath::Abs(Z - Poprzednia));
		if (k > 1 && FSwiat::OdRzeki(X, Y) > 3.0e4f)
		{
			const float Zalom = FMath::Abs((Z - Poprzednia) - Spadek);
			MaxZalom = FMath::Max(MaxZalom, Zalom);
		}
		Spadek = Z - Poprzednia;
		Poprzednia = Z;
	}
	TestTrue(FString::Printf(TEXT("Brak uskokow co 4 m (max %.0f cm)"), MaxSkok), MaxSkok < 400.f);
	TestTrue(FString::Printf(TEXT("Gladkie zbocza: zalamania co 4 m max %.1f cm (bylo 156 przed wygladzeniem)"), MaxZalom), MaxZalom < 30.f);

	// 2) Wisla przy Krakowie: najblizszy punkt wody w promieniu 2 km.
	FVector2D Woda = Krakow;
	float Min = TNumericLimits<float>::Max();
	for (int32 i = -50; i <= 50; ++i)
	{
		for (int32 j = -50; j <= 50; ++j)
		{
			const FVector2D P = Krakow + FVector2D(i, j) * 4000.0;
			const float O = FSwiat::OdRzeki(P.X, P.Y);
			if (O < Min)
			{
				Min = O;
				Woda = P;
			}
		}
	}
	if (TestTrue(FString::Printf(TEXT("Wisla w promieniu 2 km (najblizej %.0f cm od brzegu)"), Min), Min < 0.f))
	{
		TestTrue(TEXT("W rzece Woda() = Rzeka"), FSwiat::Woda(Woda.X, Woda.Y) == FSwiat::EWoda::Rzeka);
		// Lustro wody plaskie: probki +-3 m wokol punktu w wodzie.
		float Lo = TNumericLimits<float>::Max(), Hi = -Lo;
		for (int32 k = 0; k < 9; ++k)
		{
			const FVector2D P = Woda + FVector2D(k % 3 - 1, k / 3 - 1) * 300.0;
			if (FSwiat::OdRzeki(P.X, P.Y) < 0.f)
			{
				const float Z = FSwiat::Wysokosc(P.X, P.Y);
				Lo = FMath::Min(Lo, Z);
				Hi = FMath::Max(Hi, Z);
			}
		}
		TestTrue(FString::Printf(TEXT("Lustro wody plaskie (roznica %.0f cm)"), Hi - Lo), Hi - Lo < 50.f);
		// Brzeg nad woda: 100 m od rzeki teren wyzej niz lustro.
		const float Lustro = FSwiat::Wysokosc(Woda.X, Woda.Y);
		bool bBrzegWyzej = true;
		for (const FVector2D D : { FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1) })
		{
			const FVector2D P = Woda + D * 1.0e4;
			if (FSwiat::OdRzeki(P.X, P.Y) > 2000.f)
			{
				bBrzegWyzej &= FSwiat::Wysokosc(P.X, P.Y) > Lustro;
			}
		}
		TestTrue(TEXT("Teren przy rzece wyzej niz lustro wody"), bBrzegWyzej);
	}

	// 3) Wyjscie z mapy grywalnej przywraca wysokosci mapy Europy.
	FSwiat::UstawRegion(false);
	TestEqual(TEXT("Po wyjsciu wysokosc jak na mapie Europy"), FSwiat::Wysokosc(Krakow.X, Krakow.Y), PrzedEuropa);
	TestTrue(TEXT("Poza mapa grywalna brak rzek z linii"), FSwiat::OdRzeki(Woda.X, Woda.Y) >= 1.0e5f);
	return true;
}

#endif
