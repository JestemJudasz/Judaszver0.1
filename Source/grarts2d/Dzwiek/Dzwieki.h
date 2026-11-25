// Plik: Source/grarts2d/Dzwiek/Dzwieki.h
// Dzwiek gry, etap 1 (22.11.2026): jeden podsystem swiata dla calej gry.
//   - Pliki: /Game/Dzwieki/**/<Nazwa>_<n> (import Tools/Zbuduj_Dzwieki.bat). Graj("Nazwa") losuje wariant, nigdy ten sam
//     dwa razy pod rzad, z lekko inna wysokoscia i glosnoscia. Brak plikow = cisza (kod mozna podpinac przed assetami).
//   - Sluchacz stoi na ziemi w ognisku kamery (nie w kamerze 250-1500 m wyzej), obrocony jak kamera: lewo/prawo ekranu.
//   - Tlumienie z odlegloscia od ogniska (15 m pelna glosnosc, cisza od ~265 m), limit glosow na kategorie.
//   - Mix od przyblizenia: z bliska pojedyncze dzwieki, przy oddaleniu gasna (Blisko()), a gra podglasnia petle tla
//     (szum bitwy, osady). Poza mapa grywalna swiat milknie.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Dzwieki.generated.h"

class USoundBase;
class USoundAttenuation;
class USoundConcurrency;
class UAudioComponent;

/** Kategoria: glosnosc, limit jednoczesnych glosow i czy gasnie z oddaleniem. */
UENUM()
enum class EDzwiek : uint8 { Walka, Praca, Glosy, Osada, Rozkazy, Ile };

UCLASS()
class GRARTS2D_API UDzwieki : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UDzwieki* Z(const UObject* Kontekst);
	/** Dzwiek w swiecie w punkcie Gdzie. */
	static void Graj(const UObject* Kontekst, FName Nazwa, FVector Gdzie, EDzwiek K, float Glosnosc = 1.f);
	/** Bez polozenia (kwestie oddzialow, interfejs). */
	static void Graj2D(const UObject* Kontekst, FName Nazwa, EDzwiek K, float Glosnosc = 1.f);
	/** Petla tla 2D: gra woła co klatke z docelowa glosnoscia 0..1; bez wolania petla plynnie cichnie. */
	static void Petla(const UObject* Kontekst, FName Nazwa, EDzwiek K, float Glosnosc);
	/** 1 = kamera blisko (pojedyncze dzwieki), 0 = najdalej na mapie grywalnej albo poza nia. */
	float Blisko() const { return BliskoN; }
	/** Punkt na ziemi, na ktory patrzy kamera (tam stoi sluchacz). */
	FVector Ucho() const { return Ognisko; }
	/** Czy sa pliki o tej nazwie (np. zeby nie liczyc niczego dla brakujacych dzwiekow). */
	bool Ma(FName Nazwa) const { return Grupy.Contains(Nazwa); }

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type T) const override { return T == EWorldType::Game || T == EWorldType::PIE; }

private:
	USoundBase* Wybierz(FName Nazwa);
	float Gain(EDzwiek K) const;

	// Snd[i] = nullptr, dopoki plik i nie jest wczytany; trzyma go przy zyciu tablica Wczytane (UPROPERTY).
	struct FGrupa { TArray<FSoftObjectPath> Pliki; TArray<USoundBase*> Snd; int32 Ostatni = -1; double Kiedy = -1.0; };
	TMap<FName, FGrupa> Grupy;
	UPROPERTY() TArray<TObjectPtr<USoundBase>> Wczytane;
	UPROPERTY() TObjectPtr<USoundAttenuation> Tlumienie;
	UPROPERTY() TArray<TObjectPtr<USoundConcurrency>> Limity;

	struct FPetla { float Cel = 0.f; float Teraz = 0.f; EDzwiek K = EDzwiek::Osada; };
	TMap<FName, FPetla> Petle;
	UPROPERTY() TMap<FName, TObjectPtr<UAudioComponent>> PetleC;

	FVector Ognisko = FVector::ZeroVector;
	float BliskoN = 0.f;
	bool bSwiat = false; // mapa grywalna: dzwieki swiata slychac
};
