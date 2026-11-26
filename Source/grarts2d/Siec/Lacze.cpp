// Plik: Source/grarts2d/Siec/Lacze.cpp
#include "Siec/Lacze.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "SocketTypes.h"
#include "IPAddress.h"
#include "HAL/PlatformTime.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
	ISocketSubsystem* Gniazda() { return ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM); }
	constexpr double CzasLaczenia = 10.0;
}

void FPolaczenie::Zamknij(FSocket*& S)
{
	if (S)
	{
		S->Close();
		if (ISocketSubsystem* SS = Gniazda()) { SS->DestroySocket(S); }
		S = nullptr;
	}
}

bool FPolaczenie::Hostuj(FString& Blad, int32 Port)
{
	Rozlacz();
	ISocketSubsystem* SS = Gniazda();
	if (!SS)
	{
		Blad = TEXT("Brak obsługi sieci w systemie.");
		return false;
	}
	Nasluch = SS->CreateSocket(NAME_Stream, TEXT("grarts2d nasluch"), FNetworkProtocolTypes::IPv4);
	if (!Nasluch)
	{
		Blad = TEXT("Nie udało się otworzyć gniazda sieci.");
		return false;
	}
	TSharedRef<FInternetAddr> A = SS->CreateInternetAddr(FNetworkProtocolTypes::IPv4);
	A->SetAnyAddress();
	A->SetPort(Port);
	Nasluch->SetReuseAddr(true);
	Nasluch->SetNonBlocking(true);
	if (!Nasluch->Bind(*A) || !Nasluch->Listen(1))
	{
		Zamknij(Nasluch);
		Blad = FString::Printf(TEXT("Port %d jest zajęty (może druga gra już hostuje na tym komputerze)."), Port);
		return false;
	}
	bHost = true;
	StanL = EStan::Czeka;
	OstatniPowod.Empty();
	UE_LOG(LogTemp, Log, TEXT("[SIEC] Hostuje na porcie %d"), Port);
	return true;
}

bool FPolaczenie::Dolacz(const FString& AdresTekst, FString& Blad)
{
	Rozlacz();
	ISocketSubsystem* SS = Gniazda();
	if (!SS)
	{
		Blad = TEXT("Brak obsługi sieci w systemie.");
		return false;
	}
	FString Ip = AdresTekst.TrimStartAndEnd();
	int32 Port = PortDomyslny;
	FString IpS, PortS;
	if (Ip.Split(TEXT(":"), &IpS, &PortS, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
	{
		Ip = IpS;
		Port = FCString::Atoi(*PortS);
	}
	bool bPoprawny = false;
	TSharedRef<FInternetAddr> A = SS->CreateInternetAddr(FNetworkProtocolTypes::IPv4);
	A->SetIp(*Ip, bPoprawny);
	if (!bPoprawny || Port <= 0 || Port > 65535)
	{
		Blad = TEXT("Zły adres. Wpisz adres IP kolegi, np. 26.10.20.5 (Radmin VPN) albo 25.1.2.3 (Hamachi).");
		return false;
	}
	A->SetPort(Port);
	Gniazdo = SS->CreateSocket(NAME_Stream, TEXT("grarts2d lacze"), FNetworkProtocolTypes::IPv4);
	if (!Gniazdo)
	{
		Blad = TEXT("Nie udało się otworzyć gniazda sieci.");
		return false;
	}
	Gniazdo->SetNonBlocking(true);
	Gniazdo->SetNoDelay(true);
	if (!Gniazdo->Connect(*A))
	{
		Zamknij(Gniazdo);
		Blad = TEXT("Nie można połączyć z tym adresem.");
		return false;
	}
	bHost = false;
	StanL = EStan::Laczy;
	StartLaczenia = FPlatformTime::Seconds();
	AdresDrugiego = A->ToString(true);
	OstatniPowod.Empty();
	UE_LOG(LogTemp, Log, TEXT("[SIEC] Lacze z %s"), *AdresDrugiego);
	return true;
}

void FPolaczenie::Rozlacz(const FString& Powod)
{
	if (Gniazdo && StanL == EStan::Polaczony)
	{
		// Pozegnanie: druga strona od razu wie, ze to koniec (bez czekania na cisze).
		FTCHARToUTF8 U(TEXT("{\"t\":\"pa\"}\n"));
		int32 Wyslano = 0;
		Gniazdo->Send(reinterpret_cast<const uint8*>(U.Get()), U.Length(), Wyslano);
	}
	Zamknij(Gniazdo);
	Zamknij(Nasluch);
	if (StanL != EStan::Brak)
	{
		UE_LOG(LogTemp, Log, TEXT("[SIEC] Rozlaczono: %s"), Powod.IsEmpty() ? TEXT("-") : *Powod);
	}
	StanL = EStan::Brak;
	OstatniPowod = Powod;
	DoWyslania.Reset();
	Bufor.Reset();
	Odebrane.Reset();
	AdresDrugiego.Empty();
}

void FPolaczenie::Tick()
{
	const double Teraz = FPlatformTime::Seconds();
	if (StanL == EStan::Czeka && Nasluch)
	{
		bool bCzeka = false;
		if (Nasluch->HasPendingConnection(bCzeka) && bCzeka)
		{
			ISocketSubsystem* SS = Gniazda();
			TSharedRef<FInternetAddr> Od = SS->CreateInternetAddr(FNetworkProtocolTypes::IPv4);
			if (FSocket* Nowe = Nasluch->Accept(*Od, TEXT("grarts2d gracz")))
			{
				Gniazdo = Nowe;
				Gniazdo->SetNonBlocking(true);
				Gniazdo->SetNoDelay(true);
				// Jeden drugi gracz: dalej nikogo nie przyjmujemy.
				Zamknij(Nasluch);
				StanL = EStan::Polaczony;
				OstatniOdbior = Teraz;
				AdresDrugiego = Od->ToString(true);
				UE_LOG(LogTemp, Log, TEXT("[SIEC] Dolaczyl gracz z %s"), *AdresDrugiego);
			}
		}
		return;
	}
	if (StanL == EStan::Laczy && Gniazdo)
	{
		const ESocketConnectionState S = Gniazdo->GetConnectionState();
		if (S == SCS_Connected)
		{
			StanL = EStan::Polaczony;
			OstatniOdbior = Teraz;
			UE_LOG(LogTemp, Log, TEXT("[SIEC] Polaczono z %s"), *AdresDrugiego);
		}
		else if (S == SCS_ConnectionError || Teraz - StartLaczenia > CzasLaczenia)
		{
			Rozlacz(TEXT("Brak odpowiedzi hosta. Sprawdź adres, czy kolega kliknął „Hostuj” i czy oboje jesteście w tej samej sieci (Radmin VPN / Hamachi)."));
		}
		return;
	}
	if (StanL != EStan::Polaczony || !Gniazdo)
	{
		return;
	}
	if (!Wysylaj() || !Czytaj())
	{
		Rozlacz(TEXT("Połączenie zerwane."));
		return;
	}
	if (StanL != EStan::Polaczony)
	{
		return; // pozegnanie drugiej strony
	}
	if (Teraz - OstatniOdbior > CiszaDoRozlaczenia)
	{
		Rozlacz(TEXT("Drugi gracz nie odpowiada (20 s ciszy)."));
	}
}

bool FPolaczenie::Wysylaj()
{
	while (DoWyslania.Num() > 0)
	{
		int32 Wyslano = 0;
		if (!Gniazdo->Send(DoWyslania.GetData(), DoWyslania.Num(), Wyslano))
		{
			const ESocketErrors E = Gniazda()->GetLastErrorCode();
			return E == SE_EWOULDBLOCK || E == SE_NO_ERROR; // bufor systemu pelny: dokonczy w nastepnej klatce
		}
		if (Wyslano <= 0)
		{
			return true;
		}
		DoWyslania.RemoveAt(0, Wyslano, EAllowShrinking::No);
	}
	return true;
}

bool FPolaczenie::Czytaj()
{
	uint32 Rozmiar = 0;
	bool bCos = false;
	while (Gniazdo->HasPendingData(Rozmiar) && Rozmiar > 0)
	{
		bCos = true;
		const int32 Start = Bufor.Num();
		const int32 Ile = int32(FMath::Min<uint32>(Rozmiar, 64 * 1024));
		Bufor.AddUninitialized(Ile);
		int32 Przeczytano = 0;
		if (!Gniazdo->Recv(Bufor.GetData() + Start, Ile, Przeczytano))
		{
			Bufor.SetNum(Start);
			return false;
		}
		Bufor.SetNum(Start + FMath::Max(0, Przeczytano));
		if (Przeczytano <= 0)
		{
			break;
		}
	}
	if (!bCos)
	{
		// Gotowe do czytania, a zero bajtow = druga strona zamknela polaczenie.
		if (Gniazdo->Wait(ESocketWaitConditions::WaitForRead, FTimespan::Zero()))
		{
			uint8 Bajt = 0;
			int32 Przeczytano = 0;
			if (!Gniazdo->Recv(&Bajt, 1, Przeczytano, ESocketReceiveFlags::Peek) || Przeczytano == 0)
			{
				return false;
			}
		}
		return true; // zerwanie bez pozegnania wykrywa tez blad wysylki i 20 s ciszy
	}
	OstatniOdbior = FPlatformTime::Seconds();
	int32 Poczatek = 0;
	for (int32 i = 0; i < Bufor.Num(); ++i)
	{
		if (Bufor[i] != '\n')
		{
			continue;
		}
		const int32 Dl = i - Poczatek;
		if (Dl > 0)
		{
			FUTF8ToTCHAR T(reinterpret_cast<const ANSICHAR*>(Bufor.GetData() + Poczatek), Dl);
			FString L(T.Length(), T.Get());
			if (L == TEXT("{\"t\":\"pa\"}"))
			{
				Rozlacz(TEXT("Drugi gracz się rozłączył."));
				return true;
			}
			Odebrane.Add(MoveTemp(L));
		}
		Poczatek = i + 1;
	}
	if (Poczatek > 0)
	{
		Bufor.RemoveAt(0, Poczatek, EAllowShrinking::No);
	}
	return Bufor.Num() <= MaksLinii;
}

void FPolaczenie::Wyslij(const FString& Linia)
{
	if (StanL != EStan::Polaczony)
	{
		return;
	}
	FTCHARToUTF8 U(*Linia);
	DoWyslania.Append(reinterpret_cast<const uint8*>(U.Get()), U.Length());
	DoWyslania.Add('\n');
	if (DoWyslania.Num() > 4 * MaksLinii)
	{
		Rozlacz(TEXT("Za wolne łącze (zatkana kolejka wiadomości)."));
	}
}

bool FPolaczenie::Odbierz(FString& Linia)
{
	if (Odebrane.Num() == 0)
	{
		return false;
	}
	Linia = MoveTemp(Odebrane[0]);
	Odebrane.RemoveAt(0);
	return true;
}

FString FPolaczenie::Opis() const
{
	switch (StanL)
	{
	case EStan::Czeka: return FString::Printf(TEXT("Czekam na drugiego gracza (port %d)..."), PortDomyslny);
	case EStan::Laczy: return FString::Printf(TEXT("Łączę z %s..."), *AdresDrugiego);
	case EStan::Polaczony: return FString::Printf(TEXT("Połączono z %s"), *AdresDrugiego);
	default: return OstatniPowod.IsEmpty() ? FString(TEXT("Brak połączenia")) : OstatniPowod;
	}
}

TArray<FString> FPolaczenie::MojeAdresy()
{
	TArray<FString> Out;
	TArray<TSharedPtr<FInternetAddr>> Adresy;
	if (ISocketSubsystem* SS = Gniazda())
	{
		SS->GetLocalAdapterAddresses(Adresy);
	}
	for (const TSharedPtr<FInternetAddr>& A : Adresy)
	{
		if (A.IsValid() && A->GetProtocolType() == FNetworkProtocolTypes::IPv4)
		{
			const FString S = A->ToString(false);
			if (!S.StartsWith(TEXT("127.")) && !S.StartsWith(TEXT("169.254.")))
			{
				Out.AddUnique(S);
			}
		}
	}
	// Najpierw adresy sieci wirtualnych (Radmin VPN 26.x, Hamachi 25.x): te trzeba podac koledze.
	Out.Sort([](const FString& A, const FString& B)
	{
		const bool bA = A.StartsWith(TEXT("26.")) || A.StartsWith(TEXT("25."));
		const bool bB = B.StartsWith(TEXT("26.")) || B.StartsWith(TEXT("25."));
		return bA != bB ? bA : A < B;
	});
	return Out;
}

FPolaczenie& FLacze::Glowne()
{
	static FPolaczenie P;
	return P;
}

FString FLacze::OstatniAdres()
{
	FString A;
	if (GConfig)
	{
		GConfig->GetString(TEXT("Siec"), TEXT("Adres"), A, GGameUserSettingsIni);
	}
	return A;
}

void FLacze::ZapamietajAdres(const FString& A)
{
	if (GConfig)
	{
		GConfig->SetString(TEXT("Siec"), TEXT("Adres"), *A, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}
