// fts_index_format.h

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
*
*    As a special exception, the copyright holders give permission to link the
*    code of portions of this program with the OpenSSL library under certain
*    conditions as described in each individual source file and distribute
*    linked combinations including the program with the OpenSSL library. You
*    must comply with the GNU Affero General Public License in all respects for
*    all of the code used other than as permitted herein. If you modify file(s)
*    with this exception, you may extend this exception to your version of the
*    file(s), but you are not obligated to do so. If you do not wish to do so,
*    delete this exception statement from your version. If you delete this
*    exception statement from all source files in the program, then also delete
*    it in the license file.
*/

#pragma once

#include "mongo/db/fts/fts_spec.h"

namespace mongo {

namespace fts {

class FTSIndexFormat {
public:
    static void getKeys(const FTSSpec& spec, const BSONObj& document, BSONObjSet* keys);

    /*
     * Helper method to get return entry from the FTSIndex as a BSONObj
     * @param weight, the weight of the term in the entry
     * @param term, the std::string term in the entry
     * @param indexPrefix, the fields that go in the index first
     * @param textIndexVersion, index version. affects key format.
     */
    static BSONObj getIndexKey(double weight,
                               const std::string& term,
                               const BSONObj& indexPrefix,
                               TextIndexVersion textIndexVersion);

private:
    /*
     * Helper method to get return entry from the FTSIndex as a BSONObj
     * @param b, reference to the BSONOBjBuilder
     * @param weight, the weight of the term in the entry
     * @param term, the std::string term in the entry
     * @param textIndexVersion, index version. affects key format.
     */
    static void _appendIndexKey(BSONObjBuilder& b,
                                double weight,
                                const std::string& term,
                                TextIndexVersion textIndexVersion);
};
}
}
