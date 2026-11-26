// Plik: Source/grarts2d/Gra/ZapisGry.h
// Zapis gry (26.11.2026, uwaga uzytkownika po zagraniu: "nie ma zapisu gry ani slotow"). Trzy sloty i autozapis (slot 0, co 5 min),
// pliki JSON w Saved/Zapisy/Slot<N>.json. Zapisuje osade gracza (oboz, budynki z postepem, zapasami, uprawa, przebudowa, rodziny
// z domem, namiotem i praca, skarbiec, podatki, kalendarz, zadowolenie, poziom), drogi, polany i sadzonki terenu, rody AI
// i kamere. Nie zapisuje: trwajacej bitwy i oddzialow ruszenia, ognia, ruin i tego, co ludzie robia w danej chwili
// (po wczytaniu sami biora prace od nowa).
// Wczytanie przeladowuje poziom (czysty swiat), SwiatGameMode wchodzi od razu do regionu w miejscu kamery z zapisu,
// a ABudowniczy po wczytaniu terenu wola Zastosuj.
#pragma once

#include "CoreMinimal.h"

class UWorld;

struct GRARTS2D_API FZapisGry
{
	/** Sloty gracza 1..Slotow; slot 0 = autozapis. */
	static constexpr int32 Slotow = 3;
	static constexpr float CoIleAutozapis = 300.f;
	/** Slot czekajacy na wczytanie po przeladowaniu poziomu (-1 = nic). Tez z linii polecen: -wczytaj=N. */
	static int32 DoWczytania;

	static FString Plik(int32 Slot);
	static bool Istnieje(int32 Slot);
	/** "Lato, rok 2 · 12 rodzin · wieś — 26.11.2026 14:05"; pusty napis = brak zapisu. */
	static FString Opis(int32 Slot);
	static bool JakisZapis();

	static bool Zapisz(UWorld* W, int32 Slot, FString& Blad);
	/** Przeladowuje poziom i wczytuje slot (dziala z menu startowego i z gry). */
	static bool Wczytaj(UWorld* W, int32 Slot, FString& Blad);
	/** Dla SwiatGameMode: gdzie wejsc do regionu, gdy czeka wczytanie. */
	static bool KameraStartu(FVector2D& Out);
	/** ABudowniczy, gdy teren regionu jest gotowy: odtwarza stan z czekajacego slotu. */
	static void Zastosuj(UWorld* W);
};
