#include "../include/GenerateFrames.h"

#include <gtest/gtest.h>
#include <net/if.h>
#include <cstring>
#include <string>
#include <sys/ioctl.h>
#include <thread>
#include <fcntl.h>

/*Global variables*/
/*Sockets for Recive/Send*/
int s1;
int s2;
/*Const*/
const int id = 0x101;

/*Class to capture the frame sin the can-bus*/
class CaptureFrame
{
    public:
        struct can_frame frame;
        void capture()
        {
            int nbytes = read(s2, &frame, sizeof(struct can_frame));
        }
};
int createSocket()
{
    /*Create socket*/
    std::string nameInterface = "vcan0";
    struct can_frame frame;
    struct sockaddr_can addr;
    struct ifreq ifr;
    int s;

    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if( s<0)
    {
        std::cout<<"Error trying to create the socket\n";
        return 1;
    }
    /*Giving name and index to the interface created*/
    strcpy(ifr.ifr_name, nameInterface.c_str() );
    ioctl(s, SIOCGIFINDEX, &ifr);
    /*Set addr structure with info. of the CAN interface*/
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    /*Bind the socket to the CAN interface*/
    int b = bind( s, (struct sockaddr*)&addr, sizeof(addr));
    if( b < 0 )
    {
        std::cout<<"Error binding\n";
        return 1;
    }
    return s;
}
struct can_frame createFrame(std::vector<int> test_data)
{
    struct can_frame resultFrame;
    resultFrame.can_id = id;
    int i=0;
    for (auto d : test_data)
        resultFrame.data[i++] = d;
    resultFrame.can_dlc = test_data.size();
    return resultFrame;
}
void testFrames(struct can_frame expected_frame, CaptureFrame &c1 )
{
    EXPECT_EQ(expected_frame.can_id, c1.frame.can_id);
    EXPECT_EQ(expected_frame.can_dlc, c1.frame.can_dlc);
    for (int i = 0; i < expected_frame.can_dlc; ++i) {
        EXPECT_EQ(expected_frame.data[i], c1.frame.data[i]);
    }
}
/*Create object for all tests*/
struct GenerateFramesTest : testing::Test
{
    GenerateFrames* g1;
    CaptureFrame* c1;

