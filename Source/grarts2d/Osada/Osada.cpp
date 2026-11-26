// Plik: Source/grarts2d/Osada/Osada.cpp
#include "Osada/Osada.h"
#include "Osada/Rody.h"
#include "Osada/Drogi.h"
#include "Budowy/Budowa.h"
#include "Dzwiek/Dzwieki.h"
#include "Efekty/Efekty.h"
#include "Ludzie/Ludzik.h"
#include "Ludzie/Wol.h"
#include "Swiat/SwiatWysokosci.h"
#include "Swiat/TerenKafle.h"
#include "Engine/World.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"

namespace
{
	// Stonowane kolory (sRGB), jak na zrzutach wzorcowych: tuniki mezczyzn, suknie kobiet, wlosy, chusty.
	const FColor Tuniki[] = { FColor(120, 70, 50), FColor(90, 105, 70), FColor(150, 140, 110), FColor(80, 90, 115), FColor(160, 120, 60), FColor(110, 95, 80) };
	const FColor Suknie[] = { FColor(122, 60, 50), FColor(78, 88, 118), FColor(108, 80, 60), FColor(96, 104, 78), FColor(138, 118, 90), FColor(92, 70, 86) };
	const FColor Wlosy[] = { FColor(92, 64, 40), FColor(50, 40, 30), FColor(150, 110, 60), FColor(40, 35, 30), FColor(120, 90, 60) };
	const FColor Chusty[] = { FColor(224, 214, 192), FColor(206, 196, 166), FColor(178, 176, 166), FColor(196, 176, 146), FColor(214, 206, 200) };

	template <int32 N>
	FLinearColor Losuj(const FColor (&T)[N]) { return FLinearColor(T[FMath::RandHelper(N)]); }

	// Rodziny nazywane od imienia ojca (staropolskie imiona, dopelniacz): "Rodzina Mszczuja".
	const TCHAR* Ojcowie[] = { TEXT("Mszczuja"), TEXT("Bogdana"), TEXT("Wojsława"), TEXT("Dobiesława"), TEXT("Radosza"),
		TEXT("Sulisława"), TEXT("Przybysława"), TEXT("Jaksy"), TEXT("Zbyluta"), TEXT("Świętosława"), TEXT("Gniewomira"), TEXT("Bolesta") };

	ERolaLudzika RolaW(const FRodzajBudynku& R)
	{
		switch (R.Funkcja)
		{
		case EFunkcja::Drwal: return ERolaLudzika::Drwal;
		case EFunkcja::Borowy: return ERolaLudzika::Borowy;
		case EFunkcja::Kamieniolom: return ERolaLudzika::Kamieniarz;
		case EFunkcja::Zbieracz: return ERolaLudzika::Zbieracz;
		case EFunkcja::Mysliwy: return ERolaLudzika::Mysliwy;
		case EFunkcja::Pole: return ERolaLudzika::Rolnik;
		case EFunkcja::Przetworca: return ERolaLudzika::Rzemieslnik;
		case EFunkcja::Owczarnia: return ERolaLudzika::Pasterz;
		case EFunkcja::Zbrojownia: return ERolaLudzika::Woj;
		case EFunkcja::Rybak: return ERolaLudzika::Rybak;
		default: return ERolaLudzika::Pilarz;
		}
	}

	/** Kolor pory roku (mnozniki do post processu obozu). */
	struct FKolorPory { float Nasycenie; float Temperatura; FVector Wzmocnienie; float Ekspozycja; };
	const FKolorPory KoloryPor[] = {
		{ 1.00f, 6500.f, FVector(1.00f, 1.00f, 1.00f), 0.00f },  // wiosna: jak dotad
		{ 1.06f, 6750.f, FVector(1.03f, 1.02f, 0.97f), 0.05f },  // lato: cieplej, jasniej
		{ 0.96f, 7400.f, FVector(1.06f, 1.00f, 0.86f), 0.00f },  // jesien: zloto
		{ 0.50f, 5700.f, FVector(1.06f, 1.10f, 1.18f), 0.25f },  // zima: chlodno, szaro, jasny snieg
	};
}

AOsada::AOsada()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AOsada::MiejscaObozu(FVector2D Srodek, TArray<FVector>& Out)
{
	Out.Reset();
	const float R = 650.f;
	for (int32 k = 0; k < 5; ++k)
	{
		// Wejscie namiotu (lokalne -X) zwrocone do ogniska: obrot = kat polozenia na okregu.
		const float Kat = 90.f + k * 72.f;
		const FVector2D P = Srodek + FVector2D(FMath::Cos(FMath::DegreesToRadians(Kat)), FMath::Sin(FMath::DegreesToRadians(Kat))) * R;
		Out.Add(FVector(P, Kat));
	}
	Out.Add(FVector(Srodek, 0.f)); // ognisko
}

ABudowa* AOsada::Spawnuj(const FRodzajBudynku& R, FVector2D Gdzie, float Obrot, float Z)
{
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABudowa* B = GetWorld()->SpawnActor<ABudowa>(FVector(Gdzie, Z), FRotator(0.f, Obrot, 0.f), P);
	if (!B)
	{
		return nullptr;
	}
	B->Ustaw(R, false);
	B->SetActorHiddenInGame(!bWidoczne);
	Budynki.Add(B);
	return B;
}

ALudzik* AOsada::NowyLudzik(FVector2D Gdzie, bool bKobieta)
{
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALudzik* L = GetWorld()->SpawnActor<ALudzik>(FVector(Gdzie, FSwiat::Wysokosc(Gdzie.X, Gdzie.Y)), FRotator(0.f, FMath::FRand() * 360.f, 0.f), P);
	if (!L)
	{
		return nullptr;
	}
	L->UstawPlec(bKobieta);
	L->UstawKolory(bKobieta ? Losuj(Suknie) : Losuj(Tuniki), bKobieta ? Losuj(Chusty) : Losuj(Wlosy));
	L->UstawOsade(this);
	L->Przydziel(nullptr, ERolaLudzika::Budowniczy, false);
	L->SetActorHiddenInGame(!bWidoczne);
	L->SetActorTickEnabled(bWidoczne);
	Ludzie.Add(L);
	return L;
}

FVector2D AOsada::WejscieDo(const ABudowa* B) const
{
	return FVector2D(B->GetActorTransform().TransformPosition(FVector(B->Rodzaj().Wejscie, 0.0)));
}

void AOsada::ZalozOboz(FVector2D Srodek, const TArray<float>& Z)
{
	TArray<FVector> Miejsca;
	MiejscaObozu(Srodek, Miejsca);
	check(Z.Num() == Miejsca.Num());
	SrodekObozu = Srodek;
	bOboz = true;
	if (Teren.IsValid())
	{
		Teren->Wykarczuj(Srodek, PromienObozu);
	}
	if (ABudowa* Og = Spawnuj(FRodzajBudynku::Ognisko(), Srodek, 0.f, Z.Last()))
	{
		Og->ZmienZapas(ESurowiec::Kloda, KlodNaStart); // zapas przywieziony przez osadnikow
		Og->ZmienZapas(ESurowiec::Jedzenie, JedzeniaNaStart);
	}
	for (int32 k = 0; k < 5; ++k)
	{
		ABudowa* N = Spawnuj(FRodzajBudynku::Namiot(), FVector2D(Miejsca[k]), Miejsca[k].Z, Z[k]);
		if (!N)
		{
			continue;
		}
		// Rodzina stoi przed swoim namiotem, miedzy nim a ogniskiem.
		const FVector2D Wej = WejscieDo(N);
		FRodzina F;
		F.Namiot = N;
		F.Nazwa = NastepnaNazwa();
		F.Maz = NowyLudzik(Wej + FVector2D(FMath::FRandRange(-120.f, 120.f), FMath::FRandRange(-120.f, 120.f)), false);
		F.Zona = NowyLudzik(Wej + FVector2D(FMath::FRandRange(-120.f, 120.f), FMath::FRandRange(-120.f, 120.f)), true);
		for (ALudzik* L : { F.Maz.Get(), F.Zona.Get() })
		{
			if (L)
			{
				L->UstawDom(Wej);
			}
		}
		Rodziny.Add(F);
	}
	UE_LOG(LogTemp, Log, TEXT("Osada: oboz przy (%.0f, %.0f), %d rodzin"), Srodek.X, Srodek.Y, Rodziny.Num());
}

namespace
{
	/** Obrocony prostokat obrysu budynku w swiecie (pole i pastwisko zaznaczane na terenie, 21.11.2026). */
	struct FProstokat
	{
		FVector2D Srodek, OsX, OsY, Pol;
	};

	FProstokat ProstokatBudynku(const FRodzajBudynku& R, FVector2D Gdzie, float ObrotSt, float Zapas)
	{
		const FBox2D O = R.Obrys();
		const double Rad = FMath::DegreesToRadians(double(ObrotSt));
		FProstokat P;
		P.OsX = FVector2D(FMath::Cos(Rad), FMath::Sin(Rad));
		P.OsY = FVector2D(-P.OsX.Y, P.OsX.X);
		const FVector2D Cl = O.GetCenter();
		P.Srodek = Gdzie + P.OsX * Cl.X + P.OsY * Cl.Y;
		P.Pol = O.GetExtent() + FVector2D(Zapas, Zapas);
		return P;
	}

	/** Test osi rozdzielajacych dla dwoch obroconych prostokatow. */
	bool Nachodza(const FProstokat& A, const FProstokat& B)
	{
		const FVector2D D = B.Srodek - A.Srodek;
		for (const FVector2D& Os : { A.OsX, A.OsY, B.OsX, B.OsY })
		{
			const double RA = A.Pol.X * FMath::Abs(Os | A.OsX) + A.Pol.Y * FMath::Abs(Os | A.OsY);
			const double RB = B.Pol.X * FMath::Abs(Os | B.OsX) + B.Pol.Y * FMath::Abs(Os | B.OsY);
			if (FMath::Abs(D | Os) > RA + RB)
			{
				return false;
			}
		}
		return true;
	}
}

bool AOsada::Darmowy(const FRodzajBudynku& R) const
{
	if (!R.bPierwszyDarmowy)
	{
		return false;
	}
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		if (BW.IsValid() && BW->Rodzaj().Funkcja == R.Funkcja && BW->Rodzaj().Wyrob == R.Wyrob)
		{
			return false;
		}
	}
	return true;
}

ABudowa* AOsada::Postaw(const FRodzajBudynku& R, FVector2D Gdzie, float Obrot, float Z)
{
	// Ruiny pod nowym budynkiem znikaja (budowa na zgliszczach).
	for (int32 i = Ruiny.Num() - 1; i >= 0; --i)
	{
		ABudowa* Rn = Ruiny[i].Get();
		if (Rn && Nachodza(ProstokatBudynku(R, Gdzie, Obrot, 0.f),
			ProstokatBudynku(Rn->Rodzaj(), FVector2D(Rn->GetActorLocation()), float(Rn->GetActorRotation().Yaw), 0.f)))
		{
			Rn->Destroy();
			Ruiny.RemoveAt(i);
		}
	}
	const bool bDarmo = Darmowy(R);
	ABudowa* B = Spawnuj(R, Gdzie, Obrot, Z);
	if (B && bDarmo)
	{
		B->BezKosztu();
		Ogloszenie(FString::Printf(TEXT("%s: pierwsza w osadzie, bez materiału"), *R.Nazwa));
	}
	if (B && Teren.IsValid())
	{
		if (R.ZObszarem())
		{
			// Pole i pastwisko: karczowanie kolami wzdluz dluzszego boku prostokata (jedno kolo wycieloby las daleko poza plotem).
			const FProstokat P = ProstokatBudynku(R, Gdzie, Obrot, 50.f);
			const bool bWzdluzX = P.Pol.X >= P.Pol.Y;
			const double Dl = bWzdluzX ? P.Pol.X : P.Pol.Y, Kr = bWzdluzX ? P.Pol.Y : P.Pol.X;
			const FVector2D Os = bWzdluzX ? P.OsX : P.OsY;
			const int32 N = FMath::Max(1, FMath::CeilToInt32(2.0 * Dl / Kr));
			for (int32 k = 0; k <= N; ++k)
			{
				Teren->Wykarczuj(P.Srodek + Os * (-Dl + 2.0 * Dl * k / N), float(Kr * 1.12 + 50.0));
			}
		}
		else
		{
			Teren->Wykarczuj(Gdzie, R.Promien);
		}
	}
	return B;
}

