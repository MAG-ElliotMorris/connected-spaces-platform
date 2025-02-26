function ConfigBuildForClang()

    toolset "clang"

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
        "dependencies/poco/Foundation/include",
        "dependencies/poco/Util/include",
        "dependencies/poco/Net/include",
        "dependencies/poco/Crypto/include",
        "dependencies/poco/NETSSL_OpenSSL/include"
    }
end
