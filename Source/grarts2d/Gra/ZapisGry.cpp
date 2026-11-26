// Plik: Source/grarts2d/Gra/ZapisGry.cpp
#include "Gra/ZapisGry.h"
#include "Osada/Osada.h"
#include "Osada/Rody.h"
#include "Osada/Drogi.h"
#include "Budowy/Budowa.h"
#include "Ludzie/Ludzik.h"
#include "Swiat/TerenKafle.h"
#include "Kamera/KameraMapy.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"

int32 FZapisGry::DoWczytania = -1;

namespace
{
	using FObj = TSharedPtr<FJsonObject>;
	using FTab = TArray<TSharedPtr<FJsonValue>>;
	constexpr int32 WersjaZapisu = 1;

	template <typename T>
	T* Znajdz(UWorld* W)
	{
		for (TActorIterator<T> It(W); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	void Dodaj(FTab& T, double V) { T.Add(MakeShared<FJsonValueNumber>(V)); }
	double Num(const FObj& O, const TCHAR* P, double Domyslnie = 0.0)
	{
		double V = Domyslnie;
		return O.IsValid() && O->TryGetNumberField(P, V) ? V : Domyslnie;
	}
	bool Tak(const FObj& O, const TCHAR* P, bool Domyslnie = false)
	{
		bool V = Domyslnie;
		return O.IsValid() && O->TryGetBoolField(P, V) ? V : Domyslnie;
	}
	FString Napis(const FObj& O, const TCHAR* P)
	{
		FString V;
		if (O.IsValid())
		{
			O->TryGetStringField(P, V);
		}
		return V;
	}
	const FTab& Tablica(const FObj& O, const TCHAR* P)
	{
		static const FTab Pusta;
		const FTab* T = nullptr;
		return O.IsValid() && O->TryGetArrayField(P, T) && T ? *T : Pusta;
	}
	FObj Obiekt(const FObj& O, const TCHAR* P)
	{
		const FObj* V = nullptr;
		return O.IsValid() && O->TryGetObjectField(P, V) && V ? *V : FObj();
	}
	double Element(const FTab& T, int32 I) { return T.IsValidIndex(I) && T[I].IsValid() ? T[I]->AsNumber() : 0.0; }

	FTab Surowce(const FSurowce& S)
	{
		FTab T;
		for (int32 i = 0; i < int32(ESurowiec::Ile); ++i)
		{
			Dodaj(T, S.Ile[i]);
		}
		return T;
	}
	FSurowce Surowce(const FObj& O, const TCHAR* P)
	{
		FSurowce S;
		const FTab& T = Tablica(O, P);
		for (int32 i = 0; i < FMath::Min(T.Num(), int32(ESurowiec::Ile)); ++i)
		{
			S.Ile[i] = int32(Element(T, i));
		}
		return S;
	}

	/** Wszystkie rodzaje budynkow gracza: pasek budowy, ich kolejne poziomy, namiot i ognisko. */
	const TArray<const FRodzajBudynku*>& Rodzaje()
	{
		static TArray<const FRodzajBudynku*> L;
		if (L.Num() == 0)
		{
			TArray<const FRodzajBudynku*> Start = FRodzajBudynku::DoBudowania();
			Start.Add(&FRodzajBudynku::Namiot());
			Start.Add(&FRodzajBudynku::Ognisko());
			Start.Add(&FRodzajBudynku::Kosciol());
			for (const FRodzajBudynku* R : Start)
			{
				for (int32 k = 0; R && k < 8; ++k)
				{
					L.AddUnique(R);
					R = R->Ulepszenie ? &R->Ulepszenie() : nullptr;
				}
			}
		}
		return L;
	}
	const FRodzajBudynku* RodzajPo(const FString& Model, const FString& Nazwa)
	{
		const FRodzajBudynku* Sam = nullptr;
		for (const FRodzajBudynku* R : Rodzaje())
		{
			if (R->Model == Model)
			{
				if (R->Nazwa == Nazwa)
				{
					return R;
				}
				Sam = Sam ? Sam : R;
			}
		}
		return Sam;
	}
	const FRodzajBudynku* Poprzedni(const FRodzajBudynku& R)
	{
		for (const FRodzajBudynku* P : Rodzaje())
		{
			if (P->Ulepszenie && P->Ulepszenie().Model == R.Model)
			{
				return P;
			}
		}
		return nullptr;
	}

	FObj CzytajPlik(int32 Slot)
	{
		FString Tekst;
		if (!FFileHelper::LoadFileToString(Tekst, *FZapisGry::Plik(Slot)))
		{
			return FObj();
		}
		FObj Root;
		const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Tekst);
		if (!FJsonSerializer::Deserialize(R, Root) || !Root.IsValid())
		{
			return FObj();
		}
		return Root;
	}

	// Rody: pola FRod po nazwie (makra trzymaja zapis i odczyt w tej samej kolejnosci).
#define ROD_LICZBY(X) X(OdlegloscKm) X(Rodzin) X(Grosze) X(Buduje) X(DoKonca) X(Woje) X(Zadowolenie) X(Stosunek) X(ZadaDaniny) \
	X(CzasZadania) X(DoPosilku) X(DoRodziny) X(BezPostepu) X(NajRodzin) X(NajBudynkow) X(NajWojow) X(Zastojow) X(Decyzje) \
	X(Napadow) X(Napadniety) X(Napastnik) X(GroziGraczowi) X(Mury)
#define ROD_FLAGI(X) X(bLennik) X(bWrogi) X(bZastoj) X(bObrona) X(bGlodny) X(bWojna)

	FObj ZapiszRod(const FRod& R)
	{
		FObj J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("Nazwa"), R.Nazwa);
		J->SetStringField(TEXT("Ostatnio"), R.Ostatnio);
		J->SetStringField(TEXT("Dlaczego"), R.Dlaczego);
		FTab Barwa;
		Dodaj(Barwa, R.Barwa.R); Dodaj(Barwa, R.Barwa.G); Dodaj(Barwa, R.Barwa.B); Dodaj(Barwa, R.Barwa.A);
		J->SetArrayField(TEXT("Barwa"), Barwa);
		J->SetNumberField(TEXT("Cecha"), int32(R.Cecha));
		J->SetNumberField(TEXT("Poziom"), int32(R.Poziom));
		J->SetNumberField(TEXT("X"), R.Gdzie.X);
		J->SetNumberField(TEXT("Y"), R.Gdzie.Y);
		J->SetArrayField(TEXT("Zapas"), Surowce(R.Zapas));
		FTab Bud, Post, Wobec;
		for (int32 V : R.Budynkow) { Dodaj(Bud, V); }
		for (float V : R.PostepBud) { Dodaj(Post, V); }
		for (float V : R.Wobec) { Dodaj(Wobec, V); }
		J->SetArrayField(TEXT("Budynkow"), Bud);
		J->SetArrayField(TEXT("PostepBud"), Post);
		J->SetArrayField(TEXT("Wobec"), Wobec);
#define ZAP_N(P) J->SetNumberField(TEXT(#P), double(R.P));
#define ZAP_B(P) J->SetBoolField(TEXT(#P), R.P);
		ROD_LICZBY(ZAP_N)
		ROD_FLAGI(ZAP_B)
#undef ZAP_N
#undef ZAP_B
		return J;
	}

