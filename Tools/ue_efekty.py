# Plik: Tools/ue_efekty.py
# Uruchamiany WEWNATRZ Unreala (commandlet): Art/Efekty (Tools/efekty.py) -> /Game/Swiat/Efekty.
# Materialy czastek AEfekty (kolor i jasnosc z danych instancji: PerInstanceCustomData 0..3 = R, G, B, A):
#   M_Efekt_Ogien - dodawany, nieoswietlony: T_Efekt_Plomien x szum plynacy w gore (kazda instancja inaczej) x kolor x A,
#                   miekko gasnie przy scianach i dachu (DepthFade), bez ostrych krawedzi przeciecia
#   M_Efekt_Iskra - dodawany: T_Efekt_Kropka x kolor x A
#   M_Efekt_Dym   - przezroczysty, nieoswietlony: T_Efekt_Dym x kolor, krycie = alfa tekstury x A x szum x DepthFade
#   M_Efekt_Gruz  - zwykly (oswietlony): kolor z danych instancji, tekstura drewna dla faktury
# Bez ditheringu i masek (migotanie przy TSR). Uruchomienie: Tools\Zbuduj_Efekty.bat.
import glob, os
import unreal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCEL = "/Game/Swiat/Efekty"
TMP = DOCEL + "/_import"
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
assets = unreal.AssetToolsHelpers.get_asset_tools()
MP = unreal.MaterialProperty


def tekstura(nazwa, szara=False):
    t = unreal.AssetImportTask()
    t.filename = os.path.join(ROOT, "Art", "Efekty", nazwa + ".png")
    t.destination_path = DOCEL
    t.automated = True
    t.save = False
    assets.import_asset_tasks([t])
    tex = unreal.load_asset(DOCEL + "/" + nazwa)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_EFFECTS)
    tex.set_editor_property("never_stream", True)
    if szara:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
        tex.set_editor_property("srgb", False)
    EAL.save_loaded_asset(tex)
    return tex


def wyr(mat, klasa, x, y, **wl):
    e = MEL.create_material_expression(mat, klasa, x, y)
    for k, v in wl.items():
        e.set_editor_property(k, v)
    return e


def polacz(a, wy, b, we):
    assert MEL.connect_material_expressions(a, wy, b, we), f"{a} {wy} -> {b} {we}"


def mnoz(mat, a, wa, b, wb, x, y):
    m = wyr(mat, unreal.MaterialExpressionMultiply, x, y)
    polacz(a, wa, m, "A")
    polacz(b, wb, m, "B")
    return m


def nowy(nazwa, blend, unlit=True):
    mat = assets.create_asset(nazwa, DOCEL, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", blend)
    if unlit:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    return mat


def dane(mat, x, y):
    """Kolor (RGB) i A z danych instancji."""
    rgb = wyr(mat, unreal.MaterialExpressionPerInstanceCustomData3Vector, x, y, data_index=0)
    a = wyr(mat, unreal.MaterialExpressionPerInstanceCustomData, x, y + 120, data_index=3)
    return rgb, a


def szum_plynacy(mat, szum, x, y, predkosc, skala):
    """Szum przesuwany w gore, kazda instancja z innym przesunieciem: 0,45 + 1,1 x szum."""
    uv = wyr(mat, unreal.MaterialExpressionTextureCoordinate, x, y, u_tiling=skala[0], v_tiling=skala[1])
    los = wyr(mat, unreal.MaterialExpressionPerInstanceRandom, x, y + 80)
    dod = wyr(mat, unreal.MaterialExpressionAdd, x + 150, y)
    polacz(uv, "", dod, "A")
    polacz(los, "", dod, "B")
    pan = wyr(mat, unreal.MaterialExpressionPanner, x + 300, y, speed_x=0.0, speed_y=predkosc)
    polacz(dod, "", pan, "Coordinate")
    s = wyr(mat, unreal.MaterialExpressionTextureSample, x + 450, y, texture=szum)
    polacz(pan, "", s, "UVs")
    m = wyr(mat, unreal.MaterialExpressionMultiply, x + 600, y, const_b=1.1)
    polacz(s, "R", m, "A")
    a = wyr(mat, unreal.MaterialExpressionAdd, x + 720, y, const_b=0.45)
    polacz(m, "", a, "A")
    return a


def ogien(tp, ts):
    mat = nowy("M_Efekt_Ogien", unreal.BlendMode.BLEND_ADDITIVE)
    t = wyr(mat, unreal.MaterialExpressionTextureSample, -1400, -200, texture=tp)
    rgb, a = dane(mat, -1400, 200)
    sz = szum_plynacy(mat, ts, -1400, 500, -0.9, (1.0, 0.5))
    m1 = mnoz(mat, t, "RGB", t, "A", -1000, -200)
    m2 = mnoz(mat, m1, "", rgb, "", -850, -150)
    m3 = mnoz(mat, m2, "", a, "", -700, -100)
    m4 = mnoz(mat, m3, "", sz, "", -550, -50)
    df = wyr(mat, unreal.MaterialExpressionDepthFade, -550, 150, fade_distance_default=60.0)
    m5 = mnoz(mat, m4, "", df, "", -350, 0)
    MEL.connect_material_property(m5, "", MP.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)


def iskra(tk):
    mat = nowy("M_Efekt_Iskra", unreal.BlendMode.BLEND_ADDITIVE)
    t = wyr(mat, unreal.MaterialExpressionTextureSample, -1000, -200, texture=tk)
    rgb, a = dane(mat, -1000, 200)
    m1 = mnoz(mat, t, "A", rgb, "", -700, -100)
    m2 = mnoz(mat, m1, "", a, "", -500, 0)
    MEL.connect_material_property(m2, "", MP.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)


def dym(td, ts):
    mat = nowy("M_Efekt_Dym", unreal.BlendMode.BLEND_TRANSLUCENT)
    t = wyr(mat, unreal.MaterialExpressionTextureSample, -1400, -200, texture=td)
    rgb, a = dane(mat, -1400, 200)
    sz = szum_plynacy(mat, ts, -1400, 500, -0.12, (0.7, 0.7))
    kol = mnoz(mat, t, "RGB", rgb, "", -900, -200)
    MEL.connect_material_property(kol, "", MP.MP_EMISSIVE_COLOR)
    o1 = mnoz(mat, t, "A", a, "", -900, 100)
    o2 = mnoz(mat, o1, "", sz, "", -750, 150)
    df = wyr(mat, unreal.MaterialExpressionDepthFade, -600, 150, fade_distance_default=180.0)
    polacz(o2, "", df, "Opacity")
    sat = wyr(mat, unreal.MaterialExpressionSaturate, -400, 150)
    polacz(df, "", sat, "")
    MEL.connect_material_property(sat, "", MP.MP_OPACITY)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)


