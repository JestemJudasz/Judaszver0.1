# Plik: Tools/ikony.py
# Ikony interfejsu -> Art/Budynki/T_Ikona_*.png (128 px, przezroczyste tlo, pikselowe jak gra). Uruchamiane z Zbuduj_Budynki.bat.
#   Budynki paska budowy: gotowy budynek (ostatnia faza z Art/Budynki) z podgladu bez Unreala (Tools/podglad.py).
#   Surowce (licznik u gory, okno budynku): T_Ikona_Kloda / Deska / Kamien - stosy towaru (SM_Stos_*_2) z tego samego podgladu.
#   T_Ikona_Jedzenie (21.11.2026, etap 2): stos jedzenia (worki, kosze). T_Ikona_Zadowolenie: pikselowe serce (16 x 16).
#   Etap 3 (21.11.2026): pole, mlyn, piekarnia, tartak wodny; surowce T_Ikona_Zboze (snopy) i T_Ikona_Maka (worki).
#   Etap 4 (21.11.2026): owczarnia (z owcami), chata tkacza, kopalnia rudy, kuznia, zbrojownia; surowce T_Ikona_Welna,
#   T_Ikona_Sukno, T_Ikona_Ruda, T_Ikona_Bron (stosy SM_Stos_*_3).
#   Etap 5 (21.11.2026): targ; T_Ikona_Grosze - stosik srebrnych groszy rysowany piksel po pikselu (16 x 16).
#   T_Ikona_Rodzina: para osadnikow (maz w tunice, zona w sukni i chuscie) rysowana piksel po pikselu (32 x 32).
#   Pasek jak w Manor Lords (21.11.2026): T_Ikona_Znak_<Nazwa> - jasne znaki z ciemnym obrysem (32 x 32) na glowne przyciski
#   (Budowa, Wojsko, Rody) i kategorie budynkow (UI/PasekBudowy: NazwyZnakow).
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image, ImageDraw
from podglad import renderuj, wczytaj, ART

IKONY = {"SM_Chata": 4, "SM_DomPruski": 4, "SM_DomDuzy": 4, "SM_Kosciol": 5,
         "SM_PlacSkladowy": 4, "SM_ChataDrwala": 4, "SM_ChataBorowego": 4, "SM_Pilarnia": 4,
         "SM_Kamieniolom": 4, "SM_ChataZbieracza": 4, "SM_ChataMysliwego": 4, "SM_Spichlerz": 4,
         "SM_Pole": 2, "SM_Mlyn": 4, "SM_Piekarnia": 4, "SM_TartakWodny": 4,
         "SM_Owczarnia": 4, "SM_ChataTkacza": 4, "SM_KopalniaRudy": 4, "SM_Kuznia": 4, "SM_Zbrojownia": 4,
         "SM_Targ": 4, "SM_KosciolDrewniany": 4, "SM_ChataOpalowa": 4, "SM_ChataRybaka": 4, "SM_ZagrodaWolow": 4, "SM_Stajnia": 4,
         "SM_Garbarnia": 4, "SM_Szewc": 4, "SM_Krawiec": 4, "SM_Slodownia": 4, "SM_Browar": 4, "SM_Karczma": 4}
# Umocnienia gracza (26.11.2026): modele bez faz, ikona z samych czesci (plik _F<faza> nie jest czytany).
for _m in ("SM_Palisada", "SM_Mur", "SM_Palisada_Wieza", "SM_Mur_Wieza", "SM_Palisada_Brama", "SM_Mur_Brama"):
    IKONY[_m] = 1
# Etap 3: ikona zlozona z kilku modeli (pole z dojrzalym zbozem, wiatrak ze skrzydlami, tartak z kolem); przesuniecie w metrach
# ukladu budowania (Tools/budynki.py: MLYN_PIASTA, KOLO).
ZLOZONE = {"SM_Palisada": [("SM_Palisada", (-4.0, 0, 0)), ("SM_Palisada", (0, 0, 0)), ("SM_Palisada", (4.0, 0, 0))],
           "SM_Mur": [("SM_Mur", (-4.0, 0, 0)), ("SM_Mur", (0, 0, 0)), ("SM_Mur", (4.0, 0, 0))],
           "SM_Palisada_Wieza": [("SM_Palisada_Wieza", (0, 0, 0))], "SM_Mur_Wieza": [("SM_Mur_Wieza", (0, 0, 0))],
           "SM_Palisada_Brama": [("SM_Palisada_Brama", (0, 0, 0))], "SM_Mur_Brama": [("SM_Mur_Brama", (0, 0, 0))],
           "SM_Pole": [("SM_Pole_Dojrzale", (0, 0, 0))],
           "SM_Mlyn": [("SM_Mlyn_F4", (0, 0, 0)), ("SM_Mlyn_Skrzydla", (0.0, -1.95, 6.0))],
           "SM_TartakWodny": [("SM_TartakWodny_F4", (0, 0, 0)), ("SM_TartakWodny_Kolo", (-3.75, 0.0, 1.7))],
           "SM_Owczarnia": [("SM_Owczarnia_F4", (0, 0, 0)), ("SM_Owca", (-3.0, -2.0, 0.0)), ("SM_Owca", (0.5, -3.2, 0.0)),
                            ("SM_Owca_Strzyzona", (3.0, -1.4, 0.0)), ("SM_Owca", (-0.8, -1.2, 0.0))]}
