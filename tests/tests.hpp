#pragma once

#include <cstdio>

struct TestResults {
    int passed = 0;
    int failed = 0;
};

#define CHECK(results, cond)                                                                       \
    do {                                                                                           \
        if (cond) {                                                                                \
            (results).passed++;                                                                    \
        } else {                                                                                   \
            (results).failed++;                                                                    \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                            \
        }                                                                                          \
    } while (0)

void RunRulesTests(TestResults& results);
void RunFileTests(TestResults& results);