def gruz():
    mat = nowy("M_Efekt_Gruz", unreal.BlendMode.BLEND_OPAQUE, unlit=False)
    mat.set_editor_property("two_sided", False)
    drewno = unreal.load_asset("/Game/Swiat/Budynki/T_Bud_Drewno")
    rgb, _ = dane(mat, -900, 200)
    if drewno:
        t = wyr(mat, unreal.MaterialExpressionTextureSample, -900, -200, texture=drewno)
        k = mnoz(mat, t, "RGB", rgb, "", -600, 0)
    else:
        k = rgb
    j = wyr(mat, unreal.MaterialExpressionMultiply, -400, 0, const_b=1.6)  # jak budynki (ekspozycja regionu -0,7 EV)
    polacz(k, "", j, "A")
    MEL.connect_material_property(j, "", MP.MP_BASE_COLOR)
    r = wyr(mat, unreal.MaterialExpressionConstant, -400, 300, r=0.92)
    MEL.connect_material_property(r, "", MP.MP_ROUGHNESS)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)


def importuj(plik, mat):
    nazwa = os.path.splitext(os.path.basename(plik))[0]
    folder = TMP + "/" + nazwa
    t = unreal.AssetImportTask()
    t.filename = plik
    t.destination_path = folder
    t.automated = True
    t.replace_existing = True
    t.save = False
    assets.import_asset_tasks([t])
    siatki = [a for a in (unreal.load_asset(p) for p in EAL.list_assets(folder, recursive=True)) if isinstance(a, unreal.StaticMesh)]
    assert len(siatki) == 1, f"{nazwa}: oczekiwano 1 siatki, jest {len(siatki)}"
    m = siatki[0]
    m.set_material(0, unreal.load_asset(DOCEL + "/" + mat))
    assert EAL.rename_asset(m.get_path_name(), DOCEL + "/" + nazwa), f"{nazwa}: nie przeniesiono"
    EAL.save_asset(DOCEL + "/" + nazwa)
    unreal.log_warning(f"Efekty: {nazwa} gotowy")


def main():
    tp, td, tk = tekstura("T_Efekt_Plomien"), tekstura("T_Efekt_Dym"), tekstura("T_Efekt_Kropka")
    ts = tekstura("T_Efekt_Szum", szara=True)
    ogien(tp, ts)
    iskra(tk)
    dym(td, ts)
    gruz()
    importuj(os.path.join(ROOT, "Art", "Efekty", "SM_Efekt_Plomien.glb"), "M_Efekt_Ogien")
    importuj(os.path.join(ROOT, "Art", "Efekty", "SM_Efekt_Kleks.glb"), "M_Efekt_Dym")
    importuj(os.path.join(ROOT, "Art", "Efekty", "SM_Efekt_Gruz.glb"), "M_Efekt_Gruz")
    EAL.delete_directory(TMP)
    unreal.log_warning("Efekty: gotowe")


main()
