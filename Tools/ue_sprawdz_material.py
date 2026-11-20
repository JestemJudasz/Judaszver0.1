# Plik: Tools/ue_sprawdz_material.py
# Kompiluje shadery wskazanych materialow (commandlet bez -nullrhi) - bledy trafiaja do logu jako "Failed to compile".
# Uzycie: UnrealEditor-Cmd.exe grarts2d.uproject -run=pythonscript -script=Tools/ue_sprawdz_material.py
import unreal

for sciezka in ("/Game/Swiat/Teren/M_Teren", "/Game/Swiat/Teren/M_MapaRzeki", "/Game/Swiat/Natura/M_Natura"):
    if unreal.EditorAssetLibrary.does_asset_exist(sciezka):
        mat = unreal.load_asset(sciezka)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        unreal.log_warning("Sprawdzono: " + sciezka)
unreal.SystemLibrary.execute_console_command(None, "r.ShaderCompiler.FlushAll")
