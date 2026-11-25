# Plik: Tools/budynki.py
# Budynki robione przez AI -> Art/Budynki/*.glb + tekstury T_Bud_Kamien / T_Bud_Dach / T_Bud_Drewno.
# Wzor od uzytkownika (20.11.2026): kamienny kosciol z wieza, ogladany z gory pod katem, budowany w fazach:
#   F1 fundament i niskie mury w rusztowaniu, F2 wyzsze mury i wieza w rusztowaniu, F3 gola wiezba dachu,
#   F4 dach czesciowo kryty, F5 gotowy kosciol z krzyzem. Przy budowie lezy material (kamienie, skrzynie, beczki, belki).
# Kazda faza to osobny model SM_<Budynek>_F<n>; gra (ABudowa) podmienia je wedlug postepu budowy.
# Materialy w modelu: "Kamien", "Dach" (gont), "Drewno", "Tynk", "Strzecha" (tekstura kafelkowa x kolor wierzcholkow).
# UV juz w kaflach tekstury.
# Domy mieszkalne (20.11.2026, prosba uzytkownika: kilka rodzajow domow budowanych jak kosciol przez rodziny osadnikow):
#   SM_Chata (bale + strzecha), SM_DomPruski (mur pruski + strzecha), SM_DomDuzy (kamienny parter, pietro pruskie, gont),
#   po 4 fazy. Oboz startowy: SM_Namiot, SM_Ognisko (bez faz). Ikony paska budowy: Tools/ikony.py.
# Budynki drewna (21.11.2026, etap 1 ekonomii): SM_PlacSkladowy, SM_ChataDrwala, SM_ChataBorowego, SM_Pilarnia (po 4 fazy),
#   stosy towaru SM_Stos_<Kloda|Deska|Kamien>_1..4 (gra pokazuje je przy budynkach wedlug zapasu).
# Kamien i jedzenie (21.11.2026, etap 2 ekonomii): SM_Kamieniolom, SM_ChataZbieracza, SM_ChataMysliwego, SM_Spichlerz
#   (po 4 fazy), stosy jedzenia SM_Stos_Jedzenie_1..4 (worki, kosze, beczka).
# Zboze i chleb (21.11.2026, etap 3 ekonomii): SM_Pole (2 fazy: paliki, ogrodzony ugor) i wyglad uprawy SM_Pole_<Zaorane|
#   Zasiane|Rosnie|Dojrzale|Scierniska>; SM_Mlyn (wiatrak kozlak) + SM_Mlyn_Skrzydla, SM_Piekarnia (piec chlebowy),
#   SM_TartakWodny + SM_TartakWodny_Kolo (czesci ruchome: os przez poczatek ukladu, gra kreci nimi), stosy SM_Stos_Zboze /
#   SM_Stos_Maka_1..4.
# Owce, tkacz, kuznia (21.11.2026, etap 4 ekonomii): SM_Owczarnia (szopa i pastwisko) + SM_Owca / SM_Owca_Strzyzona,
#   SM_ChataTkacza (krosno pod wiata), SM_KopalniaRudy (sztolnia w skale), SM_Kuznia (palenisko, kowadlo), SM_Zbrojownia
#   (plac cwiczen z manekinami), stosy SM_Stos_<Welna|Sukno|Ruda|Bron>_1..4.
# Poziomy (22.11.2026): dom = chata (1) -> dom pruski (2) -> duzy dom (3), po 4 warianty z kominami (SM_<Dom>_W<w>_F<n>);
#   kosciol = drewniany (1) -> kamienny (2) -> wielki z placem (3). Ulepszenie: SM_<Budynek>[_W<w>]_U.
# Targ (21.11.2026, etap 5 ekonomii): SM_Targ - plac z trzema kramami pod plociennymi daszkami i waga kupiecka.
# Uklad przy budowaniu: X = wzdluz nawy (wieza na -X), Y = w poprzek, Z = gora; metry. Zapis glTF: Y = gora.
# Import do UE: Tools/Zbuduj_Budynki.bat.
import json, os, struct, zlib
import numpy as np
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Art", "Budynki")
KAMIEN, DACH, DREWNO, TYNK, STRZECHA = 0, 1, 2, 3, 4
NAZWY_MAT = ["Kamien", "Dach", "Drewno", "Tynk", "Strzecha"]
KAFEL = {KAMIEN: 2.0, DACH: 2.0, DREWNO: 1.0, TYNK: 2.0, STRZECHA: 2.0}  # metry na jeden kafel tekstury


def rgb(r, g, b):
    return np.array([r, g, b], float) / 255.0


def lin(c):
    c = np.clip(c, 0, 1)
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


# Kolory (sRGB, przed lin): jasny wapien, ciemny gont, swieze i stare drewno.
MUR = rgb(222, 210, 188)  # cieply wapien (przy chlodnym swietle nieba szary wychodzil niebieskawy)
MUR_COKOL = rgb(168, 160, 148)
GONT = rgb(118, 104, 92)
BELKA = rgb(186, 150, 104)
RUSZT = rgb(128, 100, 70)
DESKA = rgb(160, 132, 96)
DRZWI = rgb(92, 58, 34)
OKNO = rgb(40, 38, 42)
KRZYZ = rgb(70, 60, 50)


# ---------------- tekstury (kafelkowe, 256 px) ----------------

def tekstura_kamien(n=256, ss=2):
    """Ciosy kamienne w rzedach. Kafel 2 x 2 m: 4 rzedy po 50 cm, kamienie 40-90 cm. Duze roznice jasnosci kamieni:
    przy efekcie pikseli (~5 px na metr) cienkie spoiny znikaja, widac tylko mozaike jasniejszych i ciemniejszych ciosow."""
    rng = np.random.default_rng(11)
    N = n * ss
    img = Image.new("RGB", (N, N), (128, 124, 116))  # zaprawa
    d = ImageDraw.Draw(img)
    rzedy = 4
    h = N / rzedy
    for r in range(rzedy):
        x = rng.uniform(0, N)
        start = x
        while x < start + N:
            w = rng.uniform(0.2, 0.45) * N
            w = min(w, start + N - x)
            j = rng.uniform(0.72, 1.0)
            c = tuple(int(v * j) for v in (236, 233, 226))
            for off in (-N, 0, N):  # zawijanie w poziomie
                x0, x1 = x + off + ss * 2.5, x + w + off - ss * 2.5
                d.rectangle([x0, r * h + ss * 2.5, x1, (r + 1) * h - ss * 2.5], fill=c)
                # cien na dolnej krawedzi kamienia
                d.rectangle([x0, (r + 1) * h - ss * 4, x1, (r + 1) * h - ss * 1.5], fill=tuple(int(v * 0.88) for v in c))
            x += w
    a = np.asarray(img).astype(np.float32)
    a *= rng.uniform(0.96, 1.04, (N, N, 1))  # ziarno
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8)).resize((n, n), Image.LANCZOS)


def tekstura_dach(n=256, ss=2):
    """Gont: 8 rzedow na kafel 2 m (25 cm), mijankowe szczeliny, ciemna kreska cienia pod kazdym rzedem. V rosnie w dol polaci."""
    rng = np.random.default_rng(12)
    N = n * ss
    img = Image.new("RGB", (N, N), (60, 56, 52))
    d = ImageDraw.Draw(img)
    rzedy = 8
    h = N / rzedy
    for r in range(rzedy):
        x = (r % 2) * N / 16
        szer = N / 8
        k = 0
        while k < 8:
            w = szer * rng.uniform(0.7, 1.3) if k < 7 else None
            j = rng.uniform(0.7, 1.1)
            c = tuple(int(v * j) for v in (232, 226, 218))
            x1 = x + (w if w else szer)
            for off in (-N, 0, N):
                d.rectangle([x + off + ss, r * h, x1 + off - ss, (r + 1) * h - ss * 3], fill=c)
            x = x1
            k += 1
    a = np.asarray(img).astype(np.float32)
    a *= rng.uniform(0.95, 1.05, (N, N, 1))
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8)).resize((n, n), Image.LANCZOS)


def tekstura_drewno(n=128):
    """Slojowanie wzdluz U (kafel 1 m)."""
    rng = np.random.default_rng(13)
    y = np.arange(n)[:, None]
    x = np.arange(n)[None, :]
    fala = np.sin(2 * np.pi * (y / n * 9 + 0.15 * np.sin(2 * np.pi * x / n * 2))) * 0.06
    szum = rng.uniform(-0.04, 0.04, (n, n))
    j = 0.9 + fala + szum
    a = np.stack([j * 235, j * 228, j * 220], -1)
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8))


def tekstura_tynk(n=256):
    """Bielony tynk: prawie gladki, duze plamy (przy pikselach widac tylko je) i drobne ziarno. Kafel 2 m."""
    rng = np.random.default_rng(14)
    m = 8
    gr = np.tile(rng.uniform(0.9, 1.0, (m, m)), (3, 3))  # 3 x 3 kafle i wyciecie srodka = szew bez skoku
    gr = np.asarray(Image.fromarray((gr * 255).astype(np.uint8)).resize((3 * n, 3 * n), Image.BICUBIC)).astype(np.float32)[n:2 * n, n:2 * n] / 255
    j = gr * rng.uniform(0.97, 1.03, (n, n))
    a = np.stack([j * 240, j * 236, j * 228], -1)
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8))


def tekstura_strzecha(n=256, ss=2):
    """Strzecha: 5 warstw na kafel 2 m (40 cm), w kazdej pionowe pasma slomy roznej jasnosci (8-15 cm, zeby przetrwaly
    pikselizacje), dolna krawedz warstwy ciemniejsza (cien okapu warstwy). V rosnie w dol polaci."""
    rng = np.random.default_rng(15)
    N = n * ss
    img = Image.new("RGB", (N, N), (150, 140, 122))
    d = ImageDraw.Draw(img)
    rzedy = 5
    h = N / rzedy
    for r in range(rzedy):
        x = rng.uniform(0, N / 10)
        start = x
        while x < start + N:
            w = min(rng.uniform(0.035, 0.075) * N, start + N - x)
            j = rng.uniform(0.78, 1.08)
            c = tuple(int(v * j) for v in (236, 228, 212))
            # pasmo slomy: lekko wysuniete w dol (postrzepiona krawedz warstwy)
            dl = rng.uniform(0.0, 0.12) * h
            for off in (-N, 0, N):
                d.rectangle([x + off, r * h, x + w + off - ss, (r + 1) * h - ss * 6 + dl], fill=c)
            x += w
    a = np.asarray(img).astype(np.float32)
    # przyciemnienie dolnej czesci kazdej warstwy (gradient) - warstwy czytelne nawet w pikselach
    v = (np.arange(N) % h) / h
    a *= (1.0 - 0.28 * np.clip((v - 0.6) / 0.4, 0, 1) ** 1.5)[:, None, None]
    a *= rng.uniform(0.95, 1.05, (N, N, 1))
    return Image.fromarray(np.clip(a, 0, 255).astype(np.uint8)).resize((n, n), Image.LANCZOS)


# ---------------- siatka i zapis glTF ----------------

class Siatka:
    """nazwy: materialy (sloty) modelu; kolor wierzcholka RGB (sRGB 0..1) albo RGBA (A = maska dla materialu, np. tunika)."""
    def __init__(self, nazwy=None):
        self.nazwy = NAZWY_MAT if nazwy is None else nazwy
        self.grupy = {k: [] for k in range(len(self.nazwy))}

    def dodaj(self, mat, p, n, c, i, uv):
        p = np.asarray(p, float)
        c = np.asarray(c, float)
        if c.ndim == 1:
            c = np.tile(c, (len(p), 1))
        if c.shape[1] == 3:
            c = np.concatenate([c, np.ones((len(p), 1))], 1)
        self.grupy[mat].append((p, np.asarray(n, float), c, np.asarray(i, np.int64), np.asarray(uv, float)))

    def zapisz(self, plik):
        bloby, widoki, akcesory, prymitywy, tr = [], [], [], [], 0
        sloty = []

        def blob(arr, typ, komp, minmax=False):
            b = arr.tobytes(); off = sum(len(x) for x in bloby)
            bloby.append(b + b"\0" * (-len(b) % 4))
            widoki.append({"buffer": 0, "byteOffset": off, "byteLength": len(b)})
            a = {"bufferView": len(widoki) - 1, "componentType": komp, "count": len(arr), "type": typ}
            if minmax:
                a["min"], a["max"] = arr.min(0).tolist(), arr.max(0).tolist()
            akcesory.append(a)
            return len(akcesory) - 1

        for mat, czesci in self.grupy.items():
            if not czesci:
                continue
            baza, P, Nn, C, I, UV = 0, [], [], [], [], []
            for p, n, c, i, uv in czesci:
                P.append(p); Nn.append(n); C.append(c); I.append(i + baza); UV.append(uv); baza += len(p)
            P = np.concatenate(P); Nn = np.concatenate(Nn)
            # Z-gora (budowanie) -> glTF Y-gora: (x, y, z) -> (x, z, -y)
            P = np.stack([P[:, 0], P[:, 2], -P[:, 1]], 1).astype(np.float32)
            Nn = np.stack([Nn[:, 0], Nn[:, 2], -Nn[:, 1]], 1).astype(np.float32)
            Nn /= np.linalg.norm(Nn, axis=1, keepdims=True) + 1e-9
            C = np.clip(np.concatenate(C), 0, 1)
            C4 = np.concatenate([lin(C[:, :3]), C[:, 3:4]], 1).astype(np.float32)
            I = np.concatenate(I).astype(np.uint32).ravel(); UV = np.concatenate(UV).astype(np.float32)
            tr += len(I) // 3
            sloty.append(mat)
            prymitywy.append({"attributes": {"POSITION": blob(P, "VEC3", 5126, True), "NORMAL": blob(Nn, "VEC3", 5126),
                                             "COLOR_0": blob(C4, "VEC4", 5126), "TEXCOORD_0": blob(UV, "VEC2", 5126)},
                              "indices": blob(I, "SCALAR", 5125), "material": mat})
        bufor = b"".join(bloby)
        gltf = {
            "asset": {"version": "2.0", "generator": "grarts2d budynki.py"},
            "scene": 0, "scenes": [{"nodes": [0]}],
            "nodes": [{"mesh": 0, "name": os.path.splitext(os.path.basename(plik))[0]}],
            "materials": [{"name": n} for n in self.nazwy],
            "meshes": [{"primitives": prymitywy}],
            "buffers": [{"byteLength": len(bufor)}],
            "bufferViews": widoki, "accessors": akcesory,
        }
        js = json.dumps(gltf).encode(); js += b" " * (-len(js) % 4)
        with open(plik, "wb") as f:
            f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(bufor)))
            f.write(struct.pack("<II", len(js), 0x4E4F534A) + js)
            f.write(struct.pack("<II", len(bufor), 0x004E4942) + bufor)
        return tr


# ---------------- bryly ----------------

def _uv_scian(p, n, mat):
    """UV w metrach / kafel: U poziomo wzdluz sciany, V w dol (tekstury maja gore u gory)."""
    n = n / (np.linalg.norm(n) + 1e-9)
    if abs(n[2]) > 0.7:  # poziome: rzut z gory
        u, v = p[:, 0], -p[:, 1]
    else:
        osU = np.cross([0, 0, 1.0], n); osU /= np.linalg.norm(osU)
        osV = np.cross(n, osU)  # w gore po scianie / polaci
        u, v = p @ osU, -(p @ osV)
    return np.stack([u, v], 1) / KAFEL.get(mat, 1.0)


def wielokat(s, mat, pts, kolor, n=None, od=None):
    """Plaski wielokat wypukly. Kolejnosc punktow dowolna wzgledem strony: n = zadana normalna albo od = punkt wewnatrz bryly
    (normalna od niego); punkty sa wtedy odwracane tak, by trojkaty byly przeciwnie do zegara patrzac od strony normalnej."""
    pts = np.asarray(pts, float)
    nw = np.cross(pts[1] - pts[0], pts[2] - pts[0])
    if od is not None:
        n = pts.mean(0) - np.asarray(od, float)
    if n is not None and nw @ n < 0:
        pts = pts[::-1]
        nw = -nw
    n = nw / (np.linalg.norm(nw) + 1e-9)
    idx = [[0, k, k + 1] for k in range(1, len(pts) - 1)]
    s.dodaj(mat, pts, np.tile(n, (len(pts), 1)), kolor, idx, _uv_scian(pts, n, mat))


def pudlo(s, mat, a, b, kolor, bez_dolu=True):
    """Prostopadloscian osiowy od rogu a do b."""
    (x0, y0, z0), (x1, y1, z1) = a, b
    if x1 - x0 <= 1e-4 or y1 - y0 <= 1e-4 or z1 - z0 <= 1e-4:
        return
    v = lambda x, y, z: (x, y, z)
    sciany = [
        [v(x1, y0, z0), v(x1, y1, z0), v(x1, y1, z1), v(x1, y0, z1)],  # +X
        [v(x0, y1, z0), v(x0, y0, z0), v(x0, y0, z1), v(x0, y1, z1)],  # -X
        [v(x1, y1, z0), v(x0, y1, z0), v(x0, y1, z1), v(x1, y1, z1)],  # +Y
        [v(x0, y0, z0), v(x1, y0, z0), v(x1, y0, z1), v(x0, y0, z1)],  # -Y
        [v(x0, y0, z1), v(x1, y0, z1), v(x1, y1, z1), v(x0, y1, z1)],  # gora
    ]
    if not bez_dolu:
        sciany.append([v(x0, y1, z0), v(x1, y1, z0), v(x1, y0, z0), v(x0, y0, z0)])
    w = ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)
    for f in sciany:
        wielokat(s, mat, f, kolor, od=w)


def belka(s, mat, a, b, gr, kolor, gora=(0, 0, 1.0)):
    """Belka o przekroju gr x gr od punktu a do b (dowolny kierunek)."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    d = b - a
    L = np.linalg.norm(d)
    if L < 1e-4:
        return
    d /= L
    g = np.asarray(gora, float)
    if abs(d @ g) > 0.95:
        g = np.array([1.0, 0, 0]) if abs(d[0]) < 0.9 else np.array([0, 1.0, 0])
    u = np.cross(d, g); u /= np.linalg.norm(u)
    w = np.cross(u, d)
    h = gr / 2
    rog = [(-1, -1), (1, -1), (1, 1), (-1, 1)]
    A = [a + (u * i + w * j) * h for i, j in rog]
    B = [b + (u * i + w * j) * h for i, j in rog]
    w = (a + b) / 2
    for k in range(4):
        k2 = (k + 1) % 4
        wielokat(s, mat, [A[k], A[k2], B[k2], B[k]], kolor, od=w)
    wielokat(s, mat, B, kolor, od=w)
    wielokat(s, mat, A, kolor, od=w)


def walec(s, mat, a, b, r, kolor, boki=8, denka=True):
    a, b = np.asarray(a, float), np.asarray(b, float)
    d = b - a; d /= np.linalg.norm(d)
    g = np.array([0, 0, 1.0]) if abs(d[2]) < 0.9 else np.array([1.0, 0, 0])
    u = np.cross(d, g); u /= np.linalg.norm(u); w = np.cross(u, d)
    kat = np.linspace(0, 2 * np.pi, boki, endpoint=False)
    kr = [np.cos(t) * u + np.sin(t) * w for t in kat]
    for k in range(boki):
        k2 = (k + 1) % boki
        p = [a + kr[k] * r, a + kr[k2] * r, b + kr[k2] * r, b + kr[k] * r]
        n = (kr[k] + kr[k2]) / 2
        s.dodaj(mat, p, [kr[k], kr[k2], kr[k2], kr[k]], kolor, [[0, 2, 1], [0, 3, 2]], _uv_scian(np.asarray(p), n, mat))
    if denka:
        wielokat(s, mat, [b + k * r for k in kr], kolor, n=d)
        wielokat(s, mat, [a + k * r for k in kr], kolor, n=-d)


def mury(s, x0, x1, y0, y1, h, t, kolor=MUR, z0=0.0, mat=KAMIEN):
    """Puste w srodku mury prostokata (dlugie sciany wzdluz X), od z0 do h."""
    if h <= z0 + 1e-3:
        return
    pudlo(s, mat, (x0, y0, z0), (x1, y0 + t, h), kolor)
    pudlo(s, mat, (x0, y1 - t, z0), (x1, y1, h), kolor)
    pudlo(s, mat, (x0, y0 + t, z0), (x0 + t, y1 - t, h), kolor)
    pudlo(s, mat, (x1 - t, y0 + t, z0), (x1, y1 - t, h), kolor)


def szczyt(s, x, y0, y1, h, wys, t, kolor=MUR, mat=KAMIEN):
    """Trojkatny szczyt sciany na koncu nawy (sciana w plaszczyznie YZ, grubosc t w strone +X od x)."""
    yc = (y0 + y1) / 2
    w = (x + t / 2, yc, h + wys / 3)
    wielokat(s, mat, [(x, y0, h), (x, y1, h), (x, yc, h + wys)], kolor, od=w)
    wielokat(s, mat, [(x + t, y0, h), (x + t, y1, h), (x + t, yc, h + wys)], kolor, od=w)
    wielokat(s, mat, [(x, y0, h), (x, yc, h + wys), (x + t, yc, h + wys), (x + t, y0, h)], kolor, od=w)
    wielokat(s, mat, [(x, yc, h + wys), (x, y1, h), (x + t, y1, h), (x + t, yc, h + wys)], kolor, od=w)


class Bryla:
    """Prostokatna czesc kosciola z dachem dwuspadowym wzdluz X."""
    def __init__(self, x0, x1, y0, y1, h, kalenica, t=0.7):
        self.x0, self.x1, self.y0, self.y1, self.h, self.k, self.t = x0, x1, y0, y1, h, kalenica, t

    @property
    def yc(self):
        return (self.y0 + self.y1) / 2


def polac(s, B, strona, od, do, okap=0.5, gr=0.22, x0=None, x1=None, mat=DACH, kolor=GONT):
    """Polac dachu od ulamka 'od' do 'do' (0 = okap, 1 = kalenica) po jednej stronie (-1 = -Y, +1 = +Y)."""
    x0 = B.x0 - okap if x0 is None else x0
    x1 = B.x1 + okap if x1 is None else x1
    yk = B.yc
    ye = B.y0 if strona < 0 else B.y1
    # okap wysuniety poza mur wzdluz spadku
    dy, dz = ye - yk, B.h - (B.h + B.k)
    L = np.hypot(dy, dz)
    ey, ez = ye + dy / L * okap, B.h + dz / L * okap
    def pkt(f, x):
        return np.array([x, ey + (yk - ey) * f, ez + (B.h + B.k - ez) * f])
    a0, a1, b1, b0 = pkt(od, x0), pkt(od, x1), pkt(do, x1), pkt(do, x0)
    nrm = np.cross(a1 - a0, b0 - a0)
    if nrm[2] < 0:
        nrm = -nrm
    nrm /= np.linalg.norm(nrm)
    dol = [a0, a1, b1, b0]
    gora = [p + nrm * gr for p in dol]
    w = sum(dol) / 4 + nrm * gr / 2
    wielokat(s, mat, gora, kolor, od=w)
    ciemny = kolor * 0.6
    wielokat(s, mat, dol, ciemny, od=w)
    for k in range(4):
        p, q = dol[k], dol[(k + 1) % 4]
        wielokat(s, mat, [p, q, q + nrm * gr, p + nrm * gr], ciemny, od=w)


def wiezba(s, B, od=0.0, do=1.0, krok=1.0, okap=0.3, kolor=BELKA):
    """Gola wiezba: krokwie co 'krok' m od x0 do x1, platew kalenicowa, belki wiazace na murach."""
    xs = np.arange(B.x0 + 0.3, B.x1 - 0.1, krok)
    lic = 0
    for x in xs:
        for strona in (-1, 1):
            ye = B.y0 if strona < 0 else B.y1
            a = np.array([x, ye - strona * 0.15 + strona * okap * 0.7, B.h - okap * 0.7 * B.k / (B.yc - B.y0)])
            k = np.array([x, B.yc, B.h + B.k])
            belka(s, DREWNO, a, a + (k - a) * do, 0.18, kolor * (0.94 + 0.06 * ((lic * 7) % 3)), gora=(1, 0, 0))
            lic += 1
        if (int((x - B.x0) / krok)) % 3 == 0:
            belka(s, DREWNO, (x, B.y0 + 0.2, B.h + 0.1), (x, B.y1 - 0.2, B.h + 0.1), 0.2, kolor * 0.9)  # belka wiazaca
    if do > 0.95:
        belka(s, DREWNO, (B.x0 + 0.1, B.yc, B.h + B.k - 0.05), (B.x1 - 0.1, B.yc, B.h + B.k - 0.05), 0.22, kolor * 0.85)
    for strona in (-1, 1):  # murlaty
        ye = (B.y0 + B.t / 2) if strona < 0 else (B.y1 - B.t / 2)
        belka(s, DREWNO, (B.x0, ye, B.h + 0.1), (B.x1, ye, B.h + 0.1), 0.22, kolor * 0.8)


def rusztowanie(s, x0, x1, y0, y1, h, odst=0.9, boki=(True, True, True, True), krok=2.2, poziom=2.0):
    """Rusztowanie dookola prostokata: stojaki co ~krok m, rygle i pomosty z desek co 'poziom' m, krzyzulce."""
    h = max(h, 1.0)
    X0, X1, Y0, Y1 = x0 - odst, x1 + odst, y0 - odst, y1 + odst
    gora = h + 1.2
    poziomy = np.arange(poziom, h + 0.6, poziom)
    def linia(a, b, wsp):
        L = np.linalg.norm(np.subtract(b, a))
        n = max(1, int(round(L / krok)))
        pts = [np.add(a, np.subtract(b, a) * k / n) for k in range(n + 1)]
        for k, p in enumerate(pts):
            belka(s, DREWNO, (p[0], p[1], -0.2), (p[0], p[1], gora), 0.11, RUSZT * (0.92 + 0.08 * (k % 2)))
        for z in poziomy:
            belka(s, DREWNO, (a[0], a[1], z), (b[0], b[1], z), 0.08, RUSZT)
            # pomost: deski od sciany do rygla (szer. ~0,8 m)
            d = np.subtract(b, a) / L
            nrm = np.array([d[1], -d[0]]) * wsp
            m0 = np.array(a) - nrm * 0.05
            m1 = np.array(b) - nrm * 0.05
            m2 = m1 - nrm * 0.6
            m3 = m0 - nrm * 0.6
            pts4 = [(*m0, z + 0.06), (*m1, z + 0.06), (*m2, z + 0.06), (*m3, z + 0.06)]
            wielokat(s, DREWNO, pts4, DESKA, n=np.array([0, 0, 1.0]))
        # krzyzulce na co drugim polu
        for k in range(0, len(pts) - 1, 2):
            p, q = pts[k], pts[k + 1]
            belka(s, DREWNO, (p[0], p[1], 0.2), (q[0], q[1], min(gora, 0.2 + np.linalg.norm(np.subtract(q, p)) * 1.1)), 0.08, RUSZT * 0.85)
    # boki: (-Y, +Y, -X, +X); normalne skierowane na zewnatrz -> pomost do srodka
    if boki[0]: linia((X0, Y0), (X1, Y0), -1)
    if boki[1]: linia((X1, Y1), (X0, Y1), -1)
    if boki[2]: linia((X0, Y1), (X0, Y0), -1)
    if boki[3]: linia((X1, Y0), (X1, Y1), -1)


def drzwi(s, x, yc, szer, wys, os="x", strona=-1, kolor=DRZWI):
    """Drzwi lukowe (prostokat + trojkat) wystajace 5 cm z lica sciany. os='x': sciana prostopadla do X, strona = kierunek normalnej."""
    g = 0.06
    if os == "x":
        p0 = (x + strona * g, yc - szer / 2, 0.0)
        pudlo(s, DREWNO, (min(x, x + strona * g), yc - szer / 2, 0.0), (max(x, x + strona * g), yc + szer / 2, wys), kolor)
        xs = x + strona * g
        tri = [(xs, yc - szer / 2, wys), (xs, yc + szer / 2, wys), (xs, yc, wys + szer * 0.45)]
        wielokat(s, DREWNO, tri, kolor, n=np.array([strona, 0, 0.0]))
    else:
        pudlo(s, DREWNO, (x - szer / 2, min(yc, yc + strona * g), 0.0), (x + szer / 2, max(yc, yc + strona * g), wys), kolor)
        ys = yc + strona * g
        tri = [(x - szer / 2, ys, wys), (x + szer / 2, ys, wys), (x, ys, wys + szer * 0.45)]
        wielokat(s, DREWNO, tri, kolor, n=np.array([0, strona, 0.0]))


def okno(s, x, y, z0, szer, wys, os="y", strona=-1):
    """Waskie okno lukowe: ciemna plama 4 cm przed licem sciany."""
    g = 0.04
    if os == "y":
        y1 = y + strona * g
        pudlo(s, KAMIEN, (x - szer / 2, min(y, y1), z0), (x + szer / 2, max(y, y1), z0 + wys), OKNO)
        tri = [(x - szer / 2, y1, z0 + wys), (x + szer / 2, y1, z0 + wys), (x, y1, z0 + wys + szer * 0.5)]
        wielokat(s, KAMIEN, tri, OKNO, n=np.array([0, strona, 0.0]))
    else:
        x1 = x + strona * g
        pudlo(s, KAMIEN, (min(x, x1), y - szer / 2, z0), (max(x, x1), y + szer / 2, z0 + wys), OKNO)
        tri = [(x1, y - szer / 2, z0 + wys), (x1, y + szer / 2, z0 + wys), (x1, y, z0 + wys + szer * 0.5)]
        wielokat(s, KAMIEN, tri, OKNO, n=np.array([strona, 0, 0.0]))


def iglica(s, x0, x1, y0, y1, z, wys, okap=0.35):
    """Ostroslupowy dach wiezy z gontu."""
    X0, X1, Y0, Y1 = x0 - okap, x1 + okap, y0 - okap, y1 + okap
    z0 = z - 0.25
    szczytp = np.array([(x0 + x1) / 2, (y0 + y1) / 2, z + wys])
    rogi = [np.array(p) for p in ((X0, Y0, z0), (X1, Y0, z0), (X1, Y1, z0), (X0, Y1, z0))]
    w = sum(rogi) / 4 + (0, 0, wys * 0.3)
    for k in range(4):
        wielokat(s, DACH, [rogi[k], rogi[(k + 1) % 4], szczytp], GONT, od=w)
    wielokat(s, DACH, rogi, GONT * 0.6, od=w)
    return szczytp


def krzyz(s, p, h=1.6):
    p = np.asarray(p, float)
    belka(s, DREWNO, p - (0, 0, 0.3), p + (0, 0, h), 0.12, KRZYZ)
    belka(s, DREWNO, p + (-0.45, 0, h * 0.68), p + (0.45, 0, h * 0.68), 0.1, KRZYZ)


# ---------------- material na placu budowy ----------------

def skrzynia(s, x, y, a, z=0.0, kolor=None):
    k = DESKA * 0.8 if kolor is None else kolor
    pudlo(s, DREWNO, (x - a / 2, y - a / 2, z), (x + a / 2, y + a / 2, z + a * 0.8), k)
    belka(s, DREWNO, (x - a / 2 - 0.02, y - a / 2 - 0.02, z + a * 0.4), (x + a / 2 + 0.02, y - a / 2 - 0.02, z + a * 0.4), 0.06, RUSZT * 0.8)


def beczka(s, x, y, z=0.0):
    walec(s, DREWNO, (x, y, z), (x, y, z + 0.9), 0.33, DRZWI * 1.5, boki=10)
    for hh in (0.18, 0.72):
        walec(s, DREWNO, (x, y, z + hh - 0.03), (x, y, z + hh + 0.03), 0.345, KRZYZ, boki=10, denka=False)


def stos_kamieni(s, x, y, rng, n=3):
    for warstwa in range(n):
        for k in range(n - warstwa):
            for j in range(2):
                px = x + (k - (n - warstwa - 1) / 2) * 0.62
                py = y + (j - 0.5) * 0.42
                z = warstwa * 0.32
                pudlo(s, KAMIEN, (px - 0.29, py - 0.19, z), (px + 0.29, py + 0.19, z + 0.3), MUR * rng.uniform(0.9, 1.02))


def stos_belek(s, x, y, dl=4.0, n=5):
    for k in range(n):
        warstwa = 0 if k < 3 else 1
        py = y + (k - 1 if k < 3 else k - 3.5) * 0.3
        z = 0.14 + warstwa * 0.27
        walec(s, DREWNO, (x - dl / 2, py, z), (x + dl / 2, py, z), 0.14, BELKA * (0.85 + 0.05 * k), boki=6)


def plac_budowy(s, rng, ile):
    """Material lezacy przed kosciolem (po stronie -Y, widocznej z kamery). ile: 0..1 - ile go zostalo."""
    if ile <= 0:
        return
    stos_kamieni(s, -2.0, -7.6, rng, 3 if ile > 0.5 else 2)
    if ile > 0.3:
        stos_kamieni(s, 2.0, -7.4, rng, 2)
    # Wszystko dalej niz 6,2 m od osi: pas 5,4 m wokol kosciola jest sciezka budowniczych (ALudzik, ObwodMin/Max).
    skrzynia(s, -9.6, -6.8, 0.8)
    skrzynia(s, -8.7, -7.0, 0.7)
    skrzynia(s, -9.2, -6.9, 0.6, z=0.64, kolor=DESKA * 0.9)
    beczka(s, -7.8, -7.4)
    if ile > 0.5:
        beczka(s, -7.1, -6.9)
        stos_belek(s, 6.5, -7.8)
    if ile > 0.8:
        skrzynia(s, 9.5, -6.9, 0.8)


# ---------------- kosciol ----------------

NAWA = Bryla(-6.5, 7.0, -4.0, 4.0, 6.5, 4.6)
PREZB = Bryla(7.0, 11.5, -3.0, 3.0, 5.2, 3.4)
WIEZA = (-11.0, -6.5, -2.25, 2.25)  # x0, x1, y0, y1
WIEZA_H = 15.0


def kosciol(faza):
    """faza 1..5 (5 = gotowy)."""
    s = Siatka()
    rng = np.random.default_rng(100 + faza)
    wx0, wx1, wy0, wy1 = WIEZA
    t = NAWA.t
    # Wysokosci murow w kolejnych fazach (nawa, prezbiterium, wieza).
    hn = {1: 1.8, 2: 4.4, 3: NAWA.h, 4: NAWA.h, 5: NAWA.h}[faza]
    hp = {1: 1.5, 2: 3.8, 3: PREZB.h, 4: PREZB.h, 5: PREZB.h}[faza]
    hw = {1: 2.4, 2: 8.5, 3: WIEZA_H, 4: WIEZA_H, 5: WIEZA_H}[faza]

    # Fundament (cokol) pod calym kosciolem - wchodzi 3 m w ziemie, zeby na pochylym terenie nie wisial.
    for x0, x1, y0, y1 in ((NAWA.x0, NAWA.x1, NAWA.y0, NAWA.y1), (PREZB.x0 - 0.5, PREZB.x1, PREZB.y0, PREZB.y1), WIEZA):
        pudlo(s, KAMIEN, (x0 - 0.25, y0 - 0.25, -3.0), (x1 + 0.25, y1 + 0.25, 0.35), MUR_COKOL)

    # Mury (puste w srodku - w czasie budowy widac wnetrze).
    mury(s, NAWA.x0, NAWA.x1, NAWA.y0, NAWA.y1, hn, t)
    mury(s, PREZB.x0, PREZB.x1, PREZB.y0, PREZB.y1, hp, 0.6)
    mury(s, wx0, wx1, wy0, wy1, hw, 0.8)
    if faza >= 3:
        # szczyty: nawa od strony prezbiterium (widoczny ponad nim) i od wiezy; prezbiterium od wschodu
        szczyt(s, NAWA.x1 - t, NAWA.y0, NAWA.y1, NAWA.h, NAWA.k, t)
        szczyt(s, NAWA.x0, NAWA.y0, NAWA.y1, NAWA.h, NAWA.k, t)
        szczyt(s, PREZB.x1 - 0.6, PREZB.y0, PREZB.y1, PREZB.h, PREZB.k, 0.6)
        pudlo(s, KAMIEN, (wx0 + 0.8, wy0 + 0.8, WIEZA_H - 0.3), (wx1 - 0.8, wy1 - 0.8, WIEZA_H - 0.1), MUR * 0.7)  # strop wiezy
        # gzyms pod iglica
        pudlo(s, KAMIEN, (wx0 - 0.12, wy0 - 0.12, WIEZA_H - 0.5), (wx1 + 0.12, wy1 + 0.12, WIEZA_H - 0.1), MUR * 0.92)

    # Otwory: drzwi w wiezy od zachodu i boczne w nawie, okna wysokie w nawie i prezbiterium, okienka w wiezy.
    if faza >= 2:
        drzwi(s, wx0, 0.0, 1.5, 2.3, "x", -1, DRZWI if faza == 5 else rgb(30, 26, 24))
        drzwi(s, -1.0, NAWA.y0, 1.3, 2.1, "y", -1, DRZWI if faza == 5 else rgb(30, 26, 24))
    if faza >= 3:
        for x in (2.0, 5.0):
            okno(s, x, NAWA.y0, 2.6, 0.75, 2.4, "y", -1)
            okno(s, x, NAWA.y1, 2.6, 0.75, 2.4, "y", 1)
        okno(s, -4.0, NAWA.y1, 2.6, 0.75, 2.4, "y", 1)
        okno(s, 9.2, PREZB.y0, 2.0, 0.7, 2.0, "y", -1)
        okno(s, 9.2, PREZB.y1, 2.0, 0.7, 2.0, "y", 1)
        okno(s, PREZB.x1, 0.0, 2.0, 0.7, 2.0, "x", 1)
        for z0 in (WIEZA_H - 3.2,):
            okno(s, (wx0 + wx1) / 2, wy0, z0, 0.6, 1.2, "y", -1)
            okno(s, (wx0 + wx1) / 2, wy1, z0, 0.6, 1.2, "y", 1)
            okno(s, wx0, 0.0, z0, 0.6, 1.2, "x", -1)
        okno(s, wx0, 0.0, 6.5, 0.45, 1.3, "x", -1)
    if faza >= 2:
        okno(s, (wx0 + wx1) / 2, wy0, 4.5, 0.4, 1.1, "y", -1)

    # Dachy.
    if faza == 3:
        wiezba(s, NAWA)
        wiezba(s, PREZB)
    elif faza == 4:
        # prezbiterium pokryte, nawa: gont od okapu do polowy, wyzej gola wiezba
        polac(s, PREZB, -1, 0, 1, x0=PREZB.x0, x1=PREZB.x1 + 0.4)
        polac(s, PREZB, 1, 0, 1, x0=PREZB.x0, x1=PREZB.x1 + 0.4)
        wiezba(s, NAWA)
        polac(s, NAWA, -1, 0, 0.55)
        polac(s, NAWA, 1, 0, 0.7)
    elif faza == 5:
        polac(s, PREZB, -1, 0, 1, x0=PREZB.x0, x1=PREZB.x1 + 0.4)
        polac(s, PREZB, 1, 0, 1, x0=PREZB.x0, x1=PREZB.x1 + 0.4)
        polac(s, NAWA, -1, 0, 1)
        polac(s, NAWA, 1, 0, 1)
    if faza >= 4:
        p = iglica(s, wx0, wx1, wy0, wy1, WIEZA_H, 6.0)
        if faza == 5:
            krzyz(s, p)

    # Rusztowania wokol tego, co sie buduje.
    if faza == 1:
        rusztowanie(s, NAWA.x0, NAWA.x1, NAWA.y0, NAWA.y1, hn, boki=(True, False, False, False))
        rusztowanie(s, wx0, wx1, wy0, wy1, hw, boki=(True, True, True, False))
    elif faza == 2:
        rusztowanie(s, NAWA.x0, NAWA.x1, NAWA.y0, NAWA.y1, hn, boki=(True, True, False, False))
        rusztowanie(s, PREZB.x0, PREZB.x1, PREZB.y0, PREZB.y1, hp, boki=(True, True, False, True))
        rusztowanie(s, wx0, wx1, wy0, wy1, hw, boki=(True, True, True, False))
    elif faza == 3:
        rusztowanie(s, wx0, wx1, wy0, wy1, hw - 1.0, boki=(True, True, True, False))
        rusztowanie(s, NAWA.x0 + 3.5, NAWA.x1, NAWA.y0, NAWA.y1, hn - 1.0, boki=(True, False, False, False))
    elif faza == 4:
        rusztowanie(s, NAWA.x0 + 2.0, NAWA.x1 - 2.0, NAWA.y0, NAWA.y1, hn - 1.0, boki=(False, True, False, False))

    plac_budowy(s, rng, {1: 1.0, 2: 1.0, 3: 0.7, 4: 0.4, 5: 0.0}[faza])
    return s


# ---------------- kosciol: trzy poziomy (22.11.2026, prosba uzytkownika) ----------------
# Poziom 1: SM_KosciolDrewniany (zrab z bali, wieza-dzwonnica z izbica, soboty = podcien wokol nawy), 4 fazy.
# Poziom 2: SM_Kosciol (kamienny, jak dotad); ulepszenie z poziomu 1: SM_Kosciol_U (drewniany w rusztowaniu, kamien).
# Poziom 3: SM_KosciolWielki (kamienny + kaplica boczna, brukowany plac, mur z brama, studnia, lipy, figura), 5 faz;
#   kaplica od frontu; kosciol z poziomu 2 stoi caly czas, rosna tylko dodatki. Ulepszenie: SM_KosciolWielki_U = faza 1.
NAWA_D = Bryla(-4.5, 4.5, -3.0, 3.0, 4.2, 4.0, t=0.32)
PREZB_D = Bryla(4.5, 7.6, -2.2, 2.2, 3.8, 3.0, t=0.32)
WIEZA_D = (-7.8, -4.5, -1.65, 1.65)
WIEZA_D_H = 8.6      # wysokosc wiezy do okapu izbicy
DESKA_W = rgb(118, 92, 66)  # ciemne, stare deski szalunku wiezy


def kosciol_drewniany(faza):
    """faza 1..4 (4 = gotowy)."""
    s = Siatka()
    rng = np.random.default_rng(150 + faza)
    z0 = 0.35
    wx0, wx1, wy0, wy1 = WIEZA_D
    for B in (NAWA_D, PREZB_D):
        cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    cokol(s, wx0, wx1, wy0, wy1, z0)
    hn = {1: 1.5, 2: NAWA_D.h, 3: NAWA_D.h, 4: NAWA_D.h}[faza]
    hp = {1: 1.2, 2: PREZB_D.h, 3: PREZB_D.h, 4: PREZB_D.h}[faza]
    zrab(s, NAWA_D.x0, NAWA_D.x1, NAWA_D.y0, NAWA_D.y1, hn, z0, rng)
    zrab(s, PREZB_D.x0 + 0.3, PREZB_D.x1, PREZB_D.y0, PREZB_D.y1, hp, z0, rng, wysun=0.25)

    # Wieza: slupy narozne pochylone do srodka, krzyzulce; od fazy 3 szalunek z desek, na gorze szersza izbica.
    hw = {1: 2.6, 2: WIEZA_D_H, 3: WIEZA_D_H, 4: WIEZA_D_H}[faza]
    zw = 0.35  # zwezenie slupow na gorze
    rogi = [(wx0, wy0, -1, -1), (wx1, wy0, 1, -1), (wx1, wy1, 1, 1), (wx0, wy1, -1, 1)]
    for x, y, sx, sy in rogi:
        f = hw / WIEZA_D_H
        belka(s, DREWNO, (x, y, z0), (x - sx * zw * f, y - sy * zw * f, z0 + hw), 0.26, RYGIEL)
    if faza >= 2:
        for z in (2.8, 5.6):
            for (x, y, sx, sy), (x2, y2, sx2, sy2) in zip(rogi, rogi[1:] + rogi[:1]):
                f = z / WIEZA_D_H
                belka(s, DREWNO, (x - sx * zw * f, y - sy * zw * f, z0 + z), (x2 - sx2 * zw * f, y2 - sy2 * zw * f, z0 + z), 0.18, RYGIEL)
    if faza >= 3:
        # szalunek: pionowe deski, zwezajace sie ku gorze (cztery trapezy)
        a, b = z0, z0 + WIEZA_D_H
        for (x, y, sx, sy), (x2, y2, sx2, sy2) in zip(rogi, rogi[1:] + rogi[:1]):
            p = [(x + sx * 0.04, y + sy * 0.04, a), (x2 + sx2 * 0.04, y2 + sy2 * 0.04, a),
                 (x2 - sx2 * (zw - 0.04), y2 - sy2 * (zw - 0.04), b), (x - sx * (zw - 0.04), y - sy * (zw - 0.04), b)]
            wielokat(s, DREWNO, p, DESKA_W * rng.uniform(0.92, 1.04), od=((wx0 + wx1) / 2, (wy0 + wy1) / 2, (a + b) / 2))
        # izbica (dzwonnica): wystaje 0,4 m poza szalunek, ciemne otwory na dzwony
        i0, i1 = z0 + WIEZA_D_H, z0 + WIEZA_D_H + 2.4
        ix0, ix1, iy0, iy1 = wx0 - 0.05, wx1 + 0.05, wy0 - 0.05, wy1 + 0.05
        if faza == 4:
            pudlo(s, DREWNO, (ix0, iy0, i0), (ix1, iy1, i1), DESKA_W * 1.08)
            for k, (x, y, os, st) in enumerate(((ix0, 0.0, "x", -1), ((ix0 + ix1) / 2, iy0, "y", -1), ((ix0 + ix1) / 2, iy1, "y", 1))):
                for d in (-0.75, 0.75):
                    if os == "x":
                        pudlo(s, DREWNO, (x - 0.03, d - 0.28, i0 + 0.7), (x, d + 0.28, i0 + 1.9), OKNO)
                    else:
                        pudlo(s, DREWNO, (x + d - 0.28, min(y, y + st * 0.03), i0 + 0.7), (x + d + 0.28, max(y, y + st * 0.03), i0 + 1.9), OKNO)
            p = iglica(s, ix0, ix1, iy0, iy1, i1 + 0.25, 4.6, okap=0.5)
            krzyz(s, p, 1.3)
        else:
            for x, y in ((ix0, iy0), (ix1, iy0), (ix1, iy1), (ix0, iy1)):
                belka(s, DREWNO, (x, y, i0), (x, y, i1), 0.2, RYGIEL)
            belka(s, DREWNO, (ix0, iy0, i1), (ix1, iy0, i1), 0.2, RYGIEL)
            belka(s, DREWNO, (ix0, iy1, i1), (ix1, iy1, i1), 0.2, RYGIEL)

    # Otwory: drzwi w wiezy od zachodu (-X), okna nawy i prezbiterium.
    if faza >= 2:
        gotowe = faza == 4
        drzwi_dom(s, (wx0 - 0.04, wy0, 0), "x", -1, (wy1 - wy0) / 2, z0, 1.3, 2.4, DRZWI if gotowe else rgb(30, 26, 24))
        for u in (2.4, 6.0):
            okienko(s, (NAWA_D.x0, NAWA_D.y0 - 0.02, 0), "y", -1, u, z0 + 1.5, 0.75, 1.1, okiennice=False)
            okienko(s, (NAWA_D.x0, NAWA_D.y1 + 0.02, 0), "y", 1, u, z0 + 1.5, 0.75, 1.1, okiennice=False)
        okienko(s, (PREZB_D.x0, PREZB_D.y0 - 0.02, 0), "y", -1, 1.6, z0 + 1.4, 0.7, 1.0, okiennice=False)

    # Dachy z gontu: wiezba, pol dachu, caly dach + sygnaturka (wiezyczka na kalenicy) z krzyzem.
    if faza >= 3:
        for B in (NAWA_D, PREZB_D):
            for x, st in ((B.x0, -1), (B.x1, 1)):
                if B is PREZB_D and st < 0:
                    continue
                szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
            wiezba(s, B, kolor=BALE)
    if faza == 3:
        for st in (-1, 1):
            polac(s, PREZB_D, st, 0, 1, okap=0.5, x0=PREZB_D.x0, x1=PREZB_D.x1 + 0.4)
            polac(s, NAWA_D, st, 0, 0.5, okap=0.6)
    elif faza == 4:
        for st in (-1, 1):
            polac(s, PREZB_D, st, 0, 1, okap=0.5, x0=PREZB_D.x0, x1=PREZB_D.x1 + 0.4)
            polac(s, NAWA_D, st, 0, 1, okap=0.6)
        sx, sz = 2.6, NAWA_D.h + NAWA_D.k
        pudlo(s, DREWNO, (sx - 0.45, -0.45, sz - 0.6), (sx + 0.45, 0.45, sz + 1.0), DESKA_W)
        p = iglica(s, sx - 0.45, sx + 0.45, -0.45, 0.45, sz + 1.25, 1.6, okap=0.15)
        krzyz(s, p, 0.9)
        # soboty: podcien na slupkach wzdluz obu dlugich scian nawy
        for st in (-1, 1):
            ys = (NAWA_D.y0 - 1.5) if st < 0 else (NAWA_D.y1 + 1.5)
            yw = NAWA_D.y0 if st < 0 else NAWA_D.y1
            for x in np.linspace(NAWA_D.x0 + 0.2, NAWA_D.x1 - 0.2, 5):
                if st < 0 and abs(x - (NAWA_D.x0 + 2.4)) < 0.6:
                    continue
                belka(s, DREWNO, (x, ys, -0.2), (x, ys, 2.0), 0.16, RYGIEL)
            belka(s, DREWNO, (NAWA_D.x0, ys, 2.0), (NAWA_D.x1, ys, 2.0), 0.16, RYGIEL)
            plyta(s, DACH, [(NAWA_D.x0 - 0.2, yw, 2.9), (NAWA_D.x1 + 0.2, yw, 2.9), (NAWA_D.x1 + 0.2, ys + st * 0.4, 2.05),
                            (NAWA_D.x0 - 0.2, ys + st * 0.4, 2.05)], 0.1, GONT * 1.05)

    # Rusztowanie przy wiezy i material przed kosciolem (-Y).
    if faza <= 3:
        rusztowanie(s, wx0, wx1, wy0, wy1, hw if faza < 3 else WIEZA_D_H + 1.5, boki=(True, True, True, False), odst=0.8)
    if faza == 1:
        rusztowanie(s, NAWA_D.x0, NAWA_D.x1, NAWA_D.y0, NAWA_D.y1, hn, boki=(True, False, False, False), odst=0.8)
    ile = {1: 1.0, 2: 0.8, 3: 0.5, 4: 0.0}[faza]
    if ile > 0:
        stos_belek(s, -2.0, -6.6, 5.0, 5 if ile > 0.6 else 3)
        stos_desek(s, 2.4, -6.4, n=5 if ile > 0.6 else 3)
        kozly(s, 5.6, -6.6)
        if ile > 0.6:
            stos_belek(s, -2.0, -7.6, 5.0, 4)
            skrzynia(s, -6.2, -6.5, 0.8)
    return s


def kosciol_przebudowa():
    """Ulepszenie drewnianego kosciola do kamiennego: drewniany w rusztowaniu, przed nim kamien."""
    s = kosciol_drewniany(4)
    rng = np.random.default_rng(170)
    rusztowanie(s, NAWA_D.x0, PREZB_D.x1, NAWA_D.y0, NAWA_D.y1, NAWA_D.h, boki=(True, True, False, True), odst=1.2)
    plac_budowy(s, rng, 1.0)
    return s


KAPLICA = Bryla(1.5, 5.6, -7.2, -4.0, 4.6, 2.8)  # kaplica boczna od frontu (-Y), widoczna z kamery
BRUK = rgb(150, 142, 130)
LIPA = rgb(84, 110, 60)
PLAC_KOSCIOLA = (-15.0, 15.5, -11.0, 9.5)  # x0, x1, y0, y1 placu z murem


def lipa(s, x, y, h=6.5, r=2.2, rng=None):
    """Drzewo na placu: pien i korona z kilku walcow (kolor jak korony drzew na mapie)."""
    rng = np.random.default_rng(int(abs(x * 13 + y * 7))) if rng is None else rng
    walec(s, DREWNO, (x, y, -0.3), (x, y, h * 0.5), 0.22, rgb(70, 56, 44), boki=6)
    for k, (dz, rr) in enumerate(((0.45, 0.85), (0.62, 1.0), (0.8, 0.75), (0.95, 0.4))):
        z = h * dz
        walec(s, TYNK, (x, y, z - 0.9), (x, y, z + 0.6), r * rr, LIPA * rng.uniform(0.85, 1.08), boki=8)


def studnia(s, x, y):
    walec(s, KAMIEN, (x, y, -0.5), (x, y, 0.8), 0.75, MUR_COKOL, boki=10)
    walec(s, KAMIEN, (x, y, 0.79), (x, y, 0.81), 0.55, OKNO, boki=10)
    for dx in (-0.65, 0.65):
        belka(s, DREWNO, (x + dx, y, 0.6), (x + dx, y, 2.3), 0.12, RYGIEL)
    walec(s, DREWNO, (x - 0.7, y, 1.6), (x + 0.7, y, 1.6), 0.08, BELKA, boki=6)
    plyta(s, DACH, [(x - 0.9, y - 0.75, 2.2), (x + 0.9, y - 0.75, 2.2), (x + 0.9, y, 2.75), (x - 0.9, y, 2.75)], 0.08, GONT)
    plyta(s, DACH, [(x - 0.9, y, 2.75), (x + 0.9, y, 2.75), (x + 0.9, y + 0.75, 2.2), (x - 0.9, y + 0.75, 2.2)], 0.08, GONT)


def figura(s, x, y):
    """Krzyz przydrozny na kamiennym cokole."""
    pudlo(s, KAMIEN, (x - 0.5, y - 0.5, -0.5), (x + 0.5, y + 0.5, 0.6), MUR_COKOL)
    pudlo(s, KAMIEN, (x - 0.3, y - 0.3, 0.6), (x + 0.3, y + 0.3, 1.4), MUR * 0.9)
    belka(s, DREWNO, (x, y, 1.4), (x, y, 3.8), 0.14, KRZYZ)
    belka(s, DREWNO, (x - 0.6, y, 3.1), (x + 0.6, y, 3.1), 0.12, KRZYZ)


def mur_cmentarny(s, h, brama=True):
    """Niski mur wokol placu z brama od frontu (-Y) na osi."""
    x0, x1, y0, y1 = PLAC_KOSCIOLA
    t = 0.45
    if h <= 0.05:
        return
    for a, b in (((x0, y0), (-1.6, y0)), ((1.6, y0), (x1, y0))):
        pudlo(s, KAMIEN, (a[0], y0, -0.5), (b[0], y0 + t, h), MUR * 0.93)
    pudlo(s, KAMIEN, (x0, y1 - t, -0.5), (x1, y1, h), MUR * 0.93)
    pudlo(s, KAMIEN, (x0, y0, -0.5), (x0 + t, y1, h), MUR * 0.93)
    pudlo(s, KAMIEN, (x1 - t, y0, -0.5), (x1, y1, h), MUR * 0.93)
    if h > 0.8:
        # daszek z gontu na murze (od frontu, widoczny z kamery)
        for a0, a1 in ((x0, -1.6), (1.6, x1)):
            plyta(s, DACH, [(a0, y0 - 0.15, h + 0.08), (a1, y0 - 0.15, h + 0.08), (a1, y0 + t / 2, h + 0.3), (a0, y0 + t / 2, h + 0.3)], 0.06, GONT)
    if brama and h > 0.8:
        for x in (-2.1, 1.6):
            pudlo(s, KAMIEN, (x, y0 - 0.1, -0.5), (x + 0.5, y0 + t + 0.1, 2.8), MUR * 0.88)
        plyta(s, DACH, [(-2.4, y0 - 0.6, 3.0), (2.4, y0 - 0.6, 3.0), (2.4, y0 + t / 2, 3.6), (-2.4, y0 + t / 2, 3.6)], 0.1, GONT)
        plyta(s, DACH, [(-2.4, y0 + t / 2, 3.6), (2.4, y0 + t / 2, 3.6), (2.4, y0 + t + 0.6, 3.0), (-2.4, y0 + t + 0.6, 3.0)], 0.1, GONT)


def kosciol_wielki(faza):
    """faza 1..5: kosciol z poziomu 2 gotowy, rosna kaplica boczna (-Y), mur, plac i reszta."""
    s = kosciol(5)
    rng = np.random.default_rng(180 + faza)
    K = KAPLICA
    cokol(s, K.x0, K.x1, K.y0, K.y1 + 0.5, 0.35)
    hk = {1: 1.6, 2: K.h, 3: K.h, 4: K.h, 5: K.h}[faza]
    mury(s, K.x0, K.x1, K.y0, K.y1, hk, 0.6)
    if faza >= 2:
        for x, st in ((K.x0, -1), (K.x1 - 0.6, 1)):
            szczyt(s, x, K.y0, K.y1, K.h, K.k, 0.6)
        drzwi(s, (K.x0 + K.x1) / 2, K.y0, 1.2, 2.2, "y", -1, DRZWI if faza == 5 else rgb(30, 26, 24))
        for x in (K.x0 + 0.9, K.x1 - 0.9):
            okno(s, x, K.y0, 2.0, 0.6, 1.8, "y", -1)
        okno(s, K.x1, (K.y0 + K.y1) / 2, 2.2, 0.7, 1.8, "x", 1)
        okno(s, K.x0, (K.y0 + K.y1) / 2, 2.2, 0.7, 1.8, "x", -1)
    if faza == 3:
        wiezba(s, K)
    elif faza == 4:
        wiezba(s, K)
        polac(s, K, -1, 0, 1, x0=K.x0 - 0.4, x1=K.x1 + 0.4)
        polac(s, K, 1, 0, 0.5, x0=K.x0 - 0.4, x1=K.x1 + 0.4)
    elif faza == 5:
        polac(s, K, -1, 0, 1, x0=K.x0 - 0.4, x1=K.x1 + 0.4)
        polac(s, K, 1, 0, 1, x0=K.x0 - 0.4, x1=K.x1 + 0.4)
        krzyz(s, ((K.x0 + K.x1) / 2 + 2.0, K.yc, K.h + K.k), 1.0)
    if faza <= 3:
        rusztowanie(s, K.x0, K.x1, K.y0, K.y1 - 0.6, hk if faza < 3 else K.h + 1.0, boki=(True, False, True, True), odst=0.8)

    # Mur cmentarny: fundament -> pol wysokosci -> caly z brama.
    mur_cmentarny(s, {1: 0.0, 2: 0.4, 3: 1.2, 4: 1.2, 5: 1.2}[faza])
    # Plac: bruk od fazy 4 (najpierw przed kosciolem), na koniec studnia, lipy, figura, lawki.
    x0, x1, y0, y1 = PLAC_KOSCIOLA
    if faza >= 4:
        pudlo(s, KAMIEN, (x0 + 0.45, y0 + 0.45, -3.0), (x1 - 0.45, -4.6 if faza == 4 else y1 - 0.45, 0.05), BRUK)
        studnia(s, -10.5, -7.6)
    if faza == 5:
        for x, y in ((-13.8, -9.6), (14.2, -9.6), (-13.0, 7.2), (13.4, 7.2), (-8.0, 7.6)):
            lipa(s, x, y, rng=np.random.default_rng(int(190 + x * 3 + y)))
        figura(s, 10.6, -8.2)
        lawka(s, -5.4, -9.6, 2.0)
        lawka(s, 5.4, -9.6, 2.0)
    else:
        # material obok kaplicy (przed nia chodza budowniczy)
        ile = {1: 1.0, 2: 0.9, 3: 0.6, 4: 0.3}[faza]
        stos_kamieni(s, -3.6, -8.8, rng, 3 if ile > 0.5 else 2)
        if ile > 0.5:
            stos_kamieni(s, -6.4, -8.6, rng, 2)
            stos_belek(s, 9.6, -8.8)
        skrzynia(s, -8.4, -6.9, 0.8)
        beczka(s, -7.5, -7.4)
    return s


# ---------------- domy mieszkalne (4 fazy), namiot, ognisko ----------------
# Domy: X = wzdluz kalenicy, drzwi na scianie -Y (strona widoczna z kamery, tak jak plac przed kosciolem).
# Ludziki w grze sa powiekszone x1,4 (~2,3 m), wiec drzwi maja ~2,4 m, a parter ~3 m.
# Fazy: F1 podmurowka i pierwsze bale / podwalina, F2 sciany albo goly szkielet, F3 szczyty, wiezba i pol dachu, F4 gotowy.
# Material budowy lezy po -Y dalej niz 2,4 m od sciany (blizej chodza budowniczy).

TYNK_K = rgb(234, 226, 208)
SLOMA = rgb(182, 164, 124)  # przygaszona; jaskrawozolta wygladala jak gont
BALE = rgb(150, 116, 80)
RYGIEL = rgb(84, 62, 46)      # ciemne belki muru pruskiego
OKIENNICA = rgb(96, 104, 98)  # przygaszona szarozielen
DESKA_SZCZYT = rgb(134, 104, 74)
PLOTNO = rgb(208, 194, 164)


def _sciana(p0, os, strona):
    """Uklad sciany: u wzdluz sciany (od p0), z w gore, d od lica na zewnatrz. os 'y' = sciana z normalna +-Y."""
    t = np.array([1.0, 0, 0]) if os == "y" else np.array([0, 1.0, 0])
    n = np.array([0, float(strona), 0]) if os == "y" else np.array([float(strona), 0, 0])
    p0 = np.asarray(p0, float)

    def pt(u, z, d=0.0):
        return p0 + t * u + n * d + np.array([0, 0, z])

    def box(s, mat, u0, u1, z0, z1, d0, d1, kolor):
        a, b = pt(u0, z0, d0), pt(u1, z1, d1)
        pudlo(s, mat, np.minimum(a, b), np.maximum(a, b), kolor)
    return box, pt, n


def okienko(s, p0, os, strona, u, z0, szer=0.8, wys=0.8, okiennice=True):
    """Okno domu: ciemna szyba, ciemna rama z krzyzem, otwarte okiennice po bokach."""
    box, pt, n = _sciana(p0, os, strona)
    a, b = u - szer / 2, u + szer / 2
    box(s, DREWNO, a, b, z0, z0 + wys, 0, 0.04, OKNO)
    r = 0.1
    box(s, DREWNO, a - r, a, z0 - r, z0 + wys + r, 0, 0.08, RYGIEL)
    box(s, DREWNO, b, b + r, z0 - r, z0 + wys + r, 0, 0.08, RYGIEL)
    box(s, DREWNO, a, b, z0 + wys, z0 + wys + r, 0, 0.08, RYGIEL)
    box(s, DREWNO, a - r, b + r, z0 - r - 0.04, z0, 0, 0.14, RYGIEL)  # parapet
    box(s, DREWNO, u - 0.03, u + 0.03, z0, z0 + wys, 0, 0.06, RYGIEL)
    box(s, DREWNO, a, b, z0 + wys * 0.55 - 0.03, z0 + wys * 0.55 + 0.03, 0, 0.06, RYGIEL)
    if okiennice:
        box(s, DREWNO, a - r - szer / 2, a - r, z0, z0 + wys, 0, 0.05, OKIENNICA)
        box(s, DREWNO, b + r, b + r + szer / 2, z0, z0 + wys, 0, 0.05, OKIENNICA)


def drzwi_dom(s, p0, os, strona, u, z0, szer=1.2, wys=2.4, kolor=DRZWI, prog=True):
    """Drzwi z desek w ciemnej ramie, z kamiennym progiem."""
    box, pt, n = _sciana(p0, os, strona)
    a, b = u - szer / 2, u + szer / 2
    box(s, DREWNO, a, b, z0, z0 + wys, 0, 0.06, kolor)
    r = 0.13
    box(s, DREWNO, a - r, a, z0, z0 + wys + r, 0, 0.1, RYGIEL)
    box(s, DREWNO, b, b + r, z0, z0 + wys + r, 0, 0.1, RYGIEL)
    box(s, DREWNO, a, b, z0 + wys, z0 + wys + r, 0, 0.1, RYGIEL)
    for zz in (0.45, wys - 0.6):  # poprzeczki
        box(s, DREWNO, a + 0.05, b - 0.05, z0 + zz, z0 + zz + 0.12, 0.06, 0.09, kolor * 0.72)
    if prog and z0 > 0.05:
        box(s, KAMIEN, a - 0.25, b + 0.25, -0.4, z0 * 0.55, 0, 0.55, MUR_COKOL)


def cokol(s, x0, x1, y0, y1, h=0.35, m=0.12):
    """Podmurowka: siega 3 m w ziemie (na pochylym terenie dom nie wisi)."""
    pudlo(s, KAMIEN, (x0 - m, y0 - m, -3.0), (x1 + m, y1 + m, h), MUR_COKOL)


def zrab(s, x0, x1, y0, y1, h, z0, rng, d=0.32, wysun=0.3):
    """Sciany z bali na zrab: bale wzdluz X (sciany +-Y) i wzdluz Y na przemian o pol bala wyzej, konce wystaja na rogach."""
    r = d / 2
    k = 0
    while True:
        zx = z0 + r + k * d
        if zx + r > h + 1e-3:
            break
        for y in (y0 + r, y1 - r):
            walec(s, DREWNO, (x0 - wysun, y, zx), (x1 + wysun, y, zx), r * 1.06, BALE * rng.uniform(0.82, 1.05), boki=6)
        zy = zx + d / 2
        if zy + r <= h + 1e-3:
            for x in (x0 + r, x1 - r):
                walec(s, DREWNO, (x, y0 - wysun, zy), (x, y1 + wysun, zy), r * 1.06, BALE * rng.uniform(0.82, 1.05), boki=6)
        k += 1


def szachulec(s, p0, os, strona, L, z0, h, okna=(), drzwi=None, gr=0.2, kolor=RYGIEL, okno_z=(1.3, 2.3)):
    """Szkielet sciany muru pruskiego na licu (belki wystaja 4 cm): podwalina, oczep, slupy co ~1,3 m, rygiel w pol
    wysokosci, zastrzaly w skrajnych polach. okna / drzwi: numery pol (0..N-1) z otworami. Zwraca srodki pol (u)."""
    box, pt, n = _sciana(p0, os, strona)
    N = max(2, int(round(L / 1.3)))
    w = L / N
    d0, d1 = 0.04 - gr, 0.04
    box(s, DREWNO, 0, L, z0, z0 + gr, d0, d1, kolor * 0.9)
    box(s, DREWNO, 0, L, h - gr, h, d0, d1, kolor * 0.9)
    for i in range(N + 1):
        u = min(max(i * w - gr / 2, 0), L - gr)
        box(s, DREWNO, u, u + gr, z0 + gr, h - gr, d0, d1, kolor)
    zm = z0 + (h - z0) * 0.5
    dc = 0.04 - gr / 2
    for i in range(N):
        ua, ub = i * w + gr / 2, (i + 1) * w - gr / 2
        if i == drzwi:
            continue
        if i in okna:
            for zz in (z0 + okno_z[0] - 0.1, z0 + okno_z[1]):
                box(s, DREWNO, ua, ub, zz, zz + gr * 0.8, d0, d1, kolor)
            continue
        box(s, DREWNO, ua, ub, zm - gr * 0.4, zm + gr * 0.4, d0, d1, kolor)
        if i in (0, N - 1):
            zew, wew = (ua, ub) if i == 0 else (ub, ua)
            belka(s, DREWNO, pt(zew, z0 + gr, dc), pt(wew, zm, dc), gr * 0.8, kolor * 0.95, gora=n)
            belka(s, DREWNO, pt(wew, zm, dc), pt(zew, h - gr, dc), gr * 0.8, kolor * 0.95, gora=n)
    return [(i + 0.5) * w for i in range(N)]


def szczyt_pruski(s, x, y0, y1, h, k, strona, tynk=True, gr=0.2, kolor=RYGIEL):
    """Szczyt sciany szczytowej (x = lico zewnetrzne, strona = kierunek normalnej X): tynk + belki po krawedziach,
    slup srodkowy i jetka. tynk=False: sam szkielet."""
    yc = (y0 + y1) / 2
    t = 0.22
    if tynk:
        szczyt(s, x if strona < 0 else x - t, y0, y1, h, k, t, TYNK_K, mat=TYNK)
    xf = x + strona * (0.04 - gr / 2)
    g = np.array([float(strona), 0, 0])
    belka(s, DREWNO, (xf, y0 + 0.05, h), (xf, yc, h + k), gr * 0.8, kolor, gora=g)
    belka(s, DREWNO, (xf, y1 - 0.05, h), (xf, yc, h + k), gr * 0.8, kolor, gora=g)
    belka(s, DREWNO, (xf, yc, h), (xf, yc, h + k - 0.15), gr * 0.8, kolor, gora=g)
    zj = h + k * 0.45
    pol = (yc - y0) * (1 - 0.45)
    belka(s, DREWNO, (xf, yc - pol, zj), (xf, yc + pol, zj), gr * 0.8, kolor, gora=g)


def kalenica_slomiana(s, B, okap=0.5):
    walec(s, STRZECHA, (B.x0 - okap, B.yc, B.h + B.k + 0.18), (B.x1 + okap, B.yc, B.h + B.k + 0.18), 0.32, SLOMA * 0.78, boki=6)


def dach_slomiany(s, B, od=0.0, do=1.0, okap=0.7, okap_szczyt=0.5):
    for strona in (-1, 1):
        polac(s, B, strona, od, do, okap=okap, gr=0.38, x0=B.x0 - okap_szczyt, x1=B.x1 + okap_szczyt, mat=STRZECHA, kolor=SLOMA)
    if do >= 0.99:
        kalenica_slomiana(s, B, okap_szczyt)


def drabina(s, x, y, h, strona=-1, kolor=RUSZT):
    """Drabina oparta o sciane (y = lico sciany, strona = na zewnatrz)."""
    dol = y + strona * h * 0.28
    for dx in (-0.25, 0.25):
        belka(s, DREWNO, (x + dx, dol, 0), (x + dx, y + strona * 0.1, h), 0.08, kolor)
    n = int(h / 0.4)
    for k in range(1, n):
        f = k / n
        yy = dol + (y + strona * 0.1 - dol) * f
        belka(s, DREWNO, (x - 0.25, yy, h * f), (x + 0.25, yy, h * f), 0.05, kolor * 0.9)


def snopki(s, x, y, n=3):
    """Snopki slomy na strzeche, ulozone w piramide."""
    for warstwa in range(n):
        for k in range(n - warstwa):
            px = x + (k - (n - warstwa - 1) / 2) * 0.46
            walec(s, STRZECHA, (px, y - 0.65, 0.22 + warstwa * 0.38), (px, y + 0.65, 0.22 + warstwa * 0.38), 0.22,
                  SLOMA * (0.92 + 0.04 * ((k + warstwa) % 3)), boki=6)


def kozly(s, x, y):
    """Kozly do ciecia z balem."""
    for dx in (-0.6, 0.6):
        belka(s, DREWNO, (x + dx, y - 0.35, 0), (x + dx, y + 0.35, 0.85), 0.09, RUSZT)
        belka(s, DREWNO, (x + dx, y + 0.35, 0), (x + dx, y - 0.35, 0.85), 0.09, RUSZT)
    walec(s, DREWNO, (x - 1.3, y, 0.78), (x + 1.3, y, 0.78), 0.15, BALE, boki=6)


def drwa(s, x0, x1, y, h, r=0.13, strona=-1):
    """Sag polan pod sciana (y = lico, strona = na zewnatrz), polana wzdluz Y."""
    n = int((x1 - x0) / (2 * r))
    rz = int(h / (2 * r))
    for j in range(rz):
        for i in range(n):
            px = x0 + r + i * 2 * r + (r if j % 2 else 0)
            if px + r > x1:
                continue
            walec(s, DREWNO, (px, y + strona * 0.05, r + j * 2 * r * 0.9), (px, y + strona * 0.6, r + j * 2 * r * 0.9), r,
                  BALE * (0.8 + 0.08 * ((i * 3 + j) % 3)), boki=6)


def stos_desek(s, x, y, dl=3.0, n=5):
    for k in range(n):
        pudlo(s, DREWNO, (x - dl / 2, y - 0.6 + (k % 2) * 0.05, k * 0.07), (x + dl / 2, y + 0.6, k * 0.07 + 0.06), DESKA * (0.9 + 0.05 * (k % 2)))


# --- poziomy domu (22.11.2026, prosba uzytkownika) ---
# Trzy domy to teraz trzy poziomy JEDNEGO domu: poziom 1 = chata z bali (8 klod), poziom 2 = dom muru pruskiego,
# poziom 3 = duzy dom. Kazdy poziom ma 4 warianty (w = 0..3): inne wymiary, dach, kolor, komin i dodatki; warianty 1 i 3
# sa odbite w X (drzwi po drugiej stronie). Modele: SM_<Dom>_F<n> (w = 0) i SM_<Dom>_W<w>_F<n> (w = 1..3).
# Ulepszenie domu w grze: SM_<Dom>[_W<w>]_U (poprzedni poziom tego wariantu w rusztowaniu + material), potem F3 i F4.
# Drzwi zostaja w tym samym miejscu co w wariancie 0 (gra ustawia tam mieszkancow, ABudowa odbija X dla wariantow 1 i 3).
LUSTRO = (False, True, False, True)
GLINA = rgb(166, 138, 108)   # komin gliniany (plecionka oblepiona glina)
CEGLA = rgb(150, 98, 80)     # komin ceglany


def lustro(s):
    """Odbicie modelu w X (wariant domu z drzwiami po drugiej stronie)."""
    for mat, czesci in s.grupy.items():
        nowe = []
        for p, n, c, i, uv in czesci:
            p = p.copy(); p[:, 0] *= -1
            n = n.copy(); n[:, 0] *= -1
            uv = uv.copy(); uv[:, 0] *= -1
            nowe.append((p, n, c, np.asarray(i).reshape(-1, 3)[:, ::-1].copy(), uv))
        s.grupy[mat] = nowe
    return s


def dach_z(B, y, okap=0.0):
    """Wysokosc polaci dachu dwuspadowego (kalenica wzdluz X) nad punktem y."""
    pol = (B.y1 - B.y0) / 2
    return B.h + B.k * (1 - min(1.0, abs(y - B.yc) / pol))


def komin(s, B, x, y, rodzaj="kamien", nad=0.9, szer=0.8, z0=None):
    """Komin przebijajacy dach: od z0 (domyslnie mur) do nad m ponad kalenice; czapa i ciemny wylot."""
    gora = B.h + B.k + nad
    z0 = B.h - 0.6 if z0 is None else z0
    a = szer / 2
    if rodzaj == "glina":
        pudlo(s, TYNK, (x - a, y - a, z0), (x + a, y + a, gora), GLINA)
        for dx in (-a, a):  # drewniane naroza plecionki
            for dy in (-a, a):
                belka(s, DREWNO, (x + dx, y + dy, z0), (x + dx, y + dy, gora + 0.05), 0.09, RYGIEL)
        pudlo(s, DREWNO, (x - a - 0.08, y - a - 0.08, gora - 0.12), (x + a + 0.08, y + a + 0.08, gora + 0.04), RYGIEL)
    else:
        kol = CEGLA if rodzaj == "cegla" else MUR * 0.86
        pudlo(s, KAMIEN, (x - a, y - a, z0), (x + a, y + a, gora), kol)
        pudlo(s, KAMIEN, (x - a - 0.1, y - a - 0.1, gora - 0.22), (x + a + 0.1, y + a + 0.1, gora), kol * 0.85)
    pudlo(s, KAMIEN, (x - a + 0.12, y - a + 0.12, gora), (x + a - 0.12, y + a - 0.12, gora + 0.03), OKNO)  # wylot


def komin_szczytowy(s, B, strona, z_od=0.0, nad=0.7, rodzaj="kamien"):
    """Komin zewnetrzny przy scianie szczytowej (strona -1 = x0, +1 = x1), od ziemi ponad kalenice."""
    x = (B.x1 + 0.45) if strona > 0 else (B.x0 - 0.45)
    kol = CEGLA if rodzaj == "cegla" else MUR * 0.84
    pudlo(s, KAMIEN, (x - 0.5, B.yc - 0.65, -1.0), (x + 0.5, B.yc + 0.65, B.h - 0.2), kol)  # szeroka podstawa (palenisko)
    pudlo(s, KAMIEN, (x - 0.38, B.yc - 0.38, B.h - 0.2), (x + 0.38, B.yc + 0.38, B.h + B.k + nad), kol * 0.95)
    g = B.h + B.k + nad
    pudlo(s, KAMIEN, (x - 0.48, B.yc - 0.48, g - 0.2), (x + 0.48, B.yc + 0.48, g), kol * 0.82)
    pudlo(s, KAMIEN, (x - 0.24, B.yc - 0.24, g), (x + 0.24, B.yc + 0.24, g + 0.03), OKNO)


def szopka(s, B, dl=3.0, gl=1.6, x=None, h0=None, dach=STRZECHA, kolor=None, drewno=True):
    """Przybudowka z tylu domu (+Y, strona od kamery): slupki, dach pulpitowy oparty o sciane, sag drewna w srodku."""
    x = (B.x0 + B.x1) / 2 if x is None else x
    h0 = B.h - 0.3 if h0 is None else h0
    h1 = h0 - 0.8
    y0, y1 = B.y1, B.y1 + gl
    for xx in (x - dl / 2 + 0.1, x + dl / 2 - 0.1):
        belka(s, DREWNO, (xx, y1 - 0.1, -0.3), (xx, y1 - 0.1, h1), 0.14, RYGIEL)
    belka(s, DREWNO, (x - dl / 2, y1 - 0.1, h1), (x + dl / 2, y1 - 0.1, h1), 0.14, RYGIEL)
    gr = 0.3 if dach == STRZECHA else 0.1
    kol = (SLOMA if dach == STRZECHA else GONT) if kolor is None else kolor
    pts = [(x - dl / 2 - 0.2, y0 - 0.05, h0 + gr), (x + dl / 2 + 0.2, y0 - 0.05, h0 + gr),
           (x + dl / 2 + 0.2, y1 + 0.35, h1 - 0.15 + gr), (x - dl / 2 - 0.2, y1 + 0.35, h1 - 0.15 + gr)]
    plyta(s, dach, pts, gr, kol)
    if drewno:
        for k in range(2):
            px = x - dl / 2 + 0.4 + k * (dl - 0.8) / 2
            for j in range(3):
                walec(s, DREWNO, (px - 0.05, y0 + 0.2, 0.14 + j * 0.24), (px - 0.05, y1 - 0.25, 0.14 + j * 0.24), 0.12,
                      BALE * (0.82 + 0.06 * ((j + k) % 3)), boki=6)
                walec(s, DREWNO, (px + 0.22, y0 + 0.2, 0.14 + j * 0.24), (px + 0.22, y1 - 0.25, 0.14 + j * 0.24), 0.12,
                      BALE * (0.86 + 0.05 * ((j + 2 * k) % 3)), boki=6)


def lawka(s, x, y, dl=1.6):
    pudlo(s, DREWNO, (x - dl / 2, y - 0.18, 0.42), (x + dl / 2, y + 0.18, 0.5), DESKA * 0.85)
    for dx in (-dl / 2 + 0.15, dl / 2 - 0.25):
        pudlo(s, DREWNO, (x + dx, y - 0.14, 0), (x + dx + 0.1, y + 0.14, 0.42), RYGIEL)


def ganek(s, B, xc, z0, szer=2.2, gl=1.4, kolor=GONT):
    """Daszek nad drzwiami na dwoch slupkach (strona -Y)."""
    y = B.y0 - gl
    for dx in (-szer / 2, szer / 2):
        belka(s, DREWNO, (xc + dx, y, -0.2), (xc + dx, y, z0 + 2.7), 0.16, RYGIEL)
    belka(s, DREWNO, (xc - szer / 2 - 0.1, y, z0 + 2.7), (xc + szer / 2 + 0.1, y, z0 + 2.7), 0.16, RYGIEL)
    pts = [(xc - szer / 2 - 0.35, B.y0 + 0.05, z0 + 3.45), (xc + szer / 2 + 0.35, B.y0 + 0.05, z0 + 3.45),
           (xc + szer / 2 + 0.35, y - 0.4, z0 + 2.75), (xc - szer / 2 - 0.35, y - 0.4, z0 + 2.75)]
    plyta(s, DACH, pts, 0.1, kolor)


# --- poziom 1: chata z bali, strzecha ---
CHATA = Bryla(-3.5, 3.5, -2.5, 2.5, 3.0, 3.0, t=0.32)
#            pol dlugosci, pol szerokosci, wys. scian, wys. dachu, odcien slomy, odcien bali
CHATA_W = [(3.5, 2.5, 3.0, 3.0, 1.0, 1.0), (3.2, 2.4, 2.9, 3.35, 0.9, 0.9),
           (3.7, 2.6, 3.1, 2.8, 1.06, 1.06), (3.35, 2.5, 3.0, 3.15, 0.86, 0.95)]


def chata_B(w=0):
    L, Wd, h, k, _, _ = CHATA_W[w]
    return Bryla(-L, L, -Wd, Wd, h, k, t=0.32)


def chata(faza, w=0):
    s = Siatka()
    rng = np.random.default_rng(200 + 17 * w)
    L, _, _, _, ts, tb = CHATA_W[w]
    B = chata_B(w)
    z0 = 0.3
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    h = {1: 1.45, 2: B.h, 3: B.h, 4: B.h}[faza]
    zrab(s, B.x0, B.x1, B.y0, B.y1, h, z0, rng)
    przod = (B.x0, B.y0 - 0.02, 0)
    ud = L - 1.1  # drzwi zawsze 1,1 m na lewo od srodka (Wejscie w grze)
    if faza >= 2:
        k = 1 if faza < 4 else 2
        drzwi_dom(s, przod, "y", -1, ud, z0, 1.2, 2.3, DRZWI if k == 2 else rgb(30, 26, 24))
        okienko(s, przod, "y", -1, ud + 2.8, z0 + 1.1, 0.8, 0.75, okiennice=k == 2)
        okienko(s, (B.x0 - 0.02, B.y0, 0), "x", -1, B.y1 - B.y0 - 2.5, z0 + 1.1, 0.8, 0.75, okiennice=k == 2)
        if w == 2:
            okienko(s, (B.x0, B.y1 + 0.02, 0), "y", 1, L, z0 + 1.1, 0.7, 0.7, okiennice=k == 2)
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT * tb, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
    # Komin (od fazy 3 komin rosnie razem z dachem): gliniany, kamienny w dachu albo zewnetrzny przy szczycie.
    if faza >= 3:
        if w == 0:
            komin(s, B, 1.4, B.yc + 0.7, "glina", nad=0.6 if faza == 4 else -1.0, szer=0.75)
        elif w == 1:
            komin(s, B, -0.9, B.yc + 0.5, "kamien", nad=0.7 if faza == 4 else -1.2)
        elif w == 2:
            komin(s, B, B.x1 - 1.3, B.yc + 0.8, "glina", nad=0.5 if faza == 4 else -1.0, szer=0.7)
        else:
            komin_szczytowy(s, B, 1, nad=0.6 if faza == 4 else -1.4)
    if faza == 3:
        dach_slomiany(s, B, 0, 0.5)
    elif faza == 4:
        polac_kol = SLOMA * ts
        for strona in (-1, 1):
            polac(s, B, strona, 0, 1, okap=0.7, gr=0.38, x0=B.x0 - 0.5, x1=B.x1 + 0.5, mat=STRZECHA, kolor=polac_kol)
        walec(s, STRZECHA, (B.x0 - 0.5, B.yc, B.h + B.k + 0.18), (B.x1 + 0.5, B.yc, B.h + B.k + 0.18), 0.32, polac_kol * 0.78, boki=6)
        # dodatki wariantu
        if w == 0:
            drwa(s, B.x1 - 2.6, B.x1 - 0.4, B.y0 - 0.02, 1.0)
        elif w == 1:
            szopka(s, B, dl=3.0, gl=1.5, x=B.x0 + 1.9)
            beczka(s, B.x1 - 0.6, B.y0 - 0.6)
        elif w == 2:
            lawka(s, ud - L + 2.0, B.y0 - 0.45)
            drwa(s, B.x1 - 2.0, B.x1 - 0.4, B.y0 - 0.02, 0.8)
            plot(s, B.x1 + 0.6, B.x1 + 2.8, B.y0 + 0.4, B.y1 - 0.2, h=0.9, boki="ptd", krok=1.1)
        else:
            beczka(s, B.x0 + 0.6, B.y0 - 0.6)
            lawka(s, ud - L + 2.0, B.y0 - 0.45, 1.4)
            szopka(s, B, dl=2.4, gl=1.3, x=-0.6)
    # plac budowy
    if faza <= 2:
        stos_belek(s, -1.0, B.y0 - 3.4, 5.0, 5 if faza == 1 else 3)
        kozly(s, 2.6, B.y0 - 3.2)
    if faza == 1:
        stos_belek(s, 1.5, B.y0 - 4.4, 5.0, 4)
    if faza == 2:
        drabina(s, B.x0 + 0.8, B.y0, B.h + 0.6)
    if faza == 3:
        snopki(s, -2.0, B.y0 - 3.2, 3)
        snopki(s, 0.0, B.y0 - 3.2, 2)
        drabina(s, B.x1 - 1.0, B.y0, B.h + 0.8)
    return lustro(s) if LUSTRO[w] else s


# --- poziom 2: dom muru pruskiego ---
PRUSKI = Bryla(-4.5, 4.5, -3.0, 3.0, 3.4, 3.6, t=0.22)
#             pol dl., pol szer., wys. dachu, dach, kolor tynku
PRUSKI_W = [(4.5, 3.0, 3.6, "strzecha", rgb(234, 226, 208)), (4.2, 3.0, 3.9, "gont", rgb(228, 214, 184)),
            (4.6, 3.1, 3.4, "strzecha", rgb(216, 222, 224)), (4.3, 2.9, 3.7, "gont", rgb(236, 230, 214))]


def pruski_B(w=0):
    L, Wd, k, _, _ = PRUSKI_W[w]
    return Bryla(-L, L, -Wd, Wd, 3.4, k, t=0.22)


def mur_pruski(s, B, z0, faza, okna_przod=(1, 5), drzwi_przod=3, okna_bok=(1,), okna_tyl=(2, 4), tynk=TYNK_K):
    """Sciany domu muru pruskiego od z0 do B.h: faza 2 = sam szkielet, 3 = wypelnienie tynkiem (okna ciemne), 4 = gotowe."""
    L, W = B.x1 - B.x0, B.y1 - B.y0
    if faza >= 3:
        mury(s, B.x0, B.x1, B.y0, B.y1, B.h, B.t, tynk, z0=z0, mat=TYNK)
    sciany = (((B.x0, B.y0, 0), "y", -1, L, okna_przod, drzwi_przod),
              ((B.x0, B.y1, 0), "y", 1, L, okna_tyl, None),
              ((B.x0, B.y0, 0), "x", -1, W, okna_bok, None),
              ((B.x1, B.y0, 0), "x", 1, W, okna_bok, None))
    for p0, os, st, dl, okna, drz in sciany:
        srodki = szachulec(s, p0, os, st, dl, z0, B.h, okna, drz)
        if faza >= 3:
            gotowe = faza >= 4
            for i in okna:
                if i < len(srodki):
                    okienko(s, p0, os, st, srodki[i], z0 + 1.3, 0.8, 0.85, okiennice=gotowe)
            if drz is not None:
                drzwi_dom(s, p0, os, st, srodki[drz], z0, 1.1, 2.4, DRZWI if gotowe else rgb(30, 26, 24))


def dom_pruski(faza, w=0):
    s = Siatka()
    rng = np.random.default_rng(300 + 17 * w)
    L, _, _, dach, tynk = PRUSKI_W[w]
    B = pruski_B(w)
    z0 = 0.45
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    N = max(2, int(round((B.x1 - B.x0) / 1.3)))
    drz = N // 2  # pole ze drzwiami najblizej srodka
    if faza == 1:
        # podwalina i slupy narozne + przy drzwiach
        szachulec_f1 = [((B.x0, B.y0, 0), "y", -1, B.x1 - B.x0), ((B.x0, B.y1, 0), "y", 1, B.x1 - B.x0),
                        ((B.x0, B.y0, 0), "x", -1, B.y1 - B.y0), ((B.x1, B.y0, 0), "x", 1, B.y1 - B.y0)]
        for p0, os, st, dl in szachulec_f1:
            box, pt, n = _sciana(p0, os, st)
            box(s, DREWNO, 0, dl, z0, z0 + 0.2, -0.16, 0.04, RYGIEL * 0.9)
            for u in (0, dl - 0.2):
                box(s, DREWNO, u, u + 0.2, z0 + 0.2, B.h, -0.16, 0.04, RYGIEL)
        box, pt, n = _sciana((B.x0, B.y0, 0), "y", -1)
        for u in (L - 1.1, L + 0.9):
            box(s, DREWNO, u, u + 0.2, z0 + 0.2, B.h, -0.16, 0.04, RYGIEL)
        stos_belek(s, -1.5, B.y0 - 3.4, 5.5, 5)
        stos_belek(s, 2.5, B.y0 - 3.6, 5.5, 4)
        kozly(s, -3.2, B.y0 - 4.6)
    else:
        okna_przod = (1, N - 2) if N >= 6 else (1,)
        mur_pruski(s, B, z0, faza, okna_przod=okna_przod, drzwi_przod=drz, okna_tyl=(2, N - 3), tynk=tynk)
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt_pruski(s, x, B.y0, B.y1, B.h, B.k, st, tynk=faza >= 3)
        wiezba(s, B, kolor=RYGIEL * 1.4)
    if faza >= 3:
        nad = 0.8 if faza == 4 else -1.2
        if w in (0, 2):
            komin(s, B, -1.6 if w == 0 else 2.2, B.yc + 0.5, "kamien", nad=nad)
        elif w == 1:
            komin(s, B, 1.2, B.yc + 0.6, "cegla", nad=nad)
        else:
            komin_szczytowy(s, B, -1, nad=nad, rodzaj="cegla")
    if faza == 2:
        stos_belek(s, 2.5, B.y0 - 3.6, 5.5, 3)
        drabina(s, B.x0 + 1.0, B.y0, B.h + 0.6)
        stos_desek(s, -2.0, B.y0 - 3.4)
    if faza == 3:
        if dach == "strzecha":
            dach_slomiany(s, B, 0, 0.5)
            snopki(s, -2.5, B.y0 - 3.3, 3)
            snopki(s, -0.5, B.y0 - 3.3, 3)
        else:
            for st in (-1, 1):
                polac(s, B, st, 0, 0.5, okap=0.6)
            stos_desek(s, -1.5, B.y0 - 3.4, n=4)
        drabina(s, B.x1 - 1.2, B.y0, B.h + 0.8)
    if faza == 4:
        if dach == "strzecha":
            dach_slomiany(s, B)
        else:
            for st in (-1, 1):
                polac(s, B, st, 0, 1, okap=0.6, kolor=GONT * (1.08 if w == 1 else 0.95))
        if w == 0:
            drwa(s, B.x0 + 0.4, B.x0 + 2.2, B.y0 - 0.02, 1.1)
        elif w == 1:
            ganek(s, B, B.x0 + (drz + 0.5) * (2 * L / N), z0, szer=1.9, gl=1.2, kolor=GONT * 1.08)
            drwa(s, B.x1 - 2.2, B.x1 - 0.4, B.y0 - 0.02, 1.0)
            lawka(s, B.x0 + 1.6, B.y0 - 0.45)
        elif w == 2:
            szopka(s, B, dl=3.6, gl=1.7, x=B.x1 - 2.2)
            beczka(s, B.x0 + 0.7, B.y0 - 0.7)
        else:
            szopka(s, B, dl=3.0, gl=1.5, x=B.x0 + 2.0, dach=DACH)
            drwa(s, B.x1 - 2.0, B.x1 - 0.4, B.y0 - 0.02, 0.9)
    return lustro(s) if LUSTRO[w] else s


# --- poziom 3: duzy dom (kamienny parter, pietro muru pruskiego, gont, komin) ---
DUZY = Bryla(-5.5, 5.5, -3.5, 3.5, 6.2, 4.2, t=0.22)
DUZY_PARTER = 3.3
#           pol dl., pol szer., dach (kolor), mur, tynk pietra
DUZY_W = [(5.5, 3.5, GONT, MUR, TYNK_K), (5.2, 3.4, rgb(132, 94, 80), MUR * 0.95, rgb(230, 216, 186)),
          (5.7, 3.6, GONT * 0.9, rgb(214, 208, 198), rgb(218, 224, 226)), (5.4, 3.5, rgb(122, 108, 90), MUR * 0.98, TYNK_K)]


def duzy_B(w=0):
    L, Wd, _, _, _ = DUZY_W[w]
    return Bryla(-L, L, -Wd, Wd, 6.2, 4.2, t=0.22)


def dom_duzy(faza, w=0):
    s = Siatka()
    rng = np.random.default_rng(400 + 17 * w)
    L, _, kol_dach, kol_mur, tynk = DUZY_W[w]
    B = duzy_B(w)
    z0 = 0.45
    hp = DUZY_PARTER
    ud = L - 2.1  # drzwi 2,1 m na lewo od srodka (Wejscie w grze)
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0, m=0.15)
    hm = {1: 1.6, 2: hp, 3: hp, 4: hp}[faza]
    mury(s, B.x0, B.x1, B.y0, B.y1, hm, 0.5, kol_mur, z0=z0)
    przod = (B.x0, B.y0 - 0.02, 0)
    if faza >= 2:
        gotowe = faza >= 4
        drzwi_dom(s, przod, "y", -1, ud, z0, 1.3, 2.4, DRZWI if gotowe else rgb(30, 26, 24))
        for u in (ud + 3.2, ud + 5.4):
            if u < 2 * L - 0.6:
                okienko(s, przod, "y", -1, u, z0 + 1.2, 0.8, 0.9, okiennice=gotowe)
        if w != 1:
            okienko(s, przod, "y", -1, ud - 1.9, z0 + 1.2, 0.8, 0.9, okiennice=gotowe)
        okienko(s, (B.x0 - 0.02, B.y0, 0), "x", -1, B.y1 - B.y0 - 3.5, z0 + 1.2, 0.8, 0.9, okiennice=gotowe)
        okienko(s, (B.x0, B.y1 + 0.02, 0), "y", 1, L, z0 + 1.2, 0.8, 0.9, okiennice=gotowe)
        # belki stropu wystajace z muru (pas miedzy parterem a pietrem)
        pudlo(s, DREWNO, (B.x0 - 0.06, B.y0 - 0.06, hp), (B.x1 + 0.06, B.y1 + 0.06, hp + 0.25), RYGIEL)
        Bp = Bryla(B.x0, B.x1, B.y0, B.y1, B.h, B.k, t=0.22)
        N = max(2, int(round((B.x1 - B.x0) / 1.3)))
        okna_p = tuple(range(1, N - 1, 2)) if w != 2 else tuple(range(1, N - 1, 3))
        mur_pruski(s, Bp, hp + 0.25, faza, okna_przod=okna_p, drzwi_przod=None, okna_bok=(2,), okna_tyl=(2, N - 3), tynk=tynk)
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt_pruski(s, x, B.y0, B.y1, B.h, B.k, st, tynk=faza >= 3)
        wiezba(s, B, kolor=RYGIEL * 1.4)
    if faza >= 3:
        # kominy: wariant 0 jak dawniej, 1 dwa ceglane, 2 jeden przy lewym szczycie, 3 kamienny + ceglany
        miejsca = {0: [(2.85, 0.0, "kamien")], 1: [(-2.6, 0.4, "cegla"), (2.9, 0.4, "cegla")],
                   2: [(-L + 1.4, 0.3, "kamien")], 3: [(-1.8, 0.5, "kamien"), (3.4, -0.3, "cegla")]}[w]
        for x, y, r in miejsca:
            if faza == 4:
                komin(s, B, x, y, r, nad=1.0, szer=0.9, z0=B.h - 0.5)
            else:
                pudlo(s, KAMIEN, (x - 0.45, y - 0.45, B.h - 0.5), (x + 0.45, y + 0.45, B.h + 1.2), MUR * 0.9)
    if faza == 3:
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5, okap=0.6, kolor=kol_dach)
    elif faza == 4:
        for st in (-1, 1):
            polac(s, B, st, 0, 1, okap=0.6, kolor=kol_dach)
    # plac budowy: dalej niz przy malych domach, bo rusztowanie siega 1,5 m od muru
    if faza == 1:
        rng2 = np.random.default_rng(401 + w)
        stos_kamieni(s, -2.5, B.y0 - 4.6, rng2, 3)
        stos_kamieni(s, 0.5, B.y0 - 4.4, rng2, 2)
        stos_belek(s, 3.8, B.y0 - 4.7, 5.0, 5)
        rusztowanie(s, B.x0, B.x1, B.y0, B.y1, hm, boki=(True, False, False, False))
    elif faza == 2:
        stos_belek(s, 3.8, B.y0 - 4.7, 5.0, 3)
        stos_desek(s, 0.0, B.y0 - 4.6)
        rusztowanie(s, B.x0, B.x1, B.y0, B.y1, B.h - 1.0, boki=(True, False, True, False))
    elif faza == 3:
        rusztowanie(s, B.x0, B.x1, B.y0, B.y1, B.h - 1.0, boki=(False, True, False, True))
        stos_desek(s, 0.0, B.y0 - 4.6, n=3)
        skrzynia(s, -4.0, B.y0 - 4.5, 0.8)
    else:
        if w == 0:
            drwa(s, B.x1 - 2.4, B.x1 - 0.5, B.y0 - 0.02, 1.2)
            beczka(s, B.x0 + 0.9, B.y0 - 0.8)
        elif w == 1:
            szopka(s, B, dl=4.0, gl=1.8, x=B.x1 - 2.6, h0=hp + 0.1, dach=DACH, kolor=kol_dach)
            lawka(s, B.x0 + 1.4, B.y0 - 0.45)
            beczka(s, B.x1 - 0.8, B.y0 - 0.8)
        elif w == 2:
            ganek(s, B, B.x0 + ud, z0, szer=2.2, gl=1.5, kolor=kol_dach)
            drwa(s, B.x1 - 2.2, B.x1 - 0.5, B.y0 - 0.02, 1.0)
        else:
            szopka(s, B, dl=3.4, gl=1.6, x=B.x0 + 2.4, h0=hp + 0.1)
            drwa(s, B.x1 - 2.4, B.x1 - 0.5, B.y0 - 0.02, 1.2)
            lawka(s, B.x0 + ud + 1.6, B.y0 - 0.45, 1.3)
    return lustro(s) if LUSTRO[w] else s


def przebudowa(stary, B, w, kamien=False):
    """Ulepszenie domu: gotowy dom poprzedniego poziomu (wariant w) w rusztowaniu, przed nim material nowego poziomu."""
    s = stary(4, w)
    if LUSTRO[w]:
        lustro(s)  # material i rusztowanie dokladamy w ukladzie nieodbitym, na koncu calosc odbijamy z powrotem
    rusztowanie(s, B.x0, B.x1, B.y0, B.y1, B.h - 0.6, boki=(True, False, True, True), odst=0.8)
    stos_belek(s, -1.5, B.y0 - 3.6, 5.0, 5)
    stos_desek(s, 2.2, B.y0 - 3.5, n=5)
    if kamien:
        stos_kamieni(s, 2.2, B.y0 - 4.6, np.random.default_rng(450 + w), 3)
    kozly(s, -3.4, B.y0 - 4.6)
    return lustro(s) if LUSTRO[w] else s


# --- obozowisko startowe ---

def namiot():
    """Namiot dwuspadowy z plotna dla jednej rodziny: 3,6 x 3,2 m, 2,7 m wysokosci.
    Wejscie (ciemny trojkat) na szczycie -X; w grze namioty ustawiane wejsciem do ogniska."""
    s = Siatka()
    x0, x1, yh, H = -1.8, 1.8, 1.6, 2.7
    for st in (-1, 1):
        dol = [np.array([x0, st * yh, 0.0]), np.array([x1, st * yh, 0.0])]
        gora = [np.array([x1, 0.0, H]), np.array([x0, 0.0, H])]
        pts = [dol[0], dol[1], gora[0], gora[1]]
        wielokat(s, TYNK, pts, PLOTNO, od=(0, 0, H * 0.3))
        wn = np.array([0, -st * H, -yh]); wn /= np.linalg.norm(wn)  # strona wewnetrzna, 2 cm pod plotnem
        wielokat(s, TYNK, [p + wn * 0.02 for p in pts], PLOTNO * 0.5, n=wn)
        # brudny pas przy ziemi
        f = 0.12
        pas = [dol[0], dol[1], dol[1] + (gora[0] - dol[1]) * f, dol[0] + (gora[1] - dol[0]) * f]
        nz = np.array([0, st * H, yh]); nz /= np.linalg.norm(nz)
        wielokat(s, TYNK, [p + nz * 0.015 for p in pas], PLOTNO * 0.78, n=nz)
    # tyl (+X) zamkniety, przod (-X) z ciemnym wejsciem
    wielokat(s, TYNK, [(x1, -yh, 0), (x1, yh, 0), (x1, 0, H)], PLOTNO * 0.92, n=np.array([1.0, 0, 0]))
    wielokat(s, TYNK, [(x0, -yh, 0), (x0, yh, 0), (x0, 0, H)], PLOTNO * 0.92, n=np.array([-1.0, 0, 0]))
    wielokat(s, DREWNO, [(x0 - 0.02, -0.75, 0), (x0 - 0.02, 0.75, 0), (x0 - 0.02, 0, 1.9)], OKNO, n=np.array([-1.0, 0, 0]))
    # odchylone poly wejscia
    for st in (-1, 1):
        wielokat(s, TYNK, [(x0 - 0.03, st * 0.75, 0), (x0 - 0.03, 0, 1.9), (x0 - 0.5, st * 0.95, 0)], PLOTNO * 0.85, od=(x0 + 1, 0, 0.5))
    # maszty, kalenica, linki do sledzi
    for x in (x0 - 0.05, x1 + 0.05):
        belka(s, DREWNO, (x, 0, 0), (x, 0, H + 0.35), 0.1, RUSZT)
    belka(s, DREWNO, (x0 - 0.05, 0, H + 0.03), (x1 + 0.05, 0, H + 0.03), 0.08, RUSZT)
    for x, kx in ((x0 - 0.05, -1), (x1 + 0.05, 1)):
        belka(s, DREWNO, (x, 0, H + 0.3), (x + kx * 1.4, 0, 0.0), 0.03, RYGIEL)
        belka(s, DREWNO, (x + kx * 1.4, 0, -0.1), (x + kx * 1.4, 0, 0.2), 0.06, RUSZT)
    for st in (-1, 1):
        for x in (x0 + 0.4, x1 - 0.4):
            belka(s, DREWNO, (x, st * (yh - 0.3), 0.55), (x, st * (yh + 0.7), 0.0), 0.03, RYGIEL)
    # tobolek i garnek przy wejsciu
    pudlo(s, TYNK, (x0 - 0.9, 1.0, 0), (x0 - 0.4, 1.5, 0.4), PLOTNO * 0.7)
    return s


def ognisko():
    """Ognisko obozu: krag kamieni, popiol, polana, zar."""
    s = Siatka()
    for k in range(9):
        a = 2 * np.pi * k / 9
        x, y = 0.75 * np.cos(a), 0.75 * np.sin(a)
        pudlo(s, KAMIEN, (x - 0.16, y - 0.14, 0), (x + 0.16, y + 0.14, 0.22), MUR_COKOL * (0.85 + 0.1 * (k % 2)))
    walec(s, KAMIEN, (0, 0, -0.05), (0, 0, 0.04), 0.6, rgb(60, 56, 52), boki=10)
    for a in (0.3, 1.35, 2.4):
        d = np.array([np.cos(a), np.sin(a), 0]) * 0.55
        walec(s, DREWNO, -d + (0, 0, 0.12), d + (0, 0, 0.28), 0.08, rgb(52, 40, 32), boki=6)
    for k in range(5):
        a = k * 1.3
        x, y = 0.25 * np.cos(a), 0.25 * np.sin(a)
        pudlo(s, KAMIEN, (x - 0.08, y - 0.08, 0.04), (x + 0.08, y + 0.08, 0.12), rgb(255, 120, 40))
    # pien do siedzenia
    walec(s, DREWNO, (-1.6, -1.6, 0.2), (-0.6, -2.0, 0.2), 0.2, BALE, boki=6)
    return s


# --- budynki drewna (etap 1 ekonomii, 21.11.2026): plac skladowy, chata drwala, chata borowego, pilarnia ---
ZIEMIA = rgb(118, 98, 74)    # ubita ziemia placu / klepisko
DOL = rgb(46, 38, 30)        # dol pilarski
ZIELEN = rgb(92, 118, 64)    # sadzonki (przygaszona, jak korony drzew)
ZIELEN_C = rgb(70, 96, 52)
TROCINY = rgb(200, 176, 132)


def klepisko(s, x0, x1, y0, y1, kolor=ZIEMIA):
    """Ubita ziemia: plyta siegajaca 3 m w dol (na pochylym terenie nie wisi), 6 cm nad gruntem."""
    pudlo(s, TYNK, (x0, y0, -3.0), (x1, y1, 0.06), kolor)


def plyta(s, mat, rogi, gr, kolor):
    """Plaska plyta (np. daszek) z 4 rogow gornej powierzchni, gruba gr w dol."""
    g = [np.asarray(p, float) for p in rogi]
    d = [p - np.array([0, 0, gr]) for p in g]
    w = sum(g) / 4 - np.array([0, 0, gr / 2])
    wielokat(s, mat, g, kolor, od=w)
    wielokat(s, mat, d, kolor * 0.6, od=w)
    for k in range(4):
        k2 = (k + 1) % 4
        wielokat(s, mat, [g[k], g[k2], d[k2], d[k]], kolor * 0.75, od=w)


def plot(s, x0, x1, y0, y1, h=1.1, brama=None, boki="pltd", zerdzie=True, krok=1.6):
    """Plot z zerdzi: slupki co ~krok m, dwie zerdzie poziome. boki: p = przod (-Y), t = tyl (+Y), l = -X, d = +X.
    brama: (u0, u1) - przerwa w przodzie (x od..do)."""
    odc = []
    if "p" in boki:
        if brama:
            odc += [((x0, y0), (brama[0], y0)), ((brama[1], y0), (x1, y0))]
        else:
            odc.append(((x0, y0), (x1, y0)))
    if "t" in boki: odc.append(((x0, y1), (x1, y1)))
    if "l" in boki: odc.append(((x0, y0), (x0, y1)))
    if "d" in boki: odc.append(((x1, y0), (x1, y1)))
    for k, (a, b) in enumerate(odc):
        L = np.hypot(b[0] - a[0], b[1] - a[1])
        n = max(1, int(round(L / krok)))
        for i in range(n + 1):
            x = a[0] + (b[0] - a[0]) * i / n
            y = a[1] + (b[1] - a[1]) * i / n
            belka(s, DREWNO, (x, y, -0.2), (x, y, h + 0.12), 0.12, RUSZT * (0.9 + 0.1 * ((i + k) % 2)))
        if zerdzie:
            for z in (h * 0.42, h * 0.9):
                belka(s, DREWNO, (a[0], a[1], z), (b[0], b[1], z), 0.07, BELKA * 0.8)


def stozek(s, mat, p, r, h, kolor, boki=6):
    """Stozek (korona sadzonki) od podstawy w p."""
    p = np.asarray(p, float)
    kat = np.linspace(0, 2 * np.pi, boki, endpoint=False)
    kr = [p + np.array([np.cos(t) * r, np.sin(t) * r, 0]) for t in kat]
    w = p + np.array([0, 0, h * 0.3])
    top = p + np.array([0, 0, h])
    for k in range(boki):
        wielokat(s, mat, [kr[k], kr[(k + 1) % boki], top], kolor * (0.88 + 0.12 * (k % 2)), od=w)
    wielokat(s, mat, kr, kolor * 0.6, od=w)


def sadzonka(s, x, y, wys, rng):
    belka(s, DREWNO, (x, y, 0.0), (x, y, wys * 0.45), 0.04, BALE * 0.8)
    stozek(s, TYNK, (x, y, wys * 0.2), wys * 0.28, wys * 0.85, (ZIELEN if rng.uniform() < 0.6 else ZIELEN_C) * rng.uniform(0.9, 1.08))


def pieniek(s, x, y, siekiera=True):
    """Pniak do rabania z wbita siekiera."""
    walec(s, DREWNO, (x, y, -0.2), (x, y, 0.5), 0.34, BALE * 0.85, boki=8)
    walec(s, DREWNO, (x, y, 0.49), (x, y, 0.52), 0.3, TROCINY, boki=8, denka=True)
    if siekiera:
        belka(s, DREWNO, (x + 0.05, y, 0.55), (x + 0.45, y + 0.15, 1.05), 0.05, BELKA)
        pudlo(s, DREWNO, (x - 0.06, y - 0.03, 0.48), (x + 0.12, y + 0.03, 0.62), rgb(88, 88, 92))


def drewutnia(s, x0, x1, y0, y1, hw, hz):
    """Przybudowka pod jednospadowym daszkiem: od sciany (x0, wysokosc hw) do slupow (x1, hz), drwa w srodku."""
    for y in (y0 + 0.1, y1 - 0.1):
        belka(s, DREWNO, (x1 - 0.1, y, -0.2), (x1 - 0.1, y, hz), 0.14, RUSZT)
    belka(s, DREWNO, (x1 - 0.1, y0, hz - 0.05), (x1 - 0.1, y1, hz - 0.05), 0.14, RUSZT * 0.9)
    plyta(s, DREWNO, [(x0 - 0.1, y0 - 0.3, hw + 0.1), (x1 + 0.35, y0 - 0.3, hz), (x1 + 0.35, y1 + 0.3, hz), (x0 - 0.1, y1 + 0.3, hw + 0.1)], 0.1, DESKA_SZCZYT)
    for yy in np.arange(y0 + 0.15, y1 - 0.5, 0.62):
        drwa(s, x0 + 0.1, x1 - 0.3, yy, min(hz, 1.4), strona=1)


DRWAL = Bryla(-3.4, 2.2, -2.2, 2.2, 2.6, 2.6, t=0.3)
DRWAL3 = Bryla(-4.8, 2.2, -2.3, 2.3, 2.7, 2.7, t=0.3)


def chata_drwala(faza, poziom=1):
    """Chata drwala: zrab z bali, dach z gontu, z boku drewutnia, przed chata pniak z siekiera (stos klod - w grze).
    Poziom 2: komin z gliny, druga drewutnia od -X, kozly z pila i klodami. Poziom 3 (trzy rodziny): dluzsza chata
    z dwojgiem drzwi i kamiennym kominem, dwie drewutnie, dwa pniaki, sanie do zwozki klod."""
    s = Siatka()
    rng = np.random.default_rng(500 + 10 * (poziom - 1))
    B = DRWAL3 if poziom == 3 else DRWAL
    z0 = 0.3
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    h = {1: 1.3, 2: B.h, 3: B.h, 4: B.h}[faza]
    zrab(s, B.x0, B.x1, B.y0, B.y1, h, z0, rng)
    przod = (B.x0, B.y0 - 0.02, 0)
    if faza >= 2:
        gotowe = faza == 4
        drzwi_dom(s, przod, "y", -1, 2.4, z0, 1.1, 2.1, DRZWI if gotowe else rgb(30, 26, 24))
        okienko(s, przod, "y", -1, 4.4, z0 + 1.0, 0.7, 0.65, okiennice=gotowe)
        okienko(s, (B.x0 - 0.02, B.y0, 0), "x", -1, 2.2, z0 + 1.0, 0.7, 0.65, okiennice=gotowe)
        if poziom == 3:
            drzwi_dom(s, przod, "y", -1, 5.9, z0, 1.0, 2.1, DRZWI * 0.92 if gotowe else rgb(30, 26, 24))
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.5)
    if faza == 4:
        drewutnia(s, B.x1, B.x1 + 1.9, B.y0 + 0.3, B.y1 - 0.3, B.h - 0.2, 1.9)
        pieniek(s, -2.4, B.y0 - 1.3)
        walec(s, DREWNO, (-1.7, B.y0 - 1.8, 0.16), (-1.0, B.y0 - 1.6, 0.16), 0.16, BALE, boki=6)  # klocek do rabania
        if poziom == 2:
            komin(s, B, -2.4, 0.9, rodzaj="glina", nad=0.7, szer=0.7)
        if poziom >= 2:
            kozly(s, 0.4, B.y0 - 2.4)
            walec(s, DREWNO, (-0.6, B.y0 - 2.4, 0.95), (1.4, B.y0 - 2.4, 0.95), 0.18, BALE, boki=7)  # kloda na kozlach
            pudlo(s, DREWNO, (0.55, B.y0 - 2.42, 0.75), (0.6, B.y0 - 2.38, 1.3), rgb(88, 88, 92))  # pila
            for k in range(3):  # siekiery oparte o sciane
                belka(s, DREWNO, (B.x0 + 0.6 + k * 0.25, B.y0 - 0.25, 0.0), (B.x0 + 0.65 + k * 0.25, B.y0 - 0.08, 0.9), 0.05, BELKA)
        if poziom == 3:
            komin(s, B, -1.0, 0.9, rodzaj="kamien", nad=0.8, szer=0.8)
            komin(s, B, -3.9, -0.9, rodzaj="glina", nad=0.6, szer=0.6)
            pieniek(s, -4.0, B.y0 - 1.3)
            # sanie do zwozki klod z dwiema klodami
            for y in (B.y0 - 3.3, B.y0 - 2.5):
                belka(s, DREWNO, (-4.6, y, 0.08), (-2.2, y, 0.08), 0.1, RUSZT)
                belka(s, DREWNO, (-2.2, y, 0.08), (-1.8, y, 0.35), 0.1, RUSZT)
            for k, z in enumerate((0.35, 0.35)):
                walec(s, DREWNO, (-4.7, B.y0 - 3.1 + k * 0.42, z), (-2.0, B.y0 - 3.1 + k * 0.42, z), 0.2, BALE * (0.95 + 0.05 * k), boki=7)
    if faza <= 2:
        stos_belek(s, -0.8, B.y0 - 3.0, 4.5, 5 if faza == 1 else 3)
        kozly(s, 2.6, B.y0 - 3.0)
    if faza == 2:
        drabina(s, B.x0 + 0.8, B.y0, B.h + 0.6)
    if faza == 3:
        stos_desek(s, -0.8, B.y0 - 3.0, 2.6, 4)
        drabina(s, B.x1 - 1.0, B.y0, B.h + 0.8)
    return s


BOROWY = Bryla(-4.4, 1.0, -2.2, 2.2, 2.6, 2.7, t=0.3)
BOROWY3 = Bryla(-5.4, 1.0, -2.3, 2.3, 2.7, 2.8, t=0.3)


def szkolka(s, x0, x1, y0, y1, rng, rzedy=3, w_rzedzie=5, brama=None):
    """Szkolka mlodych drzew: grzadka, plotek, sadzonki w rzedach."""
    pudlo(s, TYNK, (x0, y0, -3.0), (x1, y1, 0.08), ZIEMIA * 0.85)
    plot(s, x0 - 0.1, x1 + 0.1, y0 - 0.1, y1 + 0.1, h=0.6, boki="ptd", krok=1.1, brama=brama)
    for yy in np.linspace(y0 + 0.6, y1 - 0.6, rzedy):
        for xx in np.linspace(x0 + 0.45, x1 - 0.45, w_rzedzie):
            sadzonka(s, xx + rng.uniform(-0.1, 0.1), yy + rng.uniform(-0.1, 0.1), rng.uniform(0.45, 0.95), rng)


def chata_borowego(faza, poziom=1):
    """Chata borowego: zrab, obok ogrodzona szkolka mlodych drzew (sadzonki w rzedach).
    Poziom 1: strzecha, jedna szkolka. 2: gont, komin z gliny, szkolka wieksza, beczka z woda i lawka.
    3 (dwie rodziny): dluzsza chata z dwojgiem drzwi, kamienny komin, dwie szkolki i grzadka przed chata."""
    s = Siatka()
    rng = np.random.default_rng(600 + 10 * poziom)
    B = BOROWY3 if poziom == 3 else BOROWY
    z0 = 0.3
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    h = {1: 1.3, 2: B.h, 3: B.h, 4: B.h}[faza]
    zrab(s, B.x0, B.x1, B.y0, B.y1, h, z0, rng)
    przod = (B.x0, B.y0 - 0.02, 0)
    if faza >= 2:
        gotowe = faza == 4
        drzwi_dom(s, przod, "y", -1, 2.4, z0, 1.1, 2.1, DRZWI if gotowe else rgb(30, 26, 24))
        okienko(s, przod, "y", -1, 4.3, z0 + 1.0, 0.7, 0.65, okiennice=gotowe)
        if poziom == 3:
            drzwi_dom(s, przod, "y", -1, 5.6, z0, 1.0, 2.1, DRZWI * 0.92 if gotowe else rgb(30, 26, 24))
            okienko(s, (B.x0 - 0.02, B.y0, 0), "x", -1, 2.3, z0 + 1.0, 0.7, 0.65, okiennice=gotowe)
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
    if faza == 3:
        if poziom == 1:
            dach_slomiany(s, B, 0, 0.5)
        else:
            for st in (-1, 1):
                polac(s, B, st, 0, 0.5, okap=0.5)
    if faza == 4:
        if poziom == 1:
            dach_slomiany(s, B)
            szkolka(s, 1.7, 5.0, -1.8, 1.8, rng)
        else:
            for st in (-1, 1):
                polac(s, B, st, 0, 1.0, okap=0.55)
            if poziom == 2:
                komin(s, B, -1.0, 0.9, rodzaj="glina", nad=0.7, szer=0.7)
                szkolka(s, 1.7, 5.0, -2.2, 2.2, rng, rzedy=4)
                beczka(s, -0.4, B.y0 - 0.8)
                lawka(s, -3.2, B.y0 - 0.6, 1.4)
            else:
                komin(s, B, -1.6, 0.9, rodzaj="kamien", nad=0.8, szer=0.8)
                komin(s, B, -4.2, -0.9, rodzaj="glina", nad=0.6, szer=0.6)
                szkolka(s, 1.7, 5.0, -3.4, -0.2, rng, rzedy=3)
                szkolka(s, 1.7, 5.0, 0.2, 3.4, rng, rzedy=3, w_rzedzie=4)
                pudlo(s, TYNK, (-4.6, B.y0 - 2.4, -3.0), (-2.6, B.y0 - 1.2, 0.08), ZIEMIA * 0.8)  # grzadka z siewkami
                for xx in np.linspace(-4.4, -2.8, 5):
                    sadzonka(s, xx, B.y0 - 1.8, rng.uniform(0.25, 0.4), rng)
                beczka(s, -0.4, B.y0 - 0.8)
                beczka(s, 0.3, B.y0 - 0.7)
        skrzynia(s, -1.4, B.y0 - 0.9, 0.6)  # kosz z sadzonkami i motyka przy drzwiach
        for k in range(3):
            sadzonka(s, -1.55 + k * 0.15, B.y0 - 0.9, 0.55, rng)
        belka(s, DREWNO, (-0.7, B.y0 - 0.15, 0.0), (-0.5, B.y0 - 0.05, 1.3), 0.05, BELKA)
    if faza <= 2:
        stos_belek(s, -2.0, B.y0 - 3.0, 4.5, 5 if faza == 1 else 3)
        kozly(s, 0.6, B.y0 - 2.9)
    if faza == 2:
        drabina(s, B.x0 + 0.8, B.y0, B.h + 0.6)
    if faza == 3:
        if poziom == 1:
            snopki(s, -2.6, B.y0 - 3.0, 3)
            snopki(s, -0.8, B.y0 - 3.0, 2)
        else:
            stos_desek(s, -1.6, B.y0 - 3.0, 2.6, 4)
        drabina(s, B.x1 - 1.0, B.y0, B.h + 0.8)
    return s


PILA = Bryla(-4.0, 4.0, -2.4, 2.4, 2.7, 2.0, t=0.2)
PILA3 = Bryla(-5.4, 5.4, -2.6, 2.6, 2.9, 2.2, t=0.2)


def dol_pilarski(s, xc, pol, faza, gotowe):
    """Dol pilarski z obramowaniem z bali (dlugosc 2*pol); gotowy: kozly, kloda, pila w klodzie, trociny."""
    pudlo(s, TYNK, (xc - pol, -0.45, 0.061), (xc + pol, 0.45, 0.072), DOL)
    for y in (-0.55, 0.55):
        walec(s, DREWNO, (xc - pol - 0.2, y, 0.12), (xc + pol + 0.2, y, 0.12), 0.11, BALE * 0.9, boki=6)
    if faza >= 2:
        for x in (xc - pol - 0.2, xc + pol + 0.2):
            walec(s, DREWNO, (x, -0.55, 0.12), (x, 0.55, 0.12), 0.11, BALE * 0.85, boki=6)
    if gotowe:
        for x in (xc - pol * 0.62, xc + pol * 0.62):
            belka(s, DREWNO, (x, -0.7, 0.0), (x, 0.0, 0.95), 0.1, RUSZT)
            belka(s, DREWNO, (x, 0.7, 0.0), (x, 0.0, 0.95), 0.1, RUSZT)
        walec(s, DREWNO, (xc - pol * 0.92, 0, 1.08), (xc + pol * 0.92, 0, 1.08), 0.2, BALE, boki=8)
        pudlo(s, DREWNO, (xc + 0.3, -0.02, 0.6), (xc + 0.36, 0.02, 1.5), rgb(88, 88, 92))  # brzeszczot pily w klodzie
        belka(s, DREWNO, (xc, 0.0, 1.5), (xc + 0.7, 0.0, 1.5), 0.06, BELKA)
        pudlo(s, TYNK, (xc - 0.8, -0.4, 0.06), (xc + 0.9, 0.4, 0.09), TROCINY)


def sciana_z_desek(s, x0, x1, y, h, os_="x"):
    """Sciana z pionowych desek wzdluz X (na y) albo wzdluz Y (os_='y': na x=y, od x0 do x1 w Y)."""
    for k, a in enumerate(np.arange(x0, x1 - 0.05, 0.3)):
        b = min(a + 0.29, x1)
        kol = DESKA * (0.85 + 0.08 * (k % 3))
        if os_ == "x":
            pudlo(s, DREWNO, (a, y - 0.05, 0.0), (b, y + 0.05, h), kol)
        else:
            pudlo(s, DREWNO, (y - 0.05, a, 0.0), (y + 0.05, b, h), kol)


def pilarnia(faza, poziom=1):
    """Pilarnia: otwarta wiata na slupach z gontem, pod nia dol pilarski z kozlami i kloda, z tylu sciana z desek.
    Poziom 2 (pilarnia kryta): sciany z bokow, przybudowka z suszarnia desek, stojak z deskami.
    Poziom 3 (trzy rodziny): dluzsza i wyzsza wiata z dwoma dolami, scianami i wywietrznikiem na kalenicy."""
    s = Siatka()
    B = PILA3 if poziom == 3 else PILA
    gotowe = faza == 4
    # ziemia w grze (ABudowa::Podloze): klepisko(s, B.x0 - 0.5, B.x1 + 0.5, B.y0 - 0.5, B.y1 + 0.5)
    if poziom == 3:
        dol_pilarski(s, -2.6, 2.0, faza, gotowe)
        dol_pilarski(s, 2.6, 2.0, faza, gotowe)
    else:
        dol_pilarski(s, 0.0, 2.6, faza, gotowe)
    hs = B.h if faza >= 2 else 1.6
    for x in np.linspace(B.x0 + 0.1, B.x1 - 0.1, 4 if poziom == 3 else 3):
        for y in (B.y0 + 0.1, B.y1 - 0.1):
            belka(s, DREWNO, (x, y, -0.2), (x, y, hs), 0.2, BALE * 0.95)
    if faza >= 2:
        for y in (B.y0 + 0.1, B.y1 - 0.1):
            belka(s, DREWNO, (B.x0 - 0.1, y, B.h - 0.1), (B.x1 + 0.1, y, B.h - 0.1), 0.22, BALE * 0.85)
        hsc = 1.4 if faza == 2 else B.h - 0.3
        sciana_z_desek(s, B.x0 + 0.2, B.x1 - 0.2, B.y1 - 0.07, hsc)
        if poziom >= 2 and faza >= 3:  # boczne sciany do 2/3 glebokosci, przod otwarty
            for x in (B.x0 + 0.07, B.x1 - 0.07):
                sciana_z_desek(s, B.y0 + 1.4, B.y1 - 0.2, x, hsc, os_="y")
    if faza >= 3:
        wiezba(s, B, kolor=BALE * 1.05)
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.55)
    if gotowe:
        if poziom == 1:
            for k in range(4):
                belka(s, DREWNO, (-3.2 + k * 0.35, B.y1 - 0.5, 0.0), (-3.0 + k * 0.35, B.y1 - 0.2, 2.2), 0.07, DESKA * (0.95 + 0.05 * (k % 2)))
        if poziom == 2:
            # przybudowka z jednospadowym daszkiem (suszarnia desek) od +X
            x0, x1 = B.x1, B.x1 + 1.7
            for y in (B.y0 + 0.4, B.y1 - 0.4):
                belka(s, DREWNO, (x1 - 0.1, y, -0.2), (x1 - 0.1, y, 1.9), 0.14, RUSZT)
            plyta(s, DREWNO, [(x0 - 0.1, B.y0 + 0.1, B.h - 0.2), (x1 + 0.3, B.y0 + 0.1, 1.9), (x1 + 0.3, B.y1 - 0.1, 1.9), (x0 - 0.1, B.y1 - 0.1, B.h - 0.2)], 0.1, DESKA_SZCZYT)
            for j, z in enumerate((0.25, 0.65, 1.05)):  # deski w przekladkach
                for yy in np.arange(B.y0 + 0.7, B.y1 - 0.6, 0.55):
                    pudlo(s, DREWNO, (x0 + 0.2, yy, z), (x1 - 0.3, yy + 0.4, z + 0.06), DESKA * (0.92 + 0.05 * j))
                for xx in (x0 + 0.35, x1 - 0.45):
                    pudlo(s, DREWNO, (xx, B.y0 + 0.6, z - 0.06), (xx + 0.08, B.y1 - 0.6, z), RUSZT)
            # stojak z deskami pod sciana od -X
            for k in range(5):
                belka(s, DREWNO, (B.x0 + 0.5 + k * 0.32, B.y1 - 0.5, 0.0), (B.x0 + 0.7 + k * 0.32, B.y1 - 0.2, 2.2), 0.07, DESKA * (0.95 + 0.05 * (k % 2)))
            skrzynia(s, -3.3, B.y0 - 0.6, 0.6)
        if poziom == 3:
            # wywietrznik na kalenicy i deski pod tylna sciana
            g = B.h + B.k
            pudlo(s, DREWNO, (-0.7, -0.45, g - 0.2), (0.7, 0.45, g + 0.55), DESKA_SZCZYT)
            for st in (-1, 1):
                plyta(s, DREWNO, [(-0.9, 0.0, g + 0.85), (0.9, 0.0, g + 0.85), (0.9, st * 0.7, g + 0.4), (-0.9, st * 0.7, g + 0.4)], 0.06, GONT)
            for k in range(8):
                belka(s, DREWNO, (-4.6 + k * 0.32, B.y1 - 0.5, 0.0), (-4.4 + k * 0.32, B.y1 - 0.2, 2.4), 0.07, DESKA * (0.95 + 0.05 * (k % 2)))
            skrzynia(s, 4.3, B.y0 - 0.6, 0.6)
            skrzynia(s, 3.6, B.y0 - 0.7, 0.5)
    if faza <= 2:
        stos_belek(s, 0.0, B.y0 - 2.1, 5.0, 5 if faza == 1 else 3)
        kozly(s, -3.0, B.y0 - 2.3)
    if faza == 3:
        stos_desek(s, 0.0, B.y0 - 2.1, 3.0, 4)
        drabina(s, B.x1 - 1.0, B.y0, B.h + 0.6)
    return s


def przebudowa_prosta(stary, B, kamien=False):
    """Ulepszenie budynku bez wariantow: gotowy poprzedni poziom w rusztowaniu, przed nim material."""
    s = stary(4)
    rusztowanie(s, B.x0, B.x1, B.y0, B.y1, B.h - 0.6, boki=(True, False, True, True), odst=0.8)
    stos_belek(s, -1.5, B.y0 - 3.4, 5.0, 4)
    stos_desek(s, 2.2, B.y0 - 3.3, n=4)
    if kamien:
        stos_kamieni(s, 2.2, B.y0 - 4.4, np.random.default_rng(470), 2)
    return s


PLAC = (-5.0, 5.0, -4.0, 4.0)


def plac_skladowy(faza):
    """Plac skladowy: klepisko, plot z zerdzi z brama od frontu, z tylu wiata z desek (stosy towaru - w grze)."""
    s = Siatka()
    x0, x1, y0, y1 = PLAC
    # ziemia w grze (ABudowa::Podloze): klepisko(s, x0 - 0.2, x1 + 0.2, y0 - 0.2, y1 + 0.2)
    brama = (-1.6, 1.6)
    if faza == 1:
        plot(s, x0, x1, y0, y1, brama=brama, boki="pl", zerdzie=False)
        for x, y in ((x1, y1), (x1, y0)):
            belka(s, DREWNO, (x, y, -0.2), (x, y, 1.22), 0.12, RUSZT)
        stos_belek(s, -1.5, -2.5, 4.0, 5)
        stos_belek(s, 2.0, -2.6, 4.0, 3)
    elif faza == 2:
        plot(s, x0, x1, y0, y1, brama=brama, boki="pl")
        plot(s, x0, x1, y0, y1, boki="td", zerdzie=False)
        stos_belek(s, -1.5, -2.5, 4.0, 3)
    else:
        plot(s, x0, x1, y0, y1, brama=brama)
        # brama: wyzsze slupy z belka
        for x in brama:
            belka(s, DREWNO, (x, y0, -0.2), (x, y0, 2.4), 0.18, RUSZT)
        belka(s, DREWNO, (brama[0] - 0.2, y0, 2.35), (brama[1] + 0.2, y0, 2.35), 0.16, RUSZT * 0.9)
        # wiata z tylu po lewej
        wx0, wx1, wy0, wy1 = -4.6, -0.6, 1.9, 3.8
        for x in (wx0 + 0.1, (wx0 + wx1) / 2, wx1 - 0.1):
            belka(s, DREWNO, (x, wy0, -0.2), (x, wy0, 2.4), 0.16, BALE)
            belka(s, DREWNO, (x, wy1, -0.2), (x, wy1, 1.9), 0.16, BALE)
        belka(s, DREWNO, (wx0, wy0, 2.35), (wx1, wy0, 2.35), 0.16, BALE * 0.85)
        belka(s, DREWNO, (wx0, wy1, 1.85), (wx1, wy1, 1.85), 0.16, BALE * 0.85)
        if faza == 4:
            plyta(s, DREWNO, [(wx0 - 0.3, wy0 - 0.4, 2.55), (wx1 + 0.3, wy0 - 0.4, 2.55), (wx1 + 0.3, wy1 + 0.3, 1.95), (wx0 - 0.3, wy1 + 0.3, 1.95)], 0.1, DESKA_SZCZYT)
            beczka(s, -3.8, 3.1)
            beczka(s, -3.1, 3.2)
            skrzynia(s, -2.0, 3.0, 0.8)
            skrzynia(s, -1.3, 3.1, 0.6)
        else:
            stos_desek(s, -1.5, -2.5, 3.0, 4)
    return s


# --- stosy towaru (w grze przy budynkach, poziom 1..4 wedlug zapelnienia) ---

def stos_klod(poziom):
    """Klody 3 m w piramidzie, z klinami po bokach: 3 / 6 / 10 / 15 sztuk."""
    s = Siatka()
    n = {1: 3, 2: 6, 3: 10, 4: 15}[poziom]
    r = 0.17
    rzedy = [5, 4, 3, 2, 1]
    k = 0
    for j, w in enumerate(rzedy):
        for i in range(w):
            if k >= n:
                break
            y = (i - (w - 1) / 2) * 2 * r
            z = r + j * r * 1.7
            walec(s, DREWNO, (-1.5, y, z), (1.5, y, z), r, BALE * (0.82 + 0.06 * ((i * 3 + j) % 4)), boki=7)
            # jasne czola
            walec(s, DREWNO, (1.5, y, z), (1.52, y, z), r * 0.85, rgb(196, 164, 120), boki=7, denka=True)
            walec(s, DREWNO, (-1.52, y, z), (-1.5, y, z), r * 0.85, rgb(196, 164, 120), boki=7, denka=True)
            k += 1
    for x in (-1.0, 1.0):
        for y in (-1.0, 1.0):
            belka(s, DREWNO, (x, y, -0.1), (x, y * 0.95, 0.5), 0.08, RUSZT)
    return s


def stos_desek_towar(poziom):
    """Deski 2,8 m w warstwach z przekladkami: 1 / 2 / 4 / 6 warstw po 4 deski."""
    s = Siatka()
    warstw = {1: 1, 2: 2, 3: 4, 4: 6}[poziom]
    z = 0.0
    for wz in range(warstw):
        for x in (-1.0, 0.0, 1.0):  # przekladki
            pudlo(s, DREWNO, (x - 0.05, -0.7, z), (x + 0.05, 0.7, z + 0.06), RUSZT)
        z += 0.06
        for i in range(4):
            y = -0.6 + i * 0.31
            pudlo(s, DREWNO, (-1.4, y, z), (1.4, y + 0.28, z + 0.05), DESKA * (0.88 + 0.05 * ((i + wz) % 3)))
        z += 0.05
    return s


def stos_kamieni_towar(poziom):
    s = Siatka()
    stos_kamieni(s, 0.0, 0.0, np.random.default_rng(700 + poziom), {1: 2, 2: 3, 3: 4, 4: 5}[poziom])
    return s


# --- kamien i jedzenie (etap 2 ekonomii, 21.11.2026): kamieniolom, chata zbieracza, chata mysliwego, spichlerz ---
SKALA = rgb(124, 120, 112)   # naturalna skala (przygaszona szarosc z cieplym odcieniem)
SKALA_C = rgb(92, 88, 82)
RUMOSZ = rgb(150, 144, 132)  # okruchy kamienia na ziemi
WIKLINA = rgb(168, 134, 84)  # kosze
JAGODY = rgb(112, 40, 48)
GRZYBY = rgb(186, 150, 110)
ZIOLA = rgb(96, 112, 62)
SKORA_Z = rgb(122, 86, 56)   # skora zwierzeca
MIESO = rgb(132, 62, 52)
WOREK = rgb(176, 158, 120)
POROZE = rgb(214, 198, 168)


def skala(s, x0, x1, y0, y1, h, rng, schody=3, kolor=None):
    """Wychodnia skalna: bloki w schodkach opadajacych ku przodowi (-Y); przod sciety plasko (lico, ktore kuja)."""
    SK = SKALA if kolor is None else kolor
    SKC = SKALA_C if kolor is None else kolor * 0.75
    gl = (y1 - y0) / schody
    for k in range(schody):
        ya, yb = y0 + k * gl, y0 + (k + 1) * gl
        hk = h * (0.55 + 0.45 * (k + 1) / schody)
        x = x0
        while x < x1 - 0.2:
            w = min(rng.uniform(0.9, 1.7), x1 - x)
            hh = hk * rng.uniform(0.8, 1.05)
            pudlo(s, TYNK, (x, ya + rng.uniform(-0.1, 0.1), -0.5), (x + w, yb, hh), SK * rng.uniform(0.9, 1.06))
            # pekniecia: ciemniejsza listwa na licu
            if rng.uniform() < 0.5:
                pudlo(s, TYNK, (x + w * 0.4, ya - 0.02, hh * 0.3), (x + w * 0.45, ya + 0.02, hh * 0.85), SKC)
            x += w
    # luzne glazy na grzbiecie (lamia proste linie blokow)
    for k in range(int((x1 - x0) * 0.9)):
        x = rng.uniform(x0 + 0.4, x1 - 0.4)
        y = rng.uniform(y0 + gl, y1 - 0.4)
        r = rng.uniform(0.35, 0.7)
        z = h * rng.uniform(0.75, 1.0)
        walec(s, TYNK, (x - r * 0.3, y, z - r * 0.5), (x + r * 0.3, y + 0.1, z + r * 0.6), r, SK * rng.uniform(0.85, 1.0), boki=5)


def kosz(s, x, y, r=0.32, h=0.42, zaw=JAGODY, z=0.0):
    walec(s, DREWNO, (x, y, z), (x, y, z + h), r, WIKLINA * 0.95, boki=8, denka=False)
    walec(s, DREWNO, (x, y, z + h - 0.04), (x, y, z + h + 0.02), r * 1.04, WIKLINA * 0.75, boki=8, denka=False)
    walec(s, TYNK, (x, y, z), (x, y, z + h - 0.06), r * 0.92, zaw, boki=8, denka=True)


def worek(s, x, y, z=0.0, lezy=False, kolor=WOREK):
    if lezy:
        walec(s, TYNK, (x - 0.38, y, z + 0.2), (x + 0.38, y, z + 0.2), 0.22, kolor, boki=7)
    else:
        walec(s, TYNK, (x, y, z), (x, y, z + 0.5), 0.25, kolor, boki=7)
        walec(s, TYNK, (x, y, z + 0.5), (x, y, z + 0.62), 0.1, kolor * 0.85, boki=6)


def suszarnia(s, x0, x1, y, h=1.6, wisi=ZIOLA, rng=None):
    """Dwa slupki z zerdzia, na niej peczki (ziola, grzyby, mieso)."""
    for x in (x0, x1):
        belka(s, DREWNO, (x, y, -0.2), (x, y, h), 0.1, RUSZT)
    belka(s, DREWNO, (x0, y, h - 0.05), (x1, y, h - 0.05), 0.06, BELKA * 0.85)
    n = int((x1 - x0) / 0.32)
    for k in range(1, n):
        xx = x0 + k * (x1 - x0) / n
        dl = 0.35 + 0.15 * ((k * 7) % 3) / 2
        pudlo(s, TYNK, (xx - 0.07, y - 0.07, h - 0.1 - dl), (xx + 0.07, y + 0.07, h - 0.1), wisi * (0.88 + 0.08 * (k % 3)))


def rama_skory(s, x, y, w=1.3, h=1.5, obrot="y"):
    """Rama z zerdzi z rozpieta skora (lekko pochylona do tylu)."""
    if obrot == "y":
        a, b = np.array([x - w / 2, y, 0.0]), np.array([x + w / 2, y, 0.0])
    else:
        a, b = np.array([x, y - w / 2, 0.0]), np.array([x, y + w / 2, 0.0])
    tyl = np.array([0, 0.35, 0]) if obrot == "y" else np.array([0.35, 0, 0])
    g = np.array([0, 0, h])
    for p in (a, b):
        belka(s, DREWNO, tuple(p - [0, 0, 0.2]), tuple(p + g + tyl), 0.07, RUSZT)
    belka(s, DREWNO, tuple(a + g + tyl), tuple(b + g + tyl), 0.06, RUSZT)
    belka(s, DREWNO, tuple(a + [0, 0, 0.25]), tuple(b + [0, 0, 0.25]), 0.06, RUSZT)
    m = 0.12
    d = (b - a) / np.linalg.norm(b - a)
    rogi = [a + d * m + [0, 0, 0.35] + tyl * 0.2, b - d * m + [0, 0, 0.35] + tyl * 0.2, b - d * m + g * 0.92 + tyl * 0.95, a + d * m + g * 0.92 + tyl * 0.95]
    plyta(s, TYNK, rogi, 0.03, SKORA_Z)


def poroze(s, x, y, z):
    """Poroze jelenia nad drzwiami (na scianie -Y)."""
    pudlo(s, DREWNO, (x - 0.18, y - 0.05, z - 0.1), (x + 0.18, y + 0.02, z + 0.12), DESKA_SZCZYT)
    for st in (-1, 1):
        belka(s, DREWNO, (x + st * 0.08, y - 0.1, z), (x + st * 0.55, y - 0.15, z + 0.55), 0.045, POROZE)
        belka(s, DREWNO, (x + st * 0.3, y - 0.12, z + 0.3), (x + st * 0.2, y - 0.15, z + 0.7), 0.035, POROZE)
        belka(s, DREWNO, (x + st * 0.45, y - 0.14, z + 0.45), (x + st * 0.6, y - 0.16, z + 0.85), 0.035, POROZE)


def taczka(s, x, y):
    pudlo(s, DREWNO, (x - 0.5, y - 0.3, 0.35), (x + 0.35, y + 0.3, 0.65), DESKA * 0.85)
    walec(s, DREWNO, (x + 0.55, y - 0.06, 0.2), (x + 0.55, y + 0.06, 0.2), 0.2, RUSZT, boki=8)
    for st in (-1, 1):
        belka(s, DREWNO, (x - 1.2, y + st * 0.25, 0.5), (x + 0.55, y + st * 0.08, 0.25), 0.05, RUSZT)
    stos_kamieni(s, x - 0.1, y, np.random.default_rng(1), 1)


def zuraw(s, x, y, h, wysiegnik=True):
    """Prosty zuraw z bali (trojnog i wysiegnik z lina) nad skala."""
    for dx, dy in ((-0.9, -0.6), (0.9, -0.6), (0.0, 0.9)):
        belka(s, DREWNO, (x + dx, y + dy, -0.2), (x, y, h), 0.14, BALE * 0.9)
    if wysiegnik:
        belka(s, DREWNO, (x, y, h - 0.4), (x, y + 2.2, h + 0.4), 0.1, BALE)
        belka(s, DREWNO, (x, y + 2.2, h + 0.35), (x, y + 2.2, 1.0), 0.025, rgb(70, 60, 48))
        pudlo(s, KAMIEN, (x - 0.35, y + 1.95, 0.55), (x + 0.35, y + 2.45, 1.0), MUR * 0.95, bez_dolu=False)


def kamieniolom(faza):
    """Kamieniolom: skala w schodkach z tylu (+Y), przed nia klepisko z okruchami; z boku zuraw, wiata na narzedzia.
    Kamieniarze kuja lico skaly (gra: Stanowisko), kamienie leza na stosie (w grze)."""
    s = Siatka()
    rng = np.random.default_rng(800)
    # ziemia w grze (ABudowa::Podloze): klepisko(s, -5.2, 5.2, -4.2, 1.2, ZIEMIA * 0.75 + RUMOSZ * 0.25)
    rng_s = np.random.default_rng(801)
    # Lico skaly cofa sie z postepem robot (F1 tylko oczyszczona skala).
    skala(s, -4.6, 4.6, 0.6 if faza == 1 else 1.0, 4.4, 3.4, rng_s)
    for k in range(10):
        x, y = rng.uniform(-4.5, 4.5), rng.uniform(-3.8, 0.4)
        pudlo(s, KAMIEN, (x - 0.12, y - 0.1, 0.0), (x + 0.12, y + 0.1, 0.12), RUMOSZ * rng.uniform(0.85, 1.05), bez_dolu=False)
    if faza == 1:
        for x in (-3.6, 3.6):
            belka(s, DREWNO, (x, -3.8, -0.2), (x, -3.8, 0.9), 0.08, RUSZT)
        stos_belek(s, -2.0, -3.0, 4.0, 5)
        kozly(s, 2.4, -3.0)
    if faza >= 2:
        zuraw(s, -3.3, -0.6, 4.2, wysiegnik=faza >= 3)
    # wiata na narzedzia po prawej (+X)
    wx0, wx1, wy0, wy1 = 2.4, 4.8, -3.9, -1.9
    if faza >= 2:
        for x in (wx0, wx1):
            belka(s, DREWNO, (x, wy0, -0.2), (x, wy0, 2.2), 0.14, BALE)
            belka(s, DREWNO, (x, wy1, -0.2), (x, wy1, 2.6), 0.14, BALE)
        belka(s, DREWNO, (wx0 - 0.2, wy0, 2.15), (wx1 + 0.2, wy0, 2.15), 0.14, BALE * 0.85)
        belka(s, DREWNO, (wx0 - 0.2, wy1, 2.55), (wx1 + 0.2, wy1, 2.55), 0.14, BALE * 0.85)
    if faza == 2:
        stos_desek(s, -1.0, -3.0, 2.6, 3)
    if faza >= 3:
        daszek = [(wx0 - 0.35, wy0 - 0.35, 2.3), (wx1 + 0.35, wy0 - 0.35, 2.3), (wx1 + 0.35, wy1 + 0.35, 2.75), (wx0 - 0.35, wy1 + 0.35, 2.75)]
        if faza == 3:
            daszek = [daszek[0], ((wx0 + wx1) / 2, wy0 - 0.35, 2.3), ((wx0 + wx1) / 2, wy1 + 0.35, 2.75), daszek[3]]
        plyta(s, DREWNO, daszek, 0.1, DESKA_SZCZYT)
    if faza == 4:
        skrzynia(s, wx0 + 0.6, wy1 - 0.5, 0.6)
        beczka(s, wx1 - 0.5, wy1 - 0.5)
        taczka(s, -1.2, -2.6)
        # obrobione bloki przy skale i kliny
        for k, x in enumerate((-1.8, -0.6, 0.6)):
            pudlo(s, KAMIEN, (x - 0.35, 0.1, 0.0), (x + 0.35, 0.55, 0.4), MUR * (0.92 + 0.04 * k), bez_dolu=False)
    return s


ZBIERACZ = Bryla(-3.2, 1.6, -2.0, 2.0, 2.4, 2.5, t=0.28)


def chata_zbieracza(faza):
    """Chata zbieracza: mala chata z bali pod strzecha, przed nia kosze z jagodami i grzybami, z boku suszarnia ziol."""
    s = Siatka()
    rng = np.random.default_rng(900)
    B = ZBIERACZ
    z0 = 0.3
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    h = {1: 1.2, 2: B.h, 3: B.h, 4: B.h}[faza]
    zrab(s, B.x0, B.x1, B.y0, B.y1, h, z0, rng)
    przod = (B.x0, B.y0 - 0.02, 0)
    if faza >= 2:
        gotowe = faza == 4
        drzwi_dom(s, przod, "y", -1, 1.9, z0, 1.1, 2.1, DRZWI if gotowe else rgb(30, 26, 24))
        okienko(s, przod, "y", -1, 3.8, z0 + 1.0, 0.65, 0.6, okiennice=gotowe)
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
    if faza == 3:
        dach_slomiany(s, B, 0, 0.5)
    if faza == 4:
        dach_slomiany(s, B)
        suszarnia(s, B.x1 + 0.7, B.x1 + 2.9, -0.4, 1.7, ZIOLA)
        suszarnia(s, B.x1 + 0.7, B.x1 + 2.9, 0.9, 1.7, GRZYBY)
        kosz(s, -2.6, B.y0 - 0.8, zaw=JAGODY)
        kosz(s, -1.9, B.y0 - 1.0, 0.28, 0.36, zaw=GRZYBY)
        kosz(s, -2.3, B.y0 - 1.55, 0.26, 0.34, zaw=JAGODY * 1.1)
        belka(s, DREWNO, (B.x1 + 0.2, B.y0 - 0.1, 0.0), (B.x1 + 0.35, B.y0 - 0.05, 1.4), 0.04, BELKA)  # kij
    if faza <= 2:
        stos_belek(s, -1.2, B.y0 - 3.0, 4.0, 5 if faza == 1 else 3)
    if faza == 2:
        drabina(s, B.x0 + 0.8, B.y0, B.h + 0.6)
    if faza == 3:
        snopki(s, -1.8, B.y0 - 3.0, 3)
        drabina(s, B.x1 - 0.9, B.y0, B.h + 0.8)
    return s


MYSLIWY = Bryla(-3.4, 1.8, -2.1, 2.1, 2.5, 2.6, t=0.3)


def chata_mysliwego(faza):
    """Chata mysliwego: zrab, gont, poroze nad drzwiami, z boku rama z rozpieta skora i suszarnia miesa."""
    s = Siatka()
    rng = np.random.default_rng(1000)
    B = MYSLIWY
    z0 = 0.3
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    h = {1: 1.3, 2: B.h, 3: B.h, 4: B.h}[faza]
    zrab(s, B.x0, B.x1, B.y0, B.y1, h, z0, rng)
    przod = (B.x0, B.y0 - 0.02, 0)
    if faza >= 2:
        gotowe = faza == 4
        drzwi_dom(s, przod, "y", -1, 2.0, z0, 1.1, 2.1, DRZWI if gotowe else rgb(30, 26, 24))
        okienko(s, przod, "y", -1, 4.1, z0 + 1.0, 0.65, 0.6, okiennice=gotowe)
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.5)
    if faza == 4:
        poroze(s, B.x0 + 2.55, B.y0 - 0.05, z0 + 2.15)
        rama_skory(s, B.x1 + 1.5, -1.2, 1.3, 1.5, obrot="x")
        rama_skory(s, B.x1 + 1.5, 0.6, 1.1, 1.3, obrot="x")
        suszarnia(s, -2.9, -1.3, B.y0 - 1.0, 1.6, MIESO)
        pieniek(s, -0.3, B.y0 - 1.1, siekiera=False)
        # luk i kolczan oparte o sciane
        belka(s, DREWNO, (-0.9, B.y0 - 0.12, 0.0), (-0.8, B.y0 - 0.05, 1.5), 0.035, RUSZT)
        walec(s, DREWNO, (-0.55, B.y0 - 0.15, 0.3), (-0.5, B.y0 - 0.1, 1.0), 0.09, SKORA_Z, boki=6)
    if faza <= 2:
        stos_belek(s, -1.2, B.y0 - 3.0, 4.0, 5 if faza == 1 else 3)
        kozly(s, 1.3, B.y0 - 2.9)
    if faza == 2:
        drabina(s, B.x0 + 0.8, B.y0, B.h + 0.6)
    if faza == 3:
        stos_desek(s, -1.2, B.y0 - 3.0, 2.6, 4)
        drabina(s, B.x1 - 0.9, B.y0, B.h + 0.8)
    return s


SPICH = Bryla(-3.6, 3.6, -2.4, 2.4, 3.4, 3.0, t=0.3)  # h liczone od gruntu (sciany od SPICH_Z)
SPICH_Z = 0.9  # podloga uniesiona na slupkach (myszy, wilgoc)


def spichlerz(faza):
    """Spichlerz: zrab z bali uniesiony na kamiennych slupkach, schodki do drzwi, dach z gontu z daszkiem nad drzwiami."""
    s = Siatka()
    rng = np.random.default_rng(1100)
    B = SPICH
    z0 = SPICH_Z
    for x in (B.x0 + 0.3, 0.0, B.x1 - 0.3):
        for y in (B.y0 + 0.3, B.y1 - 0.3):
            pudlo(s, KAMIEN, (x - 0.3, y - 0.3, -0.5), (x + 0.3, y + 0.3, z0 - 0.15), MUR_COKOL, bez_dolu=False)
            pudlo(s, KAMIEN, (x - 0.4, y - 0.4, z0 - 0.18), (x + 0.4, y + 0.4, z0 - 0.06), MUR_COKOL * 1.05, bez_dolu=False)
    if faza >= 2:
        pudlo(s, DREWNO, (B.x0, B.y0, z0 - 0.06), (B.x1, B.y1, z0 + 0.05), BALE * 0.8, bez_dolu=False)
    h = {1: 0.0, 2: 2.1, 3: B.h, 4: B.h}[faza]
    if faza == 1:
        belka(s, DREWNO, (B.x0, B.y0 + 0.3, z0 - 0.02), (B.x1, B.y0 + 0.3, z0 - 0.02), 0.2, BALE)
        belka(s, DREWNO, (B.x0, B.y1 - 0.3, z0 - 0.02), (B.x1, B.y1 - 0.3, z0 - 0.02), 0.2, BALE)
    else:
        zrab(s, B.x0, B.x1, B.y0, B.y1, h, z0, rng)
    przod = (B.x0, B.y0 - 0.02, 0)
    if faza >= 3:
        gotowe = faza == 4
        drzwi_dom(s, przod, "y", -1, 3.6, z0, 1.3, 2.2, DRZWI if gotowe else rgb(30, 26, 24))
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
    if faza >= 3:
        wiezba(s, B, kolor=BALE)
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.55)
    if faza == 4:
        # schodki z desek do drzwi
        for k in range(3):
            zz = z0 * (k + 1) / 4
            pudlo(s, DREWNO, (-0.7, B.y0 - 0.35 - (2 - k) * 0.32, zz - 0.06), (0.7, B.y0 - 0.05 - (2 - k) * 0.32, zz), DESKA)
        for x in (-0.75, 0.75):
            belka(s, DREWNO, (x, B.y0 - 1.25, 0.0), (x, B.y0 - 0.05, z0), 0.08, RUSZT)
        worek(s, 1.8, B.y0 - 0.6)
        worek(s, 2.35, B.y0 - 0.75)
        worek(s, 2.05, B.y0 - 0.65, z=0.42, lezy=True)  # po lewej (-X) lezy stos jedzenia z gry
    if faza <= 2:
        stos_belek(s, -1.5, B.y0 - 3.0, 4.5, 5 if faza == 1 else 3)
        kozly(s, 2.2, B.y0 - 3.0)
    if faza == 3:
        stos_desek(s, -1.5, B.y0 - 3.0, 3.0, 4)
        drabina(s, B.x1 - 1.0, B.y0, B.h + 0.6)
    return s


def stos_jedzenia(poziom):
    """Jedzenie przy budynku: worki, kosze z jagodami i grzybami, przy pelnym beczka: 2 / 4 / 6 / 9 sztuk."""
    s = Siatka()
    rng = np.random.default_rng(1200 + poziom)
    miejsca = [(-0.5, 0.0, "w"), (0.1, 0.1, "k"), (-0.2, -0.55, "w"), (0.6, -0.4, "k2"), (-0.9, -0.5, "w"),
               (0.7, 0.35, "w"), (-0.35, 0.05, "wl"), (1.1, -0.1, "b"), (-1.0, 0.25, "k")]
    n = {1: 2, 2: 4, 3: 6, 4: 9}[poziom]
    for x, y, t in miejsca[:n]:
        if t == "w":
            worek(s, x, y, kolor=WOREK * rng.uniform(0.9, 1.05))
        elif t == "wl":
            worek(s, x, y, z=0.5, lezy=True, kolor=WOREK * 0.95)
        elif t == "k":
            kosz(s, x, y, 0.28, 0.36, zaw=JAGODY)
        elif t == "k2":
            kosz(s, x, y, 0.26, 0.34, zaw=GRZYBY)
        else:
            beczka(s, x, y)
    return s


# --- zboze i chleb (etap 3 ekonomii, 21.11.2026): pole, mlyn (wiatrak kozlak), piekarnia, tartak wodny ---
ROLA = rgb(98, 76, 56)        # swiezo zaorana ziemia
ROLA_J = rgb(122, 98, 72)
UGOR = rgb(116, 108, 74)      # ugor: ziemia z trawa
ZIEL_ZBOZE = rgb(104, 128, 66)
ZLOTE = rgb(196, 166, 98)     # dojrzale zboze (przygaszone, nie jaskrawe)
ZLOTE_C = rgb(168, 138, 80)
SCIERN = rgb(176, 160, 116)
GLINA = rgb(170, 120, 86)     # piec chlebowy
GLINA_C = rgb(130, 92, 66)
MAKA_W = rgb(214, 206, 186)   # worki z maka
SKRZYDLO = rgb(196, 184, 160) # plotno skrzydel wiatraka

POLE = (-10.0, 10.0, -7.0, 7.0)   # ogrodzenie
POLE_UPR = (-9.4, 9.4, -4.4, 6.4) # uprawa (z przodu, -Y, miedza z miejscem na snopy)


def _plot_pola(s, pelny=True):
    x0, x1, y0, y1 = POLE
    if pelny:
        plot(s, x0, x1, y0, y1, h=0.9, brama=(-1.4, 1.4), krok=2.0)
    else:
        for x, y in ((x0, y0), (x1, y0), (x0, y1), (x1, y1), (-1.4, y0), (1.4, y0)):
            belka(s, DREWNO, (x, y, -0.2), (x, y, 1.0), 0.12, RUSZT)


def _grunt_pola(s, kolor):
    x0, x1, y0, y1 = POLE
    klepisko(s, x0 - 0.1, x1 + 0.1, y0 - 0.1, y1 + 0.1, UGOR * 0.95)
    a0, a1, b0, b1 = POLE_UPR
    pudlo(s, TYNK, (a0, b0, 0.06), (a1, b1, 0.08), kolor)


def _rzedy(s, rng, wys, kolor, kolor2=None, krok=0.55, segment=1.1, szer=0.3, kropki=False, obszar=None):
    """Rzedy uprawy wzdluz X: odcinki o lekko losowej wysokosci (kepy zboza albo skiby). obszar: (x0, x1, y0, y1)."""
    a0, a1, b0, b1 = obszar or POLE_UPR
    y = b0 + krok / 2
    j = 0
    while y < b1 - krok / 4:
        x = a0
        while x < a1 - 0.05:
            dl = min(segment * rng.uniform(0.8, 1.2), a1 - x)
            h = wys * rng.uniform(0.82, 1.1)
            k = (kolor if (kolor2 is None or rng.uniform() < 0.7) else kolor2) * rng.uniform(0.93, 1.06)
            if kropki:
                pudlo(s, TYNK, (x + 0.1, y - szer / 2, 0.08), (x + dl - 0.1, y + szer / 2, 0.08 + h), k)
            else:
                pudlo(s, TYNK, (x, y - szer / 2, 0.08), (x + dl, y + szer / 2, 0.08 + h), k)
            x += dl
        y += krok
        j += 1


def pole(faza):
    """Pole budowane: F1 paliki w rogach i bale na plot, F2 ogrodzony ugor (gotowe pole bez upraw)."""
    s = Siatka()
    if faza == 1:
        x0, x1, y0, y1 = POLE
        klepisko(s, x0, x1, y0, y1, UGOR)
        _plot_pola(s, pelny=False)
        stos_belek(s, -4.0, -8.2, 4.0, 5)
        stos_belek(s, 4.0, -8.2, 4.0, 3)
    else:
        _grunt_pola(s, UGOR * 1.04)
        rng = np.random.default_rng(1300)
        for k in range(26):  # kepy chwastow na ugorze
            a0, a1, b0, b1 = POLE_UPR
            x, y = rng.uniform(a0, a1), rng.uniform(b0, b1)
            pudlo(s, TYNK, (x - 0.25, y - 0.2, 0.08), (x + 0.25, y + 0.2, 0.2), ZIEL_ZBOZE * rng.uniform(0.75, 0.95))
        _plot_pola(s)
    return s


def uprawa(rodzaj):
    """Wyglad pola wedlug uprawy (gra podmienia model gotowego pola): ogrodzenie + grunt + rzedy."""
    s = Siatka()
    rng = np.random.default_rng(1310 + rodzaj)
    if rodzaj == 0:    # Zaorane: ciemne skiby
        _grunt_pola(s, ROLA * 0.85)
        _rzedy(s, rng, 0.1, ROLA, ROLA_J * 0.9, krok=0.5, segment=2.5, szer=0.26)
    elif rodzaj == 1:  # Zasiane: wyrownane skiby z pierwsza zielenia
        _grunt_pola(s, ROLA * 0.9)
        _rzedy(s, rng, 0.06, ROLA_J * 0.92, ROLA, krok=0.5, segment=3.0, szer=0.3)
        _rzedy(s, rng, 0.08, ZIEL_ZBOZE * 0.85, None, krok=1.0, segment=0.5, szer=0.12, kropki=True)
    elif rodzaj == 2:  # Rosnie: zielone rzedy
        _grunt_pola(s, ROLA * 0.9)
        _rzedy(s, rng, 0.38, ZIEL_ZBOZE, ZIEL_ZBOZE * 0.88, krok=0.55, segment=1.1, szer=0.42)
    elif rodzaj == 3:  # Dojrzale: zloty, wysoki lan
        _grunt_pola(s, ZLOTE_C * 0.8)
        _rzedy(s, rng, 0.8, ZLOTE, ZLOTE_C, krok=0.5, segment=1.0, szer=0.46)
    else:              # Scierniska: krotkie zeschle zdzbla
        _grunt_pola(s, SCIERN * 0.85)
        _rzedy(s, rng, 0.08, SCIERN, SCIERN * 0.88, krok=0.5, segment=1.8, szer=0.3)
    _plot_pola(s)
    return s


# Pole zaznaczane na terenie (21.11.2026): gra kladzie uprawe z kafli 5 x 5 m (kazdy na wysokosci terenu, skalowany
# do wielkosci pola) i stawia plot z segmentow 2 m wzdluz obwodu (ABudowa: InstancedStaticMesh).
KAFEL_POLA = (-2.5, 2.5, -2.5, 2.5)
PLOT_H = 0.95


def zboze(rodzaj):
    """Zboze w 3D (22.11.2026): kepa zdzbel na 1 m rzedu (X od -0,5 do 0,5, Y +-0,15), wysokosc ok. 1 m.
    Gra (ABudowa::ZbudujLan) stawia je na grzbietach rzedow i skaluje Z wedlug wzrostu. 0 zielone, 1 dojrzale z klosami, 2 scierniska."""
    s = Siatka()
    rng = np.random.default_rng(1500 + rodzaj)
    n = 18 if rodzaj < 2 else 22
    for k in range(n):
        x = -0.5 + (k + rng.uniform(0.1, 0.9)) / n
        y = rng.uniform(-0.15, 0.15)
        if rodzaj == 2:
            h = rng.uniform(0.12, 0.2)
            belka(s, TYNK, (x, y, -0.05), (x + rng.uniform(-0.03, 0.03), y + rng.uniform(-0.03, 0.03), h), 0.035, SCIERN * rng.uniform(0.82, 1.05))
            continue
        h = rng.uniform(0.8, 1.0)
        dx, dy = rng.normal(0.0, 0.06, 2)
        top = np.array([x + dx, y + dy, h])
        if rodzaj == 0:
            kol = (ZIEL_ZBOZE if rng.uniform() < 0.7 else ZIEL_ZBOZE * 0.85) * rng.uniform(0.88, 1.08)
            belka(s, TYNK, (x, y, -0.1), top, 0.035, kol)
            for z, kat in ((0.35, rng.uniform(0, 2 * np.pi)), (0.6, rng.uniform(0, 2 * np.pi))):  # liscie odchylone na boki
                a = np.array([x + dx * z, y + dy * z, h * z])
                b = a + np.array([np.cos(kat) * 0.2, np.sin(kat) * 0.2, 0.12])
                belka(s, TYNK, a, b, 0.03, kol * 1.05)
        else:
            kol = (ZLOTE if rng.uniform() < 0.7 else ZLOTE_C) * rng.uniform(0.9, 1.06)
            belka(s, TYNK, (x, y, -0.1), top, 0.035, kol * 1.05)
            kat = rng.uniform(0, 2 * np.pi)  # klos: grubszy, lekko przechylony
            belka(s, TYNK, top, top + np.array([np.cos(kat) * 0.06, np.sin(kat) * 0.06, 0.12]), 0.09, kol * 1.15)
    return s


def kafel_pola(rodzaj):
    """Kafel uprawy 5 x 5 m: rodzaj -1 = ugor (gotowe pole bez upraw), 0..4 jak uprawa(). Rzedy ciagna sie przez sasiednie kafle."""
    s = Siatka()
    rng = np.random.default_rng(1400 + rodzaj)
    x0, x1, y0, y1 = KAFEL_POLA
    kol = {-1: UGOR * 1.04, 0: ROLA * 0.85, 1: ROLA * 0.9, 2: ROLA * 0.9, 3: ZLOTE_C * 0.8}.get(rodzaj, SCIERN * 0.85)
    pudlo(s, TYNK, (x0, y0, -0.8), (x1, y1, 0.08), kol)
    if rodzaj == -1:
        for k in range(8):  # kepy chwastow na ugorze
            x, y = rng.uniform(x0 + 0.3, x1 - 0.3), rng.uniform(y0 + 0.3, y1 - 0.3)
            pudlo(s, TYNK, (x - 0.25, y - 0.2, 0.08), (x + 0.25, y + 0.2, 0.2), ZIEL_ZBOZE * rng.uniform(0.75, 0.95))
    elif rodzaj == 0:
        _rzedy(s, rng, 0.1, ROLA, ROLA_J * 0.9, krok=0.5, segment=2.5, szer=0.26, obszar=KAFEL_POLA)
    elif rodzaj == 1:
        _rzedy(s, rng, 0.06, ROLA_J * 0.92, ROLA, krok=0.5, segment=2.5, szer=0.3, obszar=KAFEL_POLA)
        _rzedy(s, rng, 0.08, ZIEL_ZBOZE * 0.85, None, krok=1.0, segment=0.5, szer=0.12, kropki=True, obszar=KAFEL_POLA)
    elif rodzaj == 2:
        _rzedy(s, rng, 0.38, ZIEL_ZBOZE, ZIEL_ZBOZE * 0.88, krok=0.5, segment=1.1, szer=0.4, obszar=KAFEL_POLA)
    elif rodzaj == 3:
        _rzedy(s, rng, 0.8, ZLOTE, ZLOTE_C, krok=0.5, segment=1.0, szer=0.46, obszar=KAFEL_POLA)
    else:
        _rzedy(s, rng, 0.08, SCIERN, SCIERN * 0.88, krok=0.5, segment=1.8, szer=0.3, obszar=KAFEL_POLA)
    return s


def _slupek(s, x, kolor=None):
    belka(s, DREWNO, (x, 0.0, -0.3), (x, 0.0, PLOT_H + 0.12), 0.12, RUSZT if kolor is None else kolor)
    pudlo(s, DREWNO, (x - 0.08, -0.08, PLOT_H + 0.08), (x + 0.08, 0.08, PLOT_H + 0.16), RUSZT * 0.85)


def plot_segment():
    """Segment plotu 2 m wzdluz +X: slupek na poczatku (x = 0) i dwie zerdzie do nastepnego slupka; gra skaluje X do odcinka."""
    s = Siatka()
    _slupek(s, 0.0)
    for z in (PLOT_H * 0.42, PLOT_H * 0.9):
        belka(s, DREWNO, (0.0, 0.0, z), (2.0, 0.0, z), 0.07, BELKA * 0.8)
        pudlo(s, DREWNO, (-0.09, -0.09, z - 0.06), (0.09, 0.09, z + 0.06), BELKA * 0.6)  # wiazanie
    return s


def plot_palik():
    """Sam slupek (budowany plot, koniec odcinka przy bramie i w rogach)."""
    s = Siatka()
    _slupek(s, 0.0)
    return s



# Mury osad rodow, wersja 2 (23.11.2026, uwaga uzytkownika: "mury, palisady, wieze slabo wygladaja, przy kosciele sie bardziej
# starales"). Odcinki 4 m wzdluz +X od x = 0 (gra skaluje X do boku obrysu osady), zewnetrzna strona -Y (po imporcie +Y);
# wieze i bramy na srodku ukladu, przejazd bramy wzdluz osi Y.
# Wymiary pod ludzika: w grze ma ok. 2,4 m (1,7 m x ALudzik::Skala 1,4).
#  Palisada: wal ziemny 0,9 m, pale 3,5-4 m nad walem z jasnymi zaostrzonymi czubkami, ganek obroncow 2 m nad walem
#   z porecza, od zewnatrz ostrokol na stoku walu. Odcinek ze schodami (_Schody) zamiast drabin.
#  Baszta drewniana: dolna kondygnacja z bali na zrab, wysunieta izba strzelnicza z desek na wspornikach, dach z gontu.
#  Brama drewniana: wieza bramna z przejazdem 3,8 x 4,3 m, otwarte wrota, izba i dach czterospadowy.
#  Mur kamienny: cokol ze skarpa, warstwy kamienia, gzyms, przedpiersie i blanki ze strzelnicami, chodnik 1,6 m.
#  Baszta kamienna: okragla, skarpa u dolu, wsporniki pod wysunietym zwienczeniem, dach stozkowy z dachowki.
#  Brama kamienna: wieza z przejazdem lukowym, brona, otwarte wrota, wsporniki, blanki i dach czterospadowy.
LUDZIK = 2.4
WAL_H = 0.9                    # wysokosc walu ziemnego pod palisada
WAL = rgb(116, 100, 74)         # ziemia na stokach walu
WAL_TRAWA = rgb(92, 108, 60)    # darn na koronie walu
DACHOWKA = rgb(132, 86, 68)     # dachowka baszt kamiennych (przygaszona czerwien)
ZELAZO = rgb(58, 52, 48)        # okucia i brona
CZUB = rgb(196, 168, 126)       # swiezo ociosane czubki pali


def _tri(s, mat, a, b, c, kolor, n):
    """Trojkat z zadana normalna (zdegenerowane pomija)."""
    a, b, c = (np.asarray(p, float) for p in (a, b, c))
    if np.linalg.norm(np.cross(b - a, c - a)) < 1e-8:
        return
    wielokat(s, mat, [a, b, c], kolor, n=np.asarray(n, float))


def _beben(s, mat, z0, z1, r0, r1, kolor, boki=16, cx=0.0, cy=0.0):
    """Pionowy walec albo sciety stozek (sam bok, gladkie normalne), kolejnosc scian jak w walec()."""
    kat = np.linspace(0, 2 * np.pi, boki + 1)
    nz = (r0 - r1) / max(z1 - z0, 1e-6)
    for k in range(boki):
        t0, t1 = kat[k], kat[k + 1]
        d0 = np.array([np.sin(t0), np.cos(t0)]); d1 = np.array([np.sin(t1), np.cos(t1)])
        p = [(cx + d0[0] * r0, cy + d0[1] * r0, z0), (cx + d1[0] * r0, cy + d1[1] * r0, z0),
             (cx + d1[0] * r1, cy + d1[1] * r1, z1), (cx + d0[0] * r1, cy + d0[1] * r1, z1)]
        n0 = np.array([d0[0], d0[1], nz]); n1 = np.array([d1[0], d1[1], nz])
        p = np.asarray(p, float)
        s.dodaj(mat, p, [n0, n1, n1, n0], kolor, [[0, 2, 1], [0, 3, 2]], _uv_scian(p, (n0 + n1) / 2, mat))


def _pierscien(s, mat, z, r0, r1, kolor, boki=16, w_gore=True, cx=0.0, cy=0.0):
    """Plaski pierscien (r0 < r1) na wysokosci z, normalna w gore albo w dol."""
    kat = np.linspace(0, 2 * np.pi, boki + 1)
    n = np.array([0, 0, 1.0 if w_gore else -1.0])
    for k in range(boki):
        t0, t1 = kat[k], kat[k + 1]
        q = [(cx + np.sin(t) * r, cy + np.cos(t) * r, z) for (t, r) in ((t0, r0), (t1, r0), (t1, r1), (t0, r1))]
        if r0 < 1e-3:
            _tri(s, mat, q[0], q[2], q[3], kolor, n)
        else:
            wielokat(s, mat, q, kolor, n=n)


def _stozek_dach(s, z, r, wys, kolor, boki=16, mat=DACH):
    """Dach stozkowy z okapem (bok w pasach o lekko roznych odcieniach jak gont/dachowka) i spodem."""
    kat = np.linspace(0, 2 * np.pi, boki + 1)
    top = np.array([0, 0, z + wys])
    for k in range(boki):
        t0, t1 = kat[k], kat[k + 1]
        a = np.array([np.sin(t0) * r, np.cos(t0) * r, z]); b = np.array([np.sin(t1) * r, np.cos(t1) * r, z])
        w = np.array([0, 0, z + wys * 0.25])
        wielokat(s, mat, [a, b, top], kolor * (0.94 + 0.06 * (k % 2)), od=w)
    _pierscien(s, mat, z - 0.02, 0.0, r, kolor * 0.55, boki, w_gore=False)
    return top


def _iglica_szczyt(s, p, h=1.4, kula=True):
    """Drzewce na szczycie dachu z galka."""
    p = np.asarray(p, float)
    walec(s, DREWNO, p - (0, 0, 0.4), p + (0, 0, h), 0.06, ZELAZO, boki=6)
    if kula:
        walec(s, DREWNO, p + (0, 0, h * 0.55), p + (0, 0, h * 0.55 + 0.26), 0.17, ZELAZO * 1.3, boki=8)


def _dach_kopertowy(s, x0, x1, y0, y1, z, wys, okap, kolor, mat=DACH):
    """Dach czterospadowy: kalenica wzdluz dluzszego boku, okap dookola, spod ciemniejszy."""
    X0, X1, Y0, Y1 = x0 - okap, x1 + okap, y0 - okap, y1 + okap
    zo = z - okap * wys / max((min(x1 - x0, y1 - y0)) / 2, 1e-3)
    xc, yc = (X0 + X1) / 2, (Y0 + Y1) / 2
    zk = z + wys
    if (X1 - X0) >= (Y1 - Y0):
        d = (Y1 - Y0) / 2
        k0, k1 = np.array([X0 + d, yc, zk]), np.array([X1 - d, yc, zk])
    else:
        d = (X1 - X0) / 2
        k0, k1 = np.array([xc, Y0 + d, zk]), np.array([xc, Y1 - d, zk])
    r = [np.array(p) for p in ((X0, Y0, zo), (X1, Y0, zo), (X1, Y1, zo), (X0, Y1, zo))]
    w = np.array([xc, yc, zo + wys * 0.3])
    if (X1 - X0) >= (Y1 - Y0):
        sciany = [[r[0], r[1], k1, k0], [r[1], r[2], k1], [r[2], r[3], k0, k1], [r[3], r[0], k0]]
    else:
        sciany = [[r[0], r[1], k0], [r[1], r[2], k1, k0], [r[2], r[3], k1], [r[3], r[0], k0, k1]]
    for k, f in enumerate(sciany):
        if len(f) == 4 and np.linalg.norm(f[2] - f[3]) < 1e-6:
            f = f[:3]
        wielokat(s, mat, f, kolor * (0.95 + 0.05 * (k % 2)), od=w)
    wielokat(s, mat, r, kolor * 0.55, n=np.array([0, 0, -1.0]))
    return k0, k1


def _pal(s, x, y, z0, z1, r, kolor, rng):
    """Pal palisady: pien 7-boczny lekko pochylony, zaostrzony czubek (jasne ociosane drewno)."""
    dx, dy = rng.uniform(-0.035, 0.035), rng.uniform(-0.02, 0.02)
    b = np.array([x + dx, y + dy, z1])
    walec(s, DREWNO, (x, y, z0), b, r, kolor, boki=7, denka=False)
    kat = np.linspace(0, 2 * np.pi, 7, endpoint=False) + rng.uniform(0, 1)
    kr = [b + np.array([np.cos(t) * r, np.sin(t) * r, 0]) for t in kat]
    czub = b + np.array([rng.uniform(-0.03, 0.03), rng.uniform(-0.03, 0.03), r * rng.uniform(2.4, 3.2)])
    for k in range(7):
        wielokat(s, DREWNO, [kr[k], kr[(k + 1) % 7], czub], CZUB * rng.uniform(0.85, 1.0), od=b)


def _ostrokol(s, x, rng, y0=-1.25, z0=-0.15):
    """Zaostrzony kolek wbity w stok walu, skosem na zewnatrz (przeszkoda przed palisada)."""
    a = np.array([x, y0, z0])
    b = a + np.array([rng.uniform(-0.15, 0.15), -1.25, 1.1 + rng.uniform(-0.15, 0.15)])
    walec(s, DREWNO, a, b, 0.065, RUSZT * rng.uniform(0.8, 1.0), boki=5, denka=False)
    d = (b - a) / np.linalg.norm(b - a)
    belka(s, DREWNO, b, b + d * 0.25, 0.06, CZUB * 0.9)


def _wal(s, x0, x1, y_zew, y_k0, y_k1, y_wew, h, z_dol=-0.8):
    """Wal ziemny wzdluz X: stok zewnetrzny (y_zew -> y_k0), korona (darn), stok wewnetrzny (y_k1 -> y_wew)."""
    P = [(y_zew, z_dol), (y_k0, h), (y_k1, h), (y_wew, z_dol)]
    kol = [WAL * 0.92, WAL_TRAWA, WAL * 1.0]
    for k in range(3):
        (ya, za), (yb, zb) = P[k], P[k + 1]
        wielokat(s, TYNK, [(x0, ya, za), (x1, ya, za), (x1, yb, zb), (x0, yb, zb)], kol[k], od=(0.5 * (x0 + x1), 0.5 * (y_k0 + y_k1), h - 1.5))
    for x, sx in ((x0, -1), (x1, 1)):
        wielokat(s, TYNK, [(x, y, z) for (y, z) in P], WAL * 0.85, n=np.array([sx, 0, 0.0]))


def _schodki(s, mat, x_dol, x_gora, y0, y1, z_gora, kolor, n=None, pelne=True):
    """Schody wzdluz X od ziemi (x_dol) do z_gora (x_gora): pelne stopnie (bryla) albo stopnice na policzkach."""
    n = n or max(4, int(round(z_gora / 0.32)))
    for k in range(n):
        f0, f1 = k / n, (k + 1) / n
        xa = x_dol + (x_gora - x_dol) * f0
        xb = x_dol + (x_gora - x_dol) * f1
        z = z_gora * f1
        lo, hi = min(xa, xb), max(xa, xb)
        if pelne:
            pudlo(s, mat, (lo, y0, -0.6), (hi + 0.02, y1, z), kolor * (0.94 + 0.06 * (k % 2)))
        else:
            pudlo(s, mat, (lo, y0, z - 0.07), (hi + 0.08, y1, z), kolor * (0.92 + 0.08 * (k % 2)), bez_dolu=False)
    if not pelne:
        for y in (y0 - 0.06, y1 + 0.06):  # policzki
            belka(s, DREWNO, (x_dol, y, -0.1), (x_gora, y, z_gora), 0.12, RUSZT * 0.85)


# ---------------- palisada ----------------

def palisada_segment(schody=False):
    s = Siatka()
    rng = np.random.default_rng(2440 + int(schody))
    H = WAL_H
    _wal(s, 0.0, 4.0, -2.7, -0.55, 1.15, 3.1, H)
    x = 0.0
    while True:  # pale rozne grubosci i wysokosci, scisle jeden przy drugim
        r = rng.uniform(0.13, 0.185)
        if x + 2 * r > 4.02:
            break
        _pal(s, x + r, rng.uniform(-0.03, 0.03), -0.2, H + rng.uniform(3.5, 4.0), r, RUSZT * rng.uniform(0.74, 1.0), rng)
        x += 2 * r + 0.008
    for z in (H + 0.9, H + 2.95):  # wiazania od srodka (pol-bale) z kolkami
        walec(s, DREWNO, (0.0, 0.24, z), (4.0, 0.24, z), 0.1, BELKA * 0.62, boki=6)
        for xk in (0.6, 2.0, 3.4):
            pudlo(s, DREWNO, (xk - 0.04, 0.3, z - 0.04), (xk + 0.04, 0.42, z + 0.04), BELKA * 0.85)
    # Ganek obroncow 2 m nad walem: stopnice z desek, legary, podciag na slupach z zastrzalami, porecz.
    P = H + 2.0
    for k in range(5):
        y0 = 0.3 + k * 0.28
        pudlo(s, DREWNO, (0.0, y0, P - 0.06), (4.0, y0 + 0.26, P), DESKA * rng.uniform(0.78, 0.95), bez_dolu=False)
    for xl in (0.5, 1.5, 2.5, 3.5):
        belka(s, DREWNO, (xl, 0.25, P - 0.14), (xl, 1.75, P - 0.14), 0.12, BELKA * 0.6)
    belka(s, DREWNO, (0.0, 1.6, P - 0.28), (4.0, 1.6, P - 0.28), 0.16, BELKA * 0.55)
    slupy = (1.0, 3.0)
    for xs in slupy:
        walec(s, DREWNO, (xs, 1.6, 0.1), (xs, 1.6, P + 1.05), 0.11, RUSZT * 0.9, boki=6)
        for dx in (-0.65, 0.65):
            belka(s, DREWNO, (xs, 1.6, P - 0.95), (xs + dx, 1.6, P - 0.3), 0.08, BELKA * 0.55)
    x_porecz = 1.35 if schody else 0.0
    belka(s, DREWNO, (x_porecz, 1.72, P + 1.0), (4.0, 1.72, P + 1.0), 0.08, BELKA * 0.7)
    if schody:
        # schody drewniane wzdluz ganku: od ziemi (x = 3,9) do ganku (x = 0,4), od srodka osady
        _schodki(s, DREWNO, 3.9, 0.4, 1.95, 2.75, P, DESKA * 0.85, pelne=False)
        walec(s, DREWNO, (0.4, 2.85, 0.1), (0.4, 2.85, P + 1.0), 0.08, RUSZT, boki=6)
        belka(s, DREWNO, (0.4, 2.85, P + 0.95), (3.9, 2.85, 1.0), 0.07, BELKA * 0.7)
    for xk in (0.55, 1.6, 2.65, 3.6):  # ostrokol na stoku walu
        _ostrokol(s, xk + rng.uniform(-0.15, 0.15), rng)
    return s


def _izba_z_desek(s, a, z0, z1, rng, okna=2):
    """Wysunieta izba strzelnicza: podloga, sciany z pionowych desek (kwadrat +-a), oczep, strzelnice."""
    pudlo(s, DREWNO, (-a, -a, z0 - 0.16), (a, a, z0), DESKA * 0.7, bez_dolu=False)
    n = max(4, int(round(2 * a / 0.3)))
    w = 2 * a / n
    for bok in range(4):
        for k in range(n):
            u0, u1 = -a + k * w, -a + (k + 1) * w - 0.012
            kol = DESKA_W * rng.uniform(0.82, 1.05)
            if bok == 0:
                pudlo(s, DREWNO, (u0, -a, z0), (u1, -a + 0.06, z1), kol)
            elif bok == 1:
                pudlo(s, DREWNO, (u0, a - 0.06, z0), (u1, a, z1), kol)
            elif bok == 2:
                pudlo(s, DREWNO, (-a, u0, z0), (-a + 0.06, u1, z1), kol)
            else:
                pudlo(s, DREWNO, (a - 0.06, u0, z0), (a, u1, z1), kol)
    for x in (-a, a):
        for y in (-a, a):
            pudlo(s, DREWNO, (x - 0.1, y - 0.1, z0), (x + 0.1, y + 0.1, z1 + 0.05), RUSZT * 0.8)
    for (p, q) in (((-a, -a), (a, -a)), ((a, -a), (a, a)), ((a, a), (-a, a)), ((-a, a), (-a, -a))):
        belka(s, DREWNO, (p[0], p[1], z1), (q[0], q[1], z1), 0.16, BELKA * 0.6)
        belka(s, DREWNO, (p[0], p[1], z0 + 0.95), (q[0], q[1], z0 + 0.95), 0.08, BELKA * 0.55)  # listwa pod strzelnicami
    zs = z0 + 1.05
    for k in range(okna):
        u = -a + 2 * a * (k + 0.5) / okna
        pudlo(s, DREWNO, (u - 0.14, -a - 0.02, zs), (u + 0.14, -a, zs + 0.55), OKNO)
        pudlo(s, DREWNO, (u - 0.14, a, zs), (u + 0.14, a + 0.02, zs + 0.55), OKNO)
        pudlo(s, DREWNO, (-a - 0.02, u - 0.14, zs), (-a, u + 0.14, zs + 0.55), OKNO)
        pudlo(s, DREWNO, (a, u - 0.14, zs), (a + 0.02, u + 0.14, zs + 0.55), OKNO)


def _wsporniki(s, a, b, z):
    """Belki wysuniete spod izby (od scian +-a do +-b) z zastrzalami do sciany ponizej."""
    for t in (-a * 0.55, a * 0.55):
        for (p, q) in (((t, -a), (t, -b)), ((t, a), (t, b)), ((-a, t), (-b, t)), ((a, t), (b, t))):
            belka(s, DREWNO, (p[0], p[1], z - 0.12), (q[0], q[1], z - 0.12), 0.2, BELKA * 0.55)
            belka(s, DREWNO, (p[0], p[1], z - 1.0), ((p[0] + q[0]) / 2 + (q[0] - p[0]) * 0.3, (p[1] + q[1]) / 2 + (q[1] - p[1]) * 0.3, z - 0.2),
                  0.11, BELKA * 0.5)


def _drzwi_proste(s, x, y, z0, szer, wys, strona):
    """Drzwi z desek w scianie prostopadlej do Y (strona = kierunek normalnej), z oscieznica."""
    y1 = y + strona * 0.07
    pudlo(s, DREWNO, (x - szer / 2, min(y, y1), z0), (x + szer / 2, max(y, y1), z0 + wys), DRZWI)
    for dx in (-szer / 2 - 0.08, szer / 2 + 0.08):
        pudlo(s, DREWNO, (x + dx - 0.08, min(y, y1 + strona * 0.03), z0), (x + dx + 0.08, max(y, y1 + strona * 0.03), z0 + wys + 0.1), BELKA * 0.6)
    pudlo(s, DREWNO, (x - szer / 2 - 0.16, min(y, y1 + strona * 0.03), z0 + wys), (x + szer / 2 + 0.16, max(y, y1 + strona * 0.03), z0 + wys + 0.18), BELKA * 0.6)
    for z in (z0 + 0.5, z0 + wys - 0.5):  # okucia
        pudlo(s, DREWNO, (x - szer / 2 + 0.05, min(y1, y1 + strona * 0.02), z), (x + szer / 2 - 0.1, max(y1, y1 + strona * 0.02), z + 0.07), ZELAZO)


def palisada_wieza():
    """Baszta drewniana 4,6 x 4,6 m na walu: zrab z bali do 5,5 m, izba z desek wysunieta na wspornikach, dach z gontu."""
    s = Siatka()
    rng = np.random.default_rng(2460)
    H = WAL_H
    a, Z1 = 2.3, H + 4.6
    # kopiec walu pod baszta (szerszy niz wal odcinkow)
    for (x0, x1) in ((-3.9, 3.9),):
        _wal(s, x0, x1, -3.2, -2.45, 2.45, 3.5, H)
    zrab(s, -a, a, -a, a, Z1, H - 0.1, rng, d=0.3, wysun=0.32)
    _drzwi_proste(s, 0.0, a, H, 1.2, 2.5, 1)  # wejscie od srodka osady
    for k in range(3):  # stopnie z kamieni ze stoku walu do drzwi
        pudlo(s, KAMIEN, (-0.75, a + 0.1 + k * 0.42, -0.6), (0.75, a + 0.52 + k * 0.42, H - 0.3 * (k + 1)), MUR_COKOL * (0.95 - 0.04 * k))
    for z in (H + 1.6, H + 3.1):  # strzelnice w zrebie
        for (x, y) in ((0.0, -a), (-a, 0.0), (a, 0.0)):
            if y == -a:
                pudlo(s, DREWNO, (x - 0.12, -a - 0.03, z), (x + 0.12, -a + 0.02, z + 0.45), OKNO)
            else:
                pudlo(s, DREWNO, (x - 0.03 if x < 0 else x - 0.02, -0.12, z), (x + 0.02 if x < 0 else x + 0.03, 0.12, z + 0.45), OKNO)
    b = a + 0.55
    _wsporniki(s, a, b + 0.1, Z1 + 0.05)
    _izba_z_desek(s, b, Z1 + 0.2, Z1 + 2.4, rng, okna=2)
    k0 = iglica(s, -b, b, -b, b, Z1 + 2.5, 4.0, okap=0.55)
    _iglica_szczyt(s, k0)
    return s


def palisada_brama():
    """Brama drewniana: wieza bramna 6,8 x 5,2 m, przejazd 3,8 x 4,3 m wzdluz Y, otwarte wrota, izba i dach czterospadowy."""
    s = Siatka()
    rng = np.random.default_rng(2470)
    H = WAL_H
    ax, ay, p = 3.4, 2.6, 1.9  # pol szerokosci, pol glebokosci, pol szerokosci przejazdu
    Z1, Z2 = 4.3, 6.9
    for sx in (-1, 1):  # wal po bokach przejazdu
        x0, x1 = sorted((sx * (p + 0.05), sx * 5.2))
        _wal(s, x0, x1, -3.4, -2.7, 2.7, 3.7, H)
        xa, xb = sorted((sx * p, sx * ax))
        zrab(s, xa, xb, -ay, ay, Z1, -0.2, rng, d=0.3, wysun=0.28)  # sciany przejazdu
    zrab(s, -ax, ax, -ay, ay, Z2, Z1, rng, d=0.3, wysun=0.32)
    for y in (-ay - 0.05, ay + 0.05):  # nadproze i pulap przejazdu
        belka(s, DREWNO, (-p - 0.4, y, Z1 - 0.2), (p + 0.4, y, Z1 - 0.2), 0.36, BELKA * 0.55)
    pudlo(s, DREWNO, (-p, -ay, Z1 - 0.12), (p, ay, Z1 + 0.02), DESKA * 0.5, bez_dolu=False)
    # wrota otwarte do srodka osady (+Y): skrzydla z desek z trzema poprzeczkami, przy scianach przejazdu
    for sx in (-1, 1):
        h0 = np.array([sx * (p - 0.08), ay - 0.15, 0.0])
        kier = np.array([-sx * 0.22, 0.975, 0.0]); kier /= np.linalg.norm(kier)
        L, wys, t = 1.85, 3.9, 0.12
        nrm = np.array([kier[1], -kier[0], 0.0]) * t
        A = [h0, h0 + kier * L, h0 + kier * L + (0, 0, wys), h0 + (0, 0, wys)]
        B = [q + nrm for q in A]
        w = (A[0] + A[2]) / 2 + nrm / 2
        wielokat(s, DREWNO, A, DRZWI * 1.15, od=w)
        wielokat(s, DREWNO, B, DRZWI * 1.15, od=w)
        for k in range(4):
            wielokat(s, DREWNO, [A[k], A[(k + 1) % 4], B[(k + 1) % 4], B[k]], DRZWI * 0.9, od=w)
        for z in (0.5, 2.0, 3.4):
            for q in (A[0] - nrm * 0.6, A[0] + nrm * 1.6):
                belka(s, DREWNO, q + (0, 0, z), q + kier * L + (0, 0, z), 0.1, DRZWI * 0.75)
    for t in (-2.2, -0.7, 0.7, 2.2):  # wsporniki pod izba (od przodu i tylu)
        for (y0, y1) in ((-ay, -ay - 0.65), (ay, ay + 0.65)):
            belka(s, DREWNO, (t, y0, Z2 - 0.07), (t, y1, Z2 - 0.07), 0.2, BELKA * 0.55)
            belka(s, DREWNO, (t, y0, Z2 - 0.95), (t, y0 + (y1 - y0) * 0.75, Z2 - 0.15), 0.11, BELKA * 0.5)
    # izba z desek nad przejazdem (prostokat), strzelnice od pola i od osady
    bx, by = ax + 0.4, ay + 0.55
    z0, z1 = Z2 + 0.15, Z2 + 2.3
    pudlo(s, DREWNO, (-bx, -by, z0 - 0.16), (bx, by, z0), DESKA * 0.7, bez_dolu=False)
    for (u0, u1, v, os_) in ((-bx, bx, -by, "x"), (-bx, bx, by, "x"), (-by, by, -bx, "y"), (-by, by, bx, "y")):
        n = int(round((u1 - u0) / 0.3))
        w = (u1 - u0) / n
        for k in range(n):
            a0, a1 = u0 + k * w, u0 + (k + 1) * w - 0.012
            kol = DESKA_W * rng.uniform(0.82, 1.05)
            g = 0.06 if v > 0 else -0.06
            if os_ == "x":
                pudlo(s, DREWNO, (a0, min(v, v - g), z0), (a1, max(v, v - g), z1), kol)
            else:
                pudlo(s, DREWNO, (min(v, v - g), a0, z0), (max(v, v - g), a1, z1), kol)
    for x in (-bx, bx):
        for y in (-by, by):
            pudlo(s, DREWNO, (x - 0.1, y - 0.1, z0), (x + 0.1, y + 0.1, z1 + 0.05), RUSZT * 0.8)
    for (p0, q0) in (((-bx, -by), (bx, -by)), ((bx, -by), (bx, by)), ((bx, by), (-bx, by)), ((-bx, by), (-bx, -by))):
        belka(s, DREWNO, (p0[0], p0[1], z1), (q0[0], q0[1], z1), 0.16, BELKA * 0.6)
        belka(s, DREWNO, (p0[0], p0[1], z0 + 0.95), (q0[0], q0[1], z0 + 0.95), 0.08, BELKA * 0.55)
    for u in (-2.4, -0.8, 0.8, 2.4):
        for y, g in ((-by, -0.02), (by, 0.02)):
            pudlo(s, DREWNO, (u - 0.14, min(y, y + g), z0 + 1.05), (u + 0.14, max(y, y + g), z0 + 1.6), OKNO)
    k0, k1 = _dach_kopertowy(s, -bx, bx, -by, by, z1 + 0.1, 3.4, 0.55, GONT)
    for k in (k0, k1):
        _iglica_szczyt(s, k, 1.2)
    return s


# ---------------- mur kamienny ----------------

def _warstwy(s, x0, x1, y0, y1, z0, z1, rng, kolor=MUR, wys=0.62, wciecie=0.012):
    """Mur z warstw kamienia: co druga warstwa minimalnie cofnieta od lica (cienka linia cienia), odcienie roznia sie."""
    n = max(1, int(round((z1 - z0) / wys)))
    for k in range(n):
        za, zb = z0 + (z1 - z0) * k / n, z0 + (z1 - z0) * (k + 1) / n
        d = wciecie if k % 2 else 0.0
        pudlo(s, KAMIEN, (x0 + d, y0 + d, za), (x1 - d, y1 - d, zb), kolor * rng.uniform(0.9, 1.04))


def _blanki(s, x0, x1, y0, y1, z0, z1, rng, sz=1.3, przerwa=0.7, start=0.35, os_="x", strzelnice=-1):
    """Blanki (zeby muru) na przedpiersiu wzdluz osi, z daszkiem i strzelnica od zewnatrz."""
    L = (x1 - x0) if os_ == "x" else (y1 - y0)
    u = start
    while u + sz <= L - start + 1e-3:
        if os_ == "x":
            a, b = (x0 + u, y0, z0), (x0 + u + sz, y1, z1)
        else:
            a, b = (x0, y0 + u, z0), (x1, y0 + u + sz, z1)
        pudlo(s, KAMIEN, a, b, MUR * rng.uniform(0.9, 1.02))
        pudlo(s, KAMIEN, (a[0] - 0.05, a[1] - 0.05, z1), (b[0] + 0.05, b[1] + 0.05, z1 + 0.1), MUR * 0.85)  # daszek
        c = u + sz / 2
        if strzelnice != 0:
            if os_ == "x":
                y = y0 if strzelnice < 0 else y1
                pudlo(s, KAMIEN, (x0 + c - 0.07, y - 0.012, z0 + 0.2), (x0 + c + 0.07, y + 0.012, z1 - 0.2), OKNO)
            else:
                x = x0 if strzelnice < 0 else x1
                pudlo(s, KAMIEN, (x - 0.012, y0 + c - 0.07, z0 + 0.2), (x + 0.012, y0 + c + 0.07, z1 - 0.2), OKNO)
        u += sz + przerwa


MUR_H = 5.6      # chodnik obroncow
MUR_Y0, MUR_Y1 = -1.15, 0.95   # lico zewnetrzne i wewnetrzne


def mur_segment(schody=False):
    """Mur kamienny 4 m: cokol ze skarpa, warstwy kamienia, gzyms, przedpiersie z blankami, chodnik 1,6 m."""
    s = Siatka()
    rng = np.random.default_rng(2480 + int(schody))
    H, y0, y1 = MUR_H, MUR_Y0, MUR_Y1
    pudlo(s, KAMIEN, (0.0, y0 - 0.45, -0.8), (4.0, y1 + 0.15, 0.45), MUR_COKOL)
    wielokat(s, KAMIEN, [(0.0, y0 - 0.45, 0.45), (4.0, y0 - 0.45, 0.45), (4.0, y0, 1.6), (0.0, y0, 1.6)], MUR_COKOL * 1.05, od=(2.0, 0.0, 0.0))
    for x in (0.0, 4.0):
        wielokat(s, KAMIEN, [(x, y0 - 0.45, 0.45), (x, y0, 1.6), (x, y0, 0.45)], MUR_COKOL, n=np.array([1.0 if x else -1.0, 0, 0]))
    _warstwy(s, 0.0, 4.0, y0, y1, 0.45, H, rng)
    pudlo(s, KAMIEN, (0.0, y0 - 0.12, H - 0.32), (4.0, y0 + 0.2, H - 0.1), MUR * 0.88)  # gzyms
    pudlo(s, KAMIEN, (0.0, y1 - 0.2, H - 0.25), (4.0, y1 + 0.08, H - 0.1), MUR * 0.88)
    # przedpiersie (do pasa ludzika) i blanki z daszkami
    pudlo(s, KAMIEN, (0.0, y0, H), (4.0, y0 + 0.55, H + 1.05), MUR * 0.97)
    _blanki(s, 0.0, 4.0, y0, y0 + 0.55, H + 1.05, H + 2.15, rng, strzelnice=-1)
    pudlo(s, KAMIEN, (0.0, y1 - 0.35, H), (4.0, y1, H + 0.45), MUR * 0.93)  # niski murek od srodka
    pudlo(s, KAMIEN, (0.0, y0 + 0.55, H - 0.02), (4.0, y1 - 0.35, H + 0.02), MUR_COKOL * 1.1)  # posadzka chodnika
    for (x, z) in ((0.7, 2.3), (2.6, 2.3), (1.6, 3.9), (3.4, 4.2)):  # otwory po rusztowaniu
        pudlo(s, KAMIEN, (x, y0 - 0.012, z), (x + 0.2, y0 + 0.01, z + 0.2), OKNO)
    if schody:
        # kamienne schody od srodka osady: od ziemi (x = 6,8, wchodza pod sasiedni odcinek) do chodnika (x = 0,3)
        _schodki(s, KAMIEN, 6.8, 0.3, y1, y1 + 1.2, H, MUR_COKOL * 1.05)
    return s


def mur_wieza():
    """Okragla baszta kamienna r = 3,3 m: skarpa, warstwy, gzymsy, wsporniki pod wysunietym zwienczeniem, dach stozkowy."""
    s = Siatka()
    rng = np.random.default_rng(2490)
    R, B = 3.3, 16
    _beben(s, KAMIEN, -0.8, 0.3, R + 0.55, R + 0.55, MUR_COKOL, B)
    _beben(s, KAMIEN, 0.3, 1.9, R + 0.55, R, MUR_COKOL * 1.05, B)
    z, Zk = 1.9, 9.4
    n = int(round((Zk - z) / 0.62))
    for k in range(n):
        za, zb = z + (Zk - z) * k / n, z + (Zk - z) * (k + 1) / n
        _beben(s, KAMIEN, za, zb, R - (0.012 if k % 2 else 0.0), R - (0.012 if k % 2 else 0.0), MUR * rng.uniform(0.9, 1.04), B)
    for zg in (1.9, 6.2):  # gzymsy
        _beben(s, KAMIEN, zg, zg + 0.2, R + 0.12, R + 0.12, MUR * 0.88, B)
        _pierscien(s, KAMIEN, zg + 0.2, R, R + 0.12, MUR * 0.88, B)
        _pierscien(s, KAMIEN, zg, R, R + 0.12, MUR * 0.7, B, w_gore=False)
    # wsporniki i wysuniete zwienczenie z okienkami
    Rz, z0, z1 = R + 0.45, Zk + 0.55, Zk + 2.4
    for k in range(B):
        t = 2 * np.pi * (k + 0.5) / B
        d = np.array([np.sin(t), np.cos(t), 0])
        belka(s, KAMIEN, d * (R - 0.1) + (0, 0, Zk + 0.15), d * (Rz - 0.08) + (0, 0, Zk + 0.4), 0.3, MUR * 0.86)
    _beben(s, KAMIEN, Zk, z0, R, R, MUR * 0.95, B)
    _pierscien(s, KAMIEN, z0, R - 0.2, Rz, MUR * 0.62, B, w_gore=False)
    for k in range(3):
        za, zb = z0 + (z1 - z0) * k / 3, z0 + (z1 - z0) * (k + 1) / 3
        _beben(s, KAMIEN, za, zb, Rz, Rz, MUR * rng.uniform(0.92, 1.03), B)
    for k in range(6):  # okienka zwienczenia
        t = 2 * np.pi * (k + 0.25) / 6
        d = np.array([np.sin(t), np.cos(t), 0]); e = np.array([d[1], -d[0], 0])
        c = d * (Rz + 0.015) + (0, 0, z0 + 0.65)
        wielokat(s, KAMIEN, [c - e * 0.2, c + e * 0.2, c + e * 0.2 + (0, 0, 0.75), c - e * 0.2 + (0, 0, 0.75)], OKNO, n=d)
    for (t, zz) in ((-150, 3.0), (-90, 4.6), (-30, 3.0), (-120, 7.2), (-60, 7.2), (180, 4.6), (0, 4.6)):  # strzelnice
        tt = np.radians(t + 90)  # 0 st. = +X; -90 = -Y (od pola)
        d = np.array([np.sin(tt), np.cos(tt), 0]); e = np.array([d[1], -d[0], 0])
        c = d * (R + 0.015) + (0, 0, zz)
        wielokat(s, KAMIEN, [c - e * 0.08, c + e * 0.08, c + e * 0.08 + (0, 0, 1.0), c - e * 0.08 + (0, 0, 1.0)], OKNO, n=d)
    # drzwi od srodka osady (+Y)
    d = np.array([0, 1.0, 0])
    c = d * (R + 0.02)
    wielokat(s, DREWNO, [c + (-0.65, 0, 0.3), c + (0.65, 0, 0.3), c + (0.65, 0, 2.8), c + (-0.65, 0, 2.8)], DRZWI, n=d)
    wielokat(s, DREWNO, [c + (-0.65, 0, 2.8), c + (0.65, 0, 2.8), c + (0, 0, 3.35)], DRZWI, n=d)
    pudlo(s, KAMIEN, (-0.85, R - 0.1, 0.25), (0.85, R + 0.5, 0.45), MUR_COKOL)  # prog
    top = _stozek_dach(s, z1, Rz + 0.5, 7.2, DACHOWKA, B)
    _iglica_szczyt(s, top, 1.6)
    return s


def _luk(s, y0, y1, p, cz, ztop, kolor, n=10):
    """Wypelnienie nad lukiem przejazdu (polkole o promieniu p, srodek na wys. cz) do prostokata |x| <= p, z <= ztop:
    lico zewnetrzne (y0) i wewnetrzne (y1) plus podniebienie luku."""
    katy = sorted(set(list(np.linspace(0, np.pi, n + 1)) + [np.arctan2(ztop - cz, p), np.pi - np.arctan2(ztop - cz, p)]))
    def A(t):
        return np.array([p * np.cos(t), cz + p * np.sin(t)])
    def Bk(t):
        c, sn = np.cos(t), np.sin(t)
        tx = p / abs(c) if abs(c) > 1e-6 else 1e9
        tz = (ztop - cz) / sn if sn > 1e-6 else 1e9
        return np.array([c, sn]) * min(tx, tz) + (0, cz)
    for y, ny in ((y0, -1.0), (y1, 1.0)):
        for t0, t1 in zip(katy[:-1], katy[1:]):
            a0, a1, b0, b1 = A(t0), A(t1), Bk(t0), Bk(t1)
            P = lambda q: (q[0], y, q[1])
            _tri(s, KAMIEN, P(a0), P(a1), P(b1), kolor, (0, ny, 0))
            _tri(s, KAMIEN, P(a0), P(b1), P(b0), kolor, (0, ny, 0))
    for t0, t1 in zip(katy[:-1], katy[1:]):
        a0, a1 = A(t0), A(t1)
        tm = (t0 + t1) / 2
        wielokat(s, KAMIEN, [(a0[0], y0, a0[1]), (a1[0], y0, a1[1]), (a1[0], y1, a1[1]), (a0[0], y1, a0[1])], kolor * 0.72,
                 n=np.array([-np.cos(tm), 0, -np.sin(tm)]))
    # kamienie lukowe (archiwolta) na licach
    for y, ny in ((y0 - 0.06, -1.0), (y1 + 0.06, 1.0)):
        for k in range(n):
            t0, t1 = np.pi * k / n, np.pi * (k + 1) / n
            q = [(p * np.cos(t0), cz + p * np.sin(t0)), (p * np.cos(t1), cz + p * np.sin(t1)),
                 ((p + 0.45) * np.cos(t1), cz + (p + 0.45) * np.sin(t1)), ((p + 0.45) * np.cos(t0), cz + (p + 0.45) * np.sin(t0))]
            yy = (y, y - ny * 0.06)
            pts = [(q[i][0], yy[0], q[i][1]) for i in range(4)]
            wielokat(s, KAMIEN, pts, MUR * (0.84 if k % 2 else 0.92), n=np.array([0, ny, 0]))


def mur_brama():
    """Kamienna wieza bramna 8,4 x 7 m: przejazd lukowy 3,8 m (wzdluz Y), brona, otwarte wrota, okna, wsporniki,
    blanki i dach czterospadowy z dachowki."""
    s = Siatka()
    rng = np.random.default_rng(2500)
    ax, y0, y1, p = 4.2, -4.0, 3.0, 1.9
    cz, zl, Zk = 3.0, 5.4, 11.0   # wysokosc wezglowia luku, gora wypelnienia nad lukiem, korona wiezy
    pudlo(s, KAMIEN, (-ax - 0.35, y0 - 0.35, -0.8), (-p, y1 + 0.3, 0.45), MUR_COKOL)
    pudlo(s, KAMIEN, (p, y0 - 0.35, -0.8), (ax + 0.35, y1 + 0.3, 0.45), MUR_COKOL)
    pudlo(s, KAMIEN, (-p, y0, -0.8), (p, y1, 0.04), MUR_COKOL * 0.9)  # bruk przejazdu
    for (xa, xb) in ((-ax, -p), (p, ax)):  # filary przejazdu
        _warstwy(s, xa, xb, y0, y1, 0.45, zl, rng)
    _luk(s, y0, y1, p, cz, zl, MUR * 0.97)
    _warstwy(s, -ax, ax, y0, y1, zl, Zk, rng)
    for zg in (zl + 0.1,):
        pudlo(s, KAMIEN, (-ax - 0.1, y0 - 0.1, zg), (ax + 0.1, y1 + 0.1, zg + 0.22), MUR * 0.86)
    # okna (lukowe) od pola i od osady
    for (x, z, sz, wy) in ((-2.3, 6.6, 0.6, 1.2), (2.3, 6.6, 0.6, 1.2), (0.0, 8.4, 0.75, 1.4)):
        okno(s, x, y0, z, sz, wy, "y", -1)
        okno(s, x, y1, z, sz, wy, "y", 1)
    for (y, z) in ((-2.0, 7.4), (1.2, 7.4)):
        okno(s, -ax, y, z, 0.5, 1.0, "x", -1)
        okno(s, ax, y, z, 0.5, 1.0, "x", 1)
    # brona w luku od pola: kraty z belek okutych, podniesiona do polowy
    yb = y0 + 0.45
    for k in range(11):
        x = -p + 0.2 + k * (2 * p - 0.4) / 10
        ztop = cz + np.sqrt(max(p * p - x * x, 0.0)) - 0.05
        belka(s, DREWNO, (x, yb, 2.3), (x, yb, ztop), 0.1, ZELAZO)
        belka(s, DREWNO, (x, yb, 2.05), (x, yb, 2.3), 0.05, ZELAZO)
    for z in (2.6, 3.3, 4.0, 4.6):
        dx = np.sqrt(max(p * p - max(z - cz, 0.0) ** 2, 0.0)) - 0.05
        belka(s, DREWNO, (-dx, yb - 0.08, z), (dx, yb - 0.08, z), 0.09, ZELAZO * 0.9)
    # wrota otwarte do srodka osady
    for sx in (-1, 1):
        h0 = np.array([sx * (p - 0.06), y1 - 0.2, 0.05])
        kier = np.array([-sx * 0.2, 0.98, 0.0]); kier /= np.linalg.norm(kier)
        L, wys, t = 1.85, 3.6, 0.14
        nrm = np.array([kier[1], -kier[0], 0.0]) * t * sx
        A = [h0, h0 + kier * L, h0 + kier * L + (0, 0, wys), h0 + (0, 0, wys)]
        Bq = [q + nrm for q in A]
        w = (A[0] + A[2]) / 2 + nrm / 2
        for f in (A, Bq):
            wielokat(s, DREWNO, f, DRZWI * 1.1, od=w)
        for k in range(4):
            wielokat(s, DREWNO, [A[k], A[(k + 1) % 4], Bq[(k + 1) % 4], Bq[k]], DRZWI * 0.85, od=w)
        for z in (0.5, 1.8, 3.1):
            belka(s, DREWNO, A[0] + nrm * 1.4 + (0, 0, z), A[1] + nrm * 1.4 + (0, 0, z), 0.07, ZELAZO)
    # wsporniki, wysuniete przedpiersie z blankami, dach za blankami
    for (u0, u1, os_, v) in ((-ax, ax, "x", y0), (-ax, ax, "x", y1), (y0, y1, "y", -ax), (y0, y1, "y", ax)):
        L = u1 - u0
        n = int(L / 0.9)
        for k in range(n + 1):
            u = u0 + 0.25 + k * (L - 0.5) / n
            out = -1 if v in (y0, -ax) else 1
            if os_ == "x":
                pudlo(s, KAMIEN, (u - 0.17, min(v, v + out * 0.45), Zk - 0.55), (u + 0.17, max(v, v + out * 0.45), Zk + 0.3), MUR * 0.86)
            else:
                pudlo(s, KAMIEN, (min(v, v + out * 0.45), u - 0.17, Zk - 0.55), (max(v, v + out * 0.45), u + 0.17, Zk + 0.3), MUR * 0.86)
    e = 0.45
    X0, X1, Y0, Y1 = -ax - e, ax + e, y0 - e, y1 + e
    g = 0.5
    zp0, zp1 = Zk + 0.3, Zk + 1.2
    for (a, b) in (((X0, Y0), (X1, Y0 + g)), ((X0, Y1 - g), (X1, Y1)), ((X0, Y0), (X0 + g, Y1)), ((X1 - g, Y0), (X1, Y1))):
        pudlo(s, KAMIEN, (a[0], a[1], zp0), (b[0], b[1], zp1), MUR * 0.97)
    pudlo(s, KAMIEN, (X0, Y0, zp0 - 0.02), (X1, Y1, zp0 + 0.02), MUR * 0.6, bez_dolu=False)
    _blanki(s, X0, X1, Y0, Y0 + g, zp1, zp1 + 1.0, rng, sz=1.0, przerwa=0.65, start=0.25, os_="x", strzelnice=-1)
    _blanki(s, X0, X1, Y1 - g, Y1, zp1, zp1 + 1.0, rng, sz=1.0, przerwa=0.65, start=0.25, os_="x", strzelnice=1)
    _blanki(s, X0, X0 + g, Y0, Y1, zp1, zp1 + 1.0, rng, sz=1.0, przerwa=0.65, start=0.9, os_="y", strzelnice=-1)
    _blanki(s, X1 - g, X1, Y0, Y1, zp1, zp1 + 1.0, rng, sz=1.0, przerwa=0.65, start=0.9, os_="y", strzelnice=1)
    k0, k1 = _dach_kopertowy(s, -ax + 0.2, ax - 0.2, y0 + 0.2, y1 - 0.2, zp0 + 0.5, 4.6, 0.05, DACHOWKA)
    for k in (k0, k1):
        _iglica_szczyt(s, k, 1.4)
    return s


def studnia_placu():
    """Studnia na placu osady rodu (jak przy kosciele) z korytem i wiadrem."""
    s = Siatka()
    studnia(s, 0.0, 0.0)
    pudlo(s, DREWNO, (1.0, -0.3, 0.0), (2.6, 0.3, 0.45), DESKA * 0.7)
    pudlo(s, DREWNO, (1.1, -0.2, 0.3), (2.5, 0.2, 0.46), OKNO)
    walec(s, DREWNO, (-0.2, 0.0, 0.81), (-0.2, 0.0, 1.15), 0.18, DRZWI * 1.3, boki=8)
    return s


# Wiatrak kozlak: budka na slupie (kozle) obracana do wiatru dyszlem; skrzydla od frontu (-Y) - osobny model,
# gra kreci nimi przy pracy (ABudowa::Ruchoma, os Y przez piaste).
MLYN = Bryla(-1.9, 1.9, -1.7, 1.7, 7.0, 1.6, t=0.12)
MLYN_Z = 3.2                   # podloga budki
MLYN_PIASTA = (0.0, -1.95, 6.0)


def kozlak(s, faza):
    """Podstawa: kamienne podmurowki, krzyz podwalin, zastrzaly i slup."""
    for x, y in ((-2.2, 0), (2.2, 0), (0, -2.2), (0, 2.2)):
        pudlo(s, KAMIEN, (x - 0.35, y - 0.35, -0.5), (x + 0.35, y + 0.35, 0.3), MUR_COKOL, bez_dolu=False)
    belka(s, DREWNO, (-2.4, 0, 0.45), (2.4, 0, 0.45), 0.3, BALE * 0.85)
    belka(s, DREWNO, (0, -2.4, 0.62), (0, 2.4, 0.62), 0.3, BALE * 0.8)
    if faza >= 1:
        hs = MLYN_Z if faza >= 2 else 2.2
        walec(s, DREWNO, (0, 0, 0.3), (0, 0, hs), 0.24, BALE * 0.9, boki=8)
        for x, y in ((-2.0, 0), (2.0, 0), (0, -2.0), (0, 2.0)):
            belka(s, DREWNO, (x, y, 0.6), (x * 0.1, y * 0.1, 2.0), 0.16, BALE * 0.8)


def mlyn(faza):
    """Mlyn: F1 kozlak, F2 szkielet budki, F3 budka obita deskami i pol dachu, F4 gotowy z dyszlem i schodami."""
    s = Siatka()
    B = MLYN
    z0 = MLYN_Z
    kozlak(s, faza)
    if faza >= 2:
        pudlo(s, DREWNO, (B.x0 - 0.1, B.y0 - 0.1, z0 - 0.25), (B.x1 + 0.1, B.y1 + 0.1, z0), BALE * 0.8, bez_dolu=False)
        for x in (B.x0, 0.0, B.x1):
            for y in (B.y0, B.y1):
                belka(s, DREWNO, (x, y, z0), (x, y, B.h), 0.16, BALE)
        for y in (B.y0, B.y1):
            belka(s, DREWNO, (B.x0, y, B.h - 0.08), (B.x1, y, B.h - 0.08), 0.16, BALE * 0.9)
            belka(s, DREWNO, (B.x0, y, z0 + 0.1), (B.x1, y, B.h - 0.2), 0.1, BALE * 0.85)  # zastrzal
    if faza >= 3:
        hsc = B.h if faza == 4 else z0 + 2.2
        # obicie z pionowych desek
        for y, st in ((B.y0, -1), (B.y1, 1)):
            for k, x in enumerate(np.arange(B.x0, B.x1 - 0.01, 0.32)):
                pudlo(s, DREWNO, (x, min(y, y + st * 0.08), z0), (x + 0.31, max(y, y + st * 0.08), hsc), DESKA * (0.84 + 0.07 * (k % 3)))
        for x, st in ((B.x0, -1), (B.x1, 1)):
            for k, y in enumerate(np.arange(B.y0, B.y1 - 0.01, 0.32)):
                pudlo(s, DREWNO, (min(x, x + st * 0.08), y, z0), (max(x, x + st * 0.08), y + 0.31, hsc), DESKA * (0.82 + 0.07 * (k % 3)))
        wiezba(s, B, kolor=BALE)
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.3)
    if faza >= 3:
        for x, st in ((B.x0 - 0.08, -1), (B.x1 - 0.04, 1)):
            szczyt(s, x, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
    if faza == 4:
        drzwi(s, 0.9, B.y1 + 0.08, 0.9, 1.7, os="y", strona=1)
        pudlo(s, DREWNO, (-0.9, B.y0 - 0.14, z0 + 1.6), (-0.3, B.y0 - 0.08, z0 + 2.2), OKNO)  # okienko
        # schody z tylu (+Y) i dyszel do obracania
        for k in range(8):
            f = k / 7
            pudlo(s, DREWNO, (0.3, B.y1 + 0.2 + (1 - f) * 3.0, f * z0 - 0.05), (1.5, B.y1 + 0.55 + (1 - f) * 3.0, f * z0), DESKA)
        for x in (0.3, 1.5):
            belka(s, DREWNO, (x, B.y1 + 3.5, 0.0), (x, B.y1 + 0.3, z0), 0.1, RUSZT)
        belka(s, DREWNO, (-0.6, B.y1, z0 + 0.1), (-0.9, B.y1 + 4.2, 0.4), 0.2, BALE * 0.85)
        # waly skrzydel: czop z przodu
        walec(s, DREWNO, (MLYN_PIASTA[0], B.y0 + 0.2, MLYN_PIASTA[2]), MLYN_PIASTA, 0.22, BALE * 0.75, boki=8)
        worek(s, -1.4, -3.0, kolor=MAKA_W)
        worek(s, -0.9, -3.2, kolor=MAKA_W * 0.96)
    if faza <= 2:
        stos_belek(s, -0.5, -4.2, 4.0, 5 if faza == 1 else 3)
    if faza == 2:
        stos_desek(s, 0.0, 4.2, 3.0, 4)
    if faza == 3:
        drabina(s, B.x1 - 0.6, B.y0 - 0.1, B.h - 1.0)
    return s


def mlyn_skrzydla():
    """Cztery skrzydla wiatraka wokol piasty w (0,0,0), w plaszczyznie XZ, os obrotu = Y; ramiona po 5,3 m."""
    s = Siatka()
    walec(s, DREWNO, (0, 0.25, 0), (0, -0.3, 0), 0.3, BALE * 0.7, boki=8)
    for k in range(4):
        a = k * np.pi / 2 + 0.3
        d = np.array([np.cos(a), 0, np.sin(a)])
        b = np.array([-np.sin(a), 0, np.cos(a)])  # w bok od ramienia (strona zagla)
        belka(s, DREWNO, -d * 0.2 - [0, 0.18, 0], d * 5.3 - [0, 0.18, 0], 0.18, BALE * 0.8, gora=(0, 1, 0))
        # krata z listew i plotno na niej (z jednej strony ramienia)
        for f in np.arange(1.2, 5.31, 0.55):
            p = d * f - [0, 0.2, 0]
            belka(s, DREWNO, p, p + b * 1.15, 0.06, RUSZT, gora=(0, 1, 0))
        belka(s, DREWNO, d * 1.2 + b * 1.15 - [0, 0.2, 0], d * 5.3 + b * 1.15 - [0, 0.2, 0], 0.07, RUSZT, gora=(0, 1, 0))
        rogi = [d * 1.25 + b * 0.1, d * 5.25 + b * 0.1, d * 5.25 + b * 1.1, d * 1.25 + b * 0.1 + b * 1.0]
        rogi = [r - np.array([0, 0.26, 0]) for r in rogi]
        wielokat(s, TYNK, rogi, SKRZYDLO * (0.94 + 0.04 * (k % 2)), n=np.array([0, -1.0, 0]))
        wielokat(s, TYNK, [r + np.array([0, 0.02, 0]) for r in rogi], SKRZYDLO * 0.7, n=np.array([0, 1.0, 0]))
    return s


PIEK = Bryla(-1.4, 3.2, -2.1, 2.1, 2.6, 2.2, t=0.25)
PIEC = (-3.0, 0.4)  # srodek pieca chlebowego (przybudowka -X, strona widoczna z kamery)


def piec_chlebowy(s, faza):
    """Gliniany piec w ksztalcie kopuly na kamiennym podescie, z ciemnym otworem od frontu (-Y)."""
    x, y = PIEC
    pudlo(s, KAMIEN, (x - 1.3, y - 1.3, -0.4), (x + 1.3, y + 1.2, 0.7), MUR_COKOL, bez_dolu=False)
    if faza <= 1:
        return
    pietra = [(0.7, 1.15), (0.95, 1.12), (1.2, 1.02), (1.42, 0.85), (1.6, 0.6), (1.72, 0.3)]
    n = len(pietra) if faza >= 3 else 3
    zp = 0.7
    for k, (z, r) in enumerate(pietra[:n]):
        walec(s, TYNK, (x, y, zp), (x, y, z + 0.25), r, GLINA * (0.94 + 0.03 * (k % 2)), boki=10, denka=(k == n - 1))
        zp = z + 0.25
    if faza >= 3:
        pudlo(s, TYNK, (x - 0.32, y - 1.22, 0.72), (x + 0.32, y - 1.1, 1.25), rgb(34, 28, 26))  # otwor pieca
        pudlo(s, KAMIEN, (x - 0.45, y - 1.5, 0.66), (x + 0.45, y - 1.1, 0.74), MUR * 0.9)        # polka przed otworem


def piekarnia(faza):
    """Piekarnia: bielona chata z gontem, z boku gliniany piec pod daszkiem, przed nia stol i drwa."""
    s = Siatka()
    rng = np.random.default_rng(1400)
    B = PIEK
    z0 = 0.3
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    piec_chlebowy(s, faza)
    h = {1: 1.0, 2: B.h, 3: B.h, 4: B.h}[faza]
    mury(s, B.x0, B.x1, B.y0, B.y1, h, B.t, TYNK_K, z0=z0, mat=TYNK)
    if faza >= 2:
        for x in (B.x0, B.x1 - 0.18):
            for y in (B.y0 - 0.02, B.y1 - 0.16):
                pudlo(s, DREWNO, (x, y, z0), (x + 0.18, y + 0.18, h), RYGIEL)
        pudlo(s, DREWNO, (B.x0, B.y0 - 0.04, h - 0.2), (B.x1, B.y0 + 0.02, h), RYGIEL)
        przod = (B.x0, B.y0 - 0.02, 0)
        gotowe = faza == 4
        drzwi_dom(s, przod, "y", -1, 3.0, z0, 1.1, 2.1, DRZWI if gotowe else rgb(30, 26, 24))
        okienko(s, przod, "y", -1, 1.2, z0 + 1.0, 0.7, 0.6, okiennice=gotowe)
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.5)
    if faza == 4:
        # komin z kamienia nad piecem i daszek nad piecem
        x, y = PIEC
        pudlo(s, KAMIEN, (x - 0.3, y + 0.4, 1.6), (x + 0.3, y + 1.0, 3.6), MUR_COKOL)
        for xx in (x - 1.4, x + 1.4):
            for yy in (y - 1.4, y + 1.3):
                belka(s, DREWNO, (xx, yy, -0.2), (xx, yy, 2.5), 0.14, BALE)
        plyta(s, DREWNO, [(B.x0 + 0.1, y - 1.7, 2.75), (x - 1.7, y - 1.7, 2.3), (x - 1.7, y + 1.6, 2.3), (B.x0 + 0.1, y + 1.6, 2.75)], 0.1, DESKA_SZCZYT)
        # stol z bochenkami pod oknem (drzwi wolne), drwa pod sciana, beczka z woda
        pudlo(s, DREWNO, (-1.0, B.y0 - 1.6, 0.75), (0.4, B.y0 - 0.9, 0.85), DESKA)
        for xx in (-0.9, 0.3):
            for yy in (B.y0 - 1.55, B.y0 - 0.95):
                belka(s, DREWNO, (xx, yy, 0.0), (xx, yy, 0.76), 0.07, RUSZT)
        for k in range(4):
            walec(s, TYNK, (-0.75 + k * 0.32, B.y0 - 1.25, 0.86), (-0.75 + k * 0.32, B.y0 - 1.25, 0.99), 0.13, rgb(150, 100, 58), boki=7)
        drwa(s, PIEC[0] - 1.1, PIEC[0] + 1.1, PIEC[1] + 1.3, 0.8, strona=1)
        beczka(s, B.x1 + 0.6, B.y0 - 0.6)
    if faza <= 2:
        stos_belek(s, -1.2, B.y0 - 3.0, 4.0, 5 if faza == 1 else 3)
        stos_kamieni(s, -2.6, B.y0 - 3.0, rng, 2)
    if faza == 3:
        stos_desek(s, -1.2, B.y0 - 3.0, 2.6, 4)
        drabina(s, B.x1 - 0.9, B.y0, B.h + 0.6)
    return s


# Tartak wodny: dom z desek nad brzegiem, kolo wodne z boku (-X, strona widoczna z kamery) na wale (os X),
# koryto mlynowki biegnie dalej w strone rzeki (+Y w ukladzie budowania = -Y w Unrealu).
TARTAK = Bryla(-3.0, 3.6, -2.4, 2.4, 3.0, 2.4, t=0.15)
KOLO = (-3.75, 0.0, 1.7)   # os kola (gra: RuchomeGdzie)
KOLO_R = 2.0


TARTAK3 = Bryla(-3.0, 5.8, -2.4, 2.4, 3.0, 2.4, t=0.15)


def suszarnia_desek(s, x0, y0, y1, h, dl=1.7):
    """Przybudowka z jednospadowym daszkiem od x0 w +X: deski w przekladkach na legarach."""
    x1 = x0 + dl
    for y in (y0 + 0.4, y1 - 0.4):
        belka(s, DREWNO, (x1 - 0.1, y, -0.2), (x1 - 0.1, y, 1.9), 0.14, RUSZT)
    plyta(s, DREWNO, [(x0 - 0.1, y0 + 0.1, h - 0.2), (x1 + 0.3, y0 + 0.1, 1.9), (x1 + 0.3, y1 - 0.1, 1.9), (x0 - 0.1, y1 - 0.1, h - 0.2)], 0.1, DESKA_SZCZYT)
    for j, z in enumerate((0.25, 0.65, 1.05)):
        for yy in np.arange(y0 + 0.7, y1 - 0.6, 0.55):
            pudlo(s, DREWNO, (x0 + 0.2, yy, z), (x1 - 0.3, yy + 0.4, z + 0.06), DESKA * (0.92 + 0.05 * j))
        for xx in (x0 + 0.35, x1 - 0.45):
            pudlo(s, DREWNO, (xx, y0 + 0.6, z - 0.06), (xx + 0.08, y1 - 0.6, z), RUSZT)


def trak(s, xa, xb, z0):
    """Pila ramowa (trak) z kloda na wozku, przy froncie (y = -1)."""
    for x in (xa, xb):
        belka(s, DREWNO, (x, -1.0, z0), (x, -1.0, 2.6), 0.16, BALE * 0.8)
    belka(s, DREWNO, (xa, -1.0, 2.0), (xb, -1.0, 2.0), 0.14, BALE * 0.75)
    xm = (xa + xb) / 2
    pudlo(s, DREWNO, (xm - 0.03, -1.04, 0.9), (xm + 0.03, -0.96, 2.0), rgb(88, 88, 92))
    belka(s, DREWNO, (xa - 0.9, -1.0, z0 + 0.55), (xb + 0.6, -1.0, z0 + 0.55), 0.14, RUSZT)
    walec(s, DREWNO, (xa - 1.0, -1.0, z0 + 0.8), (xb + 0.4, -1.0, z0 + 0.8), 0.22, BALE, boki=8)
    pudlo(s, TYNK, (xa - 0.9, -1.9, z0 + 0.01), (xb + 0.5, -0.2, z0 + 0.03), TROCINY)


def tartak_wodny(faza, poziom=1):
    """Poziom 1: dom z desek z kolem wodnym i jednym trakiem. 2: suszarnia desek pod daszkiem od +X, stojak z deskami.
    3 (trzy rodziny): dom dluzszy o 2,2 m z drugim trakiem, suszarnia, wywietrznik na kalenicy."""
    s = Siatka()
    B = TARTAK3 if poziom == 3 else TARTAK
    z0 = 0.4
    # podmurowka od strony rzeki wyzsza (teren opada do wody)
    pudlo(s, KAMIEN, (B.x0 - 0.1, B.y0 - 0.1, -1.5), (B.x1 + 0.1, B.y1 + 0.1, z0), MUR_COKOL, bez_dolu=False)
    if faza >= 2:
        for y in (-0.6, 0.6):  # lozyska walu kola
            pudlo(s, KAMIEN, (B.x0 - 1.5, y - 0.25, -2.5), (B.x0 - 0.1, y + 0.25, 0.4), MUR_COKOL * 0.95, bez_dolu=False)
    if faza >= 2:
        for x in np.arange(B.x0, B.x1 + 0.01, (B.x1 - B.x0) / 3):
            for y in (B.y0, B.y1):
                belka(s, DREWNO, (x, y, z0), (x, y, B.h), 0.18, BALE)
        for y in (B.y0, B.y1):
            belka(s, DREWNO, (B.x0, y, B.h - 0.09), (B.x1, y, B.h - 0.09), 0.18, BALE * 0.9)
    if faza >= 3:
        hsc = B.h if faza == 4 else 1.8
        for y, st in ((B.y1, 1),):
            for k, x in enumerate(np.arange(B.x0, B.x1 - 0.01, 0.3)):
                pudlo(s, DREWNO, (x, y, z0), (x + 0.29, y + 0.08, hsc), DESKA * (0.84 + 0.07 * (k % 3)))
        for x, st in ((B.x0, -1), (B.x1, 1)):
            for k, y in enumerate(np.arange(B.y0, B.y1 - 0.01, 0.3)):
                pudlo(s, DREWNO, (min(x, x + st * 0.08), y, z0), (max(x, x + st * 0.08), y + 0.29, hsc), DESKA * (0.82 + 0.07 * (k % 3)))
        # front (-Y) otwarty do polowy: deski tylko po lewej, po prawej wjazd na klody
        for k, x in enumerate(np.arange(B.x0, 0.0, 0.3)):
            pudlo(s, DREWNO, (x, B.y0 - 0.08, z0), (x + 0.29, B.y0, hsc), DESKA * (0.86 + 0.06 * (k % 3)))
        wiezba(s, B, kolor=BALE)
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.5)
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
    if faza == 4:
        # trak (pila ramowa) w otwartej czesci i kloda na wozku; poziom 3: drugi trak dalej w +X
        trak(s, 1.3, 2.9, z0)
        if poziom == 3:
            trak(s, 3.9, 5.3, z0)
            g = B.h + B.k
            pudlo(s, DREWNO, (1.6, -0.45, g - 0.2), (3.0, 0.45, g + 0.55), DESKA_SZCZYT)
            for st in (-1, 1):
                plyta(s, DREWNO, [(1.4, 0.0, g + 0.85), (3.2, 0.0, g + 0.85), (3.2, st * 0.7, g + 0.4), (1.4, st * 0.7, g + 0.4)], 0.06, GONT)
        if poziom >= 2:
            suszarnia_desek(s, B.x1, B.y0, B.y1, B.h)
            for k in range(5):  # stojak z deskami przy wjezdzie
                belka(s, DREWNO, (-0.2 + k * 0.3, B.y0 - 0.9, 0.0), (-0.05 + k * 0.3, B.y0 - 0.6, 2.2), 0.07, DESKA * (0.95 + 0.05 * (k % 2)))
        # wal kola i koryto mlynowki na slupach (dalej w strone rzeki, za obrysem)
        walec(s, DREWNO, (B.x0 - 1.3, KOLO[1], KOLO[2]), (B.x0, KOLO[1], KOLO[2]), 0.14, BALE * 0.7, boki=8)
        kx = KOLO[0]
        zk = KOLO[2] + KOLO_R + 0.25
        pudlo(s, DREWNO, (kx - 0.45, -0.5, zk), (kx + 0.45, 6.5, zk + 0.08), DESKA * 0.8, bez_dolu=False)
        for x in (kx - 0.5, kx + 0.45):
            pudlo(s, DREWNO, (x, -0.5, zk), (x + 0.06, 6.5, zk + 0.45), DESKA * 0.75)
        pudlo(s, TYNK, (kx - 0.42, 0.2, zk + 0.08), (kx + 0.42, 6.5, zk + 0.3), rgb(78, 104, 118))  # woda w korycie
        for y in (2.0, 4.0, 6.0):
            belka(s, DREWNO, (kx - 0.5, y, -3.0), (kx - 0.5, y, zk), 0.14, RUSZT)
            belka(s, DREWNO, (kx + 0.5, y, -3.0), (kx + 0.5, y, zk), 0.14, RUSZT)
    if faza <= 2:
        stos_belek(s, -1.0, B.y0 - 3.0, 4.5, 5 if faza == 1 else 3)
        stos_kamieni(s, 2.0, B.y0 - 3.0, np.random.default_rng(1500), 2)
    if faza == 3:
        stos_desek(s, -1.0, B.y0 - 3.0, 3.0, 4)
        drabina(s, B.x1 - 0.9, B.y0, B.h + 0.6)
    return s


def tartak_kolo():
    """Kolo wodne nasiebierne wokol osi X w (0,0,0): dwa wience, szprychy, lopatki."""
    s = Siatka()
    R = KOLO_R
    n = 12
    kat = np.linspace(0, 2 * np.pi, n, endpoint=False)
    for x in (-0.38, 0.38):
        for k in range(n):
            a, b = kat[k], kat[(k + 1) % n]
            p = np.array([x, np.cos(a) * R, np.sin(a) * R]); q = np.array([x, np.cos(b) * R, np.sin(b) * R])
            belka(s, DREWNO, p, q, 0.14, BALE * 0.78, gora=(1, 0, 0))
        for k in range(0, n, 2):
            a = kat[k]
            belka(s, DREWNO, (x, 0, 0), (x, np.cos(a) * R * 0.97, np.sin(a) * R * 0.97), 0.1, BALE * 0.72, gora=(1, 0, 0))
    walec(s, DREWNO, (-0.5, 0, 0), (0.5, 0, 0), 0.22, BALE * 0.65, boki=8)
    for k in range(n):
        a = kat[k] + np.pi / n
        r0, r1 = R * 0.8, R * 1.06
        c, sn = np.cos(a), np.sin(a)
        pts = [(-0.42, c * r0, sn * r0), (0.42, c * r0, sn * r0), (0.42, c * r1, sn * r1), (-0.42, c * r1, sn * r1)]
        nrm = np.array([0, -sn, c])
        wielokat(s, DREWNO, pts, DESKA * (0.8 + 0.06 * (k % 2)), n=nrm)
        wielokat(s, DREWNO, pts, DESKA * 0.6, n=-nrm)
    return s


def stos_zboza(poziom):
    """Snopy zboza ustawione w mendle (stojace snopki): 2 / 4 / 7 / 10."""
    s = Siatka()
    rng = np.random.default_rng(1600 + poziom)
    miejsca = [(-0.4, 0.0), (0.2, 0.05), (-0.1, -0.55), (0.55, -0.45), (-0.8, -0.45), (0.85, 0.2), (-0.75, 0.35),
               (0.3, 0.6), (-0.3, 0.6), (1.1, -0.3)]
    for x, y in miejsca[:{1: 2, 2: 4, 3: 7, 4: 10}[poziom]]:
        k = ZLOTE * rng.uniform(0.9, 1.05)
        walec(s, STRZECHA, (x, y, 0.0), (x, y, 0.55), 0.2, k, boki=7, denka=False)
        walec(s, STRZECHA, (x, y, 0.5), (x, y, 0.62), 0.12, k * 0.85, boki=7, denka=False)   # przewiazka
        walec(s, STRZECHA, (x, y, 0.6), (x, y, 0.95), 0.24, k * 1.04, boki=7, denka=True)    # klosy
    return s


def stos_maki(poziom):
    """Worki z maka: 2 / 4 / 6 / 9."""
    s = Siatka()
    rng = np.random.default_rng(1700 + poziom)
    miejsca = [(-0.4, 0.0, 0, False), (0.2, 0.05, 0, False), (-0.1, -0.55, 0, False), (0.55, -0.45, 0, False),
               (-0.1, -0.2, 0.5, True), (-0.85, -0.4, 0, False), (0.85, 0.15, 0, False), (0.6, -0.1, 0.5, True), (-0.7, 0.3, 0, False)]
    for x, y, z, lezy in miejsca[:{1: 2, 2: 4, 3: 6, 4: 9}[poziom]]:
        worek(s, x, y, z=z, lezy=lezy, kolor=MAKA_W * rng.uniform(0.92, 1.02))
    return s


UPRAWY = ["Zaorane", "Zasiane", "Rosnie", "Dojrzale", "Scierniska"]


# --- owce, tkacz, kuznia (etap 4 ekonomii, 21.11.2026): owczarnia, chata tkacza, kopalnia rudy, kuznia, zbrojownia ---
RUNO = rgb(222, 214, 194)      # welna na owcy (przygaszona biel)
RUNO_C = rgb(196, 186, 164)
OWCA_SKORA = rgb(196, 170, 156) # ostrzyzona owca
OWCA_LEB = rgb(62, 54, 48)
SIANO = rgb(176, 160, 104)
RUDA = rgb(122, 72, 52)         # ruda darniowa / hematyt: rdzawy kamien
RUDA_J = rgb(150, 96, 66)
SKALA_R = rgb(116, 96, 84)      # skala z zylami rudy
ZAR = rgb(255, 150, 70)         # zar w palenisku
SUKNA = [rgb(112, 104, 128), rgb(126, 98, 76), rgb(98, 112, 92), rgb(150, 136, 112)]
CZERWIEN = rgb(150, 52, 44)     # tarcza nad drzwiami zbrojowni


def owca(strzyzona=False):
    """Owca (poczatek: grunt pod srodkiem, przodem do +X), ~1,1 m dlugosci."""
    s = Siatka()
    r = 0.21 if strzyzona else 0.3
    z = 0.52 if strzyzona else 0.56
    k = OWCA_SKORA if strzyzona else RUNO
    walec(s, TYNK, (-0.42, 0, z), (0.36, 0, z), r, k, boki=8)
    walec(s, TYNK, (-0.46, 0, z), (-0.42, 0, z), r * 0.8, k * 0.95, boki=8)
    if not strzyzona:
        for x in (-0.25, 0.05):  # kepy runa na grzbiecie
            walec(s, TYNK, (x, 0, z + r * 0.7), (x + 0.22, 0, z + r * 0.7), r * 0.5, RUNO_C, boki=6)
    pudlo(s, TYNK, (0.34, -0.1, z - 0.02), (0.6, 0.1, z + 0.2), OWCA_LEB)          # leb
    pudlo(s, TYNK, (0.52, -0.07, z - 0.08), (0.68, 0.07, z + 0.08), OWCA_LEB * 1.1) # pysk
    for y in (-0.14, 0.14):
        belka(s, TYNK, (0.42, y * 0.8, z + 0.16), (0.42, y * 1.6, z + 0.12), 0.05, OWCA_LEB)  # uszy
    for x in (-0.3, 0.24):
        for y in (-0.12, 0.12):
            belka(s, TYNK, (x, y, 0.0), (x, y, z - r * 0.6), 0.07, OWCA_LEB)
    return s


def owca_strzyzona():
    return owca(True)


def sciana_desek(s, x0, y0, x1, y1, h, z0=0.0, kolor=DESKA):
    """Pionowe deski od (x0, y0) do (x1, y1) (sciana wzdluz X albo Y)."""
    L = np.hypot(x1 - x0, y1 - y0)
    n = max(1, int(L / 0.3))
    for k in range(n):
        a, b = k / n, (k + 1) / n
        xa, ya = x0 + (x1 - x0) * a, y0 + (y1 - y0) * a
        xb, yb = x0 + (x1 - x0) * b, y0 + (y1 - y0) * b
        pudlo(s, DREWNO, (min(xa, xb) - 0.04, min(ya, yb) - 0.04, z0), (max(xa, xb) + 0.04, max(ya, yb) + 0.04, h), kolor * (0.84 + 0.07 * (k % 3)))


OWCZ = Bryla(-6.0, -1.6, 1.0, 4.0, 2.2, 1.9, t=0.15)  # szopa z tylu po lewej, przodem (-Y) otwarta
OWCZ_PLOT = (-6.6, 6.6, -5.0, 4.4)


def owczarnia(faza):
    """Owczarnia: niska szopa pod strzecha (front otwarty), ogrodzone pastwisko z pasnikiem i korytem."""
    s = Siatka()
    B = OWCZ
    x0, x1, y0, y1 = OWCZ_PLOT
    if faza == 1:
        stos_belek(s, -2.5, y0 - 1.6, 4.0, 5)
        stos_desek(s, 2.5, y0 - 1.6, 3.0, 3)
        klepisko(s, B.x0, B.x1, B.y0, B.y1, ZIEMIA * 0.9)
        return s
    klepisko(s, B.x0 - 0.2, B.x1 + 0.2, B.y0 - 0.4, B.y1 + 0.2, ZIEMIA * 0.9)
    hs = B.h
    for x in (B.x0, (B.x0 + B.x1) / 2, B.x1):
        for y in (B.y0, B.y1):
            belka(s, DREWNO, (x, y, -0.2), (x, y, hs), 0.16, BALE)
    for y in (B.y0, B.y1):
        belka(s, DREWNO, (B.x0 - 0.1, y, hs - 0.08), (B.x1 + 0.1, y, hs - 0.08), 0.16, BALE * 0.9)
    # plot pastwiska stawia gra (segmenty SM_Plot wzdluz zaznaczonego prostokata)
    if faza >= 3:
        sciana_desek(s, B.x0, B.y1, B.x1, B.y1, hs)          # tyl
        sciana_desek(s, B.x0, B.y0, B.x0, B.y1, hs)          # bok -X
        sciana_desek(s, B.x1, B.y0 + 1.4, B.x1, B.y1, hs)    # bok +X (z przejsciem)
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
        dach_slomiany(s, B, 0, 0.5 if faza == 3 else 1.0)
    if faza == 4:
        # pasnik z sianem i koryto z woda, wiazka siana w szopie
        px, py = 2.4, 2.8
        for x in (px - 1.0, px + 1.0):
            belka(s, DREWNO, (x, py - 0.4, 0.0), (x, py, 1.3), 0.1, RUSZT)
            belka(s, DREWNO, (x, py + 0.4, 0.0), (x, py, 1.3), 0.1, RUSZT)
        belka(s, DREWNO, (px - 1.1, py, 1.3), (px + 1.1, py, 1.3), 0.1, BELKA)
        for k in range(7):
            x = px - 0.9 + k * 0.3
            belka(s, DREWNO, (x, py - 0.35, 0.55), (x, py, 1.28), 0.04, RUSZT)
            belka(s, DREWNO, (x, py + 0.35, 0.55), (x, py, 1.28), 0.04, RUSZT)
        pudlo(s, STRZECHA, (px - 0.85, py - 0.22, 0.6), (px + 0.85, py + 0.22, 1.1), SIANO)
        pudlo(s, DREWNO, (4.2, -0.3, 0.0), (5.8, 0.3, 0.4), DESKA * 0.8, bez_dolu=False)        # koryto
        pudlo(s, TYNK, (4.28, -0.22, 0.4), (5.72, 0.22, 0.42), rgb(78, 104, 118))
        pudlo(s, STRZECHA, (-5.6, 2.6, 0.0), (-4.2, 3.6, 0.7), SIANO * 0.95)
        pudlo(s, STRZECHA, (-4.0, 3.0, 0.0), (-3.0, 3.7, 0.5), SIANO * 0.9)
    if faza == 2:
        stos_belek(s, -2.5, y0 - 1.6, 4.0, 3)
        stos_desek(s, 2.5, y0 - 1.6, 3.0, 4)
    if faza == 3:
        drabina(s, B.x1 - 0.6, B.y0, B.h + 0.6)
    return s


TKACZ = Bryla(-2.0, 3.2, -2.2, 2.2, 2.6, 2.6, t=0.3)
KROSNO_X = -2.45  # krosno pod wiata po stronie -X (widac je z kamery)


def krosno(s, x, y0, y1, gotowe=True):
    """Krosno pionowe: dwie stojki, belki gora/dol, osnowa z kolorowych nici, nawiniete sukno na dole."""
    for y in (y0, y1):
        belka(s, DREWNO, (x, y, 0.0), (x, y, 1.9), 0.1, BELKA * 0.9)
        belka(s, DREWNO, (x - 0.5, y, 0.0), (x, y, 0.9), 0.07, RUSZT)  # podpora
    belka(s, DREWNO, (x, y0 - 0.05, 1.8), (x, y1 + 0.05, 1.8), 0.1, BELKA)
    belka(s, DREWNO, (x, y0 - 0.05, 0.5), (x, y1 + 0.05, 0.5), 0.1, BELKA)
    if gotowe:
        pudlo(s, TYNK, (x - 0.02, y0 + 0.1, 0.55), (x + 0.02, y1 - 0.1, 1.75), RUNO * 0.92)          # osnowa
        pudlo(s, TYNK, (x - 0.03, y0 + 0.1, 0.55), (x + 0.03, y1 - 0.1, 0.95), SUKNA[0])             # utkane
        walec(s, TYNK, (x - 0.1, y0 + 0.1, 0.42), (x - 0.1, y1 - 0.1, 0.42), 0.09, SUKNA[0] * 0.9, boki=7)


def chata_tkacza(faza):
    """Chata tkacza: zrab pod strzecha, przy scianie -X wiata z krosnem, przed chata motki welny na zerdzi."""
    s = Siatka()
    rng = np.random.default_rng(1800)
    B = TKACZ
    z0 = 0.3
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    h = {1: 1.2, 2: B.h, 3: B.h, 4: B.h}[faza]
    zrab(s, B.x0, B.x1, B.y0, B.y1, h, z0, rng)
    przod = (B.x0, B.y0 - 0.02, 0)
    if faza >= 2:
        gotowe = faza == 4
        drzwi_dom(s, przod, "y", -1, 3.0, z0, 1.1, 2.1, DRZWI if gotowe else rgb(30, 26, 24))
        okienko(s, przod, "y", -1, 1.0, z0 + 1.0, 0.7, 0.6, okiennice=gotowe)
        # wiata przy scianie -X
        wx = B.x0 - 2.8
        for y in (-1.8, 1.8):
            belka(s, DREWNO, (wx, y, -0.2), (wx, y, 2.0), 0.14, BALE)
        # ziemia w grze (ABudowa::Podloze): klepisko(s, wx - 0.2, B.x0, -2.0, 2.0, ZIEMIA * 0.9)
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
        dach_slomiany(s, B, 0, 0.5 if faza == 3 else 1.0)
    if faza == 4:
        wx = B.x0 - 2.8
        plyta(s, DREWNO, [(B.x0 + 0.05, -2.1, 2.55), (wx - 0.3, -2.1, 2.05), (wx - 0.3, 2.1, 2.05), (B.x0 + 0.05, 2.1, 2.55)], 0.1, DESKA_SZCZYT)
        krosno(s, KROSNO_X, -0.9, 0.9)
        # motki welny i sukna na zerdzi przed chata
        for x in (1.4, 3.0):
            belka(s, DREWNO, (x, B.y0 - 1.0, 0.0), (x, B.y0 - 1.0, 1.6), 0.08, RUSZT)
        belka(s, DREWNO, (1.3, B.y0 - 1.0, 1.55), (3.1, B.y0 - 1.0, 1.55), 0.06, BELKA)
        for k in range(4):
            x = 1.6 + k * 0.38
            pudlo(s, TYNK, (x - 0.1, B.y0 - 1.06, 0.9), (x + 0.1, B.y0 - 0.94, 1.52), (RUNO if k % 2 == 0 else SUKNA[k % 4]) * 0.95)
        kosz(s, -1.5, B.y0 - 0.8, 0.3, 0.4, zaw=RUNO)
    if faza <= 2:
        stos_belek(s, 0.0, B.y0 - 3.0, 4.0, 5 if faza == 1 else 3)
    if faza == 3:
        stos_desek(s, 0.0, B.y0 - 3.0, 2.6, 4)
        drabina(s, B.x1 - 0.9, B.y0, B.h + 0.6)
    return s


def kopalnia_rudy(faza):
    """Kopalnia rudy: wzgorze z rdzawymi zylami, sztolnia w ramie z bali, halda, wiata na narzedzia; gornicy kuja rude."""
    s = Siatka()
    rng = np.random.default_rng(1900)
    # ziemia w grze (ABudowa::Podloze): klepisko(s, -5.2, 5.2, -4.2, 1.2, ZIEMIA * 0.7 + RUDA * 0.3)
    skala(s, -4.6, 4.6, 0.6 if faza == 1 else 1.0, 4.4, 3.6, np.random.default_rng(1901), kolor=SKALA_R)
    for k in range(8):  # zyly rudy na licu
        x = rng.uniform(-4.2, 4.2)
        if abs(x) < 1.1:
            continue
        z = rng.uniform(0.4, 1.8)
        pudlo(s, TYNK, (x - 0.4, 0.95, z), (x + 0.4, 1.02, z + 0.18), RUDA * rng.uniform(0.9, 1.1))
    for k in range(12):
        x, y = rng.uniform(-4.5, 4.5), rng.uniform(-3.8, 0.4)
        pudlo(s, KAMIEN, (x - 0.12, y - 0.1, 0.0), (x + 0.12, y + 0.1, 0.12), RUDA_J * rng.uniform(0.85, 1.05), bez_dolu=False)
    if faza >= 2:
        # sztolnia: ciemny otwor i rama z bali
        pudlo(s, TYNK, (-0.65, 0.92, 0.0), (0.65, 1.6, 2.0), rgb(26, 22, 20))
        hs = 2.15 if faza >= 3 else 1.4
        for x in (-0.8, 0.8):
            belka(s, DREWNO, (x, 0.9, -0.1), (x, 0.9, hs), 0.2, BALE * 0.85)
        if faza >= 3:
            belka(s, DREWNO, (-1.05, 0.9, hs), (1.05, 0.9, hs), 0.22, BALE * 0.8)
    if faza == 1:
        stos_belek(s, -2.0, -3.0, 4.0, 5)
        kozly(s, 2.4, -3.0)
    # wiata na narzedzia po prawej (+X) jak w kamieniolomie
    wx0, wx1, wy0, wy1 = 2.4, 4.8, -3.9, -1.9
    if faza >= 2:
        for x in (wx0, wx1):
            belka(s, DREWNO, (x, wy0, -0.2), (x, wy0, 2.2), 0.14, BALE)
            belka(s, DREWNO, (x, wy1, -0.2), (x, wy1, 2.6), 0.14, BALE)
    if faza == 2:
        stos_desek(s, -1.0, -3.0, 2.6, 3)
    if faza >= 3:
        daszek = [(wx0 - 0.35, wy0 - 0.35, 2.3), (wx1 + 0.35, wy0 - 0.35, 2.3), (wx1 + 0.35, wy1 + 0.35, 2.75), (wx0 - 0.35, wy1 + 0.35, 2.75)]
        if faza == 3:
            daszek = [daszek[0], ((wx0 + wx1) / 2, wy0 - 0.35, 2.3), ((wx0 + wx1) / 2, wy1 + 0.35, 2.75), daszek[3]]
        plyta(s, DREWNO, daszek, 0.1, DESKA_SZCZYT)
    if faza == 4:
        # halda plonej skaly po lewej, taczka, kosze, kaganek przy wejsciu
        for k in range(14):
            x, y = rng.uniform(-4.6, -2.6), rng.uniform(-1.6, 0.4)
            pudlo(s, KAMIEN, (x - 0.3, y - 0.25, 0.0), (x + 0.3, y + 0.25, rng.uniform(0.2, 0.7)), SKALA_R * rng.uniform(0.85, 1.05), bez_dolu=False)
        taczka(s, 1.5, -1.4)
        kosz(s, wx0 + 0.6, wy1 - 0.5, 0.3, 0.4, zaw=RUDA)
        beczka(s, wx1 - 0.5, wy1 - 0.5)
        belka(s, DREWNO, (1.2, 0.8, 0.0), (1.2, 0.8, 1.6), 0.06, RUSZT)
        pudlo(s, TYNK, (1.12, 0.72, 1.6), (1.28, 0.88, 1.78), ZAR * 0.8)
    return s


KUZ = Bryla(-2.6, 3.0, -2.2, 2.2, 2.6, 2.4, t=0.25)


def kuznia(faza):
    """Kuznia: wiata z kamienna sciana z tylu i z prawej, otwarta od przodu i z lewej (widac palenisko i kowadlo z kamery);
    murowane palenisko z okapem i kominem, miech, kowadlo na pniu, beczka do hartowania, stojak na wlocznie."""
    s = Siatka()
    B = KUZ
    # ziemia w grze (ABudowa::Podloze): klepisko(s, B.x0 - 0.4, B.x1 + 0.4, B.y0 - 0.6, B.y1 + 0.3, ZIEMIA * 0.8)
    hm = {1: 0.9, 2: B.h, 3: B.h, 4: B.h}[faza]
    pudlo(s, KAMIEN, (B.x0, B.y1 - 0.5, 0.0), (B.x1, B.y1, hm), MUR * 0.9, bez_dolu=False)          # tyl
    pudlo(s, KAMIEN, (B.x1 - 0.5, B.y0, 0.0), (B.x1, B.y1 - 0.5, hm), MUR * 0.88, bez_dolu=False)    # bok +X
    # palenisko z tylu po lewej
    pudlo(s, KAMIEN, (-2.3, 0.4, 0.0), (-0.7, B.y1 - 0.5, 0.85), MUR_COKOL, bez_dolu=False)
    if faza >= 2:
        for x, y in ((B.x0 + 0.1, B.y0 + 0.1), (0.2, B.y0 + 0.1), (B.x0 + 0.1, 0.0)):
            belka(s, DREWNO, (x, y, -0.2), (x, y, B.h), 0.2, BALE)
        belka(s, DREWNO, (B.x0, B.y0 + 0.1, B.h - 0.1), (B.x1, B.y0 + 0.1, B.h - 0.1), 0.2, BALE * 0.85)
        belka(s, DREWNO, (B.x0 + 0.1, B.y0, B.h - 0.1), (B.x0 + 0.1, B.y1, B.h - 0.1), 0.2, BALE * 0.85)
        # okap i komin nad paleniskiem
        pudlo(s, KAMIEN, (-2.2, 0.6, 1.6), (-0.8, B.y1 - 0.5, 2.0), MUR_COKOL * 0.95)
        pudlo(s, KAMIEN, (-1.8, 1.0, 2.0), (-1.2, 1.6, B.h + B.k + (0.8 if faza >= 3 else -0.6)), MUR_COKOL * 0.9)
    if faza >= 3:
        szczyt(s, B.x1 - 0.12, B.y0, B.y1, B.h, B.k, 0.12, MUR * 0.9)
        szczyt(s, B.x0, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.5)
    if faza == 4:
        pudlo(s, TYNK, (-2.0, 0.7, 0.85), (-1.0, 1.5, 0.92), ZAR)                       # zar
        pudlo(s, DREWNO, (-0.65, 1.1, 0.4), (-0.15, 1.9, 0.75), SKORA_Z)                # miech
        belka(s, DREWNO, (-0.4, 1.0, 0.75), (-0.4, 0.4, 1.2), 0.05, RUSZT)
        walec(s, DREWNO, (0.2, -0.4, 0.0), (0.2, -0.4, 0.5), 0.3, BALE * 0.8, boki=8)  # pien
        pudlo(s, KAMIEN, (-0.05, -0.55, 0.5), (0.5, -0.25, 0.78), rgb(72, 72, 76))      # kowadlo
        pudlo(s, KAMIEN, (0.5, -0.5, 0.62), (0.72, -0.3, 0.74), rgb(72, 72, 76))
        beczka(s, 1.3, 0.9)
        # stojak z wloczniami przy wejsciu i wegiel drzewny w koszu
        _stojak_wloczni(s, 1.8, B.y0 - 0.5, 4)
        kosz(s, -2.3, -0.6, 0.32, 0.4, zaw=rgb(34, 30, 28))
    if faza <= 2:
        stos_kamieni(s, -1.0, B.y0 - 2.6, np.random.default_rng(1950), 3)
        stos_belek(s, 1.8, B.y0 - 2.6, 3.5, 5 if faza == 1 else 3)
    if faza == 3:
        stos_desek(s, 0.0, B.y0 - 2.6, 2.6, 4)
        drabina(s, B.x1 - 1.0, B.y0, B.h + 0.6)
    return s


def _wlocznia(s, x, y, z0, x2, y2, z2):
    """Wlocznia od (x, y, z0) do grotu w (x2, y2, z2)."""
    p, q = np.array([x, y, z0]), np.array([x2, y2, z2])
    d = (q - p) / np.linalg.norm(q - p)
    belka(s, DREWNO, p, q - d * 0.25, 0.05, BALE * 0.9)
    belka(s, DREWNO, q - d * 0.27, q, 0.07, rgb(96, 96, 102))


def _stojak_wloczni(s, x, y, n):
    """Stojak: dwa slupki z poprzeczka, n wloczni opartych grotami do gory."""
    for dx in (-0.8, 0.8):
        belka(s, DREWNO, (x + dx, y, 0.0), (x + dx, y, 1.3), 0.08, RUSZT)
    belka(s, DREWNO, (x - 0.9, y, 1.25), (x + 0.9, y, 1.25), 0.07, BELKA)
    for k in range(n):
        u = -0.65 + 1.3 * (k + 0.5) / max(n, 1)
        _wlocznia(s, x + u, y - 0.35, 0.0, x + u, y + 0.08, 2.3)


ZBR = Bryla(-3.6, 3.6, -2.4, 2.4, 3.2, 3.0, t=0.3)


def manekin(s, x, y):
    """Slomiany manekin do cwiczen: pal, poprzeczka, worek ze sloma."""
    belka(s, DREWNO, (x, y, -0.2), (x, y, 1.8), 0.12, RUSZT)
    belka(s, DREWNO, (x - 0.5, y, 1.4), (x + 0.5, y, 1.4), 0.08, RUSZT)
    walec(s, STRZECHA, (x, y, 0.8), (x, y, 1.55), 0.26, SIANO, boki=7)
    pudlo(s, STRZECHA, (x - 0.13, y - 0.13, 1.6), (x + 0.13, y + 0.13, 1.9), SIANO * 0.95)


def zbrojownia(faza):
    """Zbrojownia: zrab na kamiennym cokole, gont, waskie okienka, tarcza nad drzwiami; przed nia plac cwiczen z manekinami."""
    s = Siatka()
    rng = np.random.default_rng(2000)
    B = ZBR
    z0 = 0.5
    pudlo(s, KAMIEN, (B.x0 - 0.1, B.y0 - 0.1, -0.4), (B.x1 + 0.1, B.y1 + 0.1, z0), MUR_COKOL, bez_dolu=False)
    h = {1: 1.2, 2: B.h, 3: B.h, 4: B.h}[faza]
    zrab(s, B.x0, B.x1, B.y0, B.y1, h, z0, rng)
    przod = (B.x0, B.y0 - 0.02, 0)
    if faza >= 2:
        gotowe = faza == 4
        drzwi_dom(s, przod, "y", -1, 3.6, z0, 1.4, 2.2, DRZWI if gotowe else rgb(30, 26, 24))
        for u in (1.4, 5.8):
            pudlo(s, DREWNO, (B.x0 + u - 0.15, B.y0 - 0.06, z0 + 1.4), (B.x0 + u + 0.15, B.y0 + 0.02, z0 + 2.1), OKNO)
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.55)
    if faza == 4:
        # tarcza nad drzwiami (czerwona z jasnym pasem), schodek, plac cwiczen
        walec(s, TYNK, (0, B.y0 - 0.05, z0 + 2.75), (0, B.y0 - 0.12, z0 + 2.75), 0.42, CZERWIEN, boki=10)
        pudlo(s, TYNK, (-0.08, B.y0 - 0.14, z0 + 2.35), (0.08, B.y0 - 0.12, z0 + 3.15), TYNK_K)
        pudlo(s, KAMIEN, (-0.9, B.y0 - 0.7, 0.0), (0.9, B.y0 - 0.05, z0 - 0.1), MUR_COKOL)
        # ziemia w grze (ABudowa::Podloze): klepisko(s, -4.2, 4.2, -9.0, B.y0 - 0.8, ZIEMIA * 0.85)
        for x in (-2.6, 0.0, 2.6):
            manekin(s, x, -8.2)
        _stojak_wloczni(s, B.x1 + 0.9, B.y0 - 1.8, 3)
        beczka(s, B.x0 - 0.6, B.y0 - 0.8)
    if faza <= 2:
        stos_belek(s, -1.5, B.y0 - 3.0, 4.5, 5 if faza == 1 else 3)
        stos_kamieni(s, 2.0, B.y0 - 3.0, rng, 3)
    if faza == 3:
        stos_desek(s, -1.5, B.y0 - 3.0, 3.0, 4)
        drabina(s, B.x1 - 1.0, B.y0, B.h + 0.6)
    return s


def stos_welny(poziom):
    """Runa welny na kupce: 2 / 4 / 6 / 9."""
    s = Siatka()
    rng = np.random.default_rng(2100 + poziom)
    miejsca = [(-0.35, 0.0, 0), (0.3, 0.05, 0), (-0.05, -0.5, 0), (0.6, -0.45, 0), (0.0, -0.15, 0.3), (-0.8, -0.4, 0),
               (0.85, 0.2, 0), (-0.5, -0.3, 0.3), (0.45, -0.25, 0.3)]
    for x, y, z in miejsca[:{1: 2, 2: 4, 3: 6, 4: 9}[poziom]]:
        k = RUNO * rng.uniform(0.9, 1.02)
        walec(s, TYNK, (x, y, z), (x, y, z + 0.3), 0.3, k, boki=7)
        walec(s, TYNK, (x, y, z + 0.3), (x, y, z + 0.36), 0.2, k * 0.95, boki=7)
    return s


def stos_sukna(poziom):
    """Bele sukna na legarach: 2 / 3 / 4 / 6."""
    s = Siatka()
    for x in (-0.6, 0.6):
        pudlo(s, DREWNO, (x - 0.07, -0.55, 0.0), (x + 0.07, 0.55, 0.1), RUSZT)
    miejsca = [(-0.3, 0), (0.0, 0), (0.3, 0), (-0.15, 1), (0.15, 1), (0.0, 2)]
    for k, (y, w) in enumerate(miejsca[:{1: 2, 2: 3, 3: 4, 4: 6}[poziom]]):
        z = 0.1 + 0.14 + w * 0.25
        walec(s, TYNK, (-0.85, y, z), (0.85, y, z), 0.14, SUKNA[k % 4], boki=8)
    return s


def stos_rudy(poziom):
    """Kupa rudy (rdzawe bryly): 5 / 9 / 14 / 20 bryl."""
    s = Siatka()
    rng = np.random.default_rng(2200 + poziom)
    n = {1: 5, 2: 9, 3: 14, 4: 20}[poziom]
    R = 0.45 + 0.15 * poziom
    for k in range(n):
        a, r = rng.uniform(0, 2 * np.pi), R * np.sqrt(rng.uniform())
        x, y = np.cos(a) * r, np.sin(a) * r
        z = max(0.0, (R - r) * 0.55 + rng.uniform(-0.1, 0.05))
        w = rng.uniform(0.14, 0.24)
        pudlo(s, KAMIEN, (x - w, y - w * 0.8, z), (x + w, y + w * 0.8, z + w * 1.2), (RUDA if k % 3 else RUDA_J) * rng.uniform(0.85, 1.08), bez_dolu=False)
    return s


def stos_broni(poziom):
    """Stojak z wloczniami: 2 / 4 / 7 / 10."""
    s = Siatka()
    _stojak_wloczni(s, 0.0, 0.0, {1: 2, 2: 4, 3: 7, 4: 10}[poziom])
    return s


# --- Etap 5 (21.11.2026): targ - plac z kramami pod plociennymi daszkami i waga; towar na stosach z przodu ---
TARG_KRAMY = (-4.0, 0.0, 4.0)
TARG_DASZKI = [rgb(122, 54, 44), rgb(168, 144, 94), rgb(72, 88, 116)]  # przygaszone, jak reszta palety


def kram(s, x, y0, k, faza):
    """Kram: cztery slupki, lada z przodu (-Y, do kamery), plocienny daszek opadajacy do przodu, towar na ladzie."""
    y1 = y0 + 1.7
    for xx in (x - 1.3, x + 1.3):
        belka(s, DREWNO, (xx, y0, -0.2), (xx, y0, 2.0), 0.09, BALE)
        belka(s, DREWNO, (xx, y1, -0.2), (xx, y1, 2.3), 0.09, BALE)
    if faza < 3:
        return
    pudlo(s, DREWNO, (x - 1.2, y0 + 0.05, 0.0), (x + 1.2, y0 + 0.65, 0.85), DESKA * 0.9, bez_dolu=False)
    d = 0.5 if faza == 3 else 1.0
    yt, zt = y1 + 0.25, 2.35
    yp, zp = y0 - 0.45, 1.9
    yk, zk = yt + (yp - yt) * d, zt + (zp - zt) * d
    kol = TARG_DASZKI[k % len(TARG_DASZKI)]
    plyta(s, TYNK, [(x - 1.5, yt, zt), (x + 1.5, yt, zt), (x + 1.5, yk, zk), (x - 1.5, yk, zk)], 0.05, kol)
    if faza == 4:
        # pas na brzegu daszka i towar na ladzie
        pudlo(s, TYNK, (x - 1.5, yp - 0.02, zp - 0.25), (x + 1.5, yp + 0.03, zp), kol * 0.7)
        if k == 0:
            for j in range(3):
                pudlo(s, TYNK, (x - 0.9 + j * 0.6, y0 + 0.15, 0.85), (x - 0.5 + j * 0.6, y0 + 0.55, 1.0), SUKNA[j % 4])
        elif k == 1:
            kosz(s, x - 0.6, y0 + 0.35, 0.22, 0.3, z=0.85)
            kosz(s, x + 0.1, y0 + 0.35, 0.22, 0.3, zaw=RUNO, z=0.85)
            beczka(s, x + 1.7, y0 + 0.6)
        else:
            for j in range(4):
                pudlo(s, KAMIEN, (x - 1.0 + j * 0.5, y0 + 0.2, 0.85), (x - 0.7 + j * 0.5, y0 + 0.5, 0.97), rgb(96, 96, 102))


def waga(s, x, y):
    """Waga kupiecka: slup, belka z dwiema szalami na sznurach."""
    belka(s, DREWNO, (x, y, -0.2), (x, y, 2.2), 0.1, BALE)
    belka(s, DREWNO, (x - 0.9, y, 2.05), (x + 0.9, y, 2.05), 0.06, BELKA)
    for xx in (x - 0.85, x + 0.85):
        belka(s, DREWNO, (xx, y, 2.0), (xx, y, 1.05), 0.012, RUSZT)
        walec(s, KAMIEN, (xx, y, 1.0), (xx, y, 1.05), 0.28, rgb(150, 120, 70), boki=10)
    pudlo(s, KAMIEN, (x + 0.7, y - 0.1, 1.05), (x + 0.95, y + 0.1, 1.2), rgb(80, 80, 84))


def targ(faza):
    """Targ: ubity plac, trzy kramy z tylu (+Y w generatorze = dalej od kamery), waga na srodku; przod wolny na stosy towaru."""
    s = Siatka()
    if faza == 1:
        for x, y in ((-6.0, -5.0), (6.0, -5.0), (-6.0, 4.6), (6.0, 4.6)):
            belka(s, DREWNO, (x, y, -0.2), (x, y, 1.2), 0.12, RUSZT)
        stos_belek(s, -2.5, -6.0, 4.0, 5)
        stos_desek(s, 2.5, -6.0, 3.0, 4)
        return s
    # ziemia w grze (ABudowa::Podloze): klepisko(s, -6.2, 6.2, -5.2, 4.8, ZIEMIA * 0.9)
    for k, x in enumerate(TARG_KRAMY):
        kram(s, x, 2.0, k, faza)
    if faza >= 3:
        waga(s, 0.0, 1.3)
    if faza == 2:
        stos_belek(s, -2.5, -6.0, 4.0, 3)
        stos_desek(s, 2.5, -6.0, 3.0, 4)
    if faza == 3:
        stos_desek(s, 0.0, -6.0, 3.0, 3)
    return s




# --- Paczka A (23.11.2026): chata opalowa i chata rybaka; stosy drewna opalowego i ryb ---
POLANO = rgb(132, 100, 68)
POLANO_CZOLO = rgb(204, 174, 128)
SIEC = rgb(118, 112, 96)
RYBA = rgb(138, 144, 140)


def sagi(s, x0, x1, y0, y1, h, rng):
    """Sagi polan: drewno opalowe ulozone czolami do przodu (-Y), warstwami."""
    for z in np.arange(0.0, h, 0.16):
        for x in np.arange(x0, x1 - 0.1, 0.17):
            k = rng.uniform(0.88, 1.05)
            pudlo(s, DREWNO, (x, y0, z), (x + 0.15, y1, z + 0.15), POLANO * k)
            pudlo(s, DREWNO, (x + 0.01, y0 - 0.01, z + 0.01), (x + 0.14, y0, z + 0.14), POLANO_CZOLO * k)


OPAL = Bryla(-3.0, 1.0, -1.9, 1.9, 2.3, 2.3, t=0.28)


def chata_opalowa(faza):
    """Chata opalowa: mala chata z bali, przy niej wiata (+X) z sagami polan, z przodu pniak z siekiera i koziol."""
    s = Siatka()
    rng = np.random.default_rng(3300)
    B = OPAL
    z0 = 0.3
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    h = {1: 1.1, 2: B.h, 3: B.h, 4: B.h}[faza]
    zrab(s, B.x0, B.x1, B.y0, B.y1, h, z0, rng)
    przod = (B.x0, B.y0 - 0.02, 0)
    if faza >= 2:
        gotowe = faza == 4
        drzwi_dom(s, przod, "y", -1, 1.6, z0, 1.1, 2.1, DRZWI if gotowe else rgb(30, 26, 24))
        # wiata: slupy i daszek jednospadowy od sciany chaty w +X
        for y in (B.y0 + 0.2, B.y1 - 0.2):
            belka(s, DREWNO, (B.x1 + 2.4, y, -0.2), (B.x1 + 2.4, y, 1.9), 0.12, RUSZT)
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.45)
        plyta(s, DREWNO, [(B.x1, B.y0 - 0.3, B.h - 0.1), (B.x1 + 2.7, B.y0 - 0.3, 2.0), (B.x1 + 2.7, B.y1 + 0.3, 2.0),
                          (B.x1, B.y1 + 0.3, B.h - 0.1)], 0.1, DESKA_SZCZYT)
    if faza == 4:
        sagi(s, B.x1 + 0.3, B.x1 + 2.2, -1.2, -0.75, 1.6, rng)
        sagi(s, B.x1 + 0.3, B.x1 + 2.2, 0.3, 0.75, 1.3, rng)
        pieniek(s, -0.6, B.y0 - 1.5)
        for k in range(6):  # rozrzucone polana przy pniaku
            x, y = -0.6 + rng.uniform(-0.8, 0.8), B.y0 - 1.5 + rng.uniform(-0.7, 0.5)
            pudlo(s, DREWNO, (x - 0.2, y - 0.06, 0.0), (x + 0.2, y + 0.06, 0.12), POLANO * rng.uniform(0.9, 1.05))
    if faza <= 2:
        stos_belek(s, -1.0, B.y0 - 3.0, 4.0, 5 if faza == 1 else 3)
    if faza == 2:
        drabina(s, B.x0 + 0.8, B.y0, B.h + 0.6)
    if faza == 3:
        snopki(s, -1.6, B.y0 - 3.0, 2)
    return s


RYBAK = Bryla(-2.4, 2.4, -1.9, 1.9, 2.3, 2.4, t=0.28)


def lodka(s, x, y, dl=3.6):
    """Lodz rybacka dnem do gory na koziolkach, wzdluz X (dziob w +X)."""
    sz = 0.55
    for xx in (x - dl * 0.3, x + dl * 0.3):
        belka(s, DREWNO, (xx, y - 0.5, 0.0), (xx, y + 0.5, 0.0), 0.08, RUSZT)
    z = 0.1
    pudlo(s, DREWNO, (x - dl / 2, y - sz, z), (x + dl / 2 - 0.5, y + sz, z + 0.5), DESKA * 0.8)
    for k in range(4):  # zwezajacy sie dziob
        w = sz * (1 - (k + 1) / 5)
        pudlo(s, DREWNO, (x + dl / 2 - 0.5 + k * 0.13, y - w, z + 0.05 * k), (x + dl / 2 - 0.37 + k * 0.13, y + w, z + 0.5), DESKA * 0.78)
    pudlo(s, DREWNO, (x - dl / 2, y - sz - 0.02, z + 0.35), (x + dl / 2 - 0.5, y + sz + 0.02, z + 0.42), RUSZT)  # burta
    belka(s, DREWNO, (x - 0.3, y + sz + 0.2, 0.05), (x + 1.6, y + sz + 0.25, 0.05), 0.04, BELKA)  # wioslo


def chata_rybaka(faza):
    """Chata rybaka nad rzeka: zrab pod strzecha, z boku (-X) suszarnia sieci, od strony wody (+Y w generatorze) lodz i kosze."""
    s = Siatka()
    rng = np.random.default_rng(3400)
    B = RYBAK
    z0 = 0.35
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    h = {1: 1.2, 2: B.h, 3: B.h, 4: B.h}[faza]
    zrab(s, B.x0, B.x1, B.y0, B.y1, h, z0, rng)
    przod = (B.x0, B.y0 - 0.02, 0)
    if faza >= 2:
        gotowe = faza == 4
        drzwi_dom(s, przod, "y", -1, 1.4, z0, 1.1, 2.1, DRZWI if gotowe else rgb(30, 26, 24))
        okienko(s, przod, "y", -1, 3.5, z0 + 1.0, 0.65, 0.6, okiennice=gotowe)
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
    if faza == 3:
        dach_slomiany(s, B, 0, 0.5)
    if faza == 4:
        dach_slomiany(s, B)
        # sieci rozwieszone na zerdziach (ciemna, przeswitujaca plachta z oczkami jako pasy)
        for y in (-0.9, 0.7):
            for x in (B.x0 - 2.6, B.x0 - 0.6):
                belka(s, DREWNO, (x, y, -0.2), (x, y, 1.9), 0.08, RUSZT)
            belka(s, DREWNO, (B.x0 - 2.6, y, 1.85), (B.x0 - 0.6, y, 1.85), 0.05, BELKA * 0.85)
            for k in range(9):
                xx = B.x0 - 2.5 + k * 0.22
                pudlo(s, TYNK, (xx, y - 0.02, 0.55 + 0.1 * (k % 2)), (xx + 0.12, y + 0.02, 1.82), SIEC * (0.9 + 0.05 * (k % 3)))
        lodka(s, 0.2, B.y1 + 2.0)
        kosz(s, B.x1 + 0.6, B.y1 + 0.6, 0.3, 0.4, zaw=RYBA)
        kosz(s, B.x1 + 1.2, B.y1 + 0.3, 0.26, 0.36, zaw=RYBA * 0.9)
        beczka(s, B.x1 + 0.5, B.y0 + 0.6)
    if faza <= 2:
        stos_belek(s, -0.5, B.y0 - 3.0, 4.0, 5 if faza == 1 else 3)
    if faza == 2:
        drabina(s, B.x0 + 0.8, B.y0, B.h + 0.6)
    if faza == 3:
        snopki(s, -1.6, B.y0 - 3.0, 3)
        drabina(s, B.x1 - 0.9, B.y0, B.h + 0.8)
    return s


def stos_opalu(poziom):
    """Saga drewna opalowego miedzy palikami: wyzsza z poziomem."""
    s = Siatka()
    rng = np.random.default_rng(3500 + poziom)
    hh = {1: 0.35, 2: 0.65, 3: 0.95, 4: 1.3}[poziom]
    for x in (-0.95, 0.95):
        belka(s, DREWNO, (x, 0.0, -0.1), (x, 0.0, hh + 0.2), 0.06, RUSZT)
    sagi(s, -0.9, 0.9, -0.25, 0.25, hh, rng)
    return s


def stos_ryb(poziom):
    """Ryby w koszach i na zerdzi: 1 / 2 / 3 / 4 kosze."""
    s = Siatka()
    for x, y in [(-0.4, 0.0), (0.35, 0.05), (-0.05, -0.55), (0.6, -0.5)][:poziom]:
        kosz(s, x, y, 0.3, 0.4, zaw=RYBA)
    if poziom >= 3:
        suszarnia(s, -0.9, 0.9, 0.55, 1.3, RYBA * 0.85)
    return s


# --- Paczka A, krok 3 (23.11.2026): rzemioslo (garbarnia, szewc, krawiec) i piwo (slodownia, browar, karczma) ---
SKORA_SUR = rgb(150, 112, 80)    # surowa skora z wlosem
SKORA_W = rgb(118, 78, 48)       # wyprawiona skora
BUT = rgb(70, 48, 32)
SLOD = rgb(196, 160, 98)


def stos_skor(poziom):
    """Surowe skory zlozone w stos na palecie: 2 / 4 / 6 / 9."""
    s = Siatka()
    rng = np.random.default_rng(3800 + poziom)
    pudlo(s, DREWNO, (-0.6, -0.45, 0.0), (0.6, 0.45, 0.08), RUSZT)
    for k in range({1: 2, 2: 4, 3: 6, 4: 9}[poziom]):
        z = 0.08 + k * 0.06
        dx, dy = rng.uniform(-0.08, 0.08, 2)
        pudlo(s, TYNK, (-0.55 + dx, -0.4 + dy, z), (0.55 + dx, 0.4 + dy, z + 0.05), SKORA_SUR * rng.uniform(0.85, 1.05), bez_dolu=False)
    return s


def stos_skory(poziom):
    """Wyprawione skory zwiniete w rulony: 2 / 3 / 5 / 7."""
    s = Siatka()
    for x in (-0.55, 0.55):
        pudlo(s, DREWNO, (x - 0.06, -0.5, 0.0), (x + 0.06, 0.5, 0.08), RUSZT)
    miejsca = [(-0.3, 0), (0.0, 0), (0.3, 0), (-0.15, 1), (0.15, 1), (0.45, 0), (0.0, 2)]
    for k, (y, w) in enumerate(miejsca[:{1: 2, 2: 3, 3: 5, 4: 7}[poziom]]):
        z = 0.08 + 0.12 + w * 0.2
        walec(s, TYNK, (-0.7, y, z), (0.7, y, z), 0.12, SKORA_W * (0.9 + 0.05 * (k % 3)), boki=8)
    return s


def para_butow(s, x, y, z=0.0):
    for d in (-0.07, 0.07):
        pudlo(s, TYNK, (x - 0.13, y + d - 0.05, z), (x + 0.13, y + d + 0.05, z + 0.07), BUT)
        pudlo(s, TYNK, (x - 0.13, y + d - 0.05, z + 0.07), (x - 0.02, y + d + 0.05, z + 0.2), BUT * 1.1)


def stos_butow(poziom):
    """Polka z butami: 1 / 2 / 3 / 4 rzedy par."""
    s = Siatka()
    for x in (-0.6, 0.6):
        belka(s, DREWNO, (x, 0.0, 0.0), (x, 0.0, 1.0), 0.07, RUSZT)
    for r in range(poziom):
        z = 0.1 + r * 0.24
        pudlo(s, DREWNO, (-0.65, -0.18, z - 0.03), (0.65, 0.18, z), DESKA * 0.85)
        for k in range(4):
            para_butow(s, -0.45 + k * 0.3, 0.0, z)
    return s


def stos_ubran(poziom):
    """Zlozone ubrania na lawie: 2 / 4 / 6 / 8 stosikow."""
    s = Siatka()
    pudlo(s, DREWNO, (-0.8, -0.3, 0.35), (0.8, 0.3, 0.42), DESKA * 0.85, bez_dolu=False)
    for x in (-0.7, 0.7):
        pudlo(s, DREWNO, (x - 0.05, -0.25, 0.0), (x + 0.05, 0.25, 0.35), RUSZT)
    for k in range({1: 2, 2: 4, 3: 6, 4: 8}[poziom]):
        x = -0.6 + (k % 4) * 0.4
        z = 0.42 + (k // 4) * 0.12
        pudlo(s, TYNK, (x - 0.16, -0.2, z), (x + 0.16, 0.2, z + 0.11), SUKNA[k % 4] * 1.1, bez_dolu=False)
    return s


def stos_slodu(poziom):
    """Worki slodu: 2 / 4 / 6 / 9 (jasniejsze od maki, przewiazane)."""
    s = Siatka()
    for k in range({1: 2, 2: 4, 3: 6, 4: 9}[poziom]):
        x = -0.45 + (k % 3) * 0.45
        y = -0.25 + ((k // 3) % 2) * 0.5
        z = (k // 6) * 0.42
        walec(s, TYNK, (x, y, z), (x, y, z + 0.42), 0.2, SLOD * (0.92 + 0.04 * (k % 3)), boki=8)
        walec(s, TYNK, (x, y, z + 0.42), (x, y, z + 0.5), 0.08, SLOD * 0.8, boki=6)
    return s


def stos_piwa(poziom):
    """Beczki piwa: 1 / 2 / 3 / 5."""
    s = Siatka()
    for x, y in [(-0.4, 0.0), (0.4, 0.0), (0.0, 0.6), (-0.8, 0.6), (0.8, 0.6)][:{1: 1, 2: 2, 3: 3, 4: 5}[poziom]]:
        beczka(s, x, y)
    return s


WARSZTAT = Bryla(-2.6, 2.6, -2.0, 2.0, 2.4, 2.4, t=0.28)
KARCZMA = Bryla(-4.4, 4.4, -2.6, 2.6, 2.8, 3.0, t=0.3)


def warsztat(faza, rodzaj):
    """Chata rzemieslnika (zrab z bali, gont albo strzecha) z wyposazeniem przed nia wedlug rodzaju:
    garbarnia (kadzie, rama ze skora), szewc (lawa, kopyto, buty), krawiec (zerdz z ubraniami, stol),
    slodownia (suszarnia z kominem, worki), browar (kociol, beczki), karczma (dluga, dwa kominy, godlo, stoly)."""
    s = Siatka()
    rng = np.random.default_rng(3900 + len(rodzaj) * 7 + ord(rodzaj[0]))
    B = KARCZMA if rodzaj == "karczma" else WARSZTAT
    z0 = 0.3
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    zrab(s, B.x0, B.x1, B.y0, B.y1, {1: 1.2, 2: B.h, 3: B.h, 4: B.h}[faza], z0, rng)
    przod = (B.x0, B.y0 - 0.02, 0)
    gotowe = faza == 4
    if faza >= 2:
        drzwi_dom(s, przod, "y", -1, (B.x1 - B.x0) / 2 - 0.55, z0, 1.1, 2.1, DRZWI if gotowe else rgb(30, 26, 24))
        okienko(s, przod, "y", -1, 0.9, z0 + 1.0, 0.7, 0.65, okiennice=gotowe)
        okienko(s, przod, "y", -1, B.x1 - B.x0 - 1.5, z0 + 1.0, 0.7, 0.65, okiennice=gotowe)
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
        if rodzaj in ("garbarnia", "slodownia"):
            dach_slomiany(s, B, 0, 0.5 if faza == 3 else 1.0)
        else:
            for st in (-1, 1):
                polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.5)
    if gotowe:
        y = B.y0 - 1.4
        if rodzaj == "garbarnia":
            for x in (-1.6, -0.4):  # kadzie z garbnikiem
                walec(s, DREWNO, (x, y, 0.0), (x, y, 0.7), 0.45, DRZWI * 1.4, boki=10)
                walec(s, TYNK, (x, y, 0.66), (x, y, 0.68), 0.4, rgb(86, 66, 40), boki=10, denka=False)
            for x in (1.0, 2.2):  # rama z rozpieta skora
                belka(s, DREWNO, (x, y - 0.6, 0.0), (x, y - 0.6, 1.8), 0.08, RUSZT)
            belka(s, DREWNO, (1.0, y - 0.6, 1.75), (2.2, y - 0.6, 1.75), 0.07, RUSZT)
            pudlo(s, TYNK, (1.1, y - 0.62, 0.5), (2.1, y - 0.58, 1.7), SKORA_SUR)
        elif rodzaj == "szewc":
            lawka(s, -1.0, y, 1.6)
            belka(s, DREWNO, (0.4, y, 0.0), (0.4, y, 0.6), 0.14, BALE)  # pieniek z kopytem
            for k in range(3):
                para_butow(s, -1.5 + k * 0.45, y, 0.45)
            beczka(s, 1.6, y + 0.2)
        elif rodzaj == "krawiec":
            for x in (-2.0, 0.4):
                belka(s, DREWNO, (x, y - 0.4, 0.0), (x, y - 0.4, 1.9), 0.08, RUSZT)
            belka(s, DREWNO, (-2.0, y - 0.4, 1.85), (0.4, y - 0.4, 1.85), 0.05, BELKA)
            for k in range(4):  # ubrania na zerdzi
                x = -1.8 + k * 0.55
                pudlo(s, TYNK, (x, y - 0.43, 0.9), (x + 0.42, y - 0.37, 1.83), SUKNA[k % 4] * 1.1)
            pudlo(s, DREWNO, (1.0, y, 0.75), (2.2, y + 0.7, 0.82), DESKA)  # stol z belami
            walec(s, TYNK, (1.1, y + 0.35, 0.95), (2.1, y + 0.35, 0.95), 0.12, SUKNA[1], boki=8)
        elif rodzaj == "slodownia":
            komin(s, B, 1.4, 0.6, rodzaj="kamien", nad=1.0, szer=0.9)
            pudlo(s, KAMIEN, (B.x1 + 0.1, -1.2, 0.0), (B.x1 + 1.5, 1.2, 1.1), MUR)  # suszarnia (piec do slodu)
            pudlo(s, DREWNO, (B.x1 + 0.05, -1.25, 1.1), (B.x1 + 1.55, 1.25, 1.2), DESKA * 0.8)
            for k in range(3):
                walec(s, TYNK, (-1.4 + k * 0.5, y, 0.0), (-1.4 + k * 0.5, y, 0.45), 0.2, SLOD, boki=8)
        elif rodzaj == "browar":
            komin(s, B, -1.4, 0.6, rodzaj="kamien", nad=1.0, szer=0.8)
            pudlo(s, KAMIEN, (-2.0, y - 0.6, 0.0), (-0.6, y + 0.6, 0.5), MUR)  # palenisko pod kotlem
            walec(s, TYNK, (-1.3, y, 0.5), (-1.3, y, 1.25), 0.6, rgb(150, 96, 58), boki=12)  # miedziany kociol
            for x, yy in ((0.6, y), (1.4, y), (1.0, y - 0.8)):
                beczka(s, x, yy)
        elif rodzaj == "karczma":
            for x in (-2.6, 2.6):
                komin(s, B, x, 0.9, rodzaj="kamien", nad=0.9, szer=0.8)
            belka(s, DREWNO, (B.x0 + 1.2, B.y0 - 0.1, 2.4), (B.x0 + 1.2, B.y0 - 0.9, 2.4), 0.07, BELKA)  # godlo
            pudlo(s, DREWNO, (B.x0 + 1.15, B.y0 - 0.95, 1.6), (B.x0 + 1.25, B.y0 - 0.45, 2.35), rgb(150, 110, 60))
            for x in (-2.4, 1.8):  # stoly z lawami przed karczma
                pudlo(s, DREWNO, (x - 0.9, y - 0.4, 0.72), (x + 0.9, y + 0.4, 0.8), DESKA)
                for xx in (x - 0.7, x + 0.7):
                    pudlo(s, DREWNO, (xx - 0.05, y - 0.3, 0.0), (xx + 0.05, y + 0.3, 0.72), RUSZT)
                lawka(s, x - 0.8, y - 0.85, 1.6)
                lawka(s, x - 0.8, y + 0.85, 1.6)
            beczka(s, B.x1 - 0.6, B.y0 - 0.6)
            beczka(s, B.x1 - 1.4, B.y0 - 0.6)
    if faza <= 2:
        stos_belek(s, -0.8, B.y0 - 3.0, 4.0, 5 if faza == 1 else 3)
    if faza == 2:
        drabina(s, B.x0 + 0.8, B.y0, B.h + 0.6)
    if faza == 3:
        stos_desek(s, -0.8, B.y0 - 3.0, 2.6, 3)
    return s


# --- Woly (23.11.2026, prosba uzytkownika: jak w Manor Lords woly ciagna klody; zagroda daje kolejne woly) ---
WOLY = Bryla(-3.4, 3.4, 1.6, 4.6, 2.2, 2.2, t=0.15)    # wiata z tylu, przodem (-Y) otwarta
WOLY2 = Bryla(-4.2, 4.2, 1.4, 5.0, 2.5, 2.6, t=0.28)   # obora z bali
WOLY_PLOT = (-6.0, 6.0, -5.2, 5.4)


def zagroda_wolow(faza, poziom=1):
    """Zagroda wolow: poziom 1 - wiata pod strzecha na slupach, ogrodzony wybieg z pasnikiem i korytem;
    poziom 2 (obora) - zamknieta obora z bali pod strzecha, wiekszy pasnik, jarzma na scianie."""
    s = Siatka()
    rng = np.random.default_rng(3600 + poziom)
    B = WOLY2 if poziom == 2 else WOLY
    x0, x1, y0, y1 = WOLY_PLOT
    if faza == 1:
        klepisko(s, B.x0, B.x1, B.y0, B.y1, ZIEMIA * 0.9)
        stos_belek(s, -2.5, y0 - 1.6, 4.0, 5)
        plot(s, x0, x1, y0, y1, brama=(-1.6, 1.6), boki="pl", zerdzie=False)
        return s
    klepisko(s, B.x0 - 0.2, B.x1 + 0.2, B.y0 - 0.4, B.y1 + 0.2, ZIEMIA * 0.9)
    plot(s, x0, x1, y0, y1, brama=(-1.6, 1.6), boki="pltd" if faza >= 3 else "pl", zerdzie=faza >= 3)
    if poziom == 1:
        for x in (B.x0, 0.0, B.x1):
            for y in (B.y0, B.y1):
                belka(s, DREWNO, (x, y, -0.2), (x, y, B.h), 0.16, BALE)
        for y in (B.y0, B.y1):
            belka(s, DREWNO, (B.x0 - 0.1, y, B.h - 0.08), (B.x1 + 0.1, y, B.h - 0.08), 0.16, BALE * 0.9)
        if faza >= 3:
            sciana_desek(s, B.x0, B.y1, B.x1, B.y1, B.h)
            sciana_desek(s, B.x0, B.y0, B.x0, B.y1, B.h)
    else:
        z0 = 0.3
        cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
        zrab(s, B.x0, B.x1, B.y0, B.y1, 1.2 if faza == 2 else B.h, z0, rng)
        if faza >= 3:
            # szerokie wrota obory od przodu (-Y)
            pudlo(s, DREWNO, (-1.1, B.y0 - 0.06, z0), (1.1, B.y0 + 0.02, 2.0), DRZWI if faza == 4 else rgb(30, 26, 24))
            belka(s, DREWNO, (-1.1, B.y0 - 0.1, 1.0), (1.1, B.y0 - 0.1, 1.6), 0.06, BELKA)
    if faza >= 3:
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
        dach_slomiany(s, B, 0, 0.5 if faza == 3 else 1.0)
    if faza == 4:
        # pasnik z sianem, koryto z woda, stog siana w rogu wybiegu
        for k, px in enumerate((-3.0,) if poziom == 1 else (-3.0, 0.4)):
            py = B.y0 - 0.9
            for x in (px - 1.0, px + 1.0):
                belka(s, DREWNO, (x, py - 0.4, 0.0), (x, py, 1.3), 0.1, RUSZT)
                belka(s, DREWNO, (x, py + 0.4, 0.0), (x, py, 1.3), 0.1, RUSZT)
            belka(s, DREWNO, (px - 1.1, py, 1.3), (px + 1.1, py, 1.3), 0.1, BELKA)
            pudlo(s, STRZECHA, (px - 0.85, py - 0.22, 0.6), (px + 0.85, py + 0.22, 1.1), SIANO)
        pudlo(s, DREWNO, (3.0, -1.6, 0.0), (4.8, -1.0, 0.42), DESKA * 0.8, bez_dolu=False)        # koryto
        pudlo(s, TYNK, (3.08, -1.52, 0.42), (4.72, -1.08, 0.44), rgb(78, 104, 118))
        stozek(s, STRZECHA, (-4.6, -3.6, 0.0), 1.1, 2.0, SIANO * 0.95, boki=8)
        if poziom == 2:
            stozek(s, STRZECHA, (4.4, -3.8, 0.0), 1.0, 1.8, SIANO * 0.9, boki=8)
            for k in range(2):  # jarzma oparte o sciane obory
                xx = B.x1 - 1.6 + k * 0.7
                belka(s, DREWNO, (xx - 0.3, B.y0 - 0.25, 0.9), (xx + 0.3, B.y0 - 0.25, 0.9), 0.07, BELKA)
                belka(s, DREWNO, (xx, B.y0 - 0.3, 0.0), (xx, B.y0 - 0.15, 0.95), 0.05, BELKA * 0.9)
    if faza == 2:
        stos_belek(s, -2.5, y0 - 1.6, 4.0, 3)
    if faza == 3:
        drabina(s, B.x1 - 0.6, B.y0, B.h + 0.6)
    return s


STAJNIA = Bryla(-5.0, 5.0, 1.4, 4.6, 2.4, 2.5, t=0.28)


def stajnia(faza):
    """Stajnia: dluga chata z bali pod gontem, od przodu trzy polowkowe drzwi boksow, wybieg z korytem i zaparkowany woz."""
    s = Siatka()
    rng = np.random.default_rng(3700)
    B = STAJNIA
    x0, x1, y0, y1 = WOLY_PLOT
    if faza == 1:
        klepisko(s, B.x0, B.x1, B.y0, B.y1, ZIEMIA * 0.9)
        stos_belek(s, -2.5, y0 - 1.6, 4.0, 5)
        stos_desek(s, 2.5, y0 - 1.6, 3.0, 3)
        return s
    klepisko(s, B.x0 - 0.2, B.x1 + 0.2, B.y0 - 0.4, B.y1 + 0.2, ZIEMIA * 0.9)
    plot(s, x0, x1, y0, y1, brama=(-1.6, 1.6), boki="pld" if faza >= 3 else "p", zerdzie=faza >= 3)
    z0 = 0.3
    cokol(s, B.x0, B.x1, B.y0, B.y1, z0)
    zrab(s, B.x0, B.x1, B.y0, B.y1, 1.2 if faza == 2 else B.h, z0, rng)
    if faza >= 3:
        for k, xd in enumerate((-3.2, 0.0, 3.2)):  # drzwi boksow: dolna polowa zamknieta, gorna otwarta (ciemno)
            pudlo(s, DREWNO, (xd - 0.6, B.y0 - 0.06, z0), (xd + 0.6, B.y0 + 0.02, 1.3), DRZWI if faza == 4 else rgb(30, 26, 24))
            pudlo(s, DREWNO, (xd - 0.6, B.y0 - 0.03, 1.3), (xd + 0.6, B.y0 + 0.02, 2.1), rgb(30, 26, 24))
        for x, st in ((B.x0, -1), (B.x1, 1)):
            szczyt(s, x if st < 0 else x - 0.12, B.y0, B.y1, B.h, B.k, 0.12, DESKA_SZCZYT, mat=DREWNO)
        wiezba(s, B, kolor=BALE)
        for st in (-1, 1):
            polac(s, B, st, 0, 0.5 if faza == 3 else 1.0, okap=0.5)
    if faza == 4:
        pudlo(s, DREWNO, (3.0, -1.6, 0.0), (4.8, -1.0, 0.42), DESKA * 0.8, bez_dolu=False)        # koryto
        pudlo(s, TYNK, (3.08, -1.52, 0.42), (4.72, -1.08, 0.44), rgb(78, 104, 118))
        stozek(s, STRZECHA, (-4.6, -3.6, 0.0), 1.0, 1.8, SIANO * 0.95, boki=8)
        # woz bez konia, dyszle na ziemi
        for y in (-0.6, 0.6):
            walec(s, DREWNO, (-1.0, y - 0.04, 0.5), (-1.0, y + 0.04, 0.5), 0.5, BELKA, boki=12)
            belka(s, DREWNO, (-1.0, y * 0.8, 0.85), (1.4, y * 0.5, 0.05), 0.07, RUSZT)
        pudlo(s, DREWNO, (-2.0, -0.55, 0.75), (-0.2, 0.55, 1.05), DESKA * 0.85, bez_dolu=False)
        for k in range(3):  # siodla / uprzaz na kolkach przy scianie
            belka(s, DREWNO, (B.x1 - 1.2 - k * 0.5, B.y0 - 0.2, 1.4), (B.x1 - 1.2 - k * 0.5, B.y0 - 0.45, 1.4), 0.05, BELKA)
    if faza == 2:
        stos_belek(s, -2.5, y0 - 1.6, 4.0, 3)
    if faza == 3:
        drabina(s, B.x1 - 0.6, B.y0, B.h + 0.6)
    return s


BUDYNKI = {
    "SM_Kosciol": (kosciol, 5),
    "SM_Chata": (chata, 4),
    "SM_DomPruski": (dom_pruski, 4),
    "SM_DomDuzy": (dom_duzy, 4),
    "SM_PlacSkladowy": (plac_skladowy, 4),
    "SM_ChataDrwala": (chata_drwala, 4),
    "SM_ChataBorowego": (chata_borowego, 4),
    "SM_Pilarnia": (pilarnia, 4),
    "SM_Kamieniolom": (kamieniolom, 4),
    "SM_ChataZbieracza": (chata_zbieracza, 4),
    "SM_ChataMysliwego": (chata_mysliwego, 4),
    "SM_Spichlerz": (spichlerz, 4),
    "SM_Pole": (pole, 2),
    "SM_Mlyn": (mlyn, 4),
    "SM_Piekarnia": (piekarnia, 4),
    "SM_TartakWodny": (tartak_wodny, 4),
    "SM_Owczarnia": (owczarnia, 4),
    "SM_ChataTkacza": (chata_tkacza, 4),
    "SM_KopalniaRudy": (kopalnia_rudy, 4),
    "SM_Kuznia": (kuznia, 4),
    "SM_Zbrojownia": (zbrojownia, 4),
    "SM_Targ": (targ, 4),
    "SM_ChataOpalowa": (chata_opalowa, 4),
    "SM_ChataRybaka": (chata_rybaka, 4),
    "SM_ZagrodaWolow": (zagroda_wolow, 4),
    "SM_ZagrodaWolow2": (lambda f: zagroda_wolow(f, 2), 4),
    "SM_Stajnia": (stajnia, 4),
    "SM_Garbarnia": (lambda f: warsztat(f, "garbarnia"), 4),
    "SM_Szewc": (lambda f: warsztat(f, "szewc"), 4),
    "SM_Krawiec": (lambda f: warsztat(f, "krawiec"), 4),
    "SM_Slodownia": (lambda f: warsztat(f, "slodownia"), 4),
    "SM_Browar": (lambda f: warsztat(f, "browar"), 4),
    "SM_Karczma": (lambda f: warsztat(f, "karczma"), 4),
    "SM_KosciolDrewniany": (kosciol_drewniany, 4),
    "SM_KosciolWielki": (kosciol_wielki, 5),
    "SM_ChataDrwala2": (lambda f: chata_drwala(f, 2), 4),
    "SM_ChataDrwala3": (lambda f: chata_drwala(f, 3), 4),
    "SM_ChataBorowego2": (lambda f: chata_borowego(f, 2), 4),
    "SM_ChataBorowego3": (lambda f: chata_borowego(f, 3), 4),
}
# Warianty domow (poziomy 1..3, warianty 1..3; wariant 0 to SM_Chata / SM_DomPruski / SM_DomDuzy wyzej).
for _w in range(1, 4):
    BUDYNKI[f"SM_Chata_W{_w}"] = ((lambda w: lambda f: chata(f, w))(_w), 4)
    BUDYNKI[f"SM_DomPruski_W{_w}"] = ((lambda w: lambda f: dom_pruski(f, w))(_w), 4)
    BUDYNKI[f"SM_DomDuzy_W{_w}"] = ((lambda w: lambda f: dom_duzy(f, w))(_w), 4)
POJEDYNCZE = {"SM_Namiot": namiot, "SM_Ognisko": ognisko,  # bez faz budowy
              "SM_Mlyn_Skrzydla": mlyn_skrzydla, "SM_TartakWodny_Kolo": tartak_kolo,  # czesci ruchome (gra je kreci)
              "SM_Owca": owca, "SM_Owca_Strzyzona": owca_strzyzona,  # owce w owczarni (gra je przesuwa)
              "SM_Plot": plot_segment, "SM_Plot_Palik": plot_palik,     # plot pola i pastwiska z segmentow
              # mury osad rodow (odcinek 4 m, odcinek ze schodami, baszta, brama) i studnia placu
              "SM_Palisada": palisada_segment, "SM_Palisada_Schody": lambda: palisada_segment(True),
              "SM_Palisada_Wieza": palisada_wieza, "SM_Palisada_Brama": palisada_brama,
              "SM_Mur": mur_segment, "SM_Mur_Schody": lambda: mur_segment(True), "SM_Mur_Wieza": mur_wieza, "SM_Mur_Brama": mur_brama,
              "SM_Studnia": studnia_placu,
              "SM_Pole_Kafel_Ugor": lambda: kafel_pola(-1),
              "SM_Zboze_Zielone": lambda: zboze(0), "SM_Zboze_Dojrzale": lambda: zboze(1), "SM_Zboze_Scierniska": lambda: zboze(2),
              # ulepszenia (22.11.2026): poprzedni poziom w rusztowaniu, potem gra pokazuje F(n-1) i gotowy
              "SM_Kosciol_U": kosciol_przebudowa, "SM_KosciolWielki_U": lambda: kosciol_wielki(1),
              "SM_ChataDrwala2_U": lambda: przebudowa_prosta(chata_drwala, DRWAL),
              "SM_ChataDrwala3_U": lambda: przebudowa_prosta(lambda f: chata_drwala(f, 2), DRWAL, kamien=True),
              "SM_ChataBorowego2_U": lambda: przebudowa_prosta(chata_borowego, BOROWY),
              "SM_ChataBorowego3_U": lambda: przebudowa_prosta(lambda f: chata_borowego(f, 2), BOROWY, kamien=True),
              "SM_ZagrodaWolow2_U": lambda: przebudowa_prosta(zagroda_wolow, WOLY)}
for _w in range(4):
    _sufiks = f"_W{_w}" if _w else ""
    POJEDYNCZE[f"SM_DomPruski{_sufiks}_U"] = (lambda w: lambda: przebudowa(chata, pruski_B(w), w))(_w)
    POJEDYNCZE[f"SM_DomDuzy{_sufiks}_U"] = (lambda w: lambda: przebudowa(dom_pruski, duzy_B(w), w, kamien=True))(_w)
for _k, _u in enumerate(UPRAWY):
    POJEDYNCZE[f"SM_Pole_{_u}"] = (lambda k: lambda: uprawa(k))(_k)
    POJEDYNCZE[f"SM_Pole_Kafel_{_u}"] = (lambda k: lambda: kafel_pola(k))(_k)
for _p in range(1, 5):
    POJEDYNCZE[f"SM_Stos_Kloda_{_p}"] = (lambda p: lambda: stos_klod(p))(_p)
    POJEDYNCZE[f"SM_Stos_Deska_{_p}"] = (lambda p: lambda: stos_desek_towar(p))(_p)
    POJEDYNCZE[f"SM_Stos_Kamien_{_p}"] = (lambda p: lambda: stos_kamieni_towar(p))(_p)
    POJEDYNCZE[f"SM_Stos_Jedzenie_{_p}"] = (lambda p: lambda: stos_jedzenia(p))(_p)
    POJEDYNCZE[f"SM_Stos_Zboze_{_p}"] = (lambda p: lambda: stos_zboza(p))(_p)
    POJEDYNCZE[f"SM_Stos_Maka_{_p}"] = (lambda p: lambda: stos_maki(p))(_p)
    POJEDYNCZE[f"SM_Stos_Welna_{_p}"] = (lambda p: lambda: stos_welny(p))(_p)
    POJEDYNCZE[f"SM_Stos_Sukno_{_p}"] = (lambda p: lambda: stos_sukna(p))(_p)
    POJEDYNCZE[f"SM_Stos_Ruda_{_p}"] = (lambda p: lambda: stos_rudy(p))(_p)
    POJEDYNCZE[f"SM_Stos_Bron_{_p}"] = (lambda p: lambda: stos_broni(p))(_p)
    POJEDYNCZE[f"SM_Stos_Opal_{_p}"] = (lambda p: lambda: stos_opalu(p))(_p)
    POJEDYNCZE[f"SM_Stos_Ryby_{_p}"] = (lambda p: lambda: stos_ryb(p))(_p)
    POJEDYNCZE[f"SM_Stos_Skory_{_p}"] = (lambda p: lambda: stos_skor(p))(_p)
    POJEDYNCZE[f"SM_Stos_Skora_{_p}"] = (lambda p: lambda: stos_skory(p))(_p)
    POJEDYNCZE[f"SM_Stos_Buty_{_p}"] = (lambda p: lambda: stos_butow(p))(_p)
    POJEDYNCZE[f"SM_Stos_Ubrania_{_p}"] = (lambda p: lambda: stos_ubran(p))(_p)
    POJEDYNCZE[f"SM_Stos_Slod_{_p}"] = (lambda p: lambda: stos_slodu(p))(_p)
    POJEDYNCZE[f"SM_Stos_Piwo_{_p}"] = (lambda p: lambda: stos_piwa(p))(_p)


# ---------------- zniszczenia (23.11.2026, wojna) ----------------
# Kazdy budynek (oprocz pola) i kazdy kawalek muru osad rodow ma dwa etapy zniszczenia robione z gotowego modelu:
#   SM_<Model>[_W<w>]_Z1 - uszkodzony i nadpalony: dziury w dachu (widac zweglona wiezbe), smugi sadzy na scianach;
#   SM_<Model>[_W<w>]_Z2 - ruina po pozarze: mury sciete na nierownej wysokosci (kamien wyzej, drewno nisko), dach spalony,
#                          kopiec gruzu i popiolu w obrysie, zweglone belki i slupy.
# Siatke dzielimy na male trojkaty (ta sama krawedz zawsze dzieli sie tak samo, bez szczelin), wycinamy i przyciemniamy kolory
# wierzcholkow. Materialy sa jednostronne, wiec do wycietych czesci dokladamy odwrocone, ciemne kopie: przez dziure i nad
# scietym murem widac zweglone wnetrze, a nie przeswit.
SADZA = rgb(24, 21, 19)
WEGIEL = rgb(36, 28, 22)
POPIOL = rgb(96, 91, 84)
BEZ_ZNISZCZEN = {"SM_Pole"}
MURY_OSAD = ["SM_Palisada", "SM_Palisada_Schody", "SM_Palisada_Wieza", "SM_Palisada_Brama",
             "SM_Mur", "SM_Mur_Schody", "SM_Mur_Wieza", "SM_Mur_Brama"]
# Wysokosc sciecia: baza + wysokosc sciany w tym miejscu * (czesc stala + czesc zmienna * szum), w metrach. Skala kamieniolomu i kopalni zostaje cala; mury osad padaja nisko (wylom).
RUINA_OPCJE = {"SM_Kamieniolom": {"kamien": None, "tynk": None}, "SM_KopalniaRudy": {"kamien": None, "tynk": None}}
for _m in ("SM_Palisada", "SM_Palisada_Schody", "SM_Mur", "SM_Mur_Schody"):  # odcinki: wylom; baszty i bramy stoja wyzej
    RUINA_OPCJE[_m] = {"kamien": (0.3, 0.05, 0.3), "drewno": (0.15, 0.03, 0.2)}


def _hash3(ix, iy, iz, ziarno):
    u = lambda a: a.astype(np.int64).astype(np.uint64)
    h = (u(ix) * np.uint64(73856093)) ^ (u(iy) * np.uint64(19349663)) ^ (u(iz) * np.uint64(83492791)) ^ np.uint64((ziarno * 2654435761) % 2 ** 32)
    h = (h ^ (h >> np.uint64(13))) * np.uint64(1274126177)
    h = h ^ (h >> np.uint64(16))
    return (h & np.uint64(0xFFFF)).astype(float) / 65535.0


def szum3(p, skala, ziarno=0):
    """Gladki szum wartosci 0..1, p[N,3] w metrach."""
    q = np.asarray(p, float).reshape(-1, 3) / skala
    i0 = np.floor(q).astype(np.int64)
    f = q - i0
    f = f * f * (3 - 2 * f)
    out = np.zeros(len(q))
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                w = (f[:, 0] if dx else 1 - f[:, 0]) * (f[:, 1] if dy else 1 - f[:, 1]) * (f[:, 2] if dz else 1 - f[:, 2])
                out += w * _hash3(i0[:, 0] + dx, i0[:, 1] + dy, i0[:, 2] + dz, ziarno)
    return out


def _gladko(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def _zupa(s):
    """Siatka -> {material: [P, N, C, UV]} jako osobne trojkaty (T, 3, k)."""
    out = {}
    for mat, czesci in s.grupy.items():
        if czesci:
            A = [[], [], [], []]
            for p, n, c, i, uv in czesci:
                i = i.reshape(-1, 3)
                for lst, x in zip(A, (p, n, c, uv)):
                    lst.append(np.asarray(x, float)[i])
            out[mat] = [np.concatenate(x) for x in A]
    return out


def _z_zupy(nazwy, zupa):
    """Z powrotem do Siatki; wspolne wierzcholki sklejone (plik glb kilka razy mniejszy)."""
    s = Siatka(nazwy)
    for mat, (P, N, C, UV) in zupa.items():
        if len(P):
            V = np.concatenate([P.reshape(-1, 3), N.reshape(-1, 3), C.reshape(-1, 4), UV.reshape(-1, 2)], 1)
            u, inv = np.unique(np.round(V, 4), axis=0, return_inverse=True)
            s.dodaj(mat, u[:, :3], u[:, 3:6], u[:, 6:10], np.asarray(inv).reshape(-1, 3), u[:, 10:12])
    return s


def _podziel(A, L, potrzeba=None):
    """Dzieli najdluzsza krawedz na pol, az kazda krawedz <= L (albo potrzeba(p0, p1) == False dla krawedzi)."""
    gotowe = [[] for _ in A]
    for _ in range(80):
        if not len(A[0]):
            break
        P = A[0]
        d = np.stack([np.linalg.norm(P[:, (k + 1) % 3] - P[:, k], axis=1) for k in range(3)], 1)
        if potrzeba is not None:
            d = np.where(np.stack([potrzeba(P[:, k], P[:, (k + 1) % 3]) for k in range(3)], 1), d, 0.0)
        dziel = d.max(1) > L
        for g, x in zip(gotowe, A):
            g.append(x[~dziel])
        k = d.argmax(1)[dziel]
        A = [x[dziel] for x in A]
        idx = (k[:, None] + np.arange(3)[None, :]) % 3
        A = [np.take_along_axis(x, idx[:, :, None], 1) for x in A]
        m = [(x[:, 0] + x[:, 1]) / 2 for x in A]
        m[1] = m[1] / (np.linalg.norm(m[1], axis=1, keepdims=True) + 1e-9)
        A = [np.concatenate([np.stack([x[:, 0], mm, x[:, 2]], 1), np.stack([mm, x[:, 1], x[:, 2]], 1)]) for x, mm in zip(A, m)]
    for g, x in zip(gotowe, A):
        g.append(x)
    return [np.concatenate(g) for g in gotowe]


def _wybierz(A, m):
    return [x[m] for x in A]


def _z_tylem(A, ciemniej=0.35):
    """Dokleja odwrocone kopie trojkatow (ciemniejsze): widok od srodka przy jednostronnym materiale."""
    P, N, C, UV = A
    C2 = C.copy()
    C2[..., :3] *= ciemniej
    return [np.concatenate([P, P[:, ::-1]]), np.concatenate([N, -N[:, ::-1]]), np.concatenate([C, C2[:, ::-1]]),
            np.concatenate([UV, UV[:, ::-1]])]


def _zabarw(C, cel, t):
    C = C.copy()
    C[..., :3] = C[..., :3] * (1 - t[..., None]) + np.asarray(cel) * t[..., None]
    return C


def _pole_pow(A, zmin=0.6):
    P = A[0]
    pole = 0.5 * np.linalg.norm(np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0]), axis=1)
    return pole[P[:, :, 2].mean(1) > zmin].sum()


def _obrys(zupa):
    """Obrys bryly z gory: najwiekszy zwarty dach (bez okapu; daszek bramy cmentarnej obok kosciola sie nie liczy),
    a gdy dachu nie ma - wszystko powyzej 0,5 m."""
    T = [zupa[m][0] for m in (DACH, STRZECHA) if m in zupa]
    T = np.concatenate(T) if T else np.zeros((0, 3, 3))
    T = T[T[..., 2].mean(1) > 1.2]
    P = np.zeros((0, 3))
    if len(T):
        mn, mx = T[..., :2].min(1), T[..., :2].max(1)
        g0 = mn.min(0) - 1
        i0, i1 = (mn - g0).astype(int), (mx - g0).astype(int)
        M = np.zeros(i1.max(0) + 2, bool)
        for k in range(len(T)):
            M[i0[k, 0]:i1[k, 0] + 1, i0[k, 1]:i1[k, 1] + 1] = True
        etyk, nr = -np.ones(M.shape, int), 0
        for start in zip(*np.nonzero(M)):  # spojne kawalki dachu (sasiedztwo 4)
            if etyk[start] >= 0:
                continue
            stos = [start]
            etyk[start] = nr
            while stos:
                a, b = stos.pop()
                for q in ((a + 1, b), (a - 1, b), (a, b + 1), (a, b - 1)):
                    if 0 <= q[0] < M.shape[0] and 0 <= q[1] < M.shape[1] and M[q] and etyk[q] < 0:
                        etyk[q] = nr
                        stos.append(q)
            nr += 1
        pole = 0.5 * np.linalg.norm(np.cross(T[:, 1] - T[:, 0], T[:, 2] - T[:, 0]), axis=1)
        e = etyk[i0[:, 0], i0[:, 1]]
        P = T[e == np.bincount(e, pole).argmax()].reshape(-1, 3)
    okap = 0.5
    if len(P) < 3:
        P = np.concatenate([A[0].reshape(-1, 3) for A in zupa.values()])
        P = P[P[:, 2] > 0.5]
        okap = 0.1
    x0, x1, y0, y1 = P[:, 0].min() + okap, P[:, 0].max() - okap, P[:, 1].min() + okap, P[:, 1].max() - okap
    return (x0, x1, y0, y1) if x1 > x0 + 0.5 and y1 > y0 + 0.5 else ((x0 + x1) / 2 - 0.5, (x0 + x1) / 2 + 0.5, (y0 + y1) / 2 - 0.5, (y0 + y1) / 2 + 0.5)


def _wnetrze(A, obrys, zmin, dach=None):
    """(T, 3): wierzcholki scian zwroconych do srodka bryly albo w gore (widac je tylko przez dziury i nad scietym murem).
    dach: mapa wysokosci dachu - powierzchnie w gore licza sie tylko pod nim."""
    x0, x1, y0, y1 = obrys
    P, N = A[0], A[1]
    gora = N[..., 2] > 0.7
    if dach is not None:
        gora &= P[..., 2] < dach(P.reshape(-1, 3)[:, :2]).reshape(P.shape[:-1]) - 0.3
    w = (P[..., 0] > x0 + 0.1) & (P[..., 0] < x1 - 0.1) & (P[..., 1] > y0 + 0.1) & (P[..., 1] < y1 - 0.1) & (P[..., 2] > zmin)
    d = np.stack([(x0 + x1) / 2 - P[..., 0], (y0 + y1) / 2 - P[..., 1]], -1)
    d /= np.linalg.norm(d, axis=-1, keepdims=True) + 1e-9
    return w & ((N[..., 0] * d[..., 0] + N[..., 1] * d[..., 1] > 0.3) | gora)


def _mapa_wys(zupa, maty, kom=1.0):
    """Najwyzszy punkt scian nad kazda komorka 1 m (z gory): mur wiezy stoi po zniszczeniu wyzej niz mur nawy."""
    T = [zupa[m][0] for m in maty if m in zupa]
    if not T:
        return lambda xy: np.zeros(len(xy))
    T = np.concatenate(T)
    mn, mx, zt = T[..., :2].min(1), T[..., :2].max(1), T[..., 2].max(1)
    g0 = mn.min(0) - kom
    n = ((mx.max(0) + kom - g0) / kom).astype(int) + 2
    M = np.zeros(n)
    i0 = ((mn - g0) / kom).astype(int); i1 = ((mx - g0) / kom).astype(int)
    for k in np.argsort(zt):  # rosnaco: wyzszy nadpisuje
        M[i0[k, 0]:i1[k, 0] + 1, i0[k, 1]:i1[k, 1] + 1] = zt[k]
    Mp = np.pad(M, 1)
    M = np.max([Mp[1 + dx:Mp.shape[0] - 1 + dx, 1 + dy:Mp.shape[1] - 1 + dy] for dx in (-1, 0, 1) for dy in (-1, 0, 1)], 0)

    def f(xy):
        i = np.clip(((np.asarray(xy) - g0) / kom).astype(int), 0, np.array(M.shape) - 1)
        return M[i[:, 0], i[:, 1]]
    return f


def _przytnij(A, f):
    """Zostawia czesc trojkatow, gdzie f <= 0 (f liczone w wierzcholkach, ciecie liniowe po krawedziach)."""
    fin = f <= 0
    ile = fin.sum(1)
    wynik = [[x[ile == 3]] for x in A]
    for przypadek in (1, 2):
        m = ile == przypadek
        if not m.any():
            continue
        B = [x[m] for x in A]
        fb, ib = f[m], fin[m]
        r = ib.argmax(1) if przypadek == 1 else (~ib).argmax(1)  # wierzcholek sam po swojej stronie -> v0
        idx = (r[:, None] + np.arange(3)[None, :]) % 3
        B = [np.take_along_axis(x, idx[:, :, None], 1) for x in B]
        fb = np.take_along_axis(fb, idx, 1)
        t1 = (fb[:, 0] / (fb[:, 0] - fb[:, 1] + 1e-12))[:, None]
        t2 = (fb[:, 0] / (fb[:, 0] - fb[:, 2] + 1e-12))[:, None]
        for k, x in enumerate(B):
            a = x[:, 0] + (x[:, 1] - x[:, 0]) * t1
            b = x[:, 0] + (x[:, 2] - x[:, 0]) * t2
            if k == 1:
                a /= np.linalg.norm(a, axis=1, keepdims=True) + 1e-9
                b /= np.linalg.norm(b, axis=1, keepdims=True) + 1e-9
            if przypadek == 1:
                wynik[k].append(np.stack([x[:, 0], a, b], 1))
            else:
                wynik[k].append(np.concatenate([np.stack([a, x[:, 1], x[:, 2]], 1), np.stack([a, x[:, 2], b], 1)]))
    A = [np.concatenate(w) for w in wynik]
    P = A[0]
    pole = np.linalg.norm(np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0]), axis=1)
    return _wybierz(A, pole > 1e-7)


def uszkodzony(s, ziarno=1):
    """Etap Z1: dziury w dachu z nadpalonymi brzegami, smugi sadzy na scianach (wyzej ciemniej), zweglone wnetrze."""
    zupa = _zupa(s)
    H = max(A[0][..., 2].max() for A in zupa.values())
    obrys = _obrys(zupa)
    ma_dach = DACH in zupa or STRZECHA in zupa
    mapa_dachu = _mapa_wys(zupa, (DACH, STRZECHA))
    out = {}
    for mat, A in zupa.items():
        dach = mat in (DACH, STRZECHA)
        if mat != DREWNO:  # drewno ciemne i tak; bez dzielenia (bale to dlugie walce)
            A = _podziel(A, 0.55 if dach else 0.9)
        pv = A[0].reshape(-1, 3)
        if dach:
            cen = A[0].mean(1)
            dziura = (szum3(cen, 2.3, ziarno) > 0.64) & (cen[:, 2] > 1.6)
            t = _gladko(0.46, 0.64, szum3(pv, 2.3, ziarno)).reshape(-1, 3) * 0.92 * (A[0][..., 2] > 1.2)
            A[2] = _zabarw(A[2], SADZA, t)
            A = _z_tylem(_wybierz(A, ~dziura), 0.3)
        else:
            n = szum3(pv * [1, 1, 0.35], 1.7, ziarno + 3)
            wys = np.clip(pv[:, 2] / max(H * 0.6, 1.0), 0, 1)
            t = (_gladko(0.52, 0.82, n) * (0.3 + 0.7 * wys) * 0.85).reshape(-1, 3) * (A[0][..., 2] > 0.05)
            if ma_dach:
                t = np.maximum(t, 0.85 * _wnetrze(A, obrys, 0.6, mapa_dachu))
            A[2] = _zabarw(A[2], SADZA, t)
            if mat == DREWNO:
                A[2][..., :3] *= 0.82
        out[mat] = A
    return _z_zupy(s.nazwy, out)


def ruina(s, nazwa="", ziarno=1):
    """Etap Z2: ruina po pozarze."""
    opc = RUINA_OPCJE.get(nazwa, {})
    rng = np.random.default_rng(ziarno)
    zupa = _zupa(s)
    obrys = x0, x1, y0, y1 = _obrys(zupa)
    pole_k = _pole_pow(zupa[KAMIEN]) if KAMIEN in zupa else 0.0
    pole_d = sum(_pole_pow(zupa[m]) for m in (DREWNO, TYNK) if m in zupa)
    kamienny = pole_k > 0.6 * pole_d and opc.get("kamien", 1) is not None
    hloc = _mapa_wys(zupa, (KAMIEN, TYNK, DREWNO))
    # (baza, czesc wysokosci min, czesc wysokosci zmienna): kamien stoi wyzej i pekal stopniami, drewno i tynk spalone nisko
    ciecie = {KAMIEN: opc.get("kamien", (0.4, 0.18, 0.55)), TYNK: opc.get("tynk", (0.2, 0.04, 0.4)),
              DREWNO: opc.get("drewno", (0.2, 0.04, 0.4)), DACH: (0.3, 0.0, 0.0), STRZECHA: (0.3, 0.0, 0.0)}

    def h_ciecia(mat, xy):
        baza, cmin, czm = ciecie[mat]
        p = np.concatenate([xy, np.zeros((len(xy), 1))], 1)
        h = baza + hloc(xy) * (cmin + czm * szum3(p, 4.0, ziarno))
        if czm <= 0:
            return h
        if mat == KAMIEN:  # mur peka stopniami (warstwy kamienia)
            return np.floor((h + 0.45 * (szum3(p, 1.2, ziarno + 7) - 0.5)) / 0.3) * 0.3
        return h + 0.7 * (szum3(p, 0.6, ziarno + 9) - 0.5)  # drewno i tynk: postrzepione

    out = {}
    for mat, A in zupa.items():
        pv = A[0].reshape(-1, 3)
        if ciecie.get(mat) is None:  # zostaje cale (skala), tylko okopcone
            A[2] = _zabarw(A[2], SADZA, (0.35 * szum3(pv, 1.5, ziarno + 5)).reshape(-1, 3))
            out[mat] = A
            continue
        def h_mat(xy, mat=mat):
            if mat != KAMIEN:
                return h_ciecia(mat, xy)
            # minimum z okolicy 0,8 m: blanki i gzymsy wysuniete przed mur nie zostaja w powietrzu nad scietym murem
            return np.min([h_ciecia(mat, xy + [dx, dy]) for dx in (-0.8, 0, 0.8) for dy in (-0.8, 0, 0.8)], 0)
        f = lambda P: P[..., 2] - h_mat(P.reshape(-1, 3)[:, :2]).reshape(P.shape[:-1])
        A = _wybierz(A, f(A[0]).min(1) < 0.3)  # cale wysoko nad scieciem - spalone

        def potrzeba(a, b):
            fa, fb, fm = f(a), f(b), f((a + b) / 2)
            return (np.minimum(np.minimum(fa, fb), fm) < 0.35) & (np.maximum(np.maximum(fa, fb), fm) > -0.35)
        A = _podziel(A, 0.5, potrzeba)
        A = _przytnij(A, f(A[0]))
        if not len(A[0]):
            continue
        pv = A[0].reshape(-1, 3)
        if mat == KAMIEN and kamienny:
            hv = np.maximum(h_mat(pv[:, :2]), 0.3)
            smuga = szum3(pv * [1, 1, 0.35], 1.4, ziarno + 3)
            t = 0.12 + 0.6 * _gladko(0.3, 1.0, pv[:, 2] / hv) * (0.4 + 0.6 * smuga)
            cel = SADZA
        else:  # drewno, tynk, strzecha: zweglone
            t = 0.68 + 0.3 * szum3(pv, 0.8, ziarno + 11)
            cel = WEGIEL
        t = (t * (pv[:, 2] > -0.02)).reshape(-1, 3)
        t = np.maximum(t, 0.8 * _wnetrze(A, obrys, 0.3))
        A[2] = _zabarw(A[2], cel, t)
        out[mat] = _z_tylem(A, 0.4)
    r = _z_zupy(s.nazwy, out)

    # kopiec gruzu i popiolu w obrysie
    dx, dy = x1 - x0, y1 - y0
    wk = float(np.clip(0.09 * min(dx, dy) + 0.4, 0.5, 1.4) if kamienny else np.clip(0.05 * min(dx, dy) + 0.2, 0.25, 0.6))

    def kopiec(x, y):
        u = (np.asarray(x) - (x0 + x1) / 2) / (dx / 2)
        v = (np.asarray(y) - (y0 + y1) / 2) / (dy / 2)
        rr = (np.abs(u) ** 3 + np.abs(v) ** 3) ** (1 / 3)
        p = np.stack([np.asarray(x, float), np.asarray(y, float), np.zeros(np.shape(x))], -1)
        nz = szum3(p, 1.4, ziarno + 21).reshape(np.shape(x))
        return wk * np.clip(1.08 - rr, 0, 1) ** 0.55 * (0.5 + 0.9 * nz) - 0.08

    nx = int(np.clip(dx / 0.45, 6, 40)); ny = int(np.clip(dy / 0.45, 6, 40))
    X, Y = np.meshgrid(np.linspace(x0 - 0.3, x1 + 0.3, nx + 1), np.linspace(y0 - 0.3, y1 + 0.3, ny + 1), indexing="ij")
    V = np.stack([X, Y, kopiec(X, Y)], -1)
    a, b, c, d = V[:-1, :-1], V[1:, :-1], V[1:, 1:], V[:-1, 1:]
    T = np.concatenate([np.stack([a, b, c], 2).reshape(-1, 3, 3), np.stack([a, c, d], 2).reshape(-1, 3, 3)])
    T = T[T[..., 2].max(1) > -0.02]
    nt = np.cross(T[:, 1] - T[:, 0], T[:, 2] - T[:, 0])
    nt /= np.linalg.norm(nt, axis=1, keepdims=True) + 1e-9
    pv = T.reshape(-1, 3)
    g = (_gladko(0.4, 0.75, szum3(pv, 0.7, ziarno + 31)) * 0.7)[:, None]
    kol = (MUR * 0.72 if kamienny else POPIOL) * (1 - g) + WEGIEL * g
    kol = kol * rng.uniform(0.85, 1.1, (len(pv), 1))
    r.dodaj(KAMIEN, pv, np.repeat(nt, 3, 0), kol, np.arange(len(pv)).reshape(-1, 3), np.stack([pv[:, 0], -pv[:, 1]], 1) / KAFEL[KAMIEN])

    na = lambda x, y: max(float(kopiec(x, y)), 0.0)
    pole = dx * dy
    # kamienie na kopcu i obok (rozsypane za obrys)
    for _ in range(int(np.clip(pole * (0.7 if kamienny else 0.2), 6, 220))):
        x = rng.uniform(x0 - 0.8, x1 + 0.8); y = rng.uniform(y0 - 0.8, y1 + 0.8)
        rk = rng.uniform(0.14, 0.42) * (1.0 if kamienny else 0.7)
        kier = rng.normal(size=3); kier[2] *= 0.4; kier /= np.linalg.norm(kier) + 1e-9
        p = np.array([x, y, na(x, y) + rk * 0.25])
        belka(r, KAMIEN, p - kier * rk * 0.6, p + kier * rk * 0.6, rk * rng.uniform(0.6, 1.0),
              (MUR if kamienny else MUR_COKOL) * rng.uniform(0.5, 0.9))
    # zweglone belki: lezace na kopcu, czesc opartych o resztki scian
    for _ in range(int(np.clip((dx + dy) * 1.1, 5, 60))):
        x = rng.uniform(x0, x1); y = rng.uniform(y0, y1)
        kat = rng.uniform(0, np.pi); dl = rng.uniform(1.4, min(4.2, max(dx, dy)))
        kd = np.array([np.cos(kat), np.sin(kat), 0.0])
        A_ = np.array([x, y, 0.0]) - kd * dl / 2; B_ = A_ + kd * dl
        A_[2] = na(A_[0], A_[1]) + 0.1 + rng.uniform(0, 0.2)
        B_[2] = na(B_[0], B_[1]) + 0.1 + (rng.uniform(0.6, 1.8) if rng.random() < 0.3 else rng.uniform(0, 0.25))
        belka(r, DREWNO, A_, B_, rng.uniform(0.15, 0.26), WEGIEL * rng.uniform(0.75, 1.35))
    # sterczace zweglone slupy w rogach (budynki drewniane)
    if not kamienny:
        for px, py in ((x0 + 0.3, y0 + 0.3), (x1 - 0.3, y0 + 0.3), (x1 - 0.3, y1 - 0.3), (x0 + 0.3, y1 - 0.3)):
            if rng.random() < 0.75:
                hs = rng.uniform(1.0, 2.8)
                belka(r, DREWNO, (px, py, -0.2), (px + rng.uniform(-0.25, 0.25), py + rng.uniform(-0.25, 0.25), hs),
                      rng.uniform(0.2, 0.28), WEGIEL * rng.uniform(0.8, 1.2))
    return r


def main():
    os.makedirs(OUT, exist_ok=True)
    for nazwa, tex in (("T_Bud_Kamien", tekstura_kamien()), ("T_Bud_Dach", tekstura_dach()), ("T_Bud_Drewno", tekstura_drewno()),
                       ("T_Bud_Tynk", tekstura_tynk()), ("T_Bud_Strzecha", tekstura_strzecha())):
        tex.save(os.path.join(OUT, nazwa + ".png"))
        print(nazwa + ".png")
    gotowe = {}
    for nazwa, (f, fazy) in BUDYNKI.items():
        for faza in range(1, fazy + 1):
            plik = os.path.join(OUT, f"{nazwa}_F{faza}.glb")
            s = f(faza)
            tr = s.zapisz(plik)
            assert 50 < tr < 60000, f"{plik}: {tr} trojkatow"
            print(f"{os.path.basename(plik)}: {tr} trojkatow")
        gotowe[nazwa] = s
    for nazwa, f in POJEDYNCZE.items():
        plik = os.path.join(OUT, nazwa + ".glb")
        s = f()
        tr = s.zapisz(plik)
        assert (10 if nazwa.startswith(("SM_Plot", "SM_Stos")) else 50) < tr < 60000, f"{plik}: {tr} trojkatow"
        print(f"{os.path.basename(plik)}: {tr} trojkatow")
        if nazwa in MURY_OSAD:
            gotowe[nazwa] = s
    zniszczenia(gotowe)


def zniszczenia(gotowe):
    for nazwa, s in gotowe.items():
        if nazwa in BEZ_ZNISZCZEN:
            continue
        ziarno = zlib.crc32(nazwa.encode()) % 10007
        for etap, z in ((1, uszkodzony(s, ziarno)), (2, ruina(s, nazwa, ziarno))):
            plik = os.path.join(OUT, f"{nazwa}_Z{etap}.glb")
            tr = z.zapisz(plik)
            assert 50 < tr < 400000, f"{plik}: {tr} trojkatow"
            print(f"{os.path.basename(plik)}: {tr} trojkatow")


if __name__ == "__main__":
    main()
