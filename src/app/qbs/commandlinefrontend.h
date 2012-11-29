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
#ifndef COMMANDLINEFRONTEND_H
#define COMMANDLINEFRONTEND_H

#include "../shared/commandlineparser.h"
#include <api/project.h>
#include <api/projectdata.h>

#include <QHash>
#include <QList>
#include <QObject>

namespace qbs {
class AbstractJob;
class ConsoleProgressObserver;

class CommandLineFrontend : public QObject
{
    Q_OBJECT
public:
    explicit CommandLineFrontend(const CommandLineParser &parser, QObject *parent = 0);

    void cancel();

private slots:
    void start();
    void handleJobFinished(bool success, qbs::AbstractJob *job);
    void handleNewTaskStarted(const QString &description, int totalEffort);
    void handleTaskProgress(int value, qbs::AbstractJob *job);

private:
    typedef QHash<Project, QList<ProductData> > ProductMap;
    ProductMap productsToUse() const;

    bool resolvingMultipleProjects() const;
    bool isResolving() const;
    bool isBuilding() const;
    void handleProjectsResolved();
    void makeClean();
    int runShell();
    void build();
    int runTarget();
    void connectBuildJobs();
    void connectJob(AbstractJob *job);

    const CommandLineParser m_parser;
    QList<AbstractJob *> m_resolveJobs;
    QList<AbstractJob *> m_buildJobs;
    QList<Project> m_projects;

    ConsoleProgressObserver *m_observer;
    int m_buildEffortsNeeded;
    int m_buildEffortsRetrieved;
    int m_totalBuildEffort;
    int m_currentBuildEffort;
    QHash<AbstractJob *, int> m_buildEfforts;
};

} // namespace qbs

#endif // COMMANDLINEFRONTEND_H