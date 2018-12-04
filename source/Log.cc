// Copyright 2018 Keri Oleg

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef LINUX
#include <iostream>
#endif

#include <Log.hh>

namespace neat {

Log::~Log() {
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, tag, "%s", message_.str().c_str());
#endif

#ifdef LINUX
    std::cout << message_.str() << std::endl;
#endif
}

}  // namespace neat
