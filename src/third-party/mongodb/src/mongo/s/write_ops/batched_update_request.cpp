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
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#include "mongo/s/write_ops/batched_update_request.h"

#include "mongo/db/field_parser.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

using std::auto_ptr;
using std::string;

using mongoutils::str::stream;

const std::string BatchedUpdateRequest::BATCHED_UPDATE_REQUEST = "update";
const BSONField<std::string> BatchedUpdateRequest::collName("update");
const BSONField<std::vector<BatchedUpdateDocument*>> BatchedUpdateRequest::updates("updates");
const BSONField<BSONObj> BatchedUpdateRequest::writeConcern("writeConcern");
const BSONField<bool> BatchedUpdateRequest::ordered("ordered", true);
const BSONField<BSONObj> BatchedUpdateRequest::metadata("metadata");

BatchedUpdateRequest::BatchedUpdateRequest() {
    clear();
}

BatchedUpdateRequest::~BatchedUpdateRequest() {
    unsetUpdates();
}

bool BatchedUpdateRequest::isValid(std::string* errMsg) const {
    std::string dummy;
    if (errMsg == NULL) {
        errMsg = &dummy;
    }

    // All the mandatory fields must be present.
    if (!_isCollNameSet) {
        *errMsg = stream() << "missing " << collName.name() << " field";
        return false;
    }

    if (!_isUpdatesSet) {
        *errMsg = stream() << "missing " << updates.name() << " field";
        return false;
    }

    return true;
}

BSONObj BatchedUpdateRequest::toBSON() const {
    BSONObjBuilder builder;

    if (_isCollNameSet)
        builder.append(collName(), _collName);

    if (_isUpdatesSet) {
        BSONArrayBuilder updatesBuilder(builder.subarrayStart(updates()));
        for (std::vector<BatchedUpdateDocument*>::const_iterator it = _updates.begin();
             it != _updates.end();
             ++it) {
            BSONObj updateDocument = (*it)->toBSON();
            updatesBuilder.append(updateDocument);
        }
        updatesBuilder.done();
    }

    if (_isWriteConcernSet)
        builder.append(writeConcern(), _writeConcern);

    if (_isOrderedSet)
        builder.append(ordered(), _ordered);

    if (_metadata)
        builder.append(metadata(), _metadata->toBSON());

    return builder.obj();
}

bool BatchedUpdateRequest::parseBSON(const BSONObj& source, string* errMsg) {
    clear();

    std::string dummy;
    if (!errMsg)
        errMsg = &dummy;

    FieldParser::FieldState fieldState;

    BSONObjIterator it(source);
    while (it.more()) {
        const BSONElement& elem = it.next();
        StringData fieldName = elem.fieldNameStringData();

        if (fieldName == collName.name()) {
            std::string collNameTemp;
            fieldState = FieldParser::extract(elem, collName, &collNameTemp, errMsg);
            if (fieldState == FieldParser::FIELD_INVALID)
                return false;
            _collName = NamespaceString(collNameTemp);
            _isCollNameSet = fieldState == FieldParser::FIELD_SET;
        } else if (fieldName == updates.name()) {
            fieldState = FieldParser::extract(elem, updates, &_updates, errMsg);
            if (fieldState == FieldParser::FIELD_INVALID)
                return false;
            _isUpdatesSet = fieldState == FieldParser::FIELD_SET;
        } else if (fieldName == writeConcern.name()) {
            fieldState = FieldParser::extract(elem, writeConcern, &_writeConcern, errMsg);
            if (fieldState == FieldParser::FIELD_INVALID)
                return false;
            _isWriteConcernSet = fieldState == FieldParser::FIELD_SET;
        } else if (fieldName == ordered.name()) {
            fieldState = FieldParser::extract(elem, ordered, &_ordered, errMsg);
            if (fieldState == FieldParser::FIELD_INVALID)
                return false;
            _isOrderedSet = fieldState == FieldParser::FIELD_SET;
        } else if (fieldName == metadata.name()) {
            BSONObj metadataObj;
            fieldState = FieldParser::extract(elem, metadata, &metadataObj, errMsg);
            if (fieldState == FieldParser::FIELD_INVALID)
                return false;

            if (!metadataObj.isEmpty()) {
                _metadata.reset(new BatchedRequestMetadata());
                if (!_metadata->parseBSON(metadataObj, errMsg)) {
                    return false;
                }
            }
        }
    }
    return true;
}

void BatchedUpdateRequest::clear() {
    _collName = NamespaceString();
    _isCollNameSet = false;

    unsetUpdates();

    _writeConcern = BSONObj();
    _isWriteConcernSet = false;

    _ordered = false;
    _isOrderedSet = false;

    _metadata.reset();
}

