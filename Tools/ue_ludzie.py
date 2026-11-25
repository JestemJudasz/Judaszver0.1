# Plik: Tools/ue_ludzie.py
# Uruchamiany WEWNATRZ Unreala (commandlet): importuje czesci ludzikow z Art/Ludzie/*.glb (Tools/ludzie.py)
# do /Game/Swiat/Ludzie/SM_Ludz_* (bez Nanite: male, ruchome czesci). Material M_Ludzie:
#   kolor wierzcholkow; alfa = maska: 1 -> tunika z Custom Primitive Data 0..2, 0,5 -> wlosy z CPD 3..5, 0 -> kolor z modelu.
# Uruchomienie: Tools\Zbuduj_Ludzi.bat (kasuje caly Content/Swiat/Ludzie).
import glob, os
import unreal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCEL = "/Game/Swiat/Ludzie"
TMP = DOCEL + "/_import"
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
assets = unreal.AssetToolsHelpers.get_asset_tools()


def wyr(mat, klasa, x, y, **wl):
    e = MEL.create_material_expression(mat, klasa, x, y)
    for k, v in wl.items():
        e.set_editor_property(k, v)
    return e


def polacz(a, wy, b, we):
    assert MEL.connect_material_expressions(a, wy, b, we), f"{a} {wy} -> {b} {we}"


def maska(mat, vc, prog, y):
    """saturate((alfa - prog) * 20): 1 powyzej progu."""
    odj = wyr(mat, unreal.MaterialExpressionSubtract, -900, y, const_b=prog)
    polacz(vc, "A", odj, "A")
    mn = wyr(mat, unreal.MaterialExpressionMultiply, -750, y, const_b=20.0)
    polacz(odj, "", mn, "A")
    sat = wyr(mat, unreal.MaterialExpressionSaturate, -600, y)
    polacz(mn, "", sat, "")
    return sat


def material():
    mat = assets.create_asset("M_Ludzie", DOCEL, unreal.Material, unreal.MaterialFactoryNew())
    vc = wyr(mat, unreal.MaterialExpressionVertexColor, -1100, 0)
    tunika = wyr(mat, unreal.MaterialExpressionVectorParameter, -900, -300, parameter_name="Tunika",
                 default_value=unreal.LinearColor(0.25, 0.22, 0.15, 1), use_custom_primitive_data=True, primitive_data_index=0)
    wlosy = wyr(mat, unreal.MaterialExpressionVectorParameter, -900, -150, parameter_name="Wlosy",
                default_value=unreal.LinearColor(0.1, 0.05, 0.02, 1), use_custom_primitive_data=True, primitive_data_index=3)
    m_wl = maska(mat, vc, 0.25, 150)
    m_tu = maska(mat, vc, 0.75, 300)
    l1 = wyr(mat, unreal.MaterialExpressionLinearInterpolate, -400, 0)
    polacz(vc, "", l1, "A")
    polacz(wlosy, "", l1, "B")
    polacz(m_wl, "", l1, "Alpha")
    l2 = wyr(mat, unreal.MaterialExpressionLinearInterpolate, -250, 0)
    polacz(l1, "", l2, "A")
    polacz(tunika, "", l2, "B")
    polacz(m_tu, "", l2, "Alpha")
    jas = wyr(mat, unreal.MaterialExpressionMultiply, -100, 0, const_b=1.8)  # jak budynki: region ma ekspozycje -0,7 EV
    polacz(l2, "", jas, "A")
    MEL.connect_material_property(jas, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = wyr(mat, unreal.MaterialExpressionConstant, -100, 200, r=0.9)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mat.set_editor_property("used_with_nanite", True)  # import moze wlaczyc Nanite na siatkach; bez flagi gra bierze material domyslny
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    return mat


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
    for i in range(len(m.get_editor_property("static_materials"))):
        m.set_material(i, mat)
    assert EAL.rename_asset(m.get_path_name(), DOCEL + "/" + nazwa), f"{nazwa}: nie przeniesiono"
    EAL.save_asset(DOCEL + "/" + nazwa)
    unreal.log_warning(f"Ludzie: {nazwa} gotowy")


def main():
    mat = material()
    pliki = sorted(glob.glob(os.path.join(ROOT, "Art", "Ludzie", "*.glb")))
    assert pliki, "brak Art/Ludzie/*.glb - uruchom Tools/ludzie.py"
    for p in pliki:
        importuj(p, mat)
    EAL.delete_directory(TMP)


main()
