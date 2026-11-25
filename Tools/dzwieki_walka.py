# Plik: Tools/dzwieki_walka.py
# Tnie nagrania CC0 "Medieval sound effects" (OpenGameArt, Ben Jaszczak i Brian Nelson; Dane_zrodlowe/dzwieki/oga_medieval)
# na pojedyncze uderzenia: Art/Dzwieki/Walka/<Grupa>_<n>.wav (mono, 16 bit, 48 kHz), do importu Tools\Zbuduj_Dzwieki.bat.
# Wykrywa poczatki uderzen z obwiedni, bierze najglosniejsze, przycina z wyciszeniem, normalizuje do -3 dBFS.
# Uruchomienie: python -I Tools/dzwieki_walka.py (PYTHONPATH=Tools/_py).
import glob
import os
import wave

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Dane_zrodlowe", "dzwieki", "oga_medieval", "rozpakowane")
DST = os.path.join(ROOT, "Art", "Dzwieki", "Walka")
SR = 48000

# grupa: (wzorce plikow, dlugosc wycinka s, ile wariantow najwyzej, na plik)
# 23.11.2026 (uzytkownik: "jak patykami"): drewno nizej i ciezej, tarcza = drzewce o drzewce 0,55 x nizej i bez gory.
TON = {"Tarcza": (0.55, 2500), "Drzewce": (0.8, 6000), "Maczuga": (0.85, 0), "Szczek": (0.92, 0)}
GRUPY = {
    "Szczek": (["*Sword Blade on Blade", "Sabre Katana Blade on Blade", "Spear Sabre Blade on Blade", "Axe Sabre Blade on Blade",
                "Spear Spear Blade on Blade", "Seax * Blade on Blade"], 0.7, 14, 2),
    "Drzewce": (["* Haft on Blade", "* Blade on Haft", "* Blade on haft"], 0.6, 10, 1),
    "Tarcza": (["* Haft on Haft"], 0.6, 8, 4),
    "Maczuga": (["Mace *"], 0.7, 10, 2),
    "Zamach": (["* Swing"], 0.5, 10, 2),
    "Luk": (["English Longbow Shoot", "Scythian Recurve Shoot"], 0.9, 6, 3),
    "Kusza": (["Crossbow Shoot", "Crossbow Lever Trigger"], 0.9, 5, 3),
    "Swist": (["* Arrow Pass", "* Arrow Passby", "Crossbow Bolt Pass*"], 0.8, 8, 2),
}


def czytaj(p):
    with wave.open(p) as w:
        n, sw, sr = w.getnchannels(), w.getsampwidth(), w.getframerate()
        b = np.frombuffer(w.readframes(w.getnframes()), np.uint8)
    if sw == 3:
        b = b.reshape(-1, 3)
        x = (b[:, 0].astype(np.int32) | (b[:, 1].astype(np.int32) << 8) | (b[:, 2].astype(np.int32) << 16))
        x = np.where(x >= 1 << 23, x - (1 << 24), x) / float(1 << 23)
    else:
        x = np.frombuffer(b.tobytes(), np.int16) / 32768.0
    x = x.reshape(-1, n).mean(axis=1)
    k = sr // SR
    if k > 1:  # filtr dolnoprzepustowy (okno Hanna) i co k-ta probka
        t = np.arange(-64 * k, 64 * k + 1)
        h = np.sinc(t / k) * np.hanning(len(t))
        x = np.convolve(x, h / h.sum(), mode="same")[::k]
    return x


def uderzenia(x, dl):
    """Poczatki uderzen: obwiednia 5 ms, skok powyzej progu, przerwa co najmniej dl."""
    o = int(SR * 0.005)
    env = np.sqrt(np.convolve(x * x, np.ones(o) / o, mode="same"))
    prog = env.max() * 0.25
    wyn, i = [], 0
    while i < len(env):
        if env[i] > prog:
            s = max(0, i - int(SR * 0.012))
            wyn.append((env[i:i + int(SR * 0.05)].max(), s))
            i += int(SR * dl)
        else:
            i += 1
    return wyn


def obnizony(y, ton, odcina):
    """Nizej o ton (dluzszy wycinek) i opcjonalnie bez wysokich tonow powyzej odcina Hz."""
    if odcina:
        t = np.arange(-200, 201)
        f = odcina / SR
        h = np.sinc(2 * f * t) * np.hanning(len(t))
        y = np.convolve(y, h / h.sum(), mode="same")
    n = int(len(y) / ton)
    return np.interp(np.arange(n) * ton, np.arange(len(y)), y)


def zapisz(p, y):
    # -6 dBFS: zapas na sumowanie wielu uderzen naraz (wczesniej -3 dB trzeszczalo na sluchawkach)
    y = y / max(1e-6, np.abs(y).max()) * 0.5
    with wave.open(p, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes((y * 32767).astype("<i2").tobytes())


def main():
    os.makedirs(DST, exist_ok=True)
    for f in glob.glob(os.path.join(DST, "*.wav")):
        os.remove(f)
    for grupa, (wzorce, dl, ile, naplik) in GRUPY.items():
        pliki = sorted({p for w in wzorce for p in glob.glob(os.path.join(SRC, w + ".wav"))})
        kand = []
        for p in pliki:
            x = czytaj(p)
            u = sorted(uderzenia(x, dl), reverse=True)[:naplik]
            for g, s in u:
                y = x[s:s + int(SR * dl)].copy()
                f = int(SR * 0.08)
                y[-f:] *= np.linspace(1, 0, f)
                y[:int(SR * 0.002)] *= np.linspace(0, 1, int(SR * 0.002))
                if grupa in TON:
                    y = obnizony(y, *TON[grupa])
                kand.append((g, os.path.basename(p), y))
        kand.sort(key=lambda k: -k[0])
        for n, (_, z, y) in enumerate(kand[:ile], 1):
            zapisz(os.path.join(DST, "%s_%02d.wav" % (grupa, n)), y)
        print("Dzwieki: %s %d wariantow z %d plikow" % (grupa, min(ile, len(kand)), len(pliki)))
    assert len(glob.glob(os.path.join(DST, "*.wav"))) > 30


if __name__ == "__main__":
    main()
