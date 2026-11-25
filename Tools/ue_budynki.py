# Plik: Tools/ue_budynki.py
# Uruchamiany WEWNATRZ Unreala (commandlet): importuje budynki z Art/Budynki/*.glb (Tools/budynki.py)
# do /Game/Swiat/Budynki/SM_<Budynek>_F<faza> z Nanite. Materialy (tekstura kafelkowa x kolor wierzcholkow):
#   M_Bud_Kamien, M_Bud_Dach, M_Bud_Drewno, M_Bud_Tynk, M_Bud_Strzecha
#   T_Ikona_<Budynek> - ikony paska budowy (Tools/ikony.py), tekstury UI bez mipmap
#   M_Bud_Podglad - polprzezroczysty, nieoswietlony, parametr "Kolor" (zielony = mozna stawiac, czerwony = nie)
# Uruchomienie: Tools\Zbuduj_Budynki.bat (kasuje caly Content/Swiat/Budynki - ponowny import wywraca commandlet).
import glob, os
import unreal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCEL = "/Game/Swiat/Budynki"
TMP = DOCEL + "/_import"
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
assets = unreal.AssetToolsHelpers.get_asset_tools()


def tekstura(nazwa):
    t = unreal.AssetImportTask()
    t.filename = os.path.join(ROOT, "Art", "Budynki", nazwa + ".png")
    t.destination_path = DOCEL
    t.automated = True
    t.save = False
    assets.import_asset_tasks([t])
    tex = unreal.load_asset(DOCEL + "/" + nazwa)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    tex.set_editor_property("never_stream", True)
    EAL.save_loaded_asset(tex)
    return tex


def ikona(nazwa):
    t = unreal.AssetImportTask()
    t.filename = os.path.join(ROOT, "Art", "Budynki", nazwa + ".png")
    t.destination_path = DOCEL
    t.automated = True
    t.save = False
    assets.import_asset_tasks([t])
    tex = unreal.load_asset(DOCEL + "/" + nazwa)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)  # piksele ikony ostre jak w grze
    tex.set_editor_property("never_stream", True)
    EAL.save_loaded_asset(tex)


def stala(mat, wartosc, wlasciwosc, y):
    c = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, y)
    c.set_editor_property("r", wartosc)
    MEL.connect_material_property(c, "", wlasciwosc)


def material(nazwa, tex, jasnosc, szorstkosc):
    """BaseColor = tekstura x kolor wierzcholkow x jasnosc."""
    mat = assets.create_asset(nazwa, DOCEL, unreal.Material, unreal.MaterialFactoryNew())
    t = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -800, -200)
    t.set_editor_property("texture", tex)
    vc = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexColor, -800, 100)
    m1 = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -500, 0)
    assert MEL.connect_material_expressions(t, "RGB", m1, "A")
    assert MEL.connect_material_expressions(vc, "", m1, "B")
    m2 = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 0)
    m2.set_editor_property("const_b", jasnosc)
    assert MEL.connect_material_expressions(m1, "", m2, "A")
    MEL.connect_material_property(m2, "", unreal.MaterialProperty.MP_BASE_COLOR)
    stala(mat, szorstkosc, unreal.MaterialProperty.MP_ROUGHNESS, 300)
    stala(mat, 0.25, unreal.MaterialProperty.MP_SPECULAR, 400)
    mat.set_editor_property("used_with_nanite", True)  # siatki budynkow maja Nanite; bez flagi gra brala material domyslny (szara kratka)
    mat.set_editor_property("used_with_instanced_static_meshes", True)  # plot i kafle pola (InstancedStaticMesh)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    return mat


def material_podgladu():
    mat = assets.create_asset("M_Bud_Podglad", DOCEL, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    kol = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, 0)
    kol.set_editor_property("parameter_name", "Kolor")
    kol.set_editor_property("default_value", unreal.LinearColor(0.2, 0.9, 0.3, 1.0))
    MEL.connect_material_property(kol, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    stala(mat, 0.45, unreal.MaterialProperty.MP_OPACITY, 200)
    mat.set_editor_property("used_with_instanced_static_meshes", True)  # podglad plotu i kafli pola
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    return mat


def importuj(plik, mats):
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
        klucz = next((k for k in mats if k in slot), None)
        assert klucz, f"{nazwa}: nieznany slot {slot}"
        m.set_material(i, mats[klucz])
    ns = m.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", True)
    m.set_editor_property("nanite_settings", ns)
    assert EAL.rename_asset(m.get_path_name(), DOCEL + "/" + nazwa), f"{nazwa}: nie przeniesiono"
    EAL.save_asset(DOCEL + "/" + nazwa)
    unreal.log_warning(f"Budynki: {nazwa} gotowy, sloty {sloty}")


def main():
    # Jasnosc jak dla drzew (liscie x2-2,8): region ma ekspozycje -0,7 EV i niebieskawe swiatlo nieba; przy x1 na zrzucie
    # z 20.11.2026 mury wyszly ciemnoszare, a dach prawie czarny zamiast jasnego wapienia i brazowego gontu ze wzoru.
    mats = {
        "Kamien": material("M_Bud_Kamien", tekstura("T_Bud_Kamien"), 1.4, 0.9),  # >1,5 przepala BaseColor (obcinany do 1) i ciosy znikaja
        "Dach": material("M_Bud_Dach", tekstura("T_Bud_Dach"), 2.0, 0.85),
        "Drewno": material("M_Bud_Drewno", tekstura("T_Bud_Drewno"), 1.8, 0.9),
        # jasne jak wapien kosciola: >1,5 przepala biel tynku i znika faktura
        "Tynk": material("M_Bud_Tynk", tekstura("T_Bud_Tynk"), 1.3, 0.95),
        "Strzecha": material("M_Bud_Strzecha", tekstura("T_Bud_Strzecha"), 1.5, 0.95),
    }
    material_podgladu()
    pliki = sorted(glob.glob(os.path.join(ROOT, "Art", "Budynki", "*.glb")))
    assert pliki, "brak Art/Budynki/*.glb - uruchom Tools/budynki.py"
    for p in pliki:
        importuj(p, mats)
    for p in sorted(glob.glob(os.path.join(ROOT, "Art", "Budynki", "T_Ikona_*.png"))):
        ikona(os.path.splitext(os.path.basename(p))[0])
    EAL.delete_directory(TMP)  # materialy z glTF, nieuzywane


main()
