#pragma once

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/events/MongoEventsInfo.hpp"

namespace Robomongo
{
    namespace Events
    {
        typedef QtUtils::Event<EventsInfo::RemoveDocumentInfo, QEvent::User+1> RemoveDocumentEvent;
        typedef QtUtils::Event<EventsInfo::SaveDocumentInfo, QEvent::User+2> SaveDocumentEvent;
        typedef QtUtils::Event<EventsInfo::DropFunctionInfo, QEvent::User+3> DropFunctionEvent;
        typedef QtUtils::Event<EventsInfo::CreateFunctionInfo, QEvent::User+4> CreateFunctionEvent;
        typedef QtUtils::Event<EventsInfo::LoadFunctionInfo, QEvent::User+5> LoadFunctionEvent;
        typedef QtUtils::Event<EventsInfo::CreateUserInfo, QEvent::User+6> CreateUserEvent;
        typedef QtUtils::Event<EventsInfo::DropUserInfo, QEvent::User+7> DropUserEvent;
        typedef QtUtils::Event<EventsInfo::LoadUserInfo, QEvent::User+8> LoadUserEvent;
        typedef QtUtils::Event<EventsInfo::CreateCollectionInfo, QEvent::User+9> CreateCollectionEvent;
        typedef QtUtils::Event<EventsInfo::DropCollectionInfo, QEvent::User+10> DropCollectionEvent;
        typedef QtUtils::Event<EventsInfo::RenameCollectionInfo, QEvent::User+11> RenameCollectionEvent;
        typedef QtUtils::Event<EventsInfo::LoadCollectionInfo, QEvent::User+12> LoadCollectionEvent;
        typedef QtUtils::Event<EventsInfo::DuplicateCollectionInfo, QEvent::User+13> DuplicateCollectionEvent;
        typedef QtUtils::Event<EventsInfo::CopyCollectionToDiffServerInfo, QEvent::User+14> CopyCollectionToDiffServerEvent;
        typedef QtUtils::Event<EventsInfo::LoadCollectionIndexesInfo, QEvent::User+15> LoadCollectionIndexEvent;
        typedef QtUtils::Event<EventsInfo::CreateIndexInfo, QEvent::User+16> CreateIndexEvent;
        typedef QtUtils::Event<EventsInfo::DropIndexInfo, QEvent::User+17> DeleteIndexEvent;
        typedef QtUtils::Event<EventsInfo::CreateDataBaseInfo, QEvent::User+18> CreateDataBaseEvent;
        typedef QtUtils::Event<EventsInfo::DropDatabaseInfo, QEvent::User+19> DropDatabaseEvent;
        typedef QtUtils::Event<EventsInfo::LoadDatabaseNamesInfo, QEvent::User+20> LoadDatabaseNamesEvent;
        typedef QtUtils::Event<EventsInfo::AutoCompleteInfo, QEvent::User+21> AutoCompleteEvent;
        typedef QtUtils::Event<EventsInfo::ExecuteQueryInfo, QEvent::User+22> ExecuteQueryEvent;
        typedef QtUtils::Event<EventsInfo::ExecuteScriptInfo, QEvent::User+23> ExecuteScriptEvent;
        typedef QtUtils::Event<EventsInfo::EstablishConnectionInfo, QEvent::User+24> EstablishConnectionEvent;
    }
}
