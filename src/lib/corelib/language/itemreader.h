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

#ifndef QBS_ITEMREADER_H
#define QBS_ITEMREADER_H

#include "forward_decls.h"
#include <logging/logger.h>

#include <QHash>
#include <QSet>
#include <QStack>
#include <QStringList>

namespace qbs {
namespace Internal {

class BuiltinDeclarations;
class Item;
class ItemPool;

struct ItemReaderResult
{
    ItemReaderResult()
        : rootItem(0)
    {}

    Item *rootItem;
};

/*
 * Reads a qbs file and creates a tree of Item objects.
 *
 * In this stage the following steps are performed:
 *    - The QML/JS parser creates the AST.
 *    - The AST is converted to a tree of Item objects.
 *
 * This class is also responsible for the QMLish inheritance semantics.
 */
class ItemReader
{
    friend class ItemReaderASTVisitor;
public:
    ItemReader(BuiltinDeclarations *builtins, const Logger &logger);
    ~ItemReader();

    BuiltinDeclarations *builtins() const { return m_builtins; }
    Logger logger() const { return m_logger; }

    void setPool(ItemPool *pool) { m_pool = pool; }
    void setSearchPaths(const QStringList &searchPaths);
    void pushExtraSearchPaths(const QStringList &extraSearchPaths);
    void popExtraSearchPaths();
    QStringList searchPaths() const;

    Item *readFile(const QString &filePath);

    QSet<QString> filesRead() const;

private:
    ItemReaderResult internalReadFile(const QString &filePath);

    void cacheDirectoryEntries(const QString &dirPath, const QStringList &entries);
    bool findDirectoryEntries(const QString &dirPath, QStringList *entries) const;

    ItemPool *m_pool;
    BuiltinDeclarations *m_builtins;
    Logger m_logger;
    QStringList m_searchPaths;
    QStack<QStringList> m_extraSearchPaths;
    QHash<const Item *, QSet<JSSourceValuePtr> > m_conditionalValuesPerScopeItem;

    class ASTCache;
    ASTCache *m_astCache;
    QSet<QString> m_filesRead;
    QHash<QString, QStringList> m_directoryEntries;
};

} // namespace Internal
} // namespace qbs

#endif // QBS_ITEMREADER_H
