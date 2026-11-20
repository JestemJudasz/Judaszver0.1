# Plik: Tools/natura_drzewa.py
# Drzewa i krzaki robione przez AI -> Art/Natura/*.glb + tekstura lisci Art/Natura/T_Liscie.png.
# Styl ze zrzutow uzytkownika: korona z wielu kep drobnych lisci (karty z tekstura lisci i przeswitami), ciemne wnetrze,
# jasna gora, widoczne pnie i galezie. Normalne kart kieruja sie od srodka korony - korona swieci jak bryla, nie jak plaskie karty.
# Dwa materialy w kazdym modelu: "Kora" (pien, galezie - kolor wierzcholkow) i "Liscie" (karty z T_Liscie, maskowane).
# Rozmiary prawdziwe (metry; glTF: Y = gora). Import do UE: Tools/Zbuduj_Nature.bat.
import json, os, struct, zlib
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Art", "Natura")
KORA, LISCIE = 0, 1


# ---------------- tekstura lisci: lewa polowa = liscie, prawa = igly ----------------

def tekstura_lisci(n=256, ss=4):
    rng = np.random.default_rng(7)
    N = n * ss
    obraz = Image.new("RGBA", (2 * N, N), (0, 0, 0, 0))
    d = ImageDraw.Draw(obraz)

    def jasnosc(y, x0):
        # Gora kepy jasniejsza (slonce), dol ciemniejszy; male roznice miedzy liscmi (duze dawaly cytrynowe plamki).
        return 0.66 + 0.24 * (1 - y / N) + rng.uniform(-0.08, 0.06)

    def kolor(j, zolty):
        j = float(np.clip(j, 0.25, 1.0))
        return (int(255 * j * (1 + 0.12 * zolty)), int(255 * j), int(255 * j * (1 - 0.25 * zolty)), 255)

    # Pelne kepy jak na zrzucie wzorcowym: ciemniejszy, nieprzezroczysty srodek (bez dziur), liscie tylko na obrysie.
    for x0, r in ((N / 2, 0.32), (N + N / 2, 0.28)):
        d.ellipse([x0 - r * N, N / 2 - r * N, x0 + r * N, N / 2 + r * N], fill=kolor(0.78, 0))  # 0.62 dawalo prawie czarne srodki koron w grze

    # Liscie: ~900, gesciej w srodku, rzadziej na brzegu (poszarpany obrys kepy).
    for _ in range(900):
        r = N * 0.46 * np.sqrt(rng.random()) ** 0.6
        a = rng.random() * 2 * np.pi
        cx, cy = N / 2 + np.cos(a) * r, N / 2 + np.sin(a) * r
        dl, sz = rng.uniform(0.07, 0.10) * N, rng.uniform(0.03, 0.042) * N
        kat = rng.random() * np.pi
        t = np.linspace(0, 2 * np.pi, 14, endpoint=False)
        px = np.cos(t) * dl / 2
        py = np.sin(t) * sz / 2 * (1 - 0.35 * np.cos(t))  # lisc szerszy u nasady
        pts = [(cx + x * np.cos(kat) - y * np.sin(kat), cy + x * np.sin(kat) + y * np.cos(kat)) for x, y in zip(px, py)]
        d.polygon(pts, fill=kolor(jasnosc(cy, 0), 0.3 * (rng.random() < 0.06)))

    # Igly: galazki z grubymi kreskami na boki (cienkie znikaly z odleglosci - iglaki wygladaly jak suche patyki).
    for k in range(24):
        y0 = N * (0.18 + 0.64 * (k + rng.random()) / 24)  # rowno od gory do dolu - pokrywa caly krazek
        x0 = N + N * rng.uniform(0.3, 0.7)
        kat = rng.uniform(-0.4, 0.4) + (np.pi if rng.random() < 0.5 else 0)
        dl = N * rng.uniform(0.3, 0.45)
        for k in range(26):
            s = k / 26 * dl
            bx, by = x0 + np.cos(kat) * s - np.cos(kat) * dl / 2, y0 + np.sin(kat) * s
            for strona in (-1, 1):
                ka = kat + strona * rng.uniform(0.7, 1.1)
                l = N * rng.uniform(0.05, 0.08)
                d.line([(bx, by), (bx + np.cos(ka) * l, by + np.sin(ka) * l)], fill=kolor(jasnosc(by, 0) * 0.9, False), width=ss * 4)

    obraz = obraz.resize((2 * n, n), Image.LANCZOS)
    a = np.asarray(obraz).astype(np.float32)
    # Pod przezroczystymi pikselami srednia barwa (bez ciemnej obwodki w mipmapach).
    alfa = a[..., 3:4] / 255
    sr = (a[..., :3] * alfa).sum((0, 1)) / max(alfa.sum(), 1)
    a[..., :3] = np.where(alfa > 0.02, a[..., :3], sr)
    return Image.fromarray(a.astype(np.uint8), "RGBA")


