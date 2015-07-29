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
#ifndef MSVS_FILE_WRITER_H
#define MSVS_FILE_WRITER_H

#include <QString>

#include "msvspreparedproject.h"

QT_BEGIN_NAMESPACE
class QXmlStreamWriter;
class QTextStream;
QT_END_NAMESPACE

namespace qbs
{

class MsvsFileWriter
{
public:
    struct VersionOptions {
        bool useMSBuild;
        QString toolsVersion;
        QString platformToolset;
        QString solutionHeader;
        QString projectExtension;
    };

    MsvsFileWriter(const VersionOptions &versionOptions,
                   const QString &qbsExecutablePath,
                   const QString &qbsProjectFilePath,
                   const QString &qbsBuildDirectory,
                   const QStringList &projectCommandlineParameters,
                   const QString &installRoot);

    static QString projectBuildVariant(const Project &project);

    bool generateSolution(const QString &fileName,
                          const MsvsPreparedProject &project) const;
    bool generateVcproj(const MsvsPreparedProduct &product) const;
    bool generateVcprojFilters(const MsvsPreparedProduct &product) const;

  protected:
    typedef QHash<QString, QSet<MsvsProjectConfiguration> > ProjectConfigurations;
    void writeProjectSubFolders(QTextStream &solutionOutStream, const MsvsPreparedProject &project) const;
    void writeNestedProjects(QTextStream &solutionOutStream, const MsvsPreparedProject &project) const;
    void writeProjectHeader(QXmlStreamWriter &xmlWriter, const MsvsPreparedProduct &product) const;
    void writeOneConfiguration(QXmlStreamWriter &xmlWriter,
                               ProjectConfigurations &allProjectFilesConfigurations,
                               const MsvsPreparedProduct &product,
                               const MsvsProjectConfiguration &buildTask,
                               const ProductData &productData) const;
    void writeMsBuildFiles(QXmlStreamWriter &xmlWriter,
                           const QSet<MsvsProjectConfiguration> &allConfigurations,
                           const ProjectConfigurations &allProjectFilesConfigurations) const;
    void writeVcProjFiles(QXmlStreamWriter &xmlWriter,
                           const QSet<MsvsProjectConfiguration> &allConfigurations,
                           const ProjectConfigurations &allProjectFilesConfigurations) const;

    void writeProjectFooter(QXmlStreamWriter &xmlWriter) const;

    const VersionOptions m_options;

    struct FilterOptions
    {
        QStringList extensions;
        QString title;
        QString additionalOptions;
        bool appliesToFilename(const QString &filename) const;
        FilterOptions(const QStringList &extensions, const QString &title, const QString &additionalOptions = QLatin1String(""));
        FilterOptions(const QString &extensions, const QString &title, const QString &additionalOptions = QLatin1String(""));
    };
    QList<FilterOptions> m_filterOptions;
    QString m_qbsExecutablePath;
    QString m_qbsProjectFilePath;
    QString m_qbsBuildDirectory;
    QStringList m_projectCommandlineParameters;
    QString m_installRoot;

    typedef QPair<QString, QStringList> FilePathWithConfigurations;
};

}

#endif // MSVS_FILE_WRITER_H
