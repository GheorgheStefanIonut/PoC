/**
 * @file InterfaceConfig.h
 * @author Iancu Daniel
 * @brief Library used for creation, configuration and deletion of SocketCan interfaces.
 * @version 0.1
 * @date 2024-05-15
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef INTERFACECONFIG_H
#define INTERFACECONFIG_H

#include <string>
#include <linux/can.h>
#include <net/if.h>
#include <iostream> 
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can/raw.h>
#include <string.h>
#include <unistd.h>

class SocketCanInterface
{
private:
    int _socketFd = -1;
    std::string _interfaceName;
    struct can_frame _canFrame;
    struct sockaddr_can _addr;
    struct ifreq _ifr;

    void createLinuxVCanInterface();
    void deleteLinuxVCanInterface();
/**
 * @brief Open a new socket and connect it to the vcan interface.
 * 
 * @return true for succesfully creation and opening of socket.
 * @return false for errors
 */
    bool openInterface();
/**
 * @brief Close the owned socket and delete the vcan interface. Automatically called in destructor.
 * 
 */
    void closeInterface();

public:
    SocketCanInterface(const std::string& interfaceName);
/**
 * @brief Method used for making system calls with validation
 * 
 * @param[i] cmd 
 */
    inline void callSystem(std::string& cmd) const;
/**
 * @brief Method used for connecting 2 vcan interfaces. 
 * When the source receives a message, the destination receives it too.
 * Simulates a can bus.
 * 
 * @param[i] sourceInterface 
 * @param[i] destinationInterface 
 */
    void connectLinuxVCanInterfaces(std::string& sourceInterface, std::string& destinationInterface);

    void init();

    std::string& getInterfaceName();
    void setInterfaceName(std::string& interfaceName);
    int getSocketFd() const;

    ~SocketCanInterface();
};

#endif /* INTERFACECONFIG_H */