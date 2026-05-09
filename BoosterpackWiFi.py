import serial # type: ignore
from time import sleep
import struct
import socket
import sys

COM_PORT = 'COM5'  # Change this to your actual COM port
BAUD_RATE = 115200    # Set the baud rate

SERVER_ADDRESS = sys.argv[1]
PORT = int(sys.argv[2])

with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client_socket:
    # Open the serial port
    # Replace 'COM1' with your port name (e.g., '/dev/ttyUSB0' on Linux)
    try:
        ser = serial.Serial(COM_PORT, baudrate=BAUD_RATE, timeout=1) 
        # print(f"Opened port: {ser.name}")

        motor_data = bytearray([0xFF, 0x00, 0x7F, 0x7F])  # Example motor command data
        # first byte: start byte (0xFF)
        # second byte: header byte (just 0x0)
        # the next 6 bytes set neutral speed for left motors, right motors

        while True:
            data = ser.read(4)

            if len(data) > 0 and data[0] == 0xff:
                motor_data = data[:len(motor_data)]
            print(data)

            client_socket.sendto(motor_data, (SERVER_ADDRESS, PORT))
            client_socket.setblocking(False)

            try:
                data, _ = client_socket.recvfrom(1024)
            except:
                print("No data received")
                continue

            if len(data) > 0:
                try:
                    print(data)
                    ser.write(data)
                    unpacked_data = struct.unpack('<BBfffffff', data)
                    print(f"Received: {unpacked_data}")
                except:
                    print(f"Invalid packet:", bytes(data))
            else:
                print("No data received")

    except serial.SerialException as e:
        print(f"Error opening or communicating with serial port: {e}")

    finally:
        # Close the port if it was opened
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Serial port closed.")