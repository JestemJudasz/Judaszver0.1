# Plik: Tools/pokrycie_europa.py
# Pokrycie terenu Europy (Copernicus LC100 2018) -> Data/europa_pokrycie.r8 w ukladzie mapy gry (jak europa_wys.r16).
# "Dzika Europa": pola uprawne i miasta zamieniane na puszcze z polanami (gracz sam karczuje).
# Uruchomienie: Tools\Przelicz_Europe.bat (po dem_europa.py) albo bezposrednio Pythonem z UE z PYTHONPATH=Tools\_py.
#
# Klasy gry (bajt):
#   0 woda   1 laka   2 wrzosowisko/krzewy   3 las lisciasty   4 las iglasty   5 las mieszany
#   6 las rzadki   7 mokradlo   8 skaly/bez roslin   9 snieg/lod   10 tundra/mchy
import os, sys
import numpy as np
import tifffile
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dem_europa import N, POL_M, ROOT, do_geo  # ten sam uklad mapy

PLIK = os.path.join(ROOT, "Dane_zrodlowe", "landcover",
                    "PROBAV_LC100_global_v3.0.1_2018-conso_Discrete-Classification-map_EPSG-4326.tif")
LAT_MAX, LAT_MIN, LON_MIN, LON_MAX = 76.0, 29.0, -31.0, 61.0
CO = 4  # co ktory piksel zrodla (100 m -> ~400 m); mapa gry ma 512 m na piksel

WODA, LAKA, WRZOS, LISC, IGL, MIESZ, RZADKI, MOKR, SKALY, SNIEG, TUNDRA = range(11)
DZIKIE = 250  # tymczasowo: pole uprawne / miasto do zamiany

# Copernicus LCCS -> klasy gry
MAPA = np.full(256, LAKA, np.uint8)
MAPA[[0, 200, 80]] = WODA
MAPA[20] = WRZOS
MAPA[30] = LAKA
MAPA[[40, 50]] = DZIKIE
MAPA[60] = SKALY
MAPA[70] = SNIEG
MAPA[90] = MOKR
MAPA[100] = TUNDRA
MAPA[[111, 113]] = IGL
MAPA[[112, 114]] = LISC
MAPA[[115, 116]] = MIESZ
MAPA[121:127] = RZADKI


