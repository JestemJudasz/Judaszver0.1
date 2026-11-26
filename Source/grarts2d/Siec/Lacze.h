// Plik: Source/grarts2d/Siec/Lacze.h
// Gra w sieci, krok M1 (26.11.2026, prosba uzytkownika: "narazie multi proste w jednej sieci np jak na radminvpn czy hamaci").
// Dwa komputery w jednej sieci (LAN albo siec wirtualna Radmin VPN / Hamachi) lacza sie po adresie IP przez TCP na porcie 7777.
// Kazdy gracz ma wlasna gre i wlasna osade; przez lacze ida tylko krotkie wiadomosci tekstowe (jedna linia JSON na wiadomosc):
// stan osady, czat, dary. Jeden gracz "hostuje" (czeka na polaczenie), drugi "dolacza" (wpisuje adres hosta).
// FPolaczenie to jedno lacze (testy tworza dwa w jednym procesie), FLacze::Glowne() to lacze gry (przezywa przeladowanie poziomu).
// Bez Steam i bez serwerow: wystarczy, ze komputery widza sie w sieci (zapora Windows moze zapytac o zgode przy hostowaniu).
#pragma once

#include "CoreMinimal.h"

class FSocket;

class GRARTS2D_API FPolaczenie
{
public:
	enum class EStan : uint8 { Brak, Czeka, Laczy, Polaczony };

	static constexpr int32 PortDomyslny = 7777;
	/** Bez zadnej wiadomosci tyle sekund = drugi gracz zniknal (stan idzie co 2 s). */
	static constexpr double CiszaDoRozlaczenia = 20.0;
	/** Najdluzsza linia (bajty); dluzsza = zepsute dane, lacze sie zamyka. */
	static constexpr int32 MaksLinii = 256 * 1024;

	// Bez zamykania w destruktorze: lacze gry jest statyczne, a przy wyjsciu z gry system gniazd juz nie istnieje
	// (AGraczZdalny zamyka lacze przy wyjsciu, testy wolaja Rozlacz same).

	/** Czeka na drugiego gracza na porcie Port. */
	bool Hostuj(FString& Blad, int32 Port = PortDomyslny);
	/** Laczy z hostem: "26.10.20.5" albo "26.10.20.5:7777". */
	bool Dolacz(const FString& Adres, FString& Blad);
	/** Zamyka wszystko; Powod trafia do Opis() (np. "Drugi gracz się rozłączył"). */
	void Rozlacz(const FString& Powod = FString());

	/** Co klatke: przyjecie polaczenia, koniec laczenia, wysylka i odbior. */
	void Tick();
	/** Wiadomosc (bez znaku nowej linii) do wyslania; nic, gdy niepolaczony. */
	void Wyslij(const FString& Linia);
	/** Nastepna odebrana linia; false = brak. */
	bool Odbierz(FString& Linia);

	EStan Stan() const { return StanL; }
	bool Polaczony() const { return StanL == EStan::Polaczony; }
	bool Host() const { return bHost; }
	/** Stan slowami (menu i panel). */
	FString Opis() const;
	/** Adres drugiej strony ("26.10.20.5:51234"). */
	FString Adres() const { return AdresDrugiego; }

	/** Adresy IPv4 tego komputera (Radmin VPN zwykle 26.x.x.x, Hamachi 25.x.x.x). */
	static TArray<FString> MojeAdresy();

private:
	void Zamknij(FSocket*& S);
	bool Wysylaj();
	bool Czytaj();

	EStan StanL = EStan::Brak;
	bool bHost = false;
	FSocket* Nasluch = nullptr;
	FSocket* Gniazdo = nullptr;
	double StartLaczenia = 0.0;
	double OstatniOdbior = 0.0;
	FString AdresDrugiego;
	FString OstatniPowod;
	TArray<uint8> DoWyslania;
	TArray<uint8> Bufor;
	TArray<FString> Odebrane;
};

struct GRARTS2D_API FLacze
{
	/** Lacze gry (jedno na proces, przezywa wczytanie zapisu). */
	static FPolaczenie& Glowne();
	/** Ostatnio wpisany adres hosta (zapamietany w ustawieniach gry). */
	static FString OstatniAdres();
	static void ZapamietajAdres(const FString& A);
};
