# Plik: Tools/ue_dzwieki.py
# Uruchamiany WEWNATRZ Unreala (commandlet): Art/Dzwieki/<folder>/*.wav -> /Game/Dzwieki/<folder>/<nazwa>.
# Nazwy: <Grupa>_<n>.wav (np. Miecz_Tarcza_03), gra losuje wariant z grupy (UDzwieki). Pliki Petla_* graja w petli.
# Kazdy plik musi byc wpisany do Docs/Assety_zrodla.md (zrodlo i licencja).
# Uruchomienie: Tools\Zbuduj_Dzwieki.bat.
import os
import unreal

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ZRODLO = os.path.join(ROOT, "Art", "Dzwieki")
DOCEL = "/Game/Dzwieki"
EAL = unreal.EditorAssetLibrary
assets = unreal.AssetToolsHelpers.get_asset_tools()


def main():
    ile = 0
    for folder, _, pliki in os.walk(ZRODLO):
        rel = os.path.relpath(folder, ZRODLO).replace("\\", "/")
        cel = DOCEL if rel == "." else DOCEL + "/" + rel
        for p in sorted(pliki):
            if not p.lower().endswith(".wav"):
                continue
            t = unreal.AssetImportTask()
            t.filename = os.path.join(folder, p)
            t.destination_path = cel
            t.automated = True
            t.replace_existing = True
            t.save = False
            assets.import_asset_tasks([t])
            nazwa = os.path.splitext(p)[0]
            s = unreal.load_asset(cel + "/" + nazwa)
            if s is None:
                unreal.log_error("Dzwieki: nie wczytal " + t.filename)
                continue
            if nazwa.startswith("Petla_"):
                s.set_editor_property("looping", True)
            else:
                # Krotkie uderzenia bez kompresji: domyslna kompresja psula ostre poczatki (trzaski na sluchawkach).
                s.set_editor_property("sound_asset_compression_type", unreal.SoundAssetCompressionType.PCM)
            EAL.save_loaded_asset(s)
            ile += 1
    unreal.log("Dzwieki: zaimportowano %d plikow" % ile)


main()
