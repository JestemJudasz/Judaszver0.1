// Plik: Source/grarts2d/Osada/Drogi.cpp
#include "Osada/Drogi.h"
#include "Osada/Osada.h"
#include "Budowy/Budowa.h"
#include "Swiat/SwiatWysokosci.h"
#include "Swiat/TerenKafle.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Algo/Reverse.h"

TWeakObjectPtr<ADrogi> ADrogi::Aktywne;

namespace
{
	/** Wjazd w srodek innej drogi: o ile szersza jest wstega w odleglosci D od konca (lejek do 1,9 szerokosci). */
	float Lejek(float D, float W)
	{
		const float X = FMath::Clamp(1.f - D / (W * 2.4f), 0.f, 1.f);
		return 0.9f * X * X * (3.f - 2.f * X);
	}
}

ADrogi::ADrogi()
{
	PrimaryActorTick.bCanEverTick = false;
	Siatki = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Drogi"));
	RootComponent = Siatki;
	Szkic = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Szkic"));
	Szkic->SetupAttachment(Siatki);
	for (UProceduralMeshComponent* C : { Siatki.Get(), Szkic.Get() })
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->bUseAsyncCooking = true;
	}
	SetCanBeDamaged(false);
}

void ADrogi::BeginPlay()
{
	Super::BeginPlay();
	Aktywne = this;
	Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Swiat/Drogi/M_Droga.M_Droga"));
	if (!Material)
	{
		// Bez Tools/Zbuduj_Drogi.bat: jednolity braz (podstawowy material silnika z parametrem Color).
		UE_LOG(LogTemp, Warning, TEXT("Drogi: brak /Game/Swiat/Drogi/M_Droga (uruchom Tools/Zbuduj_Drogi.bat)"));
		if (UMaterialInterface* Baza = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Baza, this);
			M->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.36f, 0.28f, 0.18f));
			Material = M;
		}
	}
	if (UMaterialInterface* Pod = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Swiat/Budynki/M_Bud_Podglad.M_Bud_Podglad")))
	{
		MaterialSzkicu = UMaterialInstanceDynamic::Create(Pod, this);
	}
}

void ADrogi::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Aktywne.Get() == this)
	{
		Aktywne.Reset();
	}
	Super::EndPlay(Reason);
}

void ADrogi::Ustaw(AOsada* O, ATerenKafle* T)
{
	Osada = O;
	Teren = T;
}

void ADrogi::Wlacz(bool bWlacz)
{
	bRysuje = bWlacz;
	bStart = false;
	Uwaga.Empty();
	Szkic->ClearAllMeshSections();
}

// ---------------------------------------------------------------------------------------------------------------- graf

void ADrogi::WezlyWKole(FVector2D P, float R, TArray<int32>& Out) const
{
	Out.Reset();
	const FIntPoint A = Komorka(P - FVector2D(R)), B = Komorka(P + FVector2D(R));
	const double R2 = double(R) * R;
	for (int32 X = A.X; X <= B.X; ++X)
	{
		for (int32 Y = A.Y; Y <= B.Y; ++Y)
		{
			if (const TArray<int32>* L = Siatka.Find(FIntPoint(X, Y)))
			{
				for (int32 W : *L)
				{
					if (FVector2D::DistSquared(Wezly[W], P) <= R2)
					{
						Out.Add(W);
					}
				}
			}
		}
	}
}

int32 ADrogi::NajblizszyWezel(FVector2D P, float MaxOdl) const
{
	TArray<int32> L;
	WezlyWKole(P, MaxOdl, L);
	int32 Naj = -1;
	double Najl = TNumericLimits<double>::Max();
	for (int32 W : L)
	{
		const double D = FVector2D::DistSquared(Wezly[W], P);
		if (D < Najl)
		{
			Najl = D;
			Naj = W;
		}
	}
	return Naj;
}

bool ADrogi::NajblizszyPunkt(FVector2D P, float MaxOdl, FVector2D& Q, FVector2D& Kier, float& Odl) const
{
	TArray<int32> L;
	WezlyWKole(P, MaxOdl + Probka, L);
	bool bJest = false;
	double Najl = double(MaxOdl) * MaxOdl;
	for (int32 W : L)
	{
		const int32 N = Nast[W];
		if (N < 0)
		{
			continue;
		}
		const FVector2D A = Wezly[W], B = Wezly[N];
		const FVector2D AB = B - A;
		const double L2 = AB.SizeSquared();
		const double T = L2 > 1.0 ? FMath::Clamp(((P - A) | AB) / L2, 0.0, 1.0) : 0.0;
		const FVector2D X = A + AB * T;
		const double D = FVector2D::DistSquared(X, P);
		if (D <= Najl)
		{
			Najl = D;
			Q = X;
			Kier = AB.GetSafeNormal();
			bJest = true;
		}
	}
	Odl = float(FMath::Sqrt(Najl));
	return bJest;
}

