# Źródła assetów i danych

Tutaj trafia każdy plik spoza projektu oraz każdy asset zrobiony przez AI. Przy każdym podaj licencję i wymaganą atrybucję.

| Co | Źródło | Licencja | Gdzie w projekcie |
|---|---|---|---|
| Wysokości terenu i dna morza | NOAA NCEI, ETOPO 2022 15 Arc-Second Global Relief Model (ngdc.noaa.gov, 17.11.2026) | domena publiczna; cytować: NOAA National Centers for Environmental Information. 2022: ETOPO 2022 15 Arc-Second Global Relief Model. doi:10.25921/fd45-gt74 | `Dane_zrodlowe/etopo` → `Data/europa_wys.r16` |
| Pokrycie terenu (lasy, łąki, mokradła…) | Copernicus Global Land Service LC100 Collection 3, epoka 2018, Discrete Classification (zenodo.org/records/3518038) | CC-BY 4.0. Atrybucja: „Buchhorn M. i in., Copernicus Global Land Service: Land Cover 100m: collection 3: epoch 2018: Globe, doi:10.5281/zenodo.3518038” | `Dane_zrodlowe/landcover` |
| Ląd, jeziora, rzeki | Natural Earth 10m physical (naturalearthdata.com) | domena publiczna | `Dane_zrodlowe/naturalearth` → `Data/europa_woda.r8` |

## Drzewa i krzaki (własne)
- `Art/Natura/*.glb` → `/Game/Swiat/Natura/SM_*`: modele zrobione skryptem `Tools/natura_drzewa.py`, bez cudzych assetów i bez licencji zewnętrznej.
