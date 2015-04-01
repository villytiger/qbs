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

#ifndef QBS_NODESET_H
#define QBS_NODESET_H

#include <set>
#include <cstddef>

namespace qbs {
namespace Internal {

class BuildGraphNode;
class PersistentPool;

/**
  * Set of build graph nodes.
  * This is faster than QSet when iterating over the container.
  */
class NodeSet
{
public:
    NodeSet();
    NodeSet(const NodeSet &other);

    NodeSet &unite(const NodeSet &other);

    typedef std::set<BuildGraphNode *>::const_iterator const_iterator;
    typedef std::set<BuildGraphNode *>::iterator iterator;
    typedef BuildGraphNode * value_type;

    iterator begin() { return m_data.begin(); }
    iterator end() { return m_data.end(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator end() const { return m_data.end(); }
    const_iterator constBegin() const { return m_data.begin(); }
    const_iterator constEnd() const { return m_data.end(); }

    void insert(BuildGraphNode *node)
    {
        m_data.insert(node);
    }

    void operator+=(BuildGraphNode *node)
    {
        insert(node);
    }

    NodeSet &operator<<(BuildGraphNode *node)
    {
        insert(node);
        return *this;
    }

    void remove(BuildGraphNode *node);

    bool contains(BuildGraphNode *node) const
    {
        return m_data.find(node) != m_data.end();
    }

    void clear()
    {
        m_data.clear();
    }

    bool isEmpty() const
    {
        return m_data.empty();
    }

    int count() const
    {
        return (int)m_data.size();
    }

    void reserve(int)
    {
        // no-op
    }

    bool operator==(const NodeSet &other) const { return m_data == other.m_data; }
    bool operator!=(const NodeSet &other) const { return !(*this == other); }

    void load(PersistentPool &pool);
    void store(PersistentPool &pool) const;


private:
    std::set<BuildGraphNode *> m_data;
};

} // namespace Internal
} // namespace qbs

#endif // QBS_NODESET_H
