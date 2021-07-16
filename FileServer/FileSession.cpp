/**
 * FileSession.cpp
 * zhangyl, 2017.03.17
 **/
#include "FileSession.h"

#include <errno.h>                  // for errno
#include <fcntl.h>                  // for SEEK_SET, SEEK_END
#include <string.h>                 // for strerror, memcpy
#include <sstream>                  // for operator<<, basic_ostream, char_t...

#include "FileManager.h"            // for FileManager
#include "FileMsg.h"                // for file_msg_header, file_msg_error_c...
#include "FileServer/TcpSession.h"  // for TcpSession
#include "base/Logger.h"            // for LOG_ERROR, LOG_INFO
#include "base/Timestamp.h"         // for Timestamp
#include "net/Buffer.h"             // for Buffer
#include "net/InetAddress.h"        // for InetAddress
#include "net/ProtocolStream.h"     // for BinaryStreamReader
#include "net/Sockets.h"            // for net
#include "net/TcpConnection.h"      // for TcpConnection

using namespace net;

//文件服务器最大的包50M
#define MAX_PACKAGE_SIZE    (50 * 1024 * 1024)

FileSession::FileSession(const std::shared_ptr<TcpConnection> &conn, const char *filebasedir) :
        TcpSession(conn),
        m_id(0),
        m_seq(0),
        m_strFileBaseDir(filebasedir),
        m_bFileUploading(false) {
}

void FileSession::onRead(const std::shared_ptr<TcpConnection> &conn, Buffer *pBuffer, Timestamp receivTime) {
    while (true) {
        //不够一个包头大小
        if (pBuffer->readableBytes() < (size_t) sizeof(file_msg_header)) {
            //LOGI << "buffer is not enough for a package header, pBuffer->readableBytes()=" << pBuffer->readableBytes() << ", sizeof(msg)=" << sizeof(file_msg);
            return;
        }

        //不够一个整包大小
        file_msg_header header{};
        memcpy(&header, pBuffer->peek(), sizeof(file_msg_header));

        //包头有错误，立即关闭连接
        if (header.packagesize <= 0 || header.packagesize > MAX_PACKAGE_SIZE) {
            //客户端发非法数据包，服务器主动关闭之
            LOG_ERROR("Illegal package header size: {}, close TcpConnection, client: {}", header.packagesize,
                      conn->peerAddress().toIpPort().c_str());
            conn->forceClose();
            return;
        }

        if (pBuffer->readableBytes() < (size_t) header.packagesize + sizeof(file_msg_header))
            return;

        pBuffer->retrieve(sizeof(file_msg_header));
        std::string inbuf;
        inbuf.append(pBuffer->peek(), header.packagesize);
        pBuffer->retrieve(header.packagesize);
        if (!process(conn, inbuf.c_str(), inbuf.length())) {
            LOG_ERROR("Process error, close TcpConnection, client: {}", conn->peerAddress().toIpPort().c_str());
            conn->forceClose();
        }
    }// end while-loop

}

