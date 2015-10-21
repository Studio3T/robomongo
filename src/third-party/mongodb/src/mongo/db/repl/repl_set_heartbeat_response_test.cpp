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

#include <boost/scoped_ptr.hpp>

#include "mongo/unittest/unittest.h"
#include "mongo/util/assert_util.h"
#include "mongo/db/repl/repl_set_heartbeat_response.h"

namespace mongo {
namespace repl {
namespace {

using boost::scoped_ptr;
using std::auto_ptr;

bool stringContains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

TEST(ReplSetHeartbeatResponse, DefaultConstructThenSlowlyBuildToFullObj) {
    int fieldsSet = 2;
    ReplSetHeartbeatResponse hbResponse;
    ReplSetHeartbeatResponse hbResponseObjRoundTripChecker;
    ASSERT_EQUALS(false, hbResponse.hasState());
    ASSERT_EQUALS(false, hbResponse.hasElectionTime());
    ASSERT_EQUALS(false, hbResponse.hasIsElectable());
    ASSERT_EQUALS(false, hbResponse.hasTime());
    ASSERT_EQUALS(false, hbResponse.hasOpTime());
    ASSERT_EQUALS(false, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(false, hbResponse.isReplSet());
    ASSERT_EQUALS(false, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("", hbResponse.getReplicaSetName());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("", hbResponse.getSyncingTo());
    ASSERT_EQUALS(-1, hbResponse.getVersion());

    BSONObj hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());

    Status initializeResult = Status::OK();
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set version
    hbResponse.setVersion(1);
    ++fieldsSet;
    ASSERT_EQUALS(false, hbResponse.hasState());
    ASSERT_EQUALS(false, hbResponse.hasElectionTime());
    ASSERT_EQUALS(false, hbResponse.hasIsElectable());
    ASSERT_EQUALS(false, hbResponse.hasTime());
    ASSERT_EQUALS(false, hbResponse.hasOpTime());
    ASSERT_EQUALS(false, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(false, hbResponse.isReplSet());
    ASSERT_EQUALS(false, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("", hbResponse.getReplicaSetName());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set setname
    hbResponse.setSetName("rs0");
    ++fieldsSet;
    ASSERT_EQUALS(false, hbResponse.hasState());
    ASSERT_EQUALS(false, hbResponse.hasElectionTime());
    ASSERT_EQUALS(false, hbResponse.hasIsElectable());
    ASSERT_EQUALS(false, hbResponse.hasTime());
    ASSERT_EQUALS(false, hbResponse.hasOpTime());
    ASSERT_EQUALS(false, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(false, hbResponse.isReplSet());
    ASSERT_EQUALS(false, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("rs0", hbResponseObj["set"].String());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set electionTime
    hbResponse.setElectionTime(OpTime(10, 0));
    ++fieldsSet;
    ASSERT_EQUALS(false, hbResponse.hasState());
    ASSERT_EQUALS(true, hbResponse.hasElectionTime());
    ASSERT_EQUALS(false, hbResponse.hasIsElectable());
    ASSERT_EQUALS(false, hbResponse.hasTime());
    ASSERT_EQUALS(false, hbResponse.hasOpTime());
    ASSERT_EQUALS(false, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(false, hbResponse.isReplSet());
    ASSERT_EQUALS(false, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());
    ASSERT_EQUALS(OpTime(10, 0), hbResponse.getElectionTime());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("rs0", hbResponseObj["set"].String());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());
    ASSERT_EQUALS(OpTime(10, 0), hbResponseObj["electionTime"]._opTime());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set opTime
    hbResponse.setOpTime(Date_t(10));
    ++fieldsSet;
    ASSERT_EQUALS(false, hbResponse.hasState());
    ASSERT_EQUALS(true, hbResponse.hasElectionTime());
    ASSERT_EQUALS(false, hbResponse.hasIsElectable());
    ASSERT_EQUALS(false, hbResponse.hasTime());
    ASSERT_EQUALS(true, hbResponse.hasOpTime());
    ASSERT_EQUALS(false, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(false, hbResponse.isReplSet());
    ASSERT_EQUALS(false, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());
    ASSERT_EQUALS(OpTime(10, 0), hbResponse.getElectionTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponse.getOpTime());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("rs0", hbResponseObj["set"].String());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());
    ASSERT_EQUALS(OpTime(10, 0), hbResponseObj["electionTime"]._opTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponseObj["opTime"]._opTime());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set time
    hbResponse.setTime(Seconds(10));
    ++fieldsSet;
    ASSERT_EQUALS(false, hbResponse.hasState());
    ASSERT_EQUALS(true, hbResponse.hasElectionTime());
    ASSERT_EQUALS(false, hbResponse.hasIsElectable());
    ASSERT_EQUALS(true, hbResponse.hasTime());
    ASSERT_EQUALS(true, hbResponse.hasOpTime());
    ASSERT_EQUALS(false, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(false, hbResponse.isReplSet());
    ASSERT_EQUALS(false, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());
    ASSERT_EQUALS(OpTime(10, 0), hbResponse.getElectionTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponse.getOpTime());
    ASSERT_EQUALS(10, hbResponse.getTime().total_seconds());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("rs0", hbResponseObj["set"].String());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());
    ASSERT_EQUALS(OpTime(10, 0), hbResponseObj["electionTime"]._opTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponseObj["opTime"]._opTime());
    ASSERT_EQUALS(10, hbResponseObj["time"].numberLong());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set electable
    hbResponse.setElectable(true);
    ++fieldsSet;
    ASSERT_EQUALS(false, hbResponse.hasState());
    ASSERT_EQUALS(true, hbResponse.hasElectionTime());
    ASSERT_EQUALS(true, hbResponse.hasIsElectable());
    ASSERT_EQUALS(true, hbResponse.hasTime());
    ASSERT_EQUALS(true, hbResponse.hasOpTime());
    ASSERT_EQUALS(false, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(false, hbResponse.isReplSet());
    ASSERT_EQUALS(false, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());
    ASSERT_EQUALS(OpTime(10, 0), hbResponse.getElectionTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponse.getOpTime());
    ASSERT_EQUALS(10, hbResponse.getTime().total_seconds());
    ASSERT_EQUALS(true, hbResponse.isElectable());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("rs0", hbResponseObj["set"].String());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());
    ASSERT_EQUALS(OpTime(10, 0), hbResponseObj["electionTime"]._opTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponseObj["opTime"]._opTime());
    ASSERT_EQUALS(10, hbResponseObj["time"].numberLong());
    ASSERT_EQUALS(true, hbResponseObj["e"].trueValue());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set config
    ReplicaSetConfig config;
    hbResponse.setConfig(config);
    ++fieldsSet;
    ASSERT_EQUALS(false, hbResponse.hasState());
    ASSERT_EQUALS(true, hbResponse.hasElectionTime());
    ASSERT_EQUALS(true, hbResponse.hasIsElectable());
    ASSERT_EQUALS(true, hbResponse.hasTime());
    ASSERT_EQUALS(true, hbResponse.hasOpTime());
    ASSERT_EQUALS(true, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(false, hbResponse.isReplSet());
    ASSERT_EQUALS(false, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());
    ASSERT_EQUALS(OpTime(10, 0), hbResponse.getElectionTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponse.getOpTime());
    ASSERT_EQUALS(10, hbResponse.getTime().total_seconds());
    ASSERT_EQUALS(true, hbResponse.isElectable());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponse.getConfig().toBSON().toString());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("rs0", hbResponseObj["set"].String());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());
    ASSERT_EQUALS(OpTime(10, 0), hbResponseObj["electionTime"]._opTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponseObj["opTime"]._opTime());
    ASSERT_EQUALS(10, hbResponseObj["time"].numberLong());
    ASSERT_EQUALS(true, hbResponseObj["e"].trueValue());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponseObj["config"].Obj().toString());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set state
    hbResponse.setState(MemberState(MemberState::RS_SECONDARY));
    ++fieldsSet;
    ASSERT_EQUALS(true, hbResponse.hasState());
    ASSERT_EQUALS(true, hbResponse.hasElectionTime());
    ASSERT_EQUALS(true, hbResponse.hasIsElectable());
    ASSERT_EQUALS(true, hbResponse.hasTime());
    ASSERT_EQUALS(true, hbResponse.hasOpTime());
    ASSERT_EQUALS(true, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(false, hbResponse.isReplSet());
    ASSERT_EQUALS(false, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS(MemberState(MemberState::RS_SECONDARY).toString(),
                  hbResponse.getState().toString());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());
    ASSERT_EQUALS(OpTime(10, 0), hbResponse.getElectionTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponse.getOpTime());
    ASSERT_EQUALS(10, hbResponse.getTime().total_seconds());
    ASSERT_EQUALS(true, hbResponse.isElectable());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponse.getConfig().toBSON().toString());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("rs0", hbResponseObj["set"].String());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());
    ASSERT_EQUALS(OpTime(10, 0), hbResponseObj["electionTime"]._opTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponseObj["opTime"]._opTime());
    ASSERT_EQUALS(10, hbResponseObj["time"].numberLong());
    ASSERT_EQUALS(true, hbResponseObj["e"].trueValue());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponseObj["config"].Obj().toString());
    ASSERT_EQUALS(2, hbResponseObj["state"].numberLong());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set stateDisagreement
    hbResponse.noteStateDisagreement();
    ++fieldsSet;
    ASSERT_EQUALS(true, hbResponse.hasState());
    ASSERT_EQUALS(true, hbResponse.hasElectionTime());
    ASSERT_EQUALS(true, hbResponse.hasIsElectable());
    ASSERT_EQUALS(true, hbResponse.hasTime());
    ASSERT_EQUALS(true, hbResponse.hasOpTime());
    ASSERT_EQUALS(true, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(false, hbResponse.isReplSet());
    ASSERT_EQUALS(true, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS(MemberState(MemberState::RS_SECONDARY).toString(),
                  hbResponse.getState().toString());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());
    ASSERT_EQUALS(OpTime(10, 0), hbResponse.getElectionTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponse.getOpTime());
    ASSERT_EQUALS(10, hbResponse.getTime().total_seconds());
    ASSERT_EQUALS(true, hbResponse.isElectable());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponse.getConfig().toBSON().toString());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("rs0", hbResponseObj["set"].String());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());
    ASSERT_EQUALS(OpTime(10, 0), hbResponseObj["electionTime"]._opTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponseObj["opTime"]._opTime());
    ASSERT_EQUALS(10, hbResponseObj["time"].numberLong());
    ASSERT_EQUALS(true, hbResponseObj["e"].trueValue());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponseObj["config"].Obj().toString());
    ASSERT_EQUALS(2, hbResponseObj["state"].numberLong());
    ASSERT_EQUALS(false, hbResponseObj["mismatch"].trueValue());
    ASSERT_EQUALS(true, hbResponseObj["stateDisagreement"].trueValue());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set replSet
    hbResponse.noteReplSet();
    ++fieldsSet;
    ASSERT_EQUALS(true, hbResponse.hasState());
    ASSERT_EQUALS(true, hbResponse.hasElectionTime());
    ASSERT_EQUALS(true, hbResponse.hasIsElectable());
    ASSERT_EQUALS(true, hbResponse.hasTime());
    ASSERT_EQUALS(true, hbResponse.hasOpTime());
    ASSERT_EQUALS(true, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(true, hbResponse.isReplSet());
    ASSERT_EQUALS(true, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS(MemberState(MemberState::RS_SECONDARY).toString(),
                  hbResponse.getState().toString());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());
    ASSERT_EQUALS(OpTime(10, 0), hbResponse.getElectionTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponse.getOpTime());
    ASSERT_EQUALS(10, hbResponse.getTime().total_seconds());
    ASSERT_EQUALS(true, hbResponse.isElectable());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponse.getConfig().toBSON().toString());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("rs0", hbResponseObj["set"].String());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());
    ASSERT_EQUALS(OpTime(10, 0), hbResponseObj["electionTime"]._opTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponseObj["opTime"]._opTime());
    ASSERT_EQUALS(10, hbResponseObj["time"].numberLong());
    ASSERT_EQUALS(true, hbResponseObj["e"].trueValue());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponseObj["config"].Obj().toString());
    ASSERT_EQUALS(2, hbResponseObj["state"].numberLong());
    ASSERT_EQUALS(false, hbResponseObj["mismatch"].trueValue());
    ASSERT_EQUALS(true, hbResponseObj["stateDisagreement"].trueValue());
    ASSERT_EQUALS(true, hbResponseObj["rs"].trueValue());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set syncingTo
    hbResponse.setSyncingTo("syncTarget");
    ++fieldsSet;
    ASSERT_EQUALS(true, hbResponse.hasState());
    ASSERT_EQUALS(true, hbResponse.hasElectionTime());
    ASSERT_EQUALS(true, hbResponse.hasIsElectable());
    ASSERT_EQUALS(true, hbResponse.hasTime());
    ASSERT_EQUALS(true, hbResponse.hasOpTime());
    ASSERT_EQUALS(true, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(true, hbResponse.isReplSet());
    ASSERT_EQUALS(true, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS(MemberState(MemberState::RS_SECONDARY).toString(),
                  hbResponse.getState().toString());
    ASSERT_EQUALS("", hbResponse.getHbMsg());
    ASSERT_EQUALS("syncTarget", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());
    ASSERT_EQUALS(OpTime(10, 0), hbResponse.getElectionTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponse.getOpTime());
    ASSERT_EQUALS(10, hbResponse.getTime().total_seconds());
    ASSERT_EQUALS(true, hbResponse.isElectable());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponse.getConfig().toBSON().toString());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("rs0", hbResponseObj["set"].String());
    ASSERT_EQUALS("", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());
    ASSERT_EQUALS(OpTime(10, 0), hbResponseObj["electionTime"]._opTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponseObj["opTime"]._opTime());
    ASSERT_EQUALS(10, hbResponseObj["time"].numberLong());
    ASSERT_EQUALS(true, hbResponseObj["e"].trueValue());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponseObj["config"].Obj().toString());
    ASSERT_EQUALS(2, hbResponseObj["state"].numberLong());
    ASSERT_EQUALS(false, hbResponseObj["mismatch"].trueValue());
    ASSERT_EQUALS(true, hbResponseObj["stateDisagreement"].trueValue());
    ASSERT_EQUALS(true, hbResponseObj["rs"].trueValue());
    ASSERT_EQUALS("syncTarget", hbResponseObj["syncingTo"].String());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set hbmsg
    hbResponse.setHbMsg("lub dub");
    ASSERT_EQUALS(true, hbResponse.hasState());
    ASSERT_EQUALS(true, hbResponse.hasElectionTime());
    ASSERT_EQUALS(true, hbResponse.hasIsElectable());
    ASSERT_EQUALS(true, hbResponse.hasTime());
    ASSERT_EQUALS(true, hbResponse.hasOpTime());
    ASSERT_EQUALS(true, hbResponse.hasConfig());
    ASSERT_EQUALS(false, hbResponse.isMismatched());
    ASSERT_EQUALS(true, hbResponse.isReplSet());
    ASSERT_EQUALS(true, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS(MemberState(MemberState::RS_SECONDARY).toString(),
                  hbResponse.getState().toString());
    ASSERT_EQUALS("lub dub", hbResponse.getHbMsg());
    ASSERT_EQUALS("syncTarget", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());
    ASSERT_EQUALS(OpTime(10, 0), hbResponse.getElectionTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponse.getOpTime());
    ASSERT_EQUALS(10, hbResponse.getTime().total_seconds());
    ASSERT_EQUALS(true, hbResponse.isElectable());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponse.getConfig().toBSON().toString());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(fieldsSet, hbResponseObj.nFields());
    ASSERT_EQUALS("rs0", hbResponseObj["set"].String());
    ASSERT_EQUALS("lub dub", hbResponseObj["hbmsg"].String());
    ASSERT_EQUALS(1, hbResponseObj["v"].Number());
    ASSERT_EQUALS(OpTime(10, 0), hbResponseObj["electionTime"]._opTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponseObj["opTime"]._opTime());
    ASSERT_EQUALS(10, hbResponseObj["time"].numberLong());
    ASSERT_EQUALS(true, hbResponseObj["e"].trueValue());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponseObj["config"].Obj().toString());
    ASSERT_EQUALS(2, hbResponseObj["state"].numberLong());
    ASSERT_EQUALS(false, hbResponseObj["mismatch"].trueValue());
    ASSERT_EQUALS(true, hbResponseObj["stateDisagreement"].trueValue());
    ASSERT_EQUALS(true, hbResponseObj["rs"].trueValue());
    ASSERT_EQUALS("syncTarget", hbResponseObj["syncingTo"].String());

    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(hbResponseObj.toString(), hbResponseObjRoundTripChecker.toBSON().toString());

    // set mismatched
    hbResponse.noteMismatched();
    ASSERT_EQUALS(true, hbResponse.hasState());
    ASSERT_EQUALS(true, hbResponse.hasElectionTime());
    ASSERT_EQUALS(true, hbResponse.hasIsElectable());
    ASSERT_EQUALS(true, hbResponse.hasTime());
    ASSERT_EQUALS(true, hbResponse.hasOpTime());
    ASSERT_EQUALS(true, hbResponse.hasConfig());
    ASSERT_EQUALS(true, hbResponse.isMismatched());
    ASSERT_EQUALS(true, hbResponse.isReplSet());
    ASSERT_EQUALS(true, hbResponse.isStateDisagreement());
    ASSERT_EQUALS("rs0", hbResponse.getReplicaSetName());
    ASSERT_EQUALS(MemberState(MemberState::RS_SECONDARY).toString(),
                  hbResponse.getState().toString());
    ASSERT_EQUALS("lub dub", hbResponse.getHbMsg());
    ASSERT_EQUALS("syncTarget", hbResponse.getSyncingTo());
    ASSERT_EQUALS(1, hbResponse.getVersion());
    ASSERT_EQUALS(OpTime(10, 0), hbResponse.getElectionTime());
    ASSERT_EQUALS(OpTime(0, 10), hbResponse.getOpTime());
    ASSERT_EQUALS(10, hbResponse.getTime().total_seconds());
    ASSERT_EQUALS(true, hbResponse.isElectable());
    ASSERT_EQUALS(config.toBSON().toString(), hbResponse.getConfig().toBSON().toString());

    hbResponseObj = hbResponse.toBSON();
    ASSERT_EQUALS(2, hbResponseObj.nFields());
    ASSERT_EQUALS(true, hbResponseObj["mismatch"].trueValue());

    // NOTE: Does not check round-trip. Once noteMismached is set the bson will return an error
    //       from initialize parsing.
    initializeResult = hbResponseObjRoundTripChecker.initialize(hbResponseObj);
    ASSERT_NOT_EQUALS(Status::OK(), initializeResult);
    ASSERT_EQUALS(ErrorCodes::InconsistentReplicaSetNames, initializeResult.code());
}

