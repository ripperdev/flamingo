#ifndef FLAMINGO_ZLIBUTIL_H
#define FLAMINGO_ZLIBUTIL_H

#include <string>

class ZlibUtil {
public:
    ZlibUtil() = delete;

    ~ZlibUtil() = delete;

    ZlibUtil(const ZlibUtil &rhs) = delete;

    static bool compressBuf(const char *pSrcBuf, size_t nSrcBufLength, char *pDestBuf, size_t &nDestBufLength);

    static bool compressBuf(const std::string &strSrcBuf, std::string &strDestBuf);

    static bool uncompressBuf(const std::string &strSrcBuf, std::string &strDestBuf, size_t nDestBufLength);

    //gzip压缩
    static bool inflate(const std::string &strSrc, std::string &dest);

    //gzip解压
    static bool deflate(const std::string &strSrc, std::string &strDest);

};

#endif //FLAMINGO_ZLIBUTIL_H
