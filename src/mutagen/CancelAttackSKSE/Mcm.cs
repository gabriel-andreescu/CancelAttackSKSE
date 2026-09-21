using BMK.Mutagen.Skyrim;
using Mutagen.Bethesda.Skyrim;

namespace CancelAttackSKSE.Generator;

internal static class Mcm
{
    public static void AddQuest(SkyrimMod mod)
    {
        McmQuest.Add(
            mod,
            new McmQuestOptions
            {
                EditorId = "CancelAttackSKSEMCMQuest",
                DisplayName = "CancelAttackSKSE",
                ConfigScriptName = "CancelAttackSKSEMCM",
                ModName = "CancelAttackSKSE",
            }
        );
    }
}
