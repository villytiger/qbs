// NOTE: QBS and Xcode's "target" and "product" names are reversed

function isBundleProduct(product)
{
    return product.type.indexOf("applicationbundle") !== -1
        || product.type.indexOf("frameworkbundle") !== -1
        || product.type.indexOf("bundle") !== -1
        || product.type.indexOf("inapppurchase") !== -1;
}

// CONTENTS_FOLDER_PATH
// Main bundle directory
// the version parameter is only used for framework bundles
function contentsFolderPath(product, version)
{
    var path = wrapperName(product);

    if (product.type.indexOf("frameworkbundle") !== -1)
        path += "/Versions/" + (version || frameworkVersion(product));
    else if (!isShallowBundle(product))
        path += "/Contents";

    return path;
}

// DOCUMENTATION_FOLDER_PATH
// Directory for documentation files
// the version parameter is only used for framework bundles
function documentationFolderPath(product, localizationName, version)
{
    var path = localizedResourcesFolderPath(product, localizationName, version);
    if (product.type.indexOf("inapppurchase") === -1)
        path += "/Documentation";
    return path;
}

// EXECUTABLES_FOLDER_PATH
// Destination directory for auxiliary executables
// the version parameter is only used for framework bundles
function executablesFolderPath(product, localizationName, version)
{
    if (product.type.indexOf("frameworkbundle") !== -1)
        return localizedResourcesFolderPath(product, localizationName, version);
    else
        return _contentsFolderSubDirPath(product, "Executables", version);
}

// EXECUTABLE_FOLDER_PATH
// Destination directory for the primary executable
// the version parameter is only used for framework bundles
function executableFolderPath(product, version)
{
    var path = contentsFolderPath(product, version);
    if (!isShallowBundle(product)
        && product.type.indexOf("frameworkbundle") === -1
        && product.type.indexOf("inapppurchase") === -1)
        path += "/MacOS";

    return path;
}

// EXECUTABLE_PATH
// Path to the bundle's primary executable file
// the version parameter is only used for framework bundles
function executablePath(product, version)
{
    return executableFolderPath(product, version) + "/" + productName(product);
}

// FRAMEWORK_VERSION
// Major version number or letter corresponding to the bundle version
function frameworkVersion(product)
{
    if (product.type.indexOf("frameworkbundle") === -1)
        throw "Product type must be a frameworkbundle, was " + product.type;

    var n = parseInt(product.version, 10);
    return isNaN(n) ? 'A' : n;
}

// FRAMEWORKS_FOLDER_PATH
// Directory containing frameworks used by the bundle's executables
// the version parameter is only used for framework bundles
function frameworksFolderPath(product, version)
{
    return _contentsFolderSubDirPath(product, "Frameworks", version);
}

// INFOPLIST_PATH
// Path to the bundle's main information property list
// the version parameter is only used for framework bundles
function infoPlistPath(product, version)
{
    var path;
    if (product.type.indexOf("frameworkbundle") !== -1)
        path = unlocalizedResourcesFolderPath(product, version);
    else if (product.type.indexOf("inapppurchase") !== -1)
        path = wrapperName(product);
    else
        path = contentsFolderPath(product, version);

    return path + "/" + _infoFileNames(product)[0];
}

// INFOSTRINGS_PATH
// Path to the strings file corresponding to the bundle's main information property list
// the version parameter is only used for framework bundles
function infoStringsPath(product, localizationName, version)
{
    return localizedResourcesFolderPath(product, localizationName, version) + "/" + _infoFileNames(product)[1];
}

// LOCALIZED_RESOURCES_FOLDER_PATH
// Path to the bundle's resources directory for the given localization
// If localizationName is undefined, the default localization path will be given
// Note that the default localization path is NOT equivalent to the unlocalized resources path
// the version parameter is only used for framework bundles
function localizedResourcesFolderPath(product, localizationName, version)
{
    // If no localization was given, get the default one from Info.plist or fall back to English
    if (!localizationName) {
        var infoPlist = product.moduleProperty("cpp", "infoPlist");
        if (infoPlist && infoPlist.hasOwnProperty('CFBundleDevelopmentRegion'))
            localizationName = infoPlist["CFBundleDevelopmentRegion"];
        else
            localizationName = "English";
    }

    return unlocalizedResourcesFolderPath(product, version) + "/" + localizationName + ".lproj";
}

// PKGINFO_PATH
// Path to the bundle's PkgInfo file
function pkgInfoPath(product)
{
    var path = (product.type.indexOf("frameworkbundle") !== -1)
        ? wrapperName(product)
        : contentsFolderPath(product);
    return path + "/PkgInfo";
}

// PLUGINS_FOLDER_PATH
// Directory containing plugins used by the bundle's executables
// the version parameter is only used for framework bundles
function pluginsFolderPath(product, version)
{
    if (product.type.indexOf("frameworkbundle") !== -1)
        return unlocalizedResourcesFolderPath(product, version);

    return _contentsFolderSubDirPath(product, "PlugIns", version);
}

