Kubi: System Architecture & Wiring Blueprint

1. High-Level System Architecture

To ensure Kubi is highly responsive (60fps UI) while doing heavy network tasks (downloading Google Calendar), we will split the workload across the ESP32's two cores.

Core 0 (The Network & Storage Manager)

WiFi Stack: Connects to the apartment WiFi.

Web Server (ESPAsyncWebServer): Serves the pre-compiled React/Tailwind frontend from the LittleFS partition. Listens for POST requests to save settings.

HTTP Client: Wakes up every hour, reaches out to the Google Calendar iCal URL, downloads the .ics string, and parses the next 3 days into a lightweight C++ struct array.

Core 1 (The Hardware & UI Loop)

Sensor Polling: Reads the IMU (I2C) every few milliseconds to detect desk taps, orientation flips, and temperature changes.

State Machine: Decides what mode is active (Clock, Focus, Mascot, Schedule) based on the IMU data.

Display Engine (TFT_eSPI): Uses SPI DMA to push sprite frames or text to the 2" IPS display with zero flickering.

Audio Engine (ESP8266Audio): Streams 8-bit .wav files from LittleFS over I2S to the audio amplifier when a timer ends or a tap is detected.

2. ESP32 Pin Mapping Table

We need to carefully distribute the SPI (Display), I2C (Sensors), and I2S (Audio) buses across the ESP32's GPIO pins to avoid conflicts.

ESP32 Pin (WROOM 30-Pin)

Component

Pin on Component

Description

VIN / 5V

Power / Amp

VCC / VIN

Main 5V power from the battery module

3V3

Display & IMU

VCC / VIN

3.3V Logic power for sensors & screen

GND

ALL

GND

Common Ground (CRITICAL: connect all GNDs)

GPIO 18

Display (SPI)

SCL / CLK

SPI Clock

GPIO 23

Display (SPI)

SDA / DIN

SPI MOSI (Data In)

GPIO 5

Display (SPI)

CS

Chip Select

GPIO 2

Display (SPI)

DC

Data / Command

GPIO 4

Display (SPI)

RST

Reset

GPIO 32

Display (PWM)

BLK

Backlight Control (Allows dimming for sleep)

GPIO 21

IMU (I2C)

SDA

I2C Data (ADXL345 + BMP280)

GPIO 22

IMU (I2C)

SCL

I2C Clock (ADXL345 + BMP280)

GPIO 25

Audio (I2S)

LRC / WSEL

Word Select (Left/Right Clock)

GPIO 26

Audio (I2S)

BCLK

Bit Clock

GPIO 27

Audio (I2S)

DIN

Data In to Amplifier

3. Physical Wiring Strategy (The Protoboard Shield)

To make this durable and fit inside the 10cm³ wooden cube, you will build a "shield" using the protoboard you ordered.

Step 1: The Power Rail

Take the 5V output from your power bank module and solder it to a trace on the edge of the protoboard. This is your Main 5V Rail.

Create a Common Ground (GND) Rail next to it.

Solder the female headers for the ESP32 onto the board. Connect the ESP32's VIN to the 5V rail, and GND to the GND rail.

Create a 3.3V Rail on the board, fed only by the ESP32's 3V3 pin.

Step 2: The Display (SPI)

The Waveshare 2" display uses 3.3V logic. Connect its VCC to your 3.3V Rail and GND to the GND Rail.

Wire DIN (GPIO 23) and CLK (GPIO 18).

Wire CS (GPIO 5), DC (GPIO 2), and RST (GPIO 4).

Wire BLK to GPIO 32. Note: In code, we will set this pin as a PWM output so we can fade the screen to 0% brightness when Wilhelm puts it into sleep mode.

Step 3: The Brains & Brawn (IMU & Audio)

The IMU: Connect the SEN0140's VCC to the 3.3V Rail and GND to the GND Rail. Connect SDA (GPIO 21) and SCL (GPIO 22). Because both the accelerometer and barometer share the I2C bus, you only need these 4 wires!

The Audio Amp: Connect the MAX98357A's VIN directly to the Main 5V Rail (not the 3.3V rail). It needs 5V to push the full 3 Watts to the speaker. Connect GND to the GND Rail.

Wire the I2S pins: LRC to GPIO 25, BCLK to GPIO 26, and DIN to GPIO 27.

Connect the two speaker wires to the screw terminals on the MAX98357A.

Assembly Tip for Resilience

Once you have tested the circuit and confirmed everything works, use a small dab of hot glue at the base of the wires where they solder into the protoboard. This acts as "strain relief" so if the cube gets slammed on the desk, the vibrations won't snap the solder joints.