# ---------------- siatka i zapis glTF ----------------

class Siatka:
    def __init__(self):
        self.grupy = {KORA: [], LISCIE: []}

    def dodaj(self, grupa, p, n, c, i, uv=None):
        p = np.asarray(p, float)
        uv = np.zeros((len(p), 2)) if uv is None else np.asarray(uv, float)
        self.grupy[grupa].append((p, np.asarray(n, float), np.asarray(c, float), np.asarray(i, np.int64), uv))

    def zapisz(self, plik):
        bloby, widoki, akcesory, prymitywy, tr = [], [], [], [], 0
        def blob(arr, typ, komp, minmax=False):
            b = arr.tobytes(); off = sum(len(x) for x in bloby)
            bloby.append(b + b"\0" * (-len(b) % 4))
            widoki.append({"buffer": 0, "byteOffset": off, "byteLength": len(b)})
            a = {"bufferView": len(widoki) - 1, "componentType": komp, "count": len(arr), "type": typ}
            if minmax:
                a["min"], a["max"] = arr.min(0).tolist(), arr.max(0).tolist()
            akcesory.append(a)
            return len(akcesory) - 1
        for grupa, czesci in self.grupy.items():
            if not czesci:
                continue
            baza, P, Nn, C, I, UV = 0, [], [], [], [], []
            for p, n, c, i, uv in czesci:
                P.append(p); Nn.append(n); C.append(c); I.append(i + baza); UV.append(uv); baza += len(p)
            P = np.concatenate(P).astype(np.float32); Nn = np.concatenate(Nn).astype(np.float32)
            Nn /= np.linalg.norm(Nn, axis=1, keepdims=True) + 1e-9
            C = np.clip(np.concatenate(C), 0, 1).astype(np.float32)
            C4 = np.concatenate([C, np.ones((len(C), 1), np.float32)], 1)
            I = np.concatenate(I).astype(np.uint32).ravel(); UV = np.concatenate(UV).astype(np.float32)
            tr += len(I) // 3
            prymitywy.append({"attributes": {"POSITION": blob(P, "VEC3", 5126, True), "NORMAL": blob(Nn, "VEC3", 5126),
                                             "COLOR_0": blob(C4, "VEC4", 5126), "TEXCOORD_0": blob(UV, "VEC2", 5126)},
                              "indices": blob(I, "SCALAR", 5125), "material": grupa})
        bufor = b"".join(bloby)
        gltf = {
            "asset": {"version": "2.0", "generator": "grarts2d natura_drzewa.py"},
            "scene": 0, "scenes": [{"nodes": [0]}],
            "nodes": [{"mesh": 0, "name": os.path.splitext(os.path.basename(plik))[0]}],
            "materials": [{"name": "Kora"}, {"name": "Liscie", "alphaMode": "MASK", "doubleSided": True}],
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


def rgb(r, g, b):
    return np.array([r, g, b], float) / 255.0


def lin(c):
    c = np.clip(c, 0, 1)
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


# ---------------- czesci drzewa ----------------

def walec(s, rng, a, b, r0, r1, kolor, brzoza=False, boki=6):
    """Pien/galaz od punktu a do b (zwezajaca sie)."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    os_ = b - a; dl = np.linalg.norm(os_); os_ /= dl
    u = np.cross(os_, [0, 0, 1] if abs(os_[1]) > 0.9 else [0, 1, 0]); u /= np.linalg.norm(u)
    w = np.cross(os_, u)
    pierscienie = 3 if dl > 3 else 1
    p, n, c, i = [], [], [], []
    for k in range(pierscienie + 1):
        t = k / pierscienie; r = r0 + (r1 - r0) * t
        for j in range(boki):
            kat = 2 * np.pi * j / boki
            kier = np.cos(kat) * u + np.sin(kat) * w
            p.append(a + os_ * dl * t + kier * r); n.append(kier)
            kol = kolor * rng.uniform(0.75, 1.0) * (0.55 + 0.45 * t)  # u dolu ciemniej (cien korony)
            if brzoza and rng.random() < 0.3:
                kol = rgb(45, 42, 38)
            c.append(lin(kol))
    for k in range(pierscienie):
        for j in range(boki):
            a0, a1 = k * boki + j, k * boki + (j + 1) % boki
            i += [(a0, a1, a0 + boki), (a1, a1 + boki, a0 + boki)]  # przeciwnie do zegara z zewnatrz (glTF)
    s.dodaj(KORA, p, n, c, i)


def kepa(s, rng, srodek, korona_sr, korona_r, ile, rozmiar, kolor, igly=False, opadanie=0.0):
    """Kepa lisci: kilka kart (kwadratow) wokol srodka, obroconych losowo."""
    for _ in range(ile):
        sr = srodek + rng.normal(0, rozmiar * 0.28, 3)
        nor = rng.normal(0, 1, 3); nor /= np.linalg.norm(nor)
        if opadanie:
            nor = np.array([nor[0] * 0.4, 1.0, nor[2] * 0.4]); nor /= np.linalg.norm(nor)  # galazki swierka prawie poziome
        u = np.cross(nor, [0, 1, 0] if abs(nor[1]) < 0.9 else [1, 0, 0]); u /= np.linalg.norm(u)
        w = np.cross(nor, u)
        kat = rng.random() * 2 * np.pi
        u, w = np.cos(kat) * u + np.sin(kat) * w, -np.sin(kat) * u + np.cos(kat) * w
        h = rozmiar * rng.uniform(0.8, 1.2) / 2
        rogi = [sr - u * h - w * h, sr + u * h - w * h, sr + u * h + w * h, sr - u * h + w * h]
        if opadanie:
            rogi = [r - np.array([0, opadanie * h * (np.dot(r - sr, u) / h) ** 2, 0]) for r in rogi]
        u0 = 0.5 if igly else 0.0
        uv = [(u0, 1), (u0 + 0.5, 1), (u0 + 0.5, 0), (u0, 0)]
        # Normalne od srodka korony (bryla), lekko w gore.
        p = np.array(rogi)
        n = (p - korona_sr) / korona_r[None, :]
        n = n / (np.linalg.norm(n, axis=1, keepdims=True) + 1e-9) + np.array([0, 0.35, 0])
        # Zaciemnienie: wnetrze i spod korony ciemne, zewnetrze i gora jasne.
        zew = np.clip(np.linalg.norm((p - korona_sr) / korona_r[None, :], axis=1), 0, 1.2)
        gora = np.clip((p[:, 1] - (korona_sr[1] - korona_r[1])) / (2 * korona_r[1]), 0, 1)
        ao = np.clip(0.66 + 0.2 * zew + 0.16 * gora, 0.65, 1.0)  # lagodnie: wnetrze korony zielone, nie czarne
        k = kolor * rng.uniform(0.85, 1.12)
        c = lin(k[None, :] * ao[:, None])
        s.dodaj(LISCIE, p, n, c, [(0, 1, 2), (0, 2, 3)], uv)


def punkt_w_elipsoidzie(rng, sr, r, powloka=0.55):
    """Losowy punkt blizej powierzchni elipsoidy (kepy na zewnatrz, wnetrze puste - jak prawdziwa korona)."""
    d = rng.normal(0, 1, 3); d /= np.linalg.norm(d)
    return sr + d * r * rng.uniform(powloka, 1.0)


def lisciaste(ziarno, pien_h, pien_r, kora, korona_y, korona_r, kep, kart, rozmiar, kolor, brzoza=False, galezi=5):
    rng = np.random.default_rng(ziarno); s = Siatka()
    korona_sr = np.array([0, korona_y, 0]); korona_r = np.asarray(korona_r, float)
    szczyt = np.array([rng.normal(0, 0.15), korona_y + korona_r[1] * 0.5, rng.normal(0, 0.15)])
    walec(s, rng, [0, -0.3, 0], szczyt, pien_r, pien_r * 0.35, kora, brzoza)
    kepy = [punkt_w_elipsoidzie(rng, korona_sr, korona_r) for _ in range(kep)]
    # Galezie od pnia do kilku kep (widac je w przeswitach).
    for k in kepy[:galezi]:
        start = np.array([0, rng.uniform(pien_h, korona_y), 0])
        walec(s, rng, start, k * 0.8 + start * 0.2, pien_r * 0.35, pien_r * 0.12, kora, brzoza, boki=4)
    for k in kepy:
        kepa(s, rng, k, korona_sr, korona_r, kart, rozmiar, kolor)
    return s


def sosna(ziarno):
    rng = np.random.default_rng(ziarno); s = Siatka()
    kora = rgb(150, 92, 58)
    korona_sr = np.array([0, 14.0, 0]); korona_r = np.array([3.4, 2.2, 3.4])
    walec(s, rng, [0, -0.3, 0], [rng.normal(0, 0.3), 15.0, rng.normal(0, 0.3)], 0.28, 0.1, kora)
    for _ in range(16):
        k = punkt_w_elipsoidzie(rng, korona_sr, korona_r, 0.3)
        walec(s, rng, [0, rng.uniform(11, 14), 0], k, 0.1, 0.04, kora, boki=4)
        kepa(s, rng, k, korona_sr, korona_r, 8, 2.2, rgb(46, 70, 40), igly=True)
    return s


def swierk(ziarno):
    rng = np.random.default_rng(ziarno); s = Siatka()
    wys = rng.uniform(16, 19); r = 3.3
    korona_sr = np.array([0, wys * 0.5, 0]); korona_r = np.array([r, wys * 0.5, r])
    walec(s, rng, [0, -0.3, 0], [0, wys, 0], 0.3, 0.05, rgb(80, 60, 45))
    pieter = 14
    for k in range(pieter):
        y = wys * (0.1 + 0.88 * k / pieter)
        rr = r * (1 - k / pieter) ** 0.9 + 0.3
        for j in range(5 + int(6 * (1 - k / pieter))):
            kat = rng.random() * 2 * np.pi
            srodek = np.array([np.cos(kat) * rr * 0.55, y, np.sin(kat) * rr * 0.55])
            kepa(s, rng, srodek, korona_sr, korona_r, 3, rr * 1.0 + 0.8, rgb(36, 60, 42), igly=True, opadanie=0.5)
    return s


# ---------------- drobiazgi mapy grywalnej (tylko grupa KORA: M_Natura_Kora mnozy kolor x1,2-1,6) ----------------

def kamien(ziarno):
    """Kamien 0,4-0,9 m: splaszczona, zaszumiona kula, jasny wierzch, ciemny spod."""
    rng = np.random.default_rng(ziarno); s = Siatka()
    segm, pier = 10, 7
    r = np.array([rng.uniform(0.25, 0.45), rng.uniform(0.15, 0.3), rng.uniform(0.2, 0.4)])
    szary = rgb(118, 114, 106) * rng.uniform(0.85, 1.1)
    p, n, c, i = [], [], [], []
    for k in range(pier + 1):
        th = np.pi * k / pier
        for j in range(segm):
            fi = 2 * np.pi * j / segm
            d = np.array([np.sin(th) * np.cos(fi), np.cos(th), np.sin(th) * np.sin(fi)])
            q = d * r * rng.uniform(0.85, 1.12)
            q[1] = max(q[1], -0.1)  # spod wkopany w ziemie
            p.append(q); n.append(d / r)
            c.append(lin(szary * (0.55 + 0.45 * (d[1] + 1) / 2) * rng.uniform(0.9, 1.05)))
    for k in range(pier):
        for j in range(segm):
            a0, a1 = k * segm + j, k * segm + (j + 1) % segm
            i += [(a0, a1, a0 + segm), (a1, a1 + segm, a0 + segm)]
    s.dodaj(KORA, p, n, c, i)
    return s


def trawa(ziarno):
    """Kepka trawy ~0,8 m: 18 zdzbel (waskie trojkaty rozchylone na boki), z obu stron - bez dziur pod katem."""
    rng = np.random.default_rng(ziarno); s = Siatka()
    for _ in range(18):
        kat = rng.random() * 2 * np.pi
        kier = np.array([np.cos(kat), 0, np.sin(kat)])
        bok = np.array([-np.sin(kat), 0, np.cos(kat)]) * rng.uniform(0.05, 0.08)
        podst = kier * rng.uniform(0, 0.15)
        szczyt = podst + kier * rng.uniform(0.25, 0.5) + np.array([0, rng.uniform(0.45, 0.85), 0])
        p = [podst - bok, podst + bok, szczyt]
        jasn = rng.uniform(0.85, 1.15)
        c = [lin(rgb(52, 76, 34) * jasn)] * 2 + [lin(rgb(108, 128, 52) * jasn)]
        nor = np.cross(p[1] - p[0], p[2] - p[0]); nor = nor / np.linalg.norm(nor) + np.array([0, 0.8, 0])
        s.dodaj(KORA, p, [nor] * 3, c, [(0, 2, 1)])
        s.dodaj(KORA, p, [nor] * 3, c, [(0, 1, 2)])
    return s


DROBNE = {"SM_Kamien": kamien, "SM_Trawa": trawa}  # warianty 1-2 jak drzewa


GATUNKI = {
    # pien_h, pien_r, kora, korona_y, korona_r, kep, kart na kepe, rozmiar karty, kolor lisci
    "SM_Dab":    lambda z: lisciaste(z, 3.0, 0.38, rgb(85, 72, 58), 8.5, (5.5, 4.0, 5.5), 42, 7, 2.8, rgb(66, 92, 40), galezi=7),
    "SM_Buk":    lambda z: lisciaste(z, 4.0, 0.3, rgb(120, 116, 104), 10.5, (4.5, 5.2, 4.5), 40, 7, 2.6, rgb(74, 104, 42), galezi=6),
    "SM_Brzoza": lambda z: lisciaste(z, 5.0, 0.17, rgb(230, 226, 214), 10.0, (2.6, 4.6, 2.6), 32, 6, 1.9, rgb(98, 126, 52), brzoza=True, galezi=6),
    "SM_Sosna":  sosna,
    "SM_Swierk": swierk,
    "SM_Krzak":  lambda z: lisciaste(z, 0.1, 0.06, rgb(70, 60, 45), 0.9, (1.6, 0.9, 1.6), 14, 6, 1.3,rgb(62, 88, 38), galezi=0),
}


def main():
    os.makedirs(OUT, exist_ok=True)
    tex = tekstura_lisci()
    tex.save(os.path.join(OUT, "T_Liscie.png"))
    print("T_Liscie.png")
    for nazwa, f in {**GATUNKI, **DROBNE}.items():
        for wariant in (1, 2):
            plik = os.path.join(OUT, f"{nazwa}_{wariant}.glb")
            tr = f(zlib.crc32(f"{nazwa}{wariant}".encode())).zapisz(plik)
            assert 30 < tr < 5000, f"{plik}: {tr} trojkatow"
            print(f"{os.path.basename(plik)}: {tr} trojkatow")


if __name__ == "__main__":
    main()
