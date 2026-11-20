# Plik: Tools/tekstury_teren.py
# Tekstury terenu robione przez AI (bez zewnetrznych obrazkow):
#   Art/Teren/T_Europa_Kolor.png  8192x8192 - kolor bazowy calej Europy (pokrycie + glebokosc morza), 64 m w grze na piksel
#   Art/Teren/T_Detal.png         512x512, kafelkujaca - szum detalu w 3 kanalach (R drobny, G sredni, B duzy)
# Paleta zdjeta ze zrzutow referencyjnych (oliwkowe zielenie, zolte laki, wrzosowiska, ciemne lasy).
import os, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dem_europa import N, ROOT, MIN_M, MAX_M
from pokrycie_europa import szum

ART = os.path.join(ROOT, "Art", "Teren")
BOK_CM = 52428800.0       # FSwiat::Bok
POL_CM = BOK_CM / 2

# Klasy jak w pokrycie_europa.py: woda, laka, wrzos, lisc, igl, miesz, rzadki, mokr, skaly, snieg, tundra
PALETA = np.array([
    [56, 90, 104],    # 0 woda (jeziora, rzeki; morze liczone osobno z glebokosci)
    [118, 134, 66],   # 1 laka
    [112, 108, 78],   # 2 zarosla / wrzosowisko (Copernicus "shrubs": tez makia na poludniu - nie caly fiolet)
    [58, 84, 40],     # 3 las lisciasty (dno lasu z gory, zanim beda drzewa)
    [44, 66, 44],     # 4 las iglasty
    [52, 76, 42],     # 5 las mieszany
    [88, 108, 54],    # 6 las rzadki / skraj
    [92, 106, 76],    # 7 mokradlo
    [122, 114, 102],  # 8 skaly
    [232, 234, 238],  # 9 snieg
    [138, 136, 104],  # 10 tundra
], np.float32)
# Druga barwa klasy do mieszania plamami (laki zolkna, lasy ciemnieja) - daje "malarskie" plamy ze zrzutow.
PALETA2 = PALETA.copy()
PALETA2[1] = [150, 150, 78]    # laka sucha/zolta
PALETA2[2] = [120, 102, 102]   # laty kwitnacego wrzosu (stonowane)
PALETA2[3] = [50, 76, 36]
PALETA2[6] = [104, 118, 60]
PALETA2[8] = [104, 98, 90]


def kolor_europy():
    P = np.fromfile(os.path.join(ROOT, "Data", "europa_pokrycie.r8"), np.uint8).reshape(N, N)
    H = np.fromfile(os.path.join(ROOT, "Data", "europa_wys.r16"), "<u2").reshape(N, N).astype(np.float32) / 65535 * (MAX_M - MIN_M) + MIN_M
    W = np.fromfile(os.path.join(ROOT, "Data", "europa_woda.r8"), np.uint8).reshape(N, N)

    plamy = szum((N, N), 48, 7)[..., None]          # ~3 km w grze
    jasnosc = 0.92 + 0.16 * szum((N, N), 12, 8)[..., None]
    K = (PALETA[P] * (1 - plamy) + PALETA2[P] * plamy) * jasnosc

    # Wysokie gory: im wyzej, tym wiecej szarosci skal (las i laka zanikaja ku szczytom).
    gory = np.clip((H - 1600) / 900, 0, 1)[..., None]
    K = K * (1 - gory * 0.6) + PALETA[8] * gory * 0.6

    # Wode rysuje material z T_Europa_Woda (ostry brzeg). Tu pod woda rozlewamy kolory ladu, zeby rozmyta
    # krawedz tekstury nie barwila brzegu na niebiesko.
    woda = W > 0
    znane = ~woda
    for _ in range(12):
        suma = np.zeros_like(K); ile = np.zeros(K.shape[:2], np.float32)
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            m = np.roll(znane, (dy, dx), (0, 1))
            suma += np.roll(K, (dy, dx), (0, 1)) * m[..., None]; ile += m
        nowe = ~znane & (ile > 0)
        K[nowe] = suma[nowe] / ile[nowe][..., None]
        znane |= nowe
    K[~znane] = PALETA[1]

    img = Image.fromarray(np.clip(K, 0, 255).astype(np.uint8))
    return img.filter(ImageFilter.GaussianBlur(0.7))  # miekkie granice klas, bez rozmycia ksztaltow


