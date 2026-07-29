// Copyright Bifrost-Centurio2026. Licensed under Apache-2.0.

using UnrealBuildTool;

public class CenturioBridge : ModuleRules
{
    public CenturioBridge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Sockets",
            "Networking",
            "Json",
            "JsonUtilities",
            "RenderCore",
            "RHI",
            "ImageWrapper",
        });
    }
}
