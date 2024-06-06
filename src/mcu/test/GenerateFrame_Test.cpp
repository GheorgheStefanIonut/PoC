#include <gtest/gtest.h>
#include "../include/GenerateFrame.h"
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <unistd.h>

/* Mock class for testing */
class MockGenerateFrame : public GenerateFrame {
public:
    using GenerateFrame::GenerateFrame;
    using GenerateFrame::CreateFrame;
};

/* Test fixture for GenerateFrame tests */
class GenerateFrameTest : public ::testing::Test {
protected:
    int mockSocket;
    MockGenerateFrame* generateFrame;

    virtual void SetUp() {
        // Create a mock socket for testing
        mockSocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        ASSERT_TRUE(mockSocket >= 0);

        // Bind the socket to the CAN interface
        struct sockaddr_can addr;
        struct ifreq ifr;

        strcpy(ifr.ifr_name, "vcan1");
        ioctl(s, SIOCGIFINDEX, &ifr);

        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        ASSERT_EQ(bind(mockSocket, (struct sockaddr *)&addr, sizeof(addr)), 0);

        // Initialize the GenerateFrame object with the mock socket
        generateFrame = new MockGenerateFrame(mockSocket);
    }

    int s;
};

/* Test the construction of a frame */
TEST(GenerateFrameSuite1, GenerateValidSocket) {
    const int sock = 123;

    GenerateFrame gen_frame(sock);

    EXPECT_EQ(gen_frame.getSocket(), sock);
}

/* Test the construction of a frame with an invalid socket */
TEST(GenerateFrameSuite1, GenerateInvalidSocket) {
    const int sock = -1;

    EXPECT_THROW(GenerateFrame frame(sock), std::invalid_argument);
}

/* Test the creation of data frame  */
TEST(GenerateFrameSuite2, CreateDataFrame) {
    int can_id = 0x101;
    std::vector<int> data = {0x2, 0x51, 0x3};
    int dlc = data.size();
    FrameType frame_type = FrameType::DATA_FRAME;
    
    GenerateFrame gen_frame;
    can_frame frame = gen_frame.CreateFrame(can_id, data, frame_type);

    EXPECT_EQ(frame.can_id, can_id);
    EXPECT_EQ(frame.can_dlc, dlc);
    for (int i = 0; i < frame.can_dlc; ++i) {
        EXPECT_EQ(frame.data[i], data[i]);
    }
}

/* Test the creation of remote frame */
TEST(GenerateFrameSuite2, CreateRemoteFrame) {
    int can_id = 0x101;
    std::vector<int> data = {0x2, 0x51, 0x3};
    int dlc = data.size();
    FrameType frame_type = FrameType::REMOTE_FRAME;
    
    GenerateFrame gen_frame;
    can_frame frame = gen_frame.CreateFrame(can_id, data, frame_type);

    EXPECT_EQ(frame.can_id, 0x40000101);
    EXPECT_EQ(frame.can_dlc, dlc);
    for (int i = 0; i < frame.can_dlc; ++i) {
        EXPECT_EQ(frame.data[i], data[i]);
    }
}

/* Test the creation of error frame */
TEST(GenerateFrameSuite2, CreateErrorFrame) {
    int can_id = 0x20;
    std::vector<int> data = {0x2, 0x51, 0x3};
    FrameType frame_type = FrameType::ERROR_FRAME;
    
    GenerateFrame gen_frame;
    can_frame frame = gen_frame.CreateFrame(can_id, data, frame_type);

    EXPECT_EQ(frame.can_id, CAN_ERR_FLAG);
    EXPECT_EQ(frame.can_dlc, 0);
}

/* Test the creation of overload frame */
TEST(GenerateFrameSuite2, CreateOverloadFrame) {
    int can_id = 0x20;
    std::vector<int> data = {0x2, 0x51, 0x3};
    FrameType frame_type = FrameType::OVERLOAD_FRAME;
    
    GenerateFrame gen_frame;
    can_frame frame = gen_frame.CreateFrame(can_id, data, frame_type);

    EXPECT_EQ(frame.can_id, CAN_ERR_FLAG);
    EXPECT_EQ(frame.can_dlc, 0);
}

