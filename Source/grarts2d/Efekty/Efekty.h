// Plik: Source/grarts2d/Efekty/Efekty.h
// Efekty wojny (23.11.2026, prosba uzytkownika: "dym ogien realistyczny pod gre, budynki niszczone podczas wojny"):
// jeden aktor na swiat prowadzi czastki na procesorze i rysuje je w instancjach (jedno wywolanie na rodzaj):
//   - pozary: jezyki ognia nad dachem (ilosc od powierzchni i sily ognia), klab dymu (ciemny przy silnym ogniu, szary przy
//     tlacych sie zgliszczach), iskry i zar unoszace sie z wiatrem, cieple swiatlo plomieni (lagodnie pulsuje, bez migania);
//   - zawalenie: chmura kurzu przy ziemi, lecace kamienie i zweglone belki (spadaja, odbijaja sie, zostaja chwile), snop iskier;
//   - kurz po uderzeniu (taran, kamien).
// Kwadraty czastek obracane do kamery, dym sortowany od najdalszego (przezroczystosc bez bledow kolejnosci).
// Kolor i jasnosc kazdej czastki w danych instancji (materialy z Tools/ue_efekty.py, modele z Tools/efekty.py).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Efekty.generated.h"

class UInstancedStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class GRARTS2D_API AEfekty : public AActor
{
	GENERATED_BODY()

public:
	AEfekty();
	virtual void Tick(float DeltaSeconds) override;

	/** Efekty swiata (tworzone przy pierwszym uzyciu). */
	static AEfekty* W(UWorld* Swiat);

	/** Pozar budynku: Srodek na ziemi, Polowa = pol obrysu (cm, w ukladzie budynku obroconym o Obrot), Wys = wysokosc dachu.
	 *  Zwraca uchwyt do UstawPozar / ZakonczPozar. */
	int32 Pozar(FVector Srodek, FVector2D Polowa, float Obrot, float Wys);
	/** Sila 0..1 (plomienie, dym, swiatlo); Wys < 0 = bez zmian (po zawaleniu plomienie nisko, nad gruzem). */
	void UstawPozar(int32 Id, float Sila, float Wys = -1.f);
	/** Ogien zgasl: dym slabnie przez chwile i pozar znika. */
	void ZakonczPozar(int32 Id);
	/** Zawalenie budynku: kurz, gruz (kamienny albo drewniany), iskry. */
	void Zawal(FVector Srodek, FVector2D Polowa, float Obrot, float Wys, bool bKamien);
	/** Krotki oblok kurzu (uderzenie). */
	void Kurz(FVector Gdzie, float Promien, int32 Ile);
	/** Ile czastek zyje (testy, log). */
	int32 Czastek() const;
	int32 Pozarow() const { return Pozary.Num(); }

private:
	enum ERodzaj : uint8 { Plomien, Dym, Iskra, KurzR, Gruz, Rodzajow };
	struct FCzastka
	{
		FVector P, V;
		float Wiek = 0.f, Zycie = 1.f, Rozmiar = 100.f, Obrot = 0.f, Spin = 0.f;
		FLinearColor K = FLinearColor::White;
		FVector Skala = FVector::OneVector; // gruz: wydluzone belki
		FQuat Q = FQuat::Identity;          // gruz: obrot w locie
		bool bLezy = false;
	};
	TArray<FCzastka> Cz[Rodzajow];
	struct FPozar
	{
		int32 Id = 0;
		FVector S = FVector::ZeroVector;
		FVector2D Pol = FVector2D(300.f, 300.f);
		float Obrot = 0.f, Wys = 500.f, Sila = 0.f, Cel = 0.f;
		float Akum[3] = { 0.f, 0.f, 0.f }; // ulamki czastek do wypuszczenia: plomien, dym, iskra
		bool bKoniec = false;
		float Faza = 0.f;                  // przesuniecie pulsowania swiatla
	};
	TArray<FPozar> Pozary;
	int32 NastId = 1;
	float Czas = 0.f;

	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Ism;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Swiatla;

	FVector PunktPozaru(const FPozar& F, float ZOd, float ZDo) const;
	/** Punkt na powierzchni budynku: polacie dachu (nizej przy okapie) albo sciany (obwod). Ogien "na budynku", nie nad nim. */
	FVector PunktPowierzchni(const FPozar& F, bool& bSciana) const;
	void Dodaj(ERodzaj R, const FCzastka& C);
	void Symuluj(float Dt, FVector Kamera);
	void Rysuj(FVector Kamera);
	void Oswietl(FVector Kamera);
	static FVector Wiatr();
};
