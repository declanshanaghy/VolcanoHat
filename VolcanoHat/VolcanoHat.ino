#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN_ERUPT 2
#define PIN_PUMP 3  //TODO: Change to another PWM pin (5 probably)
#define PIN_LEDS 6
#define PIN_SND_RUMBLE 7
#define PIN_SND_ERUPT A0
#define PIN_VAPE 8

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

// Zero based indexes
#define N_SEGMENTS 2
#define SEG_BEGIN 0
#define SEG_END 1
uint8_t segments[N_SEGMENTS+1][2] = {
  {0, 15},    //15
  {16, 35},   //19
  {36, 47}    //11
};
Adafruit_NeoPixel strip = Adafruit_NeoPixel(
  segments[N_SEGMENTS][SEG_END]+1, 
  PIN_LEDS, 
  NEO_GRB + NEO_KHZ800);
  
#define ORANGE strip.Color(255, 140, 0)
//#define ORANGE strip.Color(0, 0, 255)
#define RED strip.Color(255, 0, 0)

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

volatile bool erupt = false;
volatile unsigned long allowErupt = 1000;

void isrErupt() {
  unsigned long now = millis();
  if ( now > allowErupt ) {
    allowErupt = now + 10000;
    erupt = true;
  }
}

void setup() { 
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  //Pin Setup
  pinMode(PIN_ERUPT, INPUT_PULLUP); // Interrupt driven eruption trigger (active low)
  attachInterrupt(digitalPinToInterrupt(PIN_ERUPT), isrErupt, FALLING);
  
  pinMode(PIN_PUMP, OUTPUT);        // Smoke pump
  pinMode(PIN_VAPE, OUTPUT);        // Vaporizer coil
  pinMode(PIN_SND_RUMBLE, OUTPUT);  // Background sound
  pinMode(PIN_SND_ERUPT, OUTPUT);   // Eruption sound

  disableAllSound();
  disableEruptSmoke();

  stripOff();
}

void loop() {
  loopReal();
}

void loopTest() {
  enableBackgroundSound();
  enableEruptSmoke();
  eruptLEDs();
}

void loopReal() {
  if ( erupt ) {
    erupt = false;
    triggerEruptSound();
    enableEruptSmoke();

    // Do something for about 8-10 seconds.
    for (int i=0; i<2; i++) {
      stripOff();
      eruptLEDs();
      delay(1000);
    }
    
    disableEruptSmoke();
    allowErupt = millis() + 15000;
  }
  else {
    enableBackgroundSound();
    theaterChase(ORANGE, 1, 200);
  }
}

void enableEruptSmoke() {
  pumpOn();
  smokeOn();
}
  
void disableEruptSmoke() {
  smokeOff();
  pumpOff();
}

void smokeOn() {  
  digitalWrite(PIN_VAPE, HIGH);
}

void smokeOff() {  
  digitalWrite(PIN_VAPE, LOW);
}
void pumpOn() {
  analogWrite(PIN_PUMP, 196);
}  

void pumpOff() {
  digitalWrite(PIN_PUMP, LOW);  
}

void disableAllSound() {
  disableBackgroundSound();
  disableEruptSound();
}

void triggerEruptSound() {
  disableBackgroundSound();
  digitalWrite(PIN_SND_ERUPT, LOW);
}

void disableEruptSound() {
  digitalWrite(PIN_SND_ERUPT, HIGH);  
}

void enableBackgroundSound() {
  digitalWrite(PIN_SND_RUMBLE, LOW);
}

void disableBackgroundSound() {
  digitalWrite(PIN_SND_RUMBLE, HIGH);
}

void wipeSegmentsTogether(uint32_t c, unsigned long wait) {
  //19 = longest segmment
  for (int x=0; x <= 19; x++) {
    for (int i=0; i <= N_SEGMENTS; i++) {
      int pixel = (segments[i][SEG_BEGIN] + x);
      if (pixel <= segments[i][SEG_END]) {
        strip.setPixelColor(pixel, c);
      }
    }
    strip.show();
    delay(wait);
  }
}

void eruptLEDs() {
  wipeSegmentsTogether(RED, 250);
}

// Fill the dots one after the other with a color
void stripOff() {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, 0x000000);
  }
  strip.show();
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t cycles, uint8_t wait) {
  for (int j=0; j < cycles; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
