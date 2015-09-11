/**
 *    Copyright (C) 2013 10gen Inc.
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

namespace mongo {

class AndCommon {
public:
    /**
     * If src has any data dest doesn't, add that data to dest.
     */
    static void mergeFrom(WorkingSetMember* dest, const WorkingSetMember& src) {
        // Both 'src' and 'dest' must have a RecordId (and they must be the same RecordId), as
        // we should have just matched them according to this RecordId while doing an
        // intersection.
        verify(dest->hasLoc());
        verify(src.hasLoc());
        verify(dest->loc == src.loc);

        // Merge computed data.
        typedef WorkingSetComputedDataType WSCD;
        for (WSCD i = WSCD(0); i < WSM_COMPUTED_NUM_TYPES; i = WSCD(i + 1)) {
            if (!dest->hasComputed(i) && src.hasComputed(i)) {
                dest->addComputed(src.getComputed(i)->clone());
            }
        }

        if (dest->hasObj()) {
            // The merged WSM that we're creating already has the full document, so there's
            // nothing left to do.
            return;
        }

        if (src.hasObj()) {
            // 'src' has the full document but 'dest' doesn't so we need to copy it over.
            dest->obj = src.obj;

            // We have an object so we don't need key data.
            dest->keyData.clear();

            // 'dest' should have the same state as 'src'. If 'src' has an unowned obj, then
            // 'dest' also should have an unowned obj; if 'src' has an owned obj, then dest
            // should also have an owned obj.
            dest->state = src.state;

            // Now 'dest' has the full object. No more work to do.
            return;
        }

        // If we're here, then both WSMs getting merged contain index keys. We need
        // to merge the key data.
        //
        // This is N^2 but N is probably pretty small.  Easy enough to revisit.
        for (size_t i = 0; i < src.keyData.size(); ++i) {
            bool found = false;
            for (size_t j = 0; j < dest->keyData.size(); ++j) {
                if (dest->keyData[j].indexKeyPattern == src.keyData[i].indexKeyPattern) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                dest->keyData.push_back(src.keyData[i]);
            }
        }
    }
};

}  // namespace mongo
