/**
*    Copyright (C) 2008 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,b
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

#include <deque>

#include <boost/thread/condition.hpp>

#include "mongo/stdx/functional.h"
#include "mongo/util/concurrency/mutex.h"
#include "mongo/util/concurrency/task.h"

namespace mongo {

namespace task {

typedef stdx::function<void()> lam;

/** typical usage is: task::fork( new Server("threadname") ); */
class Server : public Task {
public:
    /** send a message to the port */
    void send(lam);

    Server(const std::string& name) : m("server"), _name(name), rq(false) {}
    virtual ~Server() {}

    /** send message but block until function completes */
    void call(const lam&);

    void requeue() {
        rq = true;
    }

protected:
    // REMINDER : for use in mongod, you will want to have this call Client::initThread().
    virtual void starting() {}

private:
    virtual bool initClient() {
        return true;
    }
    virtual std::string name() const {
        return _name;
    }
    void doWork();
    std::deque<lam> d;
    mongo::mutex m;
    boost::condition c;
    std::string _name;
    bool rq;
};
}
}
