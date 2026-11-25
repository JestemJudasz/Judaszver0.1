# Plik: Tools/plan_osady.py
# Plan osady rodu z gory (bez uruchamiania gry): czyta Saved/Plany/<rod>.txt zapisane przez test grarts2d.AI.UkladOsady
# (AOsadaRodu::ZapiszPlan) i rysuje PNG obok: ulice, mur po obrysie z bramami, domy z ogrodkami, warsztaty, pola za murem.
# Uzycie: python plan_osady.py [katalog_planow]
import glob, math, os, sys
from PIL import Image, ImageDraw

CM_NA_PX = 18.0
TLO, ULICA, MUR, OGROD, PLOT = (86, 104, 62), (139, 112, 76), (88, 84, 80), (110, 128, 70), (92, 74, 52)
KOLORY = {"Kosciol": (226, 222, 210), "Kaplica": (226, 222, 210), "Chata": (176, 140, 96), "Dom": (176, 140, 96),
          "Pole": (196, 178, 104), "Targ": (200, 120, 80), "Magazyn": (120, 96, 70), "Spichlerz": (120, 96, 70)}


def kolor(model):
    for k, c in KOLORY.items():
        if k in model:
            return c
    return (150, 92, 74)  # warsztaty


def prostokat(cx, cy, fx, fy, hx, hy):
    lx, ly = -fy, fx  # w bok od frontu
    return [(cx + lx * a * hx + fx * b * hy, cy + ly * a * hx + fy * b * hy) for a, b in ((-1, -1), (1, -1), (1, 1), (-1, 1))]


def rysuj(plik):
    ulice, obrys, dzialki = [], [], []
    for linia in open(plik, encoding="utf-8"):
        w = linia.split()
        if not w:
            continue
        if w[0] == "U":
            v = list(map(float, w[2:]))
            ulice.append((float(w[1]), list(zip(v[0::2], v[1::2]))))
        elif w[0] == "O":
            v = list(map(float, w[1:]))
            obrys = list(zip(v[0::2], v[1::2]))
        elif w[0] == "D":
            dzialki.append((w[1], *map(float, w[2:8]), w[8] == "1", w[9] == "1"))
    pkt = obrys + [(d[1], d[2]) for d in dzialki]
    x0, x1 = min(p[0] for p in pkt) - 2500, max(p[0] for p in pkt) + 2500
    y0, y1 = min(p[1] for p in pkt) - 2500, max(p[1] for p in pkt) + 2500
    W, H = int((x1 - x0) / CM_NA_PX), int((y1 - y0) / CM_NA_PX)
    img = Image.new("RGB", (W, H), TLO)
    g = ImageDraw.Draw(img)
    px = lambda p: ((p[0] - x0) / CM_NA_PX, (p[1] - y0) / CM_NA_PX)
    for pol, p in ulice:
        g.line([px(q) for q in p], fill=ULICA, width=max(2, int(2 * pol / CM_NA_PX)), joint="curve")
    for m, x, y, fx, fy, hx, hy, ogrod, zewn in dzialki:
        if ogrod:
            g.polygon([px(q) for q in prostokat(x - fx * (hy + 350), y - fy * (hy + 350), fx, fy, hx + 60, 350)], fill=OGROD, outline=PLOT)
    for m, x, y, fx, fy, hx, hy, ogrod, zewn in dzialki:
        g.polygon([px(q) for q in prostokat(x, y, fx, fy, hx, hy)], fill=kolor(m), outline=(50, 40, 32))
    if len(obrys) > 2:
        g.line([px(q) for q in obrys + obrys[:1]], fill=MUR, width=max(3, int(250 / CM_NA_PX)), joint="curve")
    wyj = os.path.splitext(plik)[0] + ".png"
    img.save(wyj)
    return wyj, len(dzialki), sum(1 for d in dzialki if d[-1])


if __name__ == "__main__":
    kat = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "Saved", "Plany")
    for plik in sorted(glob.glob(os.path.join(kat, "*.txt"))):
        print(*rysuj(plik))