SUROWCE = {"Kloda": "SM_Stos_Kloda_2", "Deska": "SM_Stos_Deska_3", "Kamien": "SM_Stos_Kamien_2", "Jedzenie": "SM_Stos_Jedzenie_3",
           "Zboze": "SM_Stos_Zboze_3", "Maka": "SM_Stos_Maka_3",
           "Welna": "SM_Stos_Welna_3", "Sukno": "SM_Stos_Sukno_3", "Ruda": "SM_Stos_Ruda_3", "Bron": "SM_Stos_Bron_3",
           "Opal": "SM_Stos_Opal_3", "Ryby": "SM_Stos_Ryby_3", "Skory": "SM_Stos_Skory_3", "Skora": "SM_Stos_Skora_3",
           "Buty": "SM_Stos_Buty_3", "Ubrania": "SM_Stos_Ubrania_3", "Slod": "SM_Stos_Slod_3", "Piwo": "SM_Stos_Piwo_3"}

# Para osadnikow: . = przezroczyste; kolory stonowane jak w grze (Osada.cpp: tuniki, suknie, chusty).
RODZINA = [
    "................................",
    "................................",
    "................................",
    "........hhhh..........cccc......",
    ".......hhhhhh........cccccc.....",
    ".......hssssh........csssscc....",
    ".......ssssss........cssssc.....",
    "........ssss..........ssss......",
    "........tttt..........dddd......",
    "......tttttttt......dddddddd....",
    ".....tttttttttt....dddddddddd...",
    ".....tt.tttt.tt....dd.dddd.dd...",
    ".....tt.tttt.tt....dd.dddd.dd...",
    ".....tt.tttt.tt....dd.daad.dd...",
    ".....ss.bbbb.ss....ss.daad.ss...",
    "........tttt..........daad......",
    "........tttt.........ddaadd.....",
    "........tttt.........ddaadd.....",
    "........tttt........dddaaddd....",
    "........pp.pp.......dddaaddd....",
    "........pp.pp.......dddddddd....",
    "........pp.pp......dddddddddd...",
    "........pp.pp......dddddddddd...",
    "........pp.pp......dddddddddd...",
    ".......kkk.kkk.......kk..kk.....",
    "................................",
]
KOLORY = {"h": (92, 64, 40), "s": (206, 160, 124), "t": (120, 70, 50), "b": (66, 48, 34), "p": (98, 88, 72),
          "k": (54, 40, 30), "c": (224, 214, 192), "d": (78, 88, 118), "a": (196, 186, 162)}