TEST(ReplSetHeartbeatResponse, InitializeWrongElectionTimeType) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj = BSON("ok" << 1.0 << "electionTime"
                                       << "hello");
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::TypeMismatch, result);
    ASSERT_EQUALS(
        "Expected \"electionTime\" field in response to replSetHeartbeat command to "
        "have type Date or Timestamp, but found type String",
        result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeWrongTimeType) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj = BSON("ok" << 1.0 << "time"
                                       << "hello");
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::TypeMismatch, result);
    ASSERT_EQUALS(
        "Expected \"time\" field in response to replSetHeartbeat command to "
        "have a numeric type, but found type String",
        result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeWrongOpTimeType) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj = BSON("ok" << 1.0 << "opTime"
                                       << "hello");
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::TypeMismatch, result);
    ASSERT_EQUALS(
        "Expected \"opTime\" field in response to replSetHeartbeat command to "
        "have type Date or Timestamp, but found type String",
        result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeMemberStateWrongType) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj = BSON("ok" << 1.0 << "state"
                                       << "hello");
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::TypeMismatch, result);
    ASSERT_EQUALS(
        "Expected \"state\" field in response to replSetHeartbeat command to "
        "have type NumberInt or NumberLong, but found type String",
        result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeMemberStateTooLow) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj = BSON("ok" << 1.0 << "state" << -1);
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::BadValue, result);
    ASSERT_EQUALS(
        "Value for \"state\" in response to replSetHeartbeat is out of range; "
        "legal values are non-negative and no more than 10",
        result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeMemberStateTooHigh) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj = BSON("ok" << 1.0 << "state" << 11);
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::BadValue, result);
    ASSERT_EQUALS(
        "Value for \"state\" in response to replSetHeartbeat is out of range; "
        "legal values are non-negative and no more than 10",
        result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeVersionWrongType) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj = BSON("ok" << 1.0 << "v"
                                       << "hello");
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::TypeMismatch, result);
    ASSERT_EQUALS(
        "Expected \"v\" field in response to replSetHeartbeat to "
        "have type NumberInt, but found String",
        result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeReplSetNameWrongType) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj =
        BSON("ok" << 1.0 << "v" << 2 <<  // needs a version to get this far in initialize()
             "set" << 4);
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::TypeMismatch, result);
    ASSERT_EQUALS(
        "Expected \"set\" field in response to replSetHeartbeat to "
        "have type String, but found NumberInt32",
        result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeHeartbeatMeessageWrongType) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj =
        BSON("ok" << 1.0 << "v" << 2 <<  // needs a version to get this far in initialize()
             "hbmsg" << 4);
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::TypeMismatch, result);
    ASSERT_EQUALS(
        "Expected \"hbmsg\" field in response to replSetHeartbeat to "
        "have type String, but found NumberInt32",
        result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeSyncingToWrongType) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj =
        BSON("ok" << 1.0 << "v" << 2 <<  // needs a version to get this far in initialize()
             "syncingTo" << 4);
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::TypeMismatch, result);
    ASSERT_EQUALS(
        "Expected \"syncingTo\" field in response to replSetHeartbeat to "
        "have type String, but found NumberInt32",
        result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeConfigWrongType) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj =
        BSON("ok" << 1.0 << "v" << 2 <<  // needs a version to get this far in initialize()
             "config" << 4);
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::TypeMismatch, result);
    ASSERT_EQUALS(
        "Expected \"config\" in response to replSetHeartbeat to "
        "have type Object, but found NumberInt32",
        result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeBadConfig) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj =
        BSON("ok" << 1.0 << "v" << 2 <<  // needs a version to get this far in initialize()
             "config" << BSON("illegalFieldName" << 2));
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::BadValue, result);
    ASSERT_EQUALS("Unexpected field illegalFieldName in replica set configuration",
                  result.reason());
}

