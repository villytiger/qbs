// base for Cpp modules

//import qbs 1.0
import qbs.linkable as LinkableModule

Module {
    //Depends { name: "linkable" }
    property string toolchainPathPrefix: (new LinkableModule()).toolchainPathPrefix

    condition: false
    property int compilerVersionMajor
    property int compilerVersionMinor
    property int compilerVersionPatch
    property string warningLevel : 'all' // 'none', 'all'
    property bool treatWarningsAsErrors : false
    property string architecture: qbs.architecture
    property string optimization: qbs.optimization
    property bool debugInformation: qbs.debugInformation
    property bool separateDebugInformation: false
    property pathList prefixHeaders
    property path precompiledHeader
    property path cPrecompiledHeader: precompiledHeader
    property path cxxPrecompiledHeader: precompiledHeader
    // ### default to undefined on non-Apple platforms for now - QBS-346
    property path objcPrecompiledHeader: qbs.targetOS.contains("darwin") ? precompiledHeader : undefined
    property path objcxxPrecompiledHeader: qbs.targetOS.contains("darwin") ? precompiledHeader : undefined
    property path precompiledHeaderDir: product.buildDirectory
    property stringList defines
    property stringList platformDefines: qbs.enableDebugCode ? [] : ["NDEBUG"]
    property stringList compilerDefines
    PropertyOptions {
        name: "compilerDefines"
        description: "preprocessor macros that are defined when using this particular compiler"
    }

    property string windowsApiCharacterSet

    property pathList includePaths
    property pathList systemIncludePaths
    property string compilerName
    property string compilerPath: compilerName
    property var compilerPathByLanguage
    property stringList compilerWrapper

    property stringList cppFlags
    PropertyOptions {
        name: "cppFlags"
        description: "additional flags for the C preprocessor"
    }

    property stringList cFlags
    PropertyOptions {
        name: "cFlags"
        description: "additional flags for the C compiler"
    }

    property stringList cxxFlags
    PropertyOptions {
        name: "cxxFlags"
        description: "additional flags for the C++ compiler"
    }

    property stringList objcFlags
    PropertyOptions {
        name: "objcFlags"
        description: "additional flags for the Objective-C compiler"
    }

    property stringList objcxxFlags
    PropertyOptions {
        name: "objcxxFlags"
        description: "additional flags for the Objective-C++ compiler"
    }
    property stringList commonCompilerFlags
    PropertyOptions {
        name: "commonCompilerFlags"
        description: "flags added to all compilation independently of the language"
    }

    property string cLanguageVersion
    PropertyOptions {
        name: "cLanguageVersion"
        allowedValues: ["c89", "c99", "c11"]
        description: "The version of the C standard with which the code must comply."
    }

    property string cxxLanguageVersion
    PropertyOptions {
        name: "cxxLanguageVersion"
        allowedValues: ["c++98", "c++11", "c++14"]
        description: "The version of the C++ standard with which the code must comply."
    }

    property string cxxStandardLibrary
    PropertyOptions {
        name: "cxxStandardLibrary"
        allowedValues: ["libstdc++", "libc++"]
        description: "version of the C++ standard library to use"
    }

    // Platform properties. Those are intended to be set by the toolchain setup
    // and are prepended to the corresponding user properties.
    property stringList platformCommonCompilerFlags
    property stringList platformCFlags
    property stringList platformCxxFlags
    property stringList platformObjcFlags
    property stringList platformObjcxxFlags
    property stringList platformLinkerFlags

    property bool automaticReferenceCounting
    PropertyOptions {
        name: "automaticReferenceCounting"
        description: "whether to enable Automatic Reference Counting (ARC) for Objective-C"
    }

    FileTagger {
        patterns: ["*.c"]
        fileTags: ["c"]
    }

    FileTagger {
        patterns: ["*.C", "*.cpp", "*.cxx", "*.c++", "*.cc"]
        fileTags: ["cpp"]
    }

    FileTagger {
        patterns: ["*.m"]
        fileTags: ["objc"]
    }

    FileTagger {
        patterns: ["*.mm"]
        fileTags: ["objcpp"]
    }

    FileTagger {
        patterns: ["*.h", "*.H", "*.hpp", "*.hxx", "*.h++"]
        fileTags: ["hpp"]
    }
}