FString AOsada::CzemuNieUlepsz(const ABudowa* B) const
{
	if (!B || !B->Rodzaj().Ulepszenie)
	{
		return TEXT("Najwyższy poziom");
	}
	if (!B->Gotowy())
	{
		return TEXT("Trwa budowa");
	}
	const FRodzajBudynku& Nowy = B->Rodzaj().Ulepszenie();
	if (const FString Blokada = CzemuZablokowany(Nowy); !Blokada.IsEmpty())
	{
		return Blokada;
	}
	if (Koliduje(Nowy, FVector2D(B->GetActorLocation()), float(B->GetActorRotation().Yaw), B))
	{
		return TEXT("Za mało miejsca: większy budynek wszedłby na sąsiada");
	}
	return FString();
}

bool AOsada::Ulepsz(ABudowa* B)
{
	if (!CzemuNieUlepsz(B).IsEmpty())
	{
		return false;
	}
	const FRodzajBudynku& Nowy = B->Rodzaj().Ulepszenie();
	if (Teren.IsValid())
	{
		Teren->Wykarczuj(FVector2D(B->GetActorLocation()), Nowy.Promien);
	}
	B->Ulepsz(Nowy);
	Ogloszenie(FString::Printf(TEXT("Przebudowa: %s"), *Nowy.Nazwa));
	return true;
}

bool AOsada::AnulujUlepszenie(ABudowa* B)
{
	if (!B || !B->Przebudowa())
	{
		return false;
	}
	Zwroc(B->AnulujUlepszenie());
	Ogloszenie(FString::Printf(TEXT("Przebudowa wstrzymana: %s, materiał wrócił do składu"), *B->Rodzaj().Nazwa));
	return true;
}

bool AOsada::MoznaRozebrac(const ABudowa* B)
{
	return B && !B->Podglad() && !B->Ruina() && B->Rodzaj().Model != FRodzajBudynku::Ognisko().Model && B->Rodzaj().Model != FRodzajBudynku::Namiot().Model;
}

bool AOsada::Rozbierz(ABudowa* B)
{
	if (!MoznaRozebrac(B))
	{
		return false;
	}
	const FString Nazwa = B->Rodzaj().Nazwa;
	const bool bBudowa = !B->Gotowy() && !B->Przebudowa();
	OdlaczBudynek(B);
	FSurowce Zwrot = B->Zapas;
	if (B->Przebudowa())
	{
		Zwrot += B->AnulujUlepszenie(); // material przebudowy w calosci, dalej jak gotowy budynek
	}
	if (B->Gotowy())
	{
		for (ESurowiec S : WszystkieSurowce) { Zwrot[S] += B->Rodzaj().Koszt[S] / 2; }
	}
	else
	{
		Zwrot += B->Dostarczone;
	}
	B->Destroy(); // najpierw: rozbierany magazyn nie moze przyjac zwrotu
	Zwroc(Zwrot);
	Ogloszenie(FString::Printf(TEXT("Rozebrano: %s. %s"), *Nazwa, bBudowa ? TEXT("Przyniesiony materiał wrócił do składu.") : TEXT("Połowa materiału i zapasy wróciły do składu.")));
	UE_LOG(LogTemp, Log, TEXT("Osada: rozebrano %s"), *Nazwa);
	return true;
}

void AOsada::OdlaczBudynek(ABudowa* B)
{
	// Rodziny: praca przepada, mieszkancy czekaja przy obozie na wolny dom (Wprowadzaj).
	for (FRodzina& F : Rodziny)
	{
		if (F.Praca.Get() == B)
		{
			F.Praca.Reset();
		}
		if (F.Dom.Get() == B)
		{
			F.Dom.Reset();
			const FVector2D Oboz = SrodekObozu + FVector2D(FMath::FRandRange(-600.f, 600.f), FMath::FRandRange(-600.f, 600.f));
			for (const TWeakObjectPtr<ALudzik>& L : { F.Maz, F.Zona })
			{
				if (L.IsValid())
				{
					L->UstawDom(Oboz);
				}
			}
		}
	}
	// Ludzie przy budynku: pomocnik wraca do swojej pracy, reszta odpoczywa; kurs z tego budynku przerwany (towar wraca do niego).
	for (const TWeakObjectPtr<ALudzik>& LW : Ludzie)
	{
		ALudzik* L = LW.Get();
		if (!L)
		{
			continue;
		}
		if (L->Praca() == B)
		{
			L->Odwolaj();
		}
		else if (L->ZrodloKursu() == B)
		{
			L->Przydziel(L->Praca(), L->RolaPracy(), false);
		}
	}
}

void AOsada::SprawdzZniszczenia()
{
	for (int32 i = Budynki.Num() - 1; i >= 0; --i)
	{
		ABudowa* B = Budynki[i].Get();
		if (!B || (!B->Ruina() && !B->Zgliszcza()))
		{
			continue;
		}
		const FString Nazwa = B->Rodzaj().Nazwa;
		OdlaczBudynek(B);
		Budynki.RemoveAt(i);
		if (B->Zgliszcza())
		{
			B->Destroy();
			Ogloszenie(FString::Printf(TEXT("Spłonęła budowa: %s"), *Nazwa));
		}
		else
		{
			Ruiny.Add(B);
			Ogloszenie(FString::Printf(TEXT("%s zawalił się! Kliknij ruinę, żeby ją odbudować albo uprzątnąć."), *Nazwa));
		}
		UE_LOG(LogTemp, Log, TEXT("Osada: zniszczony %s (%s)"), *Nazwa, B->Zgliszcza() ? TEXT("plac budowy") : TEXT("ruina"));
	}
	Ruiny.RemoveAll([](const TWeakObjectPtr<ABudowa>& R) { return !R.IsValid(); });
}

bool AOsada::Odbuduj(ABudowa* R)
{
	if (!R || !R->Ruina() || Ruiny.Remove(R) == 0)
	{
		return false;
	}
	R->Odbuduj();
	Budynki.Add(R);
	Ogloszenie(FString::Printf(TEXT("Odbudowa: %s"), *R->Rodzaj().Nazwa));
	return true;
}

bool AOsada::Uprzatnij(ABudowa* R)
{
	if (!R || !R->Ruina() || Ruiny.Remove(R) == 0)
	{
		return false;
	}
	// Z gruzu da sie wybrac czesc kamienia; drewno splonelo.
	const int32 Kamien = R->Rodzaj().Koszt[ESurowiec::Kamien] / 3;
	const FString Nazwa = R->Rodzaj().Nazwa;
	R->Destroy();
	Zwroc(ESurowiec::Kamien, Kamien);
	Ogloszenie(Kamien > 0 ? FString::Printf(TEXT("Uprzątnięto ruinę: %s, odzyskano %d kamienia"), *Nazwa, Kamien)
		: FString::Printf(TEXT("Uprzątnięto ruinę: %s"), *Nazwa));
	return true;
}

int32 AOsada::Ruin() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ABudowa>& R : Ruiny) { N += R.IsValid() ? 1 : 0; }
	return N;
}

int32 AOsada::Plonacych() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ABudowa>& B : Budynki) { N += (B.IsValid() && B->Ogien() > 0.f) ? 1 : 0; }
	return N;
}

bool AOsada::Zablokowana(const ABudowa* B) const
{
	if (!B || B->Gotowy() || !B->CzekaNaMaterial())
	{
		return false;
	}
	for (ESurowiec S : WszystkieSurowce)
	{
		if (B->WDrodze[S] > 0 || (B->Brakuje(S) > 0 && ZrodloSurowca(S, FVector2D(B->GetActorLocation()), nullptr)))
		{
			return false;
		}
	}
	return true;
}

bool AOsada::Koliduje(const FRodzajBudynku& RObecny, FVector2D Gdzie, float Obrot, const ABudowa* Pomin) const
{
	// 26.11.2026 (prosba uzytkownika, "najwazniejsze"): kolizja liczona z obrysu po pelnym ulepszeniu - i dla stawianego
	// ducha, i dla stojacych budynkow - zeby pozniejsze Ulepsz nie bylo zablokowane brakiem miejsca.
	const FRodzajBudynku& R = RObecny.NajwyzszyPoziom();
	for (const TWeakObjectPtr<ABudowa>& B : Budynki)
	{
		if (!B.IsValid() || B.Get() == Pomin)
		{
			continue;
		}
		const FRodzajBudynku& Inny = B->Rodzaj().NajwyzszyPoziom();
		if (R.bUmocnienie && Inny.bUmocnienie)
		{
			continue; // odcinki muru, wieze i bramy stoja jeden przy drugim
		}
		if (R.ZObszarem() || Inny.ZObszarem())
		{
			// Pole i pastwisko: prostokaty obrysu (zwykly budynek z 1 m zapasu) nie moga na siebie nachodzic.
			if (Nachodza(ProstokatBudynku(R, Gdzie, Obrot, R.ZObszarem() ? 0.f : 100.f),
				ProstokatBudynku(Inny, FVector2D(B->GetActorLocation()), float(B->GetActorRotation().Yaw), Inny.ZObszarem() ? 0.f : 100.f)))
			{
				return true;
			}
		}
		else if (Nachodza(ProstokatBudynku(R, Gdzie, Obrot, 50.f), ProstokatBudynku(Inny, FVector2D(B->GetActorLocation()), float(B->GetActorRotation().Yaw), 50.f)))
		{
			// 21.11.2026 (prosba uzytkownika "lepsza kolizja"): obrocone prostokaty obrysu z 0,5 m zapasu zamiast kol Promien -
			// kola nie pasowaly do prostokatnych domow (za duze odstepy przy bokach, a obrocone rogi zachodzily).
			return true;
		}
	}
	return false;
}

void AOsada::WyslijLudzi(ABudowa* B, bool bNaMiejscu)
{
	const FRodzajBudynku& R = B->Rodzaj();
	const int32 Ile = R.Budowniczych + R.Tragarzy;
	const float Kat0 = FMath::FRand() * 360.f;
	for (int32 k = 0; k < Ile; ++k)
	{
		// Przychodza gromadka z jednej strony, ~25 m od budowy.
		const float Kat = FMath::DegreesToRadians(Kat0 + FMath::FRandRange(-25.f, 25.f));
		const FVector2D Start = FVector2D(B->GetActorLocation()) + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * FMath::FRandRange(2300.f, 2900.f);
		if (ALudzik* L = NowyLudzik(Start, false))
		{
			L->Przydziel(B, k < R.Budowniczych ? ERolaLudzika::Budowniczy : ERolaLudzika::Tragarz, bNaMiejscu);
		}
	}
}

