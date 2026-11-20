using UnrealBuildTool;

public class grarts2dEditorTarget : TargetRules
{
	public grarts2dEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("grarts2d");
	}
}
