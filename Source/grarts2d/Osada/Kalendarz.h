// Plik: Source/grarts2d/Osada/Kalendarz.h
// Pory roku (etap 3 ekonomii, 21.11.2026; plan "Ekonomia gry": "zima - pola nie rodza, a opal sie zuzywa").
// Rok gry zaczyna sie wiosna, w dniu zalozenia obozu. Kazda pora trwa DlugoscPory sekund gry (razy tempo budowy).
//   Wiosna: orka i siew na polach.  Lato: zboze rosnie.  Jesien: zniwa (niezebrane zboze gnije zima).
//   Zima: pola leza odlogiem, krzaki bez owocow, kazda rodzina przy posilku pali klode w piecu.
#pragma once

#include "CoreMinimal.h"

enum class EPora : uint8 { Wiosna, Lato, Jesien, Zima };

struct FKalendarz
{
	/** Sekundy gry na jedna pore roku (4 min; rok = 16 min przy tempie x1). */
	static constexpr float DlugoscPory = 240.f;

	/** Sekundy gry od zalozenia obozu (razy tempo). */
	float Czas = 0.f;

	int32 Rok() const { return 1 + int32(Czas / (4.f * DlugoscPory)); }
	EPora Pora() const { return EPora(int32(Czas / DlugoscPory) % 4); }
	/** 0..1: jak daleko w obecnej porze. */
	float UlamekPory() const { return FMath::Frac(Czas / DlugoscPory); }
	float DoKoncaPory() const { return (1.f - UlamekPory()) * DlugoscPory; }

	static const TCHAR* Nazwa(EPora P)
	{
		switch (P)
		{
		case EPora::Wiosna: return TEXT("Wiosna");
		case EPora::Lato: return TEXT("Lato");
		case EPora::Jesien: return TEXT("Jesień");
		default: return TEXT("Zima");
		}
	}
	static EPora Nastepna(EPora P) { return EPora((int32(P) + 1) % 4); }
};
