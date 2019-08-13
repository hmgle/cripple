#!/usr/bin/env python
#coding=utf-8

# base on https://pypi.python.org/pypi/pystun

# The MIT License (MIT)
#
# Copyright (c) 2015 Mingang He <dustgle@gmail.com>.
# Copyright (c) 2014 pystun Developers
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

import sys
import struct
import random
import socket
import binascii
import logging
import optparse


log = logging.getLogger("client")


def enable_logging():
    logging.basicConfig()
    log.setLevel(logging.DEBUG)


#stun attributes
MappedAddress    = '0001'
ResponseAddress  = '0002'
ChangeRequest    = '0003'
SourceAddress    = '0004'
ChangedAddress   = '0005'
Username         = '0006'
Password         = '0007'
MessageIntegrity = '0008'
ErrorCode        = '0009'
UnknownAttribute = '000A'
ReflectedFrom    = '000B'
XorOnly          = '0021'
XorMappedAddress = '8020'
ServerName       = '8022'
SecondaryAddress = '8050' # Non standard extention

#types for a stun message
BindRequestMsg               = '0001'
BindResponseMsg              = '0101'
BindErrorResponseMsg         = '0111'
SharedSecretRequestMsg       = '0002'
SharedSecretResponseMsg      = '0102'
SharedSecretErrorResponseMsg = '0112'

dictAttrToVal ={'MappedAddress'   : MappedAddress,
                'ResponseAddress' : ResponseAddress,
                'ChangeRequest'   : ChangeRequest,
                'SourceAddress'   : SourceAddress,
                'ChangedAddress'  : ChangedAddress,
                'Username'        : Username,
                'Password'        : Password,
                'MessageIntegrity': MessageIntegrity,
                'ErrorCode'       : ErrorCode,
                'UnknownAttribute': UnknownAttribute,
                'ReflectedFrom'   : ReflectedFrom,
                'XorOnly'         : XorOnly,
                'XorMappedAddress': XorMappedAddress,
                'ServerName'      : ServerName,
                'SecondaryAddress': SecondaryAddress}

dictMsgTypeToVal = {'BindRequestMsg'              :BindRequestMsg,
                    'BindResponseMsg'             :BindResponseMsg,
                    'BindErrorResponseMsg'        :BindErrorResponseMsg,
                    'SharedSecretRequestMsg'      :SharedSecretRequestMsg,
                    'SharedSecretResponseMsg'     :SharedSecretResponseMsg,
                    'SharedSecretErrorResponseMsg':SharedSecretErrorResponseMsg}

Blocked = "Blocked"
OpenInternet = "Open Internet"
FullCone = "Full Cone"
SymmetricUDPFirewall = "Symmetric UDP Firewall"
RestricNAT = "Restric NAT"
RestricPortNAT = "Restric Port NAT"
SymmetricNAT = "Symmetric NAT"
ChangedAddressError = "Meet an error, when do Test1 on Changed IP and Port"


def GenTranID():
    a =''
    for i in xrange(32):
        a+=random.choice('0123456789ABCDEF')
    #return binascii.a2b_hex(a)
    return a


def Test(s, host, port, source_ip, source_port, send_data=""):
    retVal = {'Resp':False, 'ExternalIP':None, 'ExternalPort':None, 'SourceIP':None, 'SourcePort':None, 'ChangedIP':None, 'ChangedPort':None}

    str_len = "%#04d" % (len(send_data)/2)
    TranID = GenTranID()
    str_data = ''.join([BindRequestMsg, str_len, TranID, send_data])

    data = binascii.a2b_hex(str_data)
    recvCorr = False
    while not recvCorr:
        recieved = False
        count = 3
        while not recieved:
            log.debug("sendto %s" % str((host, port)))
            s.sendto(data,(host, port))
            try:
                buf, addr = s.recvfrom(2048)
                log.debug("recvfrom: %s" % str(addr))
                recieved = True
            except Exception:
                recieved = False
                if count >0:
                    count-=1
                else:
                    retVal['Resp'] = False
                    return retVal

        MsgType = binascii.b2a_hex(buf[0:2])
        if dictValToMsgType[MsgType] == "BindResponseMsg" and TranID.upper() == binascii.b2a_hex(buf[4:20]).upper():
            recvCorr = True
            retVal['Resp'] = True
            len_message = int(binascii.b2a_hex(buf[2:4]), 16)
            len_remain = len_message
            base = 20
            while len_remain:
                attr_type = binascii.b2a_hex(buf[base:(base+2)])
                attr_len = int(binascii.b2a_hex(buf[(base+2):(base+4)]),16)
                if attr_type == MappedAddress:
                    port = int(binascii.b2a_hex(buf[base+6:base+8]), 16)
                    ip = "".join([str(int(binascii.b2a_hex(buf[base+8:base+9]), 16)),'.',
                    str(int(binascii.b2a_hex(buf[base+9:base+10]), 16)),'.',
                    str(int(binascii.b2a_hex(buf[base+10:base+11]), 16)),'.',
                    str(int(binascii.b2a_hex(buf[base+11:base+12]), 16))])
                    retVal['ExternalIP'] = ip
                    retVal['ExternalPort'] = port

                if attr_type == SourceAddress:
                    port = int(binascii.b2a_hex(buf[base+6:base+8]), 16)
                    ip = "".join([str(int(binascii.b2a_hex(buf[base+8:base+9]), 16)),'.',
                    str(int(binascii.b2a_hex(buf[base+9:base+10]), 16)),'.',
                    str(int(binascii.b2a_hex(buf[base+10:base+11]), 16)),'.',
                    str(int(binascii.b2a_hex(buf[base+11:base+12]), 16))])
                    retVal['SourceIP'] = ip
                    retVal['SourcePort'] = port
                if attr_type == ChangedAddress:
                    port = int(binascii.b2a_hex(buf[base+6:base+8]), 16)
                    ip = "".join([str(int(binascii.b2a_hex(buf[base+8:base+9]), 16)),'.',
                    str(int(binascii.b2a_hex(buf[base+9:base+10]), 16)),'.',
                    str(int(binascii.b2a_hex(buf[base+10:base+11]), 16)),'.',
                    str(int(binascii.b2a_hex(buf[base+11:base+12]), 16))])
                    retVal['ChangedIP'] = ip
                    retVal['ChangedPort'] = port

                if attr_type == ServerName:
                    serverName = buf[(base+4):(base+4+attr_len)]
                base = base + 4 + attr_len
                len_remain = len_remain - (4+attr_len)
    return retVal