def woda_europy():
    """T_Europa_Woda: R = odleglosc od brzegu ze znakiem (0,5 = brzeg, >0,5 woda, 1/16 na piksel, +-8 px),
    G = glebokosc morza 0..1 (0..100 m; jeziora i rzeki 0,35)."""
    H = np.fromfile(os.path.join(ROOT, "Data", "europa_wys.r16"), "<u2").reshape(N, N).astype(np.float32) / 65535 * (MAX_M - MIN_M) + MIN_M
    W = np.fromfile(os.path.join(ROOT, "Data", "europa_woda.r8"), np.uint8).reshape(N, N)
    woda = W > 0

    def odleglosc(maska):
        """Ile pikseli (do 8) od pikseli maski; same piksele maski = 0. Naprzemiennie 4- i 8-sasiedztwo ~ euklides."""
        d = np.where(maska, 0.0, 99.0).astype(np.float32)
        for k in range(8):
            sasiedzi = ((1, 0), (-1, 0), (0, 1), (0, -1)) + (((1, 1), (1, -1), (-1, 1), (-1, -1)) if k % 2 else ())
            for s in sasiedzi:
                d = np.minimum(d, np.roll(d, s, (0, 1)) + 1.0)
        return np.minimum(d, 8.0)

    # Morze i jeziora z pikseli; rzeki z linii (Data/rzeki.bin, Tools/rzeki.py) - gladkie, bez schodkow 64 m.
    woda = (W == 1) | (W == 2)
    do_ladu = odleglosc(~woda)    # dla pikseli wody
    do_wody = odleglosc(woda)     # dla pikseli ladu
    sdf = np.where(woda, do_ladu - 0.5, -(do_wody - 0.5))
    # Na mapach oddalonych rzeka ma 100 m szerokosci (2 x 50 m): wezsza niz piksel znikalaby miedzy srodkami pikseli.
    rzeka = SZER_RZEKI_DALEKO_PX - odleglosc_od_rzek()
    sdf = np.maximum(sdf, rzeka)
    R = np.clip(0.5 + sdf / 16.0, 0, 1)
    G = np.where(W == 1, np.clip(-H / 100.0, 0, 1), np.where(woda | (rzeka > -1), 0.35, 0.0))
    # B = rzeki map oddalonych z pasem blota: mapa grywalna je wylacza (ma wlasne, waskie rzeki z linii w UV2 kafli).
    B = ((rzeka > -1.5) & ~woda).astype(np.float32)
    return Image.fromarray((np.stack([R, G, B], -1) * 255 + 0.5).astype(np.uint8))


SZER_RZEKI_DALEKO_PX = 50.0 / 64.0  # polowa szerokosci rzeki na mapach oddalonych (50 m w grze) w pikselach mapy