void AOsada::Przydzielaj(bool bNaMiejscu)
{
	// 23.11.2026 (prosba uzytkownika): priorytety jak w Manor Lords, a budowy bez materialu nie trzymaja ludzi.
	// Wyzszy priorytet najpierw, przy rownym najstarsze; do kazdej najblizsi wolni osadnicy, a gdy wolnych brak,
	// zabiera budowniczych / tragarzy z budowy o nizszym priorytecie. Zablokowana budowa oddaje swoich ludzi.
	TArray<ABudowa*> Kolejka;
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		ABudowa* B = BW.Get();
		if (!B || B->Gotowy())
		{
			continue;
		}
		if (Zablokowana(B))
		{
			for (const TWeakObjectPtr<ALudzik>& L : Ludzie)
			{
				if (L.IsValid() && L->Praca() == B && !L->WBitwie())
				{
					L->Odwolaj();
				}
			}
			continue;
		}
		Kolejka.Add(B);
	}
	Kolejka.StableSort([](const ABudowa& A, const ABudowa& B) { return A.Priorytet > B.Priorytet; });
	for (ABudowa* B : Kolejka)
	{
		const FRodzajBudynku& R = B->Rodzaj();
		int32 Bud = 0, Tr = 0;
		for (const TWeakObjectPtr<ALudzik>& L : Ludzie)
		{
			if (L.IsValid() && L->Praca() == B)
			{
				++(L->RolaPracy() == ERolaLudzika::Budowniczy ? Bud : Tr);
			}
		}
		while (Bud < R.Budowniczych || Tr < R.Tragarzy)
		{
			// Najpierw wolni, potem pracownicy bez zajecia w swoim budynku (pomoc, 23.11.2026),
			// potem ludzie z budowy o nizszym priorytecie.
			ALudzik* Naj = nullptr;
			int32 Ktora = 0;
			for (int32 Proba = 0; Proba < 3 && !Naj; ++Proba)
			{
				Ktora = Proba;
				double Odl = TNumericLimits<double>::Max();
				for (const TWeakObjectPtr<ALudzik>& L : Ludzie)
				{
					const ABudowa* Teraz = L.IsValid() ? L->Praca() : nullptr;
					const bool bMoze = L.IsValid() && (Proba == 0 ? L->Wolny() : Proba == 1 ? L->DoPomocy()
						: (Teraz && !Teraz->Gotowy() && Teraz->Priorytet < B->Priorytet && !L->WBitwie()
							&& (L->RolaPracy() == ERolaLudzika::Budowniczy || L->RolaPracy() == ERolaLudzika::Tragarz)));
					if (bMoze)
					{
						const double D = FVector2D::DistSquared(FVector2D(L->GetActorLocation()), FVector2D(B->GetActorLocation()));
						if (D < Odl)
						{
							Odl = D;
							Naj = L.Get();
						}
					}
				}
			}
			if (!Naj)
			{
				break; // wszyscy zajeci (nizsze budowy moga jeszcze wziac ludzi z jeszcze nizszych)
			}
			const bool bBudowniczy = Bud < R.Budowniczych;
			const ERolaLudzika Nowa = bBudowniczy ? ERolaLudzika::Budowniczy : ERolaLudzika::Tragarz;
			if (Ktora == 1)
			{
				Naj->Pomoz(B, Nowa);
			}
			else
			{
				Naj->Przydziel(B, Nowa, bNaMiejscu);
			}
			++(bBudowniczy ? Bud : Tr);
		}
	}
}

void AOsada::Wprowadzaj()
{
	// Gotowy dom: wprowadza sie rodzina z namiotu (najblizsza), jej namiot znika.
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		ABudowa* B = BW.Get();
		if (!B || !B->Gotowy() || B->Rodzaj().Rodzin <= 0 || B->Rodzaj().Fazy <= 1)
		{
			continue;
		}
		int32 Mieszka = 0;
		for (const FRodzina& F : Rodziny)
		{
			Mieszka += F.Dom.Get() == B ? 1 : 0;
		}
		while (Mieszka < B->Rodzaj().Rodzin)
		{
			FRodzina* Wybrana = nullptr;
			double Odl = TNumericLimits<double>::Max();
			for (FRodzina& F : Rodziny)
			{
				if (!F.Dom.IsValid())
				{
					// z namiotu albo spod obozu (dom rozebrany)
					const double D = FVector2D::DistSquared(DomRodziny(F), FVector2D(B->GetActorLocation()));
					if (D < Odl)
					{
						Odl = D;
						Wybrana = &F;
					}
				}
			}
			if (!Wybrana)
			{
				return; // wszyscy juz w domach
			}
			Wybrana->Dom = B;
			if (Wybrana->Namiot.IsValid())
			{
				Wybrana->Namiot->Destroy();
			}
			Wybrana->Namiot.Reset();
			const FVector2D Wej = WejscieDo(B);
			for (const TWeakObjectPtr<ALudzik>& L : { Wybrana->Maz, Wybrana->Zona })
			{
				if (L.IsValid())
				{
					L->UstawDom(Wej);
				}
			}
			++Mieszka;
			UE_LOG(LogTemp, Log, TEXT("Osada: rodzina wprowadza sie do: %s"), *B->Rodzaj().Nazwa);
		}
	}
}

void AOsada::PrzydzielTeraz(bool bNaMiejscu)
{
	Wprowadzaj();
	Przydzielaj(bNaMiejscu);
}

void AOsada::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bOboz)
	{
		KolorPory();
		// Tlo osady: wies (ptaki, zwierzeta) blisko obozu, gwar rozmow tylko z bliska i gdy jest kogo sluchac.
		if (UDzwieki* D = UDzwieki::Z(this))
		{
			const float Bliskosc = 1.f - FMath::Clamp((FVector2D::Distance(FVector2D(D->Ucho()), SrodekObozu) - 15000.f) / 25000.f, 0.f, 1.f);
			// Wedlug liczby ludzi: namiot z jedna rodzina to cicha wies, gwar rozmow dopiero od kilkunastu osob.
			const float N = Ludzie.Num();
			UDzwieki::Petla(this, "Petla_Wioska", EDzwiek::Osada, (0.05f + 0.25f * FMath::Clamp(N / 80.f, 0.f, 1.f)) /* 26.11.2026: ciszej, bylo 0,08 + 0,42 */ * Bliskosc);
			UDzwieki::Petla(this, "Petla_Gwar", EDzwiek::Osada, 0.2f * Bliskosc * D->Blisko() * FMath::Clamp((N - 10.f) / 60.f, 0.f, 1.f));
		}
	}
	DoPrzegladu -= DeltaSeconds;
	if (DoPrzegladu > 0.f)
	{
		return;
	}
	DoPrzegladu = 0.5f;
	Budynki.RemoveAll([](const TWeakObjectPtr<ABudowa>& B) { return !B.IsValid(); });
	SprawdzZniszczenia();
	Ludzie.RemoveAll([](const TWeakObjectPtr<ALudzik>& L) { return !L.IsValid(); });
	Woly.RemoveAll([](const TWeakObjectPtr<AWol>& W) { return !W.IsValid(); });
	if (bWidoczne)
	{
		Wprowadzaj();
		Przydzielaj(false);
		Rozwoz();
		RozwozWolami();
	}
	if (Teren.IsValid())
	{
		Teren->Rosnij(0.5f * ABudowa::Tempo);
	}
	// Pory roku (od zalozenia obozu).
	if (bOboz)
	{
		Kalendarz.Czas += 0.5f * ABudowa::Tempo;
		if (Kalendarz.Pora() != PoraTeraz)
		{
			PoraTeraz = Kalendarz.Pora();
			NowaPora(PoraTeraz);
		}
		if (PoraTeraz == EPora::Lato)
		{
			for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
			{
				if (ABudowa* B = BW.Get(); B && B->Uprawa == EUprawa::Rosnie)
				{
					B->UstawWzrost(Kalendarz.UlamekPory()); // lan rosnie przez cale lato
				}
			}
		}
	}
	// Krzaki odrastaja.
	for (int32 k = Zerwane.Num() - 1; k >= 0; --k)
	{
		Zerwane[k].Z -= 0.5f * ABudowa::Tempo;
		if (Zerwane[k].Z <= 0.f)
		{
			Zerwane.RemoveAtSwap(k);
		}
	}
	if (bOboz && Rodziny.Num() > 0)
	{
		DoPosilku -= 0.5f * ABudowa::Tempo;
		if (DoPosilku <= 0.f)
		{
			DoPosilku += CzasPosilku;
			Posilek();
		}
		LiczZadowolenie(0.5f * ABudowa::Tempo);
		LiczPoziomOsady();
	}
}

void AOsada::UstawWidocznosc(bool b)
{
	bWidoczne = b;
	for (const TArray<TWeakObjectPtr<ABudowa>>* Lista : { &Budynki, &Ruiny })
	{
		for (const TWeakObjectPtr<ABudowa>& B : *Lista)
		{
			if (B.IsValid())
			{
				B->SetActorHiddenInGame(!b);
			}
		}
	}
	if (AEfekty* E = AEfekty::W(GetWorld()))
	{
		E->SetActorHiddenInGame(!b); // ogien i dym nie wisza nad papierowa mapa
	}
	for (const TWeakObjectPtr<ALudzik>& L : Ludzie)
	{
		if (L.IsValid())
		{
			L->SetActorHiddenInGame(!b);
			L->SetActorTickEnabled(b);
		}
	}
	for (const TWeakObjectPtr<AWol>& W : Woly)
	{
		if (W.IsValid())
		{
			W->SetActorHiddenInGame(!b);
			W->SetActorTickEnabled(b);
		}
	}
}

int32 AOsada::Osadnikow() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ALudzik>& L : Ludzie) { N += L.IsValid() ? 1 : 0; }
	return N;
}

int32 AOsada::Wolnych() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ALudzik>& L : Ludzie) { N += (L.IsValid() && L->Wolny()) ? 1 : 0; }
	return N;
}

int32 AOsada::Budujacych(const ABudowa* B) const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ALudzik>& L : Ludzie) { N += (B && L.IsValid() && L->Praca() == B) ? 1 : 0; }
	return N;
}

int32 AOsada::WNamiotach() const
{
	int32 N = 0;
	for (const FRodzina& F : Rodziny) { N += F.Dom.IsValid() ? 0 : 1; }
	return N;
}

int32 AOsada::Budow() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ABudowa>& B : Budynki) { N += (B.IsValid() && !B->Gotowy() && !B->Ruina() && !B->Zgliszcza()) ? 1 : 0; }
	return N;
}

FSurowce AOsada::Zapasy() const
{
	FSurowce Suma;
	for (const TWeakObjectPtr<ABudowa>& B : Budynki)
	{
		if (B.IsValid() && B->Gotowy())
		{
			Suma += B->Zapas;
		}
	}
	return Suma;
}

ABudowa* AOsada::ZrodloSurowca(ESurowiec S, FVector2D Blisko, const ABudowa* Pomin) const
{
	ABudowa* Naj = nullptr;
	double Odl = TNumericLimits<double>::Max();
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		ABudowa* B = BW.Get();
		if (!B || B == Pomin || !B->Wydaje(S) || B->Dostepne(S) <= 0)
		{
			continue;
		}
		const double D = FVector2D::DistSquared(FVector2D(B->GetActorLocation()), Blisko);
		if (D < Odl)
		{
			Odl = D;
			Naj = B;
		}
	}
	return Naj;
}

bool AOsada::Zarezerwowane(FVector2D P, float R) const
{
	for (const FVector2D& Q : RezerwacjeDrzew)
	{
		if (FVector2D::DistSquared(P, Q) < double(R) * R)
		{
			return true;
		}
	}
	return false;
}

bool AOsada::ZarezerwujDrzewo(FVector2D Srodek, float Promien, FVector2D& Out)
{
	if (!Teren.IsValid() || !Teren->ZnajdzDrzewo(Srodek, Promien, RezerwacjeDrzew, Out))
	{
		return false;
	}
	RezerwacjeDrzew.Add(Out);
	return true;
}

void AOsada::ZwolnijDrzewo(FVector2D P)
{
	for (int32 k = 0; k < RezerwacjeDrzew.Num(); ++k)
	{
		if (FVector2D::DistSquared(RezerwacjeDrzew[k], P) < 1.0)
		{
			RezerwacjeDrzew.RemoveAtSwap(k);
			return;
		}
	}
}

void AOsada::ZetnijDrzewo(FVector2D P)
{
	ZwolnijDrzewo(P);
	if (Teren.IsValid())
	{
		Teren->Zetnij(P);
	}
}

