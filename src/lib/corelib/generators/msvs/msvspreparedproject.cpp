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

#include "msvspreparedproject.h"

#include <QDebug>
#include <QUuid>
#include <QFileInfo>

using namespace qbs;

QList<QSharedPointer<MsvsPreparedProduct> > MsvsPreparedProject::allProducts() const
{
    QList<QSharedPointer<MsvsPreparedProduct> > result = products.values();
    foreach (const MsvsPreparedProject &child, subProjects)
        result << child.allProducts();
    return result;
}

void MsvsPreparedProject::prepare(const Project& qbsProject,
                                  const InstallOptions& installOptions,
                                  const ProjectData& projectData,
                                  const MsvsProjectConfiguration& config)
{
    foreach (const ProjectData &subData, projectData.subProjects()) {
        if (!subProjects.contains(subData.name())) {
            MsvsPreparedProject subPrepared;
            subPrepared.guid = QUuid::createUuid().toString();
            subPrepared.name = subData.name();
            subProjects[subData.name()] = subPrepared;
        }
        subProjects[subData.name()].prepare(qbsProject, installOptions, subData, config);
    }

    if (!projectData.isEnabled() || !projectData.isValid() || projectData.products().isEmpty())
        return;
    foreach (const ProductData &productData, projectData.products()) {
        if (!products.contains(productData.name())) {
            QSharedPointer<MsvsPreparedProduct> product(new MsvsPreparedProduct());
            product->guid = QUuid::createUuid().toString();
            product->name = productData.name();
            QString buildDirectory = productData.properties().value(QLatin1String("buildDirectory")).toString();
            product->isApplication = productData.properties().value(QLatin1String("type")).toStringList().contains(QLatin1String("application"));
            QString fullPath = qbsProject.targetExecutable(productData, installOptions);
            if (!fullPath.isEmpty()) {
                product->targetName = QFileInfo(fullPath).fileName();
                product->targetPath = QFileInfo(fullPath).absolutePath();
            } else {
                product->targetName = productData.targetName();
                product->targetPath = installOptions.installRoot();
            }
            if (product->targetPath.isEmpty())
                product->targetPath = buildDirectory;
            product->targetPath += QLatin1String("/");
            products.insert(product->name, product);
        }
        products[productData.name()]->configurations[config] = productData;
    }

    enabledConfigurations << config;
}

MsvsProjectConfiguration::MsvsProjectConfiguration()
{
}

MsvsProjectConfiguration::MsvsProjectConfiguration(const Project &project)
{
    const QVariantMap qbsSettings = project.projectConfiguration()[QLatin1String("qbs")].toMap();
    variant = qbsSettings[QLatin1String("buildVariant")].toString();
    profile = project.profile();
    const QString architecture = qbsSettings[QLatin1String("architecture")].toString();
    const QStringList toolchain = qbsSettings[QLatin1String("toolchain")].toStringList();
    if (toolchain != QStringList() << QLatin1String("msvc")) {
        qWarning() << "WARNING: Generating Visual Studio project for toolchain:" << toolchain.join(QLatin1Char(','));
    }

    // Select VS platform display name. It doesn't interfere with compilation settings.
    QMap<QString, QString> qbsToVSArch;
    qbsToVSArch[QLatin1String("x86")] = QLatin1String("Win32");
    qbsToVSArch[QLatin1String("x86_64")] = QLatin1String("Win64");
    qbsToVSArch[QLatin1String("ia64")] = QLatin1String("Itanium");
    qbsToVSArch[QLatin1String("arm")] = QLatin1String("Arm");
    qbsToVSArch[QLatin1String("arm64")] = QLatin1String("Arm64");
    if (!qbsToVSArch.contains(architecture)) {
        qWarning() << "Naming qbs platform \"" << architecture << "\" as \"Win32\" for VS project.";
    }
    platform = qbsToVSArch.value(architecture, QLatin1String("Win32"));
}

QString MsvsProjectConfiguration::cleanProfileName() const
{
    QString result = profile;
    return result.replace(QRegExp(QLatin1String("\\W+")), QLatin1String(""));
}

QString MsvsProjectConfiguration::fullName() const
{
    return QString::fromLocal8Bit("%1-%2|%3").arg(cleanProfileName()).arg(variant).arg(platform);
}

QString MsvsProjectConfiguration::profileAndVariant() const
{
    return QString::fromLocal8Bit("%1-%2").arg(cleanProfileName()).arg(variant);
}

bool MsvsProjectConfiguration::operator <(const MsvsProjectConfiguration &right) const
{
    return     platform <  right.platform
            || platform == right.platform  && variant < right.variant
            || platform == right.platform  && variant== right.variant && profile < right.profile;
}

bool MsvsProjectConfiguration::operator ==(const MsvsProjectConfiguration &right) const
{
    return profile == right.profile  && variant== right.variant && platform == right.platform;
}


quint32 qbs::qHash(const MsvsProjectConfiguration &config)
{
    return qHash(QString::fromLocal8Bit("%1-%2|%3").arg(config.profile).arg(config.variant).arg(config.platform));
}

QStringList MsvsPreparedProduct::uniquePlatforms() const
{
    QSet<QString> result;
    foreach (const MsvsProjectConfiguration &configuration, configurations.keys())
        result << configuration.platform;
    return result.toList();
}
