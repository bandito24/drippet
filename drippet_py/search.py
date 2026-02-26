import serial
import serial.tools.list_ports

# Open connection


ports = serial.tools.list_ports.comports()
for port in ports:
    print(f"{port.device}: {port.description}")