bool AOsada::MiejsceNaDrzewo(FVector2D Srodek, float Promien, FVector2D& Out)
{
	if (!Teren.IsValid())
	{
		return false;
	}
	for (int32 Proba = 0; Proba < 24; ++Proba)
	{
		const float Kat = FMath::FRand() * 2.f * PI;
		const FVector2D P = Srodek + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * FMath::Sqrt(FMath::FRandRange(600.f * 600.f, Promien * Promien));
		float OdRzeki = 1e5f;
		const float H = FSwiat::Wysokosc(P.X, P.Y, &OdRzeki);
		if (H < 60.f || OdRzeki < 400.f || FSwiat::Woda(P.X, P.Y) != FSwiat::EWoda::Lad)
		{
			continue;
		}
		bool bWolne = !(bOboz && FVector2D::Distance(P, SrodekObozu) < PromienObozu + 200.f);
		// 26.11.2026 ("borowy sadzi na budynkach i drogach"): obrocony prostokat obrysu po pelnym ulepszeniu + 3 m zamiast kola
		// Promien (pola i pastwiska sa duzo wieksze niz Promien) i 3 m od kazdej drogi.
		for (const TWeakObjectPtr<ABudowa>& B : Budynki)
		{
			if (B.IsValid() && bWolne)
			{
				const FVector Lok = B->GetActorTransform().InverseTransformPositionNoScale(FVector(P, B->GetActorLocation().Z));
				bWolne = !B->Rodzaj().NajwyzszyPoziom().Obrys().ExpandBy(300.f).IsInside(FVector2D(Lok));
			}
		}
		if (const ADrogi* D = ADrogi::Obecne())
		{
			bWolne &= !D->NaDrodze(P, 300.f);
		}
		if (!bWolne || Zarezerwowane(P, 380.f) || Teren->DrzewoBlisko(P, 380.f))
		{
			continue;
		}
		Out = P;
		RezerwacjeDrzew.Add(P);
		return true;
	}
	return false;
}

void AOsada::Zasadz(FVector2D P)
{
	ZwolnijDrzewo(P);
	if (Teren.IsValid())
	{
		Teren->Zasadz(P);
	}
}

int32 AOsada::Sadzonek(FVector2D Srodek, float Promien) const
{
	return Teren.IsValid() ? Teren->Sadzonek(Srodek, Promien) : 0;
}

FVector2D AOsada::DomRodziny(const FRodzina& F) const
{
	const ABudowa* D = F.Dom.IsValid() ? F.Dom.Get() : F.Namiot.Get();
	return D ? WejscieDo(D) : SrodekObozu;
}

bool AOsada::DodajRodzine(ABudowa* B, bool bNaMiejscu)
{
	if (!B || !B->Gotowy() || RodzinWPracy(B) >= B->Rodzaj().Miejsc)
	{
		return false;
	}
	// Najblizsza (po domu) rodzina bez pracy.
	FRodzina* Wybrana = nullptr;
	double Odl = TNumericLimits<double>::Max();
	for (FRodzina& F : Rodziny)
	{
		if (F.Praca.IsValid())
		{
			continue;
		}
		const double D = FVector2D::DistSquared(DomRodziny(F), FVector2D(B->GetActorLocation()));
		if (D < Odl)
		{
			Odl = D;
			Wybrana = &F;
		}
	}
	if (!Wybrana)
	{
		return false;
	}
	Wybrana->Praca = B;
	const ERolaLudzika Rola = RolaW(B->Rodzaj());
	for (const TWeakObjectPtr<ALudzik>& L : { Wybrana->Maz, Wybrana->Zona })
	{
		if (L.IsValid())
		{
			// Zbrojownia (etap 4): wojem zostaje maz, zona zostaje wolna (buduje, nosi).
			const bool bWoj = Rola == ERolaLudzika::Woj;
			L->Przydziel(bWoj && L == Wybrana->Zona ? nullptr : B, Rola, bNaMiejscu);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("Osada: %s pracuje w: %s"), *Wybrana->Nazwa, *B->Rodzaj().Nazwa);
	return true;
}

bool AOsada::UsunRodzine(ABudowa* B)
{
	for (int32 k = Rodziny.Num() - 1; k >= 0; --k)
	{
		FRodzina& F = Rodziny[k];
		if (F.Praca.Get() != B)
		{
			continue;
		}
		F.Praca.Reset();
		for (const TWeakObjectPtr<ALudzik>& L : { F.Maz, F.Zona })
		{
			if (L.IsValid())
			{
				L->Przydziel(nullptr, ERolaLudzika::Budowniczy, false);
			}
		}
		return true;
	}
	return false;
}

int32 AOsada::Wojow() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ALudzik>& L : Ludzie)
	{
		N += (L.IsValid() && !L->Wolny() && L->RolaPracy() == ERolaLudzika::Woj) ? 1 : 0;
	}
	return N;
}

ABudowa* AOsada::Targ() const
{
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		if (BW.IsValid() && BW->Gotowy() && BW->Rodzaj().Model == FRodzajBudynku::Targ().Model)
		{
			return BW.Get();
		}
	}
	return nullptr;
}

ABudowa* AOsada::Karczma() const
{
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		if (BW.IsValid() && BW->Gotowy() && BW->Rodzaj().Model == FRodzajBudynku::Karczma().Model)
		{
			return BW.Get();
		}
	}
	return nullptr;
}

int32 AOsada::Gotowych(const FRodzajBudynku& R) const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		N += (BW.IsValid() && BW->Gotowy() && BW->Rodzaj().Model == R.Model) ? 1 : 0;
	}
	return N;
}

int32 AOsada::Domow() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		N += (BW.IsValid() && BW->Gotowy() && BW->Rodzaj().Rodzin > 0 && BW->Rodzaj().Fazy > 1) ? 1 : 0;
	}
	return N;
}

void AOsada::RodzinyPokazu(int32 N)
{
	// Pokaz (-osada): dodatkowe rodziny przy ognisku, bez domu (do budynkow etapu 4 i zbrojowni).
	for (int32 k = 0; k < N; ++k)
	{
		const float Kat = FMath::FRand() * 2.f * PI;
		const FVector2D Wej = SrodekObozu + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * FMath::FRandRange(300.f, 700.f);
		FRodzina F;
		F.Nazwa = NastepnaNazwa();
		F.Maz = NowyLudzik(Wej, false);
		F.Zona = NowyLudzik(Wej + FVector2D(120.f, 80.f), true);
		for (ALudzik* L : { F.Maz.Get(), F.Zona.Get() })
		{
			if (L)
			{
				L->UstawDom(Wej);
			}
		}
		Rodziny.Add(F);
	}
}

int32 AOsada::WolnychRodzin() const
{
	int32 N = 0;
	for (const FRodzina& F : Rodziny) { N += F.Praca.IsValid() ? 0 : 1; }
	return N;
}

int32 AOsada::RodzinWPracy(const ABudowa* B) const
{
	int32 N = 0;
	for (const FRodzina& F : Rodziny) { N += (B && F.Praca.Get() == B) ? 1 : 0; }
	return N;
}

void AOsada::PracujacyW(const ABudowa* B, TArray<FString>& Nazwy, TArray<FString>& CoRobia) const
{
	Nazwy.Reset();
	CoRobia.Reset();
	for (const FRodzina& F : Rodziny)
	{
		if (!B || F.Praca.Get() != B)
		{
			continue;
		}
		Nazwy.Add(F.Nazwa);
		const FString M = F.Maz.IsValid() ? F.Maz->CoRobi() : FString();
		const FString Z = F.Zona.IsValid() ? F.Zona->CoRobi() : FString();
		CoRobia.Add(M == Z ? M : FString::Printf(TEXT("%s, %s"), *M, *Z));
	}
}

TArray<FString> AOsada::MieszkancyW(const ABudowa* B) const
{
	TArray<FString> W;
	for (const FRodzina& F : Rodziny)
	{
		if (B && (F.Dom.Get() == B || (!F.Dom.IsValid() && F.Namiot.Get() == B)))
		{
			W.Add(F.Nazwa);
		}
	}
	return W;
}

ABudowa* AOsada::BudynekPod(FVector2D P) const
{
	// W obrysie budynku (oboz: kolo 5 m wokol ogniska, z zapasem); najblizej srodka wygrywa.
	// Ruiny tez (okno ruiny: odbuduj / uprzatnij; drogi je omijaja).
	ABudowa* Naj = nullptr;
	float Najl = 1.f;
	TArray<ABudowa*> Wszystkie;
	for (const TArray<TWeakObjectPtr<ABudowa>>* Lista : { &Budynki, &Ruiny })
	{
		for (const TWeakObjectPtr<ABudowa>& BW : *Lista) { if (BW.IsValid()) { Wszystkie.Add(BW.Get()); } }
	}
	for (ABudowa* B : Wszystkie)
	{
		FVector2D L(B->GetActorTransform().InverseTransformPosition(FVector(P, B->GetActorLocation().Z)));
		FVector2D H = B->Rodzaj().Model == TEXT("SM_Ognisko") ? FVector2D(520.f, 520.f) : B->Rodzaj().Polowa;
		if (B->Rodzaj().ZObszarem())
		{
			// Pole i pastwisko: caly zaznaczony prostokat (owczarnia: szopa nie jest w jego srodku).
			const FBox2D O = B->Rodzaj().Obrys();
			L -= O.GetCenter();
			H = O.GetExtent();
		}
		const float Ocena = float(FMath::Max(FMath::Abs(L.X) / H.X, FMath::Abs(L.Y) / H.Y));
		if (Ocena <= Najl)
		{
			Najl = Ocena;
			Naj = B;
		}
	}
	return Naj;
}

void AOsada::Rozwoz()
{
	// Paczka A: targ trzyma zapas sukna dla rodzin (ubrania); wolny osadnik donosi je z tkacza albo magazynu.
	if (ABudowa* T = Targ(); T && T->Zapas[ESurowiec::Sukno] + T->WDrodze[ESurowiec::Sukno] < 6 && T->Miejsce(ESurowiec::Sukno) > 0)
	{
		if (ABudowa* Z = ZrodloSurowca(ESurowiec::Sukno, FVector2D(T->GetActorLocation()), T))
		{
			ALudzik* Naj = nullptr;
			double Odl = TNumericLimits<double>::Max();
			for (const TWeakObjectPtr<ALudzik>& L : Ludzie)
			{
				const double D = L.IsValid() && L->Wolny() ? FVector2D::DistSquared(FVector2D(L->GetActorLocation()), FVector2D(Z->GetActorLocation())) : Odl;
				if (D < Odl)
				{
					Odl = D;
					Naj = L.Get();
				}
			}
			if (Naj)
			{
				Naj->Przewiez(Z, T, ESurowiec::Sukno);
			}
		}
	}
	// Wyroby (klody drwala, deski pilarni) na najblizszy magazyn z miejscem; do 3 sztuk na kurs wolnego osadnika.
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		ABudowa* B = BW.Get();
		if (!B || !B->Gotowy() || B->Rodzaj().Wyrob == ESurowiec::Ile || B->Rodzaj().Magazynuje())
		{
			continue;
		}
		const ESurowiec S = B->Rodzaj().Wyrob;
		int32 Dost = B->Dostepne(S);
		while (Dost >= 2 || (Dost > 0 && B->Zapas[S] >= B->Rodzaj().Pojemnosc))
		{
			ABudowa* Cel = nullptr;
			double Odl = TNumericLimits<double>::Max();
			for (const TWeakObjectPtr<ABudowa>& MW : Budynki)
			{
				if (MW.IsValid() && MW->Miejsce(S) > 0)
				{
					const double D = FVector2D::DistSquared(FVector2D(MW->GetActorLocation()), FVector2D(B->GetActorLocation()));
					if (D < Odl)
					{
						Odl = D;
						Cel = MW.Get();
					}
				}
			}
			if (!Cel)
			{
				break; // magazyny pelne
			}
			ALudzik* Naj = nullptr;
			double OdlL = TNumericLimits<double>::Max();
			for (const TWeakObjectPtr<ALudzik>& L : Ludzie)
			{
				if (L.IsValid() && L->Wolny())
				{
					const double D = FVector2D::DistSquared(FVector2D(L->GetActorLocation()), FVector2D(B->GetActorLocation()));
					if (D < OdlL)
					{
						OdlL = D;
						Naj = L.Get();
					}
				}
			}
			if (!Naj)
			{
				return; // wszyscy zajeci
			}
			Naj->Przewiez(B, Cel, S);
			Dost = B->Dostepne(S); // noszacy bierze do 3 sztuk naraz (balans 21.11.2026)
		}
	}
}

