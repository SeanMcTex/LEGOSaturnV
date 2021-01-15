// Adafruit Soundboard library - Version: Latest
#include <Adafruit_Soundboard.h>
#include <SoftwareSerial.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define NEOPIXEL_PIN 6
#define ROCKET_LED_PIN 9
#define PIXEL_COUNT 120
#define SCALE_AMOUNT 0.5
#define FLICKER_SPEED 5
#define FLICKER_AMOUNT 50
#define POT_PIN 2


// ------ SOUND BOARD CONTROL -------
// Choose any two pins that can be used with SoftwareSerial to RX & TX
#define SFX_TX 12
#define SFX_RX 13

// Connect to the RST pin on the Sound Board
#define SFX_RST 11

// we'll be using software serial
SoftwareSerial ss = SoftwareSerial(SFX_TX, SFX_RX);

// pass the software serial to Adafruit_soundboard, the second
// argument is the debug port (not used really) and the third
// arg is the reset pin
Adafruit_Soundboard sfx = Adafruit_Soundboard(&ss, NULL, SFX_RST);
// can also try hardware serial with
// Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, SFX_RST);
// ------ END SOUND BOARD CONTROL -------

int throttle = 0;       // current throttle for rocket motor
int targetThrottle = 0;       // throttle value we're moving the rocket motor towards
#define THROTTLE_CHANGE 10

int rocket = 0;
int targetRocket = 0;
#define ROCKET_CHANGE 2

unsigned long startTime = 0; // at what time did we start monitoring?
bool soundTriggered = false;

struct RGBW {
  byte r;
  byte g;
  byte b;
};

RGBW colors[] = {
  { 255, 150, 0},    // yellow + white
  { 255, 120, 0},      // yellow + white
  { 255, 90, 0},       // orange
  { 255, 30, 0},       // orangie-red
  { 255, 0, 0},        // red
  { 255, 0, 0}         // extra red
};

int NUMBER_OF_COLORS = sizeof(colors) / sizeof(RGBW);

int percentBetween(int a, int b, float percent) {
  return (int)(((b - a) * percent) + a);
}

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip(PIXEL_COUNT, NEOPIXEL_PIN, NEO_RGB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

float scale = 1;
float inc = 0.1;
float dir = 1.0;
const int arrayLength = round(PIXEL_COUNT / 3);
byte pixelBrightness[arrayLength];
boolean pixelDirection[arrayLength];

// prepopulate the pixel array
void seedArray() {
  for (uint16_t i = 0; i < PIXEL_COUNT; i++) {
    uint16_t p = i % arrayLength;
    pixelBrightness[p] = random(255 - FLICKER_AMOUNT, 255);
    pixelDirection[p] = !!random(0, 1);
  }
}

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

  seedArray();

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // prints title with ending line break
  Serial.println("Starting Execution");

  startTime = millis();

  // ---- INIT SOUND BOARD ----
  // softwareserial at 9600 baud
  ss.begin(9600);
  // can also do Serial1.begin(9600)

  if (!sfx.reset()) {
    Serial.println("Not found");
    while (1);
  }
  Serial.println("SFX board found");
  // ---- END INIT SOUND BOARD ----
}

void loop() {
  // throttle = analogRead(POT_PIN);    // read the value from the sensor

  if ( !soundTriggered ) {
    throttle = 0;
  } else {
    unsigned long timeElapsed = millis() - startTime;
    if ( timeElapsed > 46000 ) {
      targetThrottle = 999;
      targetRocket = 255;
    } else if ( timeElapsed > 35100 ) {
      targetRocket = 0;
      targetThrottle = 600;
    } else if ( timeElapsed > 25100 ) {
      targetRocket = 0;
      targetThrottle = 1022;
    } else if ( timeElapsed > 23100 ) {
      targetRocket = 150;
      targetThrottle = 1022;
    } else if ( timeElapsed > 21100 ) {
      targetRocket = 255;
      targetThrottle = 1022;
    } else if ( timeElapsed > 20100 ) {
      targetRocket = 255;
      targetThrottle = 999;
    } else {
      targetRocket = 255;
      targetThrottle = 0;
    };

    if ( throttle < targetThrottle ) {
      throttle = throttle + THROTTLE_CHANGE;
    }
    if ( throttle > targetThrottle ) {
      throttle = throttle - THROTTLE_CHANGE;
    }

    if ( rocket < targetRocket ) {
      rocket = rocket + ROCKET_CHANGE;
    }
    if ( rocket > targetRocket ) {
      rocket = rocket - ROCKET_CHANGE;
    }
    analogWrite(ROCKET_LED_PIN, rocket);

  }

  if ( (millis() - startTime > 5000) && ( !soundTriggered ) ) {
    sfx.playTrack((uint8_t)0);
    soundTriggered = true;
    startTime = millis();
    Serial.print("playing audio");
  }

  unsigned long currentTime = millis();

  // how many leds for each color
  int ledsPerColor = ceil(PIXEL_COUNT / (NUMBER_OF_COLORS - 1));

  // the scale animation direction
  if (scale <= 0.5 || scale >= 1) {
    dir = dir * -1;
  }

  // add a random amount to inc
  inc = ((float)random(0, 50) / 1000);

  // add the increment amount to the scale
  scale += (inc * dir);

  // constrain the scale
  scale = constrain(scale, 0.5, 1);

  for (uint16_t i = 0; i < PIXEL_COUNT ; i++) {
    uint16_t p = i % arrayLength;

    float val = ((float)i * scale) / (float)ledsPerColor;
    int currentIndex = floor(val);
    int nextIndex = ceil(val);
    float transition = fmod(val, 1);

    // color variations
    if (pixelDirection[p]) {
      pixelBrightness[p] += FLICKER_SPEED;

      // throttle
      // pixelBrightness[p] = pixelBrightness[p] * ( throttle / 1024.0 );

      if (pixelBrightness[p] >= 255) {
        pixelBrightness[p] = 255;
        pixelDirection[p] = false;
      }
    } else {
      pixelBrightness[p] -= FLICKER_SPEED;

      if (pixelBrightness[p] <= 255 - FLICKER_AMOUNT) {
        pixelBrightness[p] = 255 - FLICKER_AMOUNT;
        pixelDirection[p] = true;
      }
    }

    RGBW currentColor = colors[currentIndex];
    RGBW nextColor = colors[nextIndex];
    float flux = (float)pixelBrightness[p] / 255;

    byte r = percentBetween(currentColor.r, nextColor.r, transition) * flux;
    byte g = percentBetween(currentColor.g, nextColor.g, transition) * flux;
    byte b = percentBetween(currentColor.b, nextColor.b, transition) * flux;

    strip.setPixelColor( PIXEL_COUNT - i, g, r, b);
    if ( i > PIXEL_COUNT * ( throttle / 1024.0 ) ) {
      strip.setPixelColor(PIXEL_COUNT - i, 0, 0, 0);
    }

  }

  strip.show();
}
