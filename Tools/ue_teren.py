# Plik: Tools/ue_teren.py
# Uruchamiany WEWNATRZ Unreala (commandlet), buduje assety terenu:
#   /Game/Swiat/Teren/T_Europa_Kolor, T_Detal (import z Art/Teren) i material M_Teren.
# Uruchomienie: Tools\Zbuduj_Teren.bat (edytor z tym projektem musi byc zamkniety).
#
# M_Teren:
#   UV1 = wspolrzedne calej mapy (0..1) z C++ -> T_Europa_Kolor (kolor pokrycia, morze wg glebokosci)
#   UV0 = 1 jednostka na 4 m -> T_Detal w 3 skalach (4 m, 32 m, 256 m): kepki trawy, laty, duze plamy
#   granice klas falowane szumem 256 m (bez kanciastych pikseli 64 m), strome zbocza -> skala
import os
import unreal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCEL = "/Game/Swiat/Teren"
MEL = unreal.MaterialEditingLibrary
assets = unreal.AssetToolsHelpers.get_asset_tools()


def importuj(plik, srgb, kompresja=unreal.TextureCompressionSettings.TC_MASKS):
    sciezka = DOCEL + "/" + os.path.splitext(plik)[0]
    # Ponowny import na istniejacy asset wywraca commandlet (assert !IsRooted) - zeby podmienic teksture,
    # usun najpierw Content/Swiat/Teren/<nazwa>.uasset.
    if unreal.EditorAssetLibrary.does_asset_exist(sciezka):
        return bez_streamingu(unreal.load_asset(sciezka))
    t = unreal.AssetImportTask()
    t.filename = os.path.join(ROOT, "Art", "Teren", plik)
    t.destination_path = DOCEL
    t.automated = True
    t.replace_existing = True
    t.save = False
    assets.import_asset_tasks([t])
    tex = unreal.load_asset(DOCEL + "/" + os.path.splitext(plik)[0])
    tex.set_editor_property("srgb", srgb)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    if not srgb:
        tex.set_editor_property("compression_settings", kompresja)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)
    return bez_streamingu(tex)


def bez_streamingu(tex):
    """Zawsze pelna rozdzielczosc: ProceduralMeshComponent nie podaje streamerowi gestosci tekseli, wiec mapa Europy
    potrafila zostac na niskich mipach (rozmyte, kanciaste plamy na oddaleniu). 8192^2: kolor ~43 MB, woda ~85 MB."""
    if not tex.get_editor_property("never_stream"):
        tex.set_editor_property("never_stream", True)
        unreal.EditorAssetLibrary.save_loaded_asset(tex)
    return tex


def wezel(mat, klasa, x, y, **wlasciwosci):
    n = MEL.create_material_expression(mat, klasa, x, y)
    for k, v in wlasciwosci.items():
        n.set_editor_property(k, v)
    return n


def lacz(a, wyj, b, wej):
    assert MEL.connect_material_expressions(a, wyj, b, wej), f"nie polaczono {a.get_name()}.{wyj} -> {b.get_name()}.{wej}"