void AOsada::RozwozWolami()
{
	// Dostawia zwierzeta: dwa woly startowe przy obozie, po Wolow / Koni na kazda gotowa zagrode / stajnie.
	auto Nowy = [this](FVector2D Gdzie, ABudowa* Z, FVector2D Dom, bool bKon)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AWol* W = GetWorld()->SpawnActor<AWol>(FVector(Gdzie, FSwiat::Wysokosc(Gdzie.X, Gdzie.Y)), FRotator(0.f, FMath::FRand() * 360.f, 0.f), P))
		{
			if (bKon)
			{
				W->UstawKonia();
			}
			W->Zagroda = Z;
			W->UstawDom(Dom, 300.f);
			W->SetActorHiddenInGame(!bWidoczne);
			W->SetActorTickEnabled(bWidoczne);
			Woly.Add(W);
		}
	};
	int32 Startowych = 0;
	TMap<const ABudowa*, int32> WZagrodzie;
	for (const TWeakObjectPtr<AWol>& W : Woly)
	{
		if (W.IsValid())
		{
			W->Zagroda.IsValid() ? ++WZagrodzie.FindOrAdd(W->Zagroda.Get()) : ++Startowych;
		}
	}
	if (bOboz)
	{
		for (; Startowych < WolowNaStart; ++Startowych)
		{
			const float Kat = FMath::FRand() * 2.f * PI;
			const FVector2D P = SrodekObozu + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * PromienObozu * 0.9f;
			Nowy(P, nullptr, P, false);
		}
	}
	ABudowa* PierwszaZagroda = nullptr;
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		ABudowa* B = BW.Get();
		const int32 Daje = B && B->Gotowy() ? B->Rodzaj().Wolow + B->Rodzaj().Koni : 0;
		if (Daje <= 0)
		{
			continue;
		}
		if (B->Rodzaj().Wolow > 0 && !PierwszaZagroda)
		{
			PierwszaZagroda = B;
		}
		for (int32 N = WZagrodzie.FindRef(B); N < Daje; ++N)
		{
			const FVector2D Wej = WejscieDo(B);
			Nowy(Wej + FVector2D(FMath::FRandRange(-200.f, 200.f), FMath::FRandRange(-150.f, 150.f)), B, Wej, B->Rodzaj().Koni > 0);
		}
	}
	for (const TWeakObjectPtr<AWol>& W : Woly)
	{
		if (W.IsValid() && !W->Zagroda.IsValid() && PierwszaZagroda)
		{
			W->UstawDom(WejscieDo(PierwszaZagroda), 300.f); // startowe woly przechodza do pierwszej zagrody
		}
	}

	// Kursy: woly klody, konie deski i kamien. Najpierw budowa o najwyzszym priorytecie, ktorej brakuje
	// min. 2 sztuk, potem wywozka od wytworcy (min. 3 sztuki) do najblizszego magazynu.
	auto Kurs = [this](ESurowiec S, int32 Max, ABudowa*& Z, ABudowa*& C, int32& Ile)
	{
		int32 NajP = TNumericLimits<int32>::Lowest();
		for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
		{
			ABudowa* B = BW.Get();
			if (!B || B->Gotowy() || B->Brakuje(S) < 2 || B->Priorytet <= NajP)
			{
				continue;
			}
			ABudowa* Zr = ZrodloSurowca(S, FVector2D(B->GetActorLocation()), B);
			if (Zr && Zr->Dostepne(S) >= 2)
			{
				Z = Zr;
				C = B;
				NajP = B->Priorytet;
				Ile = FMath::Min3(B->Brakuje(S), Zr->Dostepne(S), Max);
			}
		}
		for (int32 k = 0; !C && k < Budynki.Num(); ++k)
		{
			ABudowa* B = Budynki[k].Get();
			if (!B || !B->Gotowy() || B->Rodzaj().Wyrob != S || B->Rodzaj().Magazynuje() || B->Dostepne(S) < 3)
			{
				continue;
			}
			double Odl = TNumericLimits<double>::Max();
			for (const TWeakObjectPtr<ABudowa>& MW : Budynki)
			{
				const double D = MW.IsValid() && MW->Miejsce(S) > 0
					? FVector2D::DistSquared(FVector2D(MW->GetActorLocation()), FVector2D(B->GetActorLocation())) : Odl;
				if (D < Odl)
				{
					Odl = D;
					C = MW.Get();
				}
			}
			if (C)
			{
				Z = B;
				Ile = FMath::Min3(B->Dostepne(S), C->Miejsce(S), Max);
			}
		}
	};
	for (const bool bKon : { false, true })
	{
		for (;;)
		{
			AWol* Naj = nullptr;
			ABudowa* Z = nullptr;
			ABudowa* C = nullptr;
			int32 Ile = 0;
			ESurowiec S = ESurowiec::Kloda;
			for (ESurowiec Sur : bKon ? TArray<ESurowiec>{ ESurowiec::Deska, ESurowiec::Kamien } : TArray<ESurowiec>{ ESurowiec::Kloda })
			{
				if (!C)
				{
					Kurs(Sur, bKon ? 8 : 6, Z, C, Ile);
					S = Sur;
				}
			}
			if (!C)
			{
				break;
			}
			double Odl = TNumericLimits<double>::Max();
			for (const TWeakObjectPtr<AWol>& W : Woly)
			{
				const double D = W.IsValid() && W->Wolny() && W->Kon() == bKon
					? FVector2D::DistSquared(FVector2D(W->GetActorLocation()), FVector2D(Z->GetActorLocation())) : Odl;
				if (D < Odl)
				{
					Odl = D;
					Naj = W.Get();
				}
			}
			if (!Naj)
			{
				break; // wszystkie w drodze
			}
			Naj->Ciagnij(Z, C, FMath::Min(Ile, Naj->Udzwig()), S);
		}
	}
}

void AOsada::UstawTeren(ATerenKafle* T)
{
	Teren = T;
}

// --- Etap 2: krzaki, polowanie, posilki, zadowolenie ---

bool AOsada::ZarezerwujKrzak(FVector2D Srodek, float Promien, FVector2D& Out)
{
	if (!Teren.IsValid())
	{
		return false;
	}
	TArray<FVector2D> Pomin = RezerwacjeKrzakow;
	for (const FVector& Z : Zerwane)
	{
		Pomin.Add(FVector2D(Z));
	}
	if (!Teren->ZnajdzKrzak(Srodek, Promien, Pomin, Out))
	{
		return false;
	}
	RezerwacjeKrzakow.Add(Out);
	return true;
}

void AOsada::ZwolnijKrzak(FVector2D P)
{
	for (int32 k = 0; k < RezerwacjeKrzakow.Num(); ++k)
	{
		if (FVector2D::DistSquared(RezerwacjeKrzakow[k], P) < 1.0)
		{
			RezerwacjeKrzakow.RemoveAtSwap(k);
			return;
		}
	}
}

void AOsada::ZerwijKrzak(FVector2D P)
{
	ZwolnijKrzak(P);
	Zerwane.Add(FVector(P, CzasOdrostu));
}

bool AOsada::MiejscePolowania(FVector2D Srodek, float Promien, FVector2D& Out, int32& Las) const
{
	if (!Teren.IsValid())
	{
		return false;
	}
	// Kilka losowych miejsc w zasiegu, wygrywa najgestszy las (zwierzyna trzyma sie lasu).
	Las = 0;
	for (int32 Proba = 0; Proba < 10; ++Proba)
	{
		const float Kat = FMath::FRand() * 2.f * PI;
		const FVector2D P = Srodek + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * FMath::FRandRange(1200.f, Promien);
		if (FSwiat::Woda(P.X, P.Y) != FSwiat::EWoda::Lad || FSwiat::Wysokosc(P.X, P.Y) < 60.f)
		{
			continue;
		}
		const int32 N = Teren->DrzewWKole(P, 1500.f);
		if (N > Las)
		{
			Las = N;
			Out = P;
		}
	}
	return Las >= 3;
}

void AOsada::Posilek()
{
	// Kazda rodzina zjada porcje z najblizszego zapasu (spichlerz, oboz, stos przy chacie zbieracza / mysliwego).
	bPierwszyPosilek = true;
	int32 Glodne = 0, Zimne = 0, Obdarte = 0;
	for (FRodzina& F : Rodziny)
	{
		// Paczka A: kazdy rodzaj jedzenia, jaki jest (jagody / mieso / chleb i ryby), po porcji - rozne jedzenie = wiecej zadowolenia.
		F.Rodzajow = 0;
		for (ESurowiec S : { ESurowiec::Jedzenie, ESurowiec::Ryby })
		{
			if (ABudowa* Z = ZrodloSurowca(S, DomRodziny(F), nullptr))
			{
				Z->ZmienZapas(S, -1);
				++F.Rodzajow;
			}
		}
		F.bNajedzona = F.Rodzajow > 0;
		Glodne += F.bNajedzona ? 0 : 1;
		// Zima: narecze polan (paczka A), a bez nich kloda do pieca na kazdy posilek.
		F.bCieplo = true;
		if (Pora() == EPora::Zima)
		{
			ABudowa* K = ZrodloSurowca(ESurowiec::Opal, DomRodziny(F), nullptr);
			const ESurowiec Pali = K ? ESurowiec::Opal : ESurowiec::Kloda;
			K = K ? K : ZrodloSurowca(ESurowiec::Kloda, DomRodziny(F), nullptr);
			F.bCieplo = K != nullptr;
			if (K)
			{
				K->ZmienZapas(Pali, -1);
			}
			Zimne += F.bCieplo ? 0 : 1;
		}
		// Paczka A: co PosilkowNaUbranie posilkow ubranie i buty sie zdzieraja - nowe tylko z targu: ubranie od krawca,
		// a gdy go nie ma, bela sukna (gorsze); buty od szewca.
		if (--F.DoUbrania <= 0)
		{
			F.DoUbrania = PosilkowNaUbranie;
			ABudowa* T = Targ();
			F.Ubranie = 0;
			for (const ESurowiec S : { ESurowiec::Ubrania, ESurowiec::Sukno })
			{
				if (F.Ubranie == 0 && T && T->Dostepne(S) > 0)
				{
					T->ZmienZapas(S, -1);
					F.Ubranie = S == ESurowiec::Ubrania ? 2 : 1;
				}
			}
			F.bUbrana = F.Ubranie > 0;
			Obdarte += F.bUbrana ? 0 : 1;
			F.bObuta = T && T->Dostepne(ESurowiec::Buty) > 0;
			if (F.bObuta)
			{
				T->ZmienZapas(ESurowiec::Buty, -1);
			}
		}
		// Krok 3: co drugi posilek baryłka piwa z karczmy (wieczor w karczmie).
		if (--F.DoPiwa <= 0)
		{
			F.DoPiwa = 2;
			ABudowa* K = Karczma();
			F.bPiwo = K && K->Dostepne(ESurowiec::Piwo) > 0;
			if (F.bPiwo)
			{
				K->ZmienZapas(ESurowiec::Piwo, -1);
			}
		}
	}
	if (Obdarte > 0 && Glodne == 0 && Zimne == 0)
	{
		Ogloszenie(Targ() ? FString::Printf(TEXT("Na targu zabrakło ubrań i sukna: %d %s bez nowego ubrania"), Obdarte, Obdarte == 1 ? TEXT("rodzina") : (Obdarte < 5 ? TEXT("rodziny") : TEXT("rodzin")))
			: FString(TEXT("Rodziny potrzebują ubrań: postaw targ, tkacza i krawca")));
	}
	UE_LOG(LogTemp, Log, TEXT("Osada: posilek, glodnych rodzin: %d z %d, jedzenia zostalo: %d"), Glodne, Rodziny.Num(), Zapasy()[ESurowiec::Jedzenie]);
	if (Glodne > 0)
	{
		Ogloszenie(Glodne == Rodziny.Num() ? FString(TEXT("Zabrakło jedzenia: wszyscy chodzą głodni"))
			: FString::Printf(TEXT("Zabrakło jedzenia dla %d %s"), Glodne, Glodne == 1 ? TEXT("rodziny") : TEXT("rodzin")));
	}
	else if (Rodziny.Num() > 0 && Zapasy()[ESurowiec::Jedzenie] < Rodziny.Num() * 2)
	{
		// Ostrzezenie zawczasu (sprawdzenie x10: glod przychodzil bez zapowiedzi).
		const int32 Posilkow = Zapasy()[ESurowiec::Jedzenie] / Rodziny.Num();
		Ogloszenie(Posilkow == 0 ? FString(TEXT("Kończy się jedzenie: nie starczy na następny posiłek. Postaw pole, zbieracza, myśliwego albo rybaka"))
			: FString(TEXT("Kończy się jedzenie: starczy na jeden posiłek. Postaw pole, zbieracza, myśliwego albo rybaka")));
	}
	else if (Zimne > 0)
	{
		Ogloszenie(FString::Printf(TEXT("Zabrakło drewna na opał: marznie %d %s"), Zimne, Zimne == 1 ? TEXT("rodzina") : (Zimne < 5 ? TEXT("rodziny") : TEXT("rodzin"))));
	}
}

