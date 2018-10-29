// Copyright 2018 Keri Oleg

#pragma once

#if defined(LINUX)

namespace neat {

enum conf { WindowWidth = 450, WindowHeight = 800, DPI = 255 };

}  // namespace neat

#elif !defined ANDROID
#error "Unknown target"
#endif  // ANDROID

#ifndef NEAT_APPNAME
#define NEAT_APPNAME "neat application"
#endif  // NEAT_APPNAME
