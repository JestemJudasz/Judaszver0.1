# Plik: Tools/rzeki.py
# Rzeki mapy grywalnej jako linie (nie piksele): Natural Earth 10m "rivers_lake_centerlines" -> Data/rzeki.bin.
# Kazda linia: wygladzona (Chaikin), co 20 m w grze, z lagodnymi zakolami (przesuniecie w bok ~ +-20 m, fala 170/420 m),
# przedluzona o 40 m na koncach (doplyw wchodzi w wode rzeki glownej mimo jej zakoli).
# Format (little-endian): int32 ile_linii; dla kazdej: int32 n, float32 polowa_szerokosci_cm, n x (float32 X_cm, float32 Y_cm)
# Uklad gry: X = polnoc, Y = wschod, cm, srodek swiata w (0,0) - jak FSwiat (SwiatWysokosci.h).
# Uruchomienie: Tools\_py na PYTHONPATH (patrz Zbuduj_Rzeki.bat).
import os, struct, sys, zlib
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dem_europa import ROOT, OUT, POL_M, czytaj_shp, do_xy, w_zasiegu

CM_NA_M = 100.0 / 8.0           # prawdziwy metr -> cm w grze (1:8)
POLOWA_CM = POL_M * CM_NA_M
KROK_CM = 2000.0                # punkt co 20 m w grze


def polowa_szerokosci_cm(rank):
    """Szerokosc koryta w grze wg waznosci rzeki (scalerank: mniej = wieksza rzeka)."""
    return 2000.0 if rank <= 3 else 1400.0 if rank <= 5 else 1000.0 if rank <= 7 else 700.0


def chaikin(p, razy=3):
    for _ in range(razy):
        q = np.empty((2 * (len(p) - 1), 2))
        q[0::2] = 0.75 * p[:-1] + 0.25 * p[1:]
        q[1::2] = 0.25 * p[:-1] + 0.75 * p[1:]
        p = np.vstack([p[:1], q, p[-1:]])  # konce zostaja (polaczenia doplywow)
    return p


def co_krok(p, krok):
    d = np.r_[0, np.cumsum(np.hypot(*np.diff(p, axis=0).T))]
    if d[-1] < krok:
        return p, d
    s = np.linspace(0, d[-1], int(d[-1] // krok) + 1)
    return np.c_[np.interp(s, d, p[:, 0]), np.interp(s, d, p[:, 1])], s


def zakola(p, s, polowa, ziarno):
    """Przesuniecie w bok wzdluz linii; na koncach wygaszone (polaczenia sie nie rozjezdzaja)."""
    rng = np.random.default_rng(ziarno)
    t = np.gradient(p, axis=0)
    t /= np.linalg.norm(t, axis=1, keepdims=True) + 1e-9
    bok = np.c_[-t[:, 1], t[:, 0]]
    amp = 1800.0 + 0.5 * polowa
    f1, f2 = rng.uniform(0, 2 * np.pi, 2)
    fala = 0.7 * np.sin(2 * np.pi * s / 42000.0 + f1) + 0.3 * np.sin(2 * np.pi * s / 17000.0 + f2)
    wygas = np.clip(np.minimum(s, s[-1] - s) / 20000.0, 0, 1)
    return p + bok * (amp * fala * wygas)[:, None]


def przedluz(p, o=4000.0):
    a = p[0] - p[1]; b = p[-1] - p[-2]
    return np.vstack([p[0] + a / (np.linalg.norm(a) + 1e-9) * o, p, p[-1] + b / (np.linalg.norm(b) + 1e-9) * o])


def main():
    linie = []
    for sr in czytaj_shp("ne_10m_rivers_lake_centerlines").iterShapeRecords():
        s = sr.shape
        if not s.points or not w_zasiegu(s.bbox):
            continue
        rank = sr.record.as_dict().get("scalerank", 6) or 6
        polowa = polowa_szerokosci_cm(rank)
        czesci = list(s.parts) + [len(s.points)]
        for i in range(len(czesci) - 1):
            pts = np.asarray(s.points[czesci[i]:czesci[i + 1]], np.float64)
            if len(pts) < 2:
                continue
            x, y = do_xy(pts[:, 1], pts[:, 0])
            p = np.c_[y * CM_NA_M, x * CM_NA_M]  # gra: X = polnoc, Y = wschod
            if not (np.abs(p) < POLOWA_CM).all(axis=1).any():
                continue  # cala linia poza swiatem gry
            p, odl = co_krok(chaikin(p), KROK_CM)
            if len(p) < 2:
                continue
            p = przedluz(zakola(p, odl, polowa, zlib.crc32(f"{sr.record.as_dict().get('name')}{i}{len(linie)}".encode())))
            linie.append((polowa, p.astype(np.float32)))

    plik = os.path.join(OUT, "rzeki.bin")
    with open(plik, "wb") as f:
        f.write(struct.pack("<i", len(linie)))
        for polowa, p in linie:
            f.write(struct.pack("<if", len(p), polowa))
            f.write(p.astype("<f4").tobytes())
    pkt = sum(len(p) for _, p in linie)
    # Kontrola: Wisla przez Krakow - jakis punkt rzeki blizej niz 2 km (w grze) od startu kamery.
    start = np.array([float(v) for v in open(os.path.join(OUT, "europa_wys.txt")).read().split()[4:6]])
    najblizej = min(np.min(np.hypot(*(p - start).T)) for _, p in linie)
    print(f"rzeki.bin: {len(linie)} linii, {pkt} punktow, najblizej Krakowa {najblizej / 100:.0f} m")
    assert najblizej < 2e5, "Wisla nie przechodzi kolo Krakowa - zly uklad wspolrzednych"


if __name__ == "__main__":
    main()
