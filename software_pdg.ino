#include <Adafruit_NeoPixel.h>

#define STRIP_PIN 4
#define LED_COUNT 30

#define BUTTON_PIN 10
#define BUTTON_ACTIVE_LOW 1

#define PIEZO_PIN 9

#define LED_PIN   LED_BUILTIN

Adafruit_NeoPixel strip(LED_COUNT, STRIP_PIN, NEO_GRB + NEO_KHZ800);

#define GAME_WAIT 0
#define GAME_RUN 1
#define GAME_SCORE 2
#define GAME_RESET 3
int game_state = GAME_WAIT;

// the game
#define ERROR_PERCENT 10
bool successful[LED_COUNT];
int committed = 0;
#define STRIDE 6
int player_pos = 0;
int player_dir = 1;

// for latching the button press
bool key_down = false;
bool key_pressed = false;

// for random outcome of work packages
bool random_bit() {
  return random(100) > ERROR_PERCENT;
}

#define BRIGHTNESS_DIVIDER 4
#define COLOR_ETAS_BLUE strip.Color(20/BRIGHTNESS_DIVIDER, 20/BRIGHTNESS_DIVIDER, 80/BRIGHTNESS_DIVIDER)
#define COLOR_BLACK strip.Color(0,0,0)
#define COLOR_WHITE strip.Color(40/BRIGHTNESS_DIVIDER,40/BRIGHTNESS_DIVIDER,40/BRIGHTNESS_DIVIDER)
#define COLOR_WORKPACKAGE_OK COLOR_ETAS_BLUE
#define COLOR_WORKPACKAGE_FAIL strip.Color(40/BRIGHTNESS_DIVIDER, 0, 0)

void light_led( int posn ) {
  // light the LED in position posn according to its successful status.
  if ( successful[posn] ){
    strip.setPixelColor( posn, COLOR_WORKPACKAGE_OK);
  } else {
    strip.setPixelColor( posn, COLOR_WORKPACKAGE_FAIL );
    
  }
}

void extinguish_led( int posn ) {
  strip.setPixelColor( posn, COLOR_BLACK );
}

bool game_raster() {
  if (key_pressed ) {
    key_pressed = false;
    committed = player_pos;
    if (committed >= LED_COUNT-1 )
    {
      return true;
    }
  }
  if ( player_dir > 0 ) {
    digitalWrite( LED_PIN, 1 );
    ++player_pos;
    successful[player_pos] = random_bit();
    light_led( player_pos );
    if ( ( player_pos > LED_COUNT )
         || ( player_pos > (committed + STRIDE ) ) ) {
      player_dir = -1;
    }
  } else {
    digitalWrite( LED_PIN, 0 );
    extinguish_led( player_pos );
    player_pos--;
    if ( player_pos < committed ) {
      player_dir = 1;
    }
  }
  strip.show();
  return false;
}
bool score_animation_done() {
  static uint8_t i=0;
  strip.setPixelColor( i, COLOR_WHITE );
  strip.show();
  delay(10);
  strip.setPixelColor( i, COLOR_BLACK);
  strip.show();
  delay(10);
  light_led( i );
  delay(10);
  if (++i >= LED_COUNT )
  {
    i=0;
    return true;
  }
  return false;
}

void setup() {
  randomSeed(analogRead(5)); // randomize using noise from analog pin 5
  strip.begin();
  strip.show();
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  pinMode(PIEZO_PIN, OUTPUT);
}

void beep(unsigned char period)
{
  analogWrite(PIEZO_PIN,20);
  delay( period/2);
  analogWrite(PIEZO_PIN,0);
  delay( period/2);
}


void loop() {
  // key_pressed can be reset by the reader. key_down is used to detect edges
  if ( digitalRead( BUTTON_PIN ) ^ BUTTON_ACTIVE_LOW ) { // active low
    if ( !key_down ) {
      beep(60);
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
      strip.fill(COLOR_BLACK, 0, LED_COUNT);
      if ( key_pressed ) {
        key_pressed=false;
        game_state=GAME_RUN;
      }
      break;

    case GAME_RUN:
      if ( game_raster() ) { game_state = GAME_SCORE; }
      delay(20);
      break;

    case GAME_SCORE:
      key_pressed=false;
      if ( score_animation_done() ) {
          game_state = GAME_RESET;
          committed=player_pos=0;
          key_pressed=false;
      }
      break;
      
    case GAME_RESET:
      key_pressed=false;
      game_raster();
      if (player_pos<=0) {
        game_state = GAME_WAIT;
      }
  }
} 
