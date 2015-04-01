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

#ifndef QBS_VALUE_H
#define QBS_VALUE_H

#include "filecontext.h"
#include "item.h"
#include <tools/codelocation.h>
#include <QVariant>

namespace qbs {
namespace Internal {

class ValueHandler;

class Value
{
public:
    enum Type
    {
        JSSourceValueType,
        ItemValueType,
        VariantValueType,
        BuiltinValueType
    };

    Value(Type t);
    virtual ~Value();

    Type type() const { return m_type; }
    virtual void apply(ValueHandler *) = 0;
    virtual ValuePtr clone() const = 0;
    virtual CodeLocation location() const { return CodeLocation(); }

private:
    Type m_type;
};

class ValueHandler
{
public:
    virtual void handle(JSSourceValue *value) = 0;
    virtual void handle(ItemValue *value) = 0;
    virtual void handle(VariantValue *value) = 0;
    virtual void handle(BuiltinValue *value) = 0;
};

class JSSourceValue : public Value
{
    friend class ItemReaderASTVisitor;
    JSSourceValue();

    enum Flag
    {
        NoFlags = 0x00,
        SourceUsesBase = 0x01,
        SourceUsesOuter = 0x02,
        SourceUsesProduct = 0x04,
        HasFunctionForm = 0x08
    };
    Q_DECLARE_FLAGS(Flags, Flag)

public:
    static JSSourceValuePtr create();
    ~JSSourceValue();

    void apply(ValueHandler *handler) { handler->handle(this); }
    ValuePtr clone() const;

    void setSourceCode(const QStringRef &sourceCode) { m_sourceCode = sourceCode; }
    const QStringRef &sourceCode() const { return m_sourceCode; }
    QString sourceCodeForEvaluation() const;

    void setLocation(int line, int column);
    int line() const { return m_line; }
    int column() const { return m_column; }
    CodeLocation location() const;

    void setFile(const FileContextPtr &file) { m_file = file; }
    const FileContextPtr &file() const { return m_file; }

    void setSourceUsesBaseFlag() { m_flags |= SourceUsesBase; }
    bool sourceUsesBase() const { return m_flags.testFlag(SourceUsesBase); }
    bool sourceUsesOuter() const { return m_flags.testFlag(SourceUsesOuter); }
    void setSourceUsesProductFlag() { m_flags |= SourceUsesProduct; }
    bool sourceUsesProduct() const { return m_flags.testFlag(SourceUsesProduct); }
    bool hasFunctionForm() const { return m_flags.testFlag(HasFunctionForm); }
    void setHasFunctionForm(bool b);

    const JSSourceValuePtr &baseValue() const { return m_baseValue; }
    void setBaseValue(const JSSourceValuePtr &v) { m_baseValue = v; }

    struct Alternative
    {
        QString condition;
        JSSourceValuePtr value;
    };

    const QList<Alternative> &alternatives() const { return m_alternatives; }
    void setAlternatives(const QList<Alternative> &alternatives) { m_alternatives = alternatives; }
    void addAlternative(const Alternative &alternative) { m_alternatives.append(alternative); }

private:
    QStringRef m_sourceCode;
    int m_line;
    int m_column;
    FileContextPtr m_file;
    Flags m_flags;
    JSSourceValuePtr m_baseValue;
    QList<Alternative> m_alternatives;
};

class Item;

class ItemValue : public Value
{
    ItemValue(Item *item);
public:
    static ItemValuePtr create(Item *item = 0);
    ~ItemValue();

    void apply(ValueHandler *handler) { handler->handle(this); }
    ValuePtr clone() const;
    Item *item() const;
    void setItem(Item *ptr);

private:
    Item *m_item;
};

inline Item *ItemValue::item() const
{
    return m_item;
}

inline void ItemValue::setItem(Item *ptr)
{
    m_item = ptr;
}


class VariantValue : public Value
{
    VariantValue(const QVariant &v);
public:
    static VariantValuePtr create(const QVariant &v = QVariant());

    void apply(ValueHandler *handler) { handler->handle(this); }
    ValuePtr clone() const;

    void setValue(const QVariant &v) { m_value = v; }
    const QVariant &value() const { return m_value; }

private:
    QVariant m_value;
};

} // namespace Internal
} // namespace qbs

#endif // QBS_VALUE_H