TEST(ReplSetHeartbeatResponse, InitializeBothElectionTimeTypesSameResult) {
    ReplSetHeartbeatResponse hbResponseDate;
    ReplSetHeartbeatResponse hbResponseTimestamp;
    BSONObjBuilder initializerDate;
    BSONObjBuilder initializerTimestamp;
    Date_t electionTime = Date_t(974132);

    initializerDate.append("ok", 1.0);
    initializerDate.append("v", 1);
    initializerDate.appendDate("electionTime", electionTime);
    Status result = hbResponseDate.initialize(initializerDate.obj());
    ASSERT_EQUALS(Status::OK(), result);

    initializerTimestamp.append("ok", 1.0);
    initializerTimestamp.append("v", 1);
    initializerTimestamp.appendTimestamp("electionTime", electionTime);
    result = hbResponseTimestamp.initialize(initializerTimestamp.obj());
    ASSERT_EQUALS(Status::OK(), result);

    ASSERT_EQUALS(hbResponseTimestamp.getElectionTime(), hbResponseTimestamp.getElectionTime());
}

TEST(ReplSetHeartbeatResponse, InitializeBothOpTimeTypesSameResult) {
    ReplSetHeartbeatResponse hbResponseDate;
    ReplSetHeartbeatResponse hbResponseTimestamp;
    BSONObjBuilder initializerDate;
    BSONObjBuilder initializerTimestamp;
    Date_t opTime = Date_t(974132);

    initializerDate.append("ok", 1.0);
    initializerDate.append("v", 1);
    initializerDate.appendDate("opTime", opTime);
    Status result = hbResponseDate.initialize(initializerDate.obj());
    ASSERT_EQUALS(Status::OK(), result);

    initializerTimestamp.append("ok", 1.0);
    initializerTimestamp.append("v", 1);
    initializerTimestamp.appendTimestamp("opTime", opTime);
    result = hbResponseTimestamp.initialize(initializerTimestamp.obj());
    ASSERT_EQUALS(Status::OK(), result);

    ASSERT_EQUALS(hbResponseTimestamp.getOpTime(), hbResponseTimestamp.getOpTime());
}