// PRIVATE_HEADERS_FOLDER_PATH
// Directory containing private header files for the framework
// the version parameter is only used for framework bundles
function privateHeadersFolderPath(product, version)
{
    return _contentsFolderSubDirPath(product, "PrivateHeaders", version);
}

// PRODUCT_NAME
// The name of the product (in Xcode terms) which corresponds to the target name in QBS terms
function productName(product)
{
    return product.targetName;
}

// PUBLIC_HEADERS_FOLDER_PATH
// Directory containing public header files for the framework
// the version parameter is only used for framework bundles
function publicHeadersFolderPath(product, version)
{
    return _contentsFolderSubDirPath(product, "Headers", version);
}

// SCRIPTS_FOLDER_PATH
// Directory containing script files associated with the bundle
// the version parameter is only used for framework bundles
function scriptsFolderPath(product, version)
{
    return unlocalizedResourcesFolderPath(product, version) + "/Scripts";
}

// SHALLOW_BUNDLE
// Controls the presence or absence of the Contents, MacOS and Resources folders
// iOS tends to store the majority of files in its bundles in the main directory
function isShallowBundle(product)
{
    return product.moduleProperty("qbs", "targetOS") === "ios"
        && product.type.indexOf("applicationbundle") !== -1;
}

// SHARED_FRAMEWORKS_FOLDER_PATH
// Directory containing sub-frameworks that may be shared with other applications
// the version parameter is only used for framework bundles
function sharedFrameworksFolderPath(product, version)
{
    return _contentsFolderSubDirPath(product, "SharedFrameworks", version);
}

// SHARED_SUPPORT_FOLDER_PATH
// Directory containing supporting files that may be shared with other applications
// the version parameter is only used for framework bundles
function sharedSupportFolderPath(product, version)
{
    if (product.type.indexOf("frameworkbundle") !== -1)
        return unlocalizedResourcesFolderPath(product, version);

    return _contentsFolderSubDirPath(product, "SharedSupport", version);
}

// UNLOCALIZED_RESOURCES_FOLDER_PATH
// Directory containing resource files that are not specific to any given localization
function unlocalizedResourcesFolderPath(product, version)
{
    if (isShallowBundle(product))
        return contentsFolderPath(product, version);

    return _contentsFolderSubDirPath(product, "Resources", version);
}

// VERSIONPLIST_PATH
// Directory containing the bundle's version.plist file
// the version parameter is only used for framework bundles
function versionPlistPath(product, version)
{
    var path = (product.type.indexOf("frameworkbundle") !== -1)
        ? unlocalizedResourcesFolderPath(product, version)
        : contentsFolderPath(product, version);
    return path + "/version.plist";
}

// WRAPPER_EXTENSION
// The file extension of the bundle directory - app, framework, bundle, etc.
function wrapperExtension(product)
{
    if (product.type.indexOf("applicationbundle") !== -1) {
        return "app";
    } else if (product.type.indexOf("frameworkbundle") !== -1) {
        return "framework";
    } else if (product.type.indexOf("inapppurchase") !== -1) {
        return "";
    } else if (product.type.indexOf("bundle") !== -1) {
        // Potentially: kext, prefPane, qlgenerator, saver, mdimporter, or a custom extension
        var bundleExtension = ModUtils.moduleProperty(product, "bundleExtension");

        // default to bundle if none was specified by the user
        return bundleExtension || "bundle";
    } else {
        throw ("Unsupported bundle product type " + product.type + ". "
             + "Must be in {applicationbundle, frameworkbundle, bundle, inapppurchase}.");
    }
}

// WRAPPER_NAME
// The name of the bundle directory - the product name plus the bundle extension
function wrapperName(product)
{
    return productName(product) + wrapperSuffix(product);
}

// WRAPPER_SUFFIX
// The suffix of the bundle directory, that is, its extension prefixed with a '.',
// or an empty string if the extension is also an empty string
function wrapperSuffix(product)
{
    var ext = wrapperExtension(product);
    return ext ? ("." + ext) : "";
}

// Private helper functions

// In-App purchase content bundles use virtually no subfolders of Contents;
// this is a convenience method to avoid repeating that logic everywhere
// the version parameter is only used for framework bundles
function _contentsFolderSubDirPath(product, subdirectoryName, version)
{
    var path = contentsFolderPath(product, version);
    if (product.type.indexOf("inapppurchase") === -1)
        path += "/" + subdirectoryName;
    return path;
}

// Returns a list containing the filename of the bundle's main information
// property list and filename of the corresponding strings file
function _infoFileNames(product)
{
    if (product.type.indexOf("inapppurchase") !== -1)
        return ["ContentInfo.plist", "ContentInfo.strings"];
    else
        return ["Info.plist", "InfoPlist.strings"];
}