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

Audio (DAC)

PAM8403 input L

Left channel, analog from the ESP32's built-in DAC

GPIO 26

Audio (DAC)

PAM8403 input R

Right channel, analog

(No MAX98357A was in stock, so audio uses the ESP32's own DAC into a PAM8403 analog amp, build flag KUBI_AUDIO_INTERNAL_DAC in platformio.ini. With a MAX98357A instead, remove the flag and wire I2S: LRC to GPIO 25, BCLK to GPIO 26, DIN to GPIO 27.)

3. Physical Wiring Strategy (The Protoboard Shield)

To make this durable and fit inside the 10cm³ wooden cube, you will build a "shield" using the protoboard you ordered.

Step 1: The Power Rail

Power chain (decided 12 Oct; the DFRobot DFR0969 board is 104 mm long and only fits a cube of ~120 mm, so it is not used):

  2 x INR18650-26E in a bare holder, wired in PARALLEL (reads ~3.7 V, not 7.4 V)
    -> TP4056 USB-C charger board, protected version (B+ / B- to the cells, OUT+ / OUT- to the load; its USB-C is the cube's charging port)
    -> MT3608 boost board, set to 5.0 V
    -> perfboard 5V / GND

Set the MT3608 to 5.0-5.1 V with a multimeter BEFORE anything is connected to its output: these boards often ship set to 12 V or more, which kills the ESP32 and the amp instantly. Turn the brass trimmer while measuring OUT+ to OUT-, powered from the cells.

The TP4056's own over-discharge cut-off is 2.5 V, which is deep for an 18650; the firmware will shut down at ~3.3 V using the battery-sense divider (Step 4). Use two identical cells from the same batch at the same charge.

Not fitted yet: a power switch (slide switch in the TP4056 OUT+ to MT3608 IN+ wire, so the cube can be turned off while it still charges), and load sharing (P-MOSFET + Schottky + 100k, so the cube runs from USB while plugged in instead of holding the cells at 4.2 V). Leave a spot on the back wall for the switch.

The MT3608 output is your Main 5V Rail.

Create a Common Ground (GND) Rail next to it.

Solder the female headers for the ESP32 onto the board. Connect the ESP32's VIN to the 5V rail, and GND to the GND rail.

Create a 3.3V Rail on the board, fed only by the ESP32's 3V3 pin. Never also feed it from the holder's 3.3V output: two regulators on one rail fight.

Step 2: The Display (SPI)

The Waveshare 2" display uses 3.3V logic. Connect its VCC to your 3.3V Rail and GND to the GND Rail.

Wire DIN (GPIO 23) and CLK (GPIO 18).

Wire CS (GPIO 5), DC (GPIO 2), and RST (GPIO 4).

Wire BLK to GPIO 32. Note: In code, we will set this pin as a PWM output so we can fade the screen to 0% brightness when Wilhelm puts it into sleep mode.

Step 3: The Brains & Brawn (IMU & Audio)

The IMU: Connect the SEN0140's VCC to the 3.3V Rail and GND to the GND Rail. Connect SDA (GPIO 21) and SCL (GPIO 22). Because both the accelerometer and barometer share the I2C bus, you only need these 4 wires!

The Audio Amp (PAM8403): Connect "power +" directly to the Main 5V Rail (not the 3.3V rail) and "power -" to the GND Rail.

Wire the inputs: L to GPIO 25, R to GPIO 26, and G to the GND Rail.

Speakers: one speaker on "lout +" / "lout -", the other on "rout +" / "rout -". Never connect a speaker wire to GND: both outputs are driven (bridge-tied load). The volume knob sits in front of the amp; start low, the ESP32's DAC is louder than the I2S path was tuned for.

Step 4: Battery Sense (for the battery percentage)

The holder does not report its charge, so the ESP32 measures the cell voltage itself through a divider that halves it (4.2 V full becomes 2.1 V, inside the ADC's range):

- R1, 10 kΩ, from the cells' + terminal (the bare holder's + lead, both cells share it) to a sense node.
- R2, 10 kΩ, from the sense node to GND.
- C1, 100 nF ceramic, from the sense node to GND next to the ESP32 pin: optional. 10 kΩ is low enough for the ADC to sample on its own; the capacitor only steadies it further. (100 kΩ would draw less but needs the capacitor.)
- Sense node to GPIO 34 (an ADC1 pin, which keeps working while WiFi is on; ADC2 pins do not; input-only, which is all this needs).

Solder R1 directly onto the holder's + lead (or its solder tab), so the wire that runs to the perfboard is already behind 10 kΩ: if that wire ever chafes through to GND, at most 0.4 mA flows. A bare wire from battery + across the cube would be an unfused short waiting to happen. The divider draws about 0.2 mA (4 mAh a day), negligible next to the cube's ~150 mA.

Until this is wired, GPIO 34 floats and the firmware keeps reporting 100%; the reading code is added once the divider exists (TSK-391).

4. Cube Layout (90 mm, printed plastic, 3 mm walls, 84 mm inside)

The cube rolls around the front-to-back axis: the screen is on the front face, the charging port on the back face, and the four faces it rests on are around them. Directions below are with the cube on Face 1 (Clock), screen towards you.

| Zone (depth from the front wall) | What sits there |
|---|---|
| Front wall | Screen window centred. One laptop speaker either side of the screen, firing forward through grille holes (60 x 15 mm each; the side bands are ~24 mm wide). The front face never rests on the desk, so the speakers are never muffled; in landscape faces they end up above and below the screen. |
| 0-12 mm | Screen module (58 x 35 mm, portrait) and its 8-wire plug. Keep using the plug and cable: no header pins needed on the screen. |
| ~14-35 mm | Battery holder (78 x 41 x 21 mm), 78 mm side to side, lying flat behind the screen and centred, so the heaviest part sits on the roll axis and the cube rolls evenly. |
| ~40-60 mm | Perfboard with the ESP32 and the IMU, parallel to the back wall, screwed or glued to bosses. The IMU must not be able to move relative to the screen. |
| Back wall, inside | TP4056 (USB-C through a slot in the back wall), MT3608 (37 x 17 x 14 mm), PAM8403 with its volume knob kept inside (the sides are removable, so it is set once, behind a panel). Vent slots near the TP4056: it dissipates ~1.3 W while charging at 1 A. Room reserved for a future power switch. |

Route the screen cable around the battery holder (there is ~21 mm above and below it). Keep wires away from the ESP32's antenna end.

5. Perfboard (40 x 60 mm, 14 x 20 holes)

Holes are named by the coordinates printed on the board: columns A-T along the 60 mm side, rows 01-14 along the 40 mm side, so "O12" is column O, row 12. Component side (the printed side) up, column A on the left, row 01 at the bottom:

```
        A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T
   14   d  d  d  d  d  d  d  d  .  i  i  i  i  .  a  a  a  .  .  .
   13   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
   12   S  S  S  S  S  S  S  S  S  S  S  S  S  S  S  .  .  .  .  .    S = ESP32 socket, side with D23 ... 3V3
   11   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
   10   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
   09   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
   08   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
   07   .  .  .  r  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
   06   .  .  .  r  .  .  .  .  .  .  .  .  .  .  .  .  .  .  Bt .
   05   .  .  .  r  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
   04   .  .  .  r  .  .  .  .  .  .  .  .  .  .  .  .  .  .  5a Ga
   03   .  .  .  r  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
   02   S  S  S  S  S  S  S  S  S  S  S  S  S  S  S  .  .  .  5i Gi    S = ESP32 socket, side with EN ... VIN
   01   .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
```

When you turn the board over to solder, turn it side to side like a page: row 01 stays at the bottom, and the letters then run T to A from left to right.

- ESP32: two 15-way female headers (cut down from longer strips), pins in columns A-O, antenna end at column A hanging off the board edge, USB-C end towards column O. Rows 02 and 12: the two pin rows are 10 holes (25.4 mm) apart, measured on the board in use. With the chip facing up and the antenna on the left, the EN ... VIN row is the one nearest you, so it goes in row 02. The ESP32's PCB overhangs to about column R at the USB end and reaches about halfway to rows 01 and 13, so nothing tall goes in columns P-R or under it.
- Socket pin order, column A to O, confirmed against the pinout of the board in use (30-pin ESP32 DevKit, ESP32-D0WD-V3 module, CH340 USB-C):
  - Row 02: EN, VP (GPIO36), VN (GPIO39), D34, D35, D32, D33, D25 (DAC1), D26 (DAC2), D27, D14, D12, D13, GND, VIN
  - Row 12: D23 (MOSI), D22 (SCL), TX0, RX0, D21 (SDA), D19, D18 (SCK), D5, TX2, RX2, D4, D2, D15, GND, 3V3
- Male headers can be made up from short pieces placed end to end.
- d, A14-H14: 8 male header pins for the screen cable, in the order printed on the screen: VCC, GND, DIN, CLK, CS, DC, RST, BL. The cable is wired straight through; on the cable in use the colours are purple (VCC, on A14), white, green, orange, yellow, blue, brown, grey (BL, on H14).
- i, J14-M14: 4-way female header for the IMU (DFRobot 10DOF V2). Its pins are straight, so it lies flat in the socket with its body overhanging the board's row-14 edge by about 22 mm, away from the ESP32 (the two sit at the same height and would collide the other way round). Plugged in that way its pins land as J14 SDA, K14 SCL, L14 GND, M14 VIN. Its pin labels read VIN, GND, SCL, SDA; a different IMU board needs its own check of the order and which way it can lie.
- a, O14-Q14: solder points for the wires to the PAM8403 input: L, R, G.
- Columns S-T: solder points. 5i / Gi (S2 / T2) = 5 V and GND in from the MT3608. 5a / Ga (S4 / T4) = 5 V and GND out to the PAM8403 power pins. Bt (S6) = battery sense in, the wire from R1.
- Power, amp and battery-sense wires are soldered straight into these holes rather than plugged onto header pins: a cube that gets rolled and knocked shakes plugs loose. Only the screen cable (it has its own plug) and the IMU (in its socket) stay removable. Leave these holes empty while wiring the rest: the outside wire goes through first, and the board wire is soldered to its end on the solder side.
- r, D3-D7: R2 lying flat under the ESP32, its D3 leg bent over on the solder side and soldered to D2 (the D34 socket pin), which makes the bridge. Only flat parts here: the ESP32 sits ~10 mm above the board.

Wires on the solder side, in the order they were soldered (tick off while soldering):

| From | To | Note |
|---|---|---|
| N2 GND | N12 GND, D7 (R2) | ground |
| N12 GND | L14 (IMU GND) | ground |
| L14 | B14 (screen GND) | ground, run along the empty row 13 |
| N2 | T2 Gi, then T2 to T4 Ga | ground, once the pigtails are in |
| L14 | Q14 (amp input G) | ground, once the pigtail is in |
| O12 3V3 | M14 (IMU VIN), then M14 to A14 (screen VCC) | the only source on the 3.3 V rail; only one wire on O12, because its neighbour N12 is GND |
| O2 VIN | S2 5i, then S2 to S4 5a | 5 V |
| A12 D23 / G12 D18 | C14 DIN / D14 CLK | SPI |
| H12 D5 / L12 D2 / K12 D4 | E14 CS / F14 DC / G14 RST | GPIO 2 is a boot pin: hold BOOT if a USB flash fails |
| F2 D32 | H14 BL | backlight PWM |
| E12 D21 / B12 D22 | J14 SDA / K14 SCL | I2C: ADXL345 0x53, BMP280 0x77 |
| H2 D25 / I2 D26 | O14 L / P14 R | DAC audio |
| D2 D34 | D3 (R2's own leg), S6 Bt | battery sense node |

The screen wires pass over the row-13 ground wire: solder at the tip of each stub, keep the iron off the ground wire's insulation, and beep each new wire against GND straight away.

Off-board wiring:

| From | To | Note |
|---|---|---|
| Holder + / - | TP4056 B+ / B- | keep short; 26 AWG stranded is fine at these currents (up to 1 A while charging) |
| Holder + | R1 at the holder, then a wire to perfboard S6 | R1 at the battery end |
| TP4056 OUT+ / OUT- | MT3608 IN+ / IN- | power switch goes in the OUT+ wire later |
| MT3608 OUT+ / OUT- | perfboard S2 / T2 | set to 5.0 V first |
| Perfboard S4 / T4 | PAM8403 power + / - | |
| Perfboard O14 / P14 / Q14 | PAM8403 input L / R / G | |
| PAM8403 lout + / - | left speaker | never to GND |
| PAM8403 rout + / - | right speaker | never to GND |
| Screen cable | perfboard A14-H14 | VCC (purple) on A14 |

Mounting: on the mid-plate, component side towards the back plate, row 14 (the screen and IMU headers) towards the top of the cube on Face 1, the side away from the TP4056. The flat IMU then reaches to within about 0.5 mm of the top panel, which is why every side panel has two shallow pockets on its inside. Glue the board a millimetre or two away from that side for extra room. The other way up, the IMU would sit about 1 mm under the TP4056.

Parts: two 15-way female headers, one 4-way female header, 8 male header pins (for the screen cable), R1 and R2, hook-up wire (26 AWG stranded throughout; stranded also survives a cube being rolled and knocked better than solid core). C1 (100 nF ceramic) is optional: with 10 kΩ resistors the divider is stiff enough for the ADC, and the firmware averages many readings. As built, R1 measures 9.28 kΩ and R2 9.42 kΩ, so the divider ratio is 0.504 (use it in TSK-391).

After assembly, the sensor's position relative to the screen has changed from the breadboard, so rerun firmware/tools/capture_faces.ps1 and update FaceMap.h (TSK-768).

Assembly Tip for Resilience

Once you have tested the circuit and confirmed everything works, use a small dab of hot glue at the base of the wires where they solder into the protoboard. This acts as "strain relief" so if the cube gets slammed on the desk, the vibrations won't snap the solder joints.