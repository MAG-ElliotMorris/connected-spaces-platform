#!lua

if not TinySpline then
	TinySpline = {}
end

function TinySpline.AddProject()
    project "tinyspline"
    location "dependencies/tinyspline/build"
    kind "StaticLib"
    language "C++"
    cppdialect "C++11"

    files {
        "%{prj.location}/../**.h",
        "%{prj.location}/../**.c",
		"%{prj.location}/../**.cxx"
    }
    
    -- Source directories for this project
    externalincludedirs { 
        "%{prj.location}/../src"
    }
    
    -- Config for platforms
    filter "platforms:Android"
        staticruntime("On")
        disablewarnings { "unknown-pragmas" }
    filter "platforms:wasm"
        --[[
            We need to explicitly enable threading in the WASM build.
            This used to be done for all projects in the RWD project, but was
            removed to prevent confusion when debugging premake issues.
        ]]--
        buildoptions {
            "-pthread",
            "-fwasm-exceptions"
        }

        linkoptions { 
            "-fwasm-exceptions"
        }
    filter {}
end
