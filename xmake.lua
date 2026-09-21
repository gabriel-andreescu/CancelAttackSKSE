set_xmakever("3.1.1")
set_project("CancelAttackSKSE")
set_license("GPL-3.0")
set_policy("package.requires_lock", true)

local version = "2.0.0"

add_repositories("bmk https://github.com/gabriel-andreescu/BethesdaModKit.git")
add_addons("bmk 0.3.1")
includes("@addon/bmk/project")
includes("@addon/bmk/native")

-- Dependencies
add_requires("commonlibsse-ng 8.0.1", { system = false })
add_requires("clib-util 1.5.0", { system = false })
add_requires("bmk", { system = false })

add_requires("caprica", { host = true })
add_requires("skyrim-papyrus-sdk", { configs = { mcm = true } })
option("papyrus_imports", { description = "Papyrus import directories separated by ;" })
option("papyrus_flags", { description = "Optional Papyrus flags file override" })

-- Build targets

target("Native", function()
    set_default(false)
    set_basename("CancelAttackSKSE")
    set_version(version)
    add_rules("@commonlibsse-ng/plugin", {
        author = "GabonZ",
        description = "Cancel melee attacks and transition into a block with one press.",
    })
    add_rules("@addon/bmk/skyrim.plugin")
    add_files("$(projectdir)/src/native/**.cpp")
    add_includedirs("$(projectdir)/src/native")
    set_pcxxheader("$(projectdir)/src/native/PCH.h")
    add_packages("commonlibsse-ng", "clib-util", "bmk")
end)

target("Mutagen", function()
    set_default(false)
    add_extrafiles("src/mutagen/CancelAttackSKSE/FormIDs.txt")
    add_rules("@addon/bmk/dotnet", {
        project = "src/mutagen/CancelAttackSKSE/CancelAttackSKSE.csproj",
        arguments = { "$(outputdir)", path.absolute("src/mutagen/CancelAttackSKSE/FormIDs.txt") },
    })
end)

target("MCMScripts", function()
    set_default(false)
    add_rules("@addon/bmk/skyrim.papyrus", {
        root = "src/papyrus/mcm",
        imports = (get_config("papyrus_imports") or ""):split(";", { plain = true }),
        flags = get_config("papyrus_flags"),
        arguments = { "--strict", "--enable-language-extensions=true" },
    })
    add_packages("caprica", "skyrim-papyrus-sdk")
    add_files("$(projectdir)/src/papyrus/mcm/**.psc")
    add_installfiles("$(projectdir)/src/papyrus/mcm/(**.psc)", { prefixdir = "Source/Scripts" })
end)

-- Packages
target("CancelAttackSKSE", function()
    set_version(version)
    add_rules("@addon/bmk/skyrim.package", {
        targets = {
            "Native",
        },
        nexus = {
            mod_id = "7318624424498",
            file_id = "3236926",
            category = "main",
            primary = true,
            description = "Updating from an older version? Follow the Updating to 2.0.0 instructions in the mod description before installing.",
        },
    })
    add_installfiles("$(projectdir)/assets/(**)|optional/**")
end)

target("CancelAttackSKSEMCM", function()
    set_version(version)
    add_deps("Mutagen", { inherit = false })
    add_rules("@addon/bmk/skyrim.package", {
        targets = { "MCMScripts" },
        package_name = "Cancel Attack SKSE - MCM Addon",
        nexus = {
            mod_id = "7318624424498",
            file_id = "8000517",
            category = "optional",
            description = "MCM addon for Cancel Attack SKSE 1.0.0. Requires SkyUI and MCM Helper.",
        },
    })
    add_installfiles("$(builddir)/artifacts/Mutagen/mcm/(CancelAttackSKSE.esp)")
    add_installfiles("$(projectdir)/assets/optional/mcm/(**)")
end)
