/**
 *    Copyright (C) 2014 MongoDB Inc.
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

#include "mongo/platform/basic.h"

#include "mongo/db/global_environment_experiment.h"

#include "mongo/bson/bsonobjiterator.h"
#include "mongo/db/operation_context.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

namespace {

GlobalEnvironmentExperiment* globalEnvironmentExperiment = NULL;

}  // namespace

bool hasGlobalEnvironment() {
    return globalEnvironmentExperiment;
}

GlobalEnvironmentExperiment* getGlobalEnvironment() {
    fassert(17508, globalEnvironmentExperiment);
    return globalEnvironmentExperiment;
}

void setGlobalEnvironment(GlobalEnvironmentExperiment* newGlobalEnvironment) {
    fassert(17509, newGlobalEnvironment);

    if (NULL != globalEnvironmentExperiment) {
        delete globalEnvironmentExperiment;
    }

    globalEnvironmentExperiment = newGlobalEnvironment;
}

bool _supportsDocLocking = false;

bool supportsDocLocking() {
    return _supportsDocLocking;
}

bool isMMAPV1() {
    invariant(hasGlobalEnvironment());
    StorageEngine* globalStorageEngine = getGlobalEnvironment()->getGlobalStorageEngine();

    invariant(globalStorageEngine);
    return globalStorageEngine->isMmapV1();
}

Status validateStorageOptions(
    const BSONObj& storageEngineOptions,
    stdx::function<Status(const StorageEngine::Factory* const, const BSONObj&)> validateFunc) {
    BSONObjIterator storageIt(storageEngineOptions);
    while (storageIt.more()) {
        BSONElement storageElement = storageIt.next();
        StringData storageEngineName = storageElement.fieldNameStringData();
        if (storageElement.type() != mongo::Object) {
            return Status(ErrorCodes::BadValue,
                          str::stream() << "'storageEngine." << storageElement.fieldNameStringData()
                                        << "' has to be an embedded document.");
        }

        boost::scoped_ptr<StorageFactoriesIterator> sfi(
            getGlobalEnvironment()->makeStorageFactoriesIterator());
        invariant(sfi);
        bool found = false;
        while (sfi->more()) {
            const StorageEngine::Factory* const& factory = sfi->next();
            if (storageEngineName != factory->getCanonicalName()) {
                continue;
            }
            Status status = validateFunc(factory, storageElement.Obj());
            if (!status.isOK()) {
                return status;
            }
            found = true;
        }
        if (!found) {
            return Status(ErrorCodes::InvalidOptions,
                          str::stream() << storageEngineName
                                        << " is not a registered storage engine for this server");
        }
    }
    return Status::OK();
}

}  // namespace mongo
