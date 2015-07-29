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
#include "msvsgenerator.h"

#include <logging/translator.h>
#include <tools/shellutils.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

using namespace qbs;
using namespace qbs::Internal;

MsvsGenerator::MsvsGenerator(MsvsProductVersion generatorVersion)
{
    int formatVersion = 0, yearVersion = 0, toolsetVersion = 0;
    switch (generatorVersion) {
        case MSVS2005:
            formatVersion = 9;
            yearVersion = 2005;
            toolsetVersion = 8;
            m_versionOptions.toolsVersion = QLatin1String("8,00");
            break;
        case MSVS2008:
            formatVersion = 10;
            yearVersion = 2008;
            toolsetVersion = 9;
            m_versionOptions.toolsVersion = QLatin1String("9,00");
            break;
        case MSVS2010:
            formatVersion = 11;
            yearVersion = 2010;
            toolsetVersion = 10;
            m_versionOptions.toolsVersion = QLatin1String("4.0"); // When VS switched to MSBuild, version became dot-separated.
            break;
        case MSVS2012:
            formatVersion = 12;
            yearVersion = 2012;
            toolsetVersion = 11;
            m_versionOptions.toolsVersion = QLatin1String("4.0");
            break;
        case MSVS2013:
            formatVersion = 12;// No error. In 2013 format version just the same.
            yearVersion = 2013;
            toolsetVersion = 12;
            m_versionOptions.toolsVersion = QLatin1String("4.0");
            break;
        default:
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
            Q_UNREACHABLE();
#endif
            ;
    }

    m_generatorName = QString::fromLocal8Bit("msvs%1").arg(yearVersion);
    m_versionOptions.platformToolset =  QString::fromLocal8Bit("v%1").arg(toolsetVersion * 10);
    m_versionOptions.projectExtension = (generatorVersion <= MSVS2008) ? QLatin1String(".vcproj") : QLatin1String(".vcxproj");
    m_versionOptions.solutionHeader = QString::fromLocal8Bit("Microsoft Visual Studio Solution File, Format Version %1.00\n"
                                                "# Visual Studio %2\n")
                                                .arg(formatVersion)
                                                .arg(yearVersion);
}

QString MsvsGenerator::generatorName() const
{
    return m_generatorName;
}

void MsvsGenerator::generate(const InstallOptions& installOptions)
{
    // Each Project represents one build configuration. For example, for "qbs build debug release" we get two projects.
    QList<Project> allProjects = projects();
    Q_ASSERT(!allProjects.isEmpty());

    MsvsPreparedProject project;
    foreach (const Project& qbsProject, allProjects)
        project.prepare(qbsProject, installOptions, qbsProject.projectData(), MsvsProjectConfiguration(qbsProject));

    const Project qbsProject = allProjects.first();

    // Common settings setup: qbs environment, passed build settings.
    const QString qbsProjectFilePath = qbsProject.projectData().location().filePath();
    const QString qbsBaseBuildDirectory = QFileInfo(qbsProject.projectData().buildDirectory() + QLatin1String("/..")).absoluteFilePath() + QLatin1String("/");
    const QString qbsExecutablePath = QFileInfo(QCoreApplication::applicationFilePath()).absoluteFilePath();
    const QString generatedSolutionFilePath = qbsBaseBuildDirectory + QFileInfo(qbsProjectFilePath).baseName() + QLatin1String(".sln");

    QStringList projectCommandlineParameters;     // we can pass extra params to generate command; them should be saved.
    QVariantMap projectSettings = qbsProject.projectConfiguration().value(QLatin1String("project")).toMap();
    foreach (const QString& key, projectSettings.keys())
        projectCommandlineParameters += QString::fromLocal8Bit("project.%1:%2").arg(key).arg(projectSettings[key].toString());

    const MsvsFileWriter writer(m_versionOptions, qbsExecutablePath, qbsProjectFilePath, qbsBaseBuildDirectory, projectCommandlineParameters, installOptions.installRoot());

    foreach (QSharedPointer<MsvsPreparedProduct> product, project.allProducts()) {
        product->vcprojFilepath = qbsBaseBuildDirectory + product->name + m_versionOptions.projectExtension;
        if (!writer.generateVcprojFilters(*product.data())) {
            throw ErrorInfo(Tr::tr("Failed to generate %1.filters").arg(product->vcprojFilepath));
        }

        if (!writer.generateVcproj(*product.data())) {
            throw ErrorInfo(Tr::tr("Failed to generate %1").arg(product->vcprojFilepath));
        }
    }

    if (!writer.generateSolution(generatedSolutionFilePath, project)) {
        throw ErrorInfo(Tr::tr("Failed to generate %1").arg(generatedSolutionFilePath));
    }

    qDebug() << "Generated" << qPrintable(QFileInfo(generatedSolutionFilePath).fileName());
}

QList<QSharedPointer<ProjectGenerator> > MsvsGenerator::createGeneratorList()
{
    QList<QSharedPointer<ProjectGenerator> > result;
    for (int i = MSVS_MINIMUM_VERSION; i <= MSVS_MAXIMUM_VERSION; ++i)
        result << QSharedPointer<ProjectGenerator>(new MsvsGenerator(MsvsProductVersion(i)));
    return result;
}
