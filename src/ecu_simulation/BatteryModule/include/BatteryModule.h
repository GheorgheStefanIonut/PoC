/**
 *  @file BatteryModule.h
 *  This library will be used to simulate a Battery Module
 *  providing some readings and informations (like energy, voltage, percentage, etc)
 *  extracted from Linux, in order to be passed through a virtual CAN interface.
 * 
 *  How to use : Simply instantiate the class in Main solution, and access it's methods there.
 *  BatteryModule batteryObj;  *  Default Constructor for battery object with
 *                             *  moduleID 0x11 and interface name 'vcan0'
 *  BatteryModule batteryObj(interfaceNumber, moduleID);  *  Custom Constructor, for battery object
 *                             *  with custom moduleID, custom interface name
 * @author: Alexandru Nagy
 * @date: May 2024
 * @version 1.0
 */

#ifndef POC_INCLUDE_BATTERY_MODULE_H
#define POC_INCLUDE_BATTERY_MODULE_H

#define BATTERY_MODULE_ID 0x11

#include <thread>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include "../../../utils/include/CreateInterface.h"
#include "ReceiveFrames.h"
#include "GenerateFrames.h"
#include "BatteryModuleLogger.h"

class BatteryModule
{
private:
    int moduleId;

    float energy;
    float voltage;
    float percentage;
    std::string state;

    CreateInterface canInterface;
    ReceiveFrames* frameReceiver;

public:
    /**
     * @brief Default constructor for Battery Module object.
     */
    BatteryModule();
    /**
     * @brief Parameterized constructor for Battery Module object with custom interface name, custom moduleId.
     * 
     * @param _interfaceNumber Interface number used to create vcan interface.
     * @param _moduleId Custom module identifier.
     */
    BatteryModule(int _interfaceNumber, int _moduleId);
    /**
     * @brief Destructor Battery Module object.
     */
    ~BatteryModule();

    /**
     * @brief Function to notify MCU if the module is Up & Running.
     */
    void sendNotificationToMCU();

    /**
     * @brief Helper function to execute shell commands and fetch output
     * in order to read System Information about built-in Battery.
     * This method is currently 'virtual' in order to be overridden in Test.
     * 
     * @param cmd Shell command to be executed.
     * @return Output returned by the shell command. 
     */
    virtual std::string exec(const char *cmd);

    /**
     * @brief This function will parse the data from the system about battery,
     * and will store all values in separate variables
     * 
     * @param data Data taken from the system.
     * @param _energy System energy level.
     * @param _voltage System voltage.
     * @param _percentage System battery percentage.
     * @param _temperature System temperature.
     */
    void parseBatteryInfo(const std::string &data, float &energy, float &voltage, float &percentage, std::string &state);

    /**
     * @brief Function to fetch data from system about battery.
     */
    void fetchBatteryData();

    /**
     * @brief Function that starts the frame receiver.
     */
    void receiveFrames();

    /**
     * @brief Function that stops the frame receiver.
     */
    void stopFrames();

    /* Member Accessors */
    /**
     * @brief Get function for energy.
     * 
     * @return Returns energy.
     */
    float getEnergy() const;

    /**
     * @brief Get function for voltage.
     * 
     * @return Returns voltage. 
     */
    float getVoltage() const;

    /**
     * @brief Get function for battery percentage.
     * 
     * @return Returns battery percentage. 
     */
    float getPercentage() const;

    /**
     * @brief Get the Linux Battery State - charging, discharging, fully-charged, etc.
     * 
     * @return Returns Battery State - charging, discharging, fully-charged, etc.
     */
    std::string getLinuxBatteryState();
};

#endif