float AOsada::CelZadowolenia(float* Jedz, float* Domy, float* Wiara, float* Cieplo, float* Ubranie, float* Wygody) const
{
	const int32 N = FMath::Max(1, Rodziny.Num());
	int32 Syte = 0, WDomach = 0, Cieple = 0, Rozne = 0, Odziez = 0, Obute = 0, ZPiwem = 0;
	for (const FRodzina& F : Rodziny)
	{
		Syte += F.bNajedzona ? 1 : 0;
		Rozne += F.Rodzajow >= 2 ? 1 : 0;
		Cieple += F.bCieplo ? 1 : 0;
		WDomach += F.Dom.IsValid() ? 1 : 0;
		Odziez += F.Ubranie;
		Obute += F.bObuta ? 1 : 0;
		ZPiwem += F.bPiwo ? 1 : 0;
	}
	bool bKosciol = false;
	int32 PoziomKosciola = 0;
	for (const TWeakObjectPtr<ABudowa>& B : Budynki)
	{
		// Poziomy (22.11.2026): kosciol drewniany daje 12, kamienny 16, z placem 20 (liczy sie najwyzszy).
		if (B.IsValid() && B->Gotowy() && B->Rodzaj().Religia())
		{
			bKosciol = true;
			PoziomKosciola = FMath::Max(PoziomKosciola, B->Rodzaj().Poziom);
		}
	}
	// Etap 3: jedzenie 40 + cieplo 10 (poza zima zawsze pelne) + domy 30 + kosciol 20.
	// Paczka A (23.11.2026): jedzenie 35 + cieplo 10 + domy 25 + kosciol 20 + ubranie z targu 10.
	// Krok 3: jedzenie 30 (22 najedzenie + 8 drugi rodzaj) + cieplo 10 + domy 20 + kosciol 20 + ubranie 10 (sukno 5,
	// ubranie od krawca 10) + wygody 10 (buty 5, piwo 5).
	const float J = (22.f * Syte + 8.f * Rozne) / N, C = 10.f * Cieple / N, D = 20.f * WDomach / N, U = 5.f * Odziez / N,
		G = (5.f * Obute + 5.f * ZPiwem) / N,
		W = bKosciol ? (PoziomKosciola >= 3 ? 20.f : (PoziomKosciola == 2 ? 16.f : 12.f)) : 0.f;
	if (Jedz) { *Jedz = J; }
	if (Domy) { *Domy = D; }
	if (Wiara) { *Wiara = W; }
	if (Cieplo) { *Cieplo = C; }
	if (Ubranie) { *Ubranie = U; }
	if (Wygody) { *Wygody = G; }
	return FMath::Clamp(J + C + D + W + U + G + ZPodatkow(), 0.f, 100.f);
}

int32 AOsada::Ubranych() const
{
	int32 N = 0;
	for (const FRodzina& F : Rodziny) { N += F.bUbrana ? 1 : 0; }
	return N;
}

FString AOsada::NapisPoziomu() const
{
	FString T = NazwaPoziomu(EPoziomOsady(PoziomB));
	return T.Left(1).ToUpper() + T.Mid(1);
}

bool AOsada::WarunkiPoziomu(uint8 P, TArray<FString>* Out) const
{
	// Paczka A: awans jak w Manor Lords - liczba rodzin, budynki i spelnione potrzeby.
	struct FWarunek { bool bJest; FString Tekst; };
	TArray<FWarunek> W;
	const int32 N = Rodziny.Num();
	const int32 Z = Zadowolenie();
	int32 PoziomKosciola = 0;
	for (const TWeakObjectPtr<ABudowa>& B : Budynki)
	{
		if (B.IsValid() && B->Gotowy() && B->Rodzaj().Religia())
		{
			PoziomKosciola = FMath::Max(PoziomKosciola, B->Rodzaj().Poziom);
		}
	}
	auto Dodaj = [&W](bool bJest, const FString& T) { W.Add({ bJest, T }); };
	if (P == 1)
	{
		Dodaj(N >= 8, FString::Printf(TEXT("8 rodzin (jest %d)"), N));
		Dodaj(Domow() >= 4, FString::Printf(TEXT("4 domy (jest %d)"), Domow()));
		Dodaj(PoziomKosciola >= 1, TEXT("kościół"));
		Dodaj(Z >= 50, FString::Printf(TEXT("zadowolenie 50%% (jest %d%%)"), Z));
	}
	else if (P == 2)
	{
		Dodaj(N >= 15, FString::Printf(TEXT("15 rodzin (jest %d)"), N));
		Dodaj(Targ() != nullptr, TEXT("targ"));
		Dodaj(Ubranych() * 4 >= N * 3, FString::Printf(TEXT("3/4 rodzin ubranych (jest %d z %d)"), Ubranych(), N));
		Dodaj(Z >= 60, FString::Printf(TEXT("zadowolenie 60%% (jest %d%%)"), Z));
	}
	else
	{
		Dodaj(N >= 30, FString::Printf(TEXT("30 rodzin (jest %d)"), N));
		Dodaj(PoziomKosciola >= 2, TEXT("kościół kamienny"));
		Dodaj(Karczma() != nullptr, TEXT("karczma"));
		Dodaj(Z >= 70, FString::Printf(TEXT("zadowolenie 70%% (jest %d%%)"), Z));
	}
	bool bWszystkie = true;
	for (const FWarunek& X : W)
	{
		bWszystkie &= X.bJest;
		if (Out)
		{
			Out->Add(FString::Printf(TEXT("%s %s"), X.bJest ? TEXT("[tak]") : TEXT("[nie]"), *X.Tekst));
		}
	}
	return bWszystkie;
}

void AOsada::LiczPoziomOsady()
{
	// Poziom tylko rosnie (spadek zadowolenia nie odbiera awansu).
	while (PoziomB < 3 && WarunkiPoziomu(PoziomB + 1, nullptr))
	{
		++PoziomB;
		Ogloszenie(FString::Printf(TEXT("Awans! Osada jest teraz: %s"), *NapisPoziomu().ToLower()));
	}
}

FString AOsada::OpisPoziomu() const
{
	FString T = FString::Printf(TEXT("Poziom osady: %s\n"), *NapisPoziomu());
	if (PoziomB >= 3)
	{
		return T + TEXT("Najwyższy poziom.");
	}
	TArray<FString> L;
	WarunkiPoziomu(PoziomB + 1, &L);
	T += FString::Printf(TEXT("\nAwans na: %s\n"), NazwaPoziomu(EPoziomOsady(PoziomB + 1)));
	T += FString::Join(L, TEXT("\n"));
	TArray<FString> Odbl;
	for (const FRodzajBudynku* R : FRodzajBudynku::DoBudowania())
	{
		if (R->Wymaga == PoziomB + 1)
		{
			Odbl.Add(R->Nazwa);
		}
	}
	for (const FRodzajBudynku* R : { &FRodzajBudynku::Kosciol(), &FRodzajBudynku::KosciolWielki(), &FRodzajBudynku::DomDuzy() })
	{
		if (R->Wymaga == PoziomB + 1)
		{
			Odbl.Add(R->Nazwa + TEXT(" (ulepszenie)"));
		}
	}
	if (Odbl.Num())
	{
		T += TEXT("\n\nOdblokuje: ") + FString::Join(Odbl, TEXT(", "));
	}
	return T;
}

FString AOsada::CzemuZablokowany(const FRodzajBudynku& R) const
{
	return R.Wymaga > PoziomB ? FString::Printf(TEXT("Wymaga poziomu osady: %s"), NazwaPoziomu(EPoziomOsady(R.Wymaga))) : FString();
}

void AOsada::LiczZadowolenie(float Dt)
{
	// Zadowolenie idzie powoli do celu, zeby jeden glodny posilek nie wywracal osady: rosnie 1 punkt na sekunde gry,
	// spada 0,3 punktu (balans 21.11.2026: uzytkownik - "za szybko traca zadowolenie").
	const float CelZ = CelZadowolenia();
	ZadowolenieF = FMath::FInterpConstantTo(ZadowolenieF, CelZ, Dt, CelZ < ZadowolenieF ? 0.3f : 1.f);
	const int32 Z = Zadowolenie();
	if (Z >= 60 && WolnychMiejsc() > 0)
	{
		DoNowej -= Dt;
		if (DoNowej <= 0.f)
		{
			DoNowej = CzasNowejRodziny;
			NowaRodzina();
		}
	}
	else
	{
		DoNowej = CzasNowejRodziny;
	}
	if (Z < 30 && Rodziny.Num() > 2)
	{
		DoOdejscia -= Dt;
		if (DoOdejscia <= 0.f)
		{
			DoOdejscia = CzasOdejscia;
			OdejdzRodzina();
		}
	}
	else
	{
		DoOdejscia = CzasOdejscia;
	}
}

int32 AOsada::Glodnych() const
{
	int32 N = 0;
	for (const FRodzina& F : Rodziny) { N += F.bNajedzona ? 0 : 1; }
	return N;
}

int32 AOsada::WolnychMiejsc() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		const ABudowa* B = BW.Get();
		if (!B || !B->Gotowy() || B->Rodzaj().Funkcja != EFunkcja::Mieszkalny || B->Rodzaj().Fazy <= 1)
		{
			continue;
		}
		int32 Mieszka = 0;
		for (const FRodzina& F : Rodziny) { Mieszka += F.Dom.Get() == B ? 1 : 0; }
		N += FMath::Max(0, B->Rodzaj().Rodzin - Mieszka);
	}
	// Rodziny z namiotow wprowadzaja sie pierwsze (Wprowadzaj), wiec miejsca dla nowych to nadwyzka.
	return FMath::Max(0, N - WNamiotach());
}

