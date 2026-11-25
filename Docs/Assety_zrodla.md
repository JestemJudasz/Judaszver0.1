# Źródła assetów i danych

Tutaj trafia każdy plik spoza projektu oraz każdy asset zrobiony przez AI. Przy każdym podaj licencję i wymaganą atrybucję.

| Co | Źródło | Licencja | Gdzie w projekcie |
|---|---|---|---|
| Wysokości terenu i dna morza | NOAA NCEI, ETOPO 2022 15 Arc-Second Global Relief Model (ngdc.noaa.gov, 17.11.2026) | domena publiczna; cytować: NOAA National Centers for Environmental Information. 2022: ETOPO 2022 15 Arc-Second Global Relief Model. doi:10.25921/fd45-gt74 | `Dane_zrodlowe/etopo` → `Data/europa_wys.r16` |
| Pokrycie terenu (lasy, łąki, mokradła…) | Copernicus Global Land Service LC100 Collection 3, epoka 2018, Discrete Classification (zenodo.org/records/3518038) | CC-BY 4.0. Atrybucja: „Buchhorn M. i in., Copernicus Global Land Service: Land Cover 100m: collection 3: epoch 2018: Globe, doi:10.5281/zenodo.3518038” | `Dane_zrodlowe/landcover` |
| Ląd, jeziora, rzeki | Natural Earth 10m physical (naturalearthdata.com) | domena publiczna | `Dane_zrodlowe/naturalearth` → `Data/europa_woda.r8` |

## Drzewa i krzaki (własne)
- `Art/Natura/*.glb` → `/Game/Swiat/Natura/SM_*`: modele zrobione skryptem `Tools/natura_drzewa.py`, bez cudzych assetów i bez licencji zewnętrznej.

## Znaki mapy papierowej (własne)
- `Art/Teren/T_MapaZnaki.png`, `Art/Teren/T_MapaDane.png`: rysowane i liczone skryptem `Tools/mapa_znaki.py` (z danych w `Data/`), bez cudzych assetów.

## Budynki (własne)
- `Art/Budynki/*.glb`, `Art/Budynki/T_Bud_*.png` → `/Game/Swiat/Budynki/`: modele i tekstury zrobione skryptem `Tools/budynki.py`, bez cudzych assetów i bez licencji zewnętrznej.
- `Art/Budynki/T_Ikona_*.png` → `/Game/Swiat/Budynki/`: ikony paska budowy renderowane z tych modeli skryptem `Tools/ikony.py` (20.11.2026), własne.
- `Art/Ludzie/*.glb` (w tym kobiety i bal, 20.11.2026): części ludzików ze skryptu `Tools/ludzie.py`, własne.

## Ekonomia, etap 1 (własne, 21.11.2026)
- Chata drwala, chata borowego, pilarnia, plac składowy i stosy towaru `SM_Stos_*_1..4`: modele ze skryptu `Tools/budynki.py`, własne.
- Narzędzia i niesione rzeczy ludzików (`SM_Ludz_Siekiera`, `Pila`, `Motyka`, `Deski`, `Sadzonka`): skrypt `Tools/ludzie.py`, własne.
- Ikony surowców `T_Ikona_Kloda/Deska/Kamien` (render stosów) i pikselowa `T_Ikona_Rodzina`: skrypt `Tools/ikony.py`, własne.

## Ekonomia, etap 2 (własne, 21.11.2026)
- Kamieniołom, chata zbieracza, chata myśliwego, spichlerz i stosy jedzenia `SM_Stos_Jedzenie_1..4`: modele ze skryptu `Tools/budynki.py`, własne.
- Narzędzia i niesione rzeczy ludzików (`SM_Ludz_Kilof`, `Luk`, `Kosz`, `Worek`, `Sarna`): skrypt `Tools/ludzie.py`, własne.
- Ikony `T_Ikona_Kamieniolom/ChataZbieracza/ChataMysliwego/Spichlerz` (render modeli), `T_Ikona_Jedzenie` (render stosu) i pikselowe serce `T_Ikona_Zadowolenie`: skrypt `Tools/ikony.py`, własne.