/* Test send frame when socket is invalid */
TEST_F(GenerateFrameTest, SendInvalidSocketFrame) {
    int can_id = 0x123;
    std::vector<int> data = {0x2, 0x51, 0x3};
    FrameType frame_type = FrameType::DATA_FRAME;
    GenerateFrame gen_frame;

    EXPECT_THROW(gen_frame.SendFrame(can_id, data, frame_type), std::runtime_error);
}

/* Test successful send frame */
TEST_F(GenerateFrameTest, SendSuccessfulSendFrame) {
    int can_id = 0x123;
    std::vector<int> data = {0x2, 0x51, 0x3};
    FrameType frame_type = FrameType::DATA_FRAME;
    GenerateFrame gen_frame(s);

    gen_frame.SendFrame(can_id, data, frame_type);

    EXPECT_EQ(errno, 0);
}

/* Test send frame when write fails */
TEST_F(GenerateFrameTest, SendFailWriteOnSocket) {
    int can_id = 0x123;
    std::vector<int> data = {0x2, 0x51, 0x3};
    FrameType frame_type = FrameType::DATA_FRAME;
    GenerateFrame gen_frame(21);

    gen_frame.SendFrame(can_id, data, frame_type);

    EXPECT_NE(errno, 0);
}

/* Test getFrame */
TEST_F(GenerateFrameTest, GetFrame) {
    int can_id = 0x123;
    std::vector<int> data = {0x2, 0x51, 0x3};
    int dlc = data.size();
    FrameType frame_type = FrameType::DATA_FRAME;
    
    GenerateFrame gen_frame(s);
    can_frame frame = gen_frame.CreateFrame(can_id, data, frame_type);
    gen_frame.SendFrame(can_id, data, frame_type);

    EXPECT_EQ(gen_frame.getFrame().can_id, frame.can_id);
    EXPECT_EQ(gen_frame.getFrame().can_dlc, frame.can_dlc);
    for (int i = 0; i < gen_frame.getFrame().can_dlc; ++i) {
        EXPECT_EQ(gen_frame.getFrame().data[i], frame.data[i]);
    }
}

