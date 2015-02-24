/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2015 Jake Petroules.
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

#include "projectgeneratormanager.h"

#include <logging/logger.h>
#include <logging/translator.h>
#include <tools/hostosinfo.h>

#include <QCoreApplication>
#include <QDirIterator>
#include <QLibrary>

#include "msvs/msvsgenerator.h"

namespace qbs {

using namespace Internal;

ProjectGeneratorManager::~ProjectGeneratorManager()
{
    foreach (QLibrary * const lib, m_libs) {
        lib->unload();
        delete lib;
    }
}

ProjectGeneratorManager *ProjectGeneratorManager::instance()
{
    static ProjectGeneratorManager generatorPlugin;
    return &generatorPlugin;
}

ProjectGeneratorManager::ProjectGeneratorManager()
{
    QList<QSharedPointer<ProjectGenerator> > generators;
    generators << qbs::MsvsGenerator::createGeneratorList();
    foreach (QSharedPointer<ProjectGenerator> generator, generators) {
        m_generators[generator->generatorName()] = generator;
    }
}

QStringList ProjectGeneratorManager::loadedGeneratorNames()
{
    return instance()->m_generators.keys();
}

QSharedPointer<ProjectGenerator> ProjectGeneratorManager::findGenerator(const QString &generatorName)
{
    return instance()->m_generators.value(generatorName);
}

} // namespace qbs