## Ekonomia, etap 3 (własne, 21.11.2026)
- Pole i wygląd upraw `SM_Pole_*`, młyn ze skrzydłami, piekarnia, tartak wodny z kołem, stosy `SM_Stos_Zboze_1..4` i `SM_Stos_Maka_1..4`: modele ze skryptu `Tools/budynki.py`, własne.
- Płot z segmentów `SM_Plot`, `SM_Plot_Palik` i kafle uprawy `SM_Pole_Kafel_*` (21.11.2026, pole i pastwisko zaznaczane na terenie): modele ze skryptu `Tools/budynki.py`, własne.
- Narzędzia i niesione rzeczy ludzików (`SM_Ludz_Sierp`, `Snop`, `Maka`, `Siewka`): skrypt `Tools/ludzie.py`, własne.
- Ikony `T_Ikona_Pole/Mlyn/Piekarnia/TartakWodny` i `T_Ikona_Zboze/Maka`: render modeli skryptem `Tools/ikony.py`, własne.


## Ekonomia, etap 4 (własne, 21.11.2026)
- Owczarnia, owce `SM_Owca` / `SM_Owca_Strzyzona`, chata tkacza z krosnem, kopalnia rudy, kuźnia, zbrojownia z manekinami, stosy `SM_Stos_Welna/Sukno/Ruda/Bron_1..4`: modele ze skryptu `Tools/budynki.py`, własne.
- Narzędzia i niesione rzeczy ludzików (`SM_Ludz_Nozyce`, `Welna`, `Sukno`, `Ruda`, `Bron`, `Wlocznia`): skrypt `Tools/ludzie.py`, własne.
- Ikony `T_Ikona_Owczarnia/ChataTkacza/KopalniaRudy/Kuznia/Zbrojownia` i `T_Ikona_Welna/Sukno/Ruda/Bron`: render modeli skryptem `Tools/ikony.py`, własne.
- Znaki dolnego paska `T_Ikona_Znak_*` (21.11.2026): rysowane w skrypcie `Tools/ikony.py`, własne.

## Dźwięki walki (22.11.2026)
| Co | Źródło | Licencja | Gdzie w projekcie |
|---|---|---|---|
| Zderzenia broni (miecz, topór, włócznia, maczuga, sztylet, drzewca) | OpenGameArt „Medieval sound effects - Weapon impacts”, Ben Jaszczak i Brian Nelson (Still North Media), opengameart.org/node/146863, pliki `medieval_sfx_weapon_on_weapon_1_of_2.7z` i `_2_of_2.7z` | CC0, atrybucja niewymagana (mile widziana: „Ben Jaszczak & Brian Nelson”) | `Dane_zrodlowe/dzwieki/oga_medieval` → `Tools/dzwieki_walka.py` → `Art/Dzwieki/Walka/Szczek_*, Drzewce_*, Tarcza_*, Maczuga_*` |
| Zamachy, łuk, kusza, świst strzał | OpenGameArt „Medieval sound effects - Weapon Textures”, ci sami autorzy, opengameart.org/content/medieval-sound-effects-weapon-textures, pliki `medieval_sfx_textures_1_of_2.7z` i `_2_of_2.7z` | CC0, jak wyżej | `Art/Dzwieki/Walka/Zamach_*, Luk_*, Kusza_*, Swist_*` |

## Wtyczki (22.11.2026)
| Co | Źródło | Licencja | Gdzie w projekcie |
|---|---|---|---|
| NVIDIA DLSS 4.5 (DLSS-SR, DLAA, RR), wersja 8.9.0 dla UE 5.8 | developer.nvidia.com/rtx/dlss, plik `2026.10.16_UE5.8_DLSS4.5Plugin_v8.9.0.zip` (1,17 GB; kopia w `D:\GRY\_pobrane_dlss`) | Licencja NVIDIA RTX SDK (przy wydaniu na Steam: logo/wzmianka wg warunków NVIDIA) | `Plugins/DLSS` (źródła + `Binaries/ThirdParty/Win64/nvngx_dlss*.dll`) i `Plugins/StreamlineNGXCommon`; kompilowane pod nasz silnik |

## Krzyki, praca, osada: Freesound (23.11.2026)
Pobrane za zgodą użytkownika; lista i podział na grupy: `Tools/dzwieki_freesound.txt`, `Tools/dzwieki_glosy.py`.

