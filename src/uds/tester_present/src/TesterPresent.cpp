#include "TesterPresent.h"

TesterPresent::TesterPresent(uint32_t can_id, int socket, Logger &logger)
    : can_id_(can_id), generate_frames_(socket, logger), receive_frames_(socket, -1), running_(false), tester_present_(false) 
{

}

TesterPresent::~TesterPresent() 
{
    stop();
}

void TesterPresent::start() {
    running_ = true;
    monitor_thread_ = std::thread(&TesterPresent::monitorTesterPresence, this);
    receive_frames_.startListenCANBus();
}

void TesterPresent::stop() {
    running_ = false;
    receive_frames_.stopListenCANBus();
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

bool TesterPresent::isTesterPresent() const {
    return tester_present_;
}

void TesterPresent::monitorTesterPresence() {
    while (running_) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait_for(lock, S3_TIMEOUT, [this] { 
            return std::chrono::steady_clock::now() - last_tester_present_time_ < S3_TIMEOUT; 
        });

        if (std::chrono::steady_clock::now() - last_tester_present_time_ >= S3_TIMEOUT) {
            tester_present_ = false;
        } else {
            tester_present_ = true;
        }
    }
}

void TesterPresent::handleReceivedFrame(const can_frame &frame) {
    if (frame.can_id == can_id_ && frame.data[1] == 0x3E) {
        std::lock_guard<std::mutex> lock(mtx_);
        last_tester_present_time_ = std::chrono::steady_clock::now();
        generate_frames_.testerPresent(can_id_, true);
    }
}
