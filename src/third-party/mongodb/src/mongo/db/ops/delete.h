// delete.h

/**
*    Copyright (C) 2008 10gen Inc.
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

#pragma once

#include "mongo/pch.h"
#include "../jsobj.h"
#include "../clientcursor.h"

namespace mongo {

    class RemoveSaver;

    // If justOne is true, deletedId is set to the id of the deleted object.
    long long deleteObjects(const char *ns, BSONObj pattern, bool justOne, bool logop = false, bool god=false, RemoveSaver * rs=0);


}
