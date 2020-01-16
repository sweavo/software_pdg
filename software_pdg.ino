#include <Adafruit_NeoPixel.h>

//////////////////////////////////////////////////////////////////////////////////
// Commissioning parameters

// Attach something with WS2812 to this pin
#define STRIP_PIN 4
#define LED_COUNT 30

// Pushbutton to play the game.  If active high, then
// set BUTTON_ACTIVE_LOW to 0 and deal with pulldown as
// you need to.  If active low, then setup() will activate
// the input's builtin pullup resistor.
#define BUTTON_PIN 10
#define BUTTON_ACTIVE_LOW 1

// Attach a piezo speaker between this pin and GND.
#define PIEZO_PIN 9

// This is a diagnostic pin
#define LED_PIN   LED_BUILTIN

// End of commissioning parameters
//////////////////////////////////////////////////////////////////////////////////
// Tuning parameters

// "Stylesheet"

//// Globally dim LEDs, so that you can work on the program without getting a migraine.
#define BRIGHTNESS_DIVIDER 4

//// Theme Colors
#define COLOR_ETAS_BLUE strip.Color(20/BRIGHTNESS_DIVIDER, 20/BRIGHTNESS_DIVIDER, 80/BRIGHTNESS_DIVIDER)
#define COLOR_BLACK strip.Color(0,0,0)
#define COLOR_WHITE strip.Color(40/BRIGHTNESS_DIVIDER,40/BRIGHTNESS_DIVIDER,40/BRIGHTNESS_DIVIDER)
#define COLOR_RED strip.Color(40/BRIGHTNESS_DIVIDER, 0, 0)

//// Semantic colors
#define COLOR_WORKPACKAGE_OK COLOR_ETAS_BLUE
#define COLOR_WORKPACKAGE_FAIL COLOR_RED
#define COLOR_RESET COLOR_BLACK

//// Sounds
#define PITCH_KEYPRESS 523
#define DURATION_KEYPRESS 50
#define PITCH_BOTTOM_STEP 440
#define PITCH_TOP_STEP 880
#define DURATION_STEP 10
#define PITCH_BAD 220
#define DURATION_BAD 50
#define PITCH_SCORE_BAD 110
#define DURATION_SCORE_BAD 150

// Tuning values for the game
#define ERROR_PERCENT 0
#define STRIDE 6
#define DELAY_STEP 20
#define GAME_TIMEOUT_TICKS (5000/DELAY_STEP)

// End of tuning parameters
//////////////////////////////////////////////////////////////////////////////////

// Top-level states
typedef uint8_t GameState_t;
#define GAME_WAIT ((GameState_t)0)
#define GAME_RUN ((GameState_t)1)
#define GAME_SCORE ((GameState_t)2)
#define GAME_RESET ((GameState_t)3)

GameState_t game_state = GAME_WAIT;

// State used while playing the game
bool successful[LED_COUNT];
int committed = 0;
int player_pos = -1;
int player_dir = 1;
bool finally_perfect = false;

// for latching the button press
bool key_down = false;
bool key_pressed = false;

// for random outcome of work packages
bool random_bit() {
  return random(100) >= ERROR_PERCENT;
}


Adafruit_NeoPixel strip(LED_COUNT, STRIP_PIN, NEO_GRB + NEO_KHZ800);

void light_led( int posn ) {
  // light the LED in position posn according to its successful status.
  strip.setPixelColor( posn, successful[posn] ? COLOR_WORKPACKAGE_OK : COLOR_WORKPACKAGE_FAIL );
}

void extinguish_led( int posn ) {
  strip.setPixelColor( posn, COLOR_BLACK );
}

void setup() {
  randomSeed(analogRead(5)); // randomize using noise from analog pin 5
  strip.begin();
  strip.fill(COLOR_RESET, 0, LED_COUNT);
  strip.show();
  pinMode(LED_PIN, OUTPUT);
#if BUTTON_ACTIVE_LOW==1
  pinMode(BUTTON_PIN, INPUT_PULLUP);
#else
  pinMode(BUTTON_PIN, INPUT);
#endif
  pinMode(PIEZO_PIN, OUTPUT);
}

