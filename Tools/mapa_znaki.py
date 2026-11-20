# Plik: Tools/mapa_znaki.py
# Znaki mapy papierowej narysowane "jak przez dziecko" (gory, pagorki, drzewa liisciaste i iglaste) i dane, gdzie je stawiac.
#   Art/Teren/T_MapaZnaki.png  1024x1024 RGBA (sRGB) - atlas 4x4 znakow po 256 px, kontur tuszem, plaskie kolory
#   Art/Teren/T_MapaDane.png   4096x4096 RGBA (liniowe) - dane calej Europy (1 px = 128 m w grze = 1 km prawdziwy):
#       R = rzezba: (max - min wysokosci w promieniu 3 px) / 1600 m   (0,12 pagorki, 0,3 gory, 0,7 osniezone)
#       G = udzial lasu, B = udzial lasu iglastego (B/G = iglaste wsrod lasu), A = lad (1) / morze i jeziora (0)
#   Docs/mapa_znaki_podglad*.png - podglad ukladu znakow liczony tym samym wzorem co M_Teren (bez Unreala)
# M_Teren (Tools/ue_teren.py) rysuje znaki w siatce trojkatnej, ktorej punkty zostaja w tym samym miejscu przy kazdym
# przyblizeniu: przy przyblizaniu nowe znaki wyrastaja miedzy starymi (plynnie), nic sie nie podmienia ani nie doczytuje.
# Uruchomienie: python Tools/mapa_znaki.py  (numpy, pillow; potem usun Content/Swiat/Teren/T_MapaZnaki.uasset
# i T_MapaDane.uasset, jesli sa, i uruchom Tools\Zbuduj_Teren.bat).
import math, os, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(ROOT, "Art", "Teren")
N = 8192
M = 4096

# Kolory (sRGB). Tusz ciemnobrazowy, wypelnienia kredkowe, stonowane jak reszta mapy.
TUSZ = (58, 46, 38)
GORA = (196, 178, 150)
GORA_CIEN = (150, 132, 108)
SNIEG = (246, 246, 242)
SNIEG_CIEN = (206, 214, 222)
PAGOREK = (168, 176, 112)
PAGOREK_CIEN = (132, 142, 86)
KORONA = (120, 160, 82)
KORONA_CIEN = (86, 124, 60)
IGLAK = (70, 118, 78)
IGLAK_CIEN = (48, 88, 58)
PIEN = (120, 84, 56)

# Uklad atlasu (indeks = wiersz * 4 + kolumna). Ten sam uklad zna M_Teren.
ATLAS = {
    "gora": [0, 1, 2, 3],
    "snieg": [4, 5, 6],
    "pagorek": [7, 8, 15],
    "lisc": [9, 10, 11],
    "iglak": [12, 13, 14],
}
DOL = 0.92  # dol znaku w komorce (od gory), znak zajmuje szerokosc komorki


# ---------------------------------------------------------------- atlas

def chwiej(pkt, rng, amp, gestosc=10):
    """Zageszcza lamana i lekko ja faluje - kreska jak reka dziecka."""
    out = []
    faza = rng.uniform(0, 6.28, 3)
    dl = 0.0
    for (x0, y0), (x1, y1) in zip(pkt[:-1], pkt[1:]):
        n = max(2, int(math.hypot(x1 - x0, y1 - y0) / gestosc))
        for k in range(n):
            t = k / n
            x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
            s = dl + math.hypot(x1 - x0, y1 - y0) * t
            d = amp * (0.6 * math.sin(s / 37 + faza[0]) + 0.4 * math.sin(s / 13 + faza[1]))
            out.append((x + d, y + d * 0.7 * math.sin(s / 23 + faza[2])))
        dl += math.hypot(x1 - x0, y1 - y0)
    out.append(pkt[-1])
    return out


def ksztalt(d, pkt, wyp, gr, rng, amp=6, zamkn=True):
    p = chwiej(pkt + ([pkt[0]] if zamkn else []), rng, amp)
    if wyp is not None:
        d.polygon(p, fill=wyp)
    d.line(p, fill=TUSZ, width=gr, joint="curve")
    for x, y in (p[0], p[-1]):
        d.ellipse([x - gr / 2, y - gr / 2, x + gr / 2, y + gr / 2], fill=TUSZ)


