import qbs 1.0
import qbs.DarwinTools
import qbs.File
import qbs.ModUtils

DarwinGCC {
    condition: qbs.hostOS.contains('osx') && qbs.targetOS.contains('ios') && qbs.toolchain.contains('gcc')

    // Setting a minimum is especially important for Simulator or CC/LD thinks the target is OS X
    minimumIosVersion: xcodeSdkVersion || (cxxStandardLibrary === "libc++" ? "5.0" : undefined)

    platformObjcFlags: base.concat(simulatorObjcFlags)
    platformObjcxxFlags: base.concat(simulatorObjcFlags)

    // Private properties
    readonly property stringList simulatorObjcFlags: {
        // default in Xcode and also required for building 32-bit Simulator binaries with ARC
        // since the default ABI version is 0 for 32-bit targets
        return qbs.targetOS.contains("ios-simulator")
                ? ["-fobjc-abi-version=2", "-fobjc-legacy-dispatch"]
                : [];
    }

    Rule {
        condition: !product.moduleProperty("qbs", "targetOS").contains("ios-simulator")
        multiplex: true
        inputs: ["qbs"]

        Artifact {
            filePath: product.destinationDirectory + "/"
                    + product.moduleProperty("bundle", "contentsFolderPath")
                    + "/ResourceRules.plist"
            fileTags: ["resourcerules"]
        }

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "generating ResourceRules";
            cmd.highlight = "codegen";
            cmd.sysroot = product.moduleProperty("qbs","sysroot");
            cmd.sourceCode = function() {
                File.copy(sysroot + "/ResourceRules.plist", outputs.resourcerules[0].filePath);
            }
            return cmd;
        }
    }

    Rule {
        condition: product.moduleProperty("cpp", "buildIpa")
        multiplex: true
        inputs: ["application", "infoplist", "pkginfo", "resourcerules", "compiled_nib"]

        Artifact {
            filePath: product.destinationDirectory + "/" + product.targetName + ".ipa"
            fileTags: ["ipa"]
        }

        prepare: {
            var signingIdentity = product.moduleProperty("cpp", "signingIdentity");
            if (!signingIdentity)
                throw "The name of a valid signing identity must be set using " +
                        "cpp.signingIdentity in order to build an IPA package.";

            var provisioningProfile = product.moduleProperty("cpp", "provisioningProfile");
            if (!provisioningProfile)
                throw "The path to a provisioning profile must be set using " +
                        "cpp.provisioningProfile in order to build an IPA package.";

            var args = ["-sdk", product.moduleProperty("cpp", "xcodeSdkName"), "PackageApplication",
                        "-v", product.buildDirectory + "/" + product.moduleProperty("bundle", "bundleName"),
                        "-o", outputs.ipa[0].filePath, "--sign", signingIdentity,
                        "--embed", provisioningProfile];

            var command = "/usr/bin/xcrun";
            var cmd = new Command(command, args)
            cmd.description = "creating ipa, signing with " + signingIdentity;
            cmd.highlight = "codegen";
            cmd.workingDirectory = product.buildDirectory;
            return cmd;
        }
    }
}
