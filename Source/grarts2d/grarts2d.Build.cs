// Plik: Source/grarts2d/grarts2d.Build.cs
using UnrealBuildTool;

public class grarts2d : ModuleRules
{
	public grarts2d(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Pozwala pisac #include "Swiat/SwiatWysokosci.h" (foldery systemow w module).
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Json",
			"ProceduralMeshComponent",
			"Slate",
			"SlateCore"
		});
	}
}
