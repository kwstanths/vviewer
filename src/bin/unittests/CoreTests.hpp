#include <gtest/gtest.h>

#include "vengine/vulkan/VulkanEngine.hpp"
using namespace vengine;

class CoreTest : public testing::Test
{
protected:
    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestSuite() {}

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestSuite() {}

    // You can define per-test set-up logic as usual.
    void SetUp() override {}

    // You can define per-test tear-down logic as usual.
    void TearDown() override {}

    static VulkanEngine *mEngine;
};