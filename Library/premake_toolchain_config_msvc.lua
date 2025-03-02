include "dependencies/signalrclient/premake5.lua"
include "dependencies/mimalloc/premake5.lua"
include "dependencies/quickjs/premake5.lua"
include "dependencies/asyncplusplus/premake5.lua"
include "dependencies/tinyspline/premake5.lua"
include "dependencies/poco/Crypto/premake5.lua"
include "dependencies/poco/Foundation/premake5.lua"
include "dependencies/poco/Net/premake5.lua"
include "dependencies/poco/NetSSL_OpenSSL/premake5.lua"
include "dependencies/poco/Util/premake5.lua"

function ConfigBuildForMSVC()
    defines {
          "CSP_DESKTOP",
          "CSP_WINDOWS",
          "JS_STRICT_NAN_BOXING", -- For QuickJS strict NaN boxing behavior, unsure exactly why this is here.
          "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
          "NO_SIGNALRCLIENT_EXPORTS",
          "USE_MSGPACK",
          "POCO_STATIC", -- Not for WASM
          "POCO_NO_AUTOMATIC_LIBS", -- Not for WASM
          "POCO_NO_INOTIFY",
          "POCO_NO_FILECHANNEL",
          "POCO_NO_SPLITTERCHANNEL",
          "POCO_NO_SYSLOGCHANNEL",
          "POCO_UTIL_NO_INIFILECONFIGURATION",
          "POCO_UTIL_NO_JSONCONFIGURATION",
          "POCO_UTIL_NO_XMLCONFIGURATION",
          "LIBASYNC_STATIC"
    }

	disablewarnings {
        "4251",  -- Ignore dll interface warnings
        "4996",  -- Ignore deprecated warnings
        "4200"   -- Ignore nonstandard extension warnings (for quickjspp)
    }

    linkoptions {
        "-IGNORE:4099"  -- Because we don't have debug symbols for OpenSSL libs
    }

    links {
        "WS2_32", -- Windows socket library, I guess neccesary for poco?
        "signalrclient",
        "quickjs",
        "asyncplusplus",
        "mimalloc",
        "tinyspline",
        "POCONetSSL_OpenSSL"
    }

    flags {
        "MultiProcessorCompile" --MSVC specific flag to enable multi-cpu (why does MSVC have to be so unique about everything...)
    }

    excludes {
        "**EmscriptenSignalRClient**",
        "**EmscriptenWebClient**"
    }

    externalincludedirs {
        "dependencies/signalrclient/include",
        "dependencies/rapidjson/include",
        "dependencies/msgpack/include",
        "dependencies/quickjs/include",
        "dependencies/glm",
        "dependencies/asyncplusplus/include",
        "dependencies/atomic_queue/include",
        "dependencies/tinyspline/src",
        -- mimalloc is not used in WASM builds
        "dependencies/mimalloc/include",
        -- POCO is not used in WASM builds
        "dependencies/poco/Crypto/include",
        "dependencies/poco/Foundation/include",
        "dependencies/poco/Net/include",
        "dependencies/poco/NetSSL_OpenSSL/include",
        "dependencies/poco/Util/include",
        -- OpenSSL is not used in WASM builds
        "dependencies/OpenSSL/1.1.1k/include",
        "dependencies/OpenSSL/1.1.1k/include/platform/win64"
    }

    systemversion "latest" -- https://premake.github.io/docs/systemversion/ (For selecting windows SDK)

    SignalRClient.AddProject()
    QuickJS.AddProject()
    TinySpline.AddProject()
    AsyncPlusPlus.AddProject()
    MiMalloc.AddProject()
    POCO.Crypto.AddProject()
    POCO.Foundation.AddProject()
    POCO.Net.AddProject()
    POCO.NETSSL_OpenSSL.AddProject()
    POCO.Util.AddProject()
end