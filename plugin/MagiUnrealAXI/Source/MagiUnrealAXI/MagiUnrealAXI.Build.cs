using UnrealBuildTool;

public class MagiUnrealAXI : ModuleRules
{
    public MagiUnrealAXI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core" });
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "Json", "Networking", "Sockets", "UnrealEd", "MainFrame", "AssetRegistry", "Slate", "EnhancedInput", "InputCore", "ApplicationCore", "KismetCompiler", "BlueprintGraph", "UMG", "UMGEditor", "AIModule", "NavigationSystem", "GameplayTasks", "AIGraph", "BehaviorTreeEditor" });
        PrivateIncludePaths.Add(System.IO.Path.Combine(EngineDirectory, "Source/Editor/UMGEditor/Private"));
        if (Target.Platform != UnrealTargetPlatform.Mac)
        {
            throw new BuildException("MagiUnrealAXI supports only Mac UE 5.8.1");
        }
    }
}
