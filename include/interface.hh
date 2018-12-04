// Copyright 2018 Keri Oleg

#pragma once

#include <cstdint>
#include <Actions.hh>

#if defined(LINUX)

struct NeatWindowData {
    int width;
    int height;
    int dpi;
    const char* caption;
};

void queryData(NeatWindowData* data);

#elif !defined ANDROID
#error "Unknown target"
#endif  // ANDROID

void init(int width, int heigth);
int action(neat::Actions act, int x, int y);
void draw(uint64_t);
