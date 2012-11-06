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
#include "artifactvisitor.h"

#include "artifact.h"

namespace qbs {

ArtifactVisitor::ArtifactVisitor(int artifactType) : m_artifactType(artifactType)
{
}

void ArtifactVisitor::visit(const Artifact *artifact)
{
    if (m_allArtifacts.contains(artifact))
        return;
    m_allArtifacts << artifact;
    if (m_artifactType & artifact->artifactType)
        doVisit(artifact);
    else if (m_artifactType == Artifact::Generated)
        return;
    foreach (const Artifact * const child, artifact->children)
        visit(child);
}

void ArtifactVisitor::visit(const BuildProduct::ConstPtr &product)
{
    foreach (const Artifact * const artifact, product->targetArtifacts)
        visit(artifact);
}

void ArtifactVisitor::visit(const BuildProject::ConstPtr &project)
{
    foreach (const BuildProduct::ConstPtr &product, project->buildProducts())
        visit(product);
}

void ArtifactVisitor::visit(const QList<BuildProject::ConstPtr> &projects)
{
    foreach (const BuildProject::ConstPtr &project, projects)
        visit(project);
}

void ArtifactVisitor::visit(const QList<BuildProject::Ptr> &projects)
{
    foreach (const BuildProject::ConstPtr &project, projects)
        visit(project);
}

} // namespace qbs