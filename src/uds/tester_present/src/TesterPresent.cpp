#include "TesterPresent.h"

TesterPresent::TesterPresent(GenerateFrames& generateFrames, ReceiveFrames& receiveFrames, uint32_t can_id, int s3_timer)
    : genFrames(generateFrames), recFrames(receiveFrames), canId(can_id), s3Timer(s3_timer), listen_canbus(true) {
}

TesterPresent::~TesterPresent() {
    stop();
}

void TesterPresent::start() {
    listen_canbus = true;
    receiverThread = std::thread(&TesterPresent::receiveFrames, this);
    senderThread = std::thread(&TesterPresent::sendTesterPresent, this);
}

void TesterPresent::stop() {
    listen_canbus = false;
    if (receiverThread.joinable()) {
        receiverThread.join();
    }
    if (senderThread.joinable()) {
        senderThread.join();
    }
}

void TesterPresent::sendTesterPresent() {
    while (listen_canbus) {
        try {
            genFrames.testerPresent(canId, true);  // send Tester Present request
            std::this_thread::sleep_for(std::chrono::seconds(s3Timer - 1));
        } catch (const std::exception& ex) {
            std::cerr << "Exception in sendTesterPresent: " << ex.what() << std::endl;
            listen_canbus = false;
        }
    }
}

void TesterPresent::receiveFrames() {
    while (listen_canbus) {
        if (recFrames.receiveFramesFromCANBus()) {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cond_var.wait(lock, [this]() { return !frame_queue.empty(); });
            
            while (!frame_queue.empty()) {
                can_frame frame = frame_queue.front();
                frame_queue.pop();

                if (frame.can_id == canId && frame.data[1] == 0x3E) {
                    // Received a positive response for Tester Present
                    std::cout << "Tester Present Acknowledged" << std::endl;
                }
            }
        } else {
            std::cerr << "Failed to receive frames from CAN bus" << std::endl;
            listen_canbus = false;
        }
    }
}