def gora(d, cx, dol, szer, wys, rng, snieg, sx):
    gr = 26
    lx, px_ = cx - szer / 2, cx + szer / 2
    szczyt = (cx + sx, dol - wys)
    # cien: prawa polowa
    d.polygon(chwiej([szczyt, (px_, dol), (szczyt[0] + szer * 0.08, dol), szczyt], rng, 4), fill=GORA_CIEN)
    ksztalt(d, [(lx, dol), szczyt, (px_, dol)], None, gr, rng)
    # wypelnienie lewej czesci (pod kontur)
    if snieg:
        t = 0.42
        a = (szczyt[0] + (lx - szczyt[0]) * t, szczyt[1] + (dol - szczyt[1]) * t)
        b = (szczyt[0] + (px_ - szczyt[0]) * t, szczyt[1] + (dol - szczyt[1]) * t)
        zyg = [a]
        for k in range(1, 4):
            x = a[0] + (b[0] - a[0]) * k / 4
            zyg.append((x, a[1] + (wys * 0.12 if k % 2 else -wys * 0.02)))
        zyg.append(b)
        czapa = [szczyt, b] + zyg[::-1][1:-1] + [a]
        d.polygon(czapa, fill=SNIEG)
        d.polygon([szczyt, b, zyg[3], (szczyt[0] + 4, szczyt[1] + wys * 0.3)], fill=SNIEG_CIEN)
        d.line(chwiej(zyg, rng, 3), fill=TUSZ, width=gr - 8)
    # ponowny kontur na wierzchu
    ksztalt(d, [(lx, dol), szczyt, (px_, dol)], None, gr, rng, amp=4, zamkn=False)
    # kreseczki zbocza
    for k in range(2):
        t = 0.45 + 0.22 * k
        x = szczyt[0] + (px_ - szczyt[0]) * t - szer * 0.1
        y = szczyt[1] + (dol - szczyt[1]) * t
        d.line([(x, y), (x + szer * 0.07, y + wys * 0.12)], fill=TUSZ, width=14)


def wypelnij_gore(d, cx, dol, szer, wys, szczyt_dx=0.0):
    d.polygon([(cx - szer / 2, dol), (cx + szczyt_dx, dol - wys), (cx + szer / 2, dol)], fill=GORA)


def rysuj_gore(d, cx, dol, szer, wys, rng, snieg=False):
    sx = rng.uniform(-0.04, 0.04) * szer
    wypelnij_gore(d, cx, dol, szer, wys, sx)
    gora(d, cx, dol, szer, wys, np.random.default_rng(rng.integers(1 << 30)), snieg, sx)


def pagorek(d, cx, dol, szer, wys, rng):
    pkt = [(cx - szer / 2 + szer * t, dol - wys * math.sin(math.pi * t) ** 0.8) for t in np.linspace(0, 1, 24)]
    d.polygon(pkt, fill=PAGOREK)
    d.polygon([(cx + szer * 0.1, dol)] + [p for p in pkt if p[0] > cx + szer * 0.1], fill=PAGOREK_CIEN)
    ksztalt(d, pkt, None, 24, rng, amp=4, zamkn=False)
    for k in range(3):
        x = cx - szer * 0.22 + k * szer * 0.16
        y = dol - wys * 0.35 - (k % 2) * wys * 0.12
        d.line([(x, y), (x + 10, y - 26)], fill=TUSZ, width=10)


def drzewo(d, cx, dol, r, rng):
    pien_w, pien_h = r * 0.32, r * 0.75
    d.rectangle([cx - pien_w / 2, dol - pien_h, cx + pien_w / 2, dol], fill=PIEN)
    ksztalt(d, [(cx - pien_w / 2, dol - pien_h * 0.6), (cx - pien_w / 2, dol), (cx + pien_w / 2, dol),
                (cx + pien_w / 2, dol - pien_h * 0.6)], None, 18, rng, amp=2, zamkn=False)
    sy = dol - pien_h - r * 0.8
    pkt = []
    for k in range(40):
        a = 2 * math.pi * k / 40
        rr = r * (1 + 0.1 * math.sin(a * 6 + rng.uniform(0, 1)))
        pkt.append((cx + rr * math.cos(a), sy + rr * 0.95 * math.sin(a)))
    d.polygon(pkt, fill=KORONA)
    d.pieslice([cx - r * 0.95, sy - r * 0.9, cx + r * 0.95, sy + r * 0.9], -60, 120, fill=KORONA_CIEN)
    d.ellipse([cx - r * 0.55, sy - r * 0.6, cx - r * 0.1, sy - r * 0.2], fill=(150, 186, 104))
    ksztalt(d, pkt, None, 20, rng, amp=3)


