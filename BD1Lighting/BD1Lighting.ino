#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    6

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 96

// Animation Parameters
#define LED_BRIGHTEN_STEP_MIN_DURATION 1
#define LED_BRIGHTEN_STEP_MAX_DURATION 2
#define LED_DARKEN_STEP_MIN_DURATION 1
#define LED_DARKEN_STEP_MAX_DURATION 2
#define LED_OFF_MIN_DURATION 50
#define LED_OFF_MAX_DURATION 500
#define LED_ON_MIN_DURATION 25
#define LED_ON_MAX_DURATION 250

// Pwm Input Parameters
#define MIN_PWM_VALUE 1024
#define MAX_PWM_VALUE 1936
#define PWM_HYSTERESIS  10

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const uint32_t RED = 0x0020ff20;
const uint32_t GREEN = 0x00ff2010;
const uint32_t BLUE = 0x008010ff;

enum LEDSTATE {
  LEDSTATE_OFF = 0,
  LEDSTATE_BRIGHTEN,
  LEDSTATE_ON,
  LEDSTATE_DARKEN
};

union color_t {
  uint32_t color;
  uint8_t colorBytes[4];
};

class Led {
  private:
    LEDSTATE state;
    word tick;
    word tickDelta;
    uint32_t color;
    uint8_t brightness;
    

  public:
    Led::Led();
    void process(void);
    uint32_t getColor(void);
    void setColor(uint32_t color);
};

Led::Led() {
  state = LEDSTATE_OFF;
  tick = 0;
  brightness = 0;
  tickDelta = random(1, LED_OFF_MAX_DURATION * 3);
}

void Led::process(void) {
  if(++tick < tickDelta) return;
  tick = 0;
  
  switch(state) {
    case LEDSTATE_OFF:
      state = LEDSTATE_BRIGHTEN;
      tickDelta = random(LED_BRIGHTEN_STEP_MIN_DURATION, LED_BRIGHTEN_STEP_MAX_DURATION);
      //Serial.println("BRIGHTEN");
      break;
      
    case LEDSTATE_ON:
      state = LEDSTATE_DARKEN;
      tickDelta = random(LED_DARKEN_STEP_MIN_DURATION, LED_DARKEN_STEP_MAX_DURATION);
      //Serial.println("DARKEN");
      break;
      
    case LEDSTATE_BRIGHTEN:
      if(++brightness == 255) {
        state = LEDSTATE_ON;
        tickDelta = random(LED_ON_MIN_DURATION, LED_ON_MAX_DURATION);
        //Serial.println("ON");
      }
    break;
    
    case LEDSTATE_DARKEN:
      if(--brightness == 0) {
        state = LEDSTATE_OFF;
        tickDelta = random(LED_OFF_MIN_DURATION, LED_OFF_MAX_DURATION);
        //Serial.println("OFF");
      }
      break;
  }

  //Serial.println(brightness);
}

uint32_t Led::getColor(void) {
  color_t colorMix;
  uint8_t tmpBrightness = brightness;
  
  colorMix.color = color;
  
  if(tmpBrightness == 0) {
    colorMix.colorBytes[0] = 0;
    colorMix.colorBytes[1] = 0;
    colorMix.colorBytes[2] = 0;
  } else {
    colorMix.colorBytes[0] = (colorMix.colorBytes[0] * tmpBrightness) >> 8;
    colorMix.colorBytes[1] = (colorMix.colorBytes[1] * tmpBrightness) >> 8;
    colorMix.colorBytes[2] = (colorMix.colorBytes[2] * tmpBrightness) >> 8;
  }

  // ramp to white in upper brightness range
  if(tmpBrightness > 200) {
    colorMix.colorBytes[0] = map(tmpBrightness, 200, 255, colorMix.colorBytes[0], 220);
    colorMix.colorBytes[1] = map(tmpBrightness, 200, 255, colorMix.colorBytes[1], 220);
    colorMix.colorBytes[2] = map(tmpBrightness, 200, 255, colorMix.colorBytes[2], 220);
  }

  return colorMix.color;
}

void Led::setColor(uint32_t clr) {
  color = clr;
}

Led leds[LED_COUNT];

void setup() {
  int i;
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  Serial.begin(115200);

  randomSeed(analogRead(A0) | (analogRead(A1) << 8));

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(25); // Set BRIGHTNESS to about 1/5 (max = 255)

  for(i = 0; i < LED_COUNT; i++) {
    leds[i].setColor(RED);
  }

  attachInterrupt(0, rising, RISING);
  
  Serial.println("Start");
}


int ticks = 0;
int colorswitch = 0;
uint32_t nextColor = RED;
int previousPwmValue = 0;
volatile int pwm_value = 0;
volatile int prev_time = 0;
bool readyToReadPwm = false;
bool pwmReadingValid = false;

void loop() {
  int i;
  //rainbow(2);             // Flowing rainbow cycle along the whole strip

  for(i = 0; i < LED_COUNT; i++) {
    leds[i].process();
    strip.setPixelColor(i, leds[i].getColor());
  }

  if(abs(previousPwmValue - pwm_value) > PWM_HYSTERESIS) {
    previousPwmValue = pwm_value;
    uint16_t pixelHue = map(pwm_value, MIN_PWM_VALUE, MAX_PWM_VALUE, 0, 65535);
    nextColor = strip.gamma32(strip.ColorHSV(pixelHue));
    for(i = 0; i < LED_COUNT; i++) {
      leds[i].setColor(nextColor);
    }
  }

  readyToReadPwm = false;
  strip.show();
  pwmReadingValid = false;
  readyToReadPwm = true;

/*
  if(++ticks >= 500) {
    ticks = 0;
    switch(colorswitch) {
      case 0:
        nextColor = GREEN;
        colorswitch++;
        break;
      case 1:
        nextColor = BLUE;
        colorswitch++;
        break;
      case 2:
        nextColor = RED;
        colorswitch = 0;
        break;
    }
    

    for(i = 0; i < LED_COUNT; i++) {
      leds[i].setColor(nextColor);
    }
  }
*/
}

void rising() {
  attachInterrupt(0, falling, FALLING);
  prev_time = micros();
  pwmReadingValid = true;
}

void falling() {
  if(readyToReadPwm && pwmReadingValid) {
    pwm_value = micros()-prev_time;
    //Serial.println(pwm_value);
  }
  attachInterrupt(0, rising, RISING);
}
