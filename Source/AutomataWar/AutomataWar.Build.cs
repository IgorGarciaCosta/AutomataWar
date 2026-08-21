using UnrealBuildTool;

public class AutomataWar : ModuleRules
{
    public AutomataWar(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "NetCore",
            "UMG",
            "Slate",
            "SlateCore",
            "Niagara",
            "ProceduralMeshComponent",
            "OnlineSubsystem",
            "OnlineSubsystemUtils"
        });
    }
}
