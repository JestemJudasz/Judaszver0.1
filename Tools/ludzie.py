# Plik: Tools/ludzie.py
# Ludziki (osadnicy, pozniej wojsko) -> Art/Ludzie/SM_Ludz_*.glb. Decyzja uzytkownika 20.11.2026: proporcje realistyczne,
# w grze powiekszone x1,4 (ALudzik::Skala), bo przy 1,7 m ludzik ma na ekranie kilka pikseli.
# Ludzik sklada sie z czesci (sztywnych), ktore gra rusza osobno (ALudzik): kazda czesc ma poczatek ukladu w stawie.
#   SM_Ludz_Biodra  - od biodra (0,0,0): tunika do kolan, pasek; pas (staw klatki) na 0.08
#   SM_Ludz_Klatka  - od pasa: piers, barki; staw szyi (0,0,0.42), barki (0,+-0.23,0.37)
#   SM_Ludz_Glowa   - od szyi: twarz, wlosy
#   SM_Ludz_Ramie / SM_Ludz_Przedramie - reka z lokciem (22.11.2026): ramie od barku, lokiec na -0.30; przedramie od
#                     lokcia, dlon (chwyt narzedzia) na -0.25
#   SM_Ludz_Noga    - od biodra w dol: nogawka, but; stopa na -0.9 (stara noga bez kolana)
#   SM_Ludz_Udo / SM_Ludz_Lydka - noga z kolanem (21.11.2026): udo od biodra, kolano na -0.44; lydka od kolana, stopa na -0.46
#   SM_Ludz_Mlotek  - trzymany w dloni (poczatek w chwycie), trzonek do przodu (+X)
#   SM_Ludz_Kamien  - kamien niesiony przed soba (poczatek w srodku)
#   SM_Ludz_Bal     - bal niesiony przed soba w poprzek (budowa domow)
#   Praca w budynkach drewna (21.11.2026): SM_Ludz_Siekiera (drwal), SM_Ludz_Pila (pilarz), SM_Ludz_Motyka (borowy),
#   SM_Ludz_Deski (wiazka desek niesiona w poprzek), SM_Ludz_Sadzonka (mloda sadzonka z bryla ziemi).
#   Kamien i jedzenie (21.11.2026, etap 2): SM_Ludz_Kilof (kamieniarz), SM_Ludz_Luk (mysliwy), SM_Ludz_Kosz (kosz jagod
#   zbieracza), SM_Ludz_Worek (jedzenie niesione do spichlerza), SM_Ludz_Sarna (upolowana sarna niesiona w poprzek).
#   Zboze i chleb (21.11.2026, etap 3): SM_Ludz_Sierp (zniwa), SM_Ludz_Snop (snop niesiony w poprzek), SM_Ludz_Maka
#   (worek maki), SM_Ludz_Siewka (plachta z ziarnem przy siewie).
#   Owce, tkacz, kuznia (21.11.2026, etap 4): SM_Ludz_Nozyce (strzyzenie owiec), SM_Ludz_Welna (runo), SM_Ludz_Sukno
#   (bela sukna w poprzek), SM_Ludz_Ruda (kosz rudy), SM_Ludz_Bron (wiazka wloczni), SM_Ludz_Wlocznia (wlocznia woja).
#   Wojsko (21.11.2026, wyglad i krew): SM_Ludz_Tarcza (lico w barwie rodu), SM_Ludz_Helm / Czapka / Kaptur, SM_Ludz_Broda,
#   SM_Ludz_Choragiew (plat w barwie rodu), SM_Ludz_Kropla i SM_Ludz_Plama (krew w bitwie).
#   Rodzaje wojsk (21.11.2026, W2): SM_Ludz_Miecz, SM_Ludz_Maczuga, SM_Ludz_Kusza (w dloni), SM_Ludz_Strzala i SM_Ludz_Belt (pociski).
#   Kobiety (20.11.2026, rodziny osadnikow): SM_Ludz_TulowK (suknia do kostek), SM_Ludz_GlowaK (chusta), SM_Ludz_NogaK
#   (sama stopa w bucie pod suknia). Suknia = maska tuniki, chusta = maska wlosow (kolor chusty z CPD 3..5).
# Jeden material "Ludzie": kolor wierzcholkow; alfa = maska: 1 = tunika (kolor z Custom Primitive Data 0..2),
# 0,5 = wlosy (CPD 3..5), 0 = kolor stalyz modelu (skora, spodnie, buty). Dzieki temu kazdy ludzik (i kazdy gracz) ma swoj kolor.
# Uklad: Z = gora, X = przod, metry. Import: Tools/Zbuduj_Ludzi.bat.
import os
import numpy as np
from budynki import Siatka, pudlo, belka, wielokat, walec, rgb  # noqa

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "Art", "Ludzie")
M = 0


def maska(kolor, a):
    return np.array([*kolor, a])


SKORA = maska(rgb(206, 160, 124), 0.0)
WLOSY = maska(rgb(92, 64, 40), 0.5)
TUNIKA = maska(rgb(150, 140, 110), 1.0)
PASEK = maska(rgb(66, 48, 34), 0.0)
SPODNIE = maska(rgb(98, 88, 72), 0.0)
BUTY = maska(rgb(54, 40, 30), 0.0)
DREWNO = maska(rgb(150, 112, 72), 0.0)
ZELAZO = maska(rgb(88, 88, 92), 0.0)
KAMIEN = maska(rgb(214, 208, 194), 0.0)


def scietyslup(s, z0, z1, d0, w0, d1, w1, kolor, dx=0.0):
    """Bryla od z0 (glebokosc d0 w X, szerokosc w0 w Y) do z1 (d1, w1); osmiokat (sciete rogi) - mniej kanciasta sylwetka."""
    def pierscien(z, d, w, x0):
        k = 0.3
        return [(x0 + d / 2, -w / 2 * (1 - k), z), (x0 + d / 2, w / 2 * (1 - k), z), (x0 + d / 2 * (1 - k), w / 2, z), (x0 - d / 2 * (1 - k), w / 2, z),
                (x0 - d / 2, w / 2 * (1 - k), z), (x0 - d / 2, -w / 2 * (1 - k), z), (x0 - d / 2 * (1 - k), -w / 2, z), (x0 + d / 2 * (1 - k), -w / 2, z)]
    A, B = pierscien(z0, d0, w0, 0.0), pierscien(z1, d1, w1, dx)
    sr = (0, 0, (z0 + z1) / 2)
    for k in range(8):
        k2 = (k + 1) % 8
        wielokat(s, M, [A[k], A[k2], B[k2], B[k]], kolor, od=sr)
    wielokat(s, M, B, kolor, od=sr)
    wielokat(s, M, A, kolor, od=sr)


