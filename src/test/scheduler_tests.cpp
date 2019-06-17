// Copyright (c) 2012-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <scheduler.h>

#include <test/test_bitcoin.h>

#include <random>
#include <boost/test/unit_test.hpp>

#include <atomic>
#include <thread>

BOOST_AUTO_TEST_SUITE(scheduler_tests)

static void microTask(CScheduler &s, std::mutex &mutex, int &counter,
                      int delta,
                      std::chrono::system_clock::time_point rescheduleTime) {
    {
        std::unique_lock<std::mutex> lock(mutex);
        counter += delta;
    }
    std::chrono::system_clock::time_point noTime =
        std::chrono::system_clock::time_point::min();
    if (rescheduleTime != noTime) {
        CScheduler::Function f =
            std::bind(&microTask, std::ref(s), std::ref(mutex),
                        std::ref(counter), -delta + 1, noTime);
        s.schedule(f, rescheduleTime);
    }
}

static void MicroSleep(uint64_t n) {
    std::this_thread::sleep_for(std::chrono::microseconds(n));
}

BOOST_AUTO_TEST_CASE(manythreads) {
    // Stress test: hundreds of microsecond-scheduled tasks,
    // serviced by 10 threads.
    //
    // So... ten shared counters, which if all the tasks execute
    // properly will sum to the number of tasks done.
    // Each task adds or subtracts a random amount from one of the
    // counters, and then schedules another task 0-1000
    // microseconds in the future to subtract or add from
    // the counter -random_amount+1, so in the end the shared
    // counters should sum to the number of initial tasks performed.
    CScheduler microTasks;

    std::mutex counterMutex[10];
    int counter[10] = {0};
    std::mt19937 rng(42);
    std::uniform_int_distribution<> zeroToNine(0, 9);
    std::uniform_int_distribution<> randomMsec(-11, 1000);
    std::uniform_int_distribution<> randomDelta(-1000, 1000);

    std::chrono::system_clock::time_point start =
        std::chrono::system_clock::now();
    std::chrono::system_clock::time_point now = start;
    std::chrono::system_clock::time_point first, last;
    size_t nTasks = microTasks.getQueueInfo(first, last);
    BOOST_CHECK(nTasks == 0);

    for (int i = 0; i < 100; i++) {
        std::chrono::system_clock::time_point t =
            now + std::chrono::microseconds(randomMsec(rng));
        std::chrono::system_clock::time_point tReschedule =
            now + std::chrono::microseconds(500 + randomMsec(rng));
        int whichCounter = zeroToNine(rng);
        CScheduler::Function f = std::bind(
            &microTask, std::ref(microTasks),
            std::ref(counterMutex[whichCounter]),
            std::ref(counter[whichCounter]), randomDelta(rng), tReschedule);
        microTasks.schedule(f, t);
    }
    nTasks = microTasks.getQueueInfo(first, last);
    BOOST_CHECK(nTasks == 100);
    BOOST_CHECK(first < last);
    BOOST_CHECK(last > now);

    // As soon as these are created they will start running and servicing the
    // queue
    std::vector<std::thread> microThreads;
    for (int i = 0; i < 5; i++) {
        microThreads.emplace_back(std::thread(std::bind(&CScheduler::serviceQueue, &microTasks)));
    }

    MicroSleep(600);
    now = std::chrono::system_clock::now();

    // More threads and more tasks:
    for (int i = 0; i < 5; i++) {
        microThreads.emplace_back(std::thread(std::bind(&CScheduler::serviceQueue, &microTasks)));
    }

    for (int i = 0; i < 100; i++) {
        std::chrono::system_clock::time_point t =
            now + std::chrono::microseconds(randomMsec(rng));
        std::chrono::system_clock::time_point tReschedule =
            now + std::chrono::microseconds(500 + randomMsec(rng));
        int whichCounter = zeroToNine(rng);
        CScheduler::Function f = std::bind(
            &microTask, std::ref(microTasks),
            std::ref(counterMutex[whichCounter]),
            std::ref(counter[whichCounter]), randomDelta(rng), tReschedule);
        microTasks.schedule(f, t);
    }

    // Drain the task queue then exit threads
    microTasks.stop(true);
    // ... wait until all the threads are done
    for (auto&& thread : microThreads) thread.join();

    int counterSum = 0;
    for (int i = 0; i < 10; i++) {
        BOOST_CHECK(counter[i] != 0);
        counterSum += counter[i];
    }
    BOOST_CHECK_EQUAL(counterSum, 200);
}

BOOST_AUTO_TEST_CASE(schedule_every) {
    CScheduler scheduler;

    std::condition_variable cvar;
    std::atomic<int> counter{15};
    std::atomic<bool> keepRunning{true};

    scheduler.scheduleEvery(
        [&keepRunning, &cvar, &counter, &scheduler]() {
            BOOST_CHECK(counter > 0);
            cvar.notify_all();
            if (--counter > 0) {
                return true;
            }

            // We reached the end of our test, make sure nothing run again for
            // 100ms.
            scheduler.scheduleFromNow(
                [&keepRunning, &cvar]() {
                    keepRunning = false;
                    cvar.notify_all();
                },
                100);

            // We set the counter to some magic value to check the scheduler
            // empty its queue properly after 120ms.
            scheduler.scheduleFromNow([&counter]() { counter = 42; }, 120);
            return false;
        },
        5);

    // Start the scheduler thread.
    std::thread schedulerThread(
        std::bind(&CScheduler::serviceQueue, &scheduler));

    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    while (keepRunning) {
        cvar.wait(lock);
        BOOST_CHECK(counter >= 0);
    }

    BOOST_CHECK_EQUAL(counter, 0);
    scheduler.stop(true);
    schedulerThread.join();
    BOOST_CHECK_EQUAL(counter, 42);
}

BOOST_AUTO_TEST_SUITE_END()