	FRod WczytajRod(const FObj& J)
	{
		FRod R;
		R.Nazwa = Napis(J, TEXT("Nazwa"));
		R.Ostatnio = Napis(J, TEXT("Ostatnio"));
		R.Dlaczego = Napis(J, TEXT("Dlaczego"));
		const FTab& Barwa = Tablica(J, TEXT("Barwa"));
		R.Barwa = FLinearColor(Element(Barwa, 0), Element(Barwa, 1), Element(Barwa, 2), Barwa.Num() > 3 ? Element(Barwa, 3) : 1.0);
		R.Cecha = ECechaRodu(int32(Num(J, TEXT("Cecha"))));
		R.Poziom = EPoziomOsady(int32(Num(J, TEXT("Poziom"))));
		R.Gdzie = FVector2D(Num(J, TEXT("X")), Num(J, TEXT("Y")));
		R.Zapas = Surowce(J, TEXT("Zapas"));
		for (const TSharedPtr<FJsonValue>& V : Tablica(J, TEXT("Budynkow"))) { R.Budynkow.Add(int32(V->AsNumber())); }
		for (const TSharedPtr<FJsonValue>& V : Tablica(J, TEXT("PostepBud"))) { R.PostepBud.Add(float(V->AsNumber())); }
		for (const TSharedPtr<FJsonValue>& V : Tablica(J, TEXT("Wobec"))) { R.Wobec.Add(float(V->AsNumber())); }
#define WCZ_N(P) R.P = static_cast<decltype(R.P)>(Num(J, TEXT(#P), double(R.P)));
#define WCZ_B(P) R.P = Tak(J, TEXT(#P), R.P);
		ROD_LICZBY(WCZ_N)
		ROD_FLAGI(WCZ_B)
#undef WCZ_N
#undef WCZ_B
		return R;
	}
}