# Tulow w dwoch czesciach (22.11.2026, zywsze ciosy): biodra (dol tuniki i pasek, od biodra) i klatka (od pasa, 0.08 nad
# biodrem), ktora skreca sie i pochyla osobno. Klatka zaczyna sie troche w pasku, wiec przy skrecie nie ma szpary.
def biodra():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.42, 0.0, 0.30, 0.44, 0.24, 0.36, TUNIKA)          # dol tuniki (rozszerza sie ku dolowi)
    scietyslup(s, 0.0, 0.08, 0.25, 0.37, 0.25, 0.37, PASEK)
    return s


def klatka():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.03, 0.40, 0.235, 0.35, 0.24, 0.44, TUNIKA)        # piers i barki
    scietyslup(s, 0.40, 0.44, 0.24, 0.44, 0.12, 0.16, TUNIKA)          # zbiegniecie do szyi
    return s


def glowa():
    s = Siatka(["Ludzie"])
    belka(s, M, (0, 0, 0.0), (0, 0, 0.07), 0.09, SKORA)                # szyja
    scietyslup(s, 0.05, 0.27, 0.21, 0.18, 0.21, 0.18, SKORA)           # twarz
    scietyslup(s, 0.18, 0.31, 0.23, 0.20, 0.17, 0.15, WLOSY, dx=-0.01) # czupryna
    pudlo(s, M, (-0.115, -0.095, 0.08), (-0.05, 0.095, 0.25), WLOSY)   # wlosy z tylu
    pudlo(s, M, (0.10, -0.02, 0.12), (0.125, 0.02, 0.17), SKORA)       # nos
    return s


# Reka z lokciem (22.11.2026): ramie od barku do lokcia (-0.30), przedramie od lokcia z dlonia (chwyt na -0.25).
def ramie():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.31, 0.03, 0.115, 0.115, 0.13, 0.13, TUNIKA)       # rekaw
    return s


def przedramie():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.08, 0.03, 0.105, 0.105, 0.115, 0.115, TUNIKA)    # koniec rekawa, zakrywa lokiec
    belka(s, M, (0, 0, -0.05), (0, 0, -0.20), 0.085, SKORA)
    pudlo(s, M, (-0.05, -0.045, -0.30), (0.06, 0.045, -0.19), SKORA)   # dlon
    return s


def noga():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.80, 0.02, 0.12, 0.12, 0.15, 0.15, SPODNIE)
    pudlo(s, M, (-0.07, -0.065, -0.90), (0.15, 0.065, -0.79), BUTY)
    return s


# Noga z kolanem (21.11.2026, zywszy chod): udo od biodra do kolana (-0.44), lydka od kolana (poczatek w kolanie) ze stopa.
def udo():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.47, 0.02, 0.125, 0.125, 0.15, 0.15, SPODNIE)
    return s


def lydka():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.37, 0.03, 0.11, 0.11, 0.125, 0.125, SPODNIE)
    pudlo(s, M, (-0.07, -0.065, -0.46), (0.15, 0.065, -0.35), BUTY)
    return s


def udo_k():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.47, -0.20, 0.10, 0.10, 0.11, 0.11, maska(rgb(70, 58, 48), 0.0))  # pod suknia
    return s


def lydka_k():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.37, 0.03, 0.10, 0.10, 0.11, 0.11, maska(rgb(70, 58, 48), 0.0))
    pudlo(s, M, (-0.06, -0.06, -0.46), (0.14, 0.06, -0.35), BUTY)
    return s


def mlotek():
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.06, 0, 0), (0.36, 0, 0), 0.04, DREWNO)
    pudlo(s, M, (0.30, -0.035, -0.09), (0.40, 0.035, 0.07), ZELAZO)
    return s


def tulow_k():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.86, -0.30, 0.46, 0.52, 0.32, 0.42, TUNIKA)        # spodnica do kostek, szeroka u dolu (kroki pod nia)
    scietyslup(s, -0.30, 0.0, 0.32, 0.42, 0.24, 0.34, TUNIKA)
    scietyslup(s, 0.0, 0.07, 0.24, 0.34, 0.24, 0.34, PASEK)
    scietyslup(s, 0.07, 0.46, 0.23, 0.33, 0.22, 0.40, TUNIKA)
    scietyslup(s, 0.46, 0.50, 0.22, 0.40, 0.12, 0.15, TUNIKA)
    # fartuch z przodu
    pudlo(s, M, (0.13, -0.15, -0.70), (0.17, 0.15, -0.02), maska(rgb(196, 186, 162), 0.0))
    return s


def glowa_k():
    s = Siatka(["Ludzie"])
    belka(s, M, (0, 0, 0.0), (0, 0, 0.07), 0.09, SKORA)
    scietyslup(s, 0.05, 0.26, 0.20, 0.17, 0.20, 0.17, SKORA)           # twarz
    scietyslup(s, 0.17, 0.32, 0.24, 0.215, 0.18, 0.16, WLOSY, dx=-0.015)  # chusta na glowie
    pudlo(s, M, (-0.13, -0.11, 0.04), (-0.04, 0.11, 0.24), WLOSY)      # chusta z tylu, opada na kark
    pudlo(s, M, (-0.02, -0.112, 0.08), (0.06, 0.112, 0.22), WLOSY)     # boki chusty przy policzkach
    pudlo(s, M, (0.095, -0.02, 0.12), (0.12, 0.02, 0.165), SKORA)      # nos
    return s


def noga_k():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.80, -0.55, 0.10, 0.10, 0.11, 0.11, maska(rgb(70, 58, 48), 0.0))
    pudlo(s, M, (-0.06, -0.06, -0.90), (0.14, 0.06, -0.79), BUTY)
    return s


def bal_niesiony():
    s = Siatka(["Ludzie"])
    belka(s, M, (0, -0.65, 0), (0, 0.65, 0), 0.2, maska(rgb(150, 116, 80), 0.0))
    pudlo(s, M, (-0.08, -0.66, -0.08), (0.08, -0.64, 0.08), maska(rgb(196, 164, 120), 0.0))  # jasne czola bala
    pudlo(s, M, (-0.08, 0.64, -0.08), (0.08, 0.66, 0.08), maska(rgb(196, 164, 120), 0.0))
    return s


def kamien_niesiony():
    s = Siatka(["Ludzie"])
    pudlo(s, M, (-0.17, -0.22, -0.13), (0.17, 0.22, 0.13), KAMIEN, bez_dolu=False)
    return s


