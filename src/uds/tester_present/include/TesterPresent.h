#ifndef TESTER_PRESENT_H
#define TESTER_PRESENT_H

#include <cstdint>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <unistd.h>

#include "../../utils/include/GenerateFrames.h"
#include "../../mcu/include/ReceiveFrames.h"
#include "../../utils/include/Logger.h"

class TesterPresent {
public:
    TesterPresent(uint32_t can_id, int socket, Logger &logger);
    ~TesterPresent();

    /*
    * @brief Method to start the module. Sets isRunning flag to true.
    */
    void start();
    
    /*
    * @brief Method to stop the module. Sets isRunning flag to false.
    */
    void stop();

    /*
    * @brief Method to check if the tester is present.
    */
    bool isTesterPresent() const;

private:
    /*
    * @brief Method to monitor the tester presence.
    */
    void monitorTesterPresence();

    /*
    * @brief Method to handle the received frame to check if it is the required frame.
    * @param frame The received frame.
    */
    void handleReceivedFrame(const can_frame &frame);

    uint32_t can_id_;
    GenerateFrames generate_frames_;
    ReceiveFrames receive_frames_;
    std::atomic<bool> running_;
    std::atomic<bool> tester_present_;
    std::thread monitor_thread_;
    std::condition_variable cv_;
    std::mutex mtx_;
    std::chrono::steady_clock::time_point last_tester_present_time_;
    static constexpr std::chrono::seconds S3_TIMEOUT{5};
};

#endif // TESTER_PRESENT_H
