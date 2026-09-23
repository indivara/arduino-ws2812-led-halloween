#include <FastLED.h>

#define DATA_PIN 6
#define NUM_LEDS (TUBE_LEN * NUM_TUBES)
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB  // most WS2812B use GRB, not RGB

// Brightest the strip is ever allowed to get, 0-255. Everything scales to this,
// so raise it to make the dying tubes brighter.
#define MAX_BRIGHTNESS 16

// Safety cap on current draw. The Uno's USB port allows 500 mA in total and the board
// itself uses about 50 mA, so 250 mA leaves margin when the strip runs from the 5V pin.
// With an external 5V supply you can raise it (300 pixels at full white is about 18000 mA).
#define MAX_MILLIAMPS 250

// Dying fluorescent tube look
#define NUM_TUBES 4  // the strip is split into this many independent tubes
#define TUBE_LEN 75  // 60 for 1m at 60 LEDs/m. Change for other types
#define TUBE_COLOR CRGB(0xff, 0x4f, 0)  // tube color
#define ELECTRODE_COLOR CRGB(255, 80, 10)  // warm glow at the tube ends
#define ELECTRODE_PIXELS 3  // how many pixels at each end of a tube glow

// Overall speed of the effect. 1.0 is the original timing, 2.0 is half speed, 0.5 is
// double speed. Everything below is written as if this were 1.0 and then scaled.
#define SPEED_SCALE 1.0

// Scale a duration in milliseconds by SPEED_SCALE
#define scaleMs(ms) ((uint32_t)((ms) * SPEED_SCALE))

static_assert(NUM_LEDS % NUM_TUBES == 0, "NUM_LEDS must divide evenly into NUM_TUBES");

CRGB leds[NUM_LEDS];

// Each tube runs its own sequence, so they flicker independently. The tube body is
// always lit uniformly along its whole length -- never a partial segment, since real
// tubes don't do that -- only its brightness moves. The electrode glow at the ends is
// separate from the body and can be on while the body is dark, or vice versa.
enum Phase : uint8_t {
  PAUSE,      // body dark, electrodes smouldering
  RAMP_UP,    // body brightening towards this attempt's target
  LIT,        // body holds at that brightness, briefly
  RAMP_DOWN   // body dimming back towards dark
};

struct Tube {
  Phase phase;
  uint32_t nextChange;  // millis() at which the phase (or the next step) ends
  uint8_t flashesLeft;  // failed attempts left before the burst gives up
  uint8_t level;        // body brightness, 0-255, uniform along the whole tube
  uint8_t electrode;    // brightness of the end glow, 0-255
  uint8_t targetLevel;  // brightness this ramp is heading to/from
  uint8_t stepsTotal;   // random length (in ticks) of the current ramp
  uint8_t stepsDone;
  bool succeeding;      // this is the attempt that actually catches
};

Tube tubes[NUM_TUBES];

void setup() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS);
  randomSeed(analogRead(A0));  // floating pin: different pattern on every boot

  for (int i = 0; i < NUM_TUBES; i++) {
    enterPhase(tubes[i], PAUSE);
  }

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void loop() {
  dyingTubes();
}

// Random tick interval for a ramp step, kept short and jittery for a nervous flicker
uint32_t rampTickMs() {
  return scaleMs(random(4, 12));
}

// Start a phase: pick what the tube looks like and how long it lasts
void enterPhase(Tube &t, Phase p) {
  uint32_t ms = 0;
  t.phase = p;

  switch (p) {
    case PAUSE:
      t.level = 0;
      t.electrode = random(0, 40);
      t.flashesLeft = random(3, 12);
      ms = scaleMs(random(1500, 6000));  // longer dark stretch between bursts
      break;
    case RAMP_UP:
      t.stepsTotal = random(3, 10);  // random ramp length, short: this is a quick flash
      t.stepsDone = 0;
      t.electrode = 60;
      t.targetLevel = t.succeeding ? 200 : random(50, 170);
      ms = rampTickMs();
      break;
    case LIT:
      // fully lit time is short -- longer only on the rare attempt that actually catches
      ms = t.succeeding ? scaleMs(random(100, 250)) : scaleMs(random(10, 40));
      break;
    case RAMP_DOWN:
      t.stepsTotal = random(3, 10);  // independently random, not the same as the ramp up
      t.stepsDone = 0;
      ms = rampTickMs();
      break;
  }

  t.nextChange = millis() + ms;
}

// Move to the next phase once the current one has run its time
void updateTube(Tube &t) {
  if ((int32_t)(millis() - t.nextChange) < 0) return;

  switch (t.phase) {
    case PAUSE:
      t.succeeding = false;
      enterPhase(t, RAMP_UP);
      break;
    case RAMP_UP:
      if (t.stepsDone < t.stepsTotal) {
        t.stepsDone++;
        t.level = (uint16_t)t.targetLevel * t.stepsDone / t.stepsTotal;
        t.nextChange = millis() + rampTickMs();
      } else {
        enterPhase(t, LIT);
      }
      break;
    case LIT:
      enterPhase(t, RAMP_DOWN);
      break;
    case RAMP_DOWN:
      if (t.stepsDone < t.stepsTotal) {
        t.stepsDone++;
        t.level = (uint16_t)t.targetLevel * (t.stepsTotal - t.stepsDone) / t.stepsTotal;
        t.nextChange = millis() + rampTickMs();
      } else if (t.succeeding) {
        enterPhase(t, PAUSE);  // it caught and has now died down: a proper rest
      } else if (--t.flashesLeft > 0) {
        t.succeeding = false;
        enterPhase(t, RAMP_UP);  // another failed flash in the burst
      } else if (random(4) == 0) {
        t.succeeding = true;
        enterPhase(t, RAMP_UP);  // one more try, and this time it catches
      } else {
        enterPhase(t, PAUSE);  // gives up for now
      }
      break;
  }
}

// Draw one tube into its own TUBE_LEN pixels of the strip. The body is always lit
// uniformly, never a partial segment; only the electrode glow at the ends is separate.
void renderTube(const Tube &t, CRGB *seg) {
  CRGB c = TUBE_COLOR;
  c.nscale8(t.level);
  fill_solid(seg, TUBE_LEN, c);

  CRGB glow = ELECTRODE_COLOR;
  glow.nscale8(t.electrode);
  for (int i = 0; i < ELECTRODE_PIXELS; i++) {
    seg[i] += glow;
    seg[TUBE_LEN - 1 - i] += glow;
  }
}

// Worn-out tubes that keep trying to strike and never stay lit
void dyingTubes() {
  for (int i = 0; i < NUM_TUBES; i++) {
    updateTube(tubes[i]);
    renderTube(tubes[i], &leds[i * TUBE_LEN]);
  }
  FastLED.show();
}
