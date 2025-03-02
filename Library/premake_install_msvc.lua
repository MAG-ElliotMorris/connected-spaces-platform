-- Install step for MSVC (Windows)
-- Invoked automatically after build, via include in the main Premake script

function ConfigInstallForMSVC()
    
    local topLevelDir = "%{wks.location}/../" -- wks.location will be /build here.

    local copyLibCommand = 'xcopy /Y "%{cfg.buildtarget.abspath}" "' .. topLevelDir .. 'install\\lib\\CSP.lib'
    local copyDLLCommand = 'xcopy /Y "%{cfg.buildtarget.abspath}" "' .. topLevelDir .. 'install\\lib\\CSP.dll'

    local copyHeadersCommand = 'xcopy /E /I /Y "' .. topLevelDir .. 'include\\CSP" "' .. topLevelDir .. 'install\\include"'

    if _OPTIONS["build-type"] == "shared" then
        postbuildcommands {
            'mkdir "' .. topLevelDir .. 'install\\lib"',
            'mkdir "' .. topLevelDir .. 'install\\include"',
            copyDLLCommand,
            copyLibCommand,
            copyHeadersCommand
        }
    else
        postbuildcommands {
            'mkdir "' .. topLevelDir .. 'install\\lib"',
            'mkdir "' .. topLevelDir .. 'install\\include"',
            copyLibCommand,
            copyHeadersCommand
        }
    end
end