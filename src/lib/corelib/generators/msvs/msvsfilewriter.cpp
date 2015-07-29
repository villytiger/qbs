/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Build Suite.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "msvsfilewriter.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QUuid>
#include <QXmlStreamWriter>

#include <logging/translator.h>
#include <tools/shellutils.h>

using namespace qbs;

// Project file GUID (Do NOT change!)
static const QString _GUIDProjectFolder = QLatin1String("{2150E333-8FDC-42A3-9474-1A3956D46DE8}");

// Project file format specific options (Do NOT change!)
static const QString _VcprojNmakeConfig = QLatin1String("0");
static const QString _VcprojListsSeparator = QLatin1String(";");

MsvsFileWriter::MsvsFileWriter(const VersionOptions &versionOptions,
                               const QString &qbsExecutablePath,
                               const QString &qbsProjectFilePath,
                               const QString &qbsBuildDirectory,
                               const QStringList &projectCommandlineParameters,
                               const QString &installRoot)
    : m_options(versionOptions)
    , m_qbsExecutablePath(qbsExecutablePath)
    , m_qbsProjectFilePath(qbsProjectFilePath)
    , m_qbsBuildDirectory(qbsBuildDirectory)
    , m_projectCommandlineParameters(projectCommandlineParameters)
    , m_installRoot(installRoot)
{
    // TODO: retrieve tags from groupData.
    m_filterOptions << FilterOptions(QLatin1String("c;C;cpp;cxx;c++;cc;def;m;mm"), QLatin1String("Source Files"));
    m_filterOptions << FilterOptions(QLatin1String("h;H;hpp;hxx;h++"), QLatin1String("Header Files"));
    m_filterOptions << FilterOptions(QLatin1String("ui"), QLatin1String("Form Files"));
    m_filterOptions << FilterOptions(QLatin1String("qrc;rc;*"), QLatin1String("Resource Files"), QLatin1String("ParseFiles"));
    m_filterOptions << FilterOptions(QLatin1String("moc"), QLatin1String("Generated Files"), QLatin1String("SourceControlFiles"));
    m_filterOptions << FilterOptions(QLatin1String("ts"), QLatin1String("Translation Files"), QLatin1String("ParseFiles"));
}

