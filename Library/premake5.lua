-- premake5.lua


-- Option to choose library type (static or shared)
newoption {
    trigger     = "build-type",
    description = "What library type to build, static or shared",
    default     = "shared",
     allowed = {
          { "static", "Static Library (.a/.lib)" },
          { "shared", "Shared Library (.so/.dll)" }
       }
}

workspace "ConnectedSpacesPlatformLibrary"
    configurations { "Debug", "Release", "RelWithDebInfo" }
    platforms { "x64" }
    location "build"

-- Output directory based on configuration, system, and architecture
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "CSP"
    location "build"

    print("Main Premake Directory (_MAIN_SCRIPT_DIR): " .. _MAIN_SCRIPT_DIR)

    -- Choose project type, "StaticLib" and "SharedLib" are the premake spellings of these options
    local libraryKinds = {
        static = "StaticLib",
        shared = "SharedLib",
    }
   kind(libraryKinds[_OPTIONS["build-type"]])

    language "C++"
    cppdialect "C++17"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("intermediate/" .. outputdir .. "/%{prj.name}")

    -- Include source and header files
    files { "src/**.h",
            "src/**.cpp",
            "include/**.h",
            "dependencies/csp-services/generated/**.h",
            "dependencies/csp-services/generated/**.cpp"
    }

    -- Include directories for headers
    includedirs { "include", "src", "dependencies/csp-services/generated" }

    -- Configuration-specific settings
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

    filter "configurations:RelWithDebInfo"
        defines { "NDEBUG" }
        optimize "On"
        symbols "On"

    filter {}  -- clear filter

   -- Forward the link-type through as a define for use in the codebase
   if _OPTIONS["link-type"] == "shared" then
       defines { "CSP_BUILD_SHARED" }
   end

    rtti("On")

    if os.istarget("windows") then
        include "premake_toolchain_config_msvc.lua"
        ConfigBuildForMSVC()
        include "premake_install_msvc.lua"
        ConfigInstallForMSVC()
    elseif os.istarget("linux") then
        include "premake_toolchain_config_clang.lua"
        ConfigBuildForClang()
        include "premake_install_clang.lua"
        ConfigInstallForClang()
    end

-- Define a custom clean action, should be run from the root Library dir.
newaction {
    trigger     = "clean",
    description = "Remove all generated files and directories",
    execute     = function ()

        local files_to_remove = {
            "./**.make"     -- Remove all Makefiles
        }

        for _, file in ipairs(files_to_remove) do
            print("Removing file: " .. file)
            os.remove(file)
        end

        local function remove_directory(dir)
            if os.isdir(dir) then
                print("Removing directory: " .. dir)
                os.rmdir(dir)
            else
                print("Skipping (not found): " .. dir)
            end
        end

        -- Explicitly remove the build directories for dependencies
        local dirs_to_remove = {
            "./bin",
            "./intermediate",
            "./build",
            "./install",
            "./dependencies/signalrclient/build",
            "./dependencies/quickjs/build",
            "./dependencies/tinyspline/build",
            "./dependencies/asyncplusplus/build",
            "./dependencies/mimalloc/build",
            "./dependencies/poco/Crypto/build",
            "./dependencies/poco/Foundation/build",
            "./dependencies/poco/Net/build",
            "./dependencies/poco/NetSLL_OpenSSL/build",
            "./dependencies/poco/Util/build"

        }

   
        for _, dir in ipairs(dirs_to_remove) do
            remove_directory(dir)
        end

        print("Clean complete.")
    end
}

-- Define a custom build action, should be run from the root Library dir.
-- Depends on the existance of `\build` dir, which should exist if you have run premake5 before this.
newaction {
    trigger     = "build",
    description = "Navigate to the build directory and run 'make'",
    execute     = function ()
        -- Generate the CPS version
        os.execute("python3 -m pip install -r ../tools/VersionGenerator/requirements.txt")
        os.execute("python3 ../Tools/VersionGenerator/VersionGenerator.py")

        -- Specify your build directory
        local buildDir = "build"

        -- Check if the build directory exists
        if os.isdir(buildDir) then
            -- Change to the build directory
            os.chdir(buildDir)

            if os.host() == "windows" then
                local vsSolution = "ConnectedSpacesPlatformLibrary.sln"
                os.execute("msbuild " .. vsSolution .. " /verbosity:d")
            else
                os.execute("make")
            end
            os.chdir("../");
        else
            print("Build directory does not exist. Have you ran premake5 gmake? (or premake5 vs2022)")
        end
    end
}

newaction {
    trigger     = "rebuild",
    description = "Navigate to the build directory and run 'make', with the rebuild flag",
    execute     = function ()
        -- Specify your build directory
        local buildDir = "build"

        -- Check if the build directory exists
        if os.isdir(buildDir) then
            -- Change to the build directory
            os.chdir(buildDir)

            if os.host() == "windows" then
                local vsSolution = "ConnectedSpacesPlatformLibrary.sln"
                os.execute("msbuild " .. vsSolution .. " /t:Rebuild /verbosity:d")
            else
                os.execute("make -B")
            end
            os.chdir("../");
        else
            print("Build directory does not exist. Have you ran premake5 gmake? (or premake5 vs2022)")
        end
    end
}