def siekiera():
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.08, 0, 0), (0.62, 0, 0), 0.045, DREWNO)
    pudlo(s, M, (0.50, -0.03, -0.17), (0.62, 0.03, 0.05), ZELAZO)   # ostrze w dol (tnie przy opuszczonej rece)
    return s


def pila():
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.08, 0, 0), (0.08, 0, 0), 0.05, DREWNO)           # raczka
    pudlo(s, M, (0.06, -0.012, -0.07), (0.95, 0.012, 0.05), ZELAZO)   # brzeszczot
    belka(s, M, (0.93, 0, -0.08), (0.93, 0, 0.12), 0.04, DREWNO)      # drugi uchwyt
    return s


def motyka():
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.25, 0, 0), (0.75, 0, 0), 0.04, DREWNO)
    pudlo(s, M, (0.66, -0.09, -0.2), (0.76, 0.09, 0.0), ZELAZO)
    return s


def deski_niesione():
    s = Siatka(["Ludzie"])
    for k in range(3):
        pudlo(s, M, (-0.14, -0.72 + k * 0.03, -0.05 + k * 0.045), (0.14, 0.72 + k * 0.03, -0.01 + k * 0.045), maska(rgb(170, 140, 102) * (0.92 + 0.06 * k), 0.0), bez_dolu=False)
    return s


def sadzonka_niesiona():
    s = Siatka(["Ludzie"])
    pudlo(s, M, (-0.12, -0.12, -0.14), (0.12, 0.12, 0.06), maska(rgb(92, 72, 52), 0.0), bez_dolu=False)  # bryla ziemi
    belka(s, M, (0, 0, 0.0), (0, 0, 0.35), 0.035, maska(rgb(120, 92, 64), 0.0))
    k = maska(rgb(92, 118, 64), 0.0)
    for z, r in ((0.2, 0.17), (0.38, 0.13), (0.54, 0.08)):
        pudlo(s, M, (-r, -r, z), (r, r, z + 0.16), k, bez_dolu=False)
    return s


def kilof():
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.08, 0, 0), (0.62, 0, 0), 0.045, DREWNO)
    belka(s, M, (0.58, 0, 0.2), (0.6, 0, -0.24), 0.04, ZELAZO)       # grot w obie strony, dziob w dol
    return s


def luk():
    s = Siatka(["Ludzie"])
    # Luk trzymany pionowo w dloni (poczatek w chwycie): dwa ramiona wygiete do przodu, cieciwa z tylu.
    kij = maska(rgb(120, 86, 52), 0.0)
    belka(s, M, (0.0, 0, 0.0), (0.12, 0, 0.42), 0.03, kij)
    belka(s, M, (0.12, 0, 0.42), (0.06, 0, 0.72), 0.025, kij)
    belka(s, M, (0.0, 0, 0.0), (0.12, 0, -0.42), 0.03, kij)
    belka(s, M, (0.12, 0, -0.42), (0.06, 0, -0.72), 0.025, kij)
    belka(s, M, (0.06, 0, 0.72), (-0.06, 0, 0.0), 0.008, maska(rgb(200, 190, 170), 0.0))
    belka(s, M, (-0.06, 0, 0.0), (0.06, 0, -0.72), 0.008, maska(rgb(200, 190, 170), 0.0))
    return s


def kosz_niesiony():
    s = Siatka(["Ludzie"])
    wik = maska(rgb(168, 134, 84), 0.0)
    pudlo(s, M, (-0.2, -0.2, -0.16), (0.2, 0.2, 0.1), wik, bez_dolu=False)
    pudlo(s, M, (-0.17, -0.17, 0.1), (0.17, 0.17, 0.13), maska(rgb(112, 40, 48), 0.0))      # jagody
    belka(s, M, (0, -0.2, 0.1), (0, 0.2, 0.1), 0.02, wik)
    return s


def worek_niesiony():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.2, 0.16, 0.3, 0.4, 0.22, 0.3, maska(rgb(176, 158, 120), 0.0))
    scietyslup(s, 0.16, 0.24, 0.12, 0.12, 0.06, 0.06, maska(rgb(150, 134, 100), 0.0))
    return s


def sarna():
    s = Siatka(["Ludzie"])
    sier = maska(rgb(140, 96, 62), 0.0)
    brzuch = maska(rgb(196, 170, 136), 0.0)
    pudlo(s, M, (-0.16, -0.48, -0.14), (0.16, 0.42, 0.14), sier, bez_dolu=False)        # tulow w poprzek
    pudlo(s, M, (-0.12, -0.44, -0.16), (0.12, 0.38, -0.13), brzuch, bez_dolu=False)
    pudlo(s, M, (-0.08, 0.40, -0.04), (0.08, 0.62, 0.08), sier, bez_dolu=False)          # szyja
    pudlo(s, M, (-0.07, 0.58, -0.12), (0.09, 0.74, 0.0), sier, bez_dolu=False)           # glowa zwisa
    for y in (-0.38, 0.32):
        for x in (-0.07, 0.07):
            belka(s, M, (x, y, -0.12), (x * 1.6, y + 0.05, -0.55), 0.035, sier)          # nogi zwisaja
    return s


def sierp():
    """Sierp: krotka raczka do przodu (+X), zakrzywione ostrze w bok i w dol (lamana z 4 odcinkow)."""
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.06, 0, 0), (0.16, 0, 0), 0.04, DREWNO)
    pts = [(0.16, 0, 0), (0.3, 0.05, -0.02), (0.4, 0.14, -0.06), (0.42, 0.26, -0.1), (0.36, 0.36, -0.12)]
    for a, b in zip(pts, pts[1:]):
        belka(s, M, a, b, 0.025, ZELAZO)
    return s


def snop_niesiony():
    """Snop zboza niesiony w poprzek (klosy z jednej strony)."""
    s = Siatka(["Ludzie"])
    zl = maska(rgb(196, 166, 98), 0.0)
    belka(s, M, (0, -0.45, 0), (0, 0.25, 0), 0.2, zl)
    pudlo(s, M, (-0.13, 0.25, -0.13), (0.13, 0.55, 0.13), maska(rgb(176, 146, 84), 0.0), bez_dolu=False)  # klosy
    belka(s, M, (0, -0.06, 0), (0, 0.02, 0), 0.23, maska(rgb(140, 112, 66), 0.0))                      # przewiazka
    return s


def maka_niesiona():
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.2, 0.16, 0.3, 0.4, 0.22, 0.3, maska(rgb(214, 206, 186), 0.0))
    scietyslup(s, 0.16, 0.24, 0.12, 0.12, 0.06, 0.06, maska(rgb(186, 176, 152), 0.0))
    return s


