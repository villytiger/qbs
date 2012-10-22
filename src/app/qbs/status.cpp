/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Build Suite.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "status.h"

#include <app/shared/commandlineparser.h>
#include <language/sourceproject.h>
#include <logging/logger.h>
#include <tools/fileinfo.h>

#include <QDir>
#include <QFile>
#include <QRegExp>

namespace qbs {

static QList<QRegExp> createIgnoreList(const QString &projectRootPath)
{
    QList<QRegExp> ignoreRegularExpressionList;
    ignoreRegularExpressionList.append(QRegExp(projectRootPath + "/build.*"));
    ignoreRegularExpressionList.append(QRegExp("*.qbs", Qt::CaseSensitive, QRegExp::Wildcard));
    ignoreRegularExpressionList.append(QRegExp("*.qbp", Qt::CaseSensitive, QRegExp::Wildcard));
    ignoreRegularExpressionList.append(QRegExp("*.pro", Qt::CaseSensitive, QRegExp::Wildcard));
    ignoreRegularExpressionList.append(QRegExp("*Makefile", Qt::CaseSensitive, QRegExp::Wildcard));
    ignoreRegularExpressionList.append(QRegExp("*.so*", Qt::CaseSensitive, QRegExp::Wildcard));
    ignoreRegularExpressionList.append(QRegExp("*.o", Qt::CaseSensitive, QRegExp::Wildcard));
    QString ignoreFilePath = projectRootPath + "/.qbsignore";


    QFile ignoreFile(ignoreFilePath);

    if (ignoreFile.open(QFile::ReadOnly)) {
        QList<QByteArray> ignoreTokenList = ignoreFile.readAll().split('\n');
        foreach (const QString &token, ignoreTokenList) {
            if (token.left(1) == "/")
                ignoreRegularExpressionList.append(QRegExp(projectRootPath + token + ".*", Qt::CaseSensitive, QRegExp::RegExp2));
            else if (!token.isEmpty())
                ignoreRegularExpressionList.append(QRegExp(token, Qt::CaseSensitive, QRegExp::RegExp2));

        }
    }

    return ignoreRegularExpressionList;
}

static QStringList allFilesInDirectoryRecursive(const QDir &rootDirecory, const QList<QRegExp> &ignoreRegularExpressionList)
{
    QStringList fileList;

    foreach (const QFileInfo &fileInfo, rootDirecory.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name)) {
        QString absoluteFilePath = fileInfo.absoluteFilePath();
        bool inIgnoreList = false;
        foreach (const QRegExp &ignoreRegularExpression, ignoreRegularExpressionList) {
            if (ignoreRegularExpression.exactMatch(absoluteFilePath)) {
                inIgnoreList = true;
                break;
            }
        }

        if (!inIgnoreList) {
            if (fileInfo.isFile())
                fileList.append(absoluteFilePath);
            else if (fileInfo.isDir())
                fileList.append(allFilesInDirectoryRecursive(QDir(absoluteFilePath), ignoreRegularExpressionList));

        }
    }

    return fileList;
}

static QStringList allFilesInProject(const QString &projectRootPath)
{
    QList<QRegExp> ignoreRegularExpressionList = createIgnoreList(projectRootPath);

    return allFilesInDirectoryRecursive(QDir(projectRootPath), ignoreRegularExpressionList);
}

inline bool lessThanSourceFile(const SourceArtifact::ConstPtr &first,
                               const SourceArtifact::ConstPtr &second)
{
    return first->absoluteFilePath < second->absoluteFilePath;
}

int printStatus(const QString &projectFilePath, const SourceProject &sourceProject)
{
    QString projectDirectory = FileInfo::path(projectFilePath);
    int projectDirectoryPathLength = projectDirectory.length();

    QStringList untrackedFilesInProject = allFilesInProject(projectDirectory);
    QStringList missingFiles;
    BuildProject::Ptr buildProject = sourceProject.buildProjects().first();
    foreach (const BuildProduct::Ptr &buildProduct, buildProject->buildProducts()) {
        qbsInfo() << DontPrintLogLevel << TextColorBlue << "\nProduct: " << buildProduct->rProduct->name;
        QList<SourceArtifact::Ptr> sourceFiles = buildProduct->rProduct->allFiles();
        qSort(sourceFiles.begin(), sourceFiles.end(), lessThanSourceFile);
        foreach (const SourceArtifact::ConstPtr &sourceFile, sourceFiles) {
            QString fileTags = QStringList(sourceFile->fileTags.toList()).join(", ");
            TextColor statusColor = TextColorDefault;
            if (!FileInfo(sourceFile->absoluteFilePath).exists()) {
                statusColor = TextColorRed;
                missingFiles.append(sourceFile->absoluteFilePath);
            }
            qbsInfo() << DontPrintLogLevel << statusColor << "  "
                      << sourceFile->absoluteFilePath.mid(projectDirectoryPathLength + 1)
                      << " [" + fileTags + "]";
            untrackedFilesInProject.removeOne(sourceFile->absoluteFilePath);
        }
    }

    qbsInfo() << DontPrintLogLevel << TextColorDarkBlue << "\nMissing files:";
    foreach (const QString &untrackedFile, missingFiles)
        qbsInfo() << DontPrintLogLevel<< TextColorRed <<"  " << untrackedFile.mid(projectDirectoryPathLength + 1);

    qbsInfo() << DontPrintLogLevel << TextColorDarkBlue << "\nUntracked files:";
    foreach (const QString &missingFile, untrackedFilesInProject)
        qbsInfo() << DontPrintLogLevel<< TextColorCyan <<"  " << missingFile.mid(projectDirectoryPathLength + 1);

    return 0;
}

} // namespace qbs