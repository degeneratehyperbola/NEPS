#pragma once

#include <cstddef>

#define ___PASTE_TOKENS(a, b) a##b
#define ___PASTE_TOKENS_I(a, b) ___PASTE_TOKENS(a, b)
#define PAD(size) \
private: \
    std::byte ___PASTE_TOKENS_I(_pad, __COUNTER__)[size]; \
public:
#define INIT_PAD(size) \
private: \
    std::byte ___PASTE_TOKENS_I(_pad, __COUNTER__)[size]{}; \
public:
