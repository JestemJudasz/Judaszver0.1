# Plik: Tools/dem_europa.py
# Przelicza pobrane dane (ETOPO 2022 15", Natural Earth 10m) na mape gry: Europa 1:8, kwadrat 4194 km prawdziwych.
# Uruchomienie (z folderu projektu):  Tools\Przelicz_Europe.bat
# Wejscie:  Dane_zrodlowe/etopo/*.tif, Dane_zrodlowe/naturalearth/*.zip
# Wyjscie:  Data/europa_wys.r16 (uint16, wiersz 0 = polnoc, kolumna 0 = zachod)
#           Data/europa_wys.txt ("szer wys min_m max_m startX_cm startY_cm")
#           Data/europa_woda.r8 (0 lad, 1 morze, 2 jezioro, 3 rzeka)
#           Docs/europa_podglad.png (podglad mapy)
import glob, io, math, os, re, sys, zipfile
import numpy as np
import tifffile, shapefile
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Dane_zrodlowe")
OUT = os.path.join(ROOT, "Data")

N = 8192                       # pikseli na bok (512 m prawdziwych = 64 m w grze)
POL_M = 524288.0 * 8 / 2       # polowa boku w prawdziwych metrach (gra: 524,288 km w skali 1:8)
LAT0, LON0 = 52.0, 10.0        # srodek odwzorowania (rownopolowe azymutalne Lamberta, jak EPSG:3035)
R = 6371007.0
MIN_M, MAX_M = -200.0, 4900.0  # zakres zapisu; morze glebiej niz 200 m nie jest potrzebne
KRAKOW = (50.0614, 19.9366)

f0, l0 = math.radians(LAT0), math.radians(LON0)


def do_xy(lat, lon):
    """Stopnie -> metry LAEA (x wschod, y polnoc)."""
    f, l = np.radians(lat), np.radians(lon)
    k = np.sqrt(2.0 / (1.0 + math.sin(f0) * np.sin(f) + math.cos(f0) * np.cos(f) * np.cos(l - l0)))
    return R * k * np.cos(f) * np.sin(l - l0), R * k * (math.cos(f0) * np.sin(f) - math.sin(f0) * np.cos(f) * np.cos(l - l0))


def do_geo(x, y):
    """Metry LAEA -> stopnie (lat, lon)."""
    rho = np.maximum(np.hypot(x, y), 1e-9)
    c = 2.0 * np.arcsin(np.clip(rho / (2.0 * R), -1, 1))
    lat = np.arcsin(np.cos(c) * math.sin(f0) + y * np.sin(c) * math.cos(f0) / rho)
    lon = l0 + np.arctan2(x * np.sin(c), rho * math.cos(f0) * np.cos(c) - y * math.sin(f0) * np.sin(c))
    return np.degrees(lat), np.degrees(lon)


def do_px(x, y):
    """Metry LAEA -> piksel (kolumna, wiersz) mapy gry."""
    return (x + POL_M) / (2 * POL_M) * N, (POL_M - y) / (2 * POL_M) * N


def mozaika():
    pliki = glob.glob(os.path.join(SRC, "etopo", "*.tif"))
    kafle = []
    for p in pliki:
        m = re.search(r"_([NS])(\d+)([EW])(\d+)_", p)
        lat = int(m.group(2)) * (1 if m.group(1) == "N" else -1)
        lon = int(m.group(4)) * (1 if m.group(3) == "E" else -1)
        kafle.append((lat, lon, p))
    gora = max(k[0] for k in kafle); dol = min(k[0] for k in kafle) - 15
    lewo = min(k[1] for k in kafle); prawo = max(k[1] for k in kafle) + 15
    M = np.full(((gora - dol) * 240, (prawo - lewo) * 240), -100.0, np.float32)
    for lat, lon, p in kafle:
        r, c = (gora - lat) * 240, (lon - lewo) * 240
        M[r:r + 3600, c:c + 3600] = tifffile.imread(p)
        print("  kafel", os.path.basename(p))
    return M, gora, lewo


