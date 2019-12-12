#include <Adafruit_NeoPixel.h>

#define STRIP_PIN 4
#define LED_COUNT 30

#define BUTTON_PIN 10
#define LED_PIN   LED_BUILTIN

Adafruit_NeoPixel strip(LED_COUNT, STRIP_PIN, NEO_GRB + NEO_KHZ800);

#define GAME_WAIT 0
#define GAME_RUN 1
#define GAME_SCORE 2
int game_state = GAME_WAIT;

// the game
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
  return !(player_pos%3);
}


void light_led( int posn ) {
  // light the LED in position posn according to its successful status.
  if ( successful[posn] ){
    strip.setPixelColor( posn, 20, 20, 80 );
  } else {
    strip.setPixelColor( posn, 40, 0, 0 );
    
  }
}

void extinguish_led( int posn ) {
  strip.setPixelColor( posn, 0, 0, 0 );
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
  return true;
}
void setup() {
  strip.begin();
  strip.show();
  pinMode(LED_PIN, OUTPUT) ;
}

void loop() {
  // key_pressed can be reset by the reader. key_down is used to detect edges
  if ( digitalRead( BUTTON_PIN ) ) {
    if ( !key_down ) {
      key_down = true;
      key_pressed = true;
    }
  } else {
    if ( key_down ) {
      key_down = false;
    }
  }
  switch ( game_state )
  {
    case GAME_WAIT:
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
          game_state = GAME_WAIT;
          committed=player_pos=0;
      }
      break;
  }
}
