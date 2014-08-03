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
static const QString _GUIDProjectFolder = "{2150E333-8FDC-42A3-9474-1A3956D46DE8}";

// Project file format specific options (Do NOT change!)
static const QString _VcprojNmakeConfig = "0";
static const QString _VcprojListsSeparator = ";";

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
    m_filterOptions << FilterOptions("c;C;cpp;cxx;c++;cc;def;m;mm", "Source Files");
    m_filterOptions << FilterOptions("h;H;hpp;hxx;h++", "Header Files");
    m_filterOptions << FilterOptions("ui", "Form Files");
    m_filterOptions << FilterOptions("qrc;rc;*", "Resource Files", "ParseFiles");
    m_filterOptions << FilterOptions("moc", "Generated Files", "SourceControlFiles");
    m_filterOptions << FilterOptions("ts", "Translation Files", "ParseFiles");
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
        solutionOutStream << QString("Project(\"%1\") = \"%2\", \"%3\", \"%4\"\n")
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
        solutionOutStream << QString("\t\t%1 = %1\n").arg(buildTask.fullName());
    }

    solutionOutStream << "\tEndGlobalSection\n";

    solutionOutStream << "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n";
    foreach (QSharedPointer<MsvsPreparedProduct> product, project.allProducts()) {
        foreach (const MsvsProjectConfiguration &buildTask, product->configurations.keys()) {
            solutionOutStream << QString("\t\t%1.%2.ActiveCfg = %2\n"
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
    xmlWriter.writeStartElement("ItemGroup");

    foreach (const QString &filePath, allProjectFilesConfigurations.keys()) {
        xmlWriter.writeStartElement("ClCompile");
        QSet<MsvsProjectConfiguration> disabledConfigurations = allConfigurations - allProjectFilesConfigurations[filePath];
        xmlWriter.writeAttribute("Include", filePath);
        foreach (const MsvsProjectConfiguration &buildTask, disabledConfigurations) {
            xmlWriter.writeStartElement("ExcludedFromBuild");
            xmlWriter.writeAttribute("Condition", "'$(Configuration)|$(Platform)'=='"+  buildTask.fullName() + "'");
            xmlWriter.writeCharacters("true");
            xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Import");
    xmlWriter.writeAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets");
    xmlWriter.writeEndElement();
}

void MsvsFileWriter::writeVcProjFiles(QXmlStreamWriter &xmlWriter, const QSet<MsvsProjectConfiguration> &allConfigurations, const MsvsFileWriter::ProjectConfigurations &allProjectFilesConfigurations) const
{
    xmlWriter.writeStartElement("Files");
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

        xmlWriter.writeStartElement("Filter");
        xmlWriter.writeAttribute("Name", options.title);

        foreach (const FilePathWithConfigurations &filePathAndConfig, filterFilesWithDisabledConfigurations) {
            xmlWriter.writeStartElement("File");
            xmlWriter.writeAttribute("RelativePath", filePathAndConfig.first); // No error! In VS absolute paths stored such way.

            foreach (const QString &disabledConfiguration, filePathAndConfig.second) {
                xmlWriter.writeStartElement("FileConfiguration");
                xmlWriter.writeAttribute("Name", disabledConfiguration);
                xmlWriter.writeAttribute("ExcludedFromBuild", "true");
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
         xmlWriter.writeStartElement("Configurations");

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

    QFile file(product.vcprojFilepath + ".filters");
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);

    xmlWriter.writeStartDocument();
    xmlWriter.writeStartElement("Project");
    xmlWriter.writeAttribute("ToolsVersion", m_options.toolsVersion);
    xmlWriter.writeAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

    foreach (const FilterOptions &options, m_filterOptions) {
        xmlWriter.writeStartElement("ItemGroup");
        xmlWriter.writeAttribute("Label", "ProjectConfigurations");

            xmlWriter.writeStartElement("Filter");
            xmlWriter.writeAttribute("Include", options.title);

                xmlWriter.writeStartElement("UniqueIdentifier");
                xmlWriter.writeCharacters(QUuid::createUuid().toString());
                xmlWriter.writeEndElement();

                xmlWriter.writeStartElement("Extensions");
                xmlWriter.writeCharacters(options.extensions.join(_VcprojListsSeparator));
                xmlWriter.writeEndElement();

                if (!options.additionalOptions.isEmpty()) {
                    xmlWriter.writeStartElement(options.additionalOptions);
                    xmlWriter.writeCharacters("False");// We write only "False" additional options. Could be changed later.
                    xmlWriter.writeEndElement();
                }

            xmlWriter.writeEndElement();

        xmlWriter.writeEndElement();
    }

    xmlWriter.writeStartElement("ItemGroup");
    QSet<QString> allFiles;

    foreach (const MsvsProjectConfiguration &buildTask, product.configurations.keys())
        foreach (const GroupData &groupData, product.configurations[buildTask].groups())
            if (groupData.isEnabled())
                allFiles.unite(groupData.allFilePaths().toSet());

    foreach (const QString& fileName, allFiles) {
        xmlWriter.writeStartElement("ClCompile");

            xmlWriter.writeAttribute("Include", fileName);
            xmlWriter.writeStartElement("Filter");
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
        solutionOutStream << QString("Project(\"%1\") = \"%2\", \"%2\", \"%3\"\n"
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
        solutionOutStream << QString("\t\t%1 = %2\n").arg(product->guid).arg(project.guid);
}

void MsvsFileWriter::writeProjectHeader(QXmlStreamWriter &xmlWriter, const MsvsPreparedProduct &product) const
{
    xmlWriter.writeStartDocument();
    if (m_options.useMSBuild) {
        xmlWriter.writeStartElement("Project");
        xmlWriter.writeAttribute("DefaultTargets", "Build");
        xmlWriter.writeAttribute("ToolsVersion", m_options.toolsVersion);
        xmlWriter.writeAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
    } else {
        xmlWriter.writeStartElement("VisualStudioProject");
        xmlWriter.writeAttribute("ProjectType", "Visual C++");
        xmlWriter.writeAttribute("Version", m_options.toolsVersion);
        xmlWriter.writeAttribute("Name", product.name);
        xmlWriter.writeAttribute("ProjectGUID", product.guid);
    }

    // Project begin
    if (m_options.useMSBuild) {
        xmlWriter.writeStartElement("ItemGroup");
        xmlWriter.writeAttribute("Label", "ProjectConfigurations");
        foreach (const MsvsProjectConfiguration &buildTask, product.configurations.keys()) {
            xmlWriter.writeStartElement("ProjectConfiguration");
            xmlWriter.writeAttribute("Include", buildTask.fullName());
                xmlWriter.writeTextElement("Configuration", buildTask.profileAndVariant());
                xmlWriter.writeTextElement("Platform", buildTask.platform);
            xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("PropertyGroup");
        xmlWriter.writeAttribute("Label", "Globals");
            xmlWriter.writeTextElement("ProjectGuid", product.guid);
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("Import");
        xmlWriter.writeAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
        xmlWriter.writeEndElement();
        xmlWriter.writeStartElement("Import");
        xmlWriter.writeAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");
        xmlWriter.writeEndElement();
    } else { // MSVS <= 2008, vcproj format
        xmlWriter.writeStartElement("Platforms");
        foreach (const QString &platformName, product.uniquePlatforms()) {
            xmlWriter.writeStartElement("Platform");
            xmlWriter.writeAttribute("Name", platformName);
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
    const QString executableSuffix = properties.getModuleProperty("qbs", "executableSuffix").toString();
    const QString fullTargetName =  product.targetName + (product.isApplication ? executableSuffix : "");

    const bool debugBuild = properties.getModuleProperty("qbs", "debugInformation").toBool();
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

    const QString buildTaskCondition = "'$(Configuration)|$(Platform)'=='" + buildTask.fullName() + "'";
    const QString optimizationLevel = properties.getModuleProperty("qbs", "optimization").toString();
    const QString warningLevel = properties.getModuleProperty("qbs", "warningLevel").toString();

    foreach (const GroupData &groupData, productData.groups()) {
        if (groupData.isEnabled()) {
            foreach (const QString &filePath, groupData.allFilePaths()) {
                allProjectFilesConfigurations[ filePath ] << buildTask;
            }
        }
    }

    // For MSVS <= 2008 we set only NMake options, as it ignores VCCompiler options for configuration "Makefile".
    if (!m_options.useMSBuild) {
        xmlWriter.writeStartElement("Configuration");
        xmlWriter.writeAttribute("Name", buildTask.fullName());
        xmlWriter.writeAttribute("OutputDirectory", targetDir);
        xmlWriter.writeAttribute("ConfigurationType", _VcprojNmakeConfig);

        xmlWriter.writeStartElement("Tool");
        xmlWriter.writeAttribute("Name", "VCNMakeTool");
        xmlWriter.writeAttribute("BuildCommandLine", Internal::shellQuoteWin(buildCommand));
        xmlWriter.writeAttribute("ReBuildCommandLine", Internal::shellQuoteWin(buildCommand));  // using build command.
        xmlWriter.writeAttribute("CleanCommandLine", Internal::shellQuoteWin(cleanCommand));
        xmlWriter.writeAttribute("Output", QString("$(OutDir)%1").arg(fullTargetName));
        xmlWriter.writeAttribute("PreprocessorDefinitions", cppDefines.join(_VcprojListsSeparator));
        xmlWriter.writeAttribute("IncludeSearchPath", includePaths.join(_VcprojListsSeparator));
        xmlWriter.writeEndElement();

        xmlWriter.writeEndElement();
        return;
    }

    // Setup VCTool compilation option if someone wants to change configuration type.
    xmlWriter.writeStartElement("PropertyGroup");
    xmlWriter.writeAttribute("Condition", buildTaskCondition);
    xmlWriter.writeAttribute("Label", "Configuration");
        xmlWriter.writeTextElement("ConfigurationType", "Makefile");
        xmlWriter.writeTextElement("UseDebugLibraries", debugBuild ? "true" : "false");
        xmlWriter.writeTextElement("CharacterSet", // VS possible values: Unicode|MultiByte|NotSet
                                   properties.getModuleProperty("cpp", "windowsApiCharacterSet") == "unicode" ? "MultiByte" : "NotSet");
        xmlWriter.writeTextElement("PlatformToolset", m_options.platformToolset);
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("PropertyGroup");
    xmlWriter.writeAttribute("Condition", buildTaskCondition);
    xmlWriter.writeAttribute("Label", "Configuration");
        xmlWriter.writeTextElement("NMakeIncludeSearchPath", includePaths.join(_VcprojListsSeparator));
        xmlWriter.writeTextElement("NMakePreprocessorDefinitions", cppDefines.join(_VcprojListsSeparator));
        xmlWriter.writeTextElement("OutDir", targetDir);
        xmlWriter.writeTextElement("TargetName", productData.targetName());
        xmlWriter.writeTextElement("NMakeOutput", "$(OutDir)$(TargetName)$(TargetExt)");
        xmlWriter.writeTextElement("LocalDebuggerCommand", "$(OutDir)$(TargetName)$(TargetExt)");
        xmlWriter.writeTextElement("LocalDebuggerWorkingDirectory", "$(OutDir)");
        xmlWriter.writeTextElement("DebuggerFlavor", "WindowsLocalDebugger");
        xmlWriter.writeTextElement("NMakeBuildCommandLine", Internal::shellQuoteWin(buildCommand));
        xmlWriter.writeTextElement("NMakeCleanCommandLine", Internal::shellQuoteWin(cleanCommand));
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("ItemDefinitionGroup");
    xmlWriter.writeAttribute("Condition", buildTaskCondition);
        xmlWriter.writeStartElement("ClCompile");
            xmlWriter.writeStartElement("WarningLevel");
                if (warningLevel == "none")
                    xmlWriter.writeCharacters("TurnOffAllWarnings");
                else if (warningLevel == "all")
                    xmlWriter.writeCharacters("EnableAllWarnings");
                else
                    xmlWriter.writeCharacters("Level3");// this is VS default.
            xmlWriter.writeEndElement();

            xmlWriter.writeTextElement("Optimization", optimizationLevel == "none" ? "Disabled" : "MaxSpeed");
            xmlWriter.writeTextElement("RuntimeLibrary",
                                       debugBuild ? "MultiThreadedDebugDLL" : "MultiThreadedDLL");
            xmlWriter.writeTextElement("PreprocessorDefinitions",
                                       cppDefines.join(_VcprojListsSeparator) + _VcprojListsSeparator + "%(PreprocessorDefinitions)");
            xmlWriter.writeTextElement("AdditionalIncludeDirectories",
                                       includePaths.join(_VcprojListsSeparator) + _VcprojListsSeparator + "%(AdditionalIncludeDirectories)");
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("Link");
            xmlWriter.writeTextElement("GenerateDebugInformation", debugBuild ? "true" : "false");
            xmlWriter.writeTextElement("OptimizeReferences", debugBuild ? "false" : "true");
            xmlWriter.writeTextElement("AdditionalDependencies",
                                       properties.getModulePropertiesAsStringList(QLatin1String("cpp"), QLatin1String("staticLibraries")).join(_VcprojListsSeparator) + _VcprojListsSeparator + "%(AdditionalDependencies)");
            xmlWriter.writeTextElement("AdditionalLibraryDirectories",
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
