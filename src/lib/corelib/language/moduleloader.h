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

#ifndef QBS_MODULELOADER_H
#define QBS_MODULELOADER_H

#include "forward_decls.h"
#include "itempool.h"
#include <logging/logger.h>
#include <tools/setupprojectparameters.h>
#include <tools/version.h>

#include <QMap>
#include <QSet>
#include <QStringList>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QScriptContext;
class QScriptEngine;
QT_END_NAMESPACE

namespace qbs {

class CodeLocation;

namespace Internal {

class BuiltinDeclarations;
class Evaluator;
class Item;
class ItemReader;
class ProgressObserver;
class ScriptEngine;

struct ModuleLoaderResult
{
    ModuleLoaderResult()
        : itemPool(new ItemPool), root(0)
    {}

    struct ProductInfo
    {
        struct Dependency
        {
            QStringList productTypes;
            QString name;
            QString profile; // "*" <=> Match all profiles.
            bool required;
            bool limitToSubProject;

            QString uniqueName() const;
        };

        QList<Dependency> usedProducts;
        QList<Dependency> usedProductsFromExportItem;
    };

    QSharedPointer<ItemPool> itemPool;
    Item *root;
    QHash<Item *, ProductInfo> productInfos;
    QSet<QString> qbsFiles;
    QVariantMap profileConfigs;
};

/*
 * Loader stage II. Responsible for
 *      - loading modules and module dependencies,
 *      - project references,
 *      - Probe items.
 */
class ModuleLoader
{
public:
    ModuleLoader(ScriptEngine *engine, BuiltinDeclarations *builtins, const Logger &logger);
    ~ModuleLoader();

    void setProgressObserver(ProgressObserver *progressObserver);
    void setSearchPaths(const QStringList &searchPaths);
    Evaluator *evaluator() const { return m_evaluator; }

    ModuleLoaderResult load(const SetupProjectParameters &parameters);

    static QString fullModuleName(const QStringList &moduleName);

private:
    struct ItemCacheValue {
        explicit ItemCacheValue(Item *module = 0, bool enabled = false)
            : module(module), enabled(enabled) {}
        Item *module;
        bool enabled;
    };

    typedef QMap<QPair<QString, QString>, ItemCacheValue> ModuleItemCache;

    class ContextBase
    {
    public:
        ContextBase()
            : item(0), scope(0)
        {}

        Item *item;
        Item *scope;
        QStringList extraSearchPaths;
    };

    class ProjectContext : public ContextBase
    {
    public:
        ModuleLoaderResult *result;
        QString buildDirectory;
        QString localModuleSearchPath;
    };

    class ProductContext : public ContextBase
    {
    public:
        ProjectContext *project;
        ModuleLoaderResult::ProductInfo info;
        QString profileName;
        QSet<FileContextConstPtr> filesWithExportItem;
        QList<Item *> exportItems;
        QVariantMap moduleProperties;
    };

    class DependsContext
    {
    public:
        ProductContext *product;
        QList<ModuleLoaderResult::ProductInfo::Dependency> *productDependencies;
    };

    typedef QPair<Item *, ModuleLoaderResult::ProductInfo::Dependency> ProductDependencyResult;
    typedef QList<ProductDependencyResult> ProductDependencyResults;

    void handleProject(ModuleLoaderResult *loadResult, Item *item, const QString &buildDirectory,
            const QSet<QString> &referencedFilePaths);
    QList<Item *> multiplexProductItem(ProductContext *dummyContext, Item *productItem);
    void handleProduct(ProjectContext *projectContext, Item *item);
    void initProductProperties(const ProjectContext *project, Item *item);
    void handleSubProject(ProjectContext *projectContext, Item *item,
            const QSet<QString> &referencedFilePaths);
    void handleGroup(ProductContext *productContext, Item *group);
    void deferExportItem(ProductContext *productContext, Item *item);
    void mergeExportItems(ProductContext *productContext);
    void propagateModulesFromProduct(ProductContext *productContext, Item *item);
    void resolveDependencies(DependsContext *productContext, Item *item);
    class ItemModuleList;
    void resolveDependsItem(DependsContext *dependsContext, Item *item, Item *dependsItem, ItemModuleList *moduleResults, ProductDependencyResults *productResults);
    Item *moduleInstanceItem(Item *item, const QStringList &moduleName);
    Item *loadModule(ProductContext *productContext, Item *item,
            const CodeLocation &dependsItemLocation, const QString &moduleId,
            const QStringList &moduleName, bool isBaseModule, bool isRequired);
    Item *searchAndLoadModuleFile(ProductContext *productContext,
            const CodeLocation &dependsItemLocation, const QStringList &moduleName,
            const QStringList &extraSearchPaths, bool isRequired, bool *cacheHit);
    Item *loadModuleFile(ProductContext *productContext, const QString &fullModuleName,
            bool isBaseModule, const QString &filePath, bool *cacheHit);
    void loadBaseModule(ProductContext *productContext, Item *item);
    void setupBaseModulePrototype(Item *prototype);
    void instantiateModule(ProductContext *productContext, Item *instanceScope, Item *moduleInstance, Item *modulePrototype, const QStringList &moduleName);
    void createChildInstances(ProductContext *productContext, Item *instance,
                              Item *prototype, QHash<Item *, Item *> *prototypeInstanceMap) const;
    void resolveProbes(Item *item);
    void resolveProbe(Item *parent, Item *probe);
    void checkCancelation() const;
    bool checkItemCondition(Item *item);
    void checkItemTypes(Item *item);
    void callValidateScript(Item *module);
    QStringList readExtraSearchPaths(Item *item, bool *wasSet = 0);
    void copyProperties(const Item *sourceProject, Item *targetProject);
    Item *wrapWithProject(Item *item);
    static QString findExistingModulePath(const QString &searchPath,
            const QStringList &moduleName);
    static void copyProperty(const QString &propertyName, const Item *source, Item *destination);
    static void setScopeForDescendants(Item *item, Item *scope);
    void overrideItemProperties(Item *item, const QString &buildConfigKey,
                                const QVariantMap &buildConfig);

    ScriptEngine *m_engine;
    ItemPool *m_pool;
    Logger m_logger;
    ProgressObserver *m_progressObserver;
    ItemReader *m_reader;
    Evaluator *m_evaluator;
    QStringList m_moduleSearchPaths;
    QMap<QString, QStringList> m_moduleDirListCache;
    ModuleItemCache m_modulePrototypeItemCache;
    QHash<Item *, QSet<QString> > m_validItemPropertyNamesPerItem;
    QSet<Item *> m_disabledItems;
    SetupProjectParameters m_parameters;
    Version m_qbsVersion;
};

} // namespace Internal
} // namespace qbs

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(qbs::Internal::ModuleLoaderResult::ProductInfo, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(qbs::Internal::ModuleLoaderResult::ProductInfo::Dependency, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

#endif // QBS_MODULELOADER_H
