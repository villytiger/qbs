import qbs

Project {
    WindowsInstallerPackage {
        name: "QbsSetup"
        targetName: "qbs-" + qbs.architecture
        files: ["QbsSetup.wxs", "ExampleScript.bat"]
        wix.defines: ["scriptName=ExampleScript.bat"]
    }

    BurnSetupPackage {
        Depends { name: "QbsSetup" }
        name: "QbsBootstrapper"
        targetName: "qbs-setup-" + qbs.architecture
        files: ["QbsBootstrapper.wxs"]
        wix.defines: ["msiName=qbs-" + qbs.architecture + ".msi"]
    }
}