def siewka():
    """Plachta z ziarnem do siewu, trzymana w dloni przy biodrze."""
    s = Siatka(["Ludzie"])
    pudlo(s, M, (-0.04, -0.16, -0.3), (0.2, 0.16, -0.02), maska(rgb(176, 158, 120), 0.0), bez_dolu=False)
    pudlo(s, M, (0.0, -0.13, -0.04), (0.17, 0.13, 0.0), maska(rgb(200, 172, 104), 0.0))  # ziarno
    return s


def nozyce():
    """Nozyce do strzyzenia: dwa szerokie ostrza do przodu (+X) i kablak z tylu."""
    s = Siatka(["Ludzie"])
    for st in (-1, 1):
        belka(s, M, (0.04, st * 0.02, 0), (0.24, st * 0.012, 0.0), 0.03, ZELAZO)
    belka(s, M, (-0.08, -0.03, 0), (0.05, -0.02, 0), 0.03, ZELAZO)
    belka(s, M, (-0.08, 0.03, 0), (0.05, 0.02, 0), 0.03, ZELAZO)
    belka(s, M, (-0.1, -0.035, 0), (-0.1, 0.035, 0), 0.03, ZELAZO)  # kablak
    return s


def welna_niesiona():
    """Runo: zwiniety klab welny niesiony przed soba."""
    s = Siatka(["Ludzie"])
    bial = maska(rgb(222, 214, 194), 0.0)
    scietyslup(s, -0.14, 0.12, 0.4, 0.46, 0.34, 0.4, bial)
    scietyslup(s, 0.12, 0.2, 0.3, 0.34, 0.14, 0.18, bial * np.array([0.94, 0.94, 0.94, 1]))
    return s


def sukno_niesione():
    """Bela sukna (zwiniety material) niesiona w poprzek."""
    s = Siatka(["Ludzie"])
    walec(s, M, (0, -0.42, 0), (0, 0.42, 0), 0.13, maska(rgb(112, 104, 128), 0.0), boki=8)
    walec(s, M, (0, -0.44, 0), (0, -0.42, 0), 0.08, maska(rgb(84, 78, 96), 0.0), boki=8)
    walec(s, M, (0, 0.42, 0), (0, 0.44, 0), 0.08, maska(rgb(84, 78, 96), 0.0), boki=8)
    return s


def ruda_niesiona():
    """Kosz z bryłami rudy zelaza (rdzawy kamien)."""
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.2, 0.1, 0.38, 0.38, 0.44, 0.44, maska(rgb(160, 126, 82), 0.0))
    for k, (x, y) in enumerate(((-0.08, -0.08), (0.09, 0.0), (-0.04, 0.1))):
        pudlo(s, M, (x - 0.08, y - 0.07, 0.06), (x + 0.08, y + 0.07, 0.2 - 0.03 * k), maska(rgb(122, 72, 52), 0.0), bez_dolu=False)
    return s


def _wlocznia(s, x0, x1, y=0.0, z=0.0):
    belka(s, M, (x0, y, z), (x1, y, z), 0.04, DREWNO)
    belka(s, M, (x1 - 0.02, y, z), (x1 + 0.24, y, z), 0.05, ZELAZO)  # grot


def bron_niesiona():
    """Wiazka trzech wloczni niesiona w poprzek (grotami w bok)."""
    s = Siatka(["Ludzie"])
    for x, y0, z in ((0.0, 0.0, 0.0), (0.06, 0.08, 0.05), (-0.06, -0.08, 0.05)):
        belka(s, M, (x, -1.0 + y0, z), (x, 1.1 + y0, z), 0.04, DREWNO)
        belka(s, M, (x, 1.08 + y0, z), (x, 1.32 + y0, z), 0.05, ZELAZO)
    belka(s, M, (-0.1, -0.3, 0.03), (0.1, -0.3, 0.03), 0.12, maska(rgb(96, 72, 50), 0.0))  # rzemien
    return s


def wlocznia():
    """Wlocznia woja trzymana w dloni (poczatek w chwycie): drzewce do przodu (+X), grot na koncu."""
    s = Siatka(["Ludzie"])
    _wlocznia(s, -0.8, 1.55)
    return s


# Rodzaje wojsk (21.11.2026, etap W2): miecz, maczuga, kusza (w dloni, poczatek w chwycie, do przodu +X),
# strzala i belt (lecace pociski, poczatek w srodku, grot w +X).
def miecz():
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.10, 0, 0), (0.06, 0, 0), 0.035, maska(rgb(84, 58, 40), 0.0))      # rekojesc
    pudlo(s, M, (0.06, -0.09, -0.02), (0.09, 0.09, 0.02), ZELAZO)                      # jelec
    pudlo(s, M, (0.09, -0.028, -0.008), (0.80, 0.028, 0.008), maska(rgb(168, 170, 176), 0.0))  # glownia
    pudlo(s, M, (0.80, -0.016, -0.008), (0.86, 0.016, 0.008), maska(rgb(168, 170, 176), 0.0))  # sztych
    pudlo(s, M, (-0.13, -0.025, -0.025), (-0.10, 0.025, 0.025), ZELAZO)                # glowica
    return s


def maczuga():
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.08, 0, 0), (0.46, 0, 0), 0.045, DREWNO)
    # leb z nabijkami na koncu drzewca
    pudlo(s, M, (0.42, -0.075, -0.075), (0.64, 0.075, 0.075), maska(rgb(112, 82, 52), 0.0))
    for dy, dz in ((0.08, 0.0), (-0.08, 0.0), (0.0, 0.08), (0.0, -0.08)):
        pudlo(s, M, (0.50 , dy - 0.015, dz - 0.015), (0.56, dy + 0.015, dz + 0.015), ZELAZO)
    return s


def kusza():
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.12, 0, 0.02), (0.55, 0, 0.02), 0.05, DREWNO)                       # loze
    belka(s, M, (0.50, -0.30, 0.04), (0.50, 0.30, 0.04), 0.035, maska(rgb(120, 86, 52), 0.0))  # luczysko
    belka(s, M, (0.50, -0.30, 0.04), (0.18, 0, 0.05), 0.008, maska(rgb(200, 190, 170), 0.0))   # cieciwa
    belka(s, M, (0.18, 0, 0.05), (0.50, 0.30, 0.04), 0.008, maska(rgb(200, 190, 170), 0.0))
    pudlo(s, M, (0.54, -0.03, -0.03), (0.60, 0.03, 0.09), ZELAZO)                      # strzemie
    return s