def odleglosc_od_rzek(zasieg=8):
    """Dokladna odleglosc (w pikselach mapy, do `zasieg`) od linii rzek z Data/rzeki.bin."""
    import struct
    b = open(os.path.join(ROOT, "Data", "rzeki.bin"), "rb").read()
    o, A, Bk = 4, [], []
    for _ in range(struct.unpack_from("<i", b, 0)[0]):
        k, _pol = struct.unpack_from("<if", b, o); o += 8
        p = np.frombuffer(b, "<f4", k * 2, o).reshape(-1, 2).astype(np.float64); o += k * 8
        # cm gry -> piksele mapy: kolumna = (Y + polowa) / bok * N, wiersz = (polowa - X) / bok * N
        px = np.c_[(p[:, 1] + POL_CM) / BOK_CM * N, (POL_CM - p[:, 0]) / BOK_CM * N]
        A.append(px[:-1]); Bk.append(px[1:])
    A, Bk = np.concatenate(A), np.concatenate(Bk)
    AB = Bk - A
    dl2 = np.maximum((AB ** 2).sum(1), 1e-12)
    D = np.full((N, N), float(zasieg), np.float32)
    c0, r0 = np.floor(A[:, 0]).astype(np.int64), np.floor(A[:, 1]).astype(np.int64)
    for dr in range(-zasieg - 1, zasieg + 2):
        for dc in range(-zasieg - 1, zasieg + 2):
            c, r = c0 + dc, r0 + dr
            ok = (c >= 0) & (c < N) & (r >= 0) & (r < N)
            P = np.c_[c + 0.5, r + 0.5] - A  # srodek piksela wzgledem poczatku odcinka
            t = np.clip((P * AB).sum(1) / dl2, 0, 1)
            d = np.hypot(*(P - AB * t[:, None]).T).astype(np.float32)
            np.minimum.at(D, (r[ok], c[ok]), d[ok])
    return D


def szum_kafelkujacy(n, f_min, f_max, ziarno):
    """Szum pasmowy z FFT - z natury powtarza sie bez szwow."""
    rng = np.random.default_rng(ziarno)
    F = np.fft.fft2(rng.standard_normal((n, n)))
    fy, fx = np.meshgrid(np.fft.fftfreq(n) * n, np.fft.fftfreq(n) * n, indexing="ij")
    f = np.hypot(fx, fy)
    s = np.real(np.fft.ifft2(F * ((f >= f_min) & (f <= f_max)) / np.maximum(f, 1)))
    s = (s - s.mean()) / (s.std() * 4) + 0.5
    return np.clip(s, 0, 1)


def detal():
    n = 512
    r = szum_kafelkujacy(n, 24, 160, 11)   # kepki trawy
    g = szum_kafelkujacy(n, 4, 24, 12)     # nierownosci, laty
    b = szum_kafelkujacy(n, 1, 6, 13)      # duze plamy (tez do falowania granic)
    return Image.fromarray((np.stack([r, g, b], -1) * 255).astype(np.uint8))


def trawa(n=512, ziarno=21):
    """T_Trawa: kafelkujaca tekstura mapy grywalnej (kafel 16 m), styl zrzutow wzorcowych:
    R = zdzbla (krotkie pionowe pociagniecia rozjasniajace/sciemniajace, w kierunku polnocy = gora ekranu),
    G = druga, niezalezna warstwa zdzbel (laczona w innej skali - mniej widac powtarzanie),
    B = rzadkie kropki (kwiatki na lace, kamyczki na brzegu)."""
    rng = np.random.default_rng(ziarno)

    def warstwa(ile):
        img = Image.new("L", (n, n), 128)
        d = ImageDraw.Draw(img)
        for _ in range(ile):
            x, y = rng.random() * n, rng.random() * n
            dl, sz = rng.uniform(18, 34), int(rng.integers(5, 9))
            nach = rng.uniform(-0.35, 0.35) * dl
            v = int(np.clip(rng.normal(128, 55), 20, 240))
            for ox in (-n, 0, n):        # kopie przesuniete o bok - tekstura bez szwow
                for oy in (-n, 0, n):
                    # Obraz: x = U = polnoc. Zdzblo rosnie w +x (gora ekranu), lekko pochylone.
                    d.line([(x + ox, y + oy), (x + ox + dl, y + oy + nach)], fill=v, width=sz)
        return np.asarray(img.filter(ImageFilter.GaussianBlur(0.6)))

    r, g = warstwa(9000), warstwa(9000)
    b = np.zeros((n, n), np.uint8)
    img = Image.fromarray(b)
    d = ImageDraw.Draw(img)
    for _ in range(500):
        x, y, rr = rng.random() * n, rng.random() * n, rng.uniform(2.5, 4.5)
        for ox in (-n, 0, n):
            for oy in (-n, 0, n):
                d.ellipse([x + ox - rr, y + oy - rr, x + ox + rr, y + oy + rr], fill=255)
    return Image.fromarray(np.stack([r, g, np.asarray(img)], -1))


