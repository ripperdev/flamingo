/**
 * 全局唯一的UUID生成工具类，Windows上实际使用的是GUID, UUIDGenerator.cpp
 * zhangyl 20190710
 */

#include "UUIDGenerator.h"
#include <uuid.h>

std::string UUIDGenerator::generate() {
    uuid_t uuid;
    char str[40] = {0};

    uuid_generate(uuid);
    uuid_unparse(uuid, str);
    return std::string(str, 36);
}