def iglak(d, cx, dol, szer, wys, rng):
    pien_w = szer * 0.16
    d.rectangle([cx - pien_w / 2, dol - wys * 0.18, cx + pien_w / 2, dol], fill=PIEN)
    ksztalt(d, [(cx - pien_w / 2, dol - wys * 0.15), (cx - pien_w / 2, dol), (cx + pien_w / 2, dol),
                (cx + pien_w / 2, dol - wys * 0.15)], None, 16, rng, amp=2, zamkn=False)
    pietra = 3
    for k in range(pietra):
        y0 = dol - wys * 0.14 - k * wys * 0.25
        s = szer * (1 - k * 0.24) / 2
        szczyt = (cx, y0 - wys * 0.42)
        tr = [(cx - s, y0), szczyt, (cx + s, y0)]
        d.polygon(tr, fill=IGLAK)
        d.polygon([szczyt, (cx + s, y0), (cx + s * 0.15, y0)], fill=IGLAK_CIEN)
        ksztalt(d, tr, None, 18, rng, amp=3)


def komorka(rodzaj, wariant, n=1024):
    img = Image.new("RGBA", (n, n), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    rng = np.random.default_rng(1000 + wariant * 17 + hash(rodzaj) % 997)
    dol = n * DOL
    if rodzaj in ("gora", "snieg"):
        sn = rodzaj == "snieg"
        if wariant == (2 if sn else 3):
            rysuj_gore(d, n * 0.36, dol - n * 0.04, n * 0.56, n * 0.5, rng, sn)   # tylna, nizsza
            rysuj_gore(d, n * 0.58, dol, n * 0.7, n * 0.68, rng, sn)
        else:
            rysuj_gore(d, n * 0.5, dol, n * 0.8, n * (0.74 if wariant == 0 else 0.62), rng, sn)
    elif rodzaj == "pagorek":
        if wariant == 0:
            pagorek(d, n * 0.5, dol, n * 0.84, n * 0.36, rng)
        else:
            pagorek(d, n * 0.36, dol - n * 0.03, n * 0.6, n * 0.3, rng)
            pagorek(d, n * 0.6, dol, n * 0.7, n * 0.36, rng)
    elif rodzaj == "lisc":
        poz = [[(0.5, 0, 0.3)], [(0.33, -0.05, 0.22), (0.64, 0, 0.25)], [(0.5, -0.1, 0.2), (0.27, 0, 0.21), (0.72, 0.01, 0.22)]][wariant]
        for x, dy, r in poz:
            drzewo(d, n * x, dol + n * dy, n * r, rng)
    elif rodzaj == "iglak":
        poz = [[(0.5, 0, 0.5, 0.86)], [(0.34, -0.05, 0.38, 0.7), (0.64, 0, 0.42, 0.8)],
               [(0.5, -0.1, 0.32, 0.6), (0.26, 0, 0.34, 0.66), (0.72, 0.01, 0.36, 0.72)]][wariant]
        for x, dy, s, h in poz:
            iglak(d, n * x, dol + n * dy, n * s, n * h, rng)
    return img


def atlas():
    out = Image.new("RGBA", (1024, 1024), (0, 0, 0, 0))
    for rodzaj, idx in ATLAS.items():
        for w, k in enumerate(idx):
            c = komorka(rodzaj, w).resize((256, 256), Image.LANCZOS)
            out.paste(c, ((k % 4) * 256, (k // 4) * 256), c)
    # Pod przezroczystymi pikselami kolor tuszu (mipmapy i filtrowanie nie robia jasnych obwodek).
    a = np.asarray(out).copy()
    pusty = a[..., 3] == 0
    a[pusty, 0:3] = TUSZ
    return Image.fromarray(a)


# ---------------------------------------------------------------- dane

def filtr(a, r, fn):
    o = a.copy()
    for ax in (0, 1):
        b = o.copy()
        for k in range(1, r + 1):
            b = fn(b, np.roll(o, k, ax)); b = fn(b, np.roll(o, -k, ax))
        o = b
    return o


def dane():
    H = np.fromfile(os.path.join(ROOT, "Data", "europa_wys.r16"), "<u2").reshape(N, N).astype(np.float32) / 65535 * 5100 - 200
    P = np.fromfile(os.path.join(ROOT, "Data", "europa_pokrycie.r8"), np.uint8).reshape(N, N)
    W = np.fromfile(os.path.join(ROOT, "Data", "europa_woda.r8"), np.uint8).reshape(N, N)
    pol = lambda a: a.reshape(M, 2, M, 2).mean((1, 3))
    H4 = pol(H)
    rzezba = np.clip((filtr(H4, 3, np.maximum) - filtr(H4, 3, np.minimum)) / 1600.0, 0, 1)
    las_k = np.array([0, 0, 0, 1, 1, 1, 0.5, 0, 0, 0, 0], np.float32)
    igl_k = np.array([0, 0, 0, 0, 1, 0.5, 0.25, 0, 0, 0, 0], np.float32)
    las = pol(las_k[P])
    igl = pol(np.maximum(igl_k[P], las_k[P] * (H > 1100)))
    lad = pol(((W == 0) | (W == 3)).astype(np.float32))
    return np.stack([rzezba, las, igl, lad], -1)


# ---------------------------------------------------------------- uklad znakow (ten sam wzor co M_Teren)

GESTOSC = 16.0   # oczek siatki znakow na szerokosc widoku (= ATerenKafle::ZnakiGestosc)
WIELKOSC = {"gora": 0.95, "snieg": 1.0, "pagorek": 0.62, "lisc": 0.5, "iglak": 0.5}


def mieszaj(i, j, k):
    """Ten sam hash co w M_Teren (uint32)."""
    m = 0xFFFFFFFF
    h = ((i * 73856093) ^ (j * 19349663) ^ (k * 83492791)) & m
    h ^= h >> 13
    h = (h * 1274126177) & m
    h ^= h >> 16
    return (h & 0xFFFF) / 65536.0


def podglad(D, A, srodek_uv, szer_uv, plik, px=960, py=600):
    """Rysuje widok mapy papierowej (z gory, polnoc w gore) jak M_Teren - do oceny bez Unreala."""
    t = math.log2(GESTOSC / szer_uv)
    n0 = math.floor(t); f = t - n0
    L = n0 + 1
    s = 2.0 ** -L
    U = 2.0 ** -t
    img = Image.new("RGB", (px, py), (150, 147, 140))
    skala = px / szer_uv
    u0, v0 = srodek_uv[0] - szer_uv / 2, srodek_uv[1] - szer_uv * py / px / 2
    # tlo: plamy lasu i gor (jak w materiale, z danych)
    uu = u0 + (np.arange(px) + 0.5) / skala
    vv = v0 + (np.arange(py) + 0.5) / skala
    ci = np.clip((vv * M).astype(int), 0, M - 1)[:, None]
    cj = np.clip((uu * M).astype(int), 0, M - 1)[None, :]
    d = D[ci, cj]
    tlo = np.array([150, 147, 140], np.float32) * np.ones((py, px, 1))
    las = np.clip((d[..., 1] - 0.3) / 0.3, 0, 1)[..., None]
    tlo = tlo * (1 - 0.45 * las) + np.array([150, 168, 120]) * 0.45 * las
    gor = np.clip((d[..., 0] - 0.15) / 0.2, 0, 1)[..., None]
    tlo = tlo * (1 - 0.35 * gor) + np.array([176, 160, 132]) * 0.35 * gor
    tlo[d[..., 3] < 0.5] = (92, 112, 128)
    img = Image.fromarray(tlo.astype(np.uint8))
    # punkty siatki trojkatnej poziomu L
    jj = np.arange(math.floor(v0 / (0.866 * s)) - 2, math.ceil((v0 + szer_uv) / (0.866 * s)) + 2)
    znaki = []
    for j in jj:
        i_od = math.floor(u0 / s - 0.5 * j) - 2
        for i in range(i_od, i_od + int(szer_uv / s) + 5):
            tz = 32
            for q in (i, j):
                q = int(q)
                if q != 0:
                    tz = min(tz, (q & -q).bit_length() - 1)
            narodz = max(0, L - tz)
            alfa = 1.0 if narodz <= n0 else min(1, max(0, (f - 0.1) / 0.7))
            if alfa <= 0:
                continue
            pu, pv = (i + 0.5 * j) * s, j * 0.866 * s
            # wspolrzedne niezalezne od poziomu: (i, j) przeliczone na poziom narodzin
            ki, kj = int(i) >> (L - narodz), int(j) >> (L - narodz)
            h0, h1, h2, h3 = (mieszaj(ki, kj, narodz * 7 + k) for k in range(4))
            pu += (h1 - 0.5) * 0.6 * U; pv += (h2 - 0.5) * 0.6 * U
            # dane usrednione z ~1/4 oczka punktu (staly dla punktu, wiec znak nie zmienia sie przy zoomie)
            mip = max(0, min(len(A) - 1, int(round(math.log2(max(1.0, 2.0 ** -narodz * M)) - 2))))
            a = A[mip]; m_ = a.shape[0]
            dd = a[min(m_ - 1, max(0, int(pv * m_))), min(m_ - 1, max(0, int(pu * m_)))]
            d0 = D[min(M - 1, max(0, int(pv * M))), min(M - 1, max(0, int(pu * M)))]
            if dd[3] < 0.5 or d0[3] < 0.5:
                continue
            rz = dd[0] * (1 + 0.1 * mip)   # usrednianie obniza rzezbe w grubych mipach
            if rz > 0.3 and h0 < (rz - 0.25) / 0.15:
                rodz = "snieg" if rz > 0.7 else "gora"
            elif rz > 0.15 and h0 < (rz - 0.13) / 0.1 * 0.8:
                rodz = "pagorek"
            elif dd[1] > 0.3 and h0 < min(0.75, (dd[1] - 0.3) / 0.4):
                rodz = "iglak" if h3 < dd[2] / max(dd[1], 1e-3) else "lisc"
            else:
                continue
            war = ATLAS[rodz][int(h3 * 7.919 * len(ATLAS[rodz])) % len(ATLAS[rodz])]
            g = WIELKOSC[rodz] * U * (0.75 + 0.25 * alfa) * (0.9 + 0.2 * h2)
            znaki.append((pv, pu, g, war, alfa))
    at = Image.open(os.path.join(ART, "T_MapaZnaki.png"))
    for pv, pu, g, war, alfa in sorted(znaki):
        w = max(2, int(g * skala))
        c = at.crop(((war % 4) * 256, (war // 4) * 256, (war % 4 + 1) * 256, (war // 4 + 1) * 256)).resize((w, w), Image.LANCZOS)
        if alfa < 1:
            c.putalpha(c.getchannel("A").point(lambda x: int(x * alfa)))
        x = int((pu - u0) * skala - w / 2); y = int((pv - v0) * skala - w * DOL)
        img.paste(c, (x, y), c)
    img.save(plik)
    print(plik, len(znaki), "znakow")


def mipy(D):
    A = [D]
    while A[-1].shape[0] > 1:
        a = A[-1]; m_ = a.shape[0] // 2
        A.append(a.reshape(m_, 2, m_, 2, 4).mean((1, 3)))
    return A


def main():
    os.makedirs(ART, exist_ok=True)
    atlas().save(os.path.join(ART, "T_MapaZnaki.png"))
    print("T_MapaZnaki.png")
    sciezka = os.path.join(ART, "T_MapaDane.png")
    if "podglad" in sys.argv and os.path.exists(sciezka):
        D = np.asarray(Image.open(sciezka), np.float32) / 255
    else:
        D = dane()
        Image.fromarray((D * 255 + 0.5).astype(np.uint8)).save(sciezka)
        D = np.asarray(Image.open(sciezka), np.float32) / 255
        print("T_MapaDane.png")
    A = mipy(D)
    # Krakow-Tatry ~ u 0.6, v 0.62 (sprawdzane przy tworzeniu); Alpy; cala Europa
    for nazwa, c, w in (("europa", (0.5, 0.5), 1.0), ("alpy", (0.43, 0.72), 0.25), ("tatry", (0.6, 0.63), 0.06), ("blisko", (0.6, 0.63), 0.012)):
        podglad(D, A, c, w, os.path.join(ROOT, "Docs", f"mapa_znaki_podglad_{nazwa}.png"))


if __name__ == "__main__":
    main()
