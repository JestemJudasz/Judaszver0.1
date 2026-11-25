# Plik: Tools/dzwieki_glosy.py
# Nagrania CC0 z Freesound (lista: Tools/dzwieki_freesound.txt, pliki: Dane_zrodlowe/dzwieki/freesound/<id>.mp3)
# -> Art/Dzwieki/Freesound/<Grupa>_<nn>.wav (mono 16 bit 48 kHz) do importu Tools\Zbuduj_Dzwieki.bat.
# MP3 dekoduje libsndfile z silnika (ctypes). Grupy o tej samej nazwie co w Walka/ lacza sie w grze (UDzwieki).
# Uruchomienie: python Tools/dzwieki_glosy.py (PYTHONPATH=Tools/_py).
import ctypes
import glob
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dzwieki_walka import SR, obnizony, uderzenia, zapisz  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Dane_zrodlowe", "dzwieki", "freesound")
DST = os.path.join(ROOT, "Art", "Dzwieki", "Freesound")
SNDFILE = r"D:\UE_5.8\Engine\Binaries\ThirdParty\libsndfile\Win64\libsndfile-1.dll"

# grupa: [(id, tryb, dlugosc s, ile)]; tryb "cale" = caly plik, "tnij" = pojedyncze zdarzenia, "petla" = petla tla.
# Numery od 51, zeby nie nadpisac wariantow z Walka/ (te maja 01..14).
GRUPY = {
    "Okrzyk": [(521830, "cale", 0, 1), (521831, "cale", 0, 1), (521832, "cale", 0, 1), (764158, "tnij", 3.0, 2),
               (563011, "tnij", 3.5, 3)],
    "Smierc": [(416838, "cale", 0, 1), (416839, "cale", 0, 1), (556976, "tnij", 1.6, 4), (255322, "tnij", 2.0, 5)],
    "Rana": [(610998, "tnij", 1.0, 6), (103072, "tnij", 1.0, 2)],
    "Trafienie": [(276600, "cale", 0, 1), (460509, "tnij", 0.6, 3), (259542, "tnij", 0.6, 5), (264062, "tnij", 0.7, 1)],
    "Tarcza": [(400188, "tnij", 0.6, 4)],
    "Szczek": [(588278, "tnij", 0.8, 4)],
    "Siekiera": [(421928, "cale", 0, 1), (421929, "cale", 0, 1), (753925, "tnij", 0.6, 4), (419928, "tnij", 0.6, 3)],
    "Kilof": [(233630, "cale", 0, 1), (760567, "cale", 0, 1), (431019, "cale", 0, 1), (184383, "tnij", 0.5, 4)],
    "Pila": [(383725, "tnij", 0.9, 6)],
    "Kowal": [(321889, "tnij", 1.0, 4), (434339, "tnij", 1.0, 3), (363169, "tnij", 1.0, 3)],
    "Mlotek": [(394891, "tnij", 0.4, 4), (204946, "tnij", 0.4, 4)],
    "Petla_Bitwa": [(384401, "petla", 60.0, 1)],
    "Petla_Wioska": [(578072, "petla", 40.0, 1)],
    "Petla_Gwar": [(473586, "petla", 36.0, 1)],
}


class SfInfo(ctypes.Structure):
    _fields_ = [("frames", ctypes.c_int64), ("samplerate", ctypes.c_int), ("channels", ctypes.c_int),
                ("format", ctypes.c_int), ("sections", ctypes.c_int), ("seekable", ctypes.c_int)]


sf = ctypes.CDLL(SNDFILE)
sf.sf_open.restype = ctypes.c_void_p
sf.sf_open.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.POINTER(SfInfo)]
sf.sf_readf_float.restype = ctypes.c_int64
sf.sf_readf_float.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.c_int64]
sf.sf_close.argtypes = [ctypes.c_void_p]


def czytaj(id_):
    info = SfInfo()
    h = sf.sf_open(os.path.join(SRC, "%d.mp3" % id_).encode(), 0x10, ctypes.byref(info))  # SFM_READ
    assert h, "libsndfile nie otworzyl %d" % id_
    buf = np.zeros(info.frames * info.channels + 1024 * info.channels, np.float32)
    n = sf.sf_readf_float(h, buf.ctypes.data_as(ctypes.POINTER(ctypes.c_float)), info.frames + 1024)
    sf.sf_close(h)
    x = buf[:n * info.channels].reshape(-1, info.channels).mean(axis=1).astype(np.float64)
    if info.samplerate != SR:  # liniowo wystarczy: podglady maja 44,1 lub 48 kHz
        x = obnizony(x, info.samplerate / SR, 0)
    return x


def wycisz(y, kon=0.08):
    f = min(len(y) // 2, int(SR * kon))
    y[-f:] *= np.linspace(1, 0, f)
    y[:int(SR * 0.003)] *= np.linspace(0, 1, int(SR * 0.003))
    return y


def petla(x, dl):
    """Odcinek dl s z przenikaniem 2 s konca w poczatek: gra w kolko bez szwu."""
    n, p = int(SR * dl), int(SR * 2.0)
    x = x[:n + p]
    y = x[:n].copy()
    a = np.linspace(0, 1, p)
    y[:p] = y[:p] * a + x[n:n + p] * (1 - a)
    return y


def main():
    os.makedirs(DST, exist_ok=True)
    for f in glob.glob(os.path.join(DST, "*.wav")):
        os.remove(f)
    for grupa, zrodla in GRUPY.items():
        nr = 51
        for id_, tryb, dl, ile in zrodla:
            x = czytaj(id_)
            if tryb == "cale":
                kawalki = [wycisz(x.copy())]
            elif tryb == "petla":
                kawalki = [petla(x, min(dl, len(x) / SR - 2.5))]
            else:
                u = sorted(uderzenia(x, dl), reverse=True)[:ile]
                kawalki = [wycisz(x[s:s + int(SR * dl)].copy()) for _, s in sorted(u, key=lambda k: k[1])]
            for y in kawalki:
                nazwa = grupa if tryb == "petla" else "%s_%02d" % (grupa, nr)
                zapisz(os.path.join(DST, nazwa + ".wav"), y)
                nr += 1
        print("Dzwieki: %s %d plikow" % (grupa, nr - 51))
    assert len(glob.glob(os.path.join(DST, "*.wav"))) > 50


main()
