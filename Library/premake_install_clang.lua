-- Install step for Clang (Linux)
-- Invoked automatically after build, via include in the main premake script

function ConfigInstallForClang()
    
    local topLevelDir = "%{wks.location}/../" --wks.location will be /build here.

    local copyLibCommand = ""
    if _OPTIONS["build-type"] == "shared" then
        copyLibCommand = 'cp %{cfg.buildtarget.abspath} ' .. topLevelDir .. 'install/lib/libCSP.so'
    else
        copyLibCommand = 'cp %{cfg.buildtarget.abspath} ' .. topLevelDir .. 'install/lib/libCSP.a'
    end

    local copyHeadersCommand = 'cp -r ' .. topLevelDir .. 'include/CSP ' .. topLevelDir .. 'install/include'

    postbuildcommands {
        'mkdir -p -v ' .. topLevelDir .. 'install/lib',
        'mkdir -p -v ' ..topLevelDir .. 'install/include',
        copyLibCommand,
        copyHeadersCommand
    }
end
