// @file pagefault.cpp

/**
*    Copyright (C) 2012 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "pch.h"
#include "diskloc.h"
#include "pagefault.h"
#include "client.h"
#include "pdfile.h"
#include "server.h"

namespace mongo { 

    PageFaultException::PageFaultException(const Record *_r)
    {
        verify( cc().allowedToThrowPageFaultException() );
        cc().getPageFaultRetryableSection()->didLap();
        r = _r;
        era = LockMongoFilesShared::getEra();
        LOG(2) << "PageFaultException thrown" << endl;
    }

    void PageFaultException::touch() { 
        if ( Lock::isLocked() ) {
            warning() << "PageFaultException::touch happening with a lock" << endl;
        }
        LockMongoFilesShared lk;
        if( LockMongoFilesShared::getEra() != era ) {
            // files opened and closed.  we don't try to handle but just bail out; this is much simpler
            // and less error prone and saves us from taking a dbmutex readlock.
            dlog(2) << "era changed" << endl;
            return;
        }
        r->touch();
    }

    PageFaultRetryableSection::~PageFaultRetryableSection() {
        cc()._pageFaultRetryableSection = 0;
    }
    PageFaultRetryableSection::PageFaultRetryableSection() {
        _laps = 0;
        verify( cc()._pageFaultRetryableSection == 0 );
        if( Lock::isLocked() ) {
            cc()._pageFaultRetryableSection = 0;
            if( debug || logLevel > 2 ) { 
                LOGSOME << "info PageFaultRetryableSection will not yield, already locked upon reaching" << endl;
            }
        }
        else {
            cc()._pageFaultRetryableSection = this;
            cc()._hasWrittenThisPass = false;
        }
    }


    NoPageFaultsAllowed::NoPageFaultsAllowed() {
        _saved = cc()._pageFaultRetryableSection;
        cc()._pageFaultRetryableSection = 0;
    }

    NoPageFaultsAllowed::~NoPageFaultsAllowed() {
        cc()._pageFaultRetryableSection = _saved;
    }
}