def material(kolor, detal, woda, trawa_tex, mapa_tex):
    # Zbuduj_Teren.bat kasuje stary M_Teren.uasset (czyszczenie istniejacego wywraca commandlet).
    mat = assets.create_asset("M_Teren", DOCEL, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("float_precision_mode", unreal.MaterialFloatPrecisionMode.MFPM_FULL)  # UV mapy Europy w pelnej precyzji

    # UV nie z mapy w siatce: ProceduralMeshComponent trzyma UV w polowie precyzji (16 bit), co przy mapie 0..1 dawalo
    # kwadraty 128-256 m. Pozycja w swiecie / LocalPosition tez zawiodly: w pikselu liczone z glebi, na oddaleniu
    # (kafle setek km) bledy inne w kazdym kaflu - szwy, trojkaty przy brzegach, smugi i skoki przy zmianie poziomu kafli.
    # Teraz: UV1 siatki = pozycja w kaflu (i/K, j/K - dokladne nawet w 16 bitach), a C++ podaje w double jako
    # Custom Primitive Data: 0,1 = UV mapy Europy rogu kafla; 2,3 = detal rogu kafla (okres 1024); 4 = bok kafla / Bok.
    #   uv1 = cala Europa 0..1 (U = wschod, V = poludnie), uv0 = 1 jednostka na 4 m.
    BOK = 52428800.0
    tc = wezel(mat, unreal.MaterialExpressionTextureCoordinate, -2350, -150, coordinate_index=1)
    cpd = [wezel(mat, unreal.MaterialExpressionScalarParameter, -2050, 50 + 60 * k, parameter_name=f"Kafel{k}",
                 use_custom_primitive_data=True, primitive_data_index=k) for k in range(5)]
    tcs = wezel(mat, unreal.MaterialExpressionMultiply, -2200, -150)  # pozycja w kaflu jako ulamek calej Europy
    lacz(tc, "", tcs, "A"); lacz(cpd[4], "", tcs, "B")
    lx = wezel(mat, unreal.MaterialExpressionComponentMask, -2050, -250, r=True, g=False, b=False, a=False)
    lacz(tcs, "", lx, "")
    ly = wezel(mat, unreal.MaterialExpressionComponentMask, -2050, -150, r=False, g=True, b=False, a=False)
    lacz(tcs, "", ly, "")

    def plus_skala(baza, lok, skala, x, y):
        m = wezel(mat, unreal.MaterialExpressionMultiply, x, y, const_b=skala)
        lacz(lok, "", m, "A")
        s = wezel(mat, unreal.MaterialExpressionAdd, x + 100, y)
        lacz(baza, "", s, "A"); lacz(m, "", s, "B")
        return s

    u1 = plus_skala(cpd[0], ly, 1.0, -1950, -150)
    v1 = plus_skala(cpd[1], lx, -1.0, -1950, -250)
    uv1 = wezel(mat, unreal.MaterialExpressionAppendVector, -1700, -300)
    lacz(u1, "", uv1, "A"); lacz(v1, "", uv1, "B")
    u0 = plus_skala(cpd[2], lx, BOK / 400.0, -1950, 0)
    v0 = plus_skala(cpd[3], ly, BOK / 400.0, -1950, 80)
    uv0 = wezel(mat, unreal.MaterialExpressionAppendVector, -1700, 0)
    lacz(u0, "", uv0, "A"); lacz(v0, "", uv0, "B")
    LIN = unreal.MaterialSamplerType.SAMPLERTYPE_MASKS  # T_Detal ma kompresje TC_MASKS

    # Detal w 3 skalach (kafelkujaca tekstura; skale 1, 1/8, 1/64 dziela 1024 - ciaglosc miedzy kaflami).
    uv_s = wezel(mat, unreal.MaterialExpressionMultiply, -1400, 150, const_b=0.125)
    lacz(uv0, "", uv_s, "A")
    uv_d = wezel(mat, unreal.MaterialExpressionMultiply, -1400, 300, const_b=1.0 / 64.0)
    lacz(uv0, "", uv_d, "A")
    t_m = wezel(mat, unreal.MaterialExpressionTextureSample, -1200, 0, texture=detal, sampler_type=LIN)
    lacz(uv0, "", t_m, "UVs")
    t_s = wezel(mat, unreal.MaterialExpressionTextureSample, -1200, 200, texture=detal, sampler_type=LIN)
    lacz(uv_s, "", t_s, "UVs")
    t_d = wezel(mat, unreal.MaterialExpressionTextureSample, -1200, 400, texture=detal, sampler_type=LIN)
    lacz(uv_d, "", t_d, "UVs")

    # Falowanie granic klas (bez kwadratow 64 m z bliska), osobno w U i V:
    #   przesuniecie [piksele mapy] = (B - 0,5) * 8 + (G - 0,5) * 2, B = plamy 43-256 m, G = 10-64 m
    #   U z t_d, V z drugiej probki t_d przesunietej o (0,37; 0,61). Ten sam wzor liczy FSwiat::Pokrycie w C++
    #   (drzewa rosna tam, gdzie kolor lasu) - zmieniajac go tutaj, zmien tez SwiatWysokosci.cpp.
    uv_d2 = wezel(mat, unreal.MaterialExpressionAdd, -1300, 500)
    lacz(uv_d, "", uv_d2, "A")
    przes = wezel(mat, unreal.MaterialExpressionConstant2Vector, -1450, 550, r=0.37, g=0.61)
    lacz(przes, "", uv_d2, "B")
    t_d2 = wezel(mat, unreal.MaterialExpressionTextureSample, -1200, 600, texture=detal, sampler_type=LIN)
    lacz(uv_d2, "", t_d2, "UVs")

    def fala(t, x, y):
        b = wezel(mat, unreal.MaterialExpressionSubtract, x, y, const_b=0.5)
        lacz(t, "B", b, "A")
        b8 = wezel(mat, unreal.MaterialExpressionMultiply, x + 120, y, const_b=8.0)
        lacz(b, "", b8, "A")
        g = wezel(mat, unreal.MaterialExpressionSubtract, x, y + 60, const_b=0.5)
        lacz(t, "G", g, "A")
        g2 = wezel(mat, unreal.MaterialExpressionMultiply, x + 120, y + 60, const_b=2.0)
        lacz(g, "", g2, "A")
        s = wezel(mat, unreal.MaterialExpressionAdd, x + 240, y)
        lacz(b8, "", s, "A")
        lacz(g2, "", s, "B")
        return s

    fu = fala(t_d, -1050, -260)
    fv = fala(t_d2, -1050, -140)
    fuv = wezel(mat, unreal.MaterialExpressionAppendVector, -750, -200)
    lacz(fu, "", fuv, "A")
    lacz(fv, "", fuv, "B")
    w2 = wezel(mat, unreal.MaterialExpressionMultiply, -650, -200, const_b=1.0 / 8192.0)
    lacz(fuv, "", w2, "A")
    uv1w = wezel(mat, unreal.MaterialExpressionAdd, -600, -300)
    lacz(uv1, "", uv1w, "A")
    lacz(w2, "", uv1w, "B")
    t_kol = wezel(mat, unreal.MaterialExpressionTextureSample, -550, -300, texture=kolor)
    lacz(uv1w, "", t_kol, "UVs")

    # Strome zbocza -> skala: saturate((0,55 - normal.z) * 6). Przy przesadzie pionowej x2,5 to ~30 st. naprawde;
    # wczesniejsze 0,8 robilo skale z lagodnych pagorkow (bezowe pasy).
    nor = wezel(mat, unreal.MaterialExpressionVertexNormalWS, -1000, 600)
    nz = wezel(mat, unreal.MaterialExpressionComponentMask, -850, 600, r=False, g=False, b=True, a=False)
    lacz(nor, "", nz, "")
    s1 = wezel(mat, unreal.MaterialExpressionSubtract, -700, 600, const_a=0.55)
    lacz(nz, "", s1, "B")
    s2 = wezel(mat, unreal.MaterialExpressionMultiply, -550, 600, const_b=6.0)
    lacz(s1, "", s2, "A")
    stromo = wezel(mat, unreal.MaterialExpressionSaturate, -400, 600)
    lacz(s2, "", stromo, "")
    skala = wezel(mat, unreal.MaterialExpressionConstant3Vector, -400, 450, constant=unreal.LinearColor(0.155, 0.133, 0.107, 1))
    baza = wezel(mat, unreal.MaterialExpressionLinearInterpolate, -250, -100)
    lacz(t_kol, "RGB", baza, "A")
    lacz(skala, "", baza, "B")
    lacz(stromo, "", baza, "Alpha")

    # Jasnosc detalu. Rozmiary plamek (T_Detal: R 24-160, G 4-24, B 1-6 okresow na kafel tekstury):
    #   t_m (kafel 4 m):  G = 17 cm..1 m     t_s (kafel 32 m): R = 20 cm..1,3 m (ziarno trawy), G = 1,3..8 m
    #   t_d (kafel 256 m): B = duze plamy. R z t_m (1..17 cm) znika w mipmapach, wiec go nie uzywamy.
    d1 = wezel(mat, unreal.MaterialExpressionLinearInterpolate, -950, 0, const_a=0.85, const_b=1.15)
    lacz(t_m, "G", d1, "Alpha")
    d2 = wezel(mat, unreal.MaterialExpressionLinearInterpolate, -950, 200, const_a=0.88, const_b=1.12)
    lacz(t_s, "G", d2, "Alpha")
    d3 = wezel(mat, unreal.MaterialExpressionLinearInterpolate, -950, 400, const_a=0.9, const_b=1.1)
    lacz(t_d, "B", d3, "Alpha")
    m1 = wezel(mat, unreal.MaterialExpressionMultiply, -750, 100)
    lacz(d1, "", m1, "A")
    lacz(d2, "", m1, "B")
    m2 = wezel(mat, unreal.MaterialExpressionMultiply, -600, 200)
    lacz(m1, "", m2, "A")
    lacz(d3, "", m2, "B")
    # Ziarnista trawa jak na zrzutach: mocne plamki 20 cm..1,3 m.
    d0 = wezel(mat, unreal.MaterialExpressionLinearInterpolate, -950, -150, const_a=0.65, const_b=1.3)
    lacz(t_s, "R", d0, "Alpha")
    jasn = wezel(mat, unreal.MaterialExpressionMultiply, -600, 0)
    lacz(m2, "", jasn, "A")
    lacz(d0, "", jasn, "B")
    # Odcien: laty suchszej (zoltawej) i swiezszej (chlodniejszej) trawy co kilka metrow.
    zolta = wezel(mat, unreal.MaterialExpressionConstant3Vector, -800, -250, constant=unreal.LinearColor(1.12, 1.03, 0.78, 1))
    swieza = wezel(mat, unreal.MaterialExpressionConstant3Vector, -800, -350, constant=unreal.LinearColor(0.88, 1.0, 0.95, 1))
    odcien = wezel(mat, unreal.MaterialExpressionLinearInterpolate, -600, -300)
    lacz(swieza, "", odcien, "A")
    lacz(zolta, "", odcien, "B")
    lacz(t_s, "G", odcien, "Alpha")
    detal_razem = wezel(mat, unreal.MaterialExpressionMultiply, -450, 200)
    lacz(jasn, "", detal_razem, "A")
    lacz(odcien, "", detal_razem, "B")

    lad_mapa = wezel(mat, unreal.MaterialExpressionMultiply, -150, 0)
    lacz(baza, "", lad_mapa, "A")
    lacz(detal_razem, "", lad_mapa, "B")
    # ===== Mapa grywalna (parametr "Region" = 1 w kaflach regionu, C++) - wyglad zrzutow wzorcowych =====
    # Ziemia: trawa ze zdzbel (T_Trawa) w kilku odmianach - swieza, sucha (oliwkowo-brazowa), wrzos (fiolet, gdzie
    # mapa ma zarosla), ciemniejsze runo pod lasem, zolte kwiatki. Przejscia miekkie i poszarpane: progi przesuwa szum zdzbel.
    # Mapy Europy (Region = 0) to nie dotyczy.
    region = wezel(mat, unreal.MaterialExpressionScalarParameter, -400, 100, parameter_name="Region", default_value=0.0)
    X, Y = 600, 1200  # wezly regionu rysowane osobno w grafie

    def k3(r, g, b, y):
        return wezel(mat, unreal.MaterialExpressionConstant3Vector, X, Y + y, constant=unreal.LinearColor(r, g, b, 1))

    def lerp(a, b, alfa, y, wa="", wb="", wal=""):
        n = wezel(mat, unreal.MaterialExpressionLinearInterpolate, X + 400, Y + y)
        lacz(a, wa, n, "A"); lacz(b, wb, n, "B"); lacz(alfa, wal, n, "Alpha")
        return n

    def razy(a, b, y, wa="", wb=""):
        n = wezel(mat, unreal.MaterialExpressionMultiply, X + 400, Y + y)
        lacz(a, wa, n, "A"); lacz(b, wb, n, "B")
        return n

    def prog(wart, od, do, szum, y, wwart=""):
        """smoothstep(od, do, wart + szum) - miekki, poszarpany prog."""
        s = wezel(mat, unreal.MaterialExpressionAdd, X + 200, Y + y)
        lacz(wart, wwart, s, "A"); lacz(szum, "", s, "B")
        n = wezel(mat, unreal.MaterialExpressionSmoothStep, X + 300, Y + y, const_min=od, const_max=do)
        lacz(s, "", n, "Value")
        return n

    # Kafel 64 m: zdzblo 2-4 m, kropka ~1 m = 1-3 piksele ekranu przy zwyklym oddaleniu (mniejsze rozmylyby sie w mipmapach).
    uv_t1 = wezel(mat, unreal.MaterialExpressionMultiply, X - 400, Y, const_b=1.0 / 16.0)
    lacz(uv0, "", uv_t1, "A")
    uv_t2a = wezel(mat, unreal.MaterialExpressionMultiply, X - 400, Y + 100, const_b=1.0 / 32.0)  # 128 m, druga warstwa (1/2^n - bez szwow)
    lacz(uv0, "", uv_t2a, "A")
    przes2 = wezel(mat, unreal.MaterialExpressionConstant2Vector, X - 500, Y + 150, r=0.3, g=0.7)
    uv_t2 = wezel(mat, unreal.MaterialExpressionAdd, X - 300, Y + 100)
    lacz(uv_t2a, "", uv_t2, "A"); lacz(przes2, "", uv_t2, "B")
    tA = wezel(mat, unreal.MaterialExpressionTextureSample, X - 200, Y, texture=trawa_tex, sampler_type=LIN)
    lacz(uv_t1, "", tA, "UVs")
    tB = wezel(mat, unreal.MaterialExpressionTextureSample, X - 200, Y + 200, texture=trawa_tex, sampler_type=LIN)
    lacz(uv_t2, "", tB, "UVs")

    # Jasnosc zdzbel i szum do poszarpanych przejsc.
    z1 = wezel(mat, unreal.MaterialExpressionLinearInterpolate, X, Y + 300, const_a=0.7, const_b=1.3)
    lacz(tA, "R", z1, "Alpha")
    z2 = wezel(mat, unreal.MaterialExpressionLinearInterpolate, X, Y + 380, const_a=0.85, const_b=1.15)
    lacz(tB, "G", z2, "Alpha")
    zdzbla = razy(z1, z2, 340)
    sz1 = wezel(mat, unreal.MaterialExpressionSubtract, X, Y + 460, const_b=0.5)
    lacz(tA, "R", sz1, "A")
    szum = wezel(mat, unreal.MaterialExpressionMultiply, X + 100, Y + 460, const_b=0.35)
    lacz(sz1, "", szum, "A")

    # Odmiany ziemi (liniowo; sRGB ze zrzutow w komentarzach).
    swieza = k3(0.098, 0.216, 0.034, 600)    # 88,128,52
    sucha = k3(0.181, 0.150, 0.048, 680)     # 118,108,62
    runo = k3(0.048, 0.098, 0.027, 760)      # 62,88,46 pod lasem
    wrzos = k3(0.162, 0.098, 0.181, 840)     # 112,88,118
    kwiat = k3(0.610, 0.485, 0.061, 920)     # 205,185,70

    # Sucha trawa: duze laty (43-256 m) + srednie (1-8 m) -> ~25% powierzchni.
    s1 = wezel(mat, unreal.MaterialExpressionMultiply, X, Y + 1000, const_b=0.3)
    lacz(t_s, "G", s1, "A")
    s2 = wezel(mat, unreal.MaterialExpressionAdd, X + 100, Y + 1000)
    lacz(t_d, "B", s2, "A"); lacz(s1, "", s2, "B")
    w_sucha = prog(s2, 0.68, 0.78, szum, 1000)
    # Las z mapy (ciemna zielen w T_Europa_Kolor) -> ciemniejsze runo.
    # Zielen liniowa: las 0,05-0,09, laka 0,15-0,3 -> (0,14 - G) * 15: las 0,8-1,3, laka < 0.
    l1 = wezel(mat, unreal.MaterialExpressionSubtract, X, Y + 1100, const_a=0.14)
    lacz(t_kol, "G", l1, "B")
    l2 = wezel(mat, unreal.MaterialExpressionMultiply, X + 100, Y + 1100, const_b=15.0)
    lacz(l1, "", l2, "A")
    w_las = prog(l2, 0.3, 0.6, szum, 1100)
    # Wrzos/zarosla z mapy: czerwien > zielen.
    h1 = wezel(mat, unreal.MaterialExpressionSubtract, X, Y + 1200)
    lacz(t_kol, "R", h1, "A"); lacz(t_kol, "G", h1, "B")
    h2 = wezel(mat, unreal.MaterialExpressionMultiply, X + 100, Y + 1200, const_b=25.0)
    lacz(h1, "", h2, "A")
    h3 = wezel(mat, unreal.MaterialExpressionAdd, X + 150, Y + 1200, const_b=0.2)  # zarosla 0,5-1,6; laka < 0,2
    lacz(h2, "", h3, "A")
    w_wrzos = prog(h3, 0.45, 0.6, szum, 1200)
    # Kwiatki: kropki T_Trawa.B tylko w niektorych latach swiezej trawy.
    f1 = wezel(mat, unreal.MaterialExpressionSmoothStep, X, Y + 1300, const_min=0.6, const_max=0.68)
    lacz(t_s, "G", f1, "Value")
    w_kwiat = razy(f1, tA, 1300, wb="B")

    c = lerp(swieza, sucha, w_sucha, 1400)
    c = lerp(c, runo, w_las, 1450)
    c = lerp(c, wrzos, w_wrzos, 1500)
    c = razy(c, zdzbla, 1550)
    duze = razy(c, d3, 1600)  # duze plamy jasnosci (256 m)
    c = lerp(duze, kwiat, w_kwiat, 1650)
    skala_z = razy(skala, zdzbla, 1700)
    ziemia = lerp(c, skala_z, stromo, 1750)

    lad = wezel(mat, unreal.MaterialExpressionLinearInterpolate, -100, 0)
    lacz(lad_mapa, "", lad, "A")
    lacz(ziemia, "", lad, "B")
    lacz(region, "", lad, "Alpha")

    # --- Woda z T_Europa_Woda: R = odleglosc od brzegu (0,5 = brzeg), G = glebokosc. ---
    t_w = wezel(mat, unreal.MaterialExpressionTextureSample, -550, -700, texture=woda,
                sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    lacz(uv1, "", t_w, "UVs")
    # Woda tam, gdzie R > ~0,505 (ostra krawedz, rzeka wezsza niz piksel mapy).
    wm = wezel(mat, unreal.MaterialExpressionSmoothStep, -350, -750, const_min=0.502, const_max=0.508)
    lacz(t_w, "R", wm, "Value")
    # Pas blotnistego brzegu tuz przy wodzie (~15 m).
    bm = wezel(mat, unreal.MaterialExpressionSmoothStep, -350, -650, const_min=0.487, const_max=0.5)
    lacz(t_w, "R", bm, "Value")

    # Mapa grywalna: rzeki z linii (C++ Data/rzeki.bin) - UV2.x = odleglosc od brzegu w metrach (ujemna w wodzie).
    # Rzeki z pikseli (T_Europa_Woda.B) sa tam wylaczone; jeziora i morze zostaja z tekstury.
    def jeden_minus(a, y, wa=""):
        n = wezel(mat, unreal.MaterialExpressionOneMinus, X + 300, Y + y)
        lacz(a, wa, n, "")
        return n

    def maks(a, b, y):
        n = wezel(mat, unreal.MaterialExpressionMax, X + 400, Y + y)
        lacz(a, "", n, "A"); lacz(b, "", n, "B")
        return n

    uv2 = wezel(mat, unreal.MaterialExpressionTextureCoordinate, X - 400, Y + 2700, coordinate_index=2)
    odl = wezel(mat, unreal.MaterialExpressionComponentMask, X - 300, Y + 2700, r=True, g=False, b=False, a=False)
    lacz(uv2, "", odl, "")
    k1 = wezel(mat, unreal.MaterialExpressionMultiply, X, Y + 2650, const_b=2.0)
    lacz(t_w, "B", k1, "A")
    k2 = wezel(mat, unreal.MaterialExpressionSaturate, X + 100, Y + 2650)
    lacz(k1, "", k2, "")
    bez_pikseli = jeden_minus(razy(k2, region, 2650), 2650)        # 0 = piksel rzeki na mapie grywalnej
    rw0 = wezel(mat, unreal.MaterialExpressionSmoothStep, X, Y + 2750, const_min=-0.3, const_max=0.3)
    lacz(odl, "", rw0, "Value")
    rw = razy(jeden_minus(rw0, 2750), region, 2750)                # woda rzeki z linii (ostra krawedz)
    rb0 = wezel(mat, unreal.MaterialExpressionSmoothStep, X, Y + 2850, const_min=2.0, const_max=5.0)
    lacz(odl, "", rb0, "Value")
    rb = razy(jeden_minus(rb0, 2850), region, 2850)                # pas blota ~2-5 m
    wm_t = maks(razy(wm, bez_pikseli, 2900), rw, 2900)
    bm_t = maks(razy(bm, bez_pikseli, 2950), rb, 2950)
    # Glebokosc rzeki z linii: srodek koryta ciemniejszy (6 m od brzegu = gleboko).
    g1 = wezel(mat, unreal.MaterialExpressionMultiply, X, Y + 3000, const_b=-1.0 / 6.0)
    lacz(odl, "", g1, "A")
    g2 = wezel(mat, unreal.MaterialExpressionSaturate, X + 100, Y + 3000)
    lacz(g1, "", g2, "")
    g_al = lerp(t_w, g2, rw, 3000, wa="G")
    # Jasna linia wzdluz brzegu rzeki z linii (do 0,6 m od krawedzi w wodzie).
    q1 = wezel(mat, unreal.MaterialExpressionAdd, X, Y + 3100, const_b=0.6)
    lacz(odl, "", q1, "A")
    q2 = wezel(mat, unreal.MaterialExpressionAbs, X + 100, Y + 3100)
    lacz(q1, "", q2, "")
    q3 = wezel(mat, unreal.MaterialExpressionMultiply, X + 150, Y + 3100, const_b=1.5)
    lacz(q2, "", q3, "A")
    q4 = wezel(mat, unreal.MaterialExpressionSaturate, X + 200, Y + 3100)
    lacz(q3, "", q4, "")
    jeden = wezel(mat, unreal.MaterialExpressionConstant, X + 200, Y + 3150, r=1.0)
    krawedz_linii = lerp(jeden, q4, region, 3100)
    bloto = wezel(mat, unreal.MaterialExpressionConstant3Vector, -350, -550, constant=unreal.LinearColor(0.11, 0.08, 0.05, 1))
    bloto_d = wezel(mat, unreal.MaterialExpressionMultiply, -200, -550)
    lacz(bloto, "", bloto_d, "A")
    lacz(detal_razem, "", bloto_d, "B")
    # Region: brzeg jak na zrzutach - brazowe bloto ze zdzblami i jasnymi kamyczkami (kropki T_Trawa.B).
    bloto_r = razy(k3(0.127, 0.076, 0.038, 1900), zdzbla, 1950)  # 100,78,55
    kamyki = k3(0.40, 0.38, 0.35, 2000)
    k_alfa = wezel(mat, unreal.MaterialExpressionMultiply, X, Y + 2050, const_b=0.85)
    lacz(tB, "B", k_alfa, "A")
    bloto_r = lerp(bloto_r, kamyki, k_alfa, 2050)
    bloto_razem = lerp(bloto_d, bloto_r, region, 2100)
    z_brzegiem = wezel(mat, unreal.MaterialExpressionLinearInterpolate, 0, -300)
    lacz(lad, "", z_brzegiem, "A")
    lacz(bloto_razem, "", z_brzegiem, "B")
    lacz(bm_t, "", z_brzegiem, "Alpha")
    # Kolor wody: plytko jasnozielonkawa, gleboko granatowa. W regionie granat jak na zrzutach (60,105,130 / 40,78,112).
    plytko = lerp(k3(0.085, 0.175, 0.19, 2200), k3(0.045, 0.141, 0.223, 2250), region, 2220)
    gleboko = lerp(k3(0.019, 0.051, 0.085, 2300), k3(0.021, 0.076, 0.162, 2350), region, 2320)
    kol_w0 = wezel(mat, unreal.MaterialExpressionLinearInterpolate, -150, -900)
    lacz(plytko, "", kol_w0, "A")
    lacz(gleboko, "", kol_w0, "B")
    lacz(g_al, "", kol_w0, "Alpha")
    # Region: jasne blyski fal - kropki T_Trawa.B przesuwane wolno z pradem (Panner).
    uv_f = wezel(mat, unreal.MaterialExpressionMultiply, X - 400, Y + 2400, const_b=1.0 / 16.0)
    lacz(uv0, "", uv_f, "A")
    pan = wezel(mat, unreal.MaterialExpressionPanner, X - 300, Y + 2400, speed_x=0.01, speed_y=0.004)
    lacz(uv_f, "", pan, "Coordinate")
    tF = wezel(mat, unreal.MaterialExpressionTextureSample, X - 200, Y + 2400, texture=trawa_tex, sampler_type=LIN)
    lacz(pan, "", tF, "UVs")
    f_al = razy(tF, region, 2450, wa="B")
    f_al2 = wezel(mat, unreal.MaterialExpressionMultiply, X + 500, Y + 2450, const_b=0.5)
    lacz(f_al, "", f_al2, "A")
    kol_w = lerp(kol_w0, k3(0.30, 0.38, 0.42, 2500), f_al2, 2500)
    # Jasna linia na samej krawedzi wody (jak na zrzutach).
    p1 = wezel(mat, unreal.MaterialExpressionSubtract, -350, -1100, const_b=0.507)
    lacz(t_w, "R", p1, "A")
    p2 = wezel(mat, unreal.MaterialExpressionAbs, -250, -1100)
    lacz(p1, "", p2, "")
    p3 = wezel(mat, unreal.MaterialExpressionMultiply, -150, -1100, const_b=250.0)
    lacz(p2, "", p3, "A")
    p4a = wezel(mat, unreal.MaterialExpressionSaturate, -50, -1100)
    lacz(p3, "", p4a, "")
    # Linia z pikseli znika tam, gdzie rzeka z pikseli jest wylaczona; do tego linia rzeki z linii.
    p4b = maks(p4a, jeden_minus(bez_pikseli, 3200), 3200)
    p4 = wezel(mat, unreal.MaterialExpressionMin, X + 500, Y + 3250)
    lacz(p4b, "", p4, "A"); lacz(krawedz_linii, "", p4, "B")
    # lerp(jasna, woda, odleglosc od krawedzi): na krawedzi jasno, dalej kolor wody.
    piana = wezel(mat, unreal.MaterialExpressionLinearInterpolate, 50, -950)
    jasna = wezel(mat, unreal.MaterialExpressionConstant3Vector, -50, -1200, constant=unreal.LinearColor(0.35, 0.40, 0.36, 1))
    lacz(jasna, "", piana, "A")
    lacz(kol_w, "", piana, "B")
    lacz(p4, "", piana, "Alpha")

    wynik_teren = wezel(mat, unreal.MaterialExpressionLinearInterpolate, 200, -300)
    lacz(z_brzegiem, "", wynik_teren, "A")
    lacz(piana, "", wynik_teren, "B")
    lacz(wm_t, "", wynik_teren, "Alpha")

    # ===== Mapa strategiczna (pergamin): parametr "Mapa" 0..1 z C++ (oddalenie poza regionem) =====
    # SkalaMapy = log2(ile kafli T_Mapa na cala Europe). C++ dobiera ja do szerokosci widoku, wiec drzewka lasu
    # i linia brzegu maja na ekranie stala wielkosc. Dwie oktawy (2^n i 2^(n+1)) przenikaja sie wg czesci ulamkowej.
    mapa = wezel(mat, unreal.MaterialExpressionScalarParameter, 600, -1400, parameter_name="Mapa", default_value=0.0)
    MX, MY = 800, -2600

    def m3(r, g, b, y):
        return wezel(mat, unreal.MaterialExpressionConstant3Vector, MX, MY + y, constant=unreal.LinearColor(r, g, b, 1))

    def mop(klasa, a, b, y, wa="", wb=""):
        n = wezel(mat, klasa, MX + 300, MY + y)
        lacz(a, wa, n, "A"); lacz(b, wb, n, "B")
        return n

    def mlerp(a, b, alfa, y, wa="", wb="", wal=""):
        n = wezel(mat, unreal.MaterialExpressionLinearInterpolate, MX + 450, MY + y)
        lacz(a, wa, n, "A"); lacz(b, wb, n, "B"); lacz(alfa, wal, n, "Alpha")
        return n

    def jeden(klasa, wej, y, **kw):
        n = wezel(mat, klasa, MX + 300, MY + y, **kw)
        lacz(wej, "", n, "")
        return n

    def razy_c(a, c, y):
        n = wezel(mat, unreal.MaterialExpressionMultiply, MX + 300, MY + y, const_b=c)
        lacz(a, "", n, "A")
        return n

    def lerp_c(alfa, od, do, y, wal=""):
        n = wezel(mat, unreal.MaterialExpressionLinearInterpolate, MX + 300, MY + y, const_a=od, const_b=do)
        lacz(alfa, wal, n, "Alpha")
        return n

    def potega2(wyk, y):
        n = wezel(mat, unreal.MaterialExpressionPower, MX + 150, MY + y)
        lacz(dwa, "", n, "Base"); lacz(wyk, "", n, "Exp")
        return n

    skala_m = wezel(mat, unreal.MaterialExpressionScalarParameter, MX - 600, MY, parameter_name="SkalaMapy", default_value=8.0)
    dwa = wezel(mat, unreal.MaterialExpressionConstant, MX - 450, MY + 120, r=2.0)
    n0 = jeden(unreal.MaterialExpressionFloor, skala_m, 0)
    fr = jeden(unreal.MaterialExpressionFrac, skala_m, 60)
    uvA = mop(unreal.MaterialExpressionMultiply, uv1, potega2(n0, 0), 100)
    uvB = razy_c(uvA, 2.0, 150)
    mA = wezel(mat, unreal.MaterialExpressionTextureSample, MX + 450, MY + 100, texture=mapa_tex, sampler_type=LIN)
    lacz(uvA, "", mA, "UVs")
    mB = wezel(mat, unreal.MaterialExpressionTextureSample, MX + 450, MY + 300, texture=mapa_tex, sampler_type=LIN)
    lacz(uvB, "", mB, "UVs")
    drzewka = mlerp(mA, mB, fr, 400, wa="R", wb="R")
    papier = mlerp(mA, mB, fr, 450, wa="G", wb="G")

    # Lad: jasnoszary papier z wloknami; las ciemniejszy z jasnymi drzewkami; gory i skaly jasniejsze.
    pap = mop(unreal.MaterialExpressionMultiply, m3(0.30, 0.29, 0.27, 600), lerp_c(papier, 0.88, 1.1, 600), 650)
    las_k = mop(unreal.MaterialExpressionMultiply, pap, lerp_c(drzewka, 0.35, 0.95, 700), 700)
    las_m = wezel(mat, unreal.MaterialExpressionSmoothStep, MX + 300, MY + 750, const_min=0.3, const_max=0.6)
    lacz(l2, "", las_m, "Value")
    lad_m = mlerp(pap, las_k, las_m, 750)
    gory_m = wezel(mat, unreal.MaterialExpressionSmoothStep, MX + 300, MY + 800, const_min=0.09, const_max=0.2)
    lacz(t_kol, "B", gory_m, "Value")
    lad_m = mlerp(lad_m, razy_c(pap, 1.25, 850), gory_m, 850)

    # Woda: morze i jeziora z T_Europa_Woda bez rzek z pikseli (rzeki rysuje AMapaRzeki jako wstegi).
    # Przy brzegu ciemniejszy pas, na brzegu linia tuszu ~1,2 px ekranu: w pikselach mapy 103 / 2^SkalaMapy.
    bez_rzek = jeden(unreal.MaterialExpressionOneMinus, k2, 900)
    wm_m = mop(unreal.MaterialExpressionMultiply, wm, bez_rzek, 900)
    dpx0 = wezel(mat, unreal.MaterialExpressionSubtract, MX + 300, MY + 950, const_b=0.5)
    lacz(t_w, "R", dpx0, "A")
    dpx = razy_c(dpx0, 16.0, 1000)                                           # odleglosc od brzegu w px mapy (+ woda)
    hw = jeden(unreal.MaterialExpressionMax, razy_c(potega2(razy_c(skala_m, -1.0, 1050), 1050), 103.0, 1100), 1100, const_b=0.02)
    pas = jeden(unreal.MaterialExpressionSaturate, mop(unreal.MaterialExpressionDivide, dpx, razy_c(hw, 10.0, 1150), 1150), 1150)
    morze_m = mlerp(m3(0.045, 0.075, 0.10, 1200), m3(0.10, 0.15, 0.19, 1250), pas, 1200)
    t_abs = jeden(unreal.MaterialExpressionAbs, dpx, 1300)
    t_sat = jeden(unreal.MaterialExpressionSaturate, mop(unreal.MaterialExpressionDivide, t_abs, hw, 1300), 1350)
    tusz = mop(unreal.MaterialExpressionMultiply, jeden(unreal.MaterialExpressionOneMinus, t_sat, 1400), bez_rzek, 1400)
    mapa_kol = mlerp(lad_m, morze_m, wm_m, 1450)
    mapa_kol = mlerp(mapa_kol, m3(0.03, 0.03, 0.035, 1500), tusz, 1500)

    wynik = wezel(mat, unreal.MaterialExpressionLinearInterpolate, 600, -300)
    lacz(wynik_teren, "", wynik, "A"); lacz(mapa_kol, "", wynik, "B"); lacz(mapa, "", wynik, "Alpha")
    MEL.connect_material_property(wynik, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # Mapa matowa (bez polysku wody).
    szorstkosc0 = wezel(mat, unreal.MaterialExpressionLinearInterpolate, 200, 0, const_a=1.0, const_b=0.3)
    lacz(wm_t, "", szorstkosc0, "Alpha")
    szorstkosc = wezel(mat, unreal.MaterialExpressionLinearInterpolate, 400, 0, const_b=1.0)
    lacz(szorstkosc0, "", szorstkosc, "A"); lacz(mapa, "", szorstkosc, "Alpha")
    MEL.connect_material_property(szorstkosc, "", unreal.MaterialProperty.MP_ROUGHNESS)
    odbicie0 = wezel(mat, unreal.MaterialExpressionLinearInterpolate, 200, 150, const_a=0.15, const_b=0.5)
    lacz(wm_t, "", odbicie0, "Alpha")
    odbicie = wezel(mat, unreal.MaterialExpressionLinearInterpolate, 400, 150, const_b=0.1)
    lacz(odbicie0, "", odbicie, "A"); lacz(mapa, "", odbicie, "Alpha")
    MEL.connect_material_property(odbicie, "", unreal.MaterialProperty.MP_SPECULAR)

    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log("Teren: M_Teren gotowy")


def main():
    kolor = importuj("T_Europa_Kolor.png", True)
    detal = importuj("T_Detal.png", False)
    # BC7: gladkie pole odleglosci bez schodkow BC1 (8192^2 = 64 MB).
    woda = importuj("T_Europa_Woda.png", False, unreal.TextureCompressionSettings.TC_BC7)
    trawa = importuj("T_Trawa.png", False)  # mapa grywalna: zdzbla, kropki kwiatow/kamykow/blyskow
    mapa = importuj("T_Mapa.png", False)    # mapa strategiczna: drzewka lasu, faktura papieru
    material(kolor, detal, woda, trawa, mapa)
    material_rzek()


def material_rzek():
    """M_MapaRzeki: wstegi rzek mapy strategicznej (AMapaRzeki, C++). Wierzcholki leza na osi rzeki; WPO rozsuwa je
    na boki (normalna = kierunek w bok, UV0.y = -1/+1, UV0.x = wzgledna szerokosc rzeki) o parametr Szerokosc (cm,
    C++ dobiera do widoku - stala szerokosc na ekranie). Bez testu glebi: zawsze na wierzchu terenu.
    Srodek niebieski, brzegi ciemny tusz. Krycie = parametr Mapa."""
    mat = assets.create_asset("M_MapaRzeki", DOCEL, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("disable_depth_test", True)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("tangent_space_normal", False)
    mat.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
    uv = wezel(mat, unreal.MaterialExpressionTextureCoordinate, -900, 0)
    bok = wezel(mat, unreal.MaterialExpressionComponentMask, -750, 0, r=False, g=True, b=False, a=False)
    lacz(uv, "", bok, "")
    wzgl = wezel(mat, unreal.MaterialExpressionComponentMask, -750, 100, r=True, g=False, b=False, a=False)
    lacz(uv, "", wzgl, "")
    szer = wezel(mat, unreal.MaterialExpressionScalarParameter, -750, 200, parameter_name="Szerokosc", default_value=5000.0)
    nor = wezel(mat, unreal.MaterialExpressionVertexNormalWS, -750, 300)
    a = wezel(mat, unreal.MaterialExpressionMultiply, -550, 100); lacz(bok, "", a, "A"); lacz(wzgl, "", a, "B")
    b = wezel(mat, unreal.MaterialExpressionMultiply, -400, 150); lacz(a, "", b, "A"); lacz(szer, "", b, "B")
    wpo = wezel(mat, unreal.MaterialExpressionMultiply, -250, 250); lacz(b, "", wpo, "A"); lacz(nor, "", wpo, "B")
    MEL.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    # Kolor: srodek blekitny, przy krawedzi (|UV0.y| > 0,6) ciemny obrys.
    ab = wezel(mat, unreal.MaterialExpressionAbs, -550, -150); lacz(bok, "", ab, "")
    brzeg = wezel(mat, unreal.MaterialExpressionSmoothStep, -400, -150, const_min=0.55, const_max=0.75)
    lacz(ab, "", brzeg, "Value")
    nieb = wezel(mat, unreal.MaterialExpressionConstant3Vector, -400, -300, constant=unreal.LinearColor(0.07, 0.25, 0.48, 1))
    tusz = wezel(mat, unreal.MaterialExpressionConstant3Vector, -400, -400, constant=unreal.LinearColor(0.03, 0.04, 0.05, 1))
    kol = wezel(mat, unreal.MaterialExpressionLinearInterpolate, -200, -250)
    lacz(nieb, "", kol, "A"); lacz(tusz, "", kol, "B"); lacz(brzeg, "", kol, "Alpha")
    MEL.connect_material_property(kol, "", unreal.MaterialProperty.MP_BASE_COLOR)
    gora = wezel(mat, unreal.MaterialExpressionConstant3Vector, -200, 400, constant=unreal.LinearColor(0, 0, 1, 1))
    MEL.connect_material_property(gora, "", unreal.MaterialProperty.MP_NORMAL)  # oswietlenie jak plaski teren
    MEL.connect_material_property(wezel(mat, unreal.MaterialExpressionConstant, -200, 500, r=1.0), "", unreal.MaterialProperty.MP_ROUGHNESS)
    krycie = wezel(mat, unreal.MaterialExpressionScalarParameter, -200, 600, parameter_name="Mapa", default_value=1.0)
    MEL.connect_material_property(krycie, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log("Teren: M_MapaRzeki gotowy")


main()
