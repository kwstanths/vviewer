#include <gtest/gtest.h>

#include "vengine/vulkan/VulkanEngine.hpp"
using namespace vengine;

class RenderPTTest : public testing::Test
{
protected:
    // Per-test-suite set-up.
    // Called before the first test in this test suite.
    // Can be omitted if not needed.
    static void SetUpTestSuite()
    {
        mEngine = new VulkanEngine("unittests");
        mEngine->initResources();
    }

    // Per-test-suite tear-down.
    // Called after the last test in this test suite.
    // Can be omitted if not needed.
    static void TearDownTestSuite() { mEngine->releaseResources(); }

    // You can define per-test set-up logic as usual.
    void SetUp() override { mEngine->scene().clear(); }

    // You can define per-test tear-down logic as usual.
    void TearDown() override {}

    static VulkanEngine *mEngine;
};