def strzala():
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.38, 0, 0), (0.34, 0, 0), 0.016, maska(rgb(150, 120, 80), 0.0))
    pudlo(s, M, (0.34, -0.016, -0.016), (0.40, 0.016, 0.016), ZELAZO)
    pudlo(s, M, (-0.38, -0.04, -0.003), (-0.24, 0.04, 0.003), maska(rgb(225, 220, 205), 0.0))  # lotki
    pudlo(s, M, (-0.38, -0.003, -0.04), (-0.24, 0.003, 0.04), maska(rgb(225, 220, 205), 0.0))
    return s


def belt():
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.20, 0, 0), (0.18, 0, 0), 0.022, maska(rgb(130, 100, 66), 0.0))
    pudlo(s, M, (0.18, -0.022, -0.022), (0.25, 0.022, 0.022), ZELAZO)
    pudlo(s, M, (-0.20, -0.035, -0.003), (-0.12, 0.035, 0.003), maska(rgb(120, 96, 70), 0.0))
    return s


def tarcza():
    """Okragla tarcza na lewym przedramieniu (poczatek w chwycie), lico do przodu (+X). Lico = maska tuniki: gra daje mu
    barwe rodu (CPD 0..2 tego komponentu). Obrzeze ze skory, zelazny umbo."""
    s = Siatka(["Ludzie"])
    walec(s, M, (0.06, 0, 0), (0.09, 0, 0), 0.36, PASEK, boki=12)
    walec(s, M, (0.09, 0, 0), (0.105, 0, 0), 0.33, TUNIKA, boki=12)
    walec(s, M, (0.105, 0, 0), (0.15, 0, 0), 0.07, ZELAZO, boki=8)
    pudlo(s, M, (0.04, -0.05, -0.02), (0.06, 0.05, 0.02), DREWNO, bez_dolu=False)  # uchwyt
    return s


STAL = maska(rgb(150, 150, 156), 0.0)
FUTRO = maska(rgb(104, 76, 50), 0.0)
KREW = maska(rgb(118, 10, 8), 0.0)
KALUZA = maska(rgb(86, 6, 5), 0.0)


def helm():
    """Szpiczasty helm z nosalem (od szyi, jak glowa)."""
    s = Siatka(["Ludzie"])
    scietyslup(s, 0.19, 0.31, 0.245, 0.215, 0.20, 0.18, STAL, dx=-0.005)
    scietyslup(s, 0.31, 0.41, 0.20, 0.18, 0.03, 0.03, STAL, dx=-0.01)
    pudlo(s, M, (0.105, -0.013, 0.12), (0.125, 0.013, 0.23), STAL)
    scietyslup(s, 0.185, 0.21, 0.25, 0.22, 0.25, 0.22, ZELAZO, dx=-0.005)  # obrecz
    return s


def czapka():
    """Futrzana czapka z otokiem."""
    s = Siatka(["Ludzie"])
    scietyslup(s, 0.20, 0.27, 0.25, 0.22, 0.25, 0.22, FUTRO, dx=-0.01)
    scietyslup(s, 0.27, 0.34, 0.22, 0.20, 0.17, 0.15, maska(rgb(132, 52, 40), 0.0), dx=-0.015)
    return s


def kaptur():
    """Kaptur w kolorze tuniki (maska tuniki): tyl, boki i czubek glowy, twarz odslonieta."""
    s = Siatka(["Ludzie"])
    scietyslup(s, 0.22, 0.35, 0.26, 0.235, 0.17, 0.16, TUNIKA, dx=-0.02)
    pudlo(s, M, (-0.135, -0.12, 0.02), (-0.03, 0.12, 0.30), TUNIKA)
    pudlo(s, M, (-0.11, -0.122, 0.05), (0.07, -0.098, 0.28), TUNIKA)
    pudlo(s, M, (-0.11, 0.098, 0.05), (0.07, 0.122, 0.28), TUNIKA)
    scietyslup(s, -0.02, 0.06, 0.30, 0.34, 0.24, 0.26, TUNIKA)  # kolnierz na barkach
    return s


def broda():
    """Broda i wasy (maska wlosow)."""
    s = Siatka(["Ludzie"])
    pudlo(s, M, (0.04, -0.085, 0.02), (0.118, 0.085, 0.13), WLOSY, bez_dolu=False)
    pudlo(s, M, (0.09, -0.065, 0.115), (0.122, 0.065, 0.14), WLOSY)
    return s


def choragiew():
    """Choragiew oddzialu niesiona w lewej dloni (poczatek w chwycie): drzewce w gore, plat w barwie rodu (maska tuniki)
    na poprzeczce, wolny koniec do tylu (-X), z wcieciem."""
    s = Siatka(["Ludzie"])
    belka(s, M, (0, 0, -0.9), (0, 0, 2.35), 0.045, DREWNO)
    belka(s, M, (0.03, 0, 2.25), (-0.8, 0, 2.25), 0.035, DREWNO)
    pudlo(s, M, (-0.03, -0.03, 2.35), (0.03, 0.03, 2.47), ZELAZO, bez_dolu=False)
    pudlo(s, M, (-0.8, -0.012, 1.55), (-0.02, 0.012, 2.23), TUNIKA, bez_dolu=False)
    pudlo(s, M, (-0.8, -0.012, 1.30), (-0.52, 0.012, 1.55), TUNIKA, bez_dolu=False)
    pudlo(s, M, (-0.30, -0.012, 1.30), (-0.02, 0.012, 1.55), TUNIKA, bez_dolu=False)
    return s


def kropla():
    s = Siatka(["Ludzie"])
    pudlo(s, M, (-0.025, -0.025, -0.025), (0.025, 0.025, 0.025), KREW, bez_dolu=False)
    return s


# Iskra (22.11.2026): cios na tarczy / zelazie, jasnozolta drobinka.
def iskra():
    s = Siatka(["Ludzie"])
    pudlo(s, M, (-0.03, -0.012, -0.012), (0.03, 0.012, 0.012), maska(rgb(255, 236, 160), 0.0), bez_dolu=False)
    return s


def plama():
    """Plama krwi na ziemi (plaska, nieregularna, srednica ok. 1 m przy skali 1)."""
    s = Siatka(["Ludzie"])
    rng = np.random.default_rng(7)
    pts = []
    for k in range(11):
        t = 2 * np.pi * k / 11
        r = 0.5 * rng.uniform(0.6, 1.0)
        pts.append((np.cos(t) * r, np.sin(t) * r, 0.0))
    wielokat(s, M, pts, KALUZA, n=(0, 0, 1.0))
    return s




