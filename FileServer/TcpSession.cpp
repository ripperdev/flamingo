/**
 * TcpSession.cpp
 * zhangyl 2017.03.09
 **/
#include "TcpSession.h"

#include <utility>               // for move

#include "FileMsg.h"             // for file_msg_header
#include "base/Logger.h"         // for LOG_ERROR, LOG_INFO
#include "net/ProtocolStream.h"  // for BinaryStreamWriter
#include "net/TcpConnection.h"   // for TcpConnection

TcpSession::TcpSession(std::weak_ptr<TcpConnection> tmpconn) : tmpConn_(std::move(tmpconn)) {

}

void TcpSession::send(int32_t cmd, int32_t seq, int32_t errorcode, const std::string &filemd5, int64_t offset,
                      int64_t filesize, const std::string &filedata) {
    std::string outbuf;
    net::BinaryStreamWriter writeStream(&outbuf);
    writeStream.WriteInt32(cmd);
    writeStream.WriteInt32(seq);
    writeStream.WriteInt32(errorcode);
    writeStream.WriteString(filemd5);
    writeStream.WriteInt64(offset);
    writeStream.WriteInt64(filesize);
    writeStream.WriteString(filedata);
    writeStream.Flush();

    sendPackage(outbuf.c_str(), outbuf.length());
}

void TcpSession::sendPackage(const char *body, int64_t bodylength) {
    string strPackageData;
    file_msg_header header = {(int64_t) bodylength};
    strPackageData.append((const char *) &header, sizeof(header));
    strPackageData.append(body, bodylength);

    //TODO: 这些Session和connection对象的生命周期要好好梳理一下
    if (tmpConn_.expired()) {
        //FIXME: 出现这种问题需要排查
        LOG_ERROR("Tcp connection is destroyed , but why TcpSession is still alive ?");
        return;
    }

    std::shared_ptr<TcpConnection> conn = tmpConn_.lock();
    if (conn) {
        LOG_INFO("Send data, package length: %d, body length: %d", strPackageData.length(), bodylength);
        //LOG_DEBUG_BIN((unsigned char*)body, bodylength);
        conn->send(strPackageData.c_str(), strPackageData.length());
    }
}
