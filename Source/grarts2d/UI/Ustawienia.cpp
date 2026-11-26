// Plik: Source/grarts2d/UI/Ustawienia.cpp
#include "UI/Ustawienia.h"
#include "Engine/Engine.h"
#include "Engine/UserInterfaceSettings.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Internationalization/Internationalization.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"

float FUstawienia::GlosOgolna = 1.f;
float FUstawienia::GlosKat[8] = { 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f };
float FUstawienia::PrzesuwMn = 1.f;
float FUstawienia::ObrotMn = 1.f;
float FUstawienia::MyszMn = 1.f;
float FUstawienia::ZoomMn = 1.f;
bool FUstawienia::bOdwrocZoom = false;
bool FUstawienia::bOdwrocObrot = false;
FString FUstawienia::Wygladzanie = TEXT("TSR");
float FUstawienia::SkalaRenderu = 100.f;
float FUstawienia::SkalaUI = 1.f;
FString FUstawienia::Jezyk = TEXT("pl");

namespace
{
	const TCHAR* Sekcja = TEXT("Ustawienia");
	TMap<FName, FKey> Przypisane;

	IConsoleVariable* Zmienna(const TCHAR* N) { return IConsoleManager::Get().FindConsoleVariable(N); }
	void Ustaw(const TCHAR* N, float V) { if (IConsoleVariable* C = Zmienna(N)) { C->Set(V, ECVF_SetByCode); } }
	// FSR: rozne wersje wtyczki AMD maja rozne nazwy przelacznika; pierwszy istniejacy wygrywa.
	const TCHAR* ZmiennaFSR()
	{
		for (const TCHAR* N : { TEXT("r.FidelityFX.FSR4.Enabled"), TEXT("r.FidelityFX.FSR3.Enabled"), TEXT("r.FidelityFX.FSR.Enabled") })
		{
			if (Zmienna(N)) { return N; }
		}
		return nullptr;
	}
}

const TArray<FUstawienia::FAkcja>& FUstawienia::Akcje()
{
	static const TArray<FAkcja> A = {
		{ "Przod",       TEXT("Kamera do przodu"),            TEXT("Kamera"), EKeys::W },
		{ "Tyl",         TEXT("Kamera do tyłu"),              TEXT("Kamera"), EKeys::S },
		{ "Lewo",        TEXT("Kamera w lewo"),               TEXT("Kamera"), EKeys::A },
		{ "Prawo",       TEXT("Kamera w prawo"),              TEXT("Kamera"), EKeys::D },
		{ "ObrotL",      TEXT("Obrót kamery w lewo"),         TEXT("Kamera"), EKeys::Q },
		{ "ObrotP",      TEXT("Obrót kamery w prawo"),        TEXT("Kamera"), EKeys::E },
		{ "Szybciej",    TEXT("Szybszy przesuw kamery"),      TEXT("Kamera"), EKeys::LeftShift },
		{ "Budowa",      TEXT("Budowa"),                      TEXT("Osada"),  EKeys::B },
		{ "Drogi",       TEXT("Drogi"),                       TEXT("Osada"),  EKeys::G },
		{ "Wojsko",      TEXT("Okno wojska"),                 TEXT("Osada"),  EKeys::J },
		{ "Rody",        TEXT("Rody i handel"),               TEXT("Osada"),  EKeys::H },
		{ "Rozwoj",      TEXT("Rozwój osady"),                TEXT("Osada"),  EKeys::O },
		{ "Minimapa",    TEXT("Minimapa wł. / wył."),         TEXT("Kamera"), EKeys::M },
		{ "ObrotBud",    TEXT("Obrót budynku w prawo"),       TEXT("Osada"),  EKeys::R },
		{ "ObrotBudL",   TEXT("Obrót budynku w lewo"),        TEXT("Osada"),  EKeys::F },
		{ "Tempo",       TEXT("Tempo budowy x10"),            TEXT("Osada"),  EKeys::T },
		{ "Pauza",       TEXT("Pauza bitwy"),                 TEXT("Bitwa"),  EKeys::SpaceBar },
		{ "Odwrot",      TEXT("Odwrót oddziału"),             TEXT("Bitwa"),  EKeys::R },
		{ "Szyk",        TEXT("Szyk zwarty / luźny"),         TEXT("Bitwa"),  EKeys::F },
		{ "Krew",        TEXT("Krew wł. / wył."),             TEXT("Bitwa"),  EKeys::K },
	};
	return A;
}