# --- Paczka A (23.11.2026): rybak i chata opalowa ---
def wedka():
    """Wedka: dluga leszczynowa witka uniesiona do przodu, z zylka zwisajaca z konca."""
    s = Siatka(["Ludzie"])
    belka(s, M, (-0.1, 0, 0), (1.6, 0, 0.55), 0.022, DREWNO)
    belka(s, M, (1.6, 0, 0.55), (1.6, 0, -0.35), 0.005, maska(rgb(200, 196, 180), 0.0))
    return s


def ryby_niesione():
    """Trzy ryby nawleczone na wiklinowy pret, niesione przed soba."""
    s = Siatka(["Ludzie"])
    belka(s, M, (0, -0.3, 0.12), (0, 0.3, 0.12), 0.015, maska(rgb(150, 120, 80), 0.0))
    for k, y in enumerate((-0.18, 0.0, 0.18)):
        sr = maska(rgb(138, 144, 140) * (0.92 + 0.06 * k), 0.0)
        pudlo(s, M, (-0.05, y - 0.035, -0.24), (0.05, y + 0.035, 0.1), sr, bez_dolu=False)
        pudlo(s, M, (-0.03, y - 0.05, -0.3), (0.03, y + 0.05, -0.24), maska(rgb(110, 116, 112), 0.0), bez_dolu=False)  # ogon
    return s


def opal_niesiony():
    """Narecze polan: piec rozlupanych kawalkow drewna."""
    s = Siatka(["Ludzie"])
    kor = maska(rgb(120, 92, 62), 0.0)
    for k, (y, z) in enumerate(((-0.16, 0), (0.0, 0), (0.16, 0), (-0.08, 0.12), (0.08, 0.12))):
        pudlo(s, M, (-0.22, y - 0.06, z - 0.06), (0.22, y + 0.06, z + 0.06), kor * (0.95 + 0.04 * (k % 2)), bez_dolu=False)
        pudlo(s, M, (0.22, y - 0.055, z - 0.055), (0.23, y + 0.055, z + 0.055), maska(rgb(200, 170, 124), 0.0))
    return s




# --- Wol (23.11.2026): ciagnie klody jak w Manor Lords. Uklad: X = przod, Z = gora, metry; gra skaluje x1,4 jak ludzi.
#   SM_Ludz_WolTulow - od srodka tulowia (w grze 1,05 m nad ziemia): tulow, szyja, glowa z rogami, jarzmo, ogon;
#                      stawy nog (0,72 / -0,68; +-0,24; -0,3)
#   SM_Ludz_WolNoga  - od stawu w dol: noga 0,72 m z kopytem
#   SM_Ludz_WolKlody - od punktu na ziemi pod srodkiem wolu: lancuchy od jarzma i trzy klody wleczone za wolem
SIERSC = maska(rgb(126, 84, 56), 0.0)
SIERSC_J = maska(rgb(170, 128, 92), 0.0)
ROG = maska(rgb(222, 208, 176), 0.0)
KOPYTO = maska(rgb(46, 38, 32), 0.0)


def rura(s, a, b, r, kolor, boki=8, denka=True):
    """Walec od a do b o promieniu r; sciany zwrocone na zewnatrz (walec z budynki.py ma odwrotne - material
    budynkow jest dwustronny, M_Ludzie nie)."""
    a, b = np.asarray(a, float), np.asarray(b, float)
    d = (b - a) / np.linalg.norm(b - a)
    u = np.cross(d, [0, 0, 1.0] if abs(d[2]) < 0.9 else [1.0, 0, 0]); u /= np.linalg.norm(u)
    v = np.cross(d, u)
    kat = np.linspace(0, 2 * np.pi, boki, endpoint=False)
    A = [a + (np.cos(t) * u + np.sin(t) * v) * r for t in kat]
    B = [b + (np.cos(t) * u + np.sin(t) * v) * r for t in kat]
    sr = (a + b) / 2
    for k in range(boki):
        k2 = (k + 1) % boki
        wielokat(s, M, [A[k], A[k2], B[k2], B[k]], kolor, od=sr)
    if denka:
        wielokat(s, M, A, kolor, od=sr)
        wielokat(s, M, B, kolor, od=sr)


def wol_tulow():
    s = Siatka(["Ludzie"])
    rura(s, (-0.92, 0, 0.0), (0.78, 0, 0.02), 0.36, SIERSC, boki=10)
    rura(s, (-0.6, 0, -0.12), (0.5, 0, -0.12), 0.3, SIERSC_J, boki=8, denka=False)          # jasniejszy brzuch
    rura(s, (0.35, 0, 0.18), (0.78, 0, 0.2), 0.26, SIERSC * 0.95, boki=8)                   # klab
    rura(s, (0.72, 0, 0.1), (1.12, 0, -0.02), 0.22, SIERSC, boki=8)                          # szyja
    rura(s, (1.08, 0, 0.02), (1.48, 0, -0.26), 0.15, SIERSC, boki=8)                         # glowa
    rura(s, (1.4, 0, -0.2), (1.56, 0, -0.32), 0.11, SIERSC_J * 0.8, boki=8)                  # pysk
    for y in (-1, 1):
        belka(s, M, (1.1, 0.1 * y, 0.12), (1.12, 0.34 * y, 0.22), 0.05, ROG)                     # rogi
        belka(s, M, (1.12, 0.34 * y, 0.22), (1.18, 0.4 * y, 0.38), 0.04, ROG)
        belka(s, M, (1.05, 0.12 * y, 0.04), (1.05, 0.26 * y, 0.0), 0.05, SIERSC * 0.85)          # uszy
    belka(s, M, (-0.92, 0, 0.22), (-1.0, 0, -0.5), 0.04, SIERSC)                                  # ogon
    belka(s, M, (-1.0, 0, -0.5), (-1.01, 0, -0.66), 0.07, KOPYTO)
    # jarzmo na karku i luki pod szyja
    belka(s, M, (0.9, -0.55, 0.28), (0.9, 0.55, 0.28), 0.1, maska(rgb(120, 90, 60), 0.0))
    for y in (-0.22, 0.22):
        belka(s, M, (0.9, y, 0.28), (0.95, y, -0.18), 0.035, maska(rgb(100, 76, 52), 0.0))
    return s


def wol_noga():
    s = Siatka(["Ludzie"])
    rura(s, (0, 0, 0.05), (0, 0, -0.42), 0.1, SIERSC, boki=7)
    rura(s, (0, 0, -0.42), (0.02, 0, -0.68), 0.065, SIERSC * 0.9, boki=7)
    pudlo(s, M, (-0.06, -0.06, -0.75), (0.1, 0.06, -0.66), KOPYTO, bez_dolu=False)
    return s


