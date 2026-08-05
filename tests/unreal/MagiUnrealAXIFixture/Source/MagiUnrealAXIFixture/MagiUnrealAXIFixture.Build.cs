using UnrealBuildTool;

public class MagiUnrealAXIFixture : ModuleRules
{
    public MagiUnrealAXIFixture(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
    }
}
