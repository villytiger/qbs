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

#include "value.h"
#include "item.h"

namespace qbs {
namespace Internal {

Value::Value(Type t)
    : m_type(t)
{
}

Value::~Value()
{
}


JSSourceValue::JSSourceValue()
    : Value(JSSourceValueType), m_line(-1), m_column(-1)
{
}

JSSourceValuePtr JSSourceValue::create()
{
    return JSSourceValuePtr(new JSSourceValue);
}

JSSourceValue::~JSSourceValue()
{
}

ValuePtr JSSourceValue::clone() const
{
    return JSSourceValuePtr(new JSSourceValue(*this));
}

QString JSSourceValue::sourceCodeForEvaluation() const
{
    if (!hasFunctionForm())
        return m_sourceCode.toString();

    // rewrite blocks to be able to use return statements in property assignments
    static const QString prefix = "(function()";
    static const QString suffix = ")()";
    return prefix + m_sourceCode.toString() + suffix;
}

void JSSourceValue::setLocation(int line, int column)
{
    m_line = line;
    m_column = column;
}

CodeLocation JSSourceValue::location() const
{
    return CodeLocation(m_file->filePath(), m_line, m_column);
}

void JSSourceValue::setHasFunctionForm(bool b)
{
    if (b)
        m_flags |= HasFunctionForm;
    else
        m_flags &= ~HasFunctionForm;
}


ItemValue::ItemValue(Item *item)
    : Value(ItemValueType)
    , m_item(item)
{
}

ItemValuePtr ItemValue::create(Item *item)
{
    return ItemValuePtr(new ItemValue(item));
}

ItemValue::~ItemValue()
{
}

ValuePtr ItemValue::clone() const
{
    Item *clonedItem = m_item ? m_item->clone(m_item->pool()) : 0;
    return ItemValuePtr(new ItemValue(clonedItem));
}

VariantValue::VariantValue(const QVariant &v)
    : Value(VariantValueType)
    , m_value(v)
{
}

VariantValuePtr VariantValue::create(const QVariant &v)
{
    return VariantValuePtr(new VariantValue(v));
}

ValuePtr VariantValue::clone() const
{
    return VariantValuePtr(new VariantValue(*this));
}

} // namespace Internal
} // namespace qbs
