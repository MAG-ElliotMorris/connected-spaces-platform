#!lua

if not POCO then
	POCO = {}
end

if not POCO.Foundation then
	POCO.Foundation = {}
    
    -- NOTE! Poco adds a dependency on UTF8Proc in poco 1.14.0, which despite best efforts, doesn't play nice.
    -- For that reason, this is poco 1.13.0

    -- NOTE! There are several areas of poco that flat out don't compile on certain platforms due to some
    -- seemingly missing #ifdef guards. One must assume that it does compile if one were to run it through
    -- the quite complex custom cmake setup they've got going on, but in this premake world, it didn't happen.
    -- Therefore, the source has been slightly modified from the original, in like 3 or 4 places, to add
    -- ifdef guards around some functions.

    -- NOTE! Poco currently requires mc.exe (Windows message compiler) to fully build, due to needing to transform
    -- `pocomsg.mc` to `pocomsg.h`. We have just commited the generated header directly to the vendored version
    -- of poco, which is unfortunate.

    function POCO.Foundation.AddProject()
        project "POCOFoundation"
        location "dependencies/poco/Foundation/build"
        kind "StaticLib"
        language "C++"
        cppdialect "C++17"
		warnings "Off"

        files {
            "%{prj.location}/../**.h",
            "%{prj.location}/../**.cpp",
            "%{prj.location}/../**.c"
        }
        
        defines {
            "POCO_STATIC",
            "POCO_NO_AUTOMATIC_LIBS",
            "POCO_NO_INOTIFY",
            "POCO_NO_FILECHANNEL",
            "POCO_NO_SPLITTERCHANNEL",
            "POCO_NO_SYSLOGCHANNEL",
            "POCO_UTIL_NO_INIFILECONFIGURATION",
            "POCO_UTIL_NO_JSONCONFIGURATION",
            "POCO_UTIL_NO_XMLCONFIGURATION"
        }

        characterset ("ASCII")

        -- PCRE does not allow these files to be compiled directly.
        removefiles {
            "**/pcre2_jit_match.c",
            "**/pcre2_jit_misc.c"
        }
        
        externalincludedirs {
            "%{prj.location}/../include"
        }
        
        excludes { 
            "**_POSIX**", 
            "**_UNIX**",
            "**_WINCE**",
            "**_C99**",
            "**_DEC**",
            "**_SUN**",
            "**_DUMMY**",
            "**_HPUX**",
            "**_STD**",
            "**_QNX**",
            "**_VX**",
            "**_WIN32**",
            "**IOS**",
            "**Android**",
        }
        
        rtti("On")

        filter "platforms:x64"
            excludes { 
                "**SysLog**", -- UNIX-specific logging
            }
            links {
                "iphlpapi",
            }
        filter { "platforms:Android or macosx or ios" }
            excludes { 
                "**Windows**",
                "**EventLog**", -- Windows-specific logging
            }
            -- This cancels the exclusion of encoding source files with 'Windows' in the filename that we do want to compile on other platforms
            files { 
                "%{prj.location}/**Encoding**"
            }
            
            defines {
                "POCO_NO_WINDOWS_H",
                "POCO_NO_FPENVIRONMENT",
                "POCO_NO_WSTRING",
                "POCO_NO_SHAREDMEMORY"
            }
        filter "platforms:ios"
            defines { "POCO_NO_STAT64" }
        filter "platforms:Android"
            linkoptions { "-lm" } -- For gcc's math lib
            staticruntime("On")
        filter {}
    end
end