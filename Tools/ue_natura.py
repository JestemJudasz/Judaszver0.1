# Plik: Tools/ue_natura.py
# Uruchamiany WEWNATRZ Unreala (commandlet): importuje drzewa i krzaki z Art/Natura/*.glb (Tools/natura_drzewa.py)
# do /Game/Swiat/Natura/SM_<Gatunek>_<wariant> z Nanite. Materialy:
#   M_Natura      - liscie: T_Liscie (przeswity, maska) * kolor wierzcholkow * losowa jasnosc na drzewo, dwustronny
#   M_Natura_Kora - pien i galezie: kolor wierzcholkow
#   M_Natura_Trawa - kepki trawy: jak kora, dwustronna, z drganiem zdzbel
#   Wszystkie z wiatrem (WPO, KOD_WIATRU).
# Uruchomienie: Tools\Zbuduj_Nature.bat (kasuje caly Content/Swiat/Natura - ponowny import wywraca commandlet).
import glob, os
import unreal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCEL = "/Game/Swiat/Natura"
TMP = DOCEL + "/_import"
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
assets = unreal.AssetToolsHelpers.get_asset_tools()


def tekstura():
    t = unreal.AssetImportTask()
    t.filename = os.path.join(ROOT, "Art", "Natura", "T_Liscie.png")
    t.destination_path = DOCEL
    t.automated = True
    t.save = False
    assets.import_asset_tasks([t])
    tex = unreal.load_asset(DOCEL + "/T_Liscie")
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    # Mipmapy trzymaja pokrycie maski - korony nie lysieja z daleka.
    tex.set_editor_property("do_scale_mips_for_alpha_coverage", True)
    tex.set_editor_property("alpha_coverage_thresholds", unreal.Vector4(0, 0, 0, 0.4))
    EAL.save_loaded_asset(tex)
    return tex


def jasnosc_drzewa(mat, od, do):
    """Kolor wierzcholkow * lerp(od..do, PerInstanceRandom) - kazde drzewo troche inne.
    Liscie ~x2,4: w grze korony wychodzily prawie czarne, na zrzucie wzorcowym sa zielone jak trawa, tylko ciemniejsze."""
    vc = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -700, 0)
    los = MEL.create_material_expression(mat, unreal.MaterialExpressionPerInstanceRandom, -700, 200)
    jas = MEL.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -500, 200)
    jas.set_editor_property("const_a", od)
    jas.set_editor_property("const_b", do)
    assert MEL.connect_material_expressions(los, "", jas, "Alpha")
    kol = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 0)
    assert MEL.connect_material_expressions(vc, "", kol, "A")
    assert MEL.connect_material_expressions(jas, "", kol, "B")
    return kol


def jasniej(mat, kolor, swiecenie, przeswit):
    """Rozjasnienie (21.11.2026): korony i trawa wychodzily prawie czarne (gesta geometria = cien wlasny + SSAO).
    Emisja = kolor * swiecenie (lekkie podswietlenie cienia), a przeswit > 0 wlacza Two Sided Foliage: swiatlo
    slonca przechodzi przez liscie od spodu (kolor podpowierzchniowy = kolor * przeswit)."""
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 550)
    e.set_editor_property("const_b", swiecenie)
    assert MEL.connect_material_expressions(kolor, "", e, "A")
    MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if przeswit > 0:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
        p = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -150, 650)
        p.set_editor_property("const_b", przeswit)
        assert MEL.connect_material_expressions(kolor, "", p, "A")
        MEL.connect_material_property(p, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)


def stale(mat, szorstkosc, odbicie):
    for wartosc, wlasciwosc, y in ((szorstkosc, unreal.MaterialProperty.MP_ROUGHNESS, 300), (odbicie, unreal.MaterialProperty.MP_SPECULAR, 400)):
        c = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, y)
        c.set_editor_property("r", wartosc)
        MEL.connect_material_property(c, "", wlasciwosc)


# Wiatr (21.11.2026, watek grafiki): WPO liczone w jednym wezle Custom.
#   LP = pozycja w drzewie (cm, 0 = podstawa), WP = pozycja w swiecie, R = losowa na drzewo, T = czas.
#   Kolysanie calego drzewa rosnie z wysokoscia (h^1,5), faza losowa na drzewo, a podmuchy wedruja po lesie fala
#   wzdluz kierunku wiatru. Drgania (lisci, zdzbel) tylko w materialach, ktore je maja - pien sie nie wygina.
KOD_WIATRU = r"""
float2 kier = float2(0.8, 0.6);
float h = max(LP.z, 0.0);
float podmuch = 0.55 + 0.45 * sin(T * 0.35 - dot(WP.xy, kier) * 0.0008);
float ph = R * 6.2832;
float s = sin(T * 0.9 + ph) * 0.7 + sin(T * 2.1 + ph * 1.7) * 0.3;
float k = pow(h / 1000.0, 1.5) * KOLYS;
float3 o = float3(kier * (0.35 + 0.65 * s) * k * podmuch, 0.0);
float d = sin(T * 4.3 + R * 13.0 + dot(WP.xyz, float3(0.031, 0.027, 0.019)));
o += float3(d, d * 0.7, d * 0.3) * DRGANIE * saturate(h / WYS_DRGAN) * podmuch;
return o;
"""


