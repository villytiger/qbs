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

#ifndef QBS_ITEMREADERASTVISITOR_H
#define QBS_ITEMREADERASTVISITOR_H

#include "item.h"
#include "filecontext.h"

#include <parser/qmljsastvisitor_p.h>
#include <tools/version.h>

#include <QHash>

namespace qbs {
namespace Internal {

class ItemReader;
struct ItemReaderResult;

class ItemReaderASTVisitor : public QbsQmlJS::AST::Visitor
{
public:
    ItemReaderASTVisitor(ItemReader *reader, ItemReaderResult *result);
    ~ItemReaderASTVisitor();

    void setFilePath(const QString &filePath);
    void setSourceCode(const QString &sourceCode);

    bool visit(QbsQmlJS::AST::UiProgram *ast);
    bool visit(QbsQmlJS::AST::UiImportList *uiImportList);
    bool visit(QbsQmlJS::AST::UiObjectDefinition *ast);
    bool visit(QbsQmlJS::AST::UiPublicMember *ast);
    bool visit(QbsQmlJS::AST::UiScriptBinding *ast);
    bool visit(QbsQmlJS::AST::FunctionDeclaration *ast);

private:
    static Version readImportVersion(const QString &str,
            const CodeLocation &location = CodeLocation());
    bool visitStatement(QbsQmlJS::AST::Statement *statement);
    CodeLocation toCodeLocation(const QbsQmlJS::AST::SourceLocation &location) const;
    void checkDuplicateBinding(Item *item, const QStringList &bindingName,
            const QbsQmlJS::AST::SourceLocation &sourceLocation);
    Item *targetItemForBinding(Item *item, const QStringList &binding,
                                 const JSSourceValueConstPtr &value);
    void checkImportVersion(const QbsQmlJS::AST::SourceLocation &versionToken) const;
    static void inheritItem(Item *dst, const Item *src);
    void ensureIdScope(const FileContextPtr &file);
    void setupAlternatives(Item *item);
    static void replaceConditionScopes(const JSSourceValuePtr &value, Item *newScope);
    void handlePropertiesBlock(Item *item, const Item *block);
    void collectPrototypes(const QString &path, const QString &as);
    bool addPrototype(const QString &fileName, const QString &filePath, const QString &as,
                      bool needsCheck);

    ItemReader *m_reader;
    ItemReaderResult *m_readerResult;
    const Version m_languageVersion;
    FileContextPtr m_file;
    QHash<QStringList, QString> m_typeNameToFile;
    Item *m_item;
    JSSourceValuePtr m_sourceValue;
};

} // namespace Internal
} // namespace qbs

#endif // QBS_ITEMREADERASTVISITOR_H