| Co | Źródło | Licencja | Gdzie w projekcie |
|---|---|---|---|
| „People screaming in agony when charging into battle” | Freesound, autor florianreichelt, freesound.org/people/florianreichelt/sounds/563011/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/563011.mp3` → `Art/Dzwieki/Freesound` |
| „Middle Ages War Cry” | Freesound, autor joelcarrsound, freesound.org/people/joelcarrsound/sounds/521830/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/521830.mp3` → `Art/Dzwieki/Freesound` |
| „Middle Ages War Cry 3” | Freesound, autor joelcarrsound, freesound.org/people/joelcarrsound/sounds/521831/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/521831.mp3` → `Art/Dzwieki/Freesound` |
| „Middle Ages War Cry 2” | Freesound, autor joelcarrsound, freesound.org/people/joelcarrsound/sounds/521832/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/521832.mp3` → `Art/Dzwieki/Freesound` |
| „CRWDBatl_Crowd Yells Charge, Continues” | Freesound, autor ShangusBurger, freesound.org/people/ShangusBurger/sounds/764158/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/764158.mp3` → `Art/Dzwieki/Freesound` |
| „R21-07-Men Yelling While Fighting” | Freesound, autor craigsmith, freesound.org/people/craigsmith/sounds/486193/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/486193.mp3` → `Art/Dzwieki/Freesound` |
| „Crowd/Mob/Riot Noise (Voices Only) - 14 people, 2 minutes” | Freesound, autor FillMat, freesound.org/people/FillMat/sounds/384401/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/384401.mp3` → `Art/Dzwieki/Freesound` |
| „Grunt2 - Death Pain” | Freesound, autor tonsil5, freesound.org/people/tonsil5/sounds/416838/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/416838.mp3` → `Art/Dzwieki/Freesound` |
| „Grunt1 - Death Pain” | Freesound, autor tonsil5, freesound.org/people/tonsil5/sounds/416839/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/416839.mp3` → `Art/Dzwieki/Freesound` |
| „Male Screams [1]” | Freesound, autor marc3122, freesound.org/people/marc3122/sounds/556976/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/556976.mp3` → `Art/Dzwieki/Freesound` |
| „Groans and Screams” | Freesound, autor waxsocks, freesound.org/people/waxsocks/sounds/255322/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/255322.mp3` → `Art/Dzwieki/Freesound` |
| „Medium Male Pain Grunts” | Freesound, autor unfa, freesound.org/people/unfa/sounds/610998/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/610998.mp3` → `Art/Dzwieki/Freesound` |
| „groans” | Freesound, autor iggy1345, freesound.org/people/iggy1345/sounds/103072/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/103072.mp3` → `Art/Dzwieki/Freesound` |
| „Melee Weapon hit with male moan & body fall” | Freesound, autor Paul368, freesound.org/people/Paul368/sounds/264062/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/264062.mp3` → `Art/Dzwieki/Freesound` |
| „body_hit” | Freesound, autor insanity54, freesound.org/people/insanity54/sounds/276600/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/276600.mp3` → `Art/Dzwieki/Freesound` |
| „stabs” | Freesound, autor nicktermer, freesound.org/people/nicktermer/sounds/259542/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/259542.mp3` → `Art/Dzwieki/Freesound` |
| „Hitting in a Face” | Freesound, autor florianreichelt, freesound.org/people/florianreichelt/sounds/460509/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/460509.mp3` → `Art/Dzwieki/Freesound` |
| „heavy sword against sword or shield” | Freesound, autor marchelonia, freesound.org/people/marchelonia/sounds/588278/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/588278.mp3` → `Art/Dzwieki/Freesound` |
| „thud_me” | Freesound, autor opriema, freesound.org/people/opriema/sounds/400188/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/400188.mp3` → `Art/Dzwieki/Freesound` |
| „Axe chopping wood” | Freesound, autor xkeril, freesound.org/people/xkeril/sounds/753925/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/753925.mp3` → `Art/Dzwieki/Freesound` |
| „Chopping wood” | Freesound, autor 14FPanskaSilovsky_Petr, freesound.org/people/14FPanskaSilovsky_Petr/sounds/419928/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/419928.mp3` → `Art/Dzwieki/Freesound` |
| „Chopping wood 01” | Freesound, autor micahlg, freesound.org/people/micahlg/sounds/421928/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/421928.mp3` → `Art/Dzwieki/Freesound` |
| „Chopping wood 02” | Freesound, autor micahlg, freesound.org/people/micahlg/sounds/421929/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/421929.mp3` → `Art/Dzwieki/Freesound` |
| „Pickaxe #2” | Freesound, autor abstraktgeneriert, freesound.org/people/abstraktgeneriert/sounds/233630/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/233630.mp3` → `Art/Dzwieki/Freesound` |
| „Pickaxe3” | Freesound, autor NoisyRedFox, freesound.org/people/NoisyRedFox/sounds/760567/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/760567.mp3` → `Art/Dzwieki/Freesound` |
| „Stone hit” | Freesound, autor CamoMano, freesound.org/people/CamoMano/sounds/431019/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/431019.mp3` → `Art/Dzwieki/Freesound` |
| „Gathering Stone Resources” | Freesound, autor qubodup, freesound.org/people/qubodup/sounds/184383/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/184383.mp3` → `Art/Dzwieki/Freesound` |
| „Hand Saw Sawing Wood” | Freesound, autor deleted_user_7146007, freesound.org/people/deleted_user_7146007/sounds/383725/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/383725.mp3` → `Art/Dzwieki/Freesound` |
| „Hammer and anvil” | Freesound, autor Duasun, freesound.org/people/Duasun/sounds/321889/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/321889.mp3` → `Art/Dzwieki/Freesound` |
| „Hammer hitting an anvil” | Freesound, autor draftcraft, freesound.org/people/draftcraft/sounds/434339/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/434339.mp3` → `Art/Dzwieki/Freesound` |
| „Hammering on Anvil” | Freesound, autor SamsterBirdies, freesound.org/people/SamsterBirdies/sounds/363169/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/363169.mp3` → `Art/Dzwieki/Freesound` |
| „hammer pounding on wood” | Freesound, autor Ryujin95, freesound.org/people/Ryujin95/sounds/394891/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/394891.mp3` → `Art/Dzwieki/Freesound` |
| „Hammering_Wood” | Freesound, autor CapsLok, freesound.org/people/CapsLok/sounds/204946/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/204946.mp3` → `Art/Dzwieki/Freesound` |
| „medieval_village_atmosphere” | Freesound, autor ramonmineiro, freesound.org/people/ramonmineiro/sounds/578072/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/578072.mp3` → `Art/Dzwieki/Freesound` |
| „People talking (Adults & Children)” | Freesound, autor Breviceps, freesound.org/people/Breviceps/sounds/473586/ (podgląd HQ MP3) | CC0, atrybucja niewymagana | `Dane_zrodlowe/dzwieki/freesound/473586.mp3` → `Art/Dzwieki/Freesound` |
| Chata opałowa, chata rybaka, zagroda wołów / obora, stosy opału i ryb, wędka, ryby i polana niesione, wół (tułów, noga, kłody) | Własne, proceduralnie: `Tools/budynki.py`, `Tools/ludzie.py` (23.11.2026) | własne | `Art/Budynki`, `Art/Ludzie` |
| Stajnia, koń (tułów, noga), wóz i ładunek wozu (deski, kamień) | Własne, proceduralnie: `Tools/budynki.py`, `Tools/ludzie.py` (23.11.2026) | własne | `Art/Budynki`, `Art/Ludzie` |
| Garbarnia, warsztat szewca i krawca, słodownia, browar, karczma, stosy skór / skóry / butów / ubrań / słodu / piwa, niesione skóry, skóra, buty, ubrania, słód, baryłka | Własne, proceduralnie: `Tools/budynki.py`, `Tools/ludzie.py` (23.11.2026) | własne | `Art/Budynki`, `Art/Ludzie` |
| Palisada na wale (odcinek, odcinek ze schodami, baszta, brama), mur kamienny (odcinek, odcinek ze schodami, okrągła baszta, brama z łukiem), studnia placu dla osad rodów | Własne, proceduralnie: `Tools/budynki.py` (23.11.2026, druga wersja) | własne | `Art/Budynki` |
| Tekstura drogi `T_Droga` (ubita ziemia, kamyki, pęknięcia, szum brzegu w alfie), materiał `M_Droga`, znak przycisku „Drogi” | Własne, proceduralnie: `Tools/drogi.py`, `Tools/ue_drogi.py` (21.11.2026, tekstura 23.11.2026 druga wersja) | własne | `Art/Drogi` |
| Modele zniszczone budynków, murów i bram: `_Z1` (uszkodzony: dziury w dachu, wybite ściany, osmalenia) i `_Z2` (zgliszcza: resztki ścian, zwęglone belki, gruz), 110 modeli | Własne, proceduralnie: `Tools/budynki.py` (23.11.2026, wojna) | własne | `Art/Budynki` |
| Efekty wojny: tekstury płomienia, dymu, szumu i kropki (`T_Efekt_*`), materiały ognia, dymu, iskier i gruzu (`M_Efekt_*`), modele języka ognia, kłębu i kawałka gruzu (`SM_Efekt_*`) | Własne, proceduralnie: `Tools/efekty.py`, `Tools/ue_efekty.py` (23.11.2026) | własne | `Art/Efekty` |