def wol_klody():
    s = Siatka(["Ludzie"])
    lan = maska(rgb(70, 66, 62), 0.0)
    kora = maska(rgb(104, 80, 56), 0.0)
    for y in (-0.5, 0.5):  # lancuchy od konca jarzma (0,9; +-0,5; 1,33) do klod
        belka(s, M, (0.9, y, 1.33), (-1.4, y * 0.45, 0.32), 0.035, lan)
    for k, (y, z) in enumerate(((-0.24, 0.17), (0.24, 0.17), (0.0, 0.47))):
        rura(s, (-1.35 - 0.15 * k, y, z), (-5.1 - 0.25 * k, y, z - 0.02), 0.17, kora * (0.92 + 0.05 * k), boki=7)
        rura(s, (-1.33 - 0.15 * k, y, z), (-1.35 - 0.15 * k, y, z), 0.15, maska(rgb(200, 170, 124), 0.0), boki=7)
    return s




# --- Kon z wozem (23.11.2026): wozi deski i kamien. Uklad jak wol.
#   SM_Ludz_KonTulow - od srodka tulowia (w grze 1,25 m nad ziemia), stawy nog (0,55 / -0,58; +-0,17; -0,2)
#   SM_Ludz_KonNoga  - od stawu w dol: noga 1,05 m z kopytem
#   SM_Ludz_Woz      - od punktu na ziemi pod srodkiem konia: dyszle, skrzynia na dwoch kolach za koniem
#   SM_Ludz_WozDeski / SM_Ludz_WozKamien - ladunek na skrzyni wozu (ten sam uklad co SM_Ludz_Woz)
GNIADY = maska(rgb(112, 72, 46), 0.0)
GRZYWA = maska(rgb(42, 32, 26), 0.0)
UPRZAZ = maska(rgb(70, 50, 36), 0.0)
WOZ_X0, WOZ_X1 = -3.3, -1.35


def kon_tulow():
    s = Siatka(["Ludzie"])
    rura(s, (-0.75, 0, 0.0), (0.62, 0, 0.04), 0.3, GNIADY, boki=10)
    rura(s, (0.5, 0, 0.1), (0.84, 0, 0.56), 0.16, GNIADY, boki=8)                   # szyja
    rura(s, (0.8, 0, 0.64), (1.16, 0, 0.36), 0.11, GNIADY, boki=8)                  # glowa
    rura(s, (1.1, 0, 0.4), (1.2, 0, 0.3), 0.08, GNIADY * 0.75, boki=8)              # chrapy
    belka(s, M, (0.52, 0, 0.3), (0.82, 0, 0.74), 0.07, GRZYWA)                       # grzywa
    for y in (-0.05, 0.05):
        belka(s, M, (0.84, y, 0.7), (0.82, y * 1.6, 0.82), 0.035, GNIADY)            # uszy
    rura(s, (-0.74, 0, 0.14), (-0.92, 0, -0.5), 0.07, GRZYWA, boki=6)               # ogon
    pudlo(s, M, (0.52, -0.22, 0.08), (0.64, 0.22, 0.4), UPRZAZ, bez_dolu=False)      # chomato
    belka(s, M, (-0.3, -0.31, 0.18), (-0.3, 0.31, 0.18), 0.06, UPRZAZ)               # popreg
    return s


def kon_noga():
    s = Siatka(["Ludzie"])
    rura(s, (0, 0, 0.05), (0, 0, -0.55), 0.085, GNIADY, boki=7)
    rura(s, (0, 0, -0.55), (0.02, 0, -0.97), 0.05, GNIADY * 0.85, boki=7)
    pudlo(s, M, (-0.05, -0.055, -1.05), (0.09, 0.055, -0.96), GRZYWA, bez_dolu=False)
    return s


def woz():
    s = Siatka(["Ludzie"])
    drewno = maska(rgb(132, 100, 66), 0.0)
    ciemne = maska(rgb(92, 70, 48), 0.0)
    for y in (-1, 1):
        belka(s, M, (0.58, 0.3 * y, 1.15), (WOZ_X1, 0.5 * y, 0.85), 0.07, drewno)  # dyszle od chomata
        rura(s, (-2.3, 0.66 * y, 0.5), (-2.3, 0.78 * y, 0.5), 0.5, ciemne, boki=12)  # kola
        rura(s, (-2.3, 0.62 * y, 0.5), (-2.3, 0.84 * y, 0.5), 0.09, drewno, boki=8)  # piasty
        pudlo(s, M, (WOZ_X0, 0.6 * y - 0.05, 0.85), (WOZ_X1, 0.6 * y + 0.05, 1.12), drewno, bez_dolu=False)  # burty
    belka(s, M, (-2.3, -0.7, 0.5), (-2.3, 0.7, 0.5), 0.08, ciemne)                   # os
    pudlo(s, M, (WOZ_X0, -0.65, 0.75), (WOZ_X1, 0.65, 0.85), drewno * 0.95, bez_dolu=False)  # skrzynia
    for x in (WOZ_X0, WOZ_X1):
        pudlo(s, M, (x - 0.05, -0.6, 0.85), (x + 0.05, 0.6, 1.08), drewno, bez_dolu=False)
    return s


def woz_deski():
    s = Siatka(["Ludzie"])
    for k in range(4):
        for j, y in enumerate((-0.3, 0.05)):
            pudlo(s, M, (WOZ_X0 + 0.1, y, 0.85 + 0.06 * k), (WOZ_X1 - 0.05, y + 0.25, 0.9 + 0.06 * k),
                  maska(rgb(196, 160, 112) * (0.92 + 0.04 * ((k + j) % 3)), 0.0), bez_dolu=False)
    return s