bool MsvsFileWriter::generateSolution(const QString &fileName,
                                      const MsvsPreparedProject &project) const
{
    QFile solutionFile(fileName);
    if (!solutionFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    const QString solutionGuid = QUuid::createUuid().toString();

    QTextStream solutionOutStream(&solutionFile);
    solutionOutStream << m_options.solutionHeader;

    foreach (QSharedPointer<MsvsPreparedProduct> product, project.allProducts()) {
        solutionOutStream << QString::fromLocal8Bit("Project(\"%1\") = \"%2\", \"%3\", \"%4\"\n")
                             .arg(solutionGuid)
                             .arg(product->name)
                             .arg(product->vcprojFilepath)
                             .arg(product->guid);
        solutionOutStream << "EndProject\n";
    }

    writeProjectSubFolders(solutionOutStream, project);

    solutionOutStream << "Global\n";

    solutionOutStream << "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n";
    foreach (const MsvsProjectConfiguration &buildTask, project.enabledConfigurations) {
        solutionOutStream << QString::fromLocal8Bit("\t\t%1 = %1\n").arg(buildTask.fullName());
    }

    solutionOutStream << "\tEndGlobalSection\n";

    solutionOutStream << "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n";
    foreach (QSharedPointer<MsvsPreparedProduct> product, project.allProducts()) {
        foreach (const MsvsProjectConfiguration &buildTask, product->configurations.keys()) {
            solutionOutStream << QString::fromLocal8Bit("\t\t%1.%2.ActiveCfg = %2\n"
                                         "\t\t%1.%2.Build.0 = %2\n")
                                         .arg(product->guid)
                                         .arg(buildTask.fullName());
        }
    }

    solutionOutStream << "\tEndGlobalSection\n";
    solutionOutStream << "\tGlobalSection(SolutionProperties) = preSolution\n";
    solutionOutStream << "\t\tHideSolutionNode = FALSE\n";
    solutionOutStream << "\tEndGlobalSection\n";

    solutionOutStream << "\tGlobalSection(NestedProjects) = preSolution\n";
    writeNestedProjects(solutionOutStream, project);
    solutionOutStream << "\tEndGlobalSection\n";
    solutionOutStream << "EndGlobal\n";

    return solutionOutStream.status() == QTextStream::Ok && solutionFile.flush();
}

void MsvsFileWriter::writeMsBuildFiles(QXmlStreamWriter &xmlWriter,
                                       const QSet<MsvsProjectConfiguration>& allConfigurations,
                                       const ProjectConfigurations &allProjectFilesConfigurations) const
{
    xmlWriter.writeStartElement(QLatin1String("ItemGroup"));

    foreach (const QString &filePath, allProjectFilesConfigurations.keys()) {
        xmlWriter.writeStartElement(QLatin1String("ClCompile"));
        QSet<MsvsProjectConfiguration> disabledConfigurations = allConfigurations - allProjectFilesConfigurations[filePath];
        xmlWriter.writeAttribute(QLatin1String("Include"), filePath);
        foreach (const MsvsProjectConfiguration &buildTask, disabledConfigurations) {
            xmlWriter.writeStartElement(QLatin1String("ExcludedFromBuild"));
            xmlWriter.writeAttribute(QLatin1String("Condition"), QLatin1String("'$(Configuration)|$(Platform)'=='")+  buildTask.fullName() + QLatin1String("'"));
            xmlWriter.writeCharacters(QLatin1String("true"));
            xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement(QLatin1String("Import"));
    xmlWriter.writeAttribute(QLatin1String("Project"), QLatin1String("$(VCTargetsPath)\\Microsoft.Cpp.targets"));
    xmlWriter.writeEndElement();
}

void MsvsFileWriter::writeVcProjFiles(QXmlStreamWriter &xmlWriter, const QSet<MsvsProjectConfiguration> &allConfigurations, const MsvsFileWriter::ProjectConfigurations &allProjectFilesConfigurations) const
{
    xmlWriter.writeStartElement(QLatin1String("Files"));
    foreach (const FilterOptions &options, m_filterOptions) {
        QList<FilePathWithConfigurations> filterFilesWithDisabledConfigurations;
        foreach (const QString &filePath, allProjectFilesConfigurations.keys()) {
            if (options.appliesToFilename(filePath)) {
                QStringList disabledFileConfigurations;
                QSet<MsvsProjectConfiguration> disabledConfigurations = allConfigurations - allProjectFilesConfigurations[filePath];
                foreach (const MsvsProjectConfiguration &buildTask, disabledConfigurations)
                    disabledFileConfigurations << buildTask.fullName();

                filterFilesWithDisabledConfigurations << FilePathWithConfigurations(filePath, disabledFileConfigurations);
            }
        }

        if (filterFilesWithDisabledConfigurations.isEmpty())
            continue;

        xmlWriter.writeStartElement(QLatin1String("Filter"));
        xmlWriter.writeAttribute(QLatin1String("Name"), options.title);

        foreach (const FilePathWithConfigurations &filePathAndConfig, filterFilesWithDisabledConfigurations) {
            xmlWriter.writeStartElement(QLatin1String("File"));
            xmlWriter.writeAttribute(QLatin1String("RelativePath"), filePathAndConfig.first); // No error! In VS absolute paths stored such way.

            foreach (const QString &disabledConfiguration, filePathAndConfig.second) {
                xmlWriter.writeStartElement(QLatin1String("FileConfiguration"));
                xmlWriter.writeAttribute(QLatin1String("Name"), disabledConfiguration);
                xmlWriter.writeAttribute(QLatin1String("ExcludedFromBuild"), QLatin1String("true"));
                xmlWriter.writeEndElement();
            }

            xmlWriter.writeEndElement();
        }

        xmlWriter.writeEndElement();
    }
}

bool MsvsFileWriter::generateVcproj(const MsvsPreparedProduct &product) const
{
    QFile file(product.vcprojFilepath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);

    writeProjectHeader(xmlWriter, product);

    // Configurations section
    if (!m_options.useMSBuild)
         xmlWriter.writeStartElement(QLatin1String("Configurations"));

    ProjectConfigurations allProjectFilesConfigurations;
    QSet<MsvsProjectConfiguration> allConfigurations = product.configurations.keys().toSet();
    foreach (const MsvsProjectConfiguration &buildTask, allConfigurations) {
        const ProductData& productData = product.configurations[buildTask];
        writeOneConfiguration(xmlWriter, allProjectFilesConfigurations, product, buildTask, productData);
    }

    if (!m_options.useMSBuild)
         xmlWriter.writeEndElement(); // </Configurations>

    if (m_options.useMSBuild) {
        writeMsBuildFiles(xmlWriter, allConfigurations, allProjectFilesConfigurations);
    } else {
        writeVcProjFiles(xmlWriter, allConfigurations, allProjectFilesConfigurations);
    }

    writeProjectFooter(xmlWriter);

    return !xmlWriter.hasError() && file.flush();
}

bool MsvsFileWriter::generateVcprojFilters(const MsvsPreparedProduct &product) const
{
    // For older projects formats this is not needed
    if (!m_options.useMSBuild) {
        return true;
    }

    QFile file(product.vcprojFilepath + QLatin1String(".filters"));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);

    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement(QLatin1String("Project"));
    xmlWriter.writeAttribute(QLatin1String("ToolsVersion"), m_options.toolsVersion);
    xmlWriter.writeAttribute(QLatin1String("xmlns"), QLatin1String("http://schemas.microsoft.com/developer/msbuild/2003"));

    foreach (const FilterOptions &options, m_filterOptions) {
        xmlWriter.writeStartElement(QLatin1String("ItemGroup"));
        xmlWriter.writeAttribute(QLatin1String("Label"), QLatin1String("ProjectConfigurations"));

            xmlWriter.writeStartElement(QLatin1String("Filter"));
            xmlWriter.writeAttribute(QLatin1String("Include"), options.title);

                xmlWriter.writeStartElement(QLatin1String("UniqueIdentifier"));
                xmlWriter.writeCharacters(QUuid::createUuid().toString());
                xmlWriter.writeEndElement();

                xmlWriter.writeStartElement(QLatin1String("Extensions"));
                xmlWriter.writeCharacters(options.extensions.join(_VcprojListsSeparator));
                xmlWriter.writeEndElement();

                if (!options.additionalOptions.isEmpty()) {
                    xmlWriter.writeStartElement(options.additionalOptions);
                    xmlWriter.writeCharacters(QLatin1String("False"));// We write only "False" additional options. Could be changed later.
                    xmlWriter.writeEndElement();
                }

            xmlWriter.writeEndElement();

        xmlWriter.writeEndElement();
    }

    xmlWriter.writeStartElement(QLatin1String("ItemGroup"));
    QSet<QString> allFiles;

    foreach (const MsvsProjectConfiguration &buildTask, product.configurations.keys())
        foreach (const GroupData &groupData, product.configurations[buildTask].groups())
            if (groupData.isEnabled())
                allFiles.unite(groupData.allFilePaths().toSet());

    foreach (const QString& fileName, allFiles) {
        xmlWriter.writeStartElement(QLatin1String("ClCompile"));

            xmlWriter.writeAttribute(QLatin1String("Include"), fileName);
            xmlWriter.writeStartElement(QLatin1String("Filter"));
            // TODO: can we get file tags here from GroupData?
            foreach (const FilterOptions &options, m_filterOptions) // FIXME: non-optimal algorithm. Fix on bad performance.
                if (options.appliesToFilename(fileName))
                    xmlWriter.writeCharacters(options.title);

            xmlWriter.writeEndElement();

        xmlWriter.writeEndElement();
    }

    xmlWriter.writeEndElement();

    xmlWriter.writeEndDocument();

    return !xmlWriter.hasError() && file.flush();
}

QString MsvsFileWriter::projectBuildVariant(const Project &project)
{
    return project.projectConfiguration()
                   [QLatin1String("qbs")].toMap()
            [QLatin1String("buildVariant")].toString();
}

void MsvsFileWriter::writeProjectSubFolders(QTextStream &solutionOutStream, const MsvsPreparedProject &project) const
{
    foreach (const MsvsPreparedProject &subProject, project.subProjects) {
        solutionOutStream << QString::fromLocal8Bit("Project(\"%1\") = \"%2\", \"%2\", \"%3\"\n"
                                     "EndProject\n")
                                     .arg(_GUIDProjectFolder)
                                     .arg(subProject.name)
                                     .arg(subProject.guid);
        writeProjectSubFolders(solutionOutStream, subProject);
    }
}

void MsvsFileWriter::writeNestedProjects(QTextStream &solutionOutStream, const MsvsPreparedProject &project) const
{
    foreach (const MsvsPreparedProject& subProject, project.subProjects)
        writeNestedProjects(solutionOutStream, subProject);

    foreach (QSharedPointer<MsvsPreparedProduct> product, project.products.values())
        solutionOutStream << QString::fromLocal8Bit("\t\t%1 = %2\n").arg(product->guid).arg(project.guid);
}

void MsvsFileWriter::writeProjectHeader(QXmlStreamWriter &xmlWriter, const MsvsPreparedProduct &product) const
{
    xmlWriter.writeStartDocument();
    if (m_options.useMSBuild) {
        xmlWriter.writeStartElement(QLatin1String("Project"));
        xmlWriter.writeAttribute(QLatin1String("DefaultTargets"), QLatin1String("Build"));
        xmlWriter.writeAttribute(QLatin1String("ToolsVersion"), m_options.toolsVersion);
        xmlWriter.writeAttribute(QLatin1String("xmlns"), QLatin1String("http://schemas.microsoft.com/developer/msbuild/2003"));
    } else {
        xmlWriter.writeStartElement(QLatin1String("VisualStudioProject"));
        xmlWriter.writeAttribute(QLatin1String("ProjectType"), QLatin1String("Visual C++"));
        xmlWriter.writeAttribute(QLatin1String("Version"), m_options.toolsVersion);
        xmlWriter.writeAttribute(QLatin1String("Name"), product.name);
        xmlWriter.writeAttribute(QLatin1String("ProjectGUID"), product.guid);
    }

    // Project begin
    if (m_options.useMSBuild) {
        xmlWriter.writeStartElement(QLatin1String("ItemGroup"));
        xmlWriter.writeAttribute(QLatin1String("Label"), QLatin1String("ProjectConfigurations"));
        foreach (const MsvsProjectConfiguration &buildTask, product.configurations.keys()) {
            xmlWriter.writeStartElement(QLatin1String("ProjectConfiguration"));
            xmlWriter.writeAttribute(QLatin1String("Include"), buildTask.fullName());
                xmlWriter.writeTextElement(QLatin1String("Configuration"), buildTask.profileAndVariant());
                xmlWriter.writeTextElement(QLatin1String("Platform"), buildTask.platform);
            xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement(QLatin1String("PropertyGroup"));
        xmlWriter.writeAttribute(QLatin1String("Label"), QLatin1String("Globals"));
            xmlWriter.writeTextElement(QLatin1String("ProjectGuid"), product.guid);
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement(QLatin1String("Import"));
        xmlWriter.writeAttribute(QLatin1String("Project"), QLatin1String("$(VCTargetsPath)\\Microsoft.Cpp.Default.props"));
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement(QLatin1String("Import"));
        xmlWriter.writeAttribute(QLatin1String("Project"), QLatin1String("$(VCTargetsPath)\\Microsoft.Cpp.props"));
        xmlWriter.writeEndElement();
    } else { // MSVS <= 2008, vcproj format
        xmlWriter.writeStartElement(QLatin1String("Platforms"));
        foreach (const QString &platformName, product.uniquePlatforms()) {
            xmlWriter.writeStartElement(QLatin1String("Platform"));
            xmlWriter.writeAttribute(QLatin1String("Name"), platformName);
            xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();
    }
}

void MsvsFileWriter::writeOneConfiguration(QXmlStreamWriter &xmlWriter,
                                           ProjectConfigurations &allProjectFilesConfigurations,
                                           const MsvsPreparedProduct &product,
                                           const MsvsProjectConfiguration &buildTask,
                                           const ProductData &productData) const
{
    const QString targetDir = product.targetPath;
    const PropertyMap properties = productData.moduleProperties();
    const QString executableSuffix = properties.getModuleProperty(QLatin1String("qbs"), QLatin1String("executableSuffix")).toString();
    const QString fullTargetName =  product.targetName + (product.isApplication ? executableSuffix : QLatin1String(""));

    const bool debugBuild = properties.getModuleProperty(QLatin1String("qbs"), QLatin1String("debugInformation")).toBool();
    const QStringList includePaths = QStringList()
            << properties.getModulePropertiesAsStringList(QLatin1String("cpp"), QLatin1String("includePaths"))
            << properties.getModulePropertiesAsStringList(QLatin1String("cpp"), QLatin1String("systemIncludePaths"));
    const QStringList cppDefines = properties.getModulePropertiesAsStringList(QLatin1String("cpp"), QLatin1String("defines"));

    // "path/to/qbs.exe" {build|clean} -f "path/to/project.qbs" -d "/build/directory/" -p product_name {debug|release} profile:<profileName>
    const QString qbsExecutablePathEscaped = QDir::toNativeSeparators(m_qbsExecutablePath);

    const QStringList commandLineArgs = QStringList()
            << QLatin1String("-f") << QDir::toNativeSeparators(m_qbsProjectFilePath)
            << QLatin1String("-d") << QDir::toNativeSeparators(m_qbsBuildDirectory)
            << QLatin1String("-p") << product.name
            << buildTask.variant
            << QLatin1String("profile:") + buildTask.profile
            << m_projectCommandlineParameters;

    const QStringList installRootArguments = m_installRoot.isEmpty()
            ? QStringList()
            : QStringList() << QLatin1String("--install-root")
                            << QDir::toNativeSeparators(m_installRoot);

    const QStringList buildCommand = QStringList() << qbsExecutablePathEscaped
                                                   << QLatin1String("install")
                                                   << installRootArguments << commandLineArgs;
    const QStringList cleanCommand = QStringList() << qbsExecutablePathEscaped
                                                   << QLatin1String("clean") << commandLineArgs;

    const QString buildTaskCondition = QLatin1String("'$(Configuration)|$(Platform)'=='") + buildTask.fullName() + QLatin1String("'");
    const QString optimizationLevel = properties.getModuleProperty(QLatin1String("qbs"), QLatin1String("optimization")).toString();
    const QString warningLevel = properties.getModuleProperty(QLatin1String("qbs"), QLatin1String("warningLevel")).toString();

    foreach (const GroupData &groupData, productData.groups()) {
        if (groupData.isEnabled()) {
            foreach (const QString &filePath, groupData.allFilePaths()) {
                allProjectFilesConfigurations[ filePath ] << buildTask;
            }
        }
    }

    // For MSVS <= 2008 we set only NMake options, as it ignores VCCompiler options for configuration "Makefile".
    if (!m_options.useMSBuild) {
        xmlWriter.writeStartElement(QLatin1String("Configuration"));
        xmlWriter.writeAttribute(QLatin1String("Name"), buildTask.fullName());
        xmlWriter.writeAttribute(QLatin1String("OutputDirectory"), targetDir);
        xmlWriter.writeAttribute(QLatin1String("ConfigurationType"), _VcprojNmakeConfig);

        xmlWriter.writeStartElement(QLatin1String("Tool"));
        xmlWriter.writeAttribute(QLatin1String("Name"), QLatin1String("VCNMakeTool"));
        xmlWriter.writeAttribute(QLatin1String("BuildCommandLine"), Internal::shellQuote(buildCommand, Internal::HostOsInfo::HostOsWindows));
        xmlWriter.writeAttribute(QLatin1String("ReBuildCommandLine"), Internal::shellQuote(buildCommand, Internal::HostOsInfo::HostOsWindows));  // using build command.
        xmlWriter.writeAttribute(QLatin1String("CleanCommandLine"), Internal::shellQuote(cleanCommand, Internal::HostOsInfo::HostOsWindows));
        xmlWriter.writeAttribute(QLatin1String("Output"), QString::fromLocal8Bit("$(OutDir)%1").arg(fullTargetName));
        xmlWriter.writeAttribute(QLatin1String("PreprocessorDefinitions"), cppDefines.join(_VcprojListsSeparator));
        xmlWriter.writeAttribute(QLatin1String("IncludeSearchPath"), includePaths.join(_VcprojListsSeparator));
        xmlWriter.writeEndElement();

        xmlWriter.writeEndElement();
        return;
    }

    // Setup VCTool compilation option if someone wants to change configuration type.
    xmlWriter.writeStartElement(QLatin1String("PropertyGroup"));
    xmlWriter.writeAttribute(QLatin1String("Condition"), buildTaskCondition);
    xmlWriter.writeAttribute(QLatin1String("Label"), QLatin1String("Configuration"));
        xmlWriter.writeTextElement(QLatin1String("ConfigurationType"), QLatin1String("Makefile"));
        xmlWriter.writeTextElement(QLatin1String("UseDebugLibraries"), debugBuild ? QLatin1String("true") : QLatin1String("false"));
        xmlWriter.writeTextElement(QLatin1String("CharacterSet"), // VS possible values: Unicode|MultiByte|NotSet
                                   properties.getModuleProperty(QLatin1String("cpp"), QLatin1String("windowsApiCharacterSet")) == QLatin1String("unicode") ? QLatin1String("MultiByte") : QLatin1String("NotSet"));
        xmlWriter.writeTextElement(QLatin1String("PlatformToolset"), m_options.platformToolset);
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement(QLatin1String("PropertyGroup"));
    xmlWriter.writeAttribute(QLatin1String("Condition"), buildTaskCondition);
    xmlWriter.writeAttribute(QLatin1String("Label"), QLatin1String("Configuration"));
        xmlWriter.writeTextElement(QLatin1String("NMakeIncludeSearchPath"), includePaths.join(_VcprojListsSeparator));
        xmlWriter.writeTextElement(QLatin1String("NMakePreprocessorDefinitions"), cppDefines.join(_VcprojListsSeparator));
        xmlWriter.writeTextElement(QLatin1String("OutDir"), targetDir);
        xmlWriter.writeTextElement(QLatin1String("TargetName"), productData.targetName());
        xmlWriter.writeTextElement(QLatin1String("NMakeOutput"), QLatin1String("$(OutDir)$(TargetName)$(TargetExt)"));
        xmlWriter.writeTextElement(QLatin1String("LocalDebuggerCommand"), QLatin1String("$(OutDir)$(TargetName)$(TargetExt)"));
        xmlWriter.writeTextElement(QLatin1String("LocalDebuggerWorkingDirectory"), QLatin1String("$(OutDir)"));
        xmlWriter.writeTextElement(QLatin1String("DebuggerFlavor"), QLatin1String("WindowsLocalDebugger"));
        xmlWriter.writeTextElement(QLatin1String("NMakeBuildCommandLine"), Internal::shellQuote(buildCommand, Internal::HostOsInfo::HostOsWindows));
        xmlWriter.writeTextElement(QLatin1String("NMakeCleanCommandLine"), Internal::shellQuote(cleanCommand, Internal::HostOsInfo::HostOsWindows));
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement(QLatin1String("ItemDefinitionGroup"));
    xmlWriter.writeAttribute(QLatin1String("Condition"), buildTaskCondition);
        xmlWriter.writeStartElement(QLatin1String("ClCompile"));
            xmlWriter.writeStartElement(QLatin1String("WarningLevel"));
                if (warningLevel == QLatin1String("none"))
                    xmlWriter.writeCharacters(QLatin1String("TurnOffAllWarnings"));
                else if (warningLevel == QLatin1String("all"))
                    xmlWriter.writeCharacters(QLatin1String("EnableAllWarnings"));
                else
                    xmlWriter.writeCharacters(QLatin1String("Level3"));// this is VS default.
            xmlWriter.writeEndElement();

            xmlWriter.writeTextElement(QLatin1String("Optimization"), optimizationLevel == QLatin1String("none") ? QLatin1String("Disabled") : QLatin1String("MaxSpeed"));
            xmlWriter.writeTextElement(QLatin1String("RuntimeLibrary"),
                                       debugBuild ? QLatin1String("MultiThreadedDebugDLL") : QLatin1String("MultiThreadedDLL"));
            xmlWriter.writeTextElement(QLatin1String("PreprocessorDefinitions"),
                                       cppDefines.join(_VcprojListsSeparator) + _VcprojListsSeparator + QLatin1String("%(PreprocessorDefinitions)"));
            xmlWriter.writeTextElement(QLatin1String("AdditionalIncludeDirectories"),
                                       includePaths.join(_VcprojListsSeparator) + _VcprojListsSeparator + QLatin1String("%(AdditionalIncludeDirectories)"));
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement(QLatin1String("Link"));
            xmlWriter.writeTextElement(QLatin1String("GenerateDebugInformation"), debugBuild ? QLatin1String("true") : QLatin1String("false"));
            xmlWriter.writeTextElement(QLatin1String("OptimizeReferences"), debugBuild ? QLatin1String("false") : QLatin1String("true"));
            xmlWriter.writeTextElement(QLatin1String("AdditionalDependencies"),
                                       properties.getModulePropertiesAsStringList(QLatin1String("cpp"), QLatin1String("staticLibraries")).join(_VcprojListsSeparator) + _VcprojListsSeparator + QLatin1String("%(AdditionalDependencies)"));
            xmlWriter.writeTextElement(QLatin1String("AdditionalLibraryDirectories"),
                                       properties.getModulePropertiesAsStringList(QLatin1String("cpp"), QLatin1String("libraryPaths")).join(_VcprojListsSeparator));
        xmlWriter.writeEndElement();
        xmlWriter.writeEndElement();
}

void MsvsFileWriter::writeProjectFooter(QXmlStreamWriter &xmlWriter) const
{
    xmlWriter.writeEndElement();// </Project> | </VisualStudioProject>
    xmlWriter.writeEndDocument();
}

bool MsvsFileWriter::FilterOptions::appliesToFilename(const QString &filename) const
{
    foreach (const QString &extension, extensions)
        if (filename.endsWith(extension))
           return true;
    return false;
}

MsvsFileWriter::FilterOptions::FilterOptions(const QStringList &extensions, const QString &title, const QString &additionalOptions)
    : extensions(extensions), title(title), additionalOptions(additionalOptions)
{
}

MsvsFileWriter::FilterOptions::FilterOptions(const QString &extensions, const QString &title, const QString &additionalOptions)
         : extensions(extensions.split(_VcprojListsSeparator, QString::SkipEmptyParts)), title(title), additionalOptions(additionalOptions)
{
}
