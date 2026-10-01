import serial
import json
import time
from collections import deque

import matplotlib.pyplot as plt
import matplotlib.animation as animation


# ==================================================
# SETTINGS
# ==================================================

SERIAL_PORT = "COM22"       # Change this
BAUD_RATE = 115200

MAX_POINTS = 60           # Number of samples shown


# ==================================================
# SERIAL
# ==================================================

ser = serial.Serial(
    SERIAL_PORT,
    BAUD_RATE,
    timeout=1
)

time.sleep(2)

print("Connected to", SERIAL_PORT)



# ==================================================
# DATA STORAGE
# ==================================================

times = deque(maxlen=MAX_POINTS)

temperature = deque(maxlen=MAX_POINTS)
humidity = deque(maxlen=MAX_POINTS)
water = deque(maxlen=MAX_POINTS)
battery = deque(maxlen=MAX_POINTS)

acc_x = deque(maxlen=MAX_POINTS)
acc_y = deque(maxlen=MAX_POINTS)
acc_z = deque(maxlen=MAX_POINTS)
acc_mag = deque(maxlen=MAX_POINTS)


start_time = time.time()



# # ==================================================
# # GRAPH SETUP
# # ==================================================

# fig, axes = plt.subplots(
#     3,
#     1,
#     figsize=(10, 10),
#     sharex=True
# )


# # Temperature / humidity

# temp_line, = axes[0].plot([], [], label="Temperature (°C)")
# humidity_line, = axes[0].plot([], [], label="Humidity (%)")

# axes[0].set_ylabel("Temp / RH")
# axes[0].grid(True)
# axes[0].legend()



# # Water / battery

# water_line, = axes[1].plot([], [], label="Water sensor")
# battery_line, = axes[1].plot([], [], label="Battery (V)")

# axes[1].set_ylabel("Value")
# axes[1].grid(True)
# axes[1].legend()



# # Acceleration

# ax_line, = axes[2].plot([], [], label="X")
# ay_line, = axes[2].plot([], [], label="Y")
# az_line, = axes[2].plot([], [], label="Z")
# amag_line, = axes[2].plot([], [], label="Magnitude")

# axes[2].set_ylabel("m/s²")
# axes[2].set_xlabel("Time (s)")
# axes[2].grid(True)
# axes[2].legend()


# ==================================================
# GRAPH SETUP
# ==================================================

# plt.style.use('dark_background')

fig, axes = plt.subplots(
    3,
    1,
    figsize=(10, 10),
    sharex=True
)


# --------------------------------------------------
# Temperature + Humidity
# --------------------------------------------------

ax_temp = axes[0]

temp_line, = ax_temp.plot(
    [],
    [],
    label="Temperature (°C)",
    color="red"
)

ax_temp.set_ylabel(
    "Temperature (°C)"
)

ax_temp.grid(True)


# Create second y-axis
ax_humidity = ax_temp.twinx()


humidity_line, = ax_humidity.plot(
    [],
    [],
    label="Humidity (%)"
)


ax_humidity.set_ylabel(
    "Humidity (%)"
)


# Combine legends
lines = [
    temp_line,
    humidity_line
]

labels = [
    l.get_label()
    for l in lines
]

ax_temp.legend(
    lines,
    labels,
    loc="upper left"
)


# Fixed limits
ax_temp.set_ylim(
    0,
    50
)

ax_humidity.set_ylim(
    0,
    100
)



# --------------------------------------------------
# Water + Battery
# --------------------------------------------------

ax_water = axes[1]


water_line, = ax_water.plot(
    [],
    [],
    label="Water sensor"
)


ax_water.set_ylabel(
    "Water ADC"
)


ax_water.grid(True)



# Second axis
ax_battery = ax_water.twinx()


battery_line, = ax_battery.plot(
    [],
    [],
    label="Battery (V)",
    color="green"
)


ax_battery.set_ylabel(
    "Battery (V)"
)


lines = [
    water_line,
    battery_line
]

labels = [
    l.get_label()
    for l in lines
]


ax_water.legend(
    lines,
    labels,
    loc="upper left"
)



# Fixed limits

ax_water.set_ylim(
    0,
    4095
)


ax_battery.set_ylim(
    3.0,
    4.3
)



# --------------------------------------------------
# Acceleration
# --------------------------------------------------

ax_acc = axes[2]


ax_line, = ax_acc.plot([], [], label="X")
ay_line, = ax_acc.plot([], [], label="Y")
az_line, = ax_acc.plot([], [], label="Z")
amag_line, = ax_acc.plot([], [], label="Magnitude")


ax_acc.set_ylabel(
    "Acceleration (m/s²)"
)

ax_acc.set_xlabel(
    "Time (s)"
)


ax_acc.set_ylim(
    -15,
    15
)


ax_acc.grid(True)

ax_acc.legend(loc="upper left")

# ==================================================
# UPDATE FUNCTION
# ==================================================

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



        t = time.time() - start_time


        times.append(t)


        temperature.append(
            data.get("temperature", None)
        )

        humidity.append(
            data.get("humidity", None)
        )

        water.append(
            data.get("water", None)
        )

        battery.append(
            data.get("battery", None)
        )


        acc_x.append(
            data.get("acceleration_x", None)
        )

        acc_y.append(
            data.get("acceleration_y", None)
        )

        acc_z.append(
            data.get("acceleration_z", None)
        )

        acc_mag.append(
            data.get("acceleration", None)
        )


        print(
            f"{t:.1f}s  "
            f"T={temperature[-1]}°C  "
            f"Acc={acc_mag[-1]}"
        )



        # -----------------------------
        # Update plots
        # -----------------------------

        temp_line.set_data(
            times,
            temperature
        )

        humidity_line.set_data(
            times,
            humidity
        )


        water_line.set_data(
            times,
            water
        )

        battery_line.set_data(
            times,
            battery
        )


        ax_line.set_data(
            times,
            acc_x
        )

        ay_line.set_data(
            times,
            acc_y
        )

        az_line.set_data(
            times,
            acc_z
        )

        amag_line.set_data(
            times,
            acc_mag
        )


        # for ax in axes:
        #     ax.relim()
        #     ax.autoscale_view()


        # Update x-axis window

        if len(times) > 1:
            axes[0].set_xlim(
                max(0, times[-1] - 60),
                times[-1] + 1
            )
            
        # Temperature / humidity
        axes[0].set_ylim(0, 100)

        # Water sensor
        axes[1].set_ylim(0, 4095)

        # Acceleration
        axes[2].set_ylim(-20, 20)



    except json.JSONDecodeError:
        pass


    except Exception as e:
        print("Error:", e)
        
    # return (
    #     temp_line,
    #     humidity_line,
    #     water_line,
    #     battery_line,
    #     ax_line,
    #     ay_line,
    #     az_line,
    #     amag_line
    # )
    
    return (
        temp_line,
        humidity_line,
        water_line,
        battery_line,
        ax_line,
        ay_line,
        az_line,
        amag_line
    )



# ==================================================
# START
# ==================================================

ani = animation.FuncAnimation(
    fig,
    update,
    interval=200,
    blit=True
)


plt.tight_layout()

plt.show()


ser.close()