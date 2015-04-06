import qbs 1.0
import qbs.FileInfo
import qbs.ModUtils

Module {
    property string buildVariant: "debug"
    property bool enableDebugCode: buildVariant == "debug"
    property bool debugInformation: (buildVariant == "debug")
    property string optimization: (buildVariant == "debug" ? "none" : "fast")
    readonly property stringList hostOS: undefined // set internally
    property string hostOSVersion: {
        if (hostOS && hostOS.contains("osx")) {
            return getNativeSetting("/System/Library/CoreServices/ServerVersion.plist", "ProductVersion") ||
                   getNativeSetting("/System/Library/CoreServices/SystemVersion.plist", "ProductVersion");
        } else if (hostOS && hostOS.contains("windows")) {
            var version = getNativeSetting("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion", "CurrentVersion");
            return version + "." + hostOSBuildVersion;
        }
    }

    property string hostOSBuildVersion: {
        if (hostOS.contains("osx")) {
            return getNativeSetting("/System/Library/CoreServices/ServerVersion.plist", "ProductBuildVersion") ||
                   getNativeSetting("/System/Library/CoreServices/SystemVersion.plist", "ProductBuildVersion");
        } else if (hostOS.contains("windows")) {
            return getNativeSetting("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows NT\\CurrentVersion", "CurrentBuildNumber");
        }
    }

    readonly property var hostOSVersionParts: hostOSVersion ? hostOSVersion.split('.').map(function(item) { return parseInt(item, 10); }) : []
    readonly property int hostOSVersionMajor: hostOSVersionParts[0] || 0
    readonly property int hostOSVersionMinor: hostOSVersionParts[1] || 0
    readonly property int hostOSVersionPatch: hostOSVersionParts[2] || 0

    property stringList targetOS: hostOS
    property string pathListSeparator: hostOS.contains("windows") ? ";" : ":"
    property string pathSeparator: hostOS.contains("windows") ? "\\" : "/"
    property string profile
    property stringList toolchain
    property stringList architecture
    property bool install: false
    property string installSourceBase
    readonly property string installRoot: undefined
    property string installDir
    property string installPrefix: ""
    property path sysroot

    PropertyOptions {
        name: "buildVariant"
        allowedValues: ['debug', 'release']
        description: "name of the build variant"
    }

    PropertyOptions {
        name: "optimization"
        allowedValues: ['none', 'fast', 'small']
        description: "optimization level"
    }

    validate: {
        var validator = new ModUtils.PropertyValidator("qbs");
        validator.setRequiredProperty("architecture", architecture,
                                      "you might want to re-run 'qbs-setup-toolchains'");
        validator.setRequiredProperty("hostOS", hostOS);
        validator.setRequiredProperty("targetOS", targetOS);
        if (hostOS && (hostOS.contains("windows") || hostOS.contains("osx"))) {
            validator.setRequiredProperty("hostOSVersion", hostOSVersion,
                                          "could not detect host operating system version; " +
                                          "verify that system files and registry keys have not " +
                                          "been modified.");
            if (hostOSVersion)
                validator.addVersionValidator("hostOSVersion", hostOSVersion, 2, 4);

            validator.setRequiredProperty("hostOSBuildVersion", hostOSBuildVersion,
                                          "could not detect host operating system build version; " +
                                          "verify that system files or registry have not been " +
                                          "tampered with.");
        }

        validator.addCustomValidator("architecture", architecture, function (value) {
            for (var i in architecture) {
                if (architecture[i] !== canonicalArchitecture(architecture[i]))
                    return false;
            }
            return true;
        }, "'" + architecture + "' is invalid. You must use canonical names '" +
        architecture.map(function (a) {
            return canonicalArchitecture(a);
        }) + "'");

        validator.addCustomValidator("architecture", architecture, function (value) {
            return architecture.length == 1 || (architecture > 1 && qbs.targetOS.contains("darwin"));
        }, "you must specify at least one architecture; " +
        "only iOS and OSX support multiple architectures; " +
        "you might want to re-run 'qbs-setup-toolchains'");

        validator.validate();
    }

    // private properties
    property var commonRunEnvironment: {
        var env = {};
        if (hostOS.contains("darwin") && targetOS.contains("darwin")) {
            env["DYLD_FRAMEWORK_PATH"] = [
                FileInfo.joinPaths(installRoot, installPrefix, "Library", "Frameworks"),
                FileInfo.joinPaths(installRoot, installPrefix, "lib"),
                FileInfo.joinPaths(installRoot, installPrefix)
            ].join(pathListSeparator);

            env["DYLD_LIBRARY_PATH"] = [
                FileInfo.joinPaths(installRoot, installPrefix, "lib"),
                FileInfo.joinPaths(installRoot, installPrefix, "Library", "Frameworks"),
                FileInfo.joinPaths(installRoot, installPrefix)
            ].join(pathListSeparator);

            if (sysroot) {
                env["DYLD_ROOT_PATH"] = [sysroot];
            }
        }

        return env;
    }
}