void BatchedUpdateRequest::cloneTo(BatchedUpdateRequest* other) const {
    other->clear();

    other->_collName = _collName;
    other->_isCollNameSet = _isCollNameSet;

    for (std::vector<BatchedUpdateDocument*>::const_iterator it = _updates.begin();
         it != _updates.end();
         ++it) {
        auto_ptr<BatchedUpdateDocument> tempBatchUpdateDocument(new BatchedUpdateDocument);
        (*it)->cloneTo(tempBatchUpdateDocument.get());
        other->addToUpdates(tempBatchUpdateDocument.release());
    }
    other->_isUpdatesSet = _isUpdatesSet;

    other->_writeConcern = _writeConcern;
    other->_isWriteConcernSet = _isWriteConcernSet;

    other->_ordered = _ordered;
    other->_isOrderedSet = _isOrderedSet;

    if (_metadata) {
        other->_metadata.reset(new BatchedRequestMetadata());
        _metadata->cloneTo(other->_metadata.get());
    }
}

std::string BatchedUpdateRequest::toString() const {
    return toBSON().toString();
}

void BatchedUpdateRequest::setCollName(const StringData& collName) {
    _collName = NamespaceString(collName);
    _isCollNameSet = true;
}

const std::string& BatchedUpdateRequest::getCollName() const {
    dassert(_isCollNameSet);
    return _collName.ns();
}

void BatchedUpdateRequest::setCollNameNS(const NamespaceString& collName) {
    _collName = collName;
    _isCollNameSet = true;
}

const NamespaceString& BatchedUpdateRequest::getCollNameNS() const {
    dassert(_isCollNameSet);
    return _collName;
}

const NamespaceString& BatchedUpdateRequest::getTargetingNSS() const {
    return getCollNameNS();
}

void BatchedUpdateRequest::setUpdates(const std::vector<BatchedUpdateDocument*>& updates) {
    unsetUpdates();
    for (std::vector<BatchedUpdateDocument*>::const_iterator it = updates.begin();
         it != updates.end();
         ++it) {
        auto_ptr<BatchedUpdateDocument> tempBatchUpdateDocument(new BatchedUpdateDocument);
        (*it)->cloneTo(tempBatchUpdateDocument.get());
        addToUpdates(tempBatchUpdateDocument.release());
    }
    _isUpdatesSet = updates.size() > 0;
}

void BatchedUpdateRequest::addToUpdates(BatchedUpdateDocument* updates) {
    _updates.push_back(updates);
    _isUpdatesSet = true;
}

void BatchedUpdateRequest::unsetUpdates() {
    for (std::vector<BatchedUpdateDocument*>::iterator it = _updates.begin(); it != _updates.end();
         ++it) {
        delete *it;
    }
    _updates.clear();
    _isUpdatesSet = false;
}

bool BatchedUpdateRequest::isUpdatesSet() const {
    return _isUpdatesSet;
}

size_t BatchedUpdateRequest::sizeUpdates() const {
    return _updates.size();
}

const std::vector<BatchedUpdateDocument*>& BatchedUpdateRequest::getUpdates() const {
    dassert(_isUpdatesSet);
    return _updates;
}

const BatchedUpdateDocument* BatchedUpdateRequest::getUpdatesAt(size_t pos) const {
    dassert(_isUpdatesSet);
    dassert(_updates.size() > pos);
    return _updates.at(pos);
}

void BatchedUpdateRequest::setWriteConcern(const BSONObj& writeConcern) {
    _writeConcern = writeConcern.getOwned();
    _isWriteConcernSet = true;
}

void BatchedUpdateRequest::unsetWriteConcern() {
    _isWriteConcernSet = false;
}

bool BatchedUpdateRequest::isWriteConcernSet() const {
    return _isWriteConcernSet;
}

const BSONObj& BatchedUpdateRequest::getWriteConcern() const {
    dassert(_isWriteConcernSet);
    return _writeConcern;
}

void BatchedUpdateRequest::setOrdered(bool ordered) {
    _ordered = ordered;
    _isOrderedSet = true;
}

void BatchedUpdateRequest::unsetOrdered() {
    _isOrderedSet = false;
}

bool BatchedUpdateRequest::isOrderedSet() const {
    return _isOrderedSet;
}

bool BatchedUpdateRequest::getOrdered() const {
    if (_isOrderedSet) {
        return _ordered;
    } else {
        return ordered.getDefault();
    }
}

void BatchedUpdateRequest::setMetadata(BatchedRequestMetadata* metadata) {
    _metadata.reset(metadata);
}

void BatchedUpdateRequest::unsetMetadata() {
    _metadata.reset();
}

bool BatchedUpdateRequest::isMetadataSet() const {
    return _metadata.get();
}

BatchedRequestMetadata* BatchedUpdateRequest::getMetadata() const {
    return _metadata.get();
}

}  // namespace mongo