FString AOsada::OpisZadowolenia() const
{
	float J = 0.f, D = 0.f, W = 0.f, C = 0.f, U = 0.f, Wyg = 0.f;
	CelZadowolenia(&J, &D, &W, &C, &U, &Wyg);
	const int32 N = Rodziny.Num();
	const int32 G = Glodnych();
	int32 Zimne = 0;
	for (const FRodzina& F : Rodziny) { Zimne += F.bCieplo ? 0 : 1; }
	FString T = FString::Printf(TEXT("Zadowolenie: %d%%\n"), Zadowolenie());
	int32 Rozne = 0;
	for (const FRodzina& F : Rodziny) { Rozne += F.Rodzajow >= 2 ? 1 : 0; }
	T += FString::Printf(TEXT("Jedzenie: %.0f z 30 (%s, dwa rodzaje jedzenia: %d z %d rodzin)\n"), J, G == 0 ? TEXT("wszyscy najedzeni") : *FString::Printf(TEXT("głodnych rodzin: %d"), G), Rozne, N);
	T += FString::Printf(TEXT("Ciepło: %.0f z 10 (%s)\n"), C, Pora() != EPora::Zima ? TEXT("nie trzeba palić") : (Zimne == 0 ? TEXT("zimą kłoda na posiłek, wszystkim ciepło") : *FString::Printf(TEXT("marznie rodzin: %d"), Zimne)));
	T += FString::Printf(TEXT("Domy: %.0f z 20 (w domach %d z %d rodzin)\n"), D, N - WNamiotach(), N);
	int32 Obute = 0, ZPiwem = 0, OdKrawca = 0;
	for (const FRodzina& F : Rodziny) { Obute += F.bObuta ? 1 : 0; ZPiwem += F.bPiwo ? 1 : 0; OdKrawca += F.Ubranie == 2 ? 1 : 0; }
	T += FString::Printf(TEXT("Ubranie: %.0f z 10 (ubranych %d z %d rodzin, od krawca %d; sukno daje połowę; nowe co %d posiłki z targu)\n"), U, Ubranych(), N, OdKrawca, PosilkowNaUbranie);
	T += FString::Printf(TEXT("Wygody: %.0f z 10 (buty z targu: %d, piwo w karczmie: %d z %d rodzin)\n"), Wyg, Obute, ZPiwem, N);
	T += FString::Printf(TEXT("Kościół: %.0f z 20\n"), W);
	T += FString::Printf(TEXT("Podatek (%s) i dziesięcina: %+.0f\n"), NazwaPodatku(Podatek), ZPodatkow());
	T += FString::Printf(TEXT("Następny posiłek za %d s (porcja na rodzinę)\n\n"), FMath::CeilToInt32(DoPosilku / FMath::Max(1.f, ABudowa::Tempo)));
	if (Zadowolenie() >= 60)
	{
		T += WolnychMiejsc() > 0
			? FString::Printf(TEXT("Nowa rodzina przyjdzie za %d s."), FMath::CeilToInt32(DoNowej / FMath::Max(1.f, ABudowa::Tempo)))
			: FString(TEXT("Nowe rodziny chcą przyjść, ale nie ma wolnego miejsca w domach."));
	}
	else if (Zadowolenie() >= 30)
	{
		T += TEXT("Od 60% przychodzą nowe rodziny.");
	}
	else
	{
		T += N > 2 ? FString::Printf(TEXT("Poniżej 30%%: za %d s jedna rodzina odejdzie."), FMath::CeilToInt32(DoOdejscia / FMath::Max(1.f, ABudowa::Tempo)))
			: FString(TEXT("Poniżej 30%: rodziny chcą odejść."));
	}
	return T;
}

FString AOsada::NastepnaNazwa()
{
	const int32 K = NumerNazwy++;
	const int32 Ile = UE_ARRAY_COUNT(Ojcowie);
	return K < Ile ? FString::Printf(TEXT("Rodzina %s"), Ojcowie[K]) : FString::Printf(TEXT("Rodzina %s %d"), Ojcowie[K % Ile], K / Ile + 1);
}

void AOsada::NowaRodzina()
{
	// Wolny dom (najblizszy obozu); rodzina przychodzi z daleka (~45 m) i od razu idzie pod niego.
	ABudowa* Dom = nullptr;
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		ABudowa* B = BW.Get();
		if (!B || !B->Gotowy() || B->Rodzaj().Funkcja != EFunkcja::Mieszkalny || B->Rodzaj().Fazy <= 1)
		{
			continue;
		}
		int32 Mieszka = 0;
		for (const FRodzina& F : Rodziny) { Mieszka += F.Dom.Get() == B ? 1 : 0; }
		if (Mieszka < B->Rodzaj().Rodzin)
		{
			Dom = B;
			break;
		}
	}
	if (!Dom)
	{
		return;
	}
	FVector2D Start = SrodekObozu;
	for (int32 Proba = 0; Proba < 12; ++Proba)
	{
		const float Kat = FMath::FRand() * 2.f * PI;
		const FVector2D P = SrodekObozu + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * 4500.f;
		if (FSwiat::Woda(P.X, P.Y) == FSwiat::EWoda::Lad && FSwiat::Wysokosc(P.X, P.Y) > 60.f)
		{
			Start = P;
			break;
		}
	}
	FRodzina F;
	F.Dom = Dom;
	F.Nazwa = NastepnaNazwa();
	F.Maz = NowyLudzik(Start, false);
	F.Zona = NowyLudzik(Start + FVector2D(120.f, 80.f), true);
	const FVector2D Wej = WejscieDo(Dom);
	for (ALudzik* L : { F.Maz.Get(), F.Zona.Get() })
	{
		if (L)
		{
			L->UstawDom(Wej);
		}
	}
	Rodziny.Add(F);
	Ogloszenie(FString::Printf(TEXT("Do osady przybyła %s"), *F.Nazwa));
	UE_LOG(LogTemp, Log, TEXT("Osada: przybyla %s (dom: %s), rodzin: %d"), *F.Nazwa, *Dom->Rodzaj().Nazwa, Rodziny.Num());
}

void AOsada::OdejdzRodzina()
{
	// Odchodzi najpierw rodzina bez pracy, z namiotu, ostatnio przybyla.
	int32 Wybrana = INDEX_NONE, Ocena = -1;
	for (int32 k = 0; k < Rodziny.Num(); ++k)
	{
		const FRodzina& F = Rodziny[k];
		const int32 O = (F.Praca.IsValid() ? 0 : 2) + (F.Dom.IsValid() ? 0 : 1);
		if (O >= Ocena)
		{
			Ocena = O;
			Wybrana = k;
		}
	}
	if (Wybrana == INDEX_NONE)
	{
		return;
	}
	FRodzina F = Rodziny[Wybrana];
	Rodziny.RemoveAt(Wybrana);
	const float Kat = FMath::FRand() * 2.f * PI;
	const FVector2D Daleko = SrodekObozu + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * 5000.f;
	for (const TWeakObjectPtr<ALudzik>& L : { F.Maz, F.Zona })
	{
		if (L.IsValid())
		{
			L->Odejdz(Daleko);
		}
	}
	if (F.Namiot.IsValid())
	{
		F.Namiot->Destroy();
	}
	float J = 0.f, D = 0.f, W = 0.f, C = 0.f;
	CelZadowolenia(&J, &D, &W, &C);
	Ogloszenie(FString::Printf(TEXT("%s opuściła osadę: %s"), *F.Nazwa, J < 20.f ? TEXT("za mało jedzenia") : (C < 5.f ? TEXT("zimno, brak opału") : TEXT("źle im tu"))));
	UE_LOG(LogTemp, Log, TEXT("Osada: odeszla %s, rodzin: %d"), *F.Nazwa, Rodziny.Num());
}

FSurowce AOsada::Grabiez(float Czesc, int32& Gr)
{
	FSurowce Z;
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		ABudowa* B = BW.Get();
		if (!B || !B->Gotowy())
		{
			continue;
		}
		for (ESurowiec S : WszystkieSurowce)
		{
			const int32 N = FMath::CeilToInt32(B->Dostepne(S) * Czesc);
			if (N > 0)
			{
				B->ZmienZapas(S, -N);
				Z[S] += N;
			}
		}
	}
	Gr = FMath::RoundToInt32(Grosze * Czesc);
	Grosze -= Gr;
	return Z;
}

void AOsada::Zwroc(const FSurowce& Z, int32 Gr)
{
	// Do ogniska obozu, a bez niego do pierwszego gotowego magazynu (jak towar trybu deweloperskiego).
	ABudowa* Cel = nullptr;
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		ABudowa* B = BW.Get();
		if (!B || !B->Gotowy() || B->Rodzaj().Funkcja != EFunkcja::Magazyn)
		{
			continue;
		}
		if (&B->Rodzaj() == &FRodzajBudynku::Ognisko())
		{
			Cel = B;
			break;
		}
		Cel = Cel ? Cel : B;
	}
	if (Cel)
	{
		for (ESurowiec S : WszystkieSurowce)
		{
			if (Z[S] > 0) { Cel->ZmienZapas(S, Z[S]); }
		}
	}
	Grosze += Gr;
}

void AOsada::Zwroc(ESurowiec S, int32 Ile)
{
	FSurowce Z;
	Z[S] = Ile;
	Zwroc(Z, 0);
}

int32 AOsada::Wez(ESurowiec S, int32 Ile)
{
	int32 Wziete = 0;
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		ABudowa* B = BW.Get();
		if (!B || !B->Gotowy() || Wziete >= Ile)
		{
			continue;
		}
		const int32 N = FMath::Min(Ile - Wziete, B->Dostepne(S));
		if (N > 0)
		{
			B->ZmienZapas(S, -N);
			Wziete += N;
		}
	}
	return Wziete;
}

TArray<ALudzik*> AOsada::Mezczyzni() const
{
	TArray<ALudzik*> Out;
	for (const TWeakObjectPtr<ALudzik>& L : Ludzie)
	{
		// Poprawka 25.11.2026: bez mezczyzn odchodzacych z osady (powolany znikal z pola, gdy doszedl na miejsce).
		if (L.IsValid() && !L->Kobieta() && !L->WBitwie() && !L->Odchodzi())
		{
			Out.Add(L.Get());
		}
	}
	return Out;
}

