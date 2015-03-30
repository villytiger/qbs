// base for D modules

import qbs.FileInfo
import qbs.PathTools

import "dmd.js" as Dmd

Module {
    condition: true

    staticLibraryPrefix: "lib"
    dynamicLibraryPrefix: "lib"
    loadableModulePrefix: "lib"
    executablePrefix: ""
    staticLibrarySuffix: ".a"
    dynamicLibrarySuffix: ".so"
    loadableModuleSuffix: ""
    executableSuffix: ""

    property string toolchainPrefix
    property path toolchainInstallPath

    property string toolchainPathPrefix: {
        var path = ''
        if (toolchainInstallPath) {
            path += toolchainInstallPath
            if (path.substr(-1) !== '/')
                path += '/'
        }
        if (toolchainPrefix)
            path += toolchainPrefix
        return path
    }

    linkerPath: toolchainPathPrefix + 'dmd'

    Rule {
        id: linker
        inputs: ["d"]

        Artifact {
            fileTags: ["application"]
            filePath: {
                return FileInfo.joinPaths(product.destinationDirectory,
                PathTools.applicationFilePath(product))
            }
        }

        prepare: {
            return Dmd.prepareLinker.apply(this, arguments);
        }
    }

    FileTagger {
        patterns: ["*.d"]
        fileTags: ["d"]
    }
}
