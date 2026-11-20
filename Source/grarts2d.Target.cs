using UnrealBuildTool;

public class grarts2dTarget : TargetRules
{
	public grarts2dTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("grarts2d");
	}
}
