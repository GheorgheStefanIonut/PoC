#ifndef TESTERPRESENT_H
#define TESTERPRESENT_H

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <stdexcept>
#include "../../mcu/include/GenerateFrames.h"
#include "../../mcu/include/ReceiveFrames.h"

class TesterPresent {
public:
    TesterPresent(GenerateFrames& generateFrames, ReceiveFrames& receiveFrames, uint32_t can_id, int s3_timer);
    ~TesterPresent();

    void start();
    void stop();

private:
    GenerateFrames& genFrames;
    ReceiveFrames& recFrames;
    uint32_t canId;
    int s3Timer;
    bool listen_canbus;
    std::thread receiverThread;
    std::thread senderThread;
    std::mutex queue_mutex;
    std::condition_variable queue_cond_var;
    std::queue<can_frame> frame_queue;

    void sendTesterPresent();
    void receiveFrames();
};

#endif // TESTERPRESENT_H