const FKey& FUstawienia::Klawisz(FName Akcja)
{
	if (const FKey* K = Przypisane.Find(Akcja)) { return *K; }
	for (const FAkcja& A : Akcje()) { if (A.Nazwa == Akcja) { return A.Domyslny; } }
	return EKeys::Invalid;
}

void FUstawienia::UstawKlawisz(FName Akcja, const FKey& K)
{
	Przypisane.Add(Akcja, K);
	GConfig->SetString(TEXT("Klawisze"), *Akcja.ToString(), *K.ToString(), GGameUserSettingsIni);
}

void FUstawienia::DomyslneKlawisze()
{
	for (const FAkcja& A : Akcje()) { UstawKlawisz(A.Nazwa, A.Domyslny); }
}

float FUstawienia::Glosnosc(int32 K)
{
	return GlosOgolna * (K >= 0 && K < UE_ARRAY_COUNT(GlosKat) ? GlosKat[K] : 1.f);
}

TArray<FString> FUstawienia::MetodyWygladzania()
{
	TArray<FString> M = { TEXT("TSR") };
	if (Zmienna(TEXT("r.NGX.DLSS.Enable"))) { M.Add(TEXT("DLSS")); }
	if (ZmiennaFSR()) { M.Add(TEXT("FSR")); }
	if (Zmienna(TEXT("r.XeSS.Enabled"))) { M.Add(TEXT("XeSS")); }
	M.Append({ TEXT("TAA"), TEXT("FXAA"), TEXT("Brak") });
	return M;
}

void FUstawienia::ZastosujObraz()
{
	if (!MetodyWygladzania().Contains(Wygladzanie)) { Wygladzanie = TEXT("TSR"); } // wtyczka usunieta
	SkalaRenderu = FMath::Clamp(SkalaRenderu, 50.f, 100.f);
	const bool bDLSS = Wygladzanie == TEXT("DLSS"), bFSR = Wygladzanie == TEXT("FSR"), bXeSS = Wygladzanie == TEXT("XeSS");
	Ustaw(TEXT("r.NGX.DLSS.Enable"), bDLSS ? 1.f : 0.f);
	Ustaw(TEXT("r.NGX.DLAA.Enable"), bDLSS && SkalaRenderu >= 99.5f ? 1.f : 0.f); // DLSS w 100% = DLAA
	if (const TCHAR* F = ZmiennaFSR()) { Ustaw(F, bFSR ? 1.f : 0.f); }
	Ustaw(TEXT("r.XeSS.Enabled"), bXeSS ? 1.f : 0.f);
	// Upscalery z wtyczek podpinaja sie pod czasowe wygladzanie, wiec dla nich zostaje metoda 4 (TSR).
	const float Metoda = Wygladzanie == TEXT("TAA") ? 2.f : Wygladzanie == TEXT("FXAA") ? 1.f : Wygladzanie == TEXT("Brak") ? 0.f : 4.f;
	Ustaw(TEXT("r.AntiAliasingMethod"), Metoda);
	Ustaw(TEXT("r.ScreenPercentage"), SkalaRenderu);
}

void FUstawienia::ZastosujUI()
{
	GetMutableDefault<UUserInterfaceSettings>()->ApplicationScale = FMath::Clamp(SkalaUI, 0.6f, 2.f);
	FInternationalization::Get().SetCurrentCulture(Jezyk);
}

