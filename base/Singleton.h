#pragma once

#include <memory>
#include <mutex>

#include "Noncopyable.h"

template<typename T>
class Singleton : public Noncopyable {
public:
    static T &getMe() {
        std::call_once(_flag, [&]() {
            _instance.reset(new T());
        });
        return *_instance;
    }

private:
    static std::unique_ptr<T> _instance;
    static std::once_flag _flag;
};

template<typename T>
std::unique_ptr<T> Singleton<T>::_instance;

template<typename T>
std::once_flag Singleton<T>::_flag;