void AOsada::Ogloszenie(const FString& Tekst)
{
	OstatniaWiadomosc = Tekst;
	CzasWiadomosci = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

FString AOsada::Wiadomosc() const
{
	return (GetWorld() && GetWorld()->GetTimeSeconds() - CzasWiadomosci < 12.0) ? OstatniaWiadomosc : FString();
}

// --- Etap 3: pory roku, pola, kolor swiata ---

FString AOsada::NapisPory() const
{
	return FString::Printf(TEXT("%s, rok %d"), FKalendarz::Nazwa(Kalendarz.Pora()), Kalendarz.Rok());
}

FString AOsada::OpisPory() const
{
	const EPora P = Kalendarz.Pora();
	FString T = NapisPory() + TEXT("\n");
	switch (P)
	{
	case EPora::Wiosna: T += TEXT("Rolnicy orzą i sieją. Pole nieobsiane do lata nie da plonu.\n"); break;
	case EPora::Lato: T += TEXT("Zboże rośnie, rolnicy pielą grządki.\n"); break;
	case EPora::Jesien: T += TEXT("Żniwa: zbierz zboże przed zimą, bo zgnije na polu.\n"); break;
	default: T += TEXT("Pola odpoczywają, krzaki bez owoców. Każda rodzina pali kłodę na posiłek.\n"); break;
	}
	T += FString::Printf(TEXT("%s za %d s"), FKalendarz::Nazwa(FKalendarz::Nastepna(P)),
		FMath::CeilToInt32(Kalendarz.DoKoncaPory() / FMath::Max(1.f, ABudowa::Tempo)));
	return T;
}

void AOsada::UstawCzas(float Sekundy)
{
	Kalendarz.Czas = FMath::Max(0.f, Sekundy);
	PoraTeraz = Kalendarz.Pora();
}

float AOsada::DoZniw(const ABudowa* B) const
{
	if (!B || B->Uprawa == EUprawa::Dojrzale)
	{
		return 0.f;
	}
	// Do poczatku najblizszej jesieni (sekundy gry, razy tempo).
	const int32 Por = (int32(EPora::Jesien) - int32(Kalendarz.Pora()) + 3) % 4; // pelne pory miedzy obecna a jesienia
	return Kalendarz.DoKoncaPory() + Por * FKalendarz::DlugoscPory;
}

int32 AOsada::SpodziewanyPlon(const ABudowa* B) const
{
	if (!B)
	{
		return 0;
	}
	const int32 Plon = B->Rodzaj().Plon;
	switch (B->Uprawa)
	{
	case EUprawa::Dojrzale: return B->DoZebrania;
	case EUprawa::Rosnie:
	case EUprawa::Zasiane: return FMath::Max(1, FMath::RoundToInt32(Plon * FMath::Clamp(B->Zasiano, 0.f, 1.f)));
	case EUprawa::Zaorane:
		if (Pora() == EPora::Wiosna)
		{
			return Plon; // siew trwa: pelny plon, jesli rolnicy zdaza
		}
		return B->Zasiano >= 0.05f ? FMath::Max(1, FMath::RoundToInt32(Plon * B->Zasiano)) : 0;
	default: return Pora() == EPora::Lato ? 0 : Plon; // ugor latem nic nie da; poza tym - nastepne zniwa
	}
}

float AOsada::OrkaPola(const FRodzajBudynku& R)
{
	return CzasOrki * R.CzynnikObszaru;
}

float AOsada::SiewPola(const FRodzajBudynku& R)
{
	return CzasSiewu * R.CzynnikObszaru;
}

void AOsada::PracujNaPolu(ABudowa* B, float Sekundy)
{
	if (!B || Pora() != EPora::Wiosna)
	{
		return;
	}
	// 21.11.2026: praca rosnie z wielkoscia pola; niedokonczony siew daje czesc plonu (Zasiano).
	const float Orka = OrkaPola(B->Rodzaj()), Siew = SiewPola(B->Rodzaj());
	B->PracaPola += Sekundy * ABudowa::Tempo;
	if (B->Uprawa == EUprawa::Ugor && B->PracaPola >= Orka)
	{
		B->Zasiano = 0.f;
		B->UstawUprawe(EUprawa::Zaorane);
	}
	else if (B->Uprawa == EUprawa::Zaorane)
	{
		B->Zasiano = FMath::Clamp((B->PracaPola - Orka) / FMath::Max(1.f, Siew), 0.f, 1.f);
		if (B->PracaPola >= Orka + Siew)
		{
			B->Zasiano = 1.f;
			B->UstawUprawe(EUprawa::Zasiane);
		}
	}
}

void AOsada::NowaPora(EPora P)
{
	PobierzPodatki();
	int32 Zgnilo = 0;
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		ABudowa* B = BW.Get();
		if (!B || !B->Gotowy() || B->Rodzaj().Funkcja != EFunkcja::Pole)
		{
			continue;
		}
		switch (P)
		{
		case EPora::Lato:
			// Obsiane w czesci (duze pole, malo rolnikow) tez wschodzi - plon wedlug obsianej czesci.
			B->UstawWzrost(0.f);
			if (B->Uprawa == EUprawa::Zasiane || (B->Uprawa == EUprawa::Zaorane && B->Zasiano >= 0.05f)) { B->UstawUprawe(EUprawa::Rosnie); }
			break;
		case EPora::Jesien:
			if (B->Uprawa == EUprawa::Rosnie)
			{
				B->DoZebrania = FMath::Max(1, FMath::RoundToInt32(B->Rodzaj().Plon * FMath::Clamp(B->Zasiano, 0.f, 1.f)));
				B->UstawUprawe(EUprawa::Dojrzale);
			}
			break;
		case EPora::Zima:
			if (B->Uprawa == EUprawa::Dojrzale && B->DoZebrania > 0)
			{
				Zgnilo += B->DoZebrania;
				B->DoZebrania = 0;
				B->UstawUprawe(EUprawa::Scierniska);
			}
			break;
		default: // wiosna: od nowa
			B->DoZebrania = 0;
			B->PracaPola = 0.f;
			B->UstawUprawe(EUprawa::Ugor);
			break;
		}
	}
	switch (P)
	{
	case EPora::Wiosna: Ogloszenie(FString::Printf(TEXT("Wiosna, rok %d: czas orać i siać"), Kalendarz.Rok())); break;
	case EPora::Lato: Ogloszenie(TEXT("Lato: zboże rośnie")); break;
	case EPora::Jesien: Ogloszenie(TEXT("Jesień: zboże dojrzało, czas żniw")); break;
	default:
		Ogloszenie(Zgnilo > 0 ? FString::Printf(TEXT("Zima: niezebrane zboże zgniło na polach (%s)"), *IleSurowca(ESurowiec::Zboze, Zgnilo))
			: FString(TEXT("Zima: pola odpoczywają, rodziny palą w piecach")));
		break;
	}
	UE_LOG(LogTemp, Log, TEXT("Osada: nowa pora roku: %s (rok %d)"), FKalendarz::Nazwa(P), Kalendarz.Rok());
}

void AOsada::KolorPory()
{
	if (!bKorektaSzukana)
	{
		// Post process obozu (bez granic) z SwiatGameMode; wartosci bazowe zapamietane, pory roku je mnoza.
		bKorektaSzukana = true;
		for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
		{
			if (It->bUnbound)
			{
				Korekta = *It;
				BazaNasycenia = It->Settings.bOverride_ColorSaturation ? It->Settings.ColorSaturation : FVector4(1.f, 1.f, 1.f, 1.f);
				BazaEkspozycji = It->Settings.AutoExposureBias;
				break;
			}
		}
	}
	APostProcessVolume* V = Korekta.Get();
	if (!V)
	{
		return;
	}
	// Ostatnie 20% pory: plynne przejscie do koloru nastepnej.
	const EPora P = Kalendarz.Pora();
	const float A = FMath::SmoothStep(0.8f, 1.f, Kalendarz.UlamekPory());
	const FKolorPory& K0 = KoloryPor[int32(P)];
	const FKolorPory& K1 = KoloryPor[int32(FKalendarz::Nastepna(P))];
	const float Nas = FMath::Lerp(K0.Nasycenie, K1.Nasycenie, A);
	FPostProcessSettings& S = V->Settings;
	S.bOverride_ColorSaturation = true;
	S.ColorSaturation = FVector4(BazaNasycenia.X * Nas, BazaNasycenia.Y * Nas, BazaNasycenia.Z * Nas, BazaNasycenia.W);
	S.bOverride_WhiteTemp = true;
	S.WhiteTemp = FMath::Lerp(K0.Temperatura, K1.Temperatura, A);
	S.bOverride_ColorGain = true;
	const FVector G = FMath::Lerp(K0.Wzmocnienie, K1.Wzmocnienie, A);
	S.ColorGain = FVector4(G.X, G.Y, G.Z, 1.f);
	S.bOverride_AutoExposureBias = true;
	S.AutoExposureBias = BazaEkspozycji + FMath::Lerp(K0.Ekspozycja, K1.Ekspozycja, A);
}

void AOsada::DodajSurowce(int32 Ile)
{
	// Tryb deweloperski (UI/PanelDeweloperski): towar ladowany do ogniska obozu, a bez niego do pierwszego gotowego magazynu.
	ABudowa* Cel = nullptr;
	for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
	{
		ABudowa* B = BW.Get();
		if (!B || !B->Gotowy() || B->Rodzaj().Funkcja != EFunkcja::Magazyn)
		{
			continue;
		}
		if (&B->Rodzaj() == &FRodzajBudynku::Ognisko())
		{
			Cel = B;
			break;
		}
		if (!Cel)
		{
			Cel = B;
		}
	}
	if (!Cel)
	{
		Ogloszenie(TEXT("Tryb deweloperski: brak obozu ani magazynu na towar"));
		return;
	}
	for (ESurowiec S : WszystkieSurowce)
	{
		Cel->ZmienZapas(S, Ile);
	}
	Grosze += Ile;
	Ogloszenie(FString::Printf(TEXT("Tryb deweloperski: +%d każdego surowca i +%d groszy (%s)"), Ile, Ile, *Cel->Rodzaj().Nazwa));
}

// Podatki i dziesiecina (paczka A, 23.11.2026).
const TCHAR* AOsada::NazwaPodatku(int32 P)
{
	static const TCHAR* Nazwy[4] = { TEXT("brak"), TEXT("niski"), TEXT("średni"), TEXT("wysoki") };
	return Nazwy[FMath::Clamp(P, 0, 3)];
}

float AOsada::ZPodatkow() const
{
	static const float Wplyw[4] = { 5.f, 0.f, -6.f, -14.f };
	return Wplyw[FMath::Clamp(Podatek, 0, 3)] + (bDziesiecinaOddana ? 5.f : 0.f);
}

void AOsada::PobierzPodatki()
{
	// Rodzina w domu placi pelna stawke, w namiocie polowe (w gore) - po sprawdzeniu x10 sam dom dawal ledwie grosz.
	int32 WDomu = 0;
	for (const FRodzina& F : Rodziny)
	{
		WDomu += F.Dom.IsValid() ? 1 : 0;
	}
	const int32 Placi = Rodziny.Num();
	const int32 P = FMath::Clamp(Podatek, 0, 3);
	OstatniPodatek = WDomu * P + ((Placi - WDomu) * P + 1) / 2;
	Grosze += OstatniPodatek;

	OstatniaDziesiecina = 0;
	bDziesiecinaOddana = false;
	bool bKosciol = false;
	for (const TWeakObjectPtr<ABudowa>& B : Budynki)
	{
		bKosciol |= B.IsValid() && B->Gotowy() && B->Rodzaj().Religia();
	}
	if (bDziesiecina && bKosciol)
	{
		int32 Suma = 0;
		for (const TWeakObjectPtr<ABudowa>& B : Budynki)
		{
			Suma += B.IsValid() && B->Gotowy() && B->Rodzaj().Magazynuje() ? B->Dostepne(ESurowiec::Jedzenie) : 0;
		}
		int32 Reszta = (Suma + 9) / 10;
		for (const TWeakObjectPtr<ABudowa>& BW : Budynki)
		{
			ABudowa* B = BW.Get();
			if (Reszta <= 0 || !B || !B->Gotowy() || !B->Rodzaj().Magazynuje())
			{
				continue;
			}
			const int32 N = FMath::Min(Reszta, B->Dostepne(ESurowiec::Jedzenie));
			B->ZmienZapas(ESurowiec::Jedzenie, -N);
			Reszta -= N;
			OstatniaDziesiecina += N;
		}
		bDziesiecinaOddana = OstatniaDziesiecina > 0;
	}
	UE_LOG(LogTemp, Log, TEXT("Osada: podatek %s: +%d groszy od %d rodzin, dziesiecina %d jedzenia"), NazwaPodatku(Podatek), OstatniPodatek, Placi, OstatniaDziesiecina);
}

FString AOsada::OpisPodatkow() const
{
	FString T = FString::Printf(TEXT("Podatek: %s (kliknij, by zmienić)\nCo porę roku każda rodzina w domu płaci %d gr, rodzina w namiocie połowę. Zadowolenie: brak +5, niski 0, średni -6, wysoki -14.\n"),
		NazwaPodatku(Podatek), FMath::Clamp(Podatek, 0, 3));
	T += FString::Printf(TEXT("Ostatnio: +%d gr"), OstatniPodatek);
	T += FString::Printf(TEXT("\n\nDziesięcina: %s\nCo porę roku dziesiąta część jedzenia z magazynów idzie na kościół; gdy oddana, +5 zadowolenia. Bez kościoła nic nie daje.\nOstatnio oddano: %d jedzenia"),
		bDziesiecina ? TEXT("tak") : TEXT("nie"), OstatniaDziesiecina);
	return T;
}
