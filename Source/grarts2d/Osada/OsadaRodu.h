// Plik: Source/grarts2d/Osada/OsadaRodu.h
// Faza AI4, czesc widoczna (23.11.2026, prosba uzytkownika: "dawaj widocznych"): osada rodu AI w terenie, w miejscu
// FRod::Gdzie. Pokazuje gotowe budynki rodu (te same modele co u gracza, ostatnia faza) na siatce wokol srodka i
// choragiew w barwie rodu. Ma tyle budynkow, ile rod ma w liczbach (ARody), odswiezana co pore roku.
// 23.11.2026: uklad pierscieniami wokol placu (kosciol, targ, magazyny przy placu, potem domy i warsztaty), mury rodu
// (palisada albo mur kamienny z wiezami i brama) za zabudowa, gospodarstwo i las za murami.
// 23.11.2026 (prosba: "w wioskach AI nic sie nie dzieje"): pola z uprawa wedlug pory roku, plac budowy z rusztowaniem i
// ludziki przy kamerze (do 450 m): pracownicy chodza z domu do pracy i pracuja narzedziem (drwal siekiera, kamieniarz
// kilof, rolnik motyka/sierp...), budowniczowie stukaja przy budowie, woje cwicza z wloczniami na placu.
// shortcut: ludziki tylko udaja prace (towar dalej liczony liczbami w ARody), bez noszenia towaru miedzy budynkami.
// 23.11.2026 (podboj): bramy z wytrzymaloscia (ABitwa::Wyprawa rozbija je toporami), mur nie do przejscia poza rozbita
// brama, spalenie zdobytej osady (pozary w AEfekty, modele _Z1/_Z2 jak u gracza).
// shortcut: spalone budynki dalej liczone w R.Budynkow (rod traci rodziny i zapasy), zgliszcza znikaja po 2 na pore roku.
// 23.11.2026 (uwaga: "robia kolko slabe, teren dziwnie wyglada, mury slabe"): uklad wsi jak sredniowieczna ulicowka -
// plac z kosciolem, magazynem i targiem, ulica glowna przez plac do dwoch bram, boczne uliczki; domy frontem do ulicy
// z ogrodkiem i plotem za domem; mur (palisada na wale / mur kamienny) po obrysie zabudowy z basztami w rogach, schodami
// i bramami na ulicy glownej; pola i obejscia przy drodze za bramami. Ubita ziemia (placu, ulic, podworzy) jedna siatka
// na terenie (M_Droga). Dzialki zapamietane: nowy budynek nie przestawia starych. Modele w instancjach (mniej wywolan).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Osada/Kalendarz.h"
#include "OsadaRodu.generated.h"

struct FRod;
class UStaticMesh;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class ALudzik;


UCLASS()
class GRARTS2D_API AOsadaRodu : public AActor
{
	GENERATED_BODY()

public:
	AOsadaRodu();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Powod) override;
	/** Stawia budynki wedlug liczb rodu (tylko gdy cos sie zmienilo). */
	void Odswiez(const FRod& R, EPora Pora);

	/** Miejsce budynku: zostaje przy kolejnych odswiezeniach (Typ = indeks w DoBudowania, Nr = ktory z kolei). */
	struct FDzialka { int32 Typ = -1, Nr = 0; FVector2D P = FVector2D::ZeroVector, Przod = FVector2D(0.0, 1.0); bool bOgrod = false, bZewn = false; };
	const TArray<FDzialka>& ListaDzialek() const { return Dzialki; }
	/** Do testow (grarts2d.AI.UkladOsady): ile budynkow chcianych i postawionych, ile par dzialek nachodzi na siebie,
	 *  ile dzialek w srodku wystaje poza obrys muru, bramy, baszty, odcinki muru, rysowane ulice. */
	struct FSprawdzenie { int32 Chcianych = 0, Budynkow = 0, Nachodzi = 0, PozaMurem = 0, Bram = 0, Baszt = 0, Odcinkow = 0, Ulic = 0; };
	FSprawdzenie Sprawdz() const;
	/** Do testow: plan osady w pliku tekstowym (ulice, obrys muru, dzialki) dla Tools/plan_osady.py. */
	void ZapiszPlan(const FString& Plik) const;

	// --- Podboj (23.11.2026, prosba: "mechanika zeby podbic te miasta"): brama do rozbicia, pozary zdobytej osady ---
	/** Brama w murze: Na = na zewnatrz osady. Punkty: palisada 600, mur kamienny 1400. */
	struct FBrama { FVector2D P = FVector2D::ZeroVector, Na = FVector2D(1.0, 0.0); float Punkty = 1.f, Max = 1.f; };
	const TArray<FBrama>& Bramy() const { return BramyO; }
	/** Cios w brame k. Ponizej 60% model uszkodzony, przy zerze rozbita (kurz, gruz). true = wlasnie padla. */
	bool UderzBrame(int32 k, float Ile);
	bool BramaLezy(int32 k) const { return BramyO.IsValidIndex(k) && BramyO[k].Punkty <= 0.f; }
	/** W obrysie murow (tylko osada z murem i brama). */
	bool WMurach(FVector2D P) const;
	/** Przy rozbitej bramie: tedy mozna przejsc przez mur. */
	bool PrzyRozbitejBramie(FVector2D P, float Promien = 700.f) const;
	/** Spalenie zdobytej osady: Ile budynkow w murach plonie (po ~12 s gry uszkodzone, po ~35 s zawalone, po ~80 s gasna).
	 *  Zgliszcza zostaja; rod odbudowuje dwa budynki na pore roku. */
	void Spal(int32 Ile);
	/** Najblizszy budynek do podpalenia (w murach, caly, jeszcze nie plonie) najdalej Promien od P; -1 = brak.
	 *  Rozkaz "podpal ten budynek" i woje palacy zdobyta osade (ABitwa). */
	int32 DoSpalenia(FVector2D P, float Promien) const;
	/** Srodek dzialki i pol obrysu (do podejscia woja). */
	FVector2D MiejsceDzialki(int32 i, float& Promien) const;
	/** Woj podklada ogien: budynek zajmuje sie od razu. false = juz plonie albo zgliszcza. */
	bool Podpal(int32 i, float Opoznienie = 0.f);
	int32 Plonacych() const { return Pozary.Num(); }
	int32 Zniszczonych() const { return Zniszczone.Num(); }
	/** Oblezenie: mieszkancy chowaja sie (bez ludzikow przy pracy), bramy nie sa naprawiane. */
	void UstawOblezenie(bool b) { bOblezona = b; }

