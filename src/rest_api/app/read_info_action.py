"""
Author: Mujdei Ruben
Date: June 2024
Use the class ReadInfo to read information from different ECUs.

How to use:
    u = ReadInfo(bus, 0x23, [0x11, 0x12, 0x13])
    u.read_from_battery()
"""

import json
import datetime
from actions import *

class ToJSON:
    """Open-Close principle. Base class for different JSON formats."""
    def _to_json(self, data):
        pass

class BatteryToJSON(ToJSON):
    def _to_json(self, data: list):
        response_to_frontend = {
            "Battery level": data[0],
            "Voltage": data[1],
            "Battery state of charge": data[2],
            "Temperature": data[3],
            "Life cycle": data[4],
            "Serial number": data[5],
            "time_stamp": datetime.datetime.now().isoformat()
        }
        return json.dumps(response_to_frontend)

class ElementToJSON(ToJSON):
    def _to_json(self, data: list):
        response_to_frontend = {}
        for index, element in enumerate(data, start=1):
            response_to_frontend[f"Element{index}"] = element
        return json.dumps(response_to_frontend)

class ReadInfo(Action):
    """
    ReadInfo class to read information from different ECUs.

    Attributes:
    - bus: CAN bus interface for communication.
    - my_id: Identifier for the device initiating the update.
    - id_ecu: Identifier for the specific ECU being updated.
    - g: Instance of GenerateFrame for generating CAN bus frames.
    """

    def read_from_battery(self):
        """
        Method to read information from the battery module.

        Returns:
        - JSON response.
        """

        id_battery = self.id_ecu[1]
        id = self.my_id * 0x100 + id_battery

        try:
            self.g.session_control(id, 0x01)
            self._passive_response(SESSION_CONTROL, "Error changing session control")

            self._authentication(id)

            level = self._read_by_identifier(id,0x1234)
            voltage = self._read_by_identifier(id,0x0536)
            state_of_charge = self._read_by_identifier(id,0x1d23)
            temperature = self._read_by_identifier(id,0x1aaa)
            life_cycle = self._read_by_identifier(id,0xf332)
            serial_number = self._read_by_identifier(id,0xb55a)
            data = [level, voltage, state_of_charge, temperature, life_cycle, serial_number]
            module = BatteryToJSON()

            response_json = self._to_json(module, data)
            # Shutdown the CAN bus interface
            self.bus.shutdown()

            return response_json

        except CustomError as e:
            self.bus.shutdown()
            return e.message
    
    def read_from_custom(self, identifiers:list):
        id_battery = self.id_ecu[1]
        id = self.my_id * 0x100 + id_battery
        try:
            self.g.session_control(id, 0x01)
            self._passive_response(SESSION_CONTROL, "Error changing session control")

            self._authentication(id)

            data = []
            for identifier in identifiers:
                data += self._read_by_identifier(id,identifier)

            module = ElementToJSON()

            response_json = self._to_json(module, data)
            # Shutdown the CAN bus interface
            self.bus.shutdown()

            return response_json

        except CustomError as e:
            self.bus.shutdown()
            return e.message
        
    def _to_json(self, module: ToJSON, data: list):
        """
        Private method to create a JSON response with status and error information.

        Args:
        - module: An instance of ToJSON or its subclasses.
        - data: Data collected from the ECU.

        Returns:
        - JSON-formatted response containing status, errors, and timestamp.
        """
        return module._to_json(data)
        
bus = can.interface.Bus(channel="vcan0", interface='socketcan')
u = ReadInfo(bus, 0x23, [0x11,0x12,0x13])
print(u.read_from_battery())