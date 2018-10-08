#include "timer.h"
#include <iostream>

namespace REDasm {

Timer::Timer(): m_running(false), m_state(Timer::InactiveState) { }

Timer::~Timer()
{
    timer_lock m_lock(m_mutex);
    m_state = Timer::InactiveState;
    m_lock.unlock();

    if(m_worker.joinable())
        m_worker.join();
}

size_t Timer::state() const { return m_state; }
bool Timer::active() const { return m_state == Timer::ActiveState; }
bool Timer::paused() const { return m_state == Timer::PausedState; }

void Timer::stop()
{
    if(m_state == Timer::InactiveState)
        return;

    m_state = Timer::InactiveState;
    stateChanged(this);
}

void Timer::pause()
{
    if(m_state != Timer::ActiveState)
        return;

    m_state = Timer::PausedState;
    stateChanged(this);
}

void Timer::resume()
{
    if(m_state != Timer::PausedState)
        return;

    m_state = Timer::ActiveState;
    stateChanged(this);
}

void Timer::tick(TimerCallback cb, std::chrono::milliseconds interval)
{
    if(m_state != Timer::InactiveState)
        return;

    if(m_worker.joinable())
        m_worker.join();

    m_interval = interval;
    m_state = Timer::ActiveState;
    m_timercallback = cb;
    stateChanged(this);
    m_worker = std::thread(&Timer::work, this);

    if(getenv("SYNC_MODE"))
        m_worker.join();
}

void Timer::work()
{
    while(m_state != Timer::InactiveState)
    {
        timer_lock m_lock(m_mutex);

        if(m_state == Timer::ActiveState)
            m_timercallback();

        m_condition.wait_until(m_lock, clock::now() + m_interval);
    }
}

} // namespace REDasm