def probkuj(M, gora, lewo, lat, lon):
    """Dwuliniowo; piksel ETOPO 15" = 1/240 stopnia, srodek piksela w +0,5."""
    v = (gora - lat) * 240 - 0.5
    u = (lon - lewo) * 240 - 0.5
    r0 = np.clip(np.floor(v).astype(np.int64), 0, M.shape[0] - 2)
    c0 = np.clip(np.floor(u).astype(np.int64), 0, M.shape[1] - 2)
    fv = np.clip(v - r0, 0, 1); fu = np.clip(u - c0, 0, 1)
    a = M[r0, c0] * (1 - fu) + M[r0, c0 + 1] * fu
    b = M[r0 + 1, c0] * (1 - fu) + M[r0 + 1, c0 + 1] * fu
    return a * (1 - fv) + b * fv


def czytaj_shp(nazwa):
    z = zipfile.ZipFile(os.path.join(SRC, "naturalearth", nazwa + ".zip"))
    pliki = {os.path.splitext(n)[1].lower(): n for n in z.namelist()}
    return shapefile.Reader(shp=io.BytesIO(z.read(pliki[".shp"])), shx=io.BytesIO(z.read(pliki[".shx"])),
                            dbf=io.BytesIO(z.read(pliki[".dbf"])))


def w_zasiegu(bbox):
    return bbox[2] > -45 and bbox[0] < 75 and bbox[3] > 20 and bbox[1] < 85


def punkty_px(pts):
    a = np.asarray(pts, np.float64)
    x, y = do_xy(a[:, 1], a[:, 0])
    c, r = do_px(x, y)
    # Punkty daleko poza mapa sciagamy do ramki 6x wiekszej (PIL nie lubi ogromnych liczb).
    return list(zip(np.clip(c, -5 * N, 6 * N).tolist(), np.clip(r, -5 * N, 6 * N).tolist()))


def rysuj_wielokaty(img, nazwa, wartosc):
    d = ImageDraw.Draw(img)
    for sr in czytaj_shp(nazwa).iterShapeRecords():
        s = sr.shape
        if not s.points or not w_zasiegu(s.bbox):
            continue
        czesci = list(s.parts) + [len(s.points)]
        for i in range(len(czesci) - 1):
            ring = s.points[czesci[i]:czesci[i + 1]]
            if len(ring) < 3:
                continue
            # Shapefile: obwod zewnetrzny zgodnie z zegarem, dziury przeciwnie.
            pole = sum(ring[k][0] * ring[k + 1][1] - ring[k + 1][0] * ring[k][1] for k in range(len(ring) - 1))
            d.polygon(punkty_px(ring), fill=wartosc if pole < 0 else 0)


def rysuj_rzeki(img, wartosc):
    d = ImageDraw.Draw(img)
    for sr in czytaj_shp("ne_10m_rivers_lake_centerlines").iterShapeRecords():
        s = sr.shape
        if not s.points or not w_zasiegu(s.bbox):
            continue
        rank = sr.record.as_dict().get("scalerank", 6) or 6
        szer = 2 if rank <= 4 else 1
        czesci = list(s.parts) + [len(s.points)]
        for i in range(len(czesci) - 1):
            linia = s.points[czesci[i]:czesci[i + 1]]
            if len(linia) >= 2:
                d.line(punkty_px(linia), fill=wartosc, width=szer)