FString FZapisGry::Plik(int32 Slot)
{
	return FPaths::ProjectSavedDir() / TEXT("Zapisy") / FString::Printf(TEXT("Slot%d.json"), Slot);
}

bool FZapisGry::Istnieje(int32 Slot)
{
	return IFileManager::Get().FileExists(*Plik(Slot));
}

bool FZapisGry::JakisZapis()
{
	for (int32 s = 0; s <= Slotow; ++s)
	{
		if (Istnieje(s))
		{
			return true;
		}
	}
	return false;
}

FString FZapisGry::Opis(int32 Slot)
{
	if (!Istnieje(Slot))
	{
		return FString();
	}
	const FObj Root = CzytajPlik(Slot);
	if (!Root.IsValid())
	{
		return TEXT("uszkodzony plik zapisu");
	}
	return FString::Printf(TEXT("%s — %s"), *Napis(Root, TEXT("Opis")), *Napis(Root, TEXT("Data")));
}

bool FZapisGry::Zapisz(UWorld* W, int32 Slot, FString& Blad)
{
	AOsada* O = W ? Znajdz<AOsada>(W) : nullptr;
	if (!O || !O->bOboz)
	{
		Blad = TEXT("Nie ma czego zapisać: najpierw załóż obóz na mapie grywalnej.");
		return false;
	}
	FObj Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("Wersja"), WersjaZapisu);
	Root->SetStringField(TEXT("Data"), FDateTime::Now().ToString(TEXT("%d.%m.%Y %H:%M")));
	Root->SetStringField(TEXT("Opis"), FString::Printf(TEXT("%s · %d rodzin · %s"), *O->NapisPory(), O->Rodzin(), *O->NapisPoziomu()));

	// Kamera.
	if (APlayerController* PC = W->GetFirstPlayerController())
	{
		if (AKameraMapy* K = Cast<AKameraMapy>(PC->GetPawn()))
		{
			FObj J = MakeShared<FJsonObject>();
			J->SetNumberField(TEXT("X"), K->Focus.X);
			J->SetNumberField(TEXT("Y"), K->Focus.Y);
			J->SetNumberField(TEXT("Yaw"), K->CelYaw);
			J->SetNumberField(TEXT("Odl"), K->CelOdl);
			J->SetBoolField(TEXT("Region"), K->WRegionie());
			Root->SetObjectField(TEXT("Kamera"), J);
		}
	}

	// Osada gracza.
	FObj Os = MakeShared<FJsonObject>();
	Os->SetNumberField(TEXT("X"), O->SrodekObozu.X);
	Os->SetNumberField(TEXT("Y"), O->SrodekObozu.Y);
	Os->SetNumberField(TEXT("Grosze"), O->Grosze);
	Os->SetNumberField(TEXT("Podatek"), O->Podatek);
	Os->SetBoolField(TEXT("Dziesiecina"), O->bDziesiecina);
	Os->SetBoolField(TEXT("DziesiecinaOddana"), O->bDziesiecinaOddana);
	Os->SetNumberField(TEXT("OstatniPodatek"), O->OstatniPodatek);
	Os->SetNumberField(TEXT("OstatniaDziesiecina"), O->OstatniaDziesiecina);
	Os->SetNumberField(TEXT("Zadowolenie"), O->ZadowolenieF);
	Os->SetNumberField(TEXT("Poziom"), O->PoziomB);
	Os->SetNumberField(TEXT("Czas"), O->Kalendarz.Czas);
	Os->SetNumberField(TEXT("DoPosilku"), O->DoPosilku);
	Os->SetNumberField(TEXT("DoNowej"), O->DoNowej);
	Os->SetNumberField(TEXT("DoOdejscia"), O->DoOdejscia);
	Os->SetNumberField(TEXT("NumerNazwy"), O->NumerNazwy);
	Os->SetBoolField(TEXT("PierwszyPosilek"), O->bPierwszyPosilek);
	FTab Zerwane;
	for (const FVector& Z : O->Zerwane) { Dodaj(Zerwane, Z.X); Dodaj(Zerwane, Z.Y); Dodaj(Zerwane, Z.Z); }
	Os->SetArrayField(TEXT("Zerwane"), Zerwane);

	TMap<const ABudowa*, int32> Indeks;
	FTab Budynki;
	for (const TWeakObjectPtr<ABudowa>& BW : O->Budynki)
	{
		const ABudowa* B = BW.Get();
		if (!B || B->bPodgladB || B->Ruina() || B->Zgliszcza())
		{
			continue;
		}
		const FRodzajBudynku& R = B->Rodzaj();
		Indeks.Add(B, Indeks.Num());
		FObj J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("M"), R.Model);
		J->SetStringField(TEXT("N"), R.Nazwa);
		const FVector L = B->GetActorLocation();
		J->SetNumberField(TEXT("X"), L.X);
		J->SetNumberField(TEXT("Y"), L.Y);
		J->SetNumberField(TEXT("Z"), L.Z - B->ZOpuszczenie);
		J->SetNumberField(TEXT("Obr"), B->GetActorRotation().Yaw);
		J->SetNumberField(TEXT("W"), B->WariantB);
		J->SetNumberField(TEXT("P"), B->PostepBudowy);
		J->SetBoolField(TEXT("U"), B->bUlepszanie);
		J->SetNumberField(TEXT("OX"), R.Obszar.X);
		J->SetNumberField(TEXT("OY"), R.Obszar.Y);
		if (R.ZWielokatem())
		{
			FTab Wk; // rogi plotu w swiecie: po wczytaniu FRodzajBudynku::NaWielokat liczy z nich to samo
			for (const FVector2D& Lw : R.Wielokat)
			{
				const FVector2D S = B->ZUkladu(Lw);
				Dodaj(Wk, S.X);
				Dodaj(Wk, S.Y);
			}
			J->SetArrayField(TEXT("Wk"), Wk);
		}
		J->SetBoolField(TEXT("Darmo"), R.Koszt.Puste());
		J->SetArrayField(TEXT("Zapas"), Surowce(B->Zapas));
		J->SetArrayField(TEXT("Dost"), Surowce(B->Dostarczone));
		J->SetNumberField(TEXT("Prio"), B->Priorytet);
		J->SetNumberField(TEXT("Pkt"), B->PunktyB);
		J->SetNumberField(TEXT("Upr"), int32(B->Uprawa));
		J->SetNumberField(TEXT("Zas"), B->Zasiano);
		J->SetNumberField(TEXT("Pp"), B->PracaPola);
		J->SetNumberField(TEXT("DoZ"), B->DoZebrania);
		J->SetNumberField(TEXT("Wz"), B->Wzrost);
		Budynki.Add(MakeShared<FJsonValueObject>(J));
	}
	Os->SetArrayField(TEXT("Budynki"), Budynki);

	auto Ktory = [&Indeks](const TWeakObjectPtr<ABudowa>& B) -> int32
	{
		const int32* I = B.IsValid() ? Indeks.Find(B.Get()) : nullptr;
		return I ? *I : -1;
	};
	FTab Rodziny;
	for (const AOsada::FRodzina& F : O->Rodziny)
	{
		FObj J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("Nazwa"), F.Nazwa);
		J->SetBoolField(TEXT("Maz"), F.Maz.IsValid());
		J->SetBoolField(TEXT("Zona"), F.Zona.IsValid());
		J->SetNumberField(TEXT("Namiot"), Ktory(F.Namiot));
		J->SetNumberField(TEXT("Dom"), Ktory(F.Dom));
		J->SetNumberField(TEXT("Praca"), Ktory(F.Praca));
		J->SetBoolField(TEXT("Najedzona"), F.bNajedzona);
		J->SetBoolField(TEXT("Cieplo"), F.bCieplo);
		J->SetBoolField(TEXT("Ubrana"), F.bUbrana);
		J->SetNumberField(TEXT("DoUbrania"), F.DoUbrania);
		J->SetNumberField(TEXT("Rodzajow"), F.Rodzajow);
		J->SetNumberField(TEXT("Ubranie"), F.Ubranie);
		J->SetBoolField(TEXT("Obuta"), F.bObuta);
		J->SetBoolField(TEXT("Piwo"), F.bPiwo);
		J->SetNumberField(TEXT("DoPiwa"), F.DoPiwa);
		Rodziny.Add(MakeShared<FJsonValueObject>(J));
	}
	Os->SetArrayField(TEXT("Rodziny"), Rodziny);
	Root->SetObjectField(TEXT("Osada"), Os);

	// Teren: wykarczowane kola (budynki, drogi, sciete drzewa) i sadzonki borowych.
	if (ATerenKafle* T = Znajdz<ATerenKafle>(W))
	{
		FObj J = MakeShared<FJsonObject>();
		FTab Polany, Sadzonki;
		for (const FVector& P : T->Polany) { Dodaj(Polany, FMath::RoundToDouble(P.X)); Dodaj(Polany, FMath::RoundToDouble(P.Y)); Dodaj(Polany, FMath::RoundToDouble(P.Z)); }
		for (const ATerenKafle::FSadzonka& S : T->Sadzonki)
		{
			Dodaj(Sadzonki, S.P.X); Dodaj(Sadzonki, S.P.Y); Dodaj(Sadzonki, S.Wiek); Dodaj(Sadzonki, S.Skala); Dodaj(Sadzonki, S.Obrot); Dodaj(Sadzonki, S.Model);
		}
		J->SetArrayField(TEXT("Polany"), Polany);
		J->SetArrayField(TEXT("Sadzonki"), Sadzonki);
		Root->SetObjectField(TEXT("Teren"), J);
	}

	// Drogi.
	if (ADrogi* D = Znajdz<ADrogi>(W))
	{
		FTab Odcinki;
		for (const ADrogi::FOdcinek& Od : D->Odcinki)
		{
			FTab P;
			for (const FVector2D& Q : Od.P) { Dodaj(P, FMath::RoundToDouble(Q.X)); Dodaj(P, FMath::RoundToDouble(Q.Y)); }
			Odcinki.Add(MakeShared<FJsonValueArray>(P));
		}
		Root->SetArrayField(TEXT("Drogi"), Odcinki);
	}

	// Rody AI.
	if (ARody* Ro = Znajdz<ARody>(W))
	{
		FObj J = MakeShared<FJsonObject>();
		J->SetNumberField(TEXT("Czas"), Ro->Kalendarz.Czas);
		J->SetNumberField(TEXT("Pora"), int32(Ro->OstatniaPora));
		J->SetNumberField(TEXT("DoKroku"), Ro->DoKroku);
		J->SetNumberField(TEXT("DoRoku"), Ro->DoRoku);
		FTab Lista;
		for (const FRod& R : Ro->Rody)
		{
			Lista.Add(MakeShared<FJsonValueObject>(ZapiszRod(R)));
		}
		J->SetArrayField(TEXT("Lista"), Lista);
		Root->SetObjectField(TEXT("Rody"), J);
	}

	FString Tekst;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Pis = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Tekst);
	if (!FJsonSerializer::Serialize(Root.ToSharedRef(), Pis))
	{
		Blad = TEXT("Nie udało się przygotować zapisu.");
		return false;
	}
	// Najpierw plik tymczasowy: przerwany zapis nie psuje starego slotu.
	const FString Cel = Plik(Slot), Tmp = Cel + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Tekst, *Tmp, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) || !IFileManager::Get().Move(*Cel, *Tmp, true))
	{
		Blad = FString::Printf(TEXT("Nie udało się zapisać pliku %s."), *Cel);
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("Zapis gry: slot %d, %d budynkow, %d rodzin, %d KB"), Slot, Indeks.Num(), O->Rodziny.Num(), Tekst.Len() / 1024);
	return true;
}