bool FileSession::process(const std::shared_ptr<TcpConnection> &conn, const char *inbuf, size_t length) {
    BinaryStreamReader readStream(inbuf, length);
    int32_t cmd;
    if (!readStream.ReadInt32(cmd)) {
        LOG_ERROR("read cmd error, client: {}", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    //int seq;
    if (!readStream.ReadInt32(m_seq)) {
        LOG_ERROR("read seq error, client: {}", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    std::string filemd5;
    size_t md5length;
    if (!readStream.ReadString(&filemd5, 0, md5length) || md5length == 0) {
        LOG_ERROR("read filemd5 error, client: {}", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    int64_t offset;
    if (!readStream.ReadInt64(offset)) {
        LOG_ERROR("read offset error, client: {}", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    int64_t filesize;
    if (!readStream.ReadInt64(filesize)) {
        LOG_ERROR("read filesize error, client: {}", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    string filedata;
    size_t filedatalength;
    if (!readStream.ReadString(&filedata, 0, filedatalength)) {
        LOG_ERROR("read filedata error, client: {}", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    LOG_INFO("Request from client: cmd: {}, seq: {}, filemd5: {}, md5length: {}, offset: {}, filesize: {}, filedata length: {}, header.packagesize: {}, client: {}",
         cmd, m_seq, filemd5.c_str(), md5length, offset, filesize, (int64_t) filedata.length(), (int64_t) length,
         conn->peerAddress().toIpPort().c_str());

    //LOG_DEBUG_BIN((unsigned char*)filedata.c_str(), filedatalength);

    switch (cmd) {
        //客户端文件上传
        case msg_type_upload_req:
            return onUploadFileResponse(filemd5, offset, filesize, filedata, conn);

            //客户端文件下载
        case msg_type_download_req: {
            int32_t clientNetType;
            if (!readStream.ReadInt32(clientNetType)) {
                LOG_ERROR("read clientNetType error, client: {}", conn->peerAddress().toIpPort().c_str());
                return false;
            }

            //对于下载，客户端不知道文件大小， 所以值是0
            //if (filedatalength != 0)
            //    return false;
            return onDownloadFileResponse(filemd5, clientNetType, conn);
        }


        default:
            //pBuffer->retrieveAll();
            LOG_ERROR("unsupport cmd, cmd: {}, client: {}", cmd, conn->peerAddress().toIpPort().c_str());
            //conn->forceClose();
            return false;
    }// end switch

    ++m_seq;

    return true;
}

bool FileSession::onUploadFileResponse(const std::string &filemd5, int64_t offset, int64_t filesize,
                                       const std::string &filedata, const std::shared_ptr<TcpConnection> &conn) {
    if (filemd5.empty()) {
        LOG_ERROR("Empty filemd5, client: {}", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    //服务器上已经存在该文件，直接返回(如果该文件是处于打开状态说明处于正在上传的状态)
    if (FileManager::getMe().isFileExsit(filemd5.c_str()) && !m_bFileUploading) {
        offset = filesize;
        string dummyfiledata;
        send(msg_type_upload_resp, m_seq, file_msg_error_complete, filemd5, offset, filesize, dummyfiledata);


        LOG_INFO("Response to client: cmd=msg_type_upload_resp, errorcode: file_msg_error_complete, filemd5: {}, offset: {}, filesize: {}, client: {}",
             filemd5.c_str(), offset, filesize, conn->peerAddress().toIpPort().c_str());

        return true;
    }

    if (offset == 0) {
        std::string filename = m_strFileBaseDir;
        filename += filemd5;
        //这个地方我开始使用的是“w”模式，这在Linux平台没问题，但在Windows上因为是文本模式，fwrite函数在遇到文件中有0x0A时，会自动补上0x0D，造成文件内容出错
        //所以改成wb，使用二进制模式
        m_fp = fopen(filename.c_str(), "wb");
        if (m_fp == nullptr) {
            LOG_ERROR("fopen file error, filemd5: {}, client: {}", filemd5.c_str(), conn->peerAddress().toIpPort().c_str());
            return false;
        }

        //标识该文件正在上传中
        m_bFileUploading = true;
    } else {
        if (m_fp == nullptr) {
            resetFile();
            LOG_ERROR("file pointer should not be null, filemd5: {}, offset: {}, client: {}", filemd5.c_str(), offset,
                 conn->peerAddress().toIpPort().c_str());
            return false;
        }
    }

    if (fseek(m_fp, offset, SEEK_SET) == -1) {
        LOG_ERROR("fseek error, filemd5: {}, errno: {}, errinfo: {}, filedata.length(): {}, m_fp: {}, buffer size is 512*1024, client: {}",
             filemd5.c_str(), errno, strerror(errno), filedata.length(), (void*)m_fp, conn->peerAddress().toIpPort().c_str());

        resetFile();
        return false;
    }

    if (fwrite((char *) filedata.c_str(), 1, filedata.length(), m_fp) != filedata.length()) {
        resetFile();
        LOG_ERROR("fwrite error, filemd5: {}, errno: {}, errinfo: {}, filedata.length(): {}, m_fp: {}, buffer size is 512*1024, client: {}",
             filemd5.c_str(), errno, strerror(errno), filedata.length(), (void*)m_fp, conn->peerAddress().toIpPort().c_str());
        return false;
    }

    //将文件内容刷到磁盘上去
    if (fflush(m_fp) != 0) {
        LOG_ERROR("fflush error, filemd5: {}, errno: {}, errinfo: {}, filedata.length(): {}, m_fp: {}, buffer size is 512*1024, client: {}",
             filemd5.c_str(), errno, strerror(errno), filedata.length(), (void*)m_fp, conn->peerAddress().toIpPort().c_str());

        return false;
    }

    int32_t errorcode = file_msg_error_progress;

    //文件上传成功
    if (offset + (int64_t) filedata.length() == filesize) {
        offset = filesize;
        errorcode = file_msg_error_complete;
        FileManager::getMe().addFile(filemd5.c_str());
        resetFile();
    }

    string dummyfiledatax;
    send(msg_type_upload_resp, m_seq, errorcode, filemd5, offset, filesize, dummyfiledatax);

    std::string errorcodestr = "file_msg_error_progress";
    if (errorcode == file_msg_error_complete)
        errorcodestr = "file_msg_error_complete";

    LOG_INFO("Response to client: cmd=msg_type_upload_resp, errorcode: {}, filemd5: {}, offset: {}, filesize: {}, upload percent: {}%%, client: {}",
         errorcodestr.c_str(), filemd5.c_str(), offset, filesize, (int32_t) (offset * 100 / filesize),
         conn->peerAddress().toIpPort().c_str());

    return true;
}

bool FileSession::onDownloadFileResponse(const std::string &filemd5, int32_t clientNetType,
                                         const std::shared_ptr<TcpConnection> &conn) {
    if (filemd5.empty()) {
        LOG_ERROR("Empty filemd5, client: {}", conn->peerAddress().toIpPort().c_str());
        return false;
    }

    if (!FileManager::getMe().isFileExsit(filemd5.c_str())) {
        //客户端下载不存在的文件，告诉客户端不存在该文件
        string dummyfiledata;
        //文件不存在,则设置应答中偏移量offset和文件大小filesize均设置为0
        int64_t notExsitFileOffset = 0;
        int64_t notExsitFileSize = 0;
        send(msg_type_download_resp, m_seq, file_msg_error_not_exist, filemd5, notExsitFileOffset, notExsitFileSize,
             dummyfiledata);
        LOG_ERROR("filemd5 not exsit, filemd5: {}, clientNetType: {}, client: {}", filemd5.c_str(), clientNetType,
             conn->peerAddress().toIpPort().c_str());
        std::ostringstream os;
        os << "Response to client: cmd=msg_type_download_resp, errorcode=file_msg_error_not_exist "
           << ", filemd5: " << filemd5 << ", clientNetType: " << clientNetType
           << ", offset: 0"
           << ", filesize: 0"
           << ", filedataLength: 0"
           << ", client:" << conn->peerAddress().toIpPort();
        LOG_INFO(os.str().c_str());
        return true;
    }

    //文件还未打开,则先打开
    if (m_fp == nullptr) {
        string filename = m_strFileBaseDir;
        filename += filemd5;
        m_fp = fopen(filename.c_str(), "rb+");
        if (m_fp == nullptr) {
            LOG_ERROR("fopen file error, filemd5: {}, clientNetType: {}, client: {}", filemd5.c_str(), clientNetType,
                 conn->peerAddress().toIpPort().c_str());
            return false;
        }

        if (fseek(m_fp, 0, SEEK_END) == -1) {
            LOG_ERROR("fseek error, m_filesize: {}, errno: {}, filemd5: {}, clientNetType: {}, client: {}",
                 m_currentDownloadFileSize, errno, filemd5.c_str(), clientNetType,
                 conn->peerAddress().toIpPort().c_str());
            return false;
        }

        m_currentDownloadFileSize = ftell(m_fp);
        if (m_currentDownloadFileSize <= 0) {
            LOG_ERROR("m_filesize: {}, errno: {}, filemd5: {}, clientNetType: {}, client: {}", m_currentDownloadFileSize,
                 errno, filemd5.c_str(), clientNetType, conn->peerAddress().toIpPort().c_str());
            return false;
        }

        if (fseek(m_fp, 0, SEEK_SET) == -1) {

            LOG_ERROR("fseek error, m_filesize: {}, errno: {}, filemd5: {}, clientNetType: {}, client: {}",
                 m_currentDownloadFileSize, errno, filemd5.c_str(), clientNetType,
                 conn->peerAddress().toIpPort().c_str());
            return false;
        }
    }

    string filedata;

    //m_offset += offset;
    int64_t currentSendSize = 512 * 1024;
    //移动网络，每次传送64k
    if (clientNetType == client_net_type_cellular)
        currentSendSize = 64 * 1024;

    char buffer[512 * 1024] = {0};
    if (m_currentDownloadFileSize <= m_currentDownloadFileOffset + currentSendSize) {
        currentSendSize = m_currentDownloadFileSize - m_currentDownloadFileOffset;
    }

    //TODO: 这个fread()调用时可能会出现崩溃,待排查
    if (currentSendSize <= 0 || fread(buffer, currentSendSize, 1, m_fp) != 1) {
        std::ostringstream os;
        os << "fread error, filemd5: " << filemd5 << ", clientNetType: " << clientNetType
           << ", errno: " << errno << ", errinfo: " << strerror(errno)
           << ", currentSendSize: " << currentSendSize
           << ", m_fp: " << m_fp
           << ", buffer size is " << currentSendSize
           << ", connection name:" << conn->peerAddress().toIpPort();
        LOG_ERROR(os.str().c_str());
    }

    //将要发送的偏移量
    int sendoffset = m_currentDownloadFileOffset;
    m_currentDownloadFileOffset += currentSendSize;
    filedata.append(buffer, currentSendSize);

    int errorcode = file_msg_error_progress;
    //文件已经下载完成
    if (m_currentDownloadFileOffset == m_currentDownloadFileSize)
        errorcode = file_msg_error_complete;

    send(msg_type_download_resp, m_seq, errorcode, filemd5, sendoffset, m_currentDownloadFileSize, filedata);

    std::ostringstream os2;
    os2 << "Response to client: cmd=msg_type_download_resp, errorcode: "
        << (errorcode == file_msg_error_progress ? "file_msg_error_progress" : "file_msg_error_complete")
        << ", filemd5: " << filemd5 << ", clientNetType: " << clientNetType
        << ", sendoffset: " << sendoffset
        << ", filesize: " << m_currentDownloadFileSize
        << ", filedataLength: " << filedata.length()
        << ", download percent: " << (m_currentDownloadFileOffset * 100 / m_currentDownloadFileSize) << "%"
        << ", client:" << conn->peerAddress().toIpPort();

    LOG_INFO(os2.str().c_str());

    //文件下载成功,重置文件状态
    if (errorcode == file_msg_error_complete)
        resetFile();

    return true;
}

void FileSession::resetFile() {
    if (m_fp != nullptr) {
        fclose(m_fp);
        m_currentDownloadFileOffset = 0;
        m_currentDownloadFileSize = 0;
        m_fp = nullptr;
        m_bFileUploading = false;
    }
}
