#!lua

if not POCO then
	POCO = {}
end

if not POCO.Crypto then
	POCO.Crypto = {}
    
    function POCO.Crypto.AddProject()
        project "POCOCrypto"
        location "dependencies/poco/Crypto/build"
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
            "POCO_NO_AUTOMATIC_LIBS"
        }
            
        externalincludedirs {
            "%{prj.location}/../include",
            "%{prj.location}/../../Foundation/include",
            _MAIN_SCRIPT_DIR .. "/dependencies/OpenSSL/1.1.1k/include"
        }
        
        rtti("On")
        
        filter "platforms:x64"
            externalincludedirs {
                _MAIN_SCRIPT_DIR .. "/dependencies/OpenSSL/1.1.1k/include/platform/win64"
            }
        filter "platforms:Android"
            linkoptions { "-lm" } -- For gcc's math lib
            staticruntime("On")

            externalincludedirs {
                _MAIN_SCRIPT_DIR .. "dependencies/OpenSSL/1.1.1k/include/platform/android"
            }

            links {
                "POCOFoundation",
                "POCOUtil"
            }
        filter "platforms:macosx"
            externalincludedirs {
                _MAIN_SCRIPT_DIR .. "/dependencies/OpenSSL/1.1.1k/include/platform/macos"
            }
        filter "platforms:ios"
            externalincludedirs {
                _MAIN_SCRIPT_DIR .. "/dependencies/OpenSSL/1.1.1k/include/platform/ios"
            }
        filter {}
    end
end
