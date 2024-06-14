/**
 * @file InterfaceConfig.cpp
 * @author Iancu Daniel
 * @brief 
 * @version 0.1
 * @date 2024-05-15
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "../include/InterfaceConfig.h"

#ifndef TESTING
Logger interfaceLogger = Logger("interfaceLogger", "logs/interfaceFramesLog.log");
#else
Logger interfaceLogger = Logger();
#endif

SocketCanInterface::SocketCanInterface(const std::string& interfaceName) : _interfaceName{interfaceName}
{
    init();
}

void SocketCanInterface::callSystem(std::string& cmd) const
{
    if(system(cmd.c_str()) != 0)
    {
        std::cout << cmd << " failed\n";
        /* exit(EXIT_FAILURE); */
    }
}

int SocketCanInterface::setSocketBlocking()
{
    int flags = fcntl(_socketFd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Error for obtaining flags on socket: " << strerror(errno) << std::endl;
        return 1;
    }
    // Set the O_NONBLOCK flag to make the socket non-blocking
    flags |= O_NONBLOCK;
    if (fcntl(_socketFd, F_SETFL, flags) == -1) {
        std::cerr << "Error setting flags: " << strerror(errno) << std::endl;
        return -1;
    }
}

bool SocketCanInterface::openInterface()
{
    std::string cmd = "sudo ip link set " + _interfaceName + " up";
    callSystem(cmd);
    
    /* Create the socket */
    _socketFd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(_socketFd < 0)
    {
        std::cout<<"Error trying to create the socket\n";
        return 1;
    }

    /* Giving name and index to the interface created */
    strcpy(_ifr.ifr_name, _interfaceName.c_str() );
    ioctl(_socketFd, SIOCGIFINDEX, &_ifr);

    /* Set addr structure with info. of the CAN interface */
    _addr.can_family = AF_CAN;
    _addr.can_ifindex = _ifr.ifr_ifindex;

    /* Bind the socket to the CAN interface */
    if(bind(_socketFd, (struct sockaddr * )&_addr, sizeof(_addr)) < 0)
    {
        std::cout<<"Error binding\n";
        return 1;
    }
    setSocketBlocking();
    
    return 0;
}

void SocketCanInterface::closeInterface()
{
    if(_socketFd != -1)
    {
        if(close(_socketFd) == -1)
        {
            std::cout<<"Error closing socket from " << _interfaceName << std::endl;
            exit(EXIT_FAILURE);
        }
        deleteLinuxVCanInterface();
        stopLinuxVCanInterface();
    }
    else
    {
        std::cout<<"Can't close socket with fd = -1";
    }
}

void SocketCanInterface::createLinuxVCanInterface()
{
    std::string cmd = "sudo ip link add " + _interfaceName + " type vcan";
    callSystem(cmd);
}

void SocketCanInterface::deleteLinuxVCanInterface()
{
    std::string cmd = "sudo ip link set " + _interfaceName + " down";
    callSystem(cmd);
}

void SocketCanInterface::stopLinuxVCanInterface()
{
    std::string cmd = "sudo ip link delete " + _interfaceName + " type can";
    callSystem(cmd);
}

int SocketCanInterface::getSocketFd() const
{
    return _socketFd;
}

std::string& SocketCanInterface::getInterfaceName()
{
    return _interfaceName;
}

void SocketCanInterface::init()
{
    createLinuxVCanInterface();
    openInterface();
    std::cout << _interfaceName << " initialised\n";
}

SocketCanInterface::~SocketCanInterface()
{
    closeInterface();
}