/* Test Session control response */
TEST_F(GenerateFrameTest, SessionControlResponse) {
    int can_id = 0x101;
    int subfunction = 0x01;
    bool response = 1;

    std::vector<int> data = {0x2, 0x50, subfunction};
    GenerateFrame gen_frame(s);
    gen_frame.SessionControl(can_id, subfunction, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Session control request */
TEST_F(GenerateFrameTest, SessionControlRequest) {
    int can_id = 0x101;
    int subfunction = 0x01;
    bool response = 0;

    std::vector<int> data = {0x2, 0x10, subfunction};

    GenerateFrame gen_frame(s);
    gen_frame.SessionControl(can_id, subfunction, response);

    EXPECT_EQ(gen_frame.getFrame().can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test ECU reset response */
TEST_F(GenerateFrameTest, EcuResetResponse) {
    int can_id = 0x101;
    bool response = 1;

    std::vector<int> data = {0x2, 0x51, 0x3};

    GenerateFrame gen_frame(s);
    gen_frame.EcuReset(can_id, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test ECU reset request */
TEST_F(GenerateFrameTest, EcuResetRequest) {
    int can_id = 0x101;
    int subfunction = 0x01;
    bool response = 0;

    std::vector<int> data = {0x2, 0x11, 0x3};

    GenerateFrame gen_frame(s);
    gen_frame.EcuReset(can_id, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Read Data By Identifier request */
TEST_F(GenerateFrameTest, ReadDataByIdentifierRequest) {
    int can_id = 0x101;
    int data_identifier = 0x01;
    std::vector<int> response;

    std::vector<int> data = {0x3, 0x22, data_identifier/0x100, data_identifier%0x100};

    GenerateFrame gen_frame(s);
    gen_frame.ReadDataByIdentifier(can_id, data_identifier, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Read Data By Identifier response */
TEST_F(GenerateFrameTest, ReadDataByIdentifierResponse) {
    int can_id = 0x123;
    int data_identifier = 0x01;
    std::vector<int> response = {0x2};

    std::vector<int> data = {(int)response.size() + 3, 0x62, data_identifier/0x100, data_identifier%0x100, 0x2};

    GenerateFrame gen_frame(s);
    gen_frame.ReadDataByIdentifier(can_id, data_identifier, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Read Data By Identifier response too long */
TEST_F(GenerateFrameTest, RDBIResponseTooLarge) {
    int can_id = 0x123;
    int data_identifier = 0x01;    
    std::vector<int> response = {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2};

    GenerateFrame gen_frame(s);

    testing::internal::CaptureStdout();
    gen_frame.ReadDataByIdentifier(can_id, data_identifier, response);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "Response size is too large\n");
}

/* Test Create Frame Long first frame */
TEST_F(GenerateFrameTest, CreateFrameLongFirstFrame) {
    int can_id = 0x101;
    int sid = 0x50;
    int data_identifier = 0x01;  
    std::vector<int> response = {0x2, 0x2, 0x2};
    bool first_frame = true;

    std::vector<int> data = {0x10, (int)response.size() + 3, sid, data_identifier/0x100, data_identifier%0x100, 0x2, 0x2, 0x2};

    GenerateFrame gen_frame(s);
    gen_frame.CreateFrameLong(can_id, sid, data_identifier, response, first_frame);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Create Frame Long not first frame */
TEST_F(GenerateFrameTest, CreateFrameLongNotFirstFrame) {
    int can_id = 0x101;
    int sid = 0x50;
    int data_identifier = 0x01;  
    std::vector<int> response = {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2};
    bool first_frame = false;

    std::vector<int> data = {0x21, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2};

    GenerateFrame gen_frame(s);
    gen_frame.CreateFrameLong(can_id, sid, data_identifier, response, first_frame);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Read Data By Identifier Long */
TEST_F(GenerateFrameTest, ReadDataByIdentifierLong) {
    int can_id = 0x101;
    int data_identifier = 0x01;  
    std::vector<int> response = {0x2, 0x2, 0x2};
    bool first_frame = true;

    std::vector<int> data = {0x10, (int)response.size() + 3, 0x62, data_identifier/0x100, data_identifier%0x100, 0x2, 0x2, 0x2};

    GenerateFrame gen_frame(s);
    gen_frame.ReadDataByIdentifierLong(can_id, data_identifier, response, first_frame);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Flow Control Frame */
TEST_F(GenerateFrameTest, FlowControlFrame) {
    int can_id = 0x101;
    std::vector<int> data = {0x30, 0x00, 0x00, 0x00};

    GenerateFrame gen_frame(s);
    gen_frame.FlowControlFrame(can_id);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Authentication Request Seed Empty */
TEST_F(GenerateFrameTest, AuthenticationRequestSeedEmpty) {
    int can_id = 0x101;
    std::vector<int> seed;
    std::vector<int> data = {0x3, 0x29, 0x1};

    GenerateFrame gen_frame(s);
    gen_frame.AuthenticationRequestSeed(can_id, seed);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Authentication Request Seed Not Empty */
TEST_F(GenerateFrameTest, AuthenticationRequestSeedNotEmpty) {
    int can_id = 0x101;
    std::vector<int> seed = {0x2, 0x2, 0x2};
    std::vector<int> data = {(int)seed.size() + 2, 0x69, 0x1, 0x2, 0x2, 0x2};

    GenerateFrame gen_frame(s);
    gen_frame.AuthenticationRequestSeed(can_id, seed);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Authentication Send Key Empty */
TEST_F(GenerateFrameTest, AuthenticationSendKeyEmpty) {
    int can_id = 0x101;
    std::vector<int> key;
    std::vector<int> data = {0x02, 0x69, 0x02};

    GenerateFrame gen_frame(s);
    gen_frame.AuthenticationSendKey(can_id, key);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Authentication Send Key Not Empty */
TEST_F(GenerateFrameTest, AuthenticationSendKeyNotEmpty) {
    int can_id = 0x101;
    std::vector<int> key = {0x2, 0x2, 0x2};
    std::vector<int> data = {(int)key.size() + 2, 0x29, 0x2, 0x2, 0x2, 0x2};

    GenerateFrame gen_frame(s);
    gen_frame.AuthenticationSendKey(can_id, key);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Routine Control response */
TEST_F(GenerateFrameTest, RoutineControlResponse) {
    int can_id = 0x101;
    int subfunction = 0x01;
    int routine_identifier = 0x02;
    bool response = 1;

    std::vector<int> data = {0x4, 0x71, subfunction, routine_identifier / 0x100, routine_identifier % 0x100};

    GenerateFrame gen_frame(s);
    gen_frame.RoutineControl(can_id, subfunction, routine_identifier, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Routine Control request */
TEST_F(GenerateFrameTest, RoutineControlRequest) {
    int can_id = 0x101;
    int subfunction = 0x01;
    int routine_identifier = 0x02;
    bool response = 0;

    std::vector<int> data = {0x4, 0x31, subfunction, routine_identifier / 0x100, routine_identifier % 0x100};

    GenerateFrame gen_frame(s);
    gen_frame.RoutineControl(can_id, subfunction, routine_identifier, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Tester Present true */
TEST_F(GenerateFrameTest, TesterPresentTrue) {
    int can_id = 0x101;
    bool response = 1;

    std::vector<int> data = {0x02, 0x7E, 0x00};

    GenerateFrame gen_frame(s);
    gen_frame.TesterPresent(can_id, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Tester Present false */
TEST_F(GenerateFrameTest, TesterPresentFalse) {
    int can_id = 0x101;
    bool response = 0;

    std::vector<int> data = {0x2, 0x3E, 0x00};

    GenerateFrame gen_frame(s);
    gen_frame.TesterPresent(can_id, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Read Memory By Address response empty */
TEST_F(GenerateFrameTest, ReadMemoryByAddressResponseEmpty) {

    /* Segmentation fault error. 
    To be reviewed and fixed */

    /*
    int can_id = 0x101;
    int memory_size = 0x10;
    int memory_address = 0x20;
    std::vector<int> response;

    std::vector<int> data;

    GenerateFrame gen_frame(s);
    gen_frame.ReadMemoryByAddress(can_id, memory_size, memory_address, response);

    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        std::cout << data[i] << " ";
        data.push_back(gen_frame.frame.data[i]);
    }

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
    */
    EXPECT_EQ(1, 2);
}

/* Test Read Memory By Address response not emtpy */
TEST_F(GenerateFrameTest, ReadMemoryByAddressResponseNotEmpty) {

    /* Segmentation fault error. 
    To be reviewed and fixed */

    /*
    int can_id = 0x101;
    int memory_size = 0x10;
    int memory_address = 0x20;
    std::vector<int> response = {0x2, 0x2};

    std::vector<int> data;

    GenerateFrame gen_frame(s);
    gen_frame.ReadMemoryByAddress(can_id, memory_size, memory_address, response);

    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        std::cout << data[i] << " ";
        data.push_back(gen_frame.frame.data[i]);
    }

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
    */
    EXPECT_EQ(1, 2);
}

/* Test Read Memory By Address response too large */
TEST_F(GenerateFrameTest, ReadMemoryByAddressResponseTooLarge) {

    /* Segmentation fault error. 
    To be reviewed and fixed */

    /*
    int can_id = 0x101;
    int memory_size = 0x10;
    int memory_address = 0x20;
    std::vector<int> response = {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2};
    GenerateFrame gen_frame(s);

    testing::internal::CaptureStdout();
    gen_frame.ReadMemoryByAddress(can_id, memory_size, memory_address, response);

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "Response size is too large\n");
    */

    EXPECT_EQ(1, 2);
}

/* Test Read Memory By Address Long first frame */
TEST_F(GenerateFrameTest, ReadMemoryByAddressLongFirstFrame) {

    /* free(): invalid pointer. Aborted (core dumped) error received
       To be reviewed and fixed */

    /*
    int can_id = 0x101;
    int memory_size = 0x10;
    int memory_address = 0x20;
    std::vector<int> response = {0x2, 0x2, 0x2};
    bool first_frame = true;

    std::vector<int> data;

    GenerateFrame gen_frame(s);
    gen_frame.ReadMemoryByAddressLong(can_id, memory_size, memory_address, response, first_frame);

    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        std::cout << data[i] << " ";
        data.push_back(gen_frame.frame.data[i]);
    }

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
    */
    EXPECT_EQ(1, 2);
}

/* Test Read Memory By Address Long not first frame */
TEST_F(GenerateFrameTest, ReadMemoryByAddressLongNotFirstFrame) {

    /* Segmentation fault error received
       To be reviewed and fixed */

    /*
    int can_id = 0x101;
    int memory_size = 0x10;
    int memory_address = 0x20;
    std::vector<int> response = {0x2, 0x2, 0x2};
    bool first_frame = false;

    std::vector<int> data;

    GenerateFrame gen_frame(s);
    gen_frame.ReadMemoryByAddressLong(can_id, memory_size, memory_address, response, first_frame);

    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        std::cout << data[i] << " ";
        data.push_back(gen_frame.frame.data[i]);
    }

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
    */
    EXPECT_EQ(1, 2);
}

/* Test Write Data By Identifier. Data Parameter Empty*/
TEST_F(GenerateFrameTest, WriteDataByIdentifierEmpty) {
    int can_id = 0x101;
    int identifier = 0x20;
    std::vector<int> data_parameter;

    std::vector<int> data = {0x03, 0x6E, identifier / 0x100, identifier % 0x100};

    GenerateFrame gen_frame(s);
    gen_frame.WriteDataByIdentifier(can_id, identifier, data_parameter);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Write Data By Identifier. Data Parameter not empty*/
TEST_F(GenerateFrameTest, WriteDataByIdentifierNotEmpty) {
    int can_id = 0x101;
    int identifier = 0x20;
    std::vector<int> data_parameter = {0x2, 0x2, 0x2};

    std::vector<int> data = {(int)data_parameter.size() + 3, 0x2E, identifier / 0x100, identifier % 0x100, 0x2, 0x2, 0x2};

    GenerateFrame gen_frame(s);
    gen_frame.WriteDataByIdentifier(can_id, identifier, data_parameter);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Write Data By Identifier. Data Parameter too large */
TEST_F(GenerateFrameTest, WriteDataByIdentifierTooLarge) {
    int can_id = 0x101;
    int identifier = 0x20;
    std::vector<int> data_parameter = {0x2, 0x2, 0x2, 0x2, 0x2, 0x2};

    testing::internal::CaptureStdout();
    GenerateFrame gen_frame(s);
    gen_frame.WriteDataByIdentifier(can_id, identifier, data_parameter);

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "Data parameter size is too large\n"); 
}

/* Test Write Data By Identifier Long */
TEST_F(GenerateFrameTest, WriteDataByIdentifierLong) {
    int can_id = 0x101;
    int identifier = 0x02;
    std::vector<int> data_parameter = {0x2, 0x2, 0x2};
    bool first_frame = true;

    std::vector<int> data = {0x10, (int)data_parameter.size() + 3, 0x2E, identifier/0x100, identifier%0x100, 0x2, 0x2 ,0x2};

    GenerateFrame gen_frame(s);
    gen_frame.WriteDataByIdentifierLong(can_id, identifier, data_parameter, first_frame);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Read DTC Information */
TEST_F(GenerateFrameTest, ReadDTCInformation) {
    int can_id = 0x101;
    int subfunction = 0x01;
    int dtc_status_mask = 0x02;

    std::vector<int> data = {0x03, 0x19, subfunction, dtc_status_mask};

    GenerateFrame gen_frame(s);
    gen_frame.ReadDtcInformation(can_id, subfunction, dtc_status_mask);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Read DTC Information Response 01 */
TEST_F(GenerateFrameTest, ReadDTCInformationResponse01) {
    int can_id = 0x101;
    int status_availability_mask = 0x01;
    int dtc_format_identifier = 0x02;
    int dtc_count = 0x03;

    std::vector<int> data = {0x03, 0x59, 0x01, status_availability_mask, dtc_format_identifier, dtc_count};

    GenerateFrame gen_frame(s);
    gen_frame.ReadDtcInformationResponse01(can_id, status_availability_mask, dtc_format_identifier, dtc_count);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Clear Diagnostic Information Response */
TEST_F(GenerateFrameTest, ClearDiagnosticInformationResponse) {
    int can_id = 0x101;
    std::vector<int> group_of_dtc;
    bool response = true;

    std::vector<int> data = {0x01, 0x54};

    GenerateFrame gen_frame(s);
    gen_frame.ClearDiagnosticInformation(can_id, group_of_dtc, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Clear Diagnostic Information Request */
TEST_F(GenerateFrameTest, ClearDiagnosticInformationRequest) {
    int can_id = 0x101;
    std::vector<int> group_of_dtc = {0x2, 0x2};
    bool response = false;

    std::vector<int> data = {(int)group_of_dtc.size() + 1, 0x14, 0x2, 0x2};

    GenerateFrame gen_frame(s);
    gen_frame.ClearDiagnosticInformation(can_id, group_of_dtc, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Clear Diagnostic Information DTC size too large */
TEST_F(GenerateFrameTest, ClearDiagnosticInformationDTCTooLarge) {
    int can_id = 0x101;
    std::vector<int> group_of_dtc = {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2};
    bool response = false;

    testing::internal::CaptureStdout();
    GenerateFrame gen_frame(s);
    gen_frame.ClearDiagnosticInformation(can_id, group_of_dtc, response);

    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "Group of DTC size is too large\n"); 
}

/* Test Access Timing Parameters Response */
TEST_F(GenerateFrameTest, AccessTimingParametersResponse) {
    int can_id = 0x101;
    int subfunction = 0x02;
    bool response = true;

    std::vector<int> data = {0x02, 0xC3, subfunction};

    GenerateFrame gen_frame(s);
    gen_frame.AccessTimingParameters(can_id, subfunction, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Access Timing Parameters Request */
TEST_F(GenerateFrameTest, AccessTimingParametersRequest) {
    int can_id = 0x101;
    int subfunction = 0x02;
    bool response = false;

    std::vector<int> data = {0x02, 0x83, subfunction};

    GenerateFrame gen_frame(s);
    gen_frame.AccessTimingParameters(can_id, subfunction, response);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Negative Response */
TEST_F(GenerateFrameTest, NegativeResponse) {
    int can_id = 0x123;
    int sid = 0x01;
    int nrc = 0x22;

    std::vector<int> data = {0x03, 0x7F, sid, nrc};

    GenerateFrame gen_frame(s);
    gen_frame.NegativeResponse(can_id, sid, nrc);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Request Download */
TEST_F(GenerateFrameTest, RequestDownload) {

    /* Segmentation fault error received
       To be reviewed and fixed */

    /*
    int can_id = 0x101;
    int data_format_identifier = 0x02;
    int memory_size = 0x10;
    int memory_address = 0x3445;

    std::vector<int> data;

    GenerateFrame gen_frame(s);
    gen_frame.RequestDownload(can_id, data_format_identifier, memory_address, memory_size);

    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        std::cout << data[i] << " ";
        data.push_back(gen_frame.frame.data[i]);
    }

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
    */
    
    EXPECT_EQ(1, 2);
}




/* Test Request Download Response */
TEST_F(GenerateFrameTest, RequestDownloadResponse) {

    /* Segmentation fault error received
       To be reviewed and fixed */
    /*
    int can_id = 0x101;
    int max_number_block = 0x2234;

    std::vector<int> data;

    GenerateFrame gen_frame(s);
    gen_frame.RequestDownloadResponse(can_id, max_number_block);

    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        std::cout << data[i] << " ";
        data.push_back(gen_frame.frame.data[i]);
    }

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
    */

    EXPECT_EQ(1, 2);
}

/* Test Transfer Data when transfer request is empty */
TEST_F(GenerateFrameTest, TransferDataEmpty) {
    int can_id = 0x123;
    int block_sequence_counter = 0x30;
    std::vector<int> transfer_request;

    std::vector<int> data = {0x02, 0x76, block_sequence_counter};

    GenerateFrame gen_frame(s);
    gen_frame.TransferData(can_id, block_sequence_counter, transfer_request);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Transfer Data when transfer request is not empty */
TEST_F(GenerateFrameTest, TransferDataNotEmpty) {
    int can_id = 0x123;
    int block_sequence_counter = 0x30;
    std::vector<int> transfer_request = {0x2, 0x2, 0x2};

    std::vector<int> data = {(int)transfer_request.size() + 2, 0x36, block_sequence_counter, 0x2, 0x2, 0x2};

    GenerateFrame gen_frame(s);
    gen_frame.TransferData(can_id, block_sequence_counter, transfer_request);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Transfer Data Long first frame */
TEST_F(GenerateFrameTest, TransferDataLongFirstFrame) {
    int can_id = 0x101;
    int block_sequence_counter = 0x03; 
    std::vector<int> transfer_request = {0x2, 0x2, 0x2, 0x2};
    bool first_frame = true;

    std::vector<int> data = {0x10, (int)transfer_request.size() + 2, 0x36, block_sequence_counter, 0x2, 0x2, 0x2, 0x2};

    GenerateFrame gen_frame(s);
    gen_frame.TransferDataLong(can_id, block_sequence_counter, transfer_request, first_frame);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Transfer Data Long not first frame */
TEST_F(GenerateFrameTest, TransferDataLongNotFirstFrame) {
    int can_id = 0x101;
    int block_sequence_counter = 0x03; 
    std::vector<int> transfer_request = {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2};  
    bool first_frame = false;

    std::vector<int> data = {0x21, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2};

    GenerateFrame gen_frame(s);
    gen_frame.TransferDataLong(can_id, block_sequence_counter, transfer_request, first_frame);

    EXPECT_EQ(gen_frame.frame.can_id, can_id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Request Transfer Exit Response */
TEST_F(GenerateFrameTest, RequestTransferExitResponse) {
    int id = 0x101;
    bool response = true;

    std::vector<int> data = {0x01, 0x77};

    GenerateFrame gen_frame(s);
    gen_frame.RequestTransferExit(id, response);

    EXPECT_EQ(gen_frame.frame.can_id, id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Request Transfer Exit Request */
TEST_F(GenerateFrameTest, RequestTransferExitRequest) {
    int id = 0x101;
    bool response = false;

    std::vector<int> data = {0x01, 0x37};

    GenerateFrame gen_frame(s);
    gen_frame.RequestTransferExit(id, response);

    EXPECT_EQ(gen_frame.frame.can_id, id);
    for (int i = 0; i < gen_frame.frame.can_dlc; ++i) {
        EXPECT_EQ(gen_frame.frame.data[i], data[i]);
    }
}

/* Test Insert Bytes */
TEST_F(GenerateFrameTest, InsertBytes) {
    std::vector<int> data;
    int num_bytes = 1;
    int index = 0;

    GenerateFrame gen_frame(s);
    gen_frame.InsertBytes(data, index, num_bytes);

    EXPECT_EQ(data[0], 0);
}

/* Test Count Digits*/
TEST_F(GenerateFrameTest, CountDigits) {
    int number_1 = -1;
    int number_2 = 1001;

    GenerateFrame gen_frame(s);

    EXPECT_EQ(gen_frame.CountDigits(number_1), 1);
    EXPECT_EQ(gen_frame.CountDigits(number_2), 4);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}