#pragma once
#include <functional>

#include "gtest/gtest.h"

#include "types.h"
#include "exception.h"

using namespace lfmem;

static std::function<void(LFException& ex)> errorMessage = [](LFException& ex)
{
    FAIL() << ex.what() << std::endl;
};


