# Arduino WS2812B Halloween Effect

Arduino Uno sketch that makes WS2812B strips flicker like worn-out fluorescent tubes that keep trying to light and failing. Uses the [FastLED](https://fastled.io/) library.

- `NUM_TUBES` independent tubes of `TUBE_LEN` pixels each (default: 4 × 75 = 300 pixels total)
- Every tube flickers independently
- Dim by default, so the strips can run from the Uno's 5V pin

## Demo

![Dying fluorescent tubes demo](docs/demo.gif)

## The effect

Each tube runs its own independent cycle:

1. **Dark pause.** The tube body is off; the electrode pixels at each end glow faintly, like embers. This is the only time the two ends and the body can show different things — the body is otherwise always lit uniformly along its whole length. Real tubes don't light up in partial segments, so the sketch never draws one.
2. **A burst of quick flashes.** The whole tube body brightens and dims together, to a random brightness, over a random ramp length, then drops back to dark. This repeats a random number of times.
3. **Occasionally, one attempt catches** (about 1 time in 4 bursts): instead of dropping straight out, the last flash reaches full brightness and holds there briefly before fading.
4. Either way, it goes dark and the cycle returns to the dark pause.

The ramp lengths, flash counts, brightness levels and pause durations are all randomized independently, so no two tubes (or repeats) look quite the same.

## Wiring

**Use a 5V supply only.** WS2812B strips are 5V parts. Connecting a 12V supply to the strip's power pads destroys the pixels.

The strips are chained: the first strip's data input (DIN) connects to the Uno, and the data output (DOUT) of each strip connects to the DIN of the next. With the defaults (`NUM_TUBES 4`, `TUBE_LEN 75`), pixels 0-74 are tube 1, 75-149 are tube 2, 150-224 are tube 3, and 225-299 are tube 4.

### Powered from the Uno's 5V pin

This works at the default brightness (see [Power](#power)).

| From         | To                                      |
| ------------ | --------------------------------------- |
| Uno **5V**   | Strip **V+** (5V)                       |
| Uno **GND**  | Strip **GND**                           |
| Uno **D6**   | First strip **DIN** (via resistor, see below) |

```
  Uno                    Strip 1          Strip 2          Strip 3          Strip 4
 ┌──────┐               ┌────────┐       ┌────────┐       ┌────────┐       ┌────────┐
 │  5V  ├───────────────┤ V+     │       │ V+     │       │ V+     │       │ V+     │
 │  GND ├───────────────┤ GND    │       │ GND    │       │ GND    │       │ GND    │
 │  D6  ├──/\/\/────────┤ DIN    │       │ DIN    │       │ DIN    │       │ DIN    │
 └──────┘  330-470Ω     │    DOUT├───────┤DIN DOUT├───────┤DIN DOUT├───────┤DIN     │
                        └────────┘       └────────┘       └────────┘       └────────┘
```

The diagram shows only the data chain. Power and ground also need to reach each strip (see below).

**Power each strip.** Over a long chain the 5V and GND lines drop voltage, and pixels far from the source get dimmer or change color. Connect V+ and GND to the start of every strip, not only the first. Keep the shared ground between all strips and the Uno.

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

At the default settings, the strips stay close to that budget:

- **Brightness** is low (`MAX_BRIGHTNESS 16` of 255), and the orange tube color is mostly red, which draws the least current.
- **Idle current** is roughly 1 mA per pixel even when dark, so about 300 mA for the 300 pixels total. This floor is the chips' own logic current, not the color output, so it doesn't shrink along with `MAX_BRIGHTNESS`.
- **The current cap** (`MAX_MILLIAMPS 250`) makes FastLED dim the strip automatically if too many pixels are lit brightly at once. It's now below that idle floor, though, so real draw sits close to, or a bit above, the cap even with nothing lit brightly — worth watching if you add more tubes or a brighter color.

If you raise `MAX_BRIGHTNESS`, switch to an external 5V supply and raise `MAX_MILLIAMPS` to match. For scale, 300 pixels at full white draw about 18 A, so size the supply for the brightness you actually use, not the maximum. Also use thicker wires and connect power at each strip, since one long chain can't carry that current.

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
| `NUM_TUBES`        | `4`          | Number of independent tubes.                                                                 |
| `TUBE_LEN`         | `75`         | Pixels per tube. `NUM_LEDS` (total pixel count, can't be detected) is `NUM_TUBES * TUBE_LEN`, set from these two rather than directly. |
| `COLOR_ORDER`      | `GRB`        | If the colors are wrong (e.g. green instead of red), try `RGB` or `BRG`.                     |
| `MAX_BRIGHTNESS`   | `16`         | Brightest the strip ever gets, 0-255. Everything scales to it. Raise it to brighten.         |
| `MAX_MILLIAMPS`    | `250`        | Current cap. FastLED dims the strip to stay under it. See [Power](#power).                   |
| `SPEED_SCALE`      | `1.0`        | Multiplies every pause, ramp and hold duration at once. `2.0` is half speed, `0.5` is double speed. |
| `TUBE_COLOR`       | orange       | Color of the tube body when lit.                                                             |
| `ELECTRODE_COLOR`  | warm orange  | Glow at the ends of each tube.                                                               |
| `ELECTRODE_PIXELS` | `3`          | How many pixels at each end of a tube glow.                                                  |

`SPEED_SCALE` is the quick way to make the whole effect faster or slower. For finer control — e.g. longer pauses without slowing the flashes, or the reverse — edit the `random()` ranges directly in the sketch: the pause length is in `enterPhase()`'s `PAUSE` case, and the flash ramp/hold lengths are in its `RAMP_UP`/`LIT`/`RAMP_DOWN` cases and in `rampTickMs()`.

## License

GPL-3.0. See [LICENSE](LICENSE). The FastLED library it depends on is separately licensed (MIT).