def woz_kamien():
    s = Siatka(["Ludzie"])
    rng = np.random.default_rng(81)
    for k in range(9):
        x = WOZ_X0 + 0.35 + (k % 3) * 0.55 + rng.uniform(-0.08, 0.08)
        y = -0.35 + (k // 3) * 0.35 + rng.uniform(-0.05, 0.05)
        r = rng.uniform(0.12, 0.17)
        pudlo(s, M, (x - r, y - r, 0.85), (x + r, y + r, 0.85 + r * 1.5), maska(rgb(196, 190, 176) * rng.uniform(0.9, 1.05), 0.0), bez_dolu=False)
    return s



# Paczka A, krok 3 (23.11.2026): niesione skory, skora, buty, ubrania, slod, piwo.
def skory_niesione():
    """Zwoj surowych skor przerzucony przed soba."""
    s = Siatka(["Ludzie"])
    for k in range(3):
        pudlo(s, M, (-0.2 + k * 0.02, -0.3, -0.05 + k * 0.05), (0.2 - k * 0.02, 0.3, 0.0 + k * 0.05), maska(rgb(150, 112, 80) * (0.9 + 0.05 * k), 0.0), bez_dolu=False)
    return s


def skora_niesiona():
    """Rulon wyprawionej skory w poprzek."""
    s = Siatka(["Ludzie"])
    rura(s, (0, -0.32, 0.0), (0, 0.32, 0.0), 0.1, maska(rgb(118, 78, 48), 0.0), boki=8)
    belka(s, M, (0, -0.15, 0.1), (0, -0.15, -0.1), 0.02, maska(rgb(70, 50, 36), 0.0))
    return s


def buty_niesione():
    """Kosz z butami."""
    s = Siatka(["Ludzie"])
    wik = maska(rgb(168, 134, 84), 0.0)
    pudlo(s, M, (-0.18, -0.2, -0.14), (0.18, 0.2, 0.08), wik, bez_dolu=False)
    for y in (-0.1, 0.1):
        pudlo(s, M, (-0.12, y - 0.05, 0.08), (0.12, y + 0.05, 0.16), maska(rgb(70, 48, 32), 0.0), bez_dolu=False)
    return s


def ubrania_niesione():
    """Stos zlozonych ubran."""
    s = Siatka(["Ludzie"])
    for k, c in enumerate(((112, 104, 128), (126, 98, 76), (98, 112, 92))):
        pudlo(s, M, (-0.16, -0.22, -0.1 + k * 0.08), (0.16, 0.22, -0.03 + k * 0.08), maska(rgb(*c) * 1.1, 0.0), bez_dolu=False)
    return s


def slod_niesiony():
    """Worek slodu."""
    s = Siatka(["Ludzie"])
    scietyslup(s, -0.2, 0.16, 0.3, 0.4, 0.22, 0.3, maska(rgb(196, 160, 98), 0.0))
    scietyslup(s, 0.16, 0.24, 0.12, 0.12, 0.06, 0.06, maska(rgb(160, 130, 80), 0.0))
    return s


def piwo_niesione():
    """Baryłka piwa."""
    s = Siatka(["Ludzie"])
    rura(s, (0, -0.22, 0.0), (0, 0.22, 0.0), 0.17, maska(rgb(120, 84, 54), 0.0), boki=10)
    for y in (-0.14, 0.14):
        rura(s, (0, y - 0.02, 0.0), (0, y + 0.02, 0.0), 0.18, maska(rgb(70, 66, 62), 0.0), boki=10, denka=False)
    return s

CZESCI = {
    "SM_Ludz_Biodra": biodra, "SM_Ludz_Klatka": klatka, "SM_Ludz_Glowa": glowa,
    "SM_Ludz_Ramie": ramie, "SM_Ludz_Przedramie": przedramie, "SM_Ludz_Noga": noga,
    "SM_Ludz_Mlotek": mlotek, "SM_Ludz_Kamien": kamien_niesiony, "SM_Ludz_Bal": bal_niesiony,
    "SM_Ludz_TulowK": tulow_k, "SM_Ludz_GlowaK": glowa_k, "SM_Ludz_NogaK": noga_k,
    "SM_Ludz_Udo": udo, "SM_Ludz_Lydka": lydka, "SM_Ludz_UdoK": udo_k, "SM_Ludz_LydkaK": lydka_k,
    "SM_Ludz_Siekiera": siekiera, "SM_Ludz_Pila": pila, "SM_Ludz_Motyka": motyka,
    "SM_Ludz_Deski": deski_niesione, "SM_Ludz_Sadzonka": sadzonka_niesiona,
    "SM_Ludz_Kilof": kilof, "SM_Ludz_Luk": luk, "SM_Ludz_Kosz": kosz_niesiony, "SM_Ludz_Worek": worek_niesiony,
    "SM_Ludz_Sarna": sarna,
    "SM_Ludz_Sierp": sierp, "SM_Ludz_Snop": snop_niesiony, "SM_Ludz_Maka": maka_niesiona, "SM_Ludz_Siewka": siewka,
    "SM_Ludz_Nozyce": nozyce, "SM_Ludz_Welna": welna_niesiona, "SM_Ludz_Sukno": sukno_niesione, "SM_Ludz_Ruda": ruda_niesiona,
    "SM_Ludz_Bron": bron_niesiona, "SM_Ludz_Wlocznia": wlocznia,
    "SM_Ludz_Tarcza": tarcza, "SM_Ludz_Helm": helm, "SM_Ludz_Czapka": czapka, "SM_Ludz_Kaptur": kaptur,
    "SM_Ludz_Miecz": miecz, "SM_Ludz_Maczuga": maczuga, "SM_Ludz_Kusza": kusza, "SM_Ludz_Strzala": strzala, "SM_Ludz_Belt": belt,
    "SM_Ludz_Broda": broda, "SM_Ludz_Choragiew": choragiew, "SM_Ludz_Kropla": kropla, "SM_Ludz_Plama": plama,
    "SM_Ludz_Iskra": iskra,
    "SM_Ludz_Wedka": wedka, "SM_Ludz_Ryby": ryby_niesione, "SM_Ludz_Opal": opal_niesiony,
    "SM_Ludz_WolTulow": wol_tulow, "SM_Ludz_WolNoga": wol_noga, "SM_Ludz_WolKlody": wol_klody,
    "SM_Ludz_KonTulow": kon_tulow, "SM_Ludz_KonNoga": kon_noga, "SM_Ludz_Woz": woz,
    "SM_Ludz_WozDeski": woz_deski, "SM_Ludz_WozKamien": woz_kamien,
    "SM_Ludz_Skory": skory_niesione, "SM_Ludz_Skora": skora_niesiona, "SM_Ludz_Buty": buty_niesione,
    "SM_Ludz_Ubrania": ubrania_niesione, "SM_Ludz_Slod": slod_niesiony, "SM_Ludz_Piwo": piwo_niesione,
}


def main():
    os.makedirs(OUT, exist_ok=True)
    for nazwa, f in CZESCI.items():
        tr = f().zapisz(os.path.join(OUT, nazwa + ".glb"))
        assert 4 < tr < 2000, f"{nazwa}: {tr} trojkatow"
        print(f"{nazwa}.glb: {tr} trojkatow")


if __name__ == "__main__":
    main()
