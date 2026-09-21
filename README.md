# Arduino WS2812B Halloween Effect

Arduino Uno sketch that makes three 1 m WS2812B strips flicker like worn-out fluorescent tubes that keep trying to light and failing. Uses the [FastLED](https://fastled.io/) library.

- 3 tubes, each one 1 m strip of 60 LEDs (180 pixels in total)
- Every tube flickers independently
- Dim by default, so the strips can run from the Uno's 5V pin

## The effect

Each tube loops through the same sequence, with random timing:

1. **Dark pause** (0.4 to 2.5 s), ends smouldering faintly like the electrodes of a real tube.
2. **Failed starts:** 2 to 8 flashes, each lighting a random length of the tube from one end before it drops out.
3. **Almost catches** (about 1 time in 4): the whole tube fades up, holds for a moment, then dies.

## Wiring

**Use a 5V supply only.** WS2812B strips are 5V parts. Connecting a 12V supply to the strip's power pads destroys the pixels.

The three strips are chained: the first strip's data input (DIN) connects to the Uno, and the data output (DOUT) of each strip connects to the DIN of the next. The first 60 pixels are tube 1, the next 60 are tube 2, and the last 60 are tube 3.

### Powered from the Uno's 5V pin

This works at the default brightness (see [Power](#power)).

| From         | To                                      |
| ------------ | --------------------------------------- |
| Uno **5V**   | Strip **V+** (5V)                       |
| Uno **GND**  | Strip **GND**                           |
| Uno **D6**   | First strip **DIN** (via resistor, see below) |

```
  Uno                    Strip 1          Strip 2          Strip 3
 ┌──────┐               ┌────────┐       ┌────────┐       ┌────────┐
 │  5V  ├───────────────┤ V+     │       │ V+     │       │ V+     │
 │  GND ├───────────────┤ GND    │       │ GND    │       │ GND    │
 │  D6  ├──/\/\/────────┤ DIN    │       │ DIN    │       │ DIN    │
 └──────┘  330-470Ω     │    DOUT├───────┤DIN DOUT├───────┤DIN     │
                        └────────┘       └────────┘       └────────┘
```

The diagram shows only the data chain. Power and ground also need to reach each strip (see below).

**Power each strip.** Over a chain of 3 m the 5V and GND lines drop voltage, and pixels far from the source get dimmer or change color. Connect V+ and GND to the start of every strip, not only the first. Keep the shared ground between all strips and the Uno.

Connect the strip's **DIN** end (the input), not DOUT. The arrows printed on the strip point away from the input.

### Powered from an external 5V supply

Use this when you raise the brightness. The strips are powered from the supply, and the Uno only provides data:

| From                       | To                                    |
| -------------------------- | ------------------------------------- |
| Uno **GND**                | External 5V supply **GND**            |
| External 5V supply **GND** | Strip **GND**                         |
| External 5V supply **+5V** | Strip **V+** (5V)                     |
| Uno **D6**                 | First strip **DIN** (via resistor)    |

**The grounds must be common.** The strip reads the data line relative to its own GND, so the Uno, the supply and the strips all have to share ground. Without that link the strip will flicker or do nothing.

Do not also connect the supply's +5V to the Uno's 5V pin.

### Recommended extras

Neither is required for the sketch to work, but they protect the strip:

- **330-470 Ω resistor** in series on the data line, close to the first LED. It damps signal reflections and protects the first pixel. Without it, keep the data wire short. If the first few LEDs flicker, add one.
- **1000 µF capacitor** across the strip's V+ and GND, close to where the power enters. It absorbs the surge when power is first applied. This matters most with an external supply.
- Connect the strip's ground **before** its +5V, and don't hot-plug power onto a strip that is already connected to the Uno.

## Power

The Uno's 5V pin passes on the USB supply, which is limited to about 500 mA in total, including the Uno itself (about 50 mA).

At the default settings, the strips stay within that:

- **Brightness** is low (`MAX_BRIGHTNESS 20` of 255), and the orange tube color is mostly red, which draws the least current.
- **Idle current** is roughly 1 mA per pixel even when dark, so about 180 mA for 180 pixels. The dark parts of the effect are not free.
- **The current cap** (`MAX_MILLIAMPS 300`) makes FastLED dim the strip automatically if too many pixels are lit at once. The sketch counts the idle current in that limit.

If you raise `MAX_BRIGHTNESS`, switch to an external 5V supply and raise `MAX_MILLIAMPS` to match. For scale, 180 pixels at full white draw about 10.8 A, so size the supply for the brightness you actually use, not the maximum. Also use thicker wires and connect power at each strip, since one 3 m chain can't carry that current.

## Software

1. Install the **FastLED** library (Arduino IDE: *Sketch → Include Library → Manage Libraries…*, search "FastLED").
2. Open `led-driver.ino`.
3. Select **Arduino Uno** as the board and your serial port.
4. Upload.

### From the command line

With [`arduino-cli`](https://arduino.github.io/arduino-cli/):

```sh
arduino-cli core install arduino:avr
arduino-cli lib install FastLED

arduino-cli compile --fqbn arduino:avr:uno led-driver
arduino-cli board list                     # find the port, e.g. /dev/cu.usbmodemXXXX
arduino-cli upload -p /dev/cu.usbmodemXXXX --fqbn arduino:avr:uno led-driver
```

On an Apple Silicon Mac, the AVR compiler is an Intel binary and needs Rosetta 2. If the compile fails with `bad CPU type in executable`, run `softwareupdate --install-rosetta --agree-to-license`.

## Configuration

At the top of `led-driver.ino`:

| Define             | Default      | Notes                                                                                        |
| ------------------ | ------------ | -----------------------------------------------------------------------------------------    |
| `DATA_PIN`         | `6`          | Must match the Uno pin wired to DIN.                                                         |
| `NUM_LEDS`         | `60 * 3`     | **Total pixel count of all strips.** It can't be detected, so set it to your real count.     |
| `NUM_TUBES`        | `3`          | Number of independent tubes. Each tube is `NUM_LEDS / NUM_TUBES` pixels. Must divide evenly. |
| `COLOR_ORDER`      | `GRB`        | If the colors are wrong (e.g. green instead of red), try `RGB` or `BRG`.                     |
| `MAX_BRIGHTNESS`   | `20`         | Brightest the strip ever gets, 0-255. Everything scales to it. Raise it to brighten.         |
| `MAX_MILLIAMPS`    | `300`        | Current cap. FastLED dims the strip to stay under it. See [Power](#power).                   |
| `TUBE_COLOR`       | orange       | Color of the tube when it flashes.                                                           |
| `ELECTRODE_COLOR`  | warm orange  | Glow at the ends of each tube.                                                               |
| `ELECTRODE_PIXELS` | `3`          | How many pixels at each end of a tube glow.                                                  |

The flash and pause durations are in `enterPhase()` in the sketch, if you want the flicker faster or slower.