def wiatr(mat, kolys=25.0, drganie=0.0, wys_drgan=60.0):
    """Podlacza WPO wiatru do materialu. kolys = cm wychylenia na 10 m wysokosci, drganie = cm drgan (liscie/trawa)."""
    def w(klasa, x, y):
        return MEL.create_material_expression(mat, klasa, x, y)
    lp = w(unreal.MaterialExpressionLocalPosition, -900, 700)
    wp = w(unreal.MaterialExpressionWorldPosition, -900, 800)
    los = w(unreal.MaterialExpressionPerInstanceRandom, -900, 900)
    czas = w(unreal.MaterialExpressionTime, -900, 1000)
    c = w(unreal.MaterialExpressionCustom, -600, 800)
    kod = KOD_WIATRU.replace("KOLYS", f"{kolys:.3f}").replace("DRGANIE", f"{drganie:.3f}").replace("WYS_DRGAN", f"{wys_drgan:.3f}")
    c.set_editor_property("code", kod)
    c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    wejscia = []
    for nazwa in ("LP", "WP", "R", "T"):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", nazwa)
        wejscia.append(ci)
    c.set_editor_property("inputs", wejscia)
    for a, nazwa in ((lp, "LP"), (wp, "WP"), (los, "R"), (czas, "T")):
        assert MEL.connect_material_expressions(a, "", c, nazwa), nazwa
    MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
    try:  # Nanite: granica przesuniecia (bez niej obcinanie/migotanie przy krawedziach ekranu)
        mat.set_editor_property("max_world_position_offset_displacement", 120.0)
    except Exception as e:
        unreal.log_warning(f"Natura: brak max_world_position_offset_displacement ({e})")


def zapisz(mat):
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    return mat


def material_lisci(tex):
    mat = assets.create_asset("M_Natura", DOCEL, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    mat.set_editor_property("used_with_nanite", True)  # siatki z Nanite - bez flagi gra podstawia material domyslny
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("opacity_mask_clip_value", 0.4)
    kol = jasnosc_drzewa(mat, 2.6, 3.4)  # 21.11.2026: bylo 2,0-2,8 (korony prawie czarne)
    t = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -700, -300)
    t.set_editor_property("texture", tex)
    razem = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, -100)
    assert MEL.connect_material_expressions(kol, "", razem, "A")
    assert MEL.connect_material_expressions(t, "RGB", razem, "B")
    MEL.connect_material_property(razem, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(t, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    stale(mat, 0.9, 0.15)
    jasniej(mat, razem, 0.10, 0.7)
    wiatr(mat, kolys=45.0, drganie=9.0, wys_drgan=60.0)
    return zapisz(mat)


def material_kory(nazwa="M_Natura_Kora", trawa=False):
    """Kora (pien, galezie; kamienie): to samo kolysanie co liscie, bez drgan. Trawa: ta sama barwa, plus drgania zdzbel."""
    mat = assets.create_asset(nazwa, DOCEL, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    mat.set_editor_property("used_with_nanite", True)  # siatki z Nanite - bez flagi gra podstawia material domyslny
    kol = jasnosc_drzewa(mat, 1.7, 2.1) if trawa else jasnosc_drzewa(mat, 1.2, 1.6)  # trawa: bylo 1,2-1,6 (czarne kepki)
    MEL.connect_material_property(kol, "", unreal.MaterialProperty.MP_BASE_COLOR)
    stale(mat, 0.95, 0.2)
    if trawa:
        mat.set_editor_property("two_sided", True)
        jasniej(mat, kol, 0.12, 0.6)
        wiatr(mat, kolys=0.0, drganie=14.0, wys_drgan=70.0)
    else:
        wiatr(mat, kolys=45.0)
    return zapisz(mat)


def importuj(plik, liscie, kora, trawa):
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
    sloty = [str(s.get_editor_property("material_slot_name")) for s in m.get_editor_property("static_materials")]
    for i, slot in enumerate(sloty):
        m.set_material(i, liscie if "Lisc" in slot else (trawa if nazwa.startswith("SM_Trawa") else kora))
    ns = m.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", True)
    m.set_editor_property("nanite_settings", ns)
    assert EAL.rename_asset(m.get_path_name(), DOCEL + "/" + nazwa), f"{nazwa}: nie przeniesiono"
    EAL.save_asset(DOCEL + "/" + nazwa)
    unreal.log_warning(f"Natura: {nazwa} gotowy, sloty {sloty}")


def main():
    liscie = material_lisci(tekstura())
    kora = material_kory()
    trawa = material_kory("M_Natura_Trawa", trawa=True)
    pliki = sorted(glob.glob(os.path.join(ROOT, "Art", "Natura", "*.glb")))
    assert pliki, "brak Art/Natura/*.glb - uruchom Tools/natura_drzewa.py"
    for p in pliki:
        importuj(p, liscie, kora, trawa)
    EAL.delete_directory(TMP)  # materialy z glTF, nieuzywane


main()
