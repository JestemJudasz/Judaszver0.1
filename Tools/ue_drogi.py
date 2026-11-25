# Plik: Tools/ue_drogi.py
# Uruchamiany WEWNATRZ Unreala (commandlet): tekstury z Art/Drogi (Tools/drogi.py) -> /Game/Swiat/Drogi i material M_Droga:
#   BaseColor = T_Droga x kolor wierzcholka x jasnosc; maska = alfa wierzcholka + alfa tekstury (falisty, poszarpany brzeg),
#   dwustronny (wstega drogi z ADrogi). T_Ikona_Znak_Droga - znak przycisku "Drogi" (UI, bez mipmap, ostre piksele).
# Uruchomienie: Tools\Zbuduj_Drogi.bat.
import os
import unreal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DOCEL = "/Game/Swiat/Drogi"
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
assets = unreal.AssetToolsHelpers.get_asset_tools()


def importuj(nazwa):
    t = unreal.AssetImportTask()
    t.filename = os.path.join(ROOT, "Art", "Drogi", nazwa + ".png")
    t.destination_path = DOCEL
    t.automated = True
    t.save = False
    assets.import_asset_tasks([t])
    return unreal.load_asset(DOCEL + "/" + nazwa)


def wyr(mat, klasa, x, y):
    return MEL.create_material_expression(mat, klasa, x, y)


def main():
    tex = importuj("T_Droga")
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    tex.set_editor_property("never_stream", True)
    EAL.save_loaded_asset(tex)

    ik = importuj("T_Ikona_Znak_Droga")
    ik.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    ik.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    ik.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    ik.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    ik.set_editor_property("never_stream", True)
    EAL.save_loaded_asset(ik)

    mat = assets.create_asset("M_Droga", DOCEL, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)
    t = wyr(mat, unreal.MaterialExpressionTextureSample, -900, -200)
    t.set_editor_property("texture", tex)
    vc = wyr(mat, unreal.MaterialExpressionVertexColor, -900, 150)
    m1 = wyr(mat, unreal.MaterialExpressionMultiply, -600, -100)
    assert MEL.connect_material_expressions(t, "RGB", m1, "A")
    assert MEL.connect_material_expressions(vc, "", m1, "B")
    m2 = wyr(mat, unreal.MaterialExpressionMultiply, -400, -100)
    m2.set_editor_property("const_b", 1.7)  # jak budynki: region ma ekspozycje -0,7 EV
    assert MEL.connect_material_expressions(m1, "", m2, "A")
    MEL.connect_material_property(m2, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # Maska: alfa wierzcholka (1 na drodze, 0 na skraju wstegi) + (alfa tekstury - 0,5) x 0,9: falisty, poszarpany brzeg.
    s = wyr(mat, unreal.MaterialExpressionSubtract, -600, 250)
    s.set_editor_property("const_b", 0.5)
    assert MEL.connect_material_expressions(t, "A", s, "A")
    m3 = wyr(mat, unreal.MaterialExpressionMultiply, -450, 250)
    m3.set_editor_property("const_b", 0.9)
    assert MEL.connect_material_expressions(s, "", m3, "A")
    a = wyr(mat, unreal.MaterialExpressionAdd, -300, 250)
    assert MEL.connect_material_expressions(vc, "A", a, "A")
    assert MEL.connect_material_expressions(m3, "", a, "B")
    MEL.connect_material_property(a, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    for wartosc, wl, y in ((0.95, unreal.MaterialProperty.MP_ROUGHNESS, 450), (0.2, unreal.MaterialProperty.MP_SPECULAR, 550)):
        c = wyr(mat, unreal.MaterialExpressionConstant, -200, y)
        c.set_editor_property("r", wartosc)
        MEL.connect_material_property(c, "", wl)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    unreal.log_warning("Drogi: M_Droga gotowy")


main()