def rodzina():
    im = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    for y, wiersz in enumerate(RODZINA):
        for x, z in enumerate(wiersz):
            if z in KOLORY:
                im.putpixel((x, y + 3), KOLORY[z] + (255,))
    # ciemny obrys (czytelny na jasnym i ciemnym tle)
    obrys = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    px = im.load()
    for y in range(32):
        for x in range(32):
            if px[x, y][3] == 0 and any(0 <= x + dx < 32 and 0 <= y + dy < 32 and px[x + dx, y + dy][3] > 0
                                        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                obrys.putpixel((x, y), (28, 22, 16, 220))
    obrys.alpha_composite(im)
    return obrys.resize((128, 128), Image.NEAREST)


def grosze():
    """Srebrny grosz z krzyzem (16 x 16): ciemny brzeg, jasne lico, obwodka i krzyz ciemniejsze; pod spodem drugi grosz."""
    im = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    for cx, cy in ((9.5, 9.5), (6.5, 6.5)):  # najpierw dolny (z tylu), potem gorny
        for y in range(16):
            for x in range(16):
                d = ((x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2) ** 0.5
                if d > 6.0:
                    continue
                if d > 5.0:
                    c = (34, 30, 28, 240)
                elif d > 4.0:
                    c = (150, 150, 146, 255)
                elif abs(x + 0.5 - cx) < 0.9 or abs(y + 0.5 - cy) < 0.9:
                    c = (128, 128, 124, 255)
                else:
                    c = (214, 214, 206, 255)
                im.putpixel((x, y), c)
    return im.resize((128, 128), Image.NEAREST)


SERCE = [
    "................",
    "................",
    "...xxx....xxx...",
    "..xrrrx..xrrrx..",
    ".xrrjrrxxrrrrrx.",
    ".xrjjrrrrrrrrrx.",
    ".xrrrrrrrrrrrrx.",
    ".xrrrrrrrrrrrrx.",
    "..xrrrrrrrrrrx..",
    "...xrrrrrrrrx...",
    "....xrrrrrrx....",
    ".....xrrrrx.....",
    "......xrrx......",
    ".......xx.......",
    "................",
    "................",
]


def serce():
    kol = {"x": (40, 22, 18, 230), "r": (176, 70, 58, 255), "j": (226, 150, 128, 255)}
    im = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    for y, w in enumerate(SERCE):
        for x, z in enumerate(w):
            if z in kol:
                im.putpixel((x, y), kol[z])
    return im.resize((128, 128), Image.NEAREST)


# --- Znaki paska (glify) ---

JASNY = (234, 218, 182, 255)
CIEN = (172, 148, 104, 255)
OBRYS = (30, 22, 15, 235)


def _obrys(im):
    px = im.load()
    W, H = im.size
    o = Image.new("RGBA", im.size, (0, 0, 0, 0))
    for y in range(H):
        for x in range(W):
            if px[x, y][3] == 0 and any(0 <= x + dx < W and 0 <= y + dy < H and px[x + dx, y + dy][3] > 0
                                        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (-1, -1), (1, -1), (-1, 1))):
                o.putpixel((x, y), OBRYS)
    o.alpha_composite(im)
    return o


def _linia(d, a, b, w, k):
    d.line([a, b], fill=k, width=w)


def znak(nazwa):
    im = Image.new("RGBA", (32, 32), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    J, C = JASNY, CIEN
    if nazwa == "Budowa":          # mlotek ciesielski
        _linia(d, (7, 26), (19, 14), 3, C)
        d.polygon([(13, 8), (19, 2), (27, 10), (21, 16)], fill=J)
        d.polygon([(21, 16), (27, 10), (28, 11), (22, 17)], fill=C)
        d.rectangle([5, 25, 8, 28], fill=C)
    elif nazwa == "Wojsko":        # dwie skrzyzowane wlocznie i tarcza
        _linia(d, (5, 27), (26, 6), 2, C)
        _linia(d, (26, 27), (5, 6), 2, C)
        d.polygon([(30, 1), (22, 5), (26, 9)], fill=J)
        d.polygon([(1, 1), (9, 5), (5, 9)], fill=J)
        d.polygon([(10, 11), (21, 11), (21, 19), (15, 26), (10, 19)], fill=J)
        d.polygon([(15, 11), (21, 11), (21, 19), (15, 26)], fill=C)
    elif nazwa == "Rody":          # choragiew na drzewcu
        _linia(d, (8, 3), (8, 29), 2, C)
        d.polygon([(10, 5), (26, 5), (22, 11), (26, 17), (10, 17)], fill=J)
        d.rectangle([14, 9, 18, 13], fill=C)
        d.ellipse([6, 1, 10, 5], fill=J)
    elif nazwa == "Mieszkalne":    # dom ze szczytem
        d.polygon([(4, 15), (16, 4), (28, 15)], fill=C)
        d.rectangle([7, 15, 25, 28], fill=J)
        d.rectangle([14, 20, 18, 28], fill=C)
        d.rectangle([9, 18, 12, 21], fill=C)
    elif nazwa == "Surowce":       # siekiera wbita w klode
        d.ellipse([3, 20, 13, 29], fill=C)
        d.rectangle([8, 20, 28, 29], fill=J)
        d.ellipse([5, 22, 11, 27], fill=J)
        _linia(d, (17, 22), (26, 4), 3, C)
        d.polygon([(10, 6), (21, 9), (18, 16), (8, 15)], fill=J)
        d.polygon([(18, 9), (21, 9), (18, 16)], fill=C)
    elif nazwa == "Jedzenie":      # bochen chleba i dzban
        d.ellipse([3, 14, 21, 27], fill=J)
        _linia(d, (8, 17), (11, 23), 1, C)
        _linia(d, (13, 16), (16, 22), 1, C)
        d.rectangle([21, 9, 28, 27], fill=C)
        d.rectangle([22, 6, 27, 9], fill=C)
        d.arc([25, 12, 31, 20], 270, 90, fill=C, width=2)
    elif nazwa == "Rolnictwo":     # snop zboza
        for x in (9, 13, 16, 19, 23):
            _linia(d, (16, 28), (x, 6), 2, J)
        for x, y in ((8, 4), (13, 3), (16, 2), (19, 3), (24, 4)):
            d.ellipse([x - 2, y - 1, x + 2, y + 5], fill=J)
        d.rectangle([11, 17, 21, 19], fill=C)
    elif nazwa == "Rzemioslo":     # kowadlo
        d.polygon([(3, 9), (28, 9), (28, 14), (21, 16), (11, 16), (3, 12)], fill=J)
        d.rectangle([13, 16, 19, 23], fill=C)
        d.rectangle([8, 23, 24, 28], fill=J)
    elif nazwa == "Handel":        # waga kupiecka
        _linia(d, (16, 3), (16, 26), 2, C)
        _linia(d, (5, 8), (27, 8), 2, J)
        d.rectangle([10, 26, 22, 29], fill=J)
        for x in (5, 27):
            _linia(d, (x, 9), (x - 3, 18), 1, C)
            _linia(d, (x, 9), (x + 3, 18), 1, C)
            d.chord([x - 5, 14, x + 5, 23], 0, 180, fill=J)
    elif nazwa == "Religia":       # krzyz
        d.rectangle([13, 2, 19, 29], fill=J)
        d.rectangle([6, 8, 26, 14], fill=J)
        d.rectangle([17, 2, 19, 29], fill=C)
        d.rectangle([6, 12, 26, 14], fill=C)
    elif nazwa == "Wojsko_Kat":    # zakladka budynkow wojska: tarcza
        d.polygon([(6, 5), (26, 5), (26, 17), (16, 29), (6, 17)], fill=J)
        d.polygon([(16, 5), (26, 5), (26, 17), (16, 29)], fill=C)
        d.rectangle([14, 9, 18, 22], fill=OBRYS[:3] + (180,))
    elif nazwa == "Mury":          # zakladka murow: kawalek muru z blankami i wieza
        d.rectangle([2, 15, 30, 28], fill=J)
        for x in (2, 8, 14, 20, 26):
            d.rectangle([x, 11, x + 3, 15], fill=J)
        d.rectangle([19, 4, 30, 28], fill=J)
        for x in (19, 23, 27):
            d.rectangle([x, 1, x + 2, 4], fill=J)
        d.rectangle([19, 4, 30, 28], outline=C)
        d.rectangle([2, 22, 18, 28], fill=C)
        d.rectangle([23, 12, 26, 18], fill=C)
    return _obrys(im).resize((128, 128), Image.NEAREST)


ZNAKI = ["Budowa", "Wojsko", "Rody", "Mieszkalne", "Surowce", "Jedzenie", "Rolnictwo", "Rzemioslo", "Handel", "Religia", "Wojsko_Kat", "Mury"]


def main():
    for nazwa, faza in IKONY.items():
        czesci = None
        if nazwa in ZLOZONE:
            czesci = []
            for model, o in ZLOZONE[nazwa]:
                czesci += [(a, P + o, N, C, UV, T) for a, P, N, C, UV, T in wczytaj(os.path.join(ART, model + ".glb"))]
        im = renderuj(os.path.join(ART, f"{nazwa}_F{faza}.glb"), W=256, H=256, yaw=55, ziemia=False, dopasuj=True, pikselizuj=False, obetnij_z=-0.05, czesci=czesci)
        # ~64 piksele obrazu na ikone (jak efekt pikseli w grze), potem 2x bez wygladzania
        im = im.resize((64, 64), Image.BILINEAR).resize((128, 128), Image.NEAREST)
        plik = os.path.join(ART, "T_Ikona_" + nazwa[3:] + ".png")
        im.save(plik)
        print(os.path.basename(plik))
    for nazwa, model in SUROWCE.items():
        # stojak z wloczniami widac z przodu (pod katem 35 st. byl prawie bokiem)
        im = renderuj(os.path.join(ART, model + ".glb"), W=256, H=256, yaw=100 if nazwa == "Bron" else 35, pitch=34, ziemia=False, dopasuj=True, pikselizuj=False)
        im = im.resize((32, 32), Image.BILINEAR).resize((128, 128), Image.NEAREST)
        plik = os.path.join(ART, "T_Ikona_" + nazwa + ".png")
        im.save(plik)
        print(os.path.basename(plik))
    plik = os.path.join(ART, "T_Ikona_Zadowolenie.png")
    serce().save(plik)
    print(os.path.basename(plik))
    plik = os.path.join(ART, "T_Ikona_Grosze.png")
    grosze().save(plik)
    print(os.path.basename(plik))
    plik = os.path.join(ART, "T_Ikona_Rodzina.png")
    rodzina().save(plik)
    print(os.path.basename(plik))
    for n in ZNAKI:
        plik = os.path.join(ART, "T_Ikona_Znak_" + n + ".png")
        znak(n).save(plik)
        print(os.path.basename(plik))


if __name__ == "__main__":
    main()