void FUstawienia::Wczytaj()
{
	const FString& Ini = GGameUserSettingsIni;
	GConfig->GetFloat(TEXT("Dzwiek"), TEXT("Ogolna"), GlosOgolna, Ini);
	for (int32 i = 0; i < UE_ARRAY_COUNT(GlosKat); ++i)
	{
		GConfig->GetFloat(TEXT("Dzwiek"), *FString::Printf(TEXT("Kat%d"), i), GlosKat[i], Ini);
	}
	GConfig->GetFloat(Sekcja, TEXT("PrzesuwMn"), PrzesuwMn, Ini);
	GConfig->GetFloat(Sekcja, TEXT("ObrotMn"), ObrotMn, Ini);
	GConfig->GetFloat(Sekcja, TEXT("MyszMn"), MyszMn, Ini);
	GConfig->GetFloat(Sekcja, TEXT("ZoomMn"), ZoomMn, Ini);
	GConfig->GetBool(Sekcja, TEXT("OdwrocZoom"), bOdwrocZoom, Ini);
	GConfig->GetBool(Sekcja, TEXT("OdwrocObrot"), bOdwrocObrot, Ini);
	GConfig->GetString(Sekcja, TEXT("Wygladzanie"), Wygladzanie, Ini);
	GConfig->GetFloat(Sekcja, TEXT("SkalaRenderu"), SkalaRenderu, Ini);
	GConfig->GetFloat(Sekcja, TEXT("SkalaUI"), SkalaUI, Ini);
	GConfig->GetString(Sekcja, TEXT("Jezyk"), Jezyk, Ini);
	for (const FAkcja& A : Akcje())
	{
		FString N;
		if (GConfig->GetString(TEXT("Klawisze"), *A.Nazwa.ToString(), N, Ini) && FKey(*N).IsValid())
		{
			Przypisane.Add(A.Nazwa, FKey(*N));
		}
	}
	if (UGameUserSettings* G = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		// -zrzut: okno kontrolne zostaje takie, jak z linii polecen.
		FParse::Param(FCommandLine::Get(), TEXT("zrzut")) ? G->ApplyNonResolutionSettings() : G->ApplySettings(false);
	}
	ZastosujObraz();
	ZastosujUI();
}

void FUstawienia::Zapisz()
{
	const FString& Ini = GGameUserSettingsIni;
	GConfig->SetFloat(TEXT("Dzwiek"), TEXT("Ogolna"), GlosOgolna, Ini);
	for (int32 i = 0; i < UE_ARRAY_COUNT(GlosKat); ++i)
	{
		GConfig->SetFloat(TEXT("Dzwiek"), *FString::Printf(TEXT("Kat%d"), i), GlosKat[i], Ini);
	}
	GConfig->SetFloat(Sekcja, TEXT("PrzesuwMn"), PrzesuwMn, Ini);
	GConfig->SetFloat(Sekcja, TEXT("ObrotMn"), ObrotMn, Ini);
	GConfig->SetFloat(Sekcja, TEXT("MyszMn"), MyszMn, Ini);
	GConfig->SetFloat(Sekcja, TEXT("ZoomMn"), ZoomMn, Ini);
	GConfig->SetBool(Sekcja, TEXT("OdwrocZoom"), bOdwrocZoom, Ini);
	GConfig->SetBool(Sekcja, TEXT("OdwrocObrot"), bOdwrocObrot, Ini);
	GConfig->SetString(Sekcja, TEXT("Wygladzanie"), *Wygladzanie, Ini);
	GConfig->SetFloat(Sekcja, TEXT("SkalaRenderu"), SkalaRenderu, Ini);
	GConfig->SetFloat(Sekcja, TEXT("SkalaUI"), SkalaUI, Ini);
	GConfig->SetString(Sekcja, TEXT("Jezyk"), *Jezyk, Ini);
	if (UGameUserSettings* G = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		G->SaveSettings(); // zapisuje tez caly GameUserSettings.ini
	}
	GConfig->Flush(false, Ini);
}
