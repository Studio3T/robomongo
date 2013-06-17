// isself.cpp

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

#include <string>
#include <vector>

#include "mongo/db/auth/action_set.h"
#include "mongo/db/auth/action_type.h"
#include "mongo/db/auth/authorization_manager.h"
#include "mongo/db/auth/privilege.h"
#include "mongo/db/jsobj.h"
#include "../../util/net/listen.h"
#include "../commands.h"
#include "mongo/util/net/hostandport.h"
#include "mongo/client/dbclientinterface.h"

#include <boost/algorithm/string.hpp>

#ifndef _WIN32
# ifndef __sunos__
#  include <ifaddrs.h>
# endif
# include <sys/resource.h>
# include <sys/stat.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#ifdef __openbsd__
# include <sys/uio.h>
#endif

#endif


namespace mongo {

#if !defined(_WIN32) && !defined(__sunos__)

    vector<string> getMyAddrs() {
        vector<string> out;
        ifaddrs * addrs;
        
        if ( ! cmdLine.bind_ip.empty() ) {
            boost::split( out, cmdLine.bind_ip, boost::is_any_of( ", " ) );
            return out;
        }

        int status = getifaddrs(&addrs);
        massert(13469, "getifaddrs failure: " + errnoWithDescription(errno), status == 0);

        // based on example code from linux getifaddrs manpage
        for (ifaddrs * addr = addrs; addr != NULL; addr = addr->ifa_next) {
            if ( addr->ifa_addr == NULL ) continue;
            int family = addr->ifa_addr->sa_family;
            char host[NI_MAXHOST];

            if (family == AF_INET || family == AF_INET6) {
                status = getnameinfo(addr->ifa_addr,
                                     (family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)),
                                     host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                if ( status != 0 ) {
                    freeifaddrs( addrs );
                    addrs = NULL;
                    msgasserted( 13470, string("getnameinfo() failed: ") + gai_strerror(status) );
                }

                out.push_back(host);
            }

        }

        freeifaddrs( addrs );
        addrs = NULL;

        if (logLevel >= 1) {
            LOG(1) << "getMyAddrs():";
            for (vector<string>::const_iterator it=out.begin(), end=out.end(); it!=end; ++it) {
                LOG(1) << " [" << *it << ']';
            }
            LOG(1) << endl;
        }

        return out;
    }

    vector<string> getAllIPs(const string& iporhost) {
        addrinfo* addrs = NULL;
        addrinfo hints;
        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_family = (IPv6Enabled() ? AF_UNSPEC : AF_INET);

        static string portNum = BSONObjBuilder::numStr(cmdLine.port);

        vector<string> out;

        int ret = getaddrinfo(iporhost.c_str(), portNum.c_str(), &hints, &addrs);
        if ( ret ) {
            warning() << "getaddrinfo(\"" << iporhost << "\") failed: " << gai_strerror(ret) << endl;
            return out;
        }

        for (addrinfo* addr = addrs; addr != NULL; addr = addr->ai_next) {
            int family = addr->ai_family;
            char host[NI_MAXHOST];

            if (family == AF_INET || family == AF_INET6) {
                int status = getnameinfo(addr->ai_addr, addr->ai_addrlen, host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

                massert(13472, string("getnameinfo() failed: ") + gai_strerror(status), status == 0);

                out.push_back(host);
            }

        }

        freeaddrinfo(addrs);

        if (logLevel >= 1) {
            LOG(1) << "getallIPs(\"" << iporhost << "\"):";
            for (vector<string>::const_iterator it=out.begin(), end=out.end(); it!=end; ++it) {
                LOG(1) << " [" << *it << ']';
            }
            LOG(1) << endl;
        }

        return out;
    }
#endif


    class IsSelfCommand : public Command {
    public:
        IsSelfCommand() : Command("_isSelf") , _cacheLock( "IsSelfCommand::_cacheLock" ) {}
        virtual bool slaveOk() const { return true; }
        virtual LockType locktype() const { return NONE; }
        virtual void help( stringstream &help ) const {
            help << "{ _isSelf : 1 } INTERNAL ONLY";
        }
        virtual void addRequiredPrivileges(const std::string& dbname,
                                           const BSONObj& cmdObj,
                                           std::vector<Privilege>* out) {} // No auth required
        bool run(const string& dbname, BSONObj& cmdObj, int, string& errmsg, BSONObjBuilder& result, bool fromRepl ) {
            init();
            result.append( "id" , _id );
            return true;
        }

        void init() {
            scoped_lock lk( _cacheLock );
            if ( ! _id.isSet() )
                _id.init();
        }

        OID _id;

        mongo::mutex _cacheLock;
        map<string,bool> _cache;
    } isSelfCommand;

    bool HostAndPort::isSelf() const {

        int _p = port();
        int p = _p == -1 ? CmdLine::DefaultDBPort : _p;

        if( p != cmdLine.port ) {
            // shortcut - ports have to match at the very least
            return false;
        }

        string host = str::stream() << this->host() << ":" << p;

        {
            // check cache for this host
            // debatably something _could_ change, but I'm not sure right now (erh 10/14/2010)
            scoped_lock lk( isSelfCommand._cacheLock );
            map<string,bool>::const_iterator i = isSelfCommand._cache.find( host );
            if ( i != isSelfCommand._cache.end() )
                return i->second;
        }

#if !defined(_WIN32) && !defined(__sunos__)
        // on linux and os x we can do a quick check for an ip match

        const vector<string> myaddrs = getMyAddrs();
        const vector<string> addrs = getAllIPs(_host);

        for (vector<string>::const_iterator i=myaddrs.begin(), iend=myaddrs.end(); i!=iend; ++i) {
            for (vector<string>::const_iterator j=addrs.begin(), jend=addrs.end(); j!=jend; ++j) {
                string a = *i;
                string b = *j;

                if ( a == b ||
                        ( str::startsWith( a , "127." ) && str::startsWith( b , "127." ) )  // 127. is all loopback
                   ) {

                    // add to cache
                    scoped_lock lk( isSelfCommand._cacheLock );
                    isSelfCommand._cache[host] = true;
                    return true;
                }
            }
        }

#endif

        if ( ! Listener::getTimeTracker() ) {
            // this ensures we are actually running a server
            // this may return true later, so may want to retry
            return false;
        }

        try {
            isSelfCommand.init();
            DBClientConnection conn;
            string errmsg;
            if ( ! conn.connect( host , errmsg ) ) {
                // should this go in the cache?
                return false;
            }

            if (!noauth && !cmdLine.keyFile.empty() ) {
                if (!conn.auth("local", internalSecurity.user, internalSecurity.pwd, errmsg, false)) {
                    return false;
                }
            }

            BSONObj out;
            bool ok = conn.simpleCommand( "admin" , &out , "_isSelf" );
            bool me = ok && out["id"].type() == jstOID && isSelfCommand._id == out["id"].OID();

            // add to cache
            scoped_lock lk( isSelfCommand._cacheLock );
            isSelfCommand._cache[host] = me;

            return me;
        }
        catch ( std::exception& e ) {
            warning() << "could't check isSelf (" << host << ") " << e.what() << endl;
        }

        return false;
    }

}