GameState_t game_raster() {
  static uint16_t timeout = 0;

  if (key_pressed ) {
    timeout = 0;
    key_pressed = false;
    committed = player_pos;
    if (committed >= LED_COUNT - 1 )
    {
      timeout = 0;
      return GAME_SCORE;
    }
  }
  if (++timeout > GAME_TIMEOUT_TICKS )
  {
    timeout = 0;
    return GAME_RESET;
  }

  if ( player_dir > 0 ) {
    ++player_pos;
    successful[player_pos] = random_bit();
    if (!successful[player_pos])
    {
      beep( PITCH_BAD, DURATION_BAD );
    }
    light_led( player_pos );
    if ( ( player_pos >= LED_COUNT )
         || ( player_pos > (committed + STRIDE ) ) ) {
      beep_for_posn( player_pos );
      player_dir = -1;
    }
  } else {
    extinguish_led( player_pos );
    player_pos--;
    if ( player_pos <= committed ) {
      beep_for_posn( player_pos );
      player_dir = 1;
    }
  }
  strip.show();
  delay(DELAY_STEP);
  return GAME_RUN;
}

bool score_animation_done() {
  static uint8_t i = 0;
  if (!i) {
    finally_perfect = true;
  }
  strip.setPixelColor( i, COLOR_WHITE );
  strip.show();
  delay(10);
  strip.setPixelColor( i, COLOR_BLACK);
  strip.show();
  delay(10);
  if ( !successful[i] )
  {
    beep( PITCH_SCORE_BAD, DURATION_SCORE_BAD );
    finally_perfect = false;
  }
  
  strip.setPixelColor( i, finally_perfect ? COLOR_WORKPACKAGE_OK : COLOR_WORKPACKAGE_FAIL );
  //light_led( i );
  delay(10);
  
  if (++i > LED_COUNT )
  {
    i = 0;
    return true;
  }
  return false;
}

void beep( unsigned int frequency, unsigned long duration )
{
  noTone(PIEZO_PIN);
  tone(PIEZO_PIN, frequency, duration);
}

void beep_for_posn( uint8_t posn )
{
  beep( PITCH_BOTTOM_STEP + ((PITCH_TOP_STEP - PITCH_BOTTOM_STEP ) * posn ) / LED_COUNT, DURATION_STEP );
}

void loop() {
  // key_pressed can be reset by the reader. key_down is used to detect edges
  if ( digitalRead( BUTTON_PIN ) ^ BUTTON_ACTIVE_LOW ) { // active low
    if ( !key_down ) {
      beep( PITCH_KEYPRESS, DURATION_KEYPRESS );
      key_down = true;
      key_pressed = true;
    }
  } else {
    if ( key_down ) {
      key_down = false;
    }
  }

  // Overall state
  switch ( game_state )
  {
    case GAME_WAIT:
      strip.fill(COLOR_RESET, 0, LED_COUNT);
      if ( key_pressed ) {
        key_pressed = false;
        game_state = GAME_RUN;
      }
      break;

    case GAME_RUN:
      game_state = game_raster();
      break;

    case GAME_SCORE:
      if ( score_animation_done() ) {
        game_state = GAME_RESET;
      }
      break;

    case GAME_RESET:
      if (game_reset()) game_state = GAME_WAIT;

  }
}

bool game_reset() {
  static uint8_t cap = 200 / BRIGHTNESS_DIVIDER;
  uint32_t color;
  if (finally_perfect )
  {
    color = strip.Color( min( cap, 30 / BRIGHTNESS_DIVIDER),
                         min( cap, 83 / BRIGHTNESS_DIVIDER),
                         min( cap, 158 / BRIGHTNESS_DIVIDER));
  } else {
    color = strip.Color( min( cap, 80 / BRIGHTNESS_DIVIDER),
                         min( cap, 0 / BRIGHTNESS_DIVIDER),
                         min( cap, 0 / BRIGHTNESS_DIVIDER));
  }

  strip.fill( color, 0, player_pos+1);
  strip.show();
  delay(DELAY_STEP / 2);
  if (--cap)
  {
    return false;
  }
  else
  {
    strip.fill( COLOR_RESET, 0, LED_COUNT );
    strip.show();    key_pressed = false;
    committed = 0;
    player_pos = -1;
    player_dir = 1;
    cap = 200 / BRIGHTNESS_DIVIDER;
    return true;
  }
}