def main():
    os.makedirs(OUT, exist_ok=True)
    print("Mozaika ETOPO...")
    M, gora, lewo = mozaika()

    print("Wysokosci 8192x8192...")
    H = np.empty((N, N), np.float32)
    xs = (np.arange(N) + 0.5) / N * 2 * POL_M - POL_M
    for r0 in range(0, N, 512):
        ys = POL_M - (np.arange(r0, r0 + 512) + 0.5) / N * 2 * POL_M
        X, Y = np.meshgrid(xs, ys)
        lat, lon = do_geo(X, Y)
        H[r0:r0 + 512] = probkuj(M, gora, lewo, lat, lon)
    del M

    print("Lad, jeziora, rzeki (Natural Earth)...")
    lad = Image.new("L", (N, N), 0); rysuj_wielokaty(lad, "ne_10m_land", 1)
    jez = Image.new("L", (N, N), 0); rysuj_wielokaty(jez, "ne_10m_lakes", 1)
    rz = Image.new("L", (N, N), 0); rysuj_rzeki(rz, 1)
    lad, jez, rz = (np.array(i, np.uint8) > 0 for i in (lad, jez, rz))

    W = np.zeros((N, N), np.uint8)
    W[~lad] = 1
    W[lad & jez] = 2
    W[lad & rz & ~jez] = 3

    # Lad zawsze nad morzem (polderom w Holandii nie zalewamy), morze zawsze pod.
    H = np.where(lad, np.maximum(H, 1.0), np.minimum(H, -2.0))
    # Rzeki lekko wciete w teren, zeby woda lezala w korycie.
    H = np.where(W == 3, np.maximum(H - 4.0, 0.5), H)
    H = np.clip(H, MIN_M, MAX_M)

    # Kontrola na znanych punktach (ETOPO 15" wygladza szczyty, wiec szerokie progi).
    def wys(lat, lon):
        x, y = do_xy(np.array([lat]), np.array([lon])); c, r = do_px(x, y)
        return float(H[int(r[0]), int(c[0])]), int(W[int(r[0]), int(c[0])])
    kontrola = {"Mont Blanc": (45.8326, 6.8652, 3500, 4900, 0), "Sniezka": (50.7360, 15.7398, 900, 1700, 0),
                "Baltyk": (55.5, 18.0, MIN_M, -2, 1), "Krakow": (50.0614, 19.9366, 150, 300, None)}
    for nazwa, (la, lo, mn, mx, woda) in kontrola.items():
        h, w = wys(la, lo)
        print(f"  {nazwa}: {h:.0f} m, woda={w}")
        assert mn <= h <= mx, f"{nazwa}: {h} m poza {mn}..{mx}"
        assert woda is None or w == woda, f"{nazwa}: woda={w}, oczekiwano {woda}"

    q = np.round((H - MIN_M) / (MAX_M - MIN_M) * 65535).astype("<u2")
    q.tofile(os.path.join(OUT, "europa_wys.r16"))
    W.tofile(os.path.join(OUT, "europa_woda.r8"))
    kx, ky = do_xy(np.array([KRAKOW[0]]), np.array([KRAKOW[1]]))
    start_x_cm = ky[0] * 100 / 8   # gra: +X = polnoc
    start_y_cm = kx[0] * 100 / 8   # gra: +Y = wschod
    with open(os.path.join(OUT, "europa_wys.txt"), "w") as f:
        f.write(f"{N} {N} {MIN_M} {MAX_M} {start_x_cm:.0f} {start_y_cm:.0f}\n")

    # Podglad: kolory wysokosci + cieniowanie.
    h = H[::8, ::8]; w = W[::8, ::8]
    gy, gx = np.gradient(h)
    cien = np.clip(0.75 + (gx - gy) * 0.004, 0.4, 1.2)[..., None]
    kol = np.interp(h, [0, 200, 600, 1500, 2500, 4000], np.arange(6))
    pal = np.array([[96, 124, 70], [110, 132, 74], [140, 140, 90], [130, 112, 90], [150, 140, 130], [235, 235, 235]], np.float32)
    i0 = np.clip(np.floor(kol).astype(int), 0, 4); t = (kol - i0)[..., None]
    rgb = (pal[i0] * (1 - t) + pal[i0 + 1] * t) * cien
    rgb[w == 1] = [52, 86, 104]; rgb[w == 2] = [60, 96, 116]; rgb[w == 3] = [60, 96, 116]
    os.makedirs(os.path.join(ROOT, "Docs"), exist_ok=True)
    Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8)).save(os.path.join(ROOT, "Docs", "europa_podglad.png"))
    print("GOTOWE. Start Krakow:", round(start_x_cm), round(start_y_cm), "cm")


if __name__ == "__main__":
    main()
