# Plik: Tools/drogi.py
# Tekstury drog (21.11.2026) -> Art/Drogi/:
#   T_Droga.png             512 x 512 (4 m), kafelkujaca: ubita ziemia z plamami, kamykami i peknieciami (jasnosc 0,6-1,0;
#                           kolor daje wierzcholek drogi), alfa = szum poszarpanego, falistego brzegu (maska M_Droga). 23.11.2026
#   T_Ikona_Znak_Droga.png  znak przycisku "Drogi" na dolnym pasku (styl znakow z Tools/ikony.py).
# Uruchamiane z Tools/Zbuduj_Drogi.bat (potem import Tools/ue_drogi.py).
import os
import numpy as np
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(ROOT, "Art", "Drogi")
rng = np.random.default_rng(7)


def szum(n, skala):
    """Kafelkujacy szum (filtr w dziedzinie czestotliwosci), 0..1."""
    f = np.fft.fft2(rng.standard_normal((n, n)))
    k = np.fft.fftfreq(n)[:, None] ** 2 + np.fft.fftfreq(n)[None, :] ** 2
    s = np.real(np.fft.ifft2(f * np.exp(-k * (n / skala) ** 2)))
    s -= s.min()
    return s / max(s.max(), 1e-6)


def tekstura():
    """Ubita ziemia 512 px (4 m): jasnosc 0,6-1,0 z plamami ubitej i wilgotniejszej ziemi, cieple/szarawe odcienie,
    kamyki ze swiatlem i cieniem, drobne pekniecia. Alfa = szum brzegu (grube fale ok. 1 m + drobne piksele) dla maski M_Droga."""
    n = 512
    v = 0.55 * szum(n, 8) + 0.30 * szum(n, 40) + 0.15 * szum(n, 160)
    v = 0.64 + 0.34 * v
    u = np.clip((szum(n, 12) - 0.5) / 0.3, 0, 1)
    v *= 1.0 - 0.1 * u * u * (3 - 2 * u)  # ubite, ciemniejsze placki z miekkim brzegiem
    odcien = szum(n, 5)
    cieply, szary, zielony = np.array([1.0, 0.94, 0.84]), np.array([0.96, 0.95, 0.92]), np.array([0.93, 0.97, 0.84])
    kol = cieply * (1 - odcien[..., None]) + szary * odcien[..., None]
    trawka = np.clip((szum(n, 18) - 0.7) * 4, 0, 1)[..., None] * 0.6  # tu i owdzie zielonkawy nalot
    kol = kol * (1 - trawka) + zielony * trawka
    rgb = v[..., None] * kol
    # pekniecia: krotkie bladzenia, ciemniejsze o 25%
    for _ in range(26):
        x, y = rng.uniform(0, n, 2)
        kat = rng.uniform(0, 2 * np.pi)
        for _ in range(int(rng.integers(12, 40))):
            rgb[int(y) % n, int(x) % n] *= 0.75
            kat += rng.normal(0, 0.5)
            x += np.cos(kat); y += np.sin(kat)
    # kamyki 1-3 px: jasny wierzch, cien pod spodem, czesc szara
    for _ in range(900):
        x, y = rng.integers(0, n, 2)
        r = int(rng.integers(1, 4))
        jas = rng.uniform(0.85, 1.0)
        k = np.array([1.0, 0.97, 0.9]) if rng.random() < 0.6 else np.array([0.92, 0.92, 0.9])
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                if dx * dx + dy * dy <= r * r:
                    rgb[(y + dy) % n, (x + dx) % n] = k * jas * (1.0 - 0.12 * (dy + dx) / max(r, 1))
        for d in range(1, 3):
            rgb[(y + r + d - 1) % n, (x + d) % n] *= 0.78
    alfa = 0.65 * szum(n, 4) + 0.35 * szum(n, 60)
    alfa = (alfa - alfa.min()) / (alfa.max() - alfa.min())
    rgba = np.concatenate([np.clip(rgb, 0, 1), alfa[..., None]], -1)
    return Image.fromarray((rgba * 255).astype(np.uint8), "RGBA")


JASNY = (234, 218, 182, 255)
CIEN = (172, 148, 104, 255)
OBRYS = (30, 22, 15, 235)


def znak():
    im = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    # kreta droga w perspektywie: szeroka u dolu, waska u gory, z koleinami
    lewa = [(3, 31), (6, 24), (12, 18), (12, 12), (9, 7), (12, 2)]
    prawa = [(28, 31), (22, 24), (19, 18), (18, 12), (15, 7), (16, 2)]
    d.polygon(lewa + prawa[::-1], fill=JASNY)
    srodek = [((a[0] + b[0]) / 2, (a[1] + b[1]) / 2) for a, b in zip(lewa, prawa)]
    for i in range(len(srodek) - 1):
        d.line([srodek[i], srodek[i + 1]], fill=CIEN, width=2 if i < 2 else 1)
    px = im.load()
    o = Image.new("RGBA", im.size, (0, 0, 0, 0))
    for y in range(32):
        for x in range(32):
            if px[x, y][3] == 0 and any(0 <= x + dx < 32 and 0 <= y + dy < 32 and px[x + dx, y + dy][3] > 0
                                        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (-1, -1), (1, -1), (-1, 1))):
                o.putpixel((x, y), OBRYS)
    o.alpha_composite(im)
    return o.resize((128, 128), Image.NEAREST)


def main():
    os.makedirs(ART, exist_ok=True)
    tekstura().save(os.path.join(ART, "T_Droga.png"))
    znak().save(os.path.join(ART, "T_Ikona_Znak_Droga.png"))
    print("Drogi: tekstury gotowe")


if __name__ == "__main__":
    main()
