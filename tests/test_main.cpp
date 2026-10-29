#include "tests.hpp"

#include <cstdio>

int main() {
    TestResults results;
    RunRulesTests(results);
    RunAirSeaTests(results);
    RunFileTests(results);
    RunFuzzTests(results);
    RunPlannerTests(results);
    std::printf("%d passed, %d failed\n", results.passed, results.failed);
    return results.failed == 0 ? 0 : 1;
}
