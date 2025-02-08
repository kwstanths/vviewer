#include "CoreTests.hpp"

#include <thread>

#include "vengine/utils/ThreadPool.hpp"

TEST_F(CoreTest, ThreadPool1)
{
    vengine::ThreadPool tp;
    tp.init(4);

    class DummyTask : public Task
    {
    public:
        bool work(float &progress) override
        {
            val++;
            return true;
        }

        uint32_t val = 0;
    };
    DummyTask dt;

    Waitable *w = tp.push(&dt);
    w->wait();

    EXPECT_EQ(dt.val, 1);
}

TEST_F(CoreTest, ThreadPool2)
{
    vengine::ThreadPool tp;
    tp.init(4);

    class DummyTask : public Task
    {
    public:
        bool work(float &progress) override
        {
            val++;
            return true;
        }

        uint32_t val = 0;
    };

    std::vector<DummyTask> dts(400);

    std::vector<Waitable *> ws;
    for (auto &dt : dts)
        ws.push_back(tp.push(&dt));

    for (auto &w : ws)
        w->wait();

    for (auto &dt : dts)
        EXPECT_EQ(dt.val, 1);
}

TEST_F(CoreTest, ThreadPool3)
{
    vengine::ThreadPool tp;
    tp.init(std::thread::hardware_concurrency());

    class DummyTask : public Task
    {
    public:
        DummyTask(uint32_t depth, uint32_t width, ThreadPool &tp, DummyTask *parent = nullptr)
            : Task(parent)
            , mDepth(depth)
            , mWidth(width)
            , mTp(tp)
        {
        }
        ~DummyTask()
        {
            for (uint32_t i = 0; i < mChildren.size(); i++) {
                delete mChildren[i];
            }
        }

        bool work(float &progress) override
        {
            for (uint32_t i = 0; i < 100000; i++) {
                // do work
            }

            val = 1;
            if (mDepth > 1) {
                /* Create new child task */
                for (uint32_t i = 0; i < mWidth; i++) {
                    mChildren.push_back(new DummyTask(mDepth - 1, mWidth, mTp, this));
                    Waitable *w = mTp.push(mChildren.back());
                }
            }

            return true;
        }

        void validate()
        {
            EXPECT_EQ(val, 1);
            for (uint32_t i = 0; i < mChildren.size(); i++) {
                mChildren[i]->validate();
            }
        }

        uint32_t val = 0;

        uint32_t mWidth;
        uint32_t mDepth;
        ThreadPool &mTp;
        std::vector<DummyTask *> mChildren;
    };
    DummyTask dt(5, 30, tp);
    Waitable *w = tp.push(&dt);
    w->wait();
    dt.validate();
}