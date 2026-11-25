// Plik: Source/grarts2d/Wojna/PanelBitwy.h
// Panel bitwy na dole ekranu (21.11.2026, wzor ukladu: Total War, wyglad nasz - ciemne drewno, zloto, pikselowe postacie):
//   - po lewej portret wybranego oddzialu (pikselowy woj w barwach oddzialu), nazwa, ludzie, duch, zmeczenie, doswiadczenie;
//   - na srodku karty twoich oddzialow: choragiewka, postac z bronia, liczba ludzi, pasek ducha, stan; klik = wybor
//     (Shift - dodaj), podwojny klik = kamera na oddzial; pod kartami przyciski rozkazow z ikonami (atak, stoj, szyk,
//     odwrot, pauza);
//   - po prawej mapka pola bitwy: woje jako kropki (twoi czerwoni, wrog czarny, wybrani zloci); klik = kamera tam.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

class ABitwa;

TSharedRef<SWidget> ZrobPanelBitwy(TWeakObjectPtr<ABitwa> Bitwa);
