# Based on original work by David Manura
# Copyright (C) 2007-2012 LuaDist.
# Copyright (C) 2013 Brian Sidebotham

# Redistribution and use of this file is allowed according to the terms of the
# MIT license.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


include_directories ( .. ../include )
add_definitions ( -DMONOLITH )

set ( E_SRC openssl.c #
  verify.c asn1pars.c req.c dgst.c dh.c dhparam.c enc.c passwd.c gendh.c errstr.c
  ca.c pkcs7.c crl2p7.c crl.c rsa.c rsautl.c dsa.c dsaparam.c ec.c ecparam.c x509.c
  genrsa.c gendsa.c genpkey.c s_server.c s_client.c speed.c s_time.c apps.c s_cb.c
  s_socket.c app_rand.c version.c sess_id.c ciphers.c nseq.c pkcs12.c pkcs8.c pkey.c
  pkeyparam.c pkeyutl.c spkac.c smime.c cms.c rand.c engine.c ocsp.c prime.c ts.c
  srp.c )

add_executable ( openssl ${E_SRC} )
target_link_libraries ( openssl crypto ssl )

install( TARGETS openssl
    RUNTIME DESTINATION bin )

install( FILES CA.sh CA.pl tsget DESTINATION share/openssl )
