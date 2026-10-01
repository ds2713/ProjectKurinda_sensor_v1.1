import serial
import datetime
import time


PORT = "COM22"     # change this
BAUD = 115200


ser = serial.Serial(
    PORT,
    BAUD,
    timeout=1
)


time.sleep(2)


now = datetime.datetime.utcnow()


command = (
    f"SETTIME,"
    f"{now.year:04d}-"
    f"{now.month:02d}-"
    f"{now.day:02d}T"
    f"{now.hour:02d}:"
    f"{now.minute:02d}:"
    f"{now.second:02d}\n"
)


print("Sending:")
print(command)


ser.write(command.encode())


response = ser.readline().decode().strip()

print(response)