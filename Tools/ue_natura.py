# Plik: Tools/ue_natura.py
# Uruchamiany WEWNATRZ Unreala (commandlet): importuje drzewa i krzaki z Art/Natura/*.glb (Tools/natura_drzewa.py)
# do /Game/Swiat/Natura/SM_<Gatunek>_<wariant> z Nanite. Materialy:
#   M_Natura      - liscie: T_Liscie (przeswity, maska) * kolor wierzcholkow * losowa jasnosc na drzewo, dwustronny
#   M_Natura_Kora - pien i galezie: kolor wierzcholkow
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


def stale(mat, szorstkosc, odbicie):
    for wartosc, wlasciwosc, y in ((szorstkosc, unreal.MaterialProperty.MP_ROUGHNESS, 300), (odbicie, unreal.MaterialProperty.MP_SPECULAR, 400)):
        c = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, y)
        c.set_editor_property("r", wartosc)
        MEL.connect_material_property(c, "", wlasciwosc)


def zapisz(mat):
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    return mat


def material_lisci(tex):
    mat = assets.create_asset("M_Natura", DOCEL, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("opacity_mask_clip_value", 0.4)
    kol = jasnosc_drzewa(mat, 2.0, 2.8)
    t = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -700, -300)
    t.set_editor_property("texture", tex)
    razem = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -200, -100)
    assert MEL.connect_material_expressions(kol, "", razem, "A")
    assert MEL.connect_material_expressions(t, "RGB", razem, "B")
    MEL.connect_material_property(razem, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(t, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    stale(mat, 0.9, 0.15)
    return zapisz(mat)


def material_kory():
    mat = assets.create_asset("M_Natura_Kora", DOCEL, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    MEL.connect_material_property(jasnosc_drzewa(mat, 1.2, 1.6), "", unreal.MaterialProperty.MP_BASE_COLOR)
    stale(mat, 0.95, 0.2)
    return zapisz(mat)


def importuj(plik, liscie, kora):
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
        m.set_material(i, liscie if "Lisc" in slot else kora)
    ns = m.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", True)
    m.set_editor_property("nanite_settings", ns)
    assert EAL.rename_asset(m.get_path_name(), DOCEL + "/" + nazwa), f"{nazwa}: nie przeniesiono"
    EAL.save_asset(DOCEL + "/" + nazwa)
    unreal.log_warning(f"Natura: {nazwa} gotowy, sloty {sloty}")


def main():
    liscie = material_lisci(tekstura())
    kora = material_kory()
    pliki = sorted(glob.glob(os.path.join(ROOT, "Art", "Natura", "*.glb")))
    assert pliki, "brak Art/Natura/*.glb - uruchom Tools/natura_drzewa.py"
    for p in pliki:
        importuj(p, liscie, kora)
    EAL.delete_directory(TMP)  # materialy z glTF, nieuzywane


main()