def mapa(n=512, ziarno=31):
    """T_Mapa: kafelkujaca tekstura mapy strategicznej (pergamin), skala ustalana na ekranie (M_Teren, SkalaMapy):
    R = drzewka lasu (0,5 tlo; jasna korona z ciemnym cieniem w prawo w dol), 8x8 drzewek na kafel, rozrzucone,
    G = faktura papieru (wlokna i plamy), B = 0."""
    rng = np.random.default_rng(ziarno)
    img = Image.new("L", (n, n), 128)
    d = ImageDraw.Draw(img)
    oczko = n / 8
    for i in range(8):
        for j in range(8):
            x = (i + 0.5 + rng.uniform(-0.3, 0.3)) * oczko
            y = (j + 0.5 + rng.uniform(-0.3, 0.3)) * oczko
            r = oczko * rng.uniform(0.26, 0.34)
            for ox in (-n, 0, n):        # kopie przesuniete o bok - bez szwow
                for oy in (-n, 0, n):
                    cx, cy = x + ox, y + oy
                    d.ellipse([cx - r + 3, cy - r + 4, cx + r + 3, cy + r + 4], fill=55)    # cien
                    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=190)                    # korona
                    d.ellipse([cx - r * 0.45 - 2, cy - r * 0.45 - 3, cx + r * 0.2, cy + r * 0.2], fill=215)  # blik
    r = np.asarray(img.filter(ImageFilter.GaussianBlur(1.0)))
    g = 0.6 * szum_kafelkujacy(n, 2, 12, 32) + 0.4 * szum_kafelkujacy(n, 40, 200, 33)
    return Image.fromarray(np.stack([r, (g * 255).astype(np.uint8), np.zeros((n, n), np.uint8)], -1))


def main():
    os.makedirs(ART, exist_ok=True)
    if sys.argv[1:] == ["mapa"]:  # tylko T_Mapa
        m = mapa()
        assert 100 < np.asarray(m)[..., 0].mean() < 160, "drzewka: zla jasnosc tla"
        m.save(os.path.join(ART, "T_Mapa.png"))
        print("T_Mapa.png")
        return
    if sys.argv[1:] == ["trawa"]:  # tylko T_Trawa (bez przeliczania mapy Europy)
        trawa().save(os.path.join(ART, "T_Trawa.png"))
        print("T_Trawa.png")
        return
    if sys.argv[1:] == ["woda"]:  # tylko T_Europa_Woda
        woda_europy().save(os.path.join(ART, "T_Europa_Woda.png"))
        print("T_Europa_Woda.png")
        return
    d = detal()
    d.save(os.path.join(ART, "T_Detal.png"))
    # Ta sama tekstura dla C++ (FSwiat::Pokrycie faluje granice klas tak samo jak M_Teren - drzewa trzymaja sie koloru terenu).
    np.asarray(d).tofile(os.path.join(ROOT, "Data", "teren_detal.rgb8"))
    print("T_Detal.png + Data/teren_detal.rgb8")
    woda_europy().save(os.path.join(ART, "T_Europa_Woda.png"))
    print("T_Europa_Woda.png")
    k = kolor_europy()
    k.save(os.path.join(ART, "T_Europa_Kolor.png"))
    k.resize((1024, 1024), Image.BILINEAR).save(os.path.join(ROOT, "Docs", "europa_kolor_podglad.png"))
    print("T_Europa_Kolor.png GOTOWE")


if __name__ == "__main__":
    main()
