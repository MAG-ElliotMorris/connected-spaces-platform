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

function ConfigBuildForClang()

    toolset "clang"

    buildoptions {
        "-stdlib=libc++"
    }

    linkoptions {
        "-stdlib=libc++",
        "-lc++abi" --Use the libc++ ABI (clang standard library, as opposed to libstdc++ (GCC))
    }

    defines {
          "CSP_DESKTOP",
          "CSP_LINUX",
          "JS_STRICT_NAN_BOXING", -- For QuickJS strict NaN boxing behavior, unsure exactly why this is here.
          "NO_SIGNALRCLIENT_EXPORTS",
          "USE_MSGPACK",
          "POCO_STATIC",  -- Not for WASM
          "POCO_NO_AUTOMATIC_LIBS",  -- Not for WASM
          "POCO_NO_INOTIFY",
          "POCO_NO_FILECHANNEL",
          "POCO_NO_SPLITTERCHANNEL",
          "POCO_NO_SYSLOGCHANNEL",
          "POCO_UTIL_NO_INIFILECONFIGURATION",
          "POCO_UTIL_NO_JSONCONFIGURATION",
          "POCO_UTIL_NO_XMLCONFIGURATION",
          "LIBASYNC_STATIC"
    }

    links {
        "pthread",
        "ssl",
        "crypto"
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
        "dependencies/poco/Util/include"
    }

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
