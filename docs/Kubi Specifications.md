Project Kubi: 21st Birthday Desk Companion (ESP32 Edition)

Recipient: Wilhelm (W)
Deadline: November 16th (73 Days Remaining)
Core Concept: A physical, interactive desk companion cube designed to assist with productivity and inject a bit of personality into the workspace, relying entirely on physical manipulation rather than buttons.

1. Hardware & Physical Build

Dimensions: ~10cm x 10cm x 10cm (10cm³). Exterior made of custom-built wood (e.g., hardwood). Internal volume of roughly 7.5cm³ depending on wood thickness (leaving plenty of room for all components).

Enclosure: Custom wooden shell with a 3D-printed internal chassis for mounting components.

The Brain: ESP32-WROOM-32 Dev Board (30-pin). Dual-core 240MHz, built-in Wi-Fi, and 4MB of flash memory (perfect for storing the web app on LittleFS).

Display: 2-inch Full-Color IPS LCD (240x320 Resolution, SPI Interface). Gives plenty of pixel density for crisp text (schedules) and colorful pixel art, while providing near 180-degree viewing angles so colors don't wash out when viewed from above.

Power:

Battery: 3x high-capacity 18650 lithium-ion cells wired in parallel (~10,000mAh total). With the ESP32's low power draw, this will run for several days unplugged.

Management: A power-bank module (e.g., IP5328P or IP5306) providing pass-through charging via USB-C to protect battery health when plugged in long-term.

Storage (Reliability): No SD Card required! The ESP32's internal 4MB Solid State Flash Memory will be partitioned using LittleFS. This prevents the corruption issues common with Raspberry Pi OS.

Sensors:

DF Robot 10 DOF IMU Sensor (SEN0140): A single breakout board on the I2C bus containing the ADXL345 (Accelerometer for orientation and taps) and the BMP280 (Temperature).

Audio:

Amplifier: MAX98357A I2S 3W Class D Amplifier Breakout.

Speaker: 3W 8Ω Enclosed Speaker (provides a built-in acoustic chamber for punchier, clearer 8-bit chimes).

2. Interaction Design (The "No Button" Rule)

All UI navigation is handled purely via the ADXL345 accelerometer detecting physical movements.

Gestures:

Shake: Snooze a reminder, pause/reset a timer, or exit a mode.

Gentle Tap (Software Detection): Wakes the screen from night mode, toggles UI elements depending on the active face, silences a chime, or stops a reminder.

Violent Slam (Software Detection): A fun easter egg to detect when W rage-slams his desk, triggering an "ouch" response from the mascot.

3. Operating Modes (Orientation-Based)

Depending on which face of the cube is facing "up," the screen (which stays on the front face) rotates or switches modes.

Face 1 Up: Default Idle / Focus Clock Mode

Super minimal, distraction-free display: just the current time on a black screen.

Tap Interaction: The time slides up and shrinks. The screen reveals W's next scheduled Google Calendar item (Event Name, Start Time, End Time).

Financial Ticker: Beneath the schedule item, a scrolling financial ticker appears (can be toggled on/off in the web app settings).

Face 2 Up: Pomodoro Timer

The cube is rested on its side. The screen rotates to remain upright.

Starts a focus timer automatically.

The web app allows W to configure specific durations for the Pomodoro focus time, short breaks, and long breaks.

Emits an 8-bit chime when the time is up. Tapping silences the chime, shaking pauses/resets it.

Face 3 Up: Environmental Stats & Kubi's Routine

Displays the current room temperature (pulled from the BMP280).

Kubi (the jelly cube mascot) occupies the screen, engaging in simple, static idle animations based on the time of day (sleeping, drinking coffee, reading).

Face 4 Up: Schedule & Reminders

Displays upcoming agenda items for the next 3 days.

Tap Interaction: Tapping the cube cycles the displayed schedule by user-defined tags (if any) or simply pages through the list.

Night Mode / Sleep Cycle

