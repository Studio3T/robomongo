#!/bin/bash

# TRANSLATOR comment, which would be help lupdate with namespaces doesn't works for me... 
# So...

declare -A mapping
mapping[MainWindow]=Robomongo
mapping[SettingsManager]=Robomongo
mapping[EditIndexDialog]=Robomongo
mapping[FindFrame]=Robomongo
mapping[AboutDialog]=Robomongo
mapping[ConnectionsDialog]=Robomongo
mapping[ConnectionAuthTab]=Robomongo
mapping[ConnectionBasicTab]=Robomongo
mapping[ConnectionAdvancedTab]=Robomongo
mapping[ConnectionDiagnosticDialog]=Robomongo
mapping[ConnectionDialog]=Robomongo
mapping[ConnectionSslTab]=Robomongo
mapping[ConnectionsDialog]=Robomongo
mapping[CopyCollection]=Robomongo
mapping[CreateDatabaseDialog]=Robomongo
mapping[CreateUserDialog]=Robomongo
mapping[DocumentTextEditor]=Robomongo
mapping[FunctionTextEditor]=Robomongo
mapping[SshTunelTab]=Robomongo
mapping[ExplorerServerTreeItem]=Robomongo
mapping[ExplorerDatabaseTreeItem]=Robomongo
mapping[ExplorerCollectionTreeItem]=Robomongo
mapping[ExplorerDatabaseCategoryTreeItem]=Robomongo
mapping[ExplorerFunctionTreeItem]=Robomongo
mapping[ExplorerUserTreeItem]=Robomongo
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
mapping[LogWidget]=Robomongo
mapping[FindFrame]=Robomongo
mapping[Notifier]=Robomongo

for file in `find ./ -name "*.raw.ts"`
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