bool FZapisGry::Wczytaj(UWorld* W, int32 Slot, FString& Blad)
{
	if (!W || !Istnieje(Slot) || !CzytajPlik(Slot).IsValid())
	{
		Blad = TEXT("Ten zapis nie istnieje albo jest uszkodzony.");
		return false;
	}
	DoWczytania = Slot;
	UGameplayStatics::OpenLevel(W, FName(*UGameplayStatics::GetCurrentLevelName(W, true)));
	return true;
}

bool FZapisGry::KameraStartu(FVector2D& Out)
{
	if (DoWczytania < 0)
	{
		return false;
	}
	const FObj Root = CzytajPlik(DoWczytania);
	if (!Root.IsValid())
	{
		DoWczytania = -1;
		return false;
	}
	const FObj K = Obiekt(Root, TEXT("Kamera"));
	const FObj Os = Obiekt(Root, TEXT("Osada"));
	Out = K.IsValid() && Tak(K, TEXT("Region"), true) ? FVector2D(Num(K, TEXT("X")), Num(K, TEXT("Y"))) : FVector2D(Num(Os, TEXT("X")), Num(Os, TEXT("Y")));
	return true;
}

void FZapisGry::Zastosuj(UWorld* W)
{
	if (DoWczytania < 0 || !W)
	{
		return;
	}
	const int32 Slot = DoWczytania;
	DoWczytania = -1;
	const FObj Root = CzytajPlik(Slot);
	AOsada* O = Znajdz<AOsada>(W);
	if (!Root.IsValid() || !O)
	{
		return;
	}

	// Teren najpierw: polany wycinaja drzewa z gotowych kafli, potem sadzonki (Wykarczuj usuwa sadzonki w kole).
	ATerenKafle* T = Znajdz<ATerenKafle>(W);
	const FObj Te = Obiekt(Root, TEXT("Teren"));
	if (T && Te.IsValid())
	{
		const FTab& P = Tablica(Te, TEXT("Polany"));
		for (int32 i = 0; i + 2 < P.Num(); i += 3)
		{
			T->Wykarczuj(FVector2D(Element(P, i), Element(P, i + 1)), float(Element(P, i + 2)));
		}
		const FTab& S = Tablica(Te, TEXT("Sadzonki"));
		T->Sadzonki.Reset();
		for (int32 i = 0; i + 5 < S.Num(); i += 6)
		{
			ATerenKafle::FSadzonka N;
			N.P = FVector2D(Element(S, i), Element(S, i + 1));
			N.Wiek = float(Element(S, i + 2));
			N.Skala = float(Element(S, i + 3));
			N.Obrot = float(Element(S, i + 4));
			N.Model = int32(Element(S, i + 5));
			T->Sadzonki.Add(N);
		}
		for (int32 m = 0; m < T->Modele.Num(); ++m)
		{
			T->OdbudujMlode(m);
		}
	}

	// Osada.
	const FObj Os = Obiekt(Root, TEXT("Osada"));
	O->SrodekObozu = FVector2D(Num(Os, TEXT("X")), Num(Os, TEXT("Y")));
	O->bOboz = true;
	O->Grosze = int32(Num(Os, TEXT("Grosze"), O->Grosze));
	O->Podatek = int32(Num(Os, TEXT("Podatek"), O->Podatek));
	O->bDziesiecina = Tak(Os, TEXT("Dziesiecina"));
	O->bDziesiecinaOddana = Tak(Os, TEXT("DziesiecinaOddana"));
	O->OstatniPodatek = int32(Num(Os, TEXT("OstatniPodatek")));
	O->OstatniaDziesiecina = int32(Num(Os, TEXT("OstatniaDziesiecina")));
	O->ZadowolenieF = float(Num(Os, TEXT("Zadowolenie"), 50.0));
	O->PoziomB = uint8(Num(Os, TEXT("Poziom")));
	O->Kalendarz.Czas = float(Num(Os, TEXT("Czas")));
	O->PoraTeraz = O->Kalendarz.Pora();
	O->DoPosilku = float(Num(Os, TEXT("DoPosilku"), O->DoPosilku));
	O->DoNowej = float(Num(Os, TEXT("DoNowej"), O->DoNowej));
	O->DoOdejscia = float(Num(Os, TEXT("DoOdejscia"), O->DoOdejscia));
	O->NumerNazwy = int32(Num(Os, TEXT("NumerNazwy")));
	O->bPierwszyPosilek = Tak(Os, TEXT("PierwszyPosilek"));
	const FTab& Zerwane = Tablica(Os, TEXT("Zerwane"));
	for (int32 i = 0; i + 2 < Zerwane.Num(); i += 3)
	{
		O->Zerwane.Add(FVector(Element(Zerwane, i), Element(Zerwane, i + 1), Element(Zerwane, i + 2)));
	}

	TArray<ABudowa*> Lista;
	for (const TSharedPtr<FJsonValue>& V : Tablica(Os, TEXT("Budynki")))
	{
		const FObj J = V->AsObject();
		const FRodzajBudynku* R = RodzajPo(Napis(J, TEXT("M")), Napis(J, TEXT("N")));
		if (!R)
		{
			UE_LOG(LogTemp, Warning, TEXT("Zapis gry: nieznany budynek %s"), *Napis(J, TEXT("M")));
			Lista.Add(nullptr);
			continue;
		}
		FRodzajBudynku Typ = *R;
		const float OX = float(Num(J, TEXT("OX"))), OY = float(Num(J, TEXT("OY")));
		const FTab& Wk = Tablica(J, TEXT("Wk"));
		if (Wk.Num() >= 6)
		{
			TArray<FVector2D> Pkt;
			for (int32 i = 0; i + 1 < Wk.Num(); i += 2)
			{
				Pkt.Add(FVector2D(Element(Wk, i), Element(Wk, i + 1)));
			}
			FVector2D G;
			float Ob = 0.f;
			FRodzajBudynku Nowy;
			if (FRodzajBudynku::NaWielokat(*R, Pkt, Nowy, G, Ob).IsEmpty())
			{
				Typ = Nowy;
			}
		}
		else if (R->ObszarMax.X > 0.f && OX > 0.f)
		{
			Typ = FRodzajBudynku::NaObszar(*R, OX, OY);
		}
		const float Postep = float(Num(J, TEXT("P")));
		// Przebudowa w toku: najpierw gotowy poprzedni poziom, potem ulepszenie (jak przycisk "Ulepsz").
		const FRodzajBudynku* Przed = Tak(J, TEXT("U")) && Postep < 1.f ? Poprzedni(*R) : nullptr;
		ABudowa::NastepnyWariant = int32(Num(J, TEXT("W"), -1.0));
		ABudowa* B = O->Spawnuj(Przed ? *Przed : Typ, FVector2D(Num(J, TEXT("X")), Num(J, TEXT("Y"))), float(Num(J, TEXT("Obr"))), float(Num(J, TEXT("Z"))));
		ABudowa::NastepnyWariant = -1;
		Lista.Add(B);
		if (!B)
		{
			continue;
		}
		if (Przed)
		{
			B->UstawPostep(1.f);
			B->Ulepsz(Typ);
		}
		if (Tak(J, TEXT("Darmo")) && !Typ.Koszt.Puste())
		{
			B->BezKosztu();
		}
		B->Dostarczone = Surowce(J, TEXT("Dost"));
		B->UstawPostep(Postep);
		const FSurowce Zapas = Surowce(J, TEXT("Zapas"));
		for (ESurowiec S : WszystkieSurowce)
		{
			B->ZmienZapas(S, Zapas[S] - B->Zapas[S]);
		}
		B->Priorytet = int32(Num(J, TEXT("Prio"), 3.0));
		B->PunktyB = float(Num(J, TEXT("Pkt"), -1.0));
		B->Zasiano = float(Num(J, TEXT("Zas"), 1.0));
		B->PracaPola = float(Num(J, TEXT("Pp")));
		B->DoZebrania = int32(Num(J, TEXT("DoZ")));
		B->UstawUprawe(EUprawa(int32(Num(J, TEXT("Upr")))));
		B->UstawWzrost(float(Num(J, TEXT("Wz"))));
	}

	for (const TSharedPtr<FJsonValue>& V : Tablica(Os, TEXT("Rodziny")))
	{
		const FObj J = V->AsObject();
		auto Bud = [&Lista, &J](const TCHAR* P) -> ABudowa*
		{
			const int32 I = int32(Num(J, P, -1.0));
			return Lista.IsValidIndex(I) ? Lista[I] : nullptr;
		};
		AOsada::FRodzina F;
		F.Nazwa = Napis(J, TEXT("Nazwa"));
		F.Namiot = Bud(TEXT("Namiot"));
		F.Dom = Bud(TEXT("Dom"));
		F.bNajedzona = Tak(J, TEXT("Najedzona"), true);
		F.bCieplo = Tak(J, TEXT("Cieplo"), true);
		F.bUbrana = Tak(J, TEXT("Ubrana"), true);
		F.DoUbrania = int32(Num(J, TEXT("DoUbrania"), F.DoUbrania));
		F.Rodzajow = int32(Num(J, TEXT("Rodzajow"), 1.0));
		F.Ubranie = uint8(Num(J, TEXT("Ubranie"), 2.0));
		F.bObuta = Tak(J, TEXT("Obuta"), true);
		F.bPiwo = Tak(J, TEXT("Piwo"));
		F.DoPiwa = int32(Num(J, TEXT("DoPiwa"), 2.0));
		FVector2D Wej;
		if (F.Dom.IsValid())
		{
			Wej = O->WejscieDo(F.Dom.Get());
		}
		else if (F.Namiot.IsValid())
		{
			Wej = O->WejscieDo(F.Namiot.Get());
		}
		else
		{
			const float Kat = FMath::FRand() * 2.f * PI;
			Wej = O->SrodekObozu + FVector2D(FMath::Cos(Kat), FMath::Sin(Kat)) * FMath::FRandRange(300.f, 700.f);
		}
		auto Obok = [&Wej]() { return Wej + FVector2D(FMath::FRandRange(-120.f, 120.f), FMath::FRandRange(-120.f, 120.f)); };
		if (Tak(J, TEXT("Maz"), true))
		{
			F.Maz = O->NowyLudzik(Obok(), false);
		}
		if (Tak(J, TEXT("Zona"), true))
		{
			F.Zona = O->NowyLudzik(Obok(), true);
		}
		for (ALudzik* L : { F.Maz.Get(), F.Zona.Get() })
		{
			if (L)
			{
				L->UstawDom(Wej);
			}
		}
		O->Rodziny.Add(F);
		if (ABudowa* Praca = Bud(TEXT("Praca")))
		{
			O->UstawPrace(O->Rodziny.Num() - 1, Praca);
		}
	}

	// Drogi.
	if (ADrogi* D = Znajdz<ADrogi>(W))
	{
		for (const TSharedPtr<FJsonValue>& V : Tablica(Root, TEXT("Drogi")))
		{
			const FTab& P = V->AsArray();
			TArray<FVector2D> Punkty;
			for (int32 i = 0; i + 1 < P.Num(); i += 2)
			{
				Punkty.Add(FVector2D(Element(P, i), Element(P, i + 1)));
			}
			D->DodajOdcinek(Punkty);
		}
	}

	// Rody AI: te same miejsca, budynki i stosunki; osady rodow w terenie stawiaja sie od nowa.
	ARody* Ro = Znajdz<ARody>(W);
	const FObj RJ = Obiekt(Root, TEXT("Rody"));
	if (Ro && RJ.IsValid())
	{
		Ro->Osada = O;
		Ro->Rody.Reset();
		for (const TSharedPtr<FJsonValue>& V : Tablica(RJ, TEXT("Lista")))
		{
			Ro->Rody.Add(WczytajRod(V->AsObject()));
		}
		Ro->Kalendarz.Czas = float(Num(RJ, TEXT("Czas")));
		Ro->OstatniaPora = EPora(int32(Num(RJ, TEXT("Pora"))));
		Ro->DoKroku = float(Num(RJ, TEXT("DoKroku")));
		Ro->DoRoku = float(Num(RJ, TEXT("DoRoku")));
		Ro->OdswiezWidoki();
	}

	// Kamera.
	const FObj K = Obiekt(Root, TEXT("Kamera"));
	APlayerController* PC = W->GetFirstPlayerController();
	if (AKameraMapy* Kam = PC ? Cast<AKameraMapy>(PC->GetPawn()) : nullptr)
	{
		if (K.IsValid() && Tak(K, TEXT("Region"), true))
		{
			Kam->UstawOgnisko(FVector2D(Num(K, TEXT("X")), Num(K, TEXT("Y"))));
			Kam->Yaw = Kam->CelYaw = float(Num(K, TEXT("Yaw"), Kam->CelYaw));
			Kam->Odl = Kam->CelOdl = FMath::Clamp(float(Num(K, TEXT("Odl"), Kam->CelOdl)), Kam->MinOdlRegionu, Kam->MaxOdlRegionu);
		}
		else
		{
			Kam->UstawOgnisko(O->SrodekObozu);
		}
	}
	O->Ogloszenie(FString::Printf(TEXT("Wczytano: %s"), *Napis(Root, TEXT("Opis"))));
	UE_LOG(LogTemp, Log, TEXT("Zapis gry: wczytano slot %d (%d budynkow, %d rodzin)"), Slot, Lista.Num(), O->Rodziny.Num());
}