def wczytaj_europe():
    t = tifffile.TiffFile(PLIK)
    p = t.pages[0]
    lon0, lat0 = p.tags["ModelTiepointTag"].value[3:5]
    krok = p.tags["ModelPixelScaleTag"].value[0]
    r0, r1 = int((lat0 - LAT_MAX) / krok), int((lat0 - LAT_MIN) / krok)
    c0, c1 = int((LON_MIN - lon0) / krok), int((LON_MAX - lon0) / krok)
    tw, th = p.tilewidth, p.tilelength
    kafli_w_wierszu = -(-p.shape[1] // tw)
    out = np.zeros(((r1 - r0) // CO + 1, (c1 - c0) // CO + 1), np.uint8)
    fh = t.filehandle
    for tr in range(r0 // th, r1 // th + 1):
        print(f"  wiersz kafli {tr - r0 // th + 1}/{r1 // th - r0 // th + 1}", end="\r")
        for tc in range(c0 // tw, c1 // tw + 1):
            i = tr * kafli_w_wierszu + tc
            fh.seek(p.dataoffsets[i])
            seg, _, _ = p.decode(fh.read(p.databytecounts[i]), i)
            seg = seg.reshape(th, tw)
            # Wiersze/kolumny kafla, ktore traca w siatke "co CO piksel" liczona od r0/c0.
            gr = np.arange(tr * th, tr * th + th); gc = np.arange(tc * tw, tc * tw + tw)
            mr = (gr >= r0) & (gr < r1) & ((gr - r0) % CO == 0)
            mc = (gc >= c0) & (gc < c1) & ((gc - c0) % CO == 0)
            if mr.any() and mc.any():
                out[np.ix_((gr[mr] - r0) // CO, (gc[mc] - c0) // CO)] = seg[np.ix_(mr, mc)]
    print()
    return out, krok * CO, lat0 - r0 * krok, lon0 + c0 * krok


def szum(ksztalt, skala_px, ziarno):
    """Gladki szum 0..1 (kilka oktaw, powiekszanie losowych siatek)."""
    rng = np.random.default_rng(ziarno)
    wynik = np.zeros(ksztalt, np.float32); amp, suma = 1.0, 0.0
    for o in range(4):
        n = max(2, int(ksztalt[0] / (skala_px / 2 ** o)))
        mala = (rng.random((n, n)) * 255).astype(np.uint8)
        wynik += amp * np.asarray(Image.fromarray(mala).resize(ksztalt[::-1], Image.BICUBIC), np.float32) / 255
        suma += amp; amp *= 0.5
    return wynik / suma


def main():
    print("Czytam pokrycie Europy (Copernicus)...")
    zr, krok, lat_gora, lon_lewo = wczytaj_europe()

    print("Odwzorowanie na mape gry...")
    P = np.empty((N, N), np.uint8)
    lat_kol = np.empty((N, N), np.float32)
    lon_kol = np.empty((N, N), np.float32)
    xs = (np.arange(N) + 0.5) / N * 2 * POL_M - POL_M
    for r0 in range(0, N, 512):
        ys = POL_M - (np.arange(r0, r0 + 512) + 0.5) / N * 2 * POL_M
        lat, lon = do_geo(*np.meshgrid(xs, ys))
        r = np.clip(((lat_gora - lat) / krok).astype(np.int64), 0, zr.shape[0] - 1)
        c = np.clip(((lon - lon_lewo) / krok).astype(np.int64), 0, zr.shape[1] - 1)
        P[r0:r0 + 512] = MAPA[zr[r, c]]
        lat_kol[r0:r0 + 512] = lat
        lon_kol[r0:r0 + 512] = lon
    del zr

    H = (np.fromfile(os.path.join(ROOT, "Data", "europa_wys.r16"), "<u2").reshape(N, N).astype(np.float32)
         / 65535 * (4900 + 200) - 200)
    W = np.fromfile(os.path.join(ROOT, "Data", "europa_woda.r8"), np.uint8).reshape(N, N)

    # Wyspy Brytyjskie: dzisiejsze pastwiska (w danych "laka") tez zalesiamy; wyzyny Szkocji, Walii i Irlandii
    # zostaja wrzosowiskiem (decyzja uzytkownika 17.11.2026).
    wyspy = (lat_kol > 49.8) & (lat_kol < 61.0) & (lon_kol > -11.0) & (lon_kol < 2.0) & (W == 0)
    P[wyspy & (P == LAKA) & (H <= 350)] = DZIKIE
    P[wyspy & (P == LAKA) & (H > 350)] = WRZOS

    print("Dzika Europa: pola i miasta -> puszcza z polanami...")
    dz = P == DZIKIE
    polany = szum((N, N), 160, 1)        # plamy ~10-20 km w grze
    drobne = szum((N, N), 24, 2)         # brzegi polan nieregularne
    # Wiecej polan na prosbe uzytkownika (17.11.2026): prog 0.62 -> 0.56.
    otwarte = (polany * 0.75 + drobne * 0.25) > 0.56
    # Rodzaj puszczy: polnoc i gory iglaste, pas posredni mieszany, reszta lisciasta.
    typ = np.where((lat_kol > 59) | (H > 1000), IGL, np.where((lat_kol > 54) | (H > 600), MIESZ, LISC)).astype(np.uint8)
    brzeg = (polany * 0.75 + drobne * 0.25) > 0.51
    P[dz] = np.where(otwarte[dz], LAKA, np.where(brzeg[dz], RZADKI, typ[dz]))
    print(f"  puszcza: polany {otwarte[dz].mean() * 100:.0f}%, skraj lasu {(brzeg[dz] & ~otwarte[dz]).mean() * 100:.0f}%")

    # Woda wedlug naszej maski (zgodnie z terenem), lad nigdy jako woda.
    P[W > 0] = WODA
    P[(W == 0) & (P == WODA)] = MOKR
    # Wysokie gory: ponad 2500 m bez lasu.
    P[(H > 2500) & (P != WODA) & (P != SNIEG)] = SKALY

    P.tofile(os.path.join(ROOT, "Data", "europa_pokrycie.r8"))

    kolory = np.array([[52, 86, 104], [128, 150, 82], [140, 110, 130], [62, 98, 46], [40, 70, 48], [52, 84, 46],
                       [96, 122, 66], [92, 112, 92], [140, 132, 118], [236, 238, 240], [150, 150, 120]], np.uint8)
    Image.fromarray(kolory[P[::8, ::8]]).save(os.path.join(ROOT, "Docs", "europa_pokrycie_podglad.png"))
    udzial = np.bincount(P.ravel(), minlength=11) / P.size * 100
    nazwy = ["woda", "laka", "wrzos", "lisc", "igl", "miesz", "rzadki", "mokr", "skaly", "snieg", "tundra"]
    print("Udzial %:", ", ".join(f"{n} {u:.1f}" for n, u in zip(nazwy, udzial)))
    assert P.max() <= 10, "zostala nieprzypisana klasa"
    print("GOTOWE")


if __name__ == "__main__":
    main()
