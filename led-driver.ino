#include <FastLED.h>

#define DATA_PIN 6
#define NUM_LEDS (60 * 3)
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB  // most WS2812B use GRB, not RGB

// Brightest the strip is ever allowed to get, 0-255. Everything scales to this,
// so raise it to make the dying tubes brighter.
#define MAX_BRIGHTNESS 20

// Safety cap on current draw. The Uno's USB port allows 500 mA in total and the board
// itself uses about 50 mA, so 300 mA leaves margin when the strip runs from the 5V pin.
// With an external 5V supply you can raise it (180 pixels at full white is about 10800 mA).
#define MAX_MILLIAMPS 300

// Dying fluorescent tube look
#define NUM_TUBES 3  // the strip is split into this many independent tubes
#define TUBE_LEN (NUM_LEDS / NUM_TUBES)
#define TUBE_COLOR CRGB(0xff, 0x4f, 0)  // tube color
#define ELECTRODE_COLOR CRGB(255, 80, 10)  // warm glow at the tube ends
#define ELECTRODE_PIXELS 3  // how many pixels at each end of a tube glow

static_assert(NUM_LEDS % NUM_TUBES == 0, "NUM_LEDS must divide evenly into NUM_TUBES");

CRGB leds[NUM_LEDS];

// Each tube runs its own sequence, so they flicker independently
enum Phase : uint8_t {
  PAUSE,      // dark, electrodes smouldering
  FLASH_ON,   // part of the tube flashes
  FLASH_OFF,  // it drops out again
  RAMP,       // almost catches: fades up along the whole tube
  HOLD,       // hangs there for a moment
  DIE         // and goes out
};

struct Tube {
  Phase phase;
  uint32_t nextChange;  // millis() at which the phase ends
  uint8_t flashesLeft;
  int litLen;           // pixels of tube light, counted from one end
  uint8_t level;        // brightness of the tube light, 0-255
  uint8_t electrode;    // brightness of the end glow, 0-255
  bool fromEnd;         // count litLen from the far end instead
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

// Start a phase: pick what the tube looks like and how long it lasts
void enterPhase(Tube &t, Phase p) {
  uint32_t ms = 0;
  t.phase = p;

  switch (p) {
    case PAUSE:
      t.litLen = 0;
      t.level = 0;
      t.electrode = random(0, 40);
      t.flashesLeft = random(2, 9);
      ms = random(400, 2500);
      break;
    case FLASH_ON:
      t.litLen = random(TUBE_LEN / 6, TUBE_LEN + 1);
      t.level = random(60, 256);
      t.electrode = 60;
      t.fromEnd = random(2);
      ms = random(15, 90);
      break;
    case FLASH_OFF:
      t.litLen = 0;
      t.level = 0;
      t.electrode = random(0, 40);
      ms = random(20, 180);
      break;
    case RAMP:
      t.litLen = TUBE_LEN;
      t.level = 0;
      t.electrode = 80;
      ms = 10;
      break;
    case HOLD:
      ms = random(80, 250);
      break;
    case DIE:
      t.litLen = 0;
      t.level = 0;
      t.electrode = 0;
      ms = random(60, 200);
      break;
  }

  t.nextChange = millis() + ms;
}

// Move to the next phase once the current one has run its time
void updateTube(Tube &t) {
  if ((int32_t)(millis() - t.nextChange) < 0) return;

  switch (t.phase) {
    case PAUSE:
      enterPhase(t, FLASH_ON);
      break;
    case FLASH_ON:
      enterPhase(t, FLASH_OFF);
      break;
    case FLASH_OFF:
      if (--t.flashesLeft > 0) {
        enterPhase(t, FLASH_ON);
      } else if (random(4) == 0) {  // every so often it almost catches
        enterPhase(t, RAMP);
      } else {
        enterPhase(t, PAUSE);
      }
      break;
    case RAMP:
      if (t.level + 8 > 200) {
        enterPhase(t, HOLD);
      } else {
        t.level += 8;
        t.nextChange = millis() + 10;
      }
      break;
    case HOLD:
      enterPhase(t, DIE);
      break;
    case DIE:
      enterPhase(t, PAUSE);
      break;
  }
}

// Draw one tube into its own TUBE_LEN pixels of the strip
void renderTube(const Tube &t, CRGB *seg) {
  fill_solid(seg, TUBE_LEN, CRGB::Black);

  for (int i = 0; i < t.litLen; i++) {
    CRGB c = TUBE_COLOR;
    c.nscale8(t.level);
    seg[t.fromEnd ? TUBE_LEN - 1 - i : i] = c;
  }

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