void ADrogi::Odbuduj()
{
	Wezly.Reset();
	Nast.Reset();
	OdcinekWezla.Reset();
	Sasiedzi.Reset();
	Siatka.Reset();
	auto Polacz = [this](int32 A, int32 B)
	{
		Sasiedzi[A].AddUnique(B);
		Sasiedzi[B].AddUnique(A);
	};
	for (int32 O = 0; O < Odcinki.Num(); ++O)
	{
		const TArray<FVector2D>& P = Odcinki[O].P;
		for (int32 k = 0; k < P.Num(); ++k)
		{
			const int32 W = Wezly.Add(P[k]);
			Nast.Add(k + 1 < P.Num() ? W + 1 : -1);
			OdcinekWezla.Add(O);
			Sasiedzi.AddDefaulted();
			Siatka.FindOrAdd(Komorka(P[k])).Add(W);
			if (k > 0)
			{
				Polacz(W - 1, W);
			}
		}
	}
	// Skrzyzowania i polaczenia odcinkow: wezly roznych odcinkow blizej niz 2 m.
	TArray<int32> Bliskie;
	for (int32 W = 0; W < Wezly.Num(); ++W)
	{
		WezlyWKole(Wezly[W], 200.f, Bliskie);
		for (int32 B : Bliskie)
		{
			if (B > W && OdcinekWezla[B] != OdcinekWezla[W])
			{
				Polacz(W, B);
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------------ zapytania

bool ADrogi::NaDrodze(FVector2D P, float Zapas) const
{
	if (Wezly.Num() == 0)
	{
		return false;
	}
	FVector2D Q, K;
	float Odl = 0.f;
	return NajblizszyPunkt(P, Polszerokosc + Zapas, Q, K, Odl);
}

bool ADrogi::Przyciagnij(FVector2D P, float Glebokosc, FVector2D& Gdzie, float& ObrotSt) const
{
	if (Wezly.Num() == 0)
	{
		return false;
	}
	FVector2D Q, Kier;
	float Odl = 0.f;
	if (!NajblizszyPunkt(P, Glebokosc + Polszerokosc + 600.f, Q, Kier, Odl))
	{
		return false;
	}
	// Strona drogi, po ktorej jest kursor; front budynku (+Y w ukladzie budynku) patrzy na droge.
	const FVector2D Nn(-Kier.Y, Kier.X);
	const double Strona = ((P - Q) | Nn) >= 0.0 ? 1.0 : -1.0;
	const FVector2D DoDrogi = -Nn * Strona;
	ObrotSt = FMath::RadiansToDegrees(float(FMath::Atan2(-DoDrogi.X, DoDrogi.Y)));
	ObrotSt = FMath::Fmod(ObrotSt + 360.f, 360.f);
	Gdzie = Q + Nn * Strona * (Polszerokosc + Glebokosc + 80.f);
	return true;
}

bool ADrogi::PrzecinaBudynek(const FRodzajBudynku& R, FVector2D Gdzie, float ObrotSt) const
{
	if (Wezly.Num() == 0)
	{
		return false;
	}
	const FBox2D O = R.Obrys();
	const double Rad = FMath::DegreesToRadians(double(ObrotSt));
	const FVector2D OsX(FMath::Cos(Rad), FMath::Sin(Rad)), OsY(-OsX.Y, OsX.X);
	const FVector2D Cl = O.GetCenter();
	const FVector2D C = Gdzie + OsX * Cl.X + OsY * Cl.Y;
	const FVector2D Pol = O.GetExtent() + FVector2D(Polszerokosc * 0.85f);
	TArray<int32> L;
	WezlyWKole(C, float(Pol.Size()) + Probka, L);
	for (int32 W : L)
	{
		const int32 N = Nast[W];
		for (int32 k = 0; k < (N >= 0 ? 4 : 1); ++k)
		{
			const FVector2D Q = N >= 0 ? FMath::Lerp(Wezly[W], Wezly[N], k * 0.25) : Wezly[W];
			const FVector2D D = Q - C;
			if (FMath::Abs(D | OsX) < Pol.X && FMath::Abs(D | OsY) < Pol.Y)
			{
				return true;
			}
		}
	}
	return false;
}

bool ADrogi::Trasa(FVector2D Od, FVector2D Do, TArray<FVector2D>& Out) const
{
	Out.Reset();
	const double D = FVector2D::Distance(Od, Do);
	if (D < 2500.0 || Wezly.Num() == 0)
	{
		return false; // blisko: prosto
	}
	const float Szukaj = float(FMath::Min(D * 0.45, 6000.0));
	const int32 A = NajblizszyWezel(Od, Szukaj), B = NajblizszyWezel(Do, Szukaj);
	if (A < 0 || B < 0 || A == B)
	{
		return false;
	}
	const double DojA = FVector2D::Distance(Od, Wezly[A]), DojB = FVector2D::Distance(Do, Wezly[B]);
	const double Limit = (D - DojA - DojB) * Premia; // dluzsza droga nic nie daje
	if (Limit <= 0.0)
	{
		return false;
	}
	// Dijkstra po wezlach drog.
	const int32 N = Wezly.Num();
	TArray<double> Dist;
	Dist.Init(TNumericLimits<double>::Max(), N);
	TArray<int32> Prev;
	Prev.Init(-1, N);
	struct FE
	{
		double K;
		int32 W;
		bool operator<(const FE& O) const { return K < O.K; }
	};
	TArray<FE> Kopiec;
	Dist[A] = 0.0;
	Kopiec.HeapPush(FE{ 0.0, A });
	while (Kopiec.Num())
	{
		FE E;
		Kopiec.HeapPop(E);
		if (E.K > Dist[E.W])
		{
			continue;
		}
		if (E.W == B || E.K > Limit)
		{
			break;
		}
		for (int32 S : Sasiedzi[E.W])
		{
			const double K = E.K + FVector2D::Distance(Wezly[E.W], Wezly[S]);
			if (K < Dist[S])
			{
				Dist[S] = K;
				Prev[S] = E.W;
				Kopiec.HeapPush(FE{ K, S });
			}
		}
	}
	if (Prev[B] < 0 || DojA + Dist[B] / Premia + DojB >= D * 0.97)
	{
		return false;
	}
	for (int32 W = B; W >= 0; W = Prev[W])
	{
		Out.Add(Wezly[W]);
	}
	Algo::Reverse(Out);
	Out.Add(Do);
	return true;
}

// --------------------------------------------------------------------------------------------------------- rysowanie

void ADrogi::Krzywa(FVector2D A, FVector2D KierA, FVector2D B, FVector2D KierB, TArray<FVector2D>& Out)
{
	Out.Reset();
	const FVector2D AB = B - A;
	const double L = AB.Size();
	if (L < 1.0)
	{
		Out.Add(A);
		return;
	}
	// Kierunek do tylu dalby petle: wtedy na tym koncu ostry zakret (bez warunku).
	const FVector2D U = AB / L;
	if ((KierA | U) < -0.1) { KierA = FVector2D::ZeroVector; }
	if ((KierB | U) < -0.1) { KierB = FVector2D::ZeroVector; }
	const bool bA = !KierA.IsNearlyZero(), bB = !KierB.IsNearlyZero();
	const double H = L * 0.4;
	const FVector2D C1 = bA ? A + KierA * H : A + AB / 3.0;
	const FVector2D C2 = bB ? B - KierB * H : (bA ? (C1 + B) * 0.5 : B - AB / 3.0);
	auto Pt = [&](double T)
	{
		const double S = 1.0 - T;
		return A * (S * S * S) + C1 * (3.0 * S * S * T) + C2 * (3.0 * S * T * T) + B * (T * T * T);
	};
	double Dl = 0.0;
	for (int32 i = 1; i <= 32; ++i) { Dl += FVector2D::Distance(Pt((i - 1) / 32.0), Pt(i / 32.0)); }
	const int32 N = FMath::Max(1, FMath::CeilToInt32(Dl / Probka));
	for (int32 i = 0; i <= N; ++i) { Out.Add(Pt(double(i) / N)); }
}

bool ADrogi::MoznaDroge(const TArray<FVector2D>& P) const
{
	Uwaga.Empty();
	double Dl = 0.0;
	for (int32 i = 1; i < P.Num(); ++i)
	{
		Dl += FVector2D::Distance(P[i - 1], P[i]);
	}
	if (P.Num() < 2 || Dl < 300.0)
	{
		Uwaga = TEXT("za krótka");
		return false;
	}
	float Poprzednia = 0.f;
	for (int32 i = 0; i < P.Num(); ++i)
	{
		const FVector2D Q = P[i];
		float OdRzeki = 1e5f;
		const float H = FSwiat::Wysokosc(Q.X, Q.Y, &OdRzeki);
		if (H <= 0.f || OdRzeki < 300.f || FSwiat::Woda(Q.X, Q.Y) != FSwiat::EWoda::Lad)
		{
			Uwaga = TEXT("woda albo rzeka");
			return false;
		}
		if (i > 0 && FMath::Abs(H - Poprzednia) > 0.45f * FVector2D::Distance(P[i - 1], Q))
		{
			Uwaga = TEXT("za stromo");
			return false;
		}
		Poprzednia = H;
		if (AOsada* O = Osada.Get())
		{
			const FVector2D Kier = (P[FMath::Min(i + 1, P.Num() - 1)] - P[FMath::Max(i - 1, 0)]).GetSafeNormal();
			const FVector2D Bok = FVector2D(-Kier.Y, Kier.X) * (Polszerokosc * 0.8f);
			if (O->BudynekPod(Q) || O->BudynekPod(Q + Bok) || O->BudynekPod(Q - Bok))
			{
				Uwaga = TEXT("budynek na drodze");
				return false;
			}
		}
	}
	return true;
}

int32 ADrogi::PrzyciagnijKoniec(FVector2D P, FVector2D& Gdzie, FVector2D& Kier) const
{
	Gdzie = P;
	Kier = FVector2D::ZeroVector;
	if (Wezly.Num() == 0)
	{
		return 0;
	}
	// Najpierw konce drog (przedluzenie z lukiem), potem dowolny punkt drogi (skrzyzowanie).
	TArray<int32> L;
	WezlyWKole(P, 600.f, L);
	int32 Naj = -1;
	double Najl = TNumericLimits<double>::Max();
	for (int32 W : L)
	{
		const bool bPoczatek = W == 0 || OdcinekWezla[W - 1] != OdcinekWezla[W];
		const bool bKoniec = Nast[W] < 0;
		const double D = FVector2D::DistSquared(Wezly[W], P);
		if ((bPoczatek || bKoniec) && D < Najl)
		{
			Najl = D;
			Naj = W;
		}
	}
	if (Naj >= 0)
	{
		// Wyjscie z konca drogi na zewnatrz (dalej w tym samym kierunku).
		Kier = Nast[Naj] < 0 ? (Wezly[Naj] - Wezly[Naj - 1]).GetSafeNormal() : (Wezly[Naj] - Wezly[Naj + 1]).GetSafeNormal();
		Gdzie = Wezly[Naj];
		return 1;
	}
	float Odl = 0.f;
	return NajblizszyPunkt(P, 400.f, Gdzie, Kier, Odl) ? 2 : 0;
}

bool ADrogi::WchodziWDroge(int32 Ktory, FVector2D E) const
{
	TArray<int32> L;
	WezlyWKole(E, Polszerokosc + Probka, L);
	bool bWjazd = false;
	for (int32 W : L)
	{
		if (OdcinekWezla[W] == Ktory)
		{
			continue;
		}
		const bool bKraniec = Nast[W] < 0 || W == 0 || OdcinekWezla[W - 1] != OdcinekWezla[W];
		if (bKraniec && FVector2D::Distance(Wezly[W], E) < 300.0)
		{
			return false; // koniec w koniec: przedluzenie, nie wjazd
		}
		if (Nast[W] >= 0)
		{
			const FVector2D A = Wezly[W], AB = Wezly[Nast[W]] - A;
			const double L2 = AB.SizeSquared();
			const double T = L2 > 1.0 ? FMath::Clamp(((E - A) | AB) / L2, 0.0, 1.0) : 0.0;
			bWjazd |= FVector2D::Distance(A + AB * T, E) < Polszerokosc * 0.5f;
		}
	}
	return bWjazd;
}

int32 ADrogi::OdcinekPod(FVector2D P) const
{
	FVector2D Q, K;
	float Odl = 0.f;
	if (!NajblizszyPunkt(P, Polszerokosc * 1.5f, Q, K, Odl))
	{
		return -1;
	}
	// Wezel najblizej punktu Q nalezy do szukanego odcinka.
	TArray<int32> L;
	WezlyWKole(Q, Probka + 10.f, L);
	int32 Ktory = -1;
	double Najl = TNumericLimits<double>::Max();
	for (int32 W : L)
	{
		const int32 N = Nast[W];
		if (N < 0)
		{
			continue;
		}
		const FVector2D A = Wezly[W], AB = Wezly[N] - A;
		const double L2 = AB.SizeSquared();
		const double T = L2 > 1.0 ? FMath::Clamp(((Q - A) | AB) / L2, 0.0, 1.0) : 0.0;
		const double D = FVector2D::DistSquared(A + AB * T, Q);
		if (D < Najl)
		{
			Najl = D;
			Ktory = OdcinekWezla[W];
		}
	}
	return Ktory;
}

void ADrogi::DodajOdcinek(const TArray<FVector2D>& P)
{
	if (P.Num() < 2)
	{
		return;
	}
	if (Odcinki.Num() == 0)
	{
		SetActorLocation(FVector(P[0], FSwiat::Wysokosc(P[0].X, P[0].Y)));
	}
	Odcinki.AddDefaulted_GetRef().P = P;
	if (ATerenKafle* T = Teren.Get())
	{
		for (int32 i = 0; i < P.Num(); i += 2)
		{
			T->Wykarczuj(P[i], Polszerokosc + 80.f);
		}
		T->Wykarczuj(P.Last(), Polszerokosc + 80.f);
	}
	Odbuduj();
	BudujSiatke();
}

void ADrogi::UsunOdcinek(int32 Ktory)
{
	if (!Odcinki.IsValidIndex(Ktory))
	{
		return;
	}
	Odcinki.RemoveAt(Ktory);
	Odbuduj();
	BudujSiatke();
}

void ADrogi::DodajDroge(const TArray<FVector2D>& Punkty)
{
	TArray<FVector2D> P;
	for (int32 i = 0; i + 1 < Punkty.Num(); ++i)
	{
		const int32 N = FMath::Max(1, FMath::CeilToInt32(FVector2D::Distance(Punkty[i], Punkty[i + 1]) / Probka));
		for (int32 k = 0; k < N; ++k)
		{
			P.Add(FMath::Lerp(Punkty[i], Punkty[i + 1], double(k) / N));
		}
	}
	if (Punkty.Num())
	{
		P.Add(Punkty.Last());
	}
	DodajOdcinek(P);
}

void ADrogi::Rysuj(APlayerController* PC, bool bJest, FVector2D P, bool bKlik)
{
	Szkic->ClearAllMeshSections();
	if (PC->WasInputKeyJustPressed(EKeys::Escape))
	{
		Wlacz(false);
		return;
	}
	if (PC->WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		if (bStart)
		{
			bStart = false; // koniec ciagu, nastepny klik zaczyna nowa droge
		}
		else
		{
			Wlacz(false);
		}
		return;
	}
	if (!bJest)
	{
		return;
	}
	if (Odcinki.Num() == 0)
	{
		SetActorLocation(FVector(P, FSwiat::Wysokosc(P.X, P.Y))); // punkt odniesienia siatki (dokladnosc float)
	}
	auto Pokaz = [this](const TArray<FVector2D>& Pkt, bool bDobry)
	{
		Wstega(Szkic, 0, Pkt, true, 24.f);
		if (MaterialSzkicu)
		{
			MaterialSzkicu->SetVectorParameterValue(TEXT("Kolor"), bDobry ? FLinearColor(0.2f, 0.9f, 0.3f) : FLinearColor(1.f, 0.15f, 0.1f));
			Szkic->SetMaterial(0, MaterialSzkicu);
		}
	};
	// Ctrl: usuwanie odcinka pod kursorem (podswietlony na czerwono).
	if (PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl))
	{
		bStart = false;
		const int32 Ktory = OdcinekPod(P);
		if (Ktory >= 0)
		{
			Pokaz(Odcinki[Ktory].P, false);
			if (bKlik)
			{
				UsunOdcinek(Ktory);
				Szkic->ClearAllMeshSections();
			}
		}
		return;
	}
	FVector2D Koniec, KierKonca;
	int32 Tryb = PrzyciagnijKoniec(P, Koniec, KierKonca);
	if (bStart && Tryb != 0 && FVector2D::Distance(Koniec, Start) < 1.0)
	{
		Tryb = 0; // nie przyciagaj do wlasnego poczatku
		Koniec = P;
	}
	if (!bStart)
	{
		const TArray<FVector2D> Znacznik = { Koniec - FVector2D(30.f, 0.f), Koniec + FVector2D(30.f, 0.f) };
		Pokaz(Znacznik, true);
		if (bKlik)
		{
			bStart = true;
			Start = Koniec;
			StartKier = KierKonca;
			StartTryb = Tryb;
		}
		return;
	}
	TArray<FVector2D> Pkt;
	const bool bProsto = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
	FVector2D Ka = FVector2D::ZeroVector, Kb = FVector2D::ZeroVector;
	if (!bProsto)
	{
		// Wyjscie: dalej w kierunku drogi albo prostopadle ze srodka drogi. Dojscie: stycznie w koniec drogi, prostopadle w srodek.
		auto Prostopadle = [](FVector2D Os, FVector2D Do) { const FVector2D N(-Os.Y, Os.X); return (Do | N) >= 0.0 ? N : -N; };
		Ka = StartTryb == 2 ? Prostopadle(StartKier, Koniec - Start) : StartKier;
		Kb = Tryb == 1 ? -KierKonca : (Tryb == 2 ? -Prostopadle(KierKonca, Start - Koniec) : FVector2D::ZeroVector);
	}
	Krzywa(Start, Ka, Koniec, Kb, Pkt);
	bSzkicDobry = MoznaDroge(Pkt);
	if (Pkt.Num() >= 2)
	{
		Pokaz(Pkt, bSzkicDobry);
	}
	if (bKlik && bSzkicDobry)
	{
		DodajOdcinek(Pkt);
		Start = Pkt.Last();
		StartKier = (Pkt.Last() - Pkt[Pkt.Num() - 2]).GetSafeNormal();
		StartTryb = 1;
		bStart = Tryb == 0; // wejscie w inna droge konczy ciag
		Szkic->ClearAllMeshSections();
	}
}

FString ADrogi::Pomoc() const
{
	if (!bStart)
	{
		return TEXT("Drogi (darmowe): kliknij początek drogi (przyciąga się do końca drogi obok)\nCtrl + klik - usuń odcinek     prawy klik / Esc / G - wyjdź");
	}
	FString T = TEXT("Drogi: klik - następny punkt (łuk wygina się sam), klik na innej drodze - połącz i zakończ, Shift - prosto\nprawy klik - koniec drogi     Ctrl + klik - usuń odcinek     Esc - wyjdź");
	if (!bSzkicDobry && !Uwaga.IsEmpty())
	{
		T += TEXT("\nNie można: ") + Uwaga;
	}
	return T;
}

// ------------------------------------------------------------------------------------------------------------ wyglad

void ADrogi::Wstega(UProceduralMeshComponent* C, int32 Sekcja, const TArray<FVector2D>& P0, bool bKonce, float Nad, bool bWjazdA, bool bWjazdB) const
{
	if (P0.Num() < 2)
	{
		return;
	}
	// Gesciej niz punkty grafu (srodki odcinkow z Catmulla-Roma): gladsze luki i lejek wjazdu.
	TArray<FVector2D> P;
	for (int32 i = 0; i + 1 < P0.Num(); ++i)
	{
		const FVector2D A = P0[FMath::Max(i - 1, 0)], B = P0[i], Cc = P0[i + 1], D = P0[FMath::Min(i + 2, P0.Num() - 1)];
		P.Add(B);
		P.Add((B + Cc) * 0.5625 - (A + D) * 0.0625);
	}
	P.Add(P0.Last());
	const int32 N = P.Num();
	TArray<float> Odl; // od poczatku wzdluz drogi
	Odl.Add(0.f);
	for (int32 i = 1; i < N; ++i) { Odl.Add(Odl.Last() + float(FVector2D::Distance(P[i - 1], P[i]))); }
	const FVector Baza = GetActorLocation();
	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> Nrm;
	TArray<FVector2D> UV;
	TArray<FLinearColor> Kol;
	TArray<FProcMeshTangent> Tg;
	// Wstega szersza od drogi: brzeg ma alfa 0, maska materialu tnie go w polowie z szumem (poszarpany pikselowy skraj).
	const float W = Polszerokosc * 1.3f;
	// Ubita brazowa ziemia (zrzut 22.11.2026: jasniejsze wychodzily jak piasek): dwie ciemne koleiny (1,6 m rozstawu),
	// miedzy nimi zielonkawy pas trawy, brzeg wpada w zielen.
	const FLinearColor Brzeg(0.30f, 0.27f, 0.15f), Pobocze(0.35f, 0.27f, 0.17f), Koleina(0.21f, 0.15f, 0.09f), Srodek(0.30f, 0.29f, 0.155f);
	constexpr int32 K = 7;
	const float Off[K] = { -1.f, -0.62f, -0.38f, 0.f, 0.38f, 0.62f, 1.f };
	const float Alfa[K] = { 0.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.f };
	const FLinearColor Kolory[K] = { Brzeg, Pobocze, Koleina, Srodek, Koleina, Pobocze, Brzeg };
	auto Punkt = [&](FVector2D Q, float A, FLinearColor K, float Dz = 0.f)
	{
		const float H = FSwiat::Wysokosc(Q.X, Q.Y);
		const float Hx = FSwiat::Wysokosc(Q.X + 50.0, Q.Y) - FSwiat::Wysokosc(Q.X - 50.0, Q.Y);
		const float Hy = FSwiat::Wysokosc(Q.X, Q.Y + 50.0) - FSwiat::Wysokosc(Q.X, Q.Y - 50.0);
		V.Add(FVector(Q.X - Baza.X, Q.Y - Baza.Y, H + Nad + Dz - Baza.Z));
		Nrm.Add(FVector(-Hx / 100.f, -Hy / 100.f, 1.f).GetSafeNormal());
		UV.Add(FVector2D((Q.X - Baza.X) / 400.0, (Q.Y - Baza.Y) / 400.0));
		Kol.Add(FLinearColor(K.R, K.G, K.B, A));
		Tg.Add(FProcMeshTangent(1.f, 0.f, 0.f));
	};
	// Wjazd w srodek innej drogi: wstega rozszerza sie lejkiem (do 1,9 szerokosci), zamiast konczyc sie kolem.
	for (int32 i = 0; i < N; ++i)
	{
		const FVector2D D = (P[FMath::Min(i + 1, N - 1)] - P[FMath::Max(i - 1, 0)]).GetSafeNormal();
		const FVector2D Nn(-D.Y, D.X);
		const float Wi = W * (1.f + (bWjazdA ? Lejek(Odl[i], W) : 0.f) + (bWjazdB ? Lejek(Odl.Last() - Odl[i], W) : 0.f));
		for (int32 j = 0; j < K; ++j)
		{
			Punkt(P[i] + Nn * (Wi * Off[j]), Alfa[j], Kolory[j]);
		}
	}
	for (int32 i = 0; i + 1 < N; ++i)
	{
		for (int32 j = 0; j + 1 < K; ++j)
		{
			const int32 A = i * K + j, B = A + 1, Cc = A + K, D = Cc + 1;
			T.Append({ A, B, Cc, B, D, Cc }); // zgodnie z ruchem wskazowek patrzac z gory (przod sciany w UE)
		}
	}
	if (bKonce)
	{
		// Okragle konce: skrzyzowania i zakrety bez szpar.
		constexpr int32 Ile = 16;
		for (const FVector2D& E : { P[0], P.Last() })
		{
			const int32 S = V.Num();
			// kolo troche nizej niz wstega, zeby nie migaly na sobie
			Punkt(E, 1.f, Pobocze, -0.7f);
			for (int32 k = 0; k < Ile; ++k)
			{
				const double Kat = 2.0 * PI * k / Ile;
				const FVector2D R(FMath::Cos(Kat), FMath::Sin(Kat));
				Punkt(E + R * (W * 0.62f), 1.f, Pobocze, -0.7f);
				Punkt(E + R * W, 0.f, Brzeg, -0.7f);
			}
			for (int32 k = 0; k < Ile; ++k)
			{
				const int32 A1 = S + 1 + 2 * k, A2 = A1 + 1;
				const int32 B1 = S + 1 + 2 * ((k + 1) % Ile), B2 = B1 + 1;
				T.Append({ S, B1, A1, A1, B1, A2, B1, B2, A2 });
			}
		}
	}
	C->CreateMeshSection_LinearColor(Sekcja, V, T, Nrm, UV, Kol, Tg, false);
}

void ADrogi::BudujSiatke()
{
	Siatki->ClearAllMeshSections();
	for (int32 i = 0; i < Odcinki.Num(); ++i)
	{
		// kazdy odcinek troche na innej wysokosci: nachodzace na siebie (skrzyzowania, laczenia) nie migaja
		const TArray<FVector2D>& P = Odcinki[i].P;
		Wstega(Siatki, i, P, true, 14.f + float(i % 6) * 1.5f, WchodziWDroge(i, P[0]), WchodziWDroge(i, P.Last()));
		if (Material)
		{
			Siatki->SetMaterial(i, Material);
		}
	}
	PosadzKepki();
}

float ADrogi::OdInnejDrogi(int32 Ktory, FVector2D Q, float Max) const
{
	TArray<int32> L;
	WezlyWKole(Q, Max + Probka, L);
	double Naj = Max;
	for (int32 W : L)
	{
		if (OdcinekWezla[W] == Ktory || Nast[W] < 0)
		{
			continue;
		}
		const FVector2D A = Wezly[W], AB = Wezly[Nast[W]] - A;
		const double L2 = AB.SizeSquared();
		const double T = L2 > 1.0 ? FMath::Clamp(((Q - A) | AB) / L2, 0.0, 1.0) : 0.0;
		Naj = FMath::Min(Naj, FVector2D::Distance(A + AB * T, Q));
	}
	return float(Naj);
}

void ADrogi::PosadzKepki()
{
	if (Kepki.Num() == 0)
	{
		for (const TCHAR* Nazwa : { TEXT("SM_Trawa_1"), TEXT("SM_Trawa_2"), TEXT("SM_Kamien_1") })
		{
			UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(this);
			C->SetupAttachment(Siatki);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false); // jak trawa terenu: drobiazgi bez cieni (pamiec VSM)
			C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Swiat/Natura/%s.%s"), Nazwa, Nazwa)));
			C->WorldPositionOffsetDisableDistance = 60000;
			C->SetCullDistances(0, 250000);
			C->RegisterComponent();
			Kepki.Add(C);
		}
	}
	TArray<TArray<FTransform>> T;
	T.SetNum(Kepki.Num());
	const FVector Baza = GetActorLocation();
	const float W = Polszerokosc * 1.3f;
	AOsada* O = Osada.Get();
	auto Dodaj = [&](int32 Odc, FVector2D X, int32 Model, float Skala, float Obrot)
	{
		if (OdInnejDrogi(Odc, X, Polszerokosc * 1.05f) < Polszerokosc * 1.05f || (O && O->BudynekPod(X)))
		{
			return; // na innej drodze (skrzyzowanie) albo pod budynkiem
		}
		const float Zanurz = (Model == 2 ? 10.f : 5.f) * Skala;
		T[Model].Add(FTransform(FRotator(0.f, Obrot, 0.f), FVector(X.X - Baza.X, X.Y - Baza.Y, FSwiat::Wysokosc(X.X, X.Y) - Zanurz - Baza.Z), FVector(Skala)));
	};
	for (int32 i = 0; i < Odcinki.Num(); ++i)
	{
		const TArray<FVector2D>& P = Odcinki[i].P;
		const bool bA = WchodziWDroge(i, P[0]), bB = WchodziWDroge(i, P.Last());
		double Dl = 0.0;
		for (int32 k = 1; k < P.Num(); ++k) { Dl += FVector2D::Distance(P[k - 1], P[k]); }
		double S = 0.0;
		for (int32 k = 0; k + 1 < P.Num(); ++k)
		{
			const FVector2D D = P[k + 1] - P[k];
			const double Lk = D.Size();
			if (Lk < 1.0)
			{
				continue;
			}
			const FVector2D U = D / Lk, Nn(-U.Y, U.X);
			for (double t = 0.0; t < Lk; t += 60.0)
			{
				const FVector2D Q = P[k] + U * t;
				const float Od = float(S + t), Do = float(Dl - S - t);
				const float Wi = W * (1.f + (bA ? Lejek(Od, W) : 0.f) + (bB ? Lejek(Do, W) : 0.f));
				// Los z polozenia punktu: te same kepki po kazdej przebudowie drog.
				uint32 Z = HashCombine(GetTypeHash(FMath::RoundToInt32(Q.X / 20.0)), GetTypeHash(FMath::RoundToInt32(Q.Y / 20.0)));
				auto Los = [&Z]() { Z = Z * 1664525u + 1013904223u; return float(Z >> 8) / 16777216.f; };
				for (const float Strona : { -1.f, 1.f })
				{
					const float Jest = Los(), Gdzie = Los(), Wzdluz = Los(), Jaki = Los(), Wielkosc = Los(), Obrot = Los();
					if (Jest > 0.6f)
					{
						continue;
					}
					const FVector2D X = Q + Nn * (Strona * Wi * (0.66f + 0.3f * Gdzie)) + U * ((Wzdluz - 0.5f) * 50.f);
					if (Jaki < 0.06f)
					{
						Dodaj(i, X, 2, 0.25f + 0.2f * Wielkosc, Obrot * 360.f);
					}
					else
					{
						Dodaj(i, X, Jaki < 0.53f ? 0 : 1, 0.35f + 0.3f * Wielkosc, Obrot * 360.f);
					}
				}
				// rzadkie niskie kepki w pasie trawy miedzy koleinami (nie przy koncach: skrzyzowania)
				const float Jest = Los(), Bok = Los(), Wielkosc = Los(), Obrot = Los();
				if (Jest < 0.1f && Od > 300.f && Do > 300.f)
				{
					Dodaj(i, Q + Nn * ((Bok - 0.5f) * 40.f), Jest < 0.05f ? 0 : 1, 0.22f + 0.1f * Wielkosc, Obrot * 360.f);
				}
			}
			S += Lk;
		}
	}
	for (int32 m = 0; m < Kepki.Num(); ++m)
	{
		Kepki[m]->ClearInstances();
		if (Kepki[m]->GetStaticMesh() && T[m].Num())
		{
			Kepki[m]->AddInstances(T[m], false, false, false);
		}
	}
}