TEST(ReplSetHeartbeatResponse, NoConfigStillInitializing) {
    ReplSetHeartbeatResponse hbResp;
    std::string msg = "still initializing";
    Status result = hbResp.initialize(BSON("ok" << 1.0 << "rs" << true << "hbmsg" << msg));
    ASSERT_EQUALS(Status::OK(), result);
    ASSERT_EQUALS(true, hbResp.isReplSet());
    ASSERT_EQUALS(msg, hbResp.getHbMsg());
}

TEST(ReplSetHeartbeatResponse, InvalidResponseOpTimeMissesConfigVersion) {
    ReplSetHeartbeatResponse hbResp;
    std::string msg = "still initializing";
    Status result = hbResp.initialize(BSON("ok" << 1.0 << "opTime" << OpTime()));
    ASSERT_EQUALS(ErrorCodes::NoSuchKey, result.code());
    ASSERT_TRUE(stringContains(result.reason(), "\"v\""))
        << result.reason() << " doesn't contain 'v' field required error msg";
}

TEST(ReplSetHeartbeatResponse, MismatchedRepliSetNames) {
    ReplSetHeartbeatResponse hbResponse;
    BSONObj initializerObj = BSON("ok" << 0.0 << "mismatch" << true);
    Status result = hbResponse.initialize(initializerObj);
    ASSERT_EQUALS(ErrorCodes::InconsistentReplicaSetNames, result.code());
}

TEST(ReplSetHeartbeatResponse, AuthFailure) {
    ReplSetHeartbeatResponse hbResp;
    std::string errMsg = "Unauthorized";
    Status result = hbResp.initialize(
        BSON("ok" << 0.0 << "errmsg" << errMsg << "code" << ErrorCodes::Unauthorized));
    ASSERT_EQUALS(ErrorCodes::Unauthorized, result.code());
    ASSERT_EQUALS(errMsg, result.reason());
}

TEST(ReplSetHeartbeatResponse, ServerError) {
    ReplSetHeartbeatResponse hbResp;
    std::string errMsg = "Random Error";
    Status result = hbResp.initialize(BSON("ok" << 0.0 << "errmsg" << errMsg));
    ASSERT_EQUALS(ErrorCodes::UnknownError, result.code());
    ASSERT_EQUALS(errMsg, result.reason());
}

}  // namespace
}  // namespace repl
}  // namespace mongo