def Initialize():
    items = dictAttrToVal.items()
    global dictValToAttr
    dictValToAttr = {}
    for i in xrange(len(items)):
        dictValToAttr.update({items[i][1]:items[i][0]})
    items = dictMsgTypeToVal.items()
    global dictValToMsgType
    dictValToMsgType = {}
    for i in xrange(len(items)):
        dictValToMsgType.update({items[i][1]:items[i][0]})


def GetNATType(s, source_ip, source_port, stun_host=None):
    Initialize()
    if stun_host:
        host = stun_host
    else:
        host = "138.128.215.119"
    port = 3478
    log.debug("Do Test1")
    ret = Test(s, host, port, source_ip, source_port)
    log.debug("Result: %s" % ret)
    type = None
    exIP = ret['ExternalIP']
    exPort = ret['ExternalPort']
    changedIP = ret['ChangedIP']
    changedPort = ret['ChangedPort']
    if not ret['Resp']:
        type = Blocked
    else:
        if ret['ExternalIP'] == source_ip:
            changeRequest = ''.join([ChangeRequest,'0004',"00000006"])
            ret = Test(s, host, port, source_ip, source_port, changeRequest)
            if ret['Resp'] == True:
                type = OpenInternet
            else:
                type = SymmetricUDPFirewall
        else:
            changeRequest = ''.join([ChangeRequest,'0004',"00000006"])
            log.debug("Do Test2")
            ret = Test(s, host, port, source_ip, source_port, changeRequest)
            log.debug("Result: %s" % ret)
            if ret['Resp'] == True:
                type = FullCone
            else:
                log.debug("Do Test1")
                ret = Test(s, host, changedPort, source_ip, source_port)
                log.debug("Result: %s" % ret)
                if not ret['Resp']:
                    type = ChangedAddressError
                else:
                    if exIP == ret['ExternalIP'] and exPort == ret['ExternalPort']:
                        changePortRequest = ''.join([ChangeRequest,'0004',"00000002"])
                        log.debug("Do Test3")
                        ret = Test(s, host, changedPort, source_ip, source_port, changePortRequest)
                        log.debug("Result: %s" % ret)
                        if ret['Resp'] == True:
                            type = RestricNAT
                        else:
                            type = RestricPortNAT
                    else:
                        type = SymmetricNAT
    return type, ret


def get_ip_info(source_ip="0.0.0.0", source_port=54320, stun_host=None):
    socket.setdefaulttimeout(2)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
    s.bind((source_ip, source_port))
    nat_type, nat = GetNATType(s, source_ip, source_port, stun_host=stun_host)
    external_ip = nat['ExternalIP']
    external_port = nat['ExternalPort']
    s.close()
    return (nat_type, external_ip, external_port)


def main(source_ip="0.0.0.0", source_port=54320):
    parser = optparse.OptionParser()
    parser.add_option("-d", "--debug", dest="DEBUG", action="store_true",
                      default=False, help="Enable debug logging")
    parser.add_option("-H", "--host", dest="stun_host", default=None, help="STUN host to use")
    parser.add_option("-i", "--interface", dest="source_ip", default="0.0.0.0",
                      help="network interface for client (default: 0.0.0.0)")
    parser.add_option("-p", "--port", dest="source_port", type="int", default=54320,
                      help="port to listen on for client (default: 54320)")
    (options, args) = parser.parse_args()
    if options.DEBUG:
        enable_logging()
    kwargs = dict(source_ip=options.source_ip,
                  source_port=int(options.source_port),
                  stun_host=options.stun_host)
    nat_type, external_ip, external_port = get_ip_info(**kwargs)
    print "NAT Type:", nat_type
    print "External IP:", external_ip
    print "External Port:", external_port

if __name__ == '__main__':
    main()