private:
	TArray<FBrama> BramyO;
	bool bMuryKamienne = false;
	bool bOblezona = false;
	TMap<FIntPoint, int32> Zniszczone; // (Typ, Nr) dzialki: 1 uszkodzona, 2 zgliszcza
	struct FPozar { FIntPoint K; int32 Id = -1; float Czas = 0.f; FVector S; FVector2D Pol; float Obrot = 0.f, Wys = 400.f; };
	TArray<FPozar> Pozary;
	TSharedPtr<FRod> OstatniRod; // do przebudowy po zmianie bramy albo pozarze
	/** Ten sam rod jeszcze raz (inne modele bram i spalonych budynkow), bez przebudowy ubitej ziemi. */
	void Przebuduj();
	void PozaryTick(float DtG);

	/** Modele osady w instancjach: jeden komponent na model (budynki, mury, ploty, ogrodki). */
	UPROPERTY() TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> Instancje;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Choragiew;
	/** Ubita ziemia placu, ulic i podworzy (budowana, gdy kamera jest blisko). */
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Ziemia;

	TArray<FDzialka> Dzialki;
	TArray<FVector2D> ObrysMuru; // otoczka zabudowy (przeciwnie do zegara)
	int32 Chcianych = 0, Bram = 0, Baszt = 0, Odcinkow = 0;
	/** Ubita ziemia: ulice (punkty, polowa szerokosci), podworza (srodek, front, polowa budynku), os placu. */
	TArray<TArray<FVector2D>> UliceZiemi;
	TArray<float> UliceSzer;
	struct FPodworze { FVector2D P, Przod, Pol; };
	TArray<FPodworze> Podworza;
	FVector2D OsPlacu = FVector2D(1.0, 0.0);
	bool bZiemiaDoZbudowania = false;
	TMap<FIntPoint, float> Wysokosci; // siatka ziemi co KrokZiemi cm (teren sie nie zmienia)
	TSet<FIntVector> Karczowane;      // polany juz zgloszone do ATerenKafle (bez powtorek przy odswiezaniu)
	void ZbudujZiemie();
	float WysokoscZiemi(int32 I, int32 J);
	void Karczuj(class ATerenKafle* Teren, FVector2D P, float Promien);
	TArray<int32> Ostatnio; // liczby budynkow z ostatniego odswiezenia
	int32 OstatnieMury = -1;
	int32 OstatnioBuduje = -100;
	int32 OstatniaPora = -1;

	// Ludziki przy kamerze.
	struct FMiejsce { FVector2D P; const struct FRodzajBudynku* B; bool bBudowa; FVector2D Przod; };
	TArray<FMiejsce> Miejsca;   // postawione budynki (do celow ludzikow)
	FVector2D Srodek = FVector2D::ZeroVector;
	FLinearColor Barwa = FLinearColor::White;
	int32 Rodzin = 0, Woje = 0;
	EPora PoraTeraz = EPora::Wiosna;
	enum class EFaza : uint8 { DoPracy, Pracuje, DoDomu, Odpoczywa };
	struct FMieszkaniec { int32 Praca = -1; FVector2D Cel, Dom; EFaza Faza = EFaza::DoDomu; float Czas = 0.f, DoCiosu = 0.f; bool bWoj = false; };
	UPROPERTY() TArray<TObjectPtr<ALudzik>> Ludzie;
	TArray<FMieszkaniec> Stan;
	float DoSprawdzenia = 0.f;
	void Zaludnij();
	void Wyludnij();
	void NowyCel(int32 k);
};
