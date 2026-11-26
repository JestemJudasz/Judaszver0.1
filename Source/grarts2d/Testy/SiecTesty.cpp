// Plik: Source/grarts2d/Testy/SiecTesty.cpp
// Gra w sieci (M1, 26.11.2026): dwa lacza w jednym procesie przez 127.0.0.1 - host czeka, drugi dolacza, wiadomosci
// ida w obie strony (z polskimi literami i w kawalkach), pozegnanie rozlacza hosta, zly adres jest odrzucony.
// Linie [SIEC-TEST] w Saved\Testy.log.
#include "Misc/AutomationTest.h"
#include "Siec/Lacze.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	bool Czekaj(FPolaczenie& A, FPolaczenie& B, TFunction<bool()> Warunek, double Sekundy = 5.0)
	{
		const double Koniec = FPlatformTime::Seconds() + Sekundy;
		while (FPlatformTime::Seconds() < Koniec)
		{
			A.Tick();
			B.Tick();
			if (Warunek())
			{
				return true;
			}
			FPlatformProcess::Sleep(0.01f);
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiecLaczeTest, "grarts2d.Siec.Lacze", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FSiecLaczeTest::RunTest(const FString&)
{
	constexpr int32 Port = 7791;
	FPolaczenie Host, Gosc;
	FString Blad;
	TestTrue(TEXT("host nasluchuje"), Host.Hostuj(Blad, Port));
	TestFalse(TEXT("zly adres odrzucony"), Gosc.Dolacz(TEXT("to nie adres"), Blad));
	TestTrue(TEXT("gosc laczy"), Gosc.Dolacz(FString::Printf(TEXT("127.0.0.1:%d"), Port), Blad));
	const bool bPol = Czekaj(Host, Gosc, [&]() { return Host.Polaczony() && Gosc.Polaczony(); });
	TestTrue(TEXT("obie strony polaczone"), bPol);
	UE_LOG(LogTemp, Display, TEXT("[SIEC-TEST] polaczenie: %s / %s"), *Host.Opis(), *Gosc.Opis());

	const FString Dluga = FString::ChrN(20000, TEXT('ż'));
	Gosc.Wyslij(TEXT("{\"t\":\"czat\",\"s\":\"Cześć, łódź!\"}"));
	Gosc.Wyslij(Dluga);
	Host.Wyslij(TEXT("{\"t\":\"stan\"}"));
	TArray<FString> UHosta, UGoscia;
	Czekaj(Host, Gosc, [&]()
	{
		FString L;
		while (Host.Odbierz(L)) { UHosta.Add(L); }
		while (Gosc.Odbierz(L)) { UGoscia.Add(L); }
		return UHosta.Num() >= 2 && UGoscia.Num() >= 1;
	});
	TestEqual(TEXT("host dostal 2 linie"), UHosta.Num(), 2);
	TestEqual(TEXT("czat z polskimi literami"), UHosta.Num() > 0 ? UHosta[0] : FString(), FString(TEXT("{\"t\":\"czat\",\"s\":\"Cześć, łódź!\"}")));
	TestTrue(TEXT("dluga linia cala"), UHosta.Num() > 1 && UHosta[1] == Dluga);
	TestEqual(TEXT("gosc dostal stan"), UGoscia.Num() > 0 ? UGoscia[0] : FString(), FString(TEXT("{\"t\":\"stan\"}")));

	Gosc.Rozlacz(TEXT("test"));
	const bool bKoniec = Czekaj(Host, Gosc, [&]() { return Host.Stan() == FPolaczenie::EStan::Brak; });
	TestTrue(TEXT("host widzi rozlaczenie"), bKoniec);
	UE_LOG(LogTemp, Display, TEXT("[SIEC-TEST] po rozlaczeniu host: %s"), *Host.Opis());

	// Port po rozlaczeniu znow wolny: mozna hostowac od nowa.
	TestTrue(TEXT("ponowne hostowanie"), Host.Hostuj(Blad, Port));
	Host.Rozlacz();
	Gosc.Rozlacz();
	return true;
}

#endif
