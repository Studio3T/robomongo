#!/bin/bash

# TRANSLATOR comment, which would be help lupdate with namespaces doesn't works for me... 
# So...

declare -A mapping
# core/domain
mapping[Notifier]=Robomongo
# core/mongodb
mapping[MongoWorker]=Robomongo
# core/settings
mapping[SettingsManager]=Robomongo
# gui/
mapping[MainWindow]=Robomongo
# gui/dialogs
mapping[AboutDialog]=Robomongo
mapping[ConnectionAdvancedTab]=Robomongo
mapping[ConnectionAuthTab]=Robomongo
mapping[ConnectionBasicTab]=Robomongo
mapping[ConnectionDiagnosticDialog]=Robomongo
mapping[ConnectionDialog]=Robomongo
mapping[ConnectionSslTab]=Robomongo
mapping[ConnectionsDialog]=Robomongo
mapping[CopyCollectionDialog]=Robomongo
mapping[CreateDatabaseDialog]=Robomongo
mapping[CreateUserDialog]=Robomongo
mapping[DocumentTextEditor]=Robomongo
mapping[FunctionTextEditor]=Robomongo
mapping[PreferencesDialog]=Robomongo
mapping[SshTunelTab]=Robomongo
# gui/editors
mapping[FindFrame]=Robomongo
# gui/widgets
mapping[LogWidget]=Robomongo
# gui/widgets/explorer
mapping[EditIndexDialog]=Robomongo
mapping[ExplorerCollectionTreeItem]=Robomongo
mapping[ExplorerDatabaseCategoryTreeItem]=Robomongo
mapping[ExplorerDatabaseTreeItem]=Robomongo
mapping[ExplorerFunctionTreeItem]=Robomongo
mapping[ExplorerServerTreeItem]=Robomongo
mapping[ExplorerUserTreeItem]=Robomongo
# gui/widgets/workarea
mapping[BsonTreeModel]=Robomongo
mapping[BsonTreeView]=Robomongo
mapping[CollectionStatsTreeWidget]=Robomongo
mapping[OutputItemContentWidget]=Robomongo
mapping[OutputItemHeaderWidget]=Robomongo
mapping[PagingWidget]=Robomongo
mapping[QueryWidget]=Robomongo
mapping[ScriptWidget]=Robomongo
mapping[WorkAreaTabBar]=Robomongo
mapping[WorkAreaTabWidget]=Robomongo

for file in `find ./ -name "robomongo_*.raw.ts"`
do
    right_name=`echo "$file" | sed 's/.raw//g'`
    cp $file $right_name
    for index in ${!mapping[*]}
    do
	sed -i -e "s/<name>$index<\/name>/<name>${mapping[$index]}::$index<\/name>/g" $right_name
    done
    lrelease $right_name
    rm $right_name
done