Configured via the web app on a static schedule.

Screen backlight drops to 0% (off) to save battery and reduce light pollution.

A tap instantly wakes the screen back to full brightness for 30 seconds.

4. Software & Network (C++ / ESP32)

Core Logic: Written in C++ using the Arduino framework via PlatformIO.

Core 0: Handles Wi-Fi, ESPAsyncWebServer, and HTTPClient requests (fetching Google Calendar).

Core 1: Handles the main loop(), rendering graphics (TFT_eSPI), playing audio (ESP8266Audio), and polling the IMU.

Companion Web App:

Frontend built with React and TailwindCSS. It compiles down to static HTML/JS/CSS, which is gzipped and uploaded to the ESP32's LittleFS partition.

Served via ESPAsyncWebServer locally (accessed via kubi.local via mDNS).

Persistence: User settings (Pomodoro times, sleep cycle, iCal URL) are saved to a settings.json file on LittleFS.

The "Hidden" API: A secret local POST endpoint (/api/override) that allows the creator (brother) to push custom text alerts to the screen.

5. Google Calendar Integration Strategy

Wilhelm manages his schedule in Google Calendar.

He pastes his Secret iCal Address (from Google Calendar settings) into the Kubi Web App.

The React App sends this URL to the ESP32, which saves it to LittleFS.

Every hour, the ESP32 uses HTTPClient to download the raw .ics text file.

A lightweight C++ parser strips out VEVENT, DTSTART, DTEND, and SUMMARY for the next 3 days, storing them in a C++ struct array in RAM for instant display.

6. Budget Estimation (Realistic ZAR)

ESP32-WROOM Devboard: R0 (Already owned)

2" IPS LCD (SPI): ~R200 (Waveshare/Micro Robotics)

Battery Powerbank Module: ~R80 (DIY Electronics)

3x 18650 Cells: ~R255 (Micro Robotics)

DF Robot 10 DOF IMU (SEN0140): R258 (Micro Robotics)

MAX98357A I2S Amp: R124 (Takealot 2-pack split)

Enclosed Speaker (3W 8Ω): R78 (Micro Robotics)

Perfboard, Headers, Wire: ~R100

Wood, filament, brass inserts: ~R150

Estimated Grand Total: ~R1,245.00

7. Physical Design & Durability

Because Kubi relies on physical manipulation, internal stability is critical.

The Motherboard Shield: The ESP32 will plug into female headers soldered onto a custom perfboard. All sensors and the screen will wire directly into this perfboard, eliminating loose wires.

The Internal Chassis: Use a single 3D-printed internal skeleton (PETG).

Heat-Set Brass Inserts: Melt brass threaded inserts into the 3D chassis to bolt down the perfboard securely using M2 machine screws.

Protecting the Display: Sandwhich the IPS screen between a thin acrylic window and the 3D chassis using EVA foam tape to absorb shock from desk slams.

Wiring & Connections: Use highly flexible silicone-sheathed wire for all internal routing.

8. PlatformIO Configuration (platformio.ini)

To ensure the ESP32 allocates enough flash memory for your React app (LittleFS) and has the right libraries, use this exact configuration in VS Code:

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
board_build.filesystem = littlefs
; Custom partition table: 2MB App, 1.5MB LittleFS (for Web App), 512KB OTA/Data
board_build.partitions = default_8MB.csv ; Or create a custom partitions.csv if using 4MB

lib_deps = 
    bodmer/TFT_eSPI@^2.5.31          ; Extremely fast display driver
    bblanchon/ArduinoJson@^6.21.3    ; JSON parsing for settings
    me-no-dev/ESP Async WebServer@^1.2.3 ; Async Web Server
    earlephilhower/ESP8266Audio@^1.9.7 ; Audio decoding/I2S
    adafruit/Adafruit_ADXL345@^1.3.2 ; Accelerometer
    adafruit/Adafruit_BMP280@^2.6.8  ; Temperature
