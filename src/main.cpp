#include <Arduino.h>
#include <Wire.h>

#include <LIS2DW12Sensor.h>
#include <Adafruit_SHT4x.h>
#include <ArduinoJson.h>

// =====================================================
// PIN DEFINITIONS
// =====================================================

#define WATER_SENSOR_PIN 0
#define BATTERY_ADC_PIN 1

#define LIS_INT1_PIN 3

#define SENSOR_ENABLE_PIN 7

#define I2C_SDA 8
#define I2C_SCL 9

#define RTC_INT_PIN 10

// =====================================================
// SENSOR OBJECTS
// =====================================================

LIS2DW12Sensor accelerometer(&Wire);

Adafruit_SHT4x sht40;

// =====================================================
// RTC SETTINGS
// =====================================================

#define RTC_ADDRESS 0x52

// =====================================================
// SENSOR DATA STRUCTURE
// =====================================================

struct SensorData
{
  float water;

  float battery;

  float ax;
  float ay;
  float az;
  float acceleration;

  float temperature;
  float humidity;

  String timestamp;
};

// =====================================================
// RTC FUNCTIONS
// =====================================================

uint8_t bcdToDecimal(uint8_t value)
{
  return ((value >> 4) * 10) + (value & 0x0F);
}

bool readRTC(String &timestamp)
{

  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(0x00);

  if (Wire.endTransmission(false) != 0)
  {
    return false;
  }

  if (Wire.requestFrom(RTC_ADDRESS, 7) != 7)
  {
    return false;
  }

  uint8_t seconds = bcdToDecimal(Wire.read() & 0x7F);
  uint8_t minutes = bcdToDecimal(Wire.read() & 0x7F);
  uint8_t hours = bcdToDecimal(Wire.read() & 0x3F);

  Wire.read(); // weekday

  uint8_t day = bcdToDecimal(Wire.read() & 0x3F);
  uint8_t month = bcdToDecimal(Wire.read() & 0x1F);
  uint8_t year = bcdToDecimal(Wire.read());

  char buffer[32];

  snprintf(
      buffer,
      sizeof(buffer),
      "20%02d-%02d-%02dT%02d:%02d:%02dZ",
      year,
      month,
      day,
      hours,
      minutes,
      seconds);

  timestamp = String(buffer);

  return true;
}

// =====================================================
// SET RTC FROM SERIAL COMMAND
// Format:
// SETTIME,YYYY-MM-DDTHH:MM:SS
// =====================================================

uint8_t decimalToBCD(uint8_t value)
{
  return ((value / 10) << 4) | (value % 10);
}

bool setRTC(
    int year,
    int month,
    int day,
    int hour,
    int minute,
    int second)
{

  Wire.beginTransmission(RTC_ADDRESS);

  // Start at seconds register
  Wire.write(0x00);

  Wire.write(decimalToBCD(second));
  Wire.write(decimalToBCD(minute));
  Wire.write(decimalToBCD(hour));

  // weekday (not important)
  Wire.write(0x01);

  Wire.write(decimalToBCD(day));
  Wire.write(decimalToBCD(month));

  // RV-3028 stores years since 2000
  Wire.write(decimalToBCD(year - 2000));

  if (Wire.endTransmission() != 0)
  {
    return false;
  }

  return true;
}

void checkSerialRTC()
{

  if (!Serial.available())
  {
    return;
  }

  String command = Serial.readStringUntil('\n');

  command.trim();

  if (!command.startsWith("SETTIME"))
  {
    return;
  }

  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;

  int result = sscanf(
      command.c_str(),
      "SETTIME,%d-%d-%dT%d:%d:%d",
      &year,
      &month,
      &day,
      &hour,
      &minute,
      &second);

  if (result == 6)
  {

    if (setRTC(
            year,
            month,
            day,
            hour,
            minute,
            second))
    {
      Serial.println(
          "RTC updated successfully");
    }
    else
    {
      Serial.println(
          "RTC write failed");
    }
  }
  else
  {
    Serial.println(
        "Invalid time format");
  }
}

void readRTCControl()
{
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x0F);
    Wire.endTransmission(false);

    Wire.requestFrom(RTC_ADDRESS, 2);

    uint8_t ctrl1 = Wire.read();
    uint8_t ctrl2 = Wire.read();

    Serial.print("CTRL1: 0x");
    Serial.println(ctrl1, HEX);

    Serial.print("CTRL2: 0x");
    Serial.println(ctrl2, HEX);
}

void readRTCStatus()
{
    Wire.beginTransmission(RTC_ADDRESS);
    Wire.write(0x0E);
    Wire.endTransmission(false);

    Wire.requestFrom(RTC_ADDRESS, 1);

    uint8_t status = Wire.read();

    Serial.print("RTC STATUS: 0x");
    Serial.println(status, HEX);
}

// =====================================================
// SENSOR READING FUNCTIONS
// =====================================================

float readWater()
{
  return analogRead(WATER_SENSOR_PIN);
}

