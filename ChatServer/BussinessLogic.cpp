/**
 * 即时通讯的业务逻辑都统一放在这里，BusinessLogic.cpp
 * zhangyl 2018.05.16
 */

#include "BussinessLogic.h"
#include "jsoncpp/json/json.h"
#include "base/AsyncLog.h"
#include "base/Singleton.h"
#include "ChatServer.h"
#include "UserManager.h"

void BussinessLogic::registerUser(const std::string &data, const std::shared_ptr<TcpConnection> &conn, bool keepalive,
                                  std::string &retData) {
    //{ "user": "13917043329", "nickname" : "balloon", "password" : "123" }

    Json::CharReaderBuilder b;
    Json::CharReader *reader(b.newCharReader());
    Json::Value jsonRoot;
    JSONCPP_STRING errs;
    bool ok = reader->parse(data.c_str(), data.c_str() + data.length(), &jsonRoot, &errs);
    if (!ok || !errs.empty()) {
        LOGW("invalid json: %s, client: %s", data.c_str(), conn->peerAddress().toIpPort().c_str());
        delete reader;
        return;
    }
    delete reader;

    if (!jsonRoot["username"].isString() || !jsonRoot["nickname"].isString() || !jsonRoot["password"].isString()) {
        LOGW("invalid json: %s, client: %s", data.c_str(), conn->peerAddress().toIpPort().c_str());
        return;
    }

    User u;
    u.username = jsonRoot["username"].asString();
    u.nickname = jsonRoot["nickname"].asString();
    u.password = jsonRoot["password"].asString();

    //std::string retData;
    User cachedUser;
    cachedUser.userid = 0;
    UserManager::getMe().getUserInfoByUsername(u.username, cachedUser);
    if (cachedUser.userid != 0)
        retData = R"({"code": 101, "msg": "registered already"})";
    else {
        if (!UserManager::getMe().addUser(u))
            retData = R"({"code": 100, "msg": "register failed"})";
        else {
            retData = R"({"code": 0, "msg": "ok"})";
        }
    }

    //conn->Send(msg_type_register, m_seq, retData);

    //LOGI << "Response to client: cmd=msg_type_register" << ", userid=" << u.userid << ", data=" << retData;
}
