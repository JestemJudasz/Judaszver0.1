# Plik: Tools/efekty.py
# Efekty wojny (23.11.2026: plonace i walace sie budynki) -> Art/Efekty: tekstury i proste siatki czastek dla AEfekty
# (Source/grarts2d/Efekty). Wszystko robione tu, bez pobierania.
#   T_Efekt_Plomien  64 x 128 RGBA - jezyk ognia: jasny srodek u dolu, pomaranczowe i czerwone brzegi, ksztalt kropli
#   T_Efekt_Dym     128 x 128 RGBA - klab dymu: miekki brzeg, klebki z szumu, cieniowanie (jasniej z gory, od slonca)
#   T_Efekt_Szum     64 x  64      - szum kafelkowy (ruch wewnatrz plomienia i dymu w materiale)
#   T_Efekt_Kropka   32 x  32 RGBA - iskra / zar
#   SM_Efekt_Plomien - kwadrat 1 x 1 m frontem do +X, punkt zaczepienia u dolu (plomien rosnie w gore)
#   SM_Efekt_Kleks   - kwadrat 1 x 1 m frontem do +X, punkt zaczepienia w srodku (dym, kurz, iskry)
#   SM_Efekt_Gruz    - szescian 1 m (kamienie i belki lecace przy zawaleniu; kolor z danych instancji)
# Gra obraca kwadraty do kamery. Import: Tools/Zbuduj_Efekty.bat (Tools/ue_efekty.py robi materialy).
import os
import numpy as np
from PIL import Image
import budynki as b

OUT = os.path.join(b.ROOT, "Art", "Efekty")


def szum_kafel(n, okres, ziarno):
    """Szum wartosci kafelkowy (okres w komorkach siatki)."""
    rng = np.random.default_rng(ziarno)
    g = rng.random((okres, okres))
    x = np.arange(n) / n * okres
    i0 = np.floor(x).astype(int)
    f = x - i0
    f = f * f * (3 - 2 * f)
    i1 = (i0 + 1) % okres
    a = g[i0][:, i0] * (1 - f)[None, :] + g[i0][:, i1] * f[None, :]
    c = g[i1][:, i0] * (1 - f)[None, :] + g[i1][:, i1] * f[None, :]
    return a * (1 - f)[:, None] + c * f[:, None]


def fbm(n, ziarno):
    return sum(szum_kafel(n, 4 * 2 ** k, ziarno + k) * 0.5 ** k for k in range(4)) / 1.875


def zapisz(a, nazwa, tryb="RGBA"):
    Image.fromarray((np.clip(a, 0, 1) * 255).astype(np.uint8), tryb).save(os.path.join(OUT, nazwa + ".png"))
    print(nazwa + ".png")


def plomien():
    W, H = 64, 128
    u = (np.arange(W) + 0.5) / W * 2 - 1           # -1..1 w poprzek
    v = (np.arange(H) + 0.5) / H                    # 0 gora .. 1 dol
    U, V = np.meshgrid(u, v)
    t = 1 - V                                       # 0 dol .. 1 czubek
    szer = np.clip(0.92 * np.sin(np.clip(t * 1.15 + 0.12, 0, 1) * np.pi) ** 0.8 * (1 - t) ** 0.35, 0, 1) + 1e-3
    n = fbm(128, 11)[:H, :W]
    n2 = fbm(128, 12)[:H, :W]
    d = np.abs(U + 0.55 * (n2 - 0.5) * t ** 0.8) / szer  # poszarpane brzegi, wyzej bardziej
    ksztalt = np.clip(1 - d ** 1.6, 0, 1) * np.clip((1 - t) * 3.0, 0, 1) * np.clip(t * 8 + 0.2, 0, 1)
    jezyki = np.clip((n - 0.25 - 0.55 * t) * 3.0 + 0.6, 0, 1)  # u gory rozpada sie na jezyki
    alfa = np.clip(ksztalt * jezyki * 1.2, 0, 1)
    goraco = np.clip((1 - d) * (1 - t * 1.1) * 1.4, 0, 1)  # srodek u dolu najgoretszy
    rdzen, srodek, brzeg = np.array([1.0, 0.93, 0.62]), np.array([1.0, 0.52, 0.12]), np.array([0.78, 0.16, 0.03])
    k = np.where(goraco[..., None] > 0.5, srodek + (rdzen - srodek) * ((goraco[..., None] - 0.5) * 2),
                 brzeg + (srodek - brzeg) * (goraco[..., None] * 2))
    zapisz(np.concatenate([k, alfa[..., None]], -1), "T_Efekt_Plomien")


def dym():
    n = 128
    x = (np.arange(n) + 0.5) / n * 2 - 1
    X, Y = np.meshgrid(x, x)
    r = np.sqrt(X ** 2 + Y ** 2)
    f = fbm(n, 21)
    f2 = fbm(n, 22)
    klebki = np.clip((f + 0.45 * (1 - r) - 0.55) * 3.5 + 0.5, 0, 1)  # wyrazne klebki, zwarte w srodku
    alfa = np.clip((1 - r ** 2.2) * 1.3, 0, 1) * (0.35 + 0.65 * klebki) * (0.8 + 0.4 * f2)
    alfa = np.clip(alfa, 0, 1)
    swiatlo = np.clip(0.78 + 0.22 * (-X * 0.5 - Y * 0.8) + 0.18 * (f - 0.5), 0.45, 1.0)  # gora-lewo jasniej
    zapisz(np.concatenate([np.repeat(swiatlo[..., None], 3, -1), alfa[..., None]], -1), "T_Efekt_Dym")


def kropka():
    n = 32
    x = (np.arange(n) + 0.5) / n * 2 - 1
    X, Y = np.meshgrid(x, x)
    a = np.clip(1 - np.sqrt(X ** 2 + Y ** 2), 0, 1) ** 2.2
    zapisz(np.concatenate([np.ones((n, n, 3)), a[..., None]], -1), "T_Efekt_Kropka")


def szum():
    zapisz(fbm(64, 31), "T_Efekt_Szum", "L")


def kwadrat(nazwa, z0, mat):
    s = b.Siatka([mat])
    p = np.array([[0, -0.5, z0], [0, 0.5, z0], [0, 0.5, z0 + 1], [0, -0.5, z0 + 1]], float)
    uv = np.array([[0, 1], [1, 1], [1, 0], [0, 0]], float)
    s.dodaj(0, p, np.tile([1.0, 0, 0], (4, 1)), np.ones(3), [[0, 1, 2], [0, 2, 3]], uv)
    s.zapisz(os.path.join(OUT, nazwa + ".glb"))
    print(nazwa + ".glb")


def gruz():
    s = b.Siatka(["Gruz"])
    b.pudlo(s, 0, (-0.5, -0.5, -0.5), (0.5, 0.5, 0.5), np.ones(3), bez_dolu=False)
    s.zapisz(os.path.join(OUT, "SM_Efekt_Gruz.glb"))
    print("SM_Efekt_Gruz.glb")


def main():
    os.makedirs(OUT, exist_ok=True)
    plomien()
    dym()
    kropka()
    szum()
    kwadrat("SM_Efekt_Plomien", 0.0, "Ogien")
    kwadrat("SM_Efekt_Kleks", -0.5, "Dym")
    gruz()


if __name__ == "__main__":
    main()
