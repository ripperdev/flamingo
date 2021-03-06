#include <spdlog/sinks/hourly_file_sink.h>

#include "Logger.h"

std::shared_ptr<spdlog::logger> &Logger::getLogger() {
    return logger;
}

bool Logger::init(const std::string &name, const std::string &file) {
    if (inited) return true;
    logger = spdlog::hourly_logger_mt(name, file);
    if (!logger)
        return false;
    inited = true;
    logger->set_pattern("[%Y%m%d %H:%M:%S] [%l] [%v]");
    logger->flush_on(spdlog::level::level_enum::trace);
    return true;
}
