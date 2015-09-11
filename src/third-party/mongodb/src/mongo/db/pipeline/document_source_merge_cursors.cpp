/**
 * Copyright 2013 (c) 10gen Inc.
 *
 * This program is free software: you can redistribute it and/or  modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, the copyright holders give permission to link the
 * code of portions of this program with the OpenSSL library under certain
 * conditions as described in each individual source file and distribute
 * linked combinations including the program with the OpenSSL library. You
 * must comply with the GNU Affero General Public License in all respects for
 * all of the code used other than as permitted herein. If you modify file(s)
 * with this exception, you may extend this exception to your version of the
 * file(s), but you are not obligated to do so. If you do not wish to do so,
 * delete this exception statement from your version. If you delete this
 * exception statement from all source files in the program, then also delete
 * it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/db/pipeline/document_source.h"

#include <boost/make_shared.hpp>

namespace mongo {

using boost::intrusive_ptr;
using std::make_pair;
using std::string;
using std::vector;

const char DocumentSourceMergeCursors::name[] = "$mergeCursors";

const char* DocumentSourceMergeCursors::getSourceName() const {
    return name;
}

void DocumentSourceMergeCursors::setSource(DocumentSource* pSource) {
    /* this doesn't take a source */
    verify(false);
}

DocumentSourceMergeCursors::DocumentSourceMergeCursors(
    const CursorIds& cursorIds, const intrusive_ptr<ExpressionContext>& pExpCtx)
    : DocumentSource(pExpCtx), _cursorIds(cursorIds), _unstarted(true) {}

intrusive_ptr<DocumentSource> DocumentSourceMergeCursors::create(
    const CursorIds& cursorIds, const intrusive_ptr<ExpressionContext>& pExpCtx) {
    return new DocumentSourceMergeCursors(cursorIds, pExpCtx);
}

intrusive_ptr<DocumentSource> DocumentSourceMergeCursors::createFromBson(
    BSONElement elem, const intrusive_ptr<ExpressionContext>& pExpCtx) {
    massert(17026,
            string("Expected an Array, but got a ") + typeName(elem.type()),
            elem.type() == Array);

    CursorIds cursorIds;
    BSONObj array = elem.embeddedObject();
    BSONForEach(cursor, array) {
        massert(17027,
                string("Expected an Object, but got a ") + typeName(cursor.type()),
                cursor.type() == Object);

        cursorIds.push_back(
            make_pair(ConnectionString(HostAndPort(cursor["host"].String())), cursor["id"].Long()));
    }

    return new DocumentSourceMergeCursors(cursorIds, pExpCtx);
}

Value DocumentSourceMergeCursors::serialize(bool explain) const {
    vector<Value> cursors;
    for (size_t i = 0; i < _cursorIds.size(); i++) {
        cursors.push_back(Value(
            DOC("host" << Value(_cursorIds[i].first.toString()) << "id" << _cursorIds[i].second)));
    }
    return Value(DOC(getSourceName() << Value(cursors)));
}

DocumentSourceMergeCursors::CursorAndConnection::CursorAndConnection(ConnectionString host,
                                                                     NamespaceString ns,
                                                                     CursorId id)
    : connection(host), cursor(connection.get(), ns, id, 0, 0) {}

vector<DBClientCursor*> DocumentSourceMergeCursors::getCursors() {
    verify(_unstarted);
    start();
    vector<DBClientCursor*> out;
    for (Cursors::const_iterator it = _cursors.begin(); it != _cursors.end(); ++it) {
        out.push_back(&((*it)->cursor));
    }

    return out;
}

void DocumentSourceMergeCursors::start() {
    _unstarted = false;

    // open each cursor and send message asking for a batch
    for (CursorIds::const_iterator it = _cursorIds.begin(); it != _cursorIds.end(); ++it) {
        _cursors.push_back(
            boost::make_shared<CursorAndConnection>(it->first, pExpCtx->ns, it->second));
        verify(_cursors.back()->connection->lazySupported());
        _cursors.back()->cursor.initLazy();  // shouldn't block
    }

    // wait for all cursors to return a batch
    // TODO need a way to keep cursors alive if some take longer than 10 minutes.
    for (Cursors::const_iterator it = _cursors.begin(); it != _cursors.end(); ++it) {
        bool retry = false;
        bool ok = (*it)->cursor.initLazyFinish(retry);  // blocks here for first batch

        uassert(
            17028, "error reading response from " + _cursors.back()->connection->toString(), ok);
        verify(!retry);
    }

    _currentCursor = _cursors.begin();
}

Document DocumentSourceMergeCursors::nextSafeFrom(DBClientCursor* cursor) {
    const BSONObj next = cursor->next();
    if (next.hasField("$err")) {
        const int code = next.hasField("code") ? next["code"].numberInt() : 17029;
        uasserted(code,
                  str::stream() << "Received error in response from " << cursor->originalHost()
                                << ": " << next);
    }
    return Document::fromBsonWithMetaData(next);
}

boost::optional<Document> DocumentSourceMergeCursors::getNext() {
    if (_unstarted)
        start();

    // purge eof cursors and release their connections
    while (!_cursors.empty() && !(*_currentCursor)->cursor.more()) {
        (*_currentCursor)->connection.done();
        _cursors.erase(_currentCursor);
        _currentCursor = _cursors.begin();
    }

    if (_cursors.empty())
        return boost::none;

    const Document next = nextSafeFrom(&((*_currentCursor)->cursor));

    // advance _currentCursor, wrapping if needed
    if (++_currentCursor == _cursors.end())
        _currentCursor = _cursors.begin();

    return next;
}

void DocumentSourceMergeCursors::dispose() {
    // Note it is an error to call done() on a connection before consuming the response from a
    // request. Therefore it is an error to call dispose() if there are any outstanding connections
    // which have not received a reply.
    for (auto&& cursorAndConn : _cursors) {
        cursorAndConn->cursor.kill();
        cursorAndConn->connection.done();
    }
    _cursors.clear();
    _currentCursor = _cursors.end();
}
}
