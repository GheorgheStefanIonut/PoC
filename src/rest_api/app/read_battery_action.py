'''
Authot: Mujdei Ruben 6/2024
Use class ReadInfo to read the informtaion from the different ECUs
How to use?
    u = ReadInfo(bus, 0x23, [0x11,0x12,0x13])
    u.read_from_battery()
'''

from actions import *

class ReadInfo(Action):

    def read_from_battery(self):

        id_battery = self.id_ecu[1]
        id = self.my_id + id_battery

        def _read_by_identifier(identifier):
            self.g.read_data_by_identifier(id,identifier)
            frame_response = self._passive_response(READ_BY_IDENTIFIER,"Error reading data from identifier "+  str(identifier))
            data = self._data_from_frame(frame_response)
            data_str = self._list_to_number(data)
            return data_str

        def algorithm(seed):
            pass

        def _authentication():
            self.g.authntication_seed(id)
            frame_response = self._passive_response(AUTHENTICATION,"Error requesting seed")
            seed = self._data_from_frame(frame_response)
            key = algorithm(seed)
            #key = [0,1,2,3,4]
            self.g.authntication_key(id,key)
            self._passive_response(AUTHENTICATION,"Error sending key")

        try:

            self.g.session_control(id,0x01)
            self._passive_response(SESSION_CONTROL,"Error changing session control")

            _authentication()

            level = _read_by_identifier(0x1234)
            voltage = _read_by_identifier(0x536)
            state_of_charge = _read_by_identifier(0x1d23)
            temperature = _read_by_identifier(0x1aaa)
            life_cycle = _read_by_identifier(0xf332)
            serial_number = _read_by_identifier(0xb55a)

            response_json = self._to_json(level,voltage,state_of_charge,temperature,life_cycle,serial_number)
            # Shutdown the CAN bus interface
            self.bus.shutdown()

            return response_json

        except CustomError as e:

            self.bus.shutdown()
            return e.message
        
    def _to_json(self, level,voltage,state_of_charge,temperature,life_cycle,serial_number):
        """
        Private method to create a JSON response with status and error information.

        Args:
            Data collected from the ECU

        Returns:
        - JSON-formatted response containing status, errors, and timestamp.
        """
        response_to_frontend = {
            "Battery level": level,
            "Voltage": voltage,
            "Battery state of charge": state_of_charge,
            "Temperature": temperature,
            "Life cycle": life_cycle,
            "Serial number": serial_number,
            "time_stamp": datetime.datetime.now().isoformat()
        }
        return json.dumps(response_to_frontend)

bus = can.interface.Bus(channel="vcan0", interface='socketcan')
u = ReadInfo(bus, 0x23, [0x11,0x12,0x13])
print(u.read_from_battery())