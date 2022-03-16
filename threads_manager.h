//
// Created by user on 14.03.2022.
//

#ifndef FOCUSIPCCTRL_THREADS_MANAGER_H
#define FOCUSIPCCTRL_THREADS_MANAGER_H

#include <functional>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

class IPCthread2 {

public:

    explicit IPCthread2() = default;
    explicit IPCthread2(std::chrono::microseconds interval) : m_timeout(interval) {};

    template <typename F, typename... Args>
    void registerCallback(F f, Args&&... args)
    {
        m_laterCB = [=] { f(args...); };
    }

    void setInterval(std::chrono::microseconds interval) {
        m_timeout = interval;
    };

    void start() {
        m_t = std::thread(&IPCthread2::run, this);
    }

    void stop() {
        m_exitThread = true;
        m_cv.notify_all();
        m_t.join();
        m_exitThread = false;
    }

private:

    void run() {
        do {

            std::unique_lock<std::mutex> lk(m_mutex);

            m_laterCB();

            m_cv.wait_for(lk, m_timeout, [this]() { return m_exitThread; });

            if (m_exitThread) {
                return;
            }

        } while (!m_exitThread);
    }

    std::thread m_t;
    std::function<void()> m_laterCB;
    bool m_exitThread = false;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::chrono::microseconds m_timeout = 500ms;
};

class ThreadsManager {

public:
    void addThread(std::function<void(void)> f) {
        std::lock_guard lk(m_mutex);
        auto &t = m_threads.emplace_back(std::make_unique<IPCthread2>());
        t->registerCallback(std::move(f));
    };

    void startThreads() {
        std::lock_guard lk(m_mutex);
        for (auto &t: m_threads)
            t->start();
    };

    void stopThreads() {
        std::lock_guard lk(m_mutex);
        for (auto &t: m_threads) {
            t->stop();
        }
    };

    void setThreadsInterval(std::chrono::microseconds interval) {
        std::lock_guard lk(m_mutex);
        for (auto &t: m_threads) {
            t->setInterval(interval);
        }
    };

    size_t threadsCount() {
        return m_threads.size();
    }

private:
    std::vector<std::unique_ptr<IPCthread2>> m_threads;
    std::mutex m_mutex;
};

#endif //FOCUSIPCCTRL_THREADS_MANAGER_H
