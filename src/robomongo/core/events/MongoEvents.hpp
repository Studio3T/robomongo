#pragma once

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/events/MongoEventsInfo.hpp"

namespace Robomongo
{
    namespace Events
    {
        typedef QtUtils::Event<EventsInfo::RemoveDocumenInfo, QEvent::User+1> RemoveDocumentRequestEvent;
        typedef QtUtils::Event<EventsInfo::RemoveDocumenInfo, QEvent::User+2> RemoveDocumentResponceEvent;

        typedef QtUtils::Event<EventsInfo::SaveDocumentInfo, QEvent::User+3> SaveDocumentRequestEvent;
        typedef QtUtils::Event<EventsInfo::SaveDocumentInfo, QEvent::User+4> SaveDocumentResponceEvent;

        typedef QtUtils::Event<EventsInfo::DropFunctionInfo, QEvent::User+5> DropFunctionRequestEvent;
        typedef QtUtils::Event<EventsInfo::DropFunctionInfo, QEvent::User+6> DropFunctionResponceEvent;

        typedef QtUtils::Event<EventsInfo::CreateFunctionInfo, QEvent::User+7> CreateFunctionRequestEvent;
        typedef QtUtils::Event<EventsInfo::CreateFunctionInfo, QEvent::User+8> CreateFunctionResponceEvent;

        typedef QtUtils::Event<EventsInfo::LoadFunctionRequestInfo, QEvent::User+9> LoadFunctionRequestEvent;
        typedef QtUtils::Event<EventsInfo::LoadFunctionResponceInfo, QEvent::User+10> LoadFunctionResponceEvent;

        typedef QtUtils::Event<EventsInfo::CreateUserInfo, QEvent::User+11> CreateUserRequestEvent;
        typedef QtUtils::Event<EventsInfo::CreateUserInfo, QEvent::User+12> CreateUserResponceEvent;

        typedef QtUtils::Event<EventsInfo::DropUserInfo, QEvent::User+13> DropUserRequestEvent;
        typedef QtUtils::Event<EventsInfo::DropUserInfo, QEvent::User+14> DropUserResponceEvent;

        typedef QtUtils::Event<EventsInfo::LoadUserRequestInfo, QEvent::User+15> LoadUserRequestEvent;
        typedef QtUtils::Event<EventsInfo::LoadUserResponceInfo, QEvent::User+16> LoadUserResponceEvent;

        typedef QtUtils::Event<EventsInfo::CreateCollectionInfo, QEvent::User+17> CreateCollectionRequestEvent;
        typedef QtUtils::Event<EventsInfo::CreateCollectionInfo, QEvent::User+18> CreateCollectionResponceEvent;

        typedef QtUtils::Event<EventsInfo::DropCollectionInfo, QEvent::User+19> DropCollectionRequestEvent;
        typedef QtUtils::Event<EventsInfo::DropCollectionInfo, QEvent::User+20> DropCollectionResponceEvent;

        typedef QtUtils::Event<EventsInfo::RenameCollectionInfo, QEvent::User+21> RenameCollectionRequestEvent;
        typedef QtUtils::Event<EventsInfo::RenameCollectionInfo, QEvent::User+22> RenameCollectionResponceEvent;

        typedef QtUtils::Event<EventsInfo::LoadCollectionRequestInfo, QEvent::User+23> LoadCollectionRequestEvent;
        typedef QtUtils::Event<EventsInfo::LoadCollectionResponceInfo, QEvent::User+24> LoadCollectionResponceEvent;

        typedef QtUtils::Event<EventsInfo::DuplicateCollectionInfo, QEvent::User+25> DuplicateCollectionRequestEvent;
        typedef QtUtils::Event<EventsInfo::DuplicateCollectionInfo, QEvent::User+26> DuplicateCollectionResponceEvent;

        typedef QtUtils::Event<EventsInfo::CopyCollectionToDiffServerInfo, QEvent::User+27> CopyCollectionToDiffServerRequestEvent;
        typedef QtUtils::Event<EventsInfo::CopyCollectionToDiffServerInfo, QEvent::User+28> CopyCollectionToDiffServerResponceEvent;

        typedef QtUtils::Event<EventsInfo::LoadCollectionIndexesRequestInfo, QEvent::User+29> LoadCollectionIndexRequestEvent;
        typedef QtUtils::Event<EventsInfo::LoadCollectionIndexesResponceInfo, QEvent::User+30> LoadCollectionIndexResponceEvent;

        typedef QtUtils::Event<EventsInfo::CreateIndexInfo, QEvent::User+31> CreateIndexRequestEvent;
        typedef QtUtils::Event<EventsInfo::CreateIndexInfo, QEvent::User+32> CreateIndexResponceEvent;

        typedef QtUtils::Event<EventsInfo::DropIndexInfo, QEvent::User+33> DropIndexRequestEvent;
        typedef QtUtils::Event<EventsInfo::DropIndexInfo, QEvent::User+34> DropIndexResponceEvent;

        typedef QtUtils::Event<EventsInfo::CreateDataBaseInfo, QEvent::User+35> CreateDataBaseRequestEvent;
        typedef QtUtils::Event<EventsInfo::CreateDataBaseInfo, QEvent::User+36> CreateDataBaseResponceEvent;

        typedef QtUtils::Event<EventsInfo::DropDatabaseInfo, QEvent::User+37> DropDatabaseRequestEvent;
        typedef QtUtils::Event<EventsInfo::DropDatabaseInfo, QEvent::User+38> DropDatabaseResponceEvent;

        typedef QtUtils::Event<EventsInfo::LoadDatabaseNamesRequestInfo, QEvent::User+39> LoadDatabaseNamesRequestEvent;
        typedef QtUtils::Event<EventsInfo::LoadDatabaseNamesResponceInfo, QEvent::User+40> LoadDatabaseNamesResponceEvent;

        typedef QtUtils::Event<EventsInfo::AutoCompleteRequestInfo, QEvent::User+41> AutoCompleteRequestEvent;
        typedef QtUtils::Event<EventsInfo::AutoCompleteResponceInfo, QEvent::User+42> AutoCompleteResponceEvent;

        typedef QtUtils::Event<EventsInfo::ExecuteQueryRequestInfo, QEvent::User+43> ExecuteQueryRequestEvent;
        typedef QtUtils::Event<EventsInfo::ExecuteQueryResponceInfo, QEvent::User+44> ExecuteQueryResponceEvent;

        typedef QtUtils::Event<EventsInfo::ExecuteScriptRequestInfo, QEvent::User+45> ExecuteScriptRequestEvent;
        typedef QtUtils::Event<EventsInfo::ExecuteScriptResponceInfo, QEvent::User+46> ExecuteScriptResponceEvent;

        typedef QtUtils::Event<EventsInfo::EstablishConnectionRequestInfo, QEvent::User+47> EstablishConnectionRequestEvent;
        typedef QtUtils::Event<EventsInfo::EstablishConnectionResponceInfo, QEvent::User+48> EstablishConnectionResponceEvent;
    }
}
