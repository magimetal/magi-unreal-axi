using UnrealBuildTool;

public class MagiUnrealAXI : ModuleRules
{
    public MagiUnrealAXI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core" });
        PrivateDependencyModuleNames.AddRange(new[] { "CoreUObject", "Engine", "Json", "Networking", "Sockets", "UnrealEd", "MainFrame", "AssetRegistry", "Slate", "EnhancedInput", "InputCore", "KismetCompiler", "BlueprintGraph" });
        if (Target.Platform != UnrealTargetPlatform.Mac)
        {
            throw new BuildException("MagiUnrealAXI 0.1.0 supports only Mac UE 5.8.1");
        }
    }
}