    GenerateFramesTest()
    {
        g1 = new GenerateFrames(s1);
        c1 = new CaptureFrame();
    }
    ~GenerateFramesTest()
    {
        delete g1;
        delete c1;
    }
};
/* Test for AddSocket */
TEST_F(GenerateFramesTest, AddSocket)
{ 
    EXPECT_EQ(s1, g1->getSocket());
}
/* Test for Service SessionConroll */
TEST_F(GenerateFramesTest, SessionControlTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x02,0x10,0x01});
    /*Start listening for frame in the CAN-BUS */
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->sessionControl(0x101,0x01);
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for Service EcuReset */
TEST_F(GenerateFramesTest, EcuResetTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x02,0x11,0x03});
    /*Start listening for frame in the CAN-BUS */
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->ecuReset(id);
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for method AuthenticationSeedRequest */
TEST_F(GenerateFramesTest, AuthSeedTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x05,0x69,0x1,0x23,0x34,0x35});
    /*Start listening for frame in the CAN-BUS*/
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->authenticationRequestSeed(id,{0x23,0x34,0x35});
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for method AuthenticationSendKey */
TEST_F(GenerateFramesTest, AuthKeyTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x05,0x29,0x2,0x23,0x34,0x35});
    /*Start listening for frame in the CAN-BUS*/
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->authenticationSendKey(id,{0x23,0x34,0x35});
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for method AuthenticationSendKey as a response */
TEST_F(GenerateFramesTest, AuthKeyResponseTest) 
{
    /*Create expected frame*/
    struct can_frame expected_frame = createFrame({0x2,0x69,0x02});
    /*Start listening for frame in the CAN-BUS*/
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->authenticationSendKey(id);
    receive_thread.join();
    /*TEST*/
    testFrames(expected_frame, *c1);
}
/* Test for Service RoutinControll */
TEST_F(GenerateFramesTest, RoutinControlFrame) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x04,0x31,0x02,0x34,0x1A});
    /*Start listening for frame in the CAN-BUS*/
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->routineControl(id,0x02,0x341A);
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for Service ReadByIdentifier */
TEST_F(GenerateFramesTest, ReadByIdentRespTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x05,0x22,0x33,0x22,0x32,0x11});
    /*Start listening for frame in the CAN-BUS */
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->readDataByIdentifier(id,0x3322,{0x32,0x11});
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for Service ReadByIdentifier for multiple frames */
TEST_F(GenerateFramesTest, ReadByIdentLongRespTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x10,12,0x22,0x12,0x34,1,2,3});
    /*Start listening for frame in the CAN-BUS */ 
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame simulation*/
    std::vector<int> response = {1,2,3,4,5,6,7,8,9};
    if (response.size() > 4)
    {
        g1->readDataByIdentifierLongResponse(id,0x1234,response);
    }
    else{
        g1->readDataByIdentifier(id,0x1234,response);
    }
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for Service ReadMemoryByAddress */
TEST_F(GenerateFramesTest, ReadByAddressRespTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x07,0x63,0x21,0x01,0x23,0x45,1,2});
    /*Start listening for frame in the CAN-BUS */
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->readMemoryByAddress(id,0x2345,0x01,{1,2});
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for Service clearDiagnosticInformation */
TEST_F(GenerateFramesTest, ClearDTCTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x04,0x14,0xFF,0xFF,0xFF});
    /*Start listening for frame in the CAN-BUS */ 
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->clearDiagnosticInformation(id);
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for Service accessTimingParameters */
TEST_F(GenerateFramesTest, AccesTimeParamTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x02,0x83,0x1});
    /*Start listening for frame in the CAN-BUS*/
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->accessTimingParameters(id,0x01);
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for Service requestDownload */
TEST_F(GenerateFramesTest, RequestDownloadTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x06,0x34, 0x00, 0x12, 0x34,0x45, 0x10});
    /*Start listening for frame in the CAN-BUS */ 
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->requestDownload(id,0x00,0x3445,0x10);
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for Service requestDownload a response */
TEST_F(GenerateFramesTest, RequestDownloadTest2) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x04,0x74, 0x20,0x22, 0x34});
    /*Start listening for frame in the CAN-BUS */ 
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->requestDownloadResponse(id,0x2234);
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for Service transferData*/
TEST_F(GenerateFramesTest, TransferDataTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x07,0x36, 0x20, 1,2,3,4,5});
    /*Start listening for frame in the CAN-BUS */ 
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->transferData(id, 0x20,{1,2,3,4,5});
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Second Test for Service transferData*/
TEST_F(GenerateFramesTest, TransferDataTest2) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x02,0x76, 0x20});
    /*Start listening for frame in the CAN-BUS */ 
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->transferData(id, 0x20);
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for Service transferData for multiple frames */
TEST_F(GenerateFramesTest, TransferDataLongTest) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x10,0x09,0x36,0x20,1,2,3,4});
    /*Start listening for frame in the CAN-BUS */ 
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame simulation*/
    std::vector<int> data = {1,2,3,4,5,6,7};
    if (data.size() > 4)
    {
        g1->transferDataLong(id,0x20,data);
    }
    else{
        g1->transferData(id,0x20,data);
    }
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Second Test for Service transferData for multiple frames */
TEST_F(GenerateFramesTest, TransferDataLongTest2) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x05,0x36,0x20,1,2,3});
    /*Start listening for frame in the CAN-BUS */ 
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame simulation*/
    std::vector<int> data = {1,2,3};
    if (data.size() > 4)
    {
        g1->transferDataLong(id,0x20,data);
    }
    else{
        g1->transferData(id,0x20,data);
    }
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
/* Test for Service requestTransferExit */
TEST_F(GenerateFramesTest, ReqTransferExit) 
{
    /*Create expected frame*/
    struct can_frame result_frame = createFrame({0x01,0x37});
    /*Start listening for frame in the CAN-BUS */ 
    std::thread receive_thread([this]() {
        c1->capture();
    });
    /*Send frame*/
    g1->requestTransferExit(id);
    receive_thread.join();
    /*TEST*/
    testFrames(result_frame, *c1);
}
int main(int argc, char* argv[])
{
    s1 = createSocket();
    s2 = createSocket();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}