float readBattery()
{
  /*
     Assumes:

     Battery
        |
       100k
        |
        +---- ADC
        |
       100k
        |
       GND

     Therefore ADC voltage x 2 = battery voltage
  */

  float adcVoltage =
      analogReadMilliVolts(BATTERY_ADC_PIN) / 1000.0;

  return adcVoltage * (4.0/3.0);
}

bool readAccelerometer(SensorData &data)
{

  int32_t accel[3];

  accelerometer.Get_X_Axes(accel);

  // Library gives mg
  constexpr float MG_TO_MS2 = 9.80665 / 1000.0;

  data.ax = accel[0] * MG_TO_MS2;
  data.ay = accel[1] * MG_TO_MS2;
  data.az = accel[2] * MG_TO_MS2;

  data.acceleration =
      sqrt(
          data.ax * data.ax +
          data.ay * data.ay +
          data.az * data.az);

  return true;
}

bool readSHT40(SensorData &data)
{

  sensors_event_t humidity;
  sensors_event_t temperature;

  sht40.getEvent(
      &humidity,
      &temperature);

  data.temperature =
      temperature.temperature;

  data.humidity =
      humidity.relative_humidity;

  return true;
}

bool collectSensors(SensorData &data)
{

  data.water =
      readWater();

  data.battery =
      readBattery();

  readAccelerometer(data);

  readSHT40(data);

  readRTC(data.timestamp);

  return true;
}

// =====================================================
// PRINT FUNCTION
// =====================================================

void printJSON(const SensorData &data)
{
    StaticJsonDocument<512> doc;

    doc["timestamp"] = data.timestamp;

    doc["water"] = data.water;

    doc["battery"] = data.battery;

    doc["acceleration_x"] = data.ax;
    doc["acceleration_y"] = data.ay;
    doc["acceleration_z"] = data.az;
    doc["acceleration"] = data.acceleration;

    doc["temperature"] = data.temperature;
    doc["humidity"] = data.humidity;


    serializeJson(doc, Serial);

    Serial.println();
}

void printData(const SensorData &data)
{

  Serial.println();
  Serial.println("====================");

  Serial.print("Timestamp: ");
  Serial.println(data.timestamp);

  Serial.print("Water sensor: ");
  Serial.println(data.water);

  Serial.print("Battery: ");
  Serial.print(data.battery);
  Serial.println(" V");

  Serial.print("Acceleration X: ");
  Serial.print(data.ax);
  Serial.println(" m/s2");

  Serial.print("Acceleration Y: ");
  Serial.print(data.ay);
  Serial.println(" m/s2");

  Serial.print("Acceleration Z: ");
  Serial.print(data.az);
  Serial.println(" m/s2");

  Serial.print("Acceleration magnitude: ");
  Serial.print(data.acceleration);
  Serial.println(" m/s2");

  Serial.print("Temperature: ");
  Serial.print(data.temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(data.humidity);
  Serial.println(" %");

  Serial.println("====================");
}

// =====================================================
// SETUP
// =====================================================

void setup()
{

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("Landslide sensor starting");

  // -----------------------------
  // Sensor power rail
  // -----------------------------

  pinMode(
      SENSOR_ENABLE_PIN,
      OUTPUT);

  // Turn sensors ON
  digitalWrite(
      SENSOR_ENABLE_PIN,
      LOW);

  delay(100);

  // -----------------------------
  // ADC
  // -----------------------------

  analogReadResolution(12);

  analogSetPinAttenuation(
      WATER_SENSOR_PIN,
      ADC_11db);

  analogSetPinAttenuation(
      BATTERY_ADC_PIN,
      ADC_11db);

  // -----------------------------
  // I2C
  // -----------------------------

  Wire.begin(
      I2C_SDA,
      I2C_SCL);

  Wire.setClock(100000);

  // -----------------------------
  // LIS2DW12
  // -----------------------------

  Serial.println(
      "Initialising LIS2DW12...");

  accelerometer.begin();

  accelerometer.Enable_X();

  // ±2g range
  accelerometer.Set_X_FS(2);

  // 25 Hz sample rate
  accelerometer.Set_X_ODR(25.0);

  Serial.println(
      "LIS2DW12 ready");

  // -----------------------------
  // SHT40
  // -----------------------------

  if (!sht40.begin())
  {
    Serial.println(
        "SHT40 not detected");
  }
  else
  {
    Serial.println(
        "SHT40 ready");
  }

  // -----------------------------
  // RTC
  // -----------------------------

  String timestamp;

  if (readRTC(timestamp))
  {
    Serial.print(
        "RTC time: ");

    Serial.println(timestamp);
  }
  else
  {
    Serial.println(
        "RTC read failed");
  }
}

// =====================================================
// LOOP
// =====================================================

void loop()
{

  checkSerialRTC();

  static uint32_t lastSample = 0;

  if (
      millis() - lastSample >= 1000)
  {

    lastSample = millis();

    SensorData data;

    collectSensors(data);

    // printData(data);
    printJSON(data);

    // readRTCControl();
    // readRTCStatus();
  }
}