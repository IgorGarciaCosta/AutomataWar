using UnrealBuildTool;

public class AutomataWarEditor : ModuleRules
{
    public AutomataWarEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "UMG",
            "UMGEditor",
            "Niagara",
            "AutomataWar"
        });
    }
}