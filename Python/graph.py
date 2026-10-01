import serial
import json
import time
from collections import deque

import matplotlib.pyplot as plt
import matplotlib.animation as animation


# ==========================
# SETTINGS
# ==========================

SERIAL_PORT = "COM22"     # Change this
BAUD_RATE = 115200

MAX_POINTS = 120          # Show last 2 minutes


# ==========================
# SERIAL CONNECTION
# ==========================

ser = serial.Serial(
    SERIAL_PORT,
    BAUD_RATE,
    timeout=1
)

time.sleep(2)

print("Connected to", SERIAL_PORT)


# ==========================
# DATA STORAGE
# ==========================

times = deque(maxlen=MAX_POINTS)
temperatures = deque(maxlen=MAX_POINTS)

start_time = time.time()



# ==========================
# GRAPH SETUP
# ==========================

fig, ax = plt.subplots()

line, = ax.plot([], [])

ax.set_title("Temperature")
ax.set_xlabel("Time (s)")
ax.set_ylabel("Temperature (°C)")

ax.grid(True)



# ==========================
# UPDATE FUNCTION
# ==========================

def update(frame):

    try:

        serial_data = ser.readline()

        if not serial_data:
            return


        text = serial_data.decode(
            "utf-8",
            errors="ignore"
        ).strip()


        if not text.startswith("{"):
            return


        data = json.loads(text)


        temperature = data.get(
            "temperature"
        )


        if temperature is None:
            return


        elapsed = time.time() - start_time


        times.append(elapsed)
        temperatures.append(temperature)


        print(
            f"{elapsed:.1f}s : {temperature:.2f} °C"
        )


        # Update plot

        line.set_data(
            times,
            temperatures
        )


        ax.relim()
        ax.autoscale_view()


    except json.JSONDecodeError:
        pass


    except Exception as e:
        print("Error:", e)



# ==========================
# START LIVE PLOT
# ==========================

ani = animation.FuncAnimation(
    fig,
    update,
    interval=200
)


plt.show()


# Cleanup

ser.close()