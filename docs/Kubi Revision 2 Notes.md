# Kubi Revision 2 Notes

Things the first cube taught us, to fix in the next build. Kubi 1 stays as it
is: it's Wilhelm's, and it works. Ordered roughly by how much each one matters.

## Power

- **Charger signal to the ESP32.** Kubi can't see the TP4056, so after every
  restart he has to guess from the voltage trend whether he's charging (up to
  30 minutes of "Checking if I'm charging..."). Wire the TP4056's CHRG pin
  (low while charging) to GPIO 35 and STDBY (low when full) to GPIO 39. Both
  are input-only pins with no internal pull-ups: add 10 kΩ pull-ups to 3.3 V.
  The board's LEDs pull those pins towards the USB's 5 V, so check the voltage
  at the pin with USB in and charging finished, and add a series diode or a
  small transistor if it goes above 3.3 V. Then `BatteryMonitor` takes the
  state from the pins and the voltage only gives the percentage.
- **A power switch.** There is room for it in the OUT+ wire between the TP4056
  and the MT3608 (wiring guide). Without one the only "off" is the nap, and a
  cube in a box keeps draining.
- **Battery life.** Kubi draws about 0.3 A and lasts about 15 hours. The
  biggest waste is boosting 3.7 V up to 5 V with the MT3608 and then dropping
  it to 3.3 V in the DevKit's linear regulator: only about 60 % arrives. Feed
  the 3.3 V rail from a buck-boost converter straight off the cells.
- **Amp off between chimes.** Wire the PAM8403's shutdown pin to a GPIO so the
  amp (about 10 mA idle) is off when nothing plays. Mind the start-up pop and
  the brownout history in AGENTS.md (audio hardware) when it switches on.

## Sensors

- **Temperature.** The BMP280 sits inside the closed cube and reads the
  electronics' own heat (about 37 °C on the scene face, worse while
  charging). Put the sensor at a vent, on a short lead away from the TP4056
  and the boost converter, or measure the offset and correct it in firmware.

## Build and service

- **Flashing over USB.** The DevKit in the cube doesn't enter download mode by
  itself ("Wrong boot mode 0x13"), so USB flashing needs the BOOT/EN button
  dance. Pick a board whose auto-reset works, or make the buttons reachable.
- **Battery holder access.** The cells were very hard to get in, and the
  speaker leads were cut too short to work around them comfortably. Leave
  slack in the speaker wires (or use plug-in connectors) and make the holder
  reachable without stressing anything.
- **The ESP32's USB port.** It only powers the board (the battery side wins on
  the 5 V rail and it never charges the cells). Either make that clear on the
  case or route only the USB-C charger to the outside.

## If we build more than one (client desk toys)

Shae's uncle and aunt asked about 10 cubes for clients (2026-11-20). Before
any cube leaves the family:

- **Remove the personal bits.** Wilhelm's birthday, the Birthday and Christmas
  scenes as he has them, the "Happy 21st" manual, and the hidden Creator panel.
- **Security.** The dashboard API and OTA updates have no password: anyone on
  the same WiFi can change settings, put up banners or flash new firmware.
  Fine at home, not on a client's office network. Add an OTA password and an
  owner PIN for the API at the very least.
- **Setup for strangers.** The Kubi-Setup hotspot flow works, but a client
  needs a printed card with the steps, and a way to get help.
- **A custom PCB** instead of hand-wired perfboard, so ten cubes come out the
  same and take hours, not days.
- **Cost, enclosure time, and shipping lithium cells** need working out before
  quoting a price.
