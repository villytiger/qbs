/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of the Qt Build Suite.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms and
** conditions see http://www.qt.io/terms-conditions. For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "preferences.h"

#include "buildoptions.h"
#include "hostosinfo.h"
#include "profile.h"
#include "settings.h"

namespace qbs {

/*!
 * \class Preferences
 * \brief The \c Preferences class gives access to all general qbs preferences.
 * If a non-empty \c profileName is given, the profile's preferences take precedence over global
 * ones. Otherwise, the global preferences are used.
 */
Preferences::Preferences(Settings *settings, const QString &profileName)
    : m_settings(settings), m_profile(profileName)
{
}


/*!
 * \brief Returns true <=> colored output should be used for printing messages.
 * This is only relevant for command-line frontends.
 */
bool Preferences::useColoredOutput() const
{
    return getPreference(QLatin1String("useColoredOutput"), true).toBool();
}

/*!
 * \brief Returns the number of parallel jobs to use for building.
 * Uses a sensible default value if there is no such setting.
 */
int Preferences::jobs() const
{
    return getPreference(QLatin1String("jobs"), BuildOptions::defaultMaxJobCount()).toInt();
}

/*!
 * \brief Returns the shell to use for the "qbs shell" command.
 * This is only relevant for command-line frontends.
 */
QString Preferences::shell() const
{
    return getPreference(QLatin1String("shell")).toString();
}

/*!
 * \brief Returns the default build directory used by Qbs if none is specified.
 */
QString Preferences::defaultBuildDirectory() const
{
    return getPreference(QLatin1String("defaultBuildDirectory")).toString();
}

/*!
 * \brief Returns the default echo mode used by Qbs if none is specified.
 */
CommandEchoMode Preferences::defaultEchoMode() const
{
    return commandEchoModeFromName(getPreference(QLatin1String("defaultEchoMode")).toString());
}

/*!
 * \brief Returns the list of paths where qbs looks for modules and imports.
 * In addition to user-supplied locations, they will also be looked up at \c{baseDir}/share/qbs.
 */
QStringList Preferences::searchPaths(const QString &baseDir) const
{
    return pathList(QLatin1String("qbsSearchPaths"), baseDir + QLatin1String("/share/qbs"));
}

/*!
 * \brief Returns the list of paths where qbs looks for plugins.
 * In addition to user-supplied locations, they will be looked up at \c{baseDir}/qbs/plugins.
 */
QStringList Preferences::pluginPaths(const QString &baseDir) const
{
    return pathList(QLatin1String("pluginsPath"), baseDir + QLatin1String("/qbs/plugins"));
}

QVariant Preferences::getPreference(const QString &key, const QVariant &defaultValue) const
{
    const QString fullKey = QLatin1String("preferences.") + key;
    if (!m_profile.isEmpty()) {
        const QVariant value = Profile(m_profile, m_settings).value(fullKey);
        if (value.isValid())
            return value;
    }

    return m_settings->value(fullKey, defaultValue);
}

QStringList Preferences::pathList(const QString &key, const QString &defaultValue) const
{
    QStringList paths = getPreference(key).toString().split(
                Internal::HostOsInfo::pathListSeparator(), QString::SkipEmptyParts);
    paths << defaultValue;
    return paths;
}

} // namespace qbs
