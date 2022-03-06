#ifndef FOCUSIPCCTRL_SCHEDULER_H
#define FOCUSIPCCTRL_SCHEDULER_H

#include <thread>
#include <condition_variable>
#include <set>
#include <vector>

#include "controllers/CanController.h"

inline void log_dbg(const char *message, ...) {
#ifdef DEBUG
    va_list args;
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
#endif
}

class Scheduler {

public:



    Scheduler(CanController* controller): m_controller(controller) {
        start();
    };

    Scheduler(const Scheduler&) = delete;

    virtual ~Scheduler() {
        stop();
    };


    int send_package(const can_packet &packet) {

        const auto &ecu_address = packet.first;
        const auto &data = packet.second;
        std::unique_lock<std::mutex> lk(m_mutex);

        auto p = m_q.insert(std::make_pair(ecu_address, data));
        if(!p.second) {
            fprintf(stderr, "Evt_%03X already exists in Q. Q overflow\n", ecu_address);
        } else {
            log_dbg("Evt_%03X PUT to Q\n", ecu_address);
        }

        lk.unlock();

        m_cv.notify_one();

        return 0;
    };

private:

    void start() {

        std::unique_lock<std::mutex> lk(m_mutex);

        m_run = true;
        m_sched_tr = std::thread(&Scheduler::process_packages, this);
    }

    void stop() {
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_run = false;
        }
        m_cv.notify_one();
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cv.notify_one();
            m_cv.wait(lk, [this]{return m_stopped;});
            m_stopped = false;
        }
        m_sched_tr.join();
        log_dbg("Scheduler stopped\n");
    }

    void process_packages() {

        std::unique_lock<std::mutex> lk(m_mutex);

        log_dbg("Start packages scheduler\n");

        while (m_run) {

            m_cv.wait(lk);

            log_dbg("Scheduler wake\n");

            while (true) {
                /** locked state **/

                log_dbg("Q size %lu\n", m_q.size());

                // check if not empty
                if (m_q.empty()) {
                    log_dbg("Q empty\n");
                    break;
                }

                // pop one
                auto w_id_h = m_q.extract(m_q.begin()); //m_q.extract(--m_q.end())
                auto w_id = w_id_h.value();
                log_dbg("Evt_%03X POP from Q\n", w_id.first);

                lk.unlock();
                /** unlocked state **/

                //process
                log_dbg("Evt_%03X processing...\n", w_id.first);
                m_controller->transaction(w_id.first, w_id.second);

                lk.lock();
                /** locked state **/
            }
        }

        log_dbg("Stopping scheduler...\n");

        m_stopped = true;
        lk.unlock();
        m_cv.notify_one();
    }

    CanController *m_controller;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_sched_tr;
    bool m_run = false;
    bool m_stopped = false;
    std::set<can_packet> m_q;
};

#endif //FOCUSIPCCTRL_SCHEDULER_H
