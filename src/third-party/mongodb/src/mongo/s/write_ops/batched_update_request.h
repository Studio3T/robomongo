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

#pragma once

#include <boost/scoped_ptr.hpp>
#include <string>
#include <vector>

#include "mongo/base/string_data.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/namespace_string.h"
#include "mongo/s/bson_serializable.h"
#include "mongo/s/chunk_version.h"
#include "mongo/s/write_ops/batched_request_metadata.h"
#include "mongo/s/write_ops/batched_update_document.h"

namespace mongo {

/**
 * This class represents the layout and content of a batched update runCommand,
 * the request side.
 */
class BatchedUpdateRequest : public BSONSerializable {
    MONGO_DISALLOW_COPYING(BatchedUpdateRequest);

public:
    //
    // schema declarations
    //

    // Name used for the batched update invocation.
    static const std::string BATCHED_UPDATE_REQUEST;

    // Field names and types in the batched update command type.
    static const BSONField<std::string> collName;
    static const BSONField<std::vector<BatchedUpdateDocument*>> updates;
    static const BSONField<BSONObj> writeConcern;
    static const BSONField<bool> ordered;
    static const BSONField<BSONObj> metadata;

    //
    // construction / destruction
    //

    BatchedUpdateRequest();
    virtual ~BatchedUpdateRequest();

    /** Copies all the fields present in 'this' to 'other'. */
    void cloneTo(BatchedUpdateRequest* other) const;

    //
    // bson serializable interface implementation
    //

    virtual bool isValid(std::string* errMsg) const;
    virtual BSONObj toBSON() const;
    virtual bool parseBSON(const BSONObj& source, std::string* errMsg);
    virtual void clear();
    virtual std::string toString() const;

    //
    // individual field accessors
    //

    void setCollName(const StringData& collName);
    void setCollNameNS(const NamespaceString& collName);
    const std::string& getCollName() const;
    const NamespaceString& getCollNameNS() const;

    const NamespaceString& getTargetingNSS() const;

    void setUpdates(const std::vector<BatchedUpdateDocument*>& updates);

    /**
     * updates ownership is transferred to here.
     */
    void addToUpdates(BatchedUpdateDocument* updates);
    void unsetUpdates();
    bool isUpdatesSet() const;
    std::size_t sizeUpdates() const;
    const std::vector<BatchedUpdateDocument*>& getUpdates() const;
    const BatchedUpdateDocument* getUpdatesAt(std::size_t pos) const;

    void setWriteConcern(const BSONObj& writeConcern);
    void unsetWriteConcern();
    bool isWriteConcernSet() const;
    const BSONObj& getWriteConcern() const;

    void setOrdered(bool ordered);
    void unsetOrdered();
    bool isOrderedSet() const;
    bool getOrdered() const;

    /*
    * metadata ownership will be transferred to this.
    */
    void setMetadata(BatchedRequestMetadata* metadata);
    void unsetMetadata();
    bool isMetadataSet() const;
    BatchedRequestMetadata* getMetadata() const;

private:
    // Convention: (M)andatory, (O)ptional

    // (M)  collection we're updating from
    NamespaceString _collName;
    bool _isCollNameSet;

    // (M)  array of individual updates
    std::vector<BatchedUpdateDocument*> _updates;
    bool _isUpdatesSet;

    // (O)  to be issued after the batch applied
    BSONObj _writeConcern;
    bool _isWriteConcernSet;

    // (O)  whether batch is issued in parallel or not
    bool _ordered;
    bool _isOrderedSet;

    // (O)  metadata associated with this request for internal use.
    boost::scoped_ptr<BatchedRequestMetadata> _metadata;
};

}  // namespace mongo
