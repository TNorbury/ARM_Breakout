//------------------------------------------------------------------------------
//             __             __   ___  __
//     | |\ | /  ` |    |  | |  \ |__  /__`
//     | | \| \__, |___ \__/ |__/ |___ .__/
//
//------------------------------------------------------------------------------

#include "buttons.h"
#include "joystick.h"
#include "font.h"
#include "sam.h"
#include "spi.h"
#include "speaker.h"
#include "video.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <strings.h>

//-----------------------------------------------------------------------------
//      __   ___  ___         ___  __
//     |  \ |__  |__  | |\ | |__  /__`
//     |__/ |___ |    | | \| |___ .__/
//
//-----------------------------------------------------------------------------

#define BALL_LEFT (-1)
#define BALL_RIGHT (1)

#define BALL_UP (-1)
#define BALL_DOWN (1)

#define LEFT_BOUNDARY (10)
#define RIGHT_BOUNDARY (166)
#define TOP_BOUNDARY (44)
#define BOTTOM_BOUNDARY (220)

#define SPEED_MIN (1)
#define SPEED_MAX (4)

#define BALL_SIZE (4)
#define BALL_COLOR (0xffff)

#define PADDLE_LENGTH (35)
#define PADDLE_HEIGHT (4)
#define PADDLE_COLOR (0x0C5A)
#define PADDLE_SPEED (2)
#define PADDLE_CENTER ((paddle_x + PADDLE_LENGTH) - (PADDLE_LENGTH / 2))
#define PADDLE_START_X (100)
#define PADDLE_START_Y (190)

#define BRICK_WIDTH (12)
#define BRICK_HEIGHT (10)
#define BRICKS_IN_ROW (156 / BRICK_WIDTH)
#define NUM_ROWS (6)

#define BRICK_BOTTOM_BOUNDARY (124)

#define NO_GAME (0)
#define DEMO_MODE (1)
#define LIMIT_MODE (2)
#define UNLIMIT_MODE (3)

#define PADDLE_TOP_BOUNDARY (140)

#define G4 (115)
#define D4 (154)
#define B3 (190)
#define G3 (229)

#define NOTE_LENGTH (100)

//-----------------------------------------------------------------------------
//     ___      __   ___  __   ___  ___  __
//      |  \ / |__) |__  |  \ |__  |__  /__`
//      |   |  |    |___ |__/ |___ |    .__/
//
//-----------------------------------------------------------------------------

typedef struct {
  uint8_t x;
  uint8_t y;
  uint16_t color;
  bool hit;
} brick_t;


//-----------------------------------------------------------------------------
//                __          __        ___  __
//     \  /  /\  |__) |  /\  |__) |    |__  /__`
//      \/  /~~\ |  \ | /~~\ |__) |___ |___ .__/
//
//-----------------------------------------------------------------------------

enum uint8_t {
  LEVEL_1,
  LEVEL_2,
  LEVEL_3,
  LEVEL_4,
  LEVEL_5,
  LEVEL_6
};

static volatile uint32_t millis;
static volatile uint32_t demo_timer;
static volatile uint32_t note_timer;
static const uint16_t ROW_COLORS[6] = {0x1d60, 0x93BB, 0x70E6, 0x4639, 0xCD28,
0x4B4E};
static font_t* font6x8;
static font_t* font8x8;
static font_t* font8x12;
static font_t* font12x16;
static uint8_t brick_count;

//-----------------------------------------------------------------------------
//      __   __   __  ___  __  ___      __   ___  __
//     |__) |__) /  \  |  /  \  |  \ / |__) |__  /__`
//     |    |  \ \__/  |  \__/  |   |  |    |___ .__/
//
//-----------------------------------------------------------------------------

void init_bricks(brick_t* bricks);
void paint_bricks(brick_t* bricks);
bool check_brick_collision(uint16_t* score, brick_t* bricks, uint8_t ball_x, uint8_t ball_y,
uint8_t* new_x, uint8_t* new_y, int8_t* hort_dir, int8_t* vert_dir,
int8_t hort_speed, int8_t vert_speed);
void display_score(uint16_t score, uint8_t x, uint8_t y, uint16_t fg,
uint16_t bg, bool is_lives);
void reset_demo_timer();
void wait_for_button_release();
void play_note(uint32_t note);


//-----------------------------------------------------------------------------
//      __        __          __
//     |__) |  | |__) |    | /  `
//     |    \__/ |__) |___ | \__,
//
//-----------------------------------------------------------------------------

//=============================================================================
int main(void)
{
  uint8_t ball_x, ball_y;
  uint8_t new_x, new_y;
  int8_t ball_hort_dir, ball_vert_dir;
  int8_t ball_hort_speed, ball_vert_speed;
  uint8_t ball_speed = 1;
  uint8_t game_mode = NO_GAME;
  
  uint8_t paddle_x = PADDLE_START_X;
  uint8_t paddle_y = PADDLE_START_Y;
  
  uint16_t joy_x;
  uint16_t joy_y;
  
  uint16_t high_score, current_score;
  uint8_t num_lives = 5;
  
  bool game_over = false;
  bool init_game = false;
  bool boundary_hit = false;
  
  brick_t bricks[(BRICKS_IN_ROW * NUM_ROWS)];

  /* Initialize the SAM system */
  SystemInit();

  //Initialize drivers
  buttons_init();
  joystick_init();
  speaker_init();
  video_init();

  SysTick_Config(48000); //  Configure the SysTick timer for a ms
  
  font6x8 = font_get(FONT_6x8);
  font8x8 = font_get(FONT_8x8);
  font8x12 = font_get(FONT_8x12);
  font12x16 = font_get(FONT_12x16);

  video_paint_rect(0, 0, 176, 220, 0);

  //Create a black screen with a white border on the left, right, and top
  video_paint_rect(0, 34, 176, 220, 0xffff);
  video_paint_rect(10, 44, 156, 210, 0);

  ball_x = PADDLE_CENTER;
  ball_y = paddle_y - BALL_SIZE - 1;

  ball_hort_dir = BALL_LEFT;

  ball_vert_dir = BALL_UP;

  ball_hort_speed = ball_speed;
  ball_vert_speed = 1;
  
  
  //Initialize the bricks
  init_bricks(bricks);
  paint_bricks(bricks);
  
  //Current Score
  current_score = 0;
  display_score(current_score, 104, 0, 0xffff, 0, false);
  
  //High Score
  high_score = 0;
  display_score(high_score, 0, 0, 0xffff, 0, false);
  
  //Current Lives
  display_score(num_lives, 25, 0, 0xffff, 0, true);
  
  
  while (1)
  {
    
    //If it's game over, then reset the game to it's initial state
    if (game_over)
    {
      game_over = false;
      
      video_paint_string("GAME OVER!!!", font12x16, 17, 110, 0xffff, 0);
      video_paint_string("Press any button", font6x8, 40, 126, 0x07FE, 0);
      video_paint_string("to return to the", font6x8, 41, 134, 0x07FE, 0);
      video_paint_string("start menu", font6x8, 41, 142, 0x07FE, 0);
      
      //Turn the speaker off
      speaker_disable();
      
      //Wait for a button to be pressed before playing again.
      while (buttons_get() == NO_BUTTON)
      {
      }
      
      wait_for_button_release();


      game_mode = NO_GAME;
      init_game = true;
      
      reset_demo_timer();
    }
    
    if (game_mode == NO_GAME)
    {
      video_paint_string("BREAKOUT", font12x16, 40, 126, 0xffff, 0);
      video_paint_string("Press PB3 for limited", font6x8, 15, 142, 0x07FE, 0);
      video_paint_string("Press PB1 for unlimited", font6x8, 15, 150, 0x07FE, 0);
      
      
      if (buttons_get() == PB_3)
      {
        //Wait for the button to be released
        wait_for_button_release();
        
        init_game = true;
        game_mode = LIMIT_MODE;
      }
      else if (buttons_get() == PB_1)
      {
        wait_for_button_release();
        
        init_game = true;
        game_mode = UNLIMIT_MODE;
      }
      
      if (demo_timer > 5000)
      {
        init_game = true;
        game_mode = DEMO_MODE;
        
        //Reset the ball to it's initial position, direction, and speed.
        new_x = PADDLE_CENTER;
        new_y = paddle_y - BALL_SIZE - 1;
        
        ball_hort_dir = BALL_LEFT;
        ball_vert_dir = BALL_UP;
        
        ball_hort_speed = ball_speed;
        ball_vert_speed = 1;
        
        video_paint_rect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, 0);
        video_paint_rect(new_x, new_y, BALL_SIZE, BALL_SIZE, BALL_COLOR);
        
        ball_x = new_x;
        ball_y = new_y;
      }
    }
    
    if (init_game)
    {
      init_game = false;
      
      current_score = 0;
      num_lives = 5;
      display_score(num_lives, 25, 0, 0xffff, 0, true);
      display_score(current_score, 104, 0, 0xffff, 0, false);
      video_paint_rect(10, 44, 156, 210, 0);
      init_bricks(bricks);
      paint_bricks(bricks);
      
      paddle_x = PADDLE_START_X;
      paddle_y = PADDLE_START_Y;
      
      //Reset the ball to it's initial position, direction, and speed.
      new_x = PADDLE_CENTER;
      new_y = paddle_y - BALL_SIZE - 1;

      ball_hort_dir = BALL_LEFT;
      ball_vert_dir = BALL_UP;
      
      ball_speed = 1;
      
      ball_hort_speed = ball_speed;
      ball_vert_speed = 1;
    }
    
    if (note_timer > NOTE_LENGTH)
    {
      speaker_disable();
      
      __disable_irq();
      note_timer = 0;
      __enable_irq();
    }
    
    ////////////////////////////////////////////////////////////////////////
    ///////////////  Refresh the screen every 1/60 of a second  ////////////
    //////////////////////////////////////////////////////////////////////// 
    if (millis > 16)
    {
      __disable_irq();
      millis = 0;
      __enable_irq();

      //If demo mode is active have the computer play the game.
      if (game_mode == DEMO_MODE)
      {
        video_paint_string("Press any button to", font6x8, 20, 134, 0x07FE, 0);
        video_paint_string("stop demo", font6x8, 20, 142, 0x07FE, 0);
        
        
        if (buttons_get() != NO_BUTTON)
        {
          wait_for_button_release();
          
          init_game = true;
          game_mode = NO_GAME;
          
          //Reset the demo timer
          reset_demo_timer();
        }
      }
      
      if (game_mode != NO_GAME)
      {
        new_y = ball_y + (ball_vert_dir * ball_vert_speed);
        new_x = ball_x + (ball_hort_dir * ball_hort_speed);
        
        ////////////////////////////////////////////////////////////////////////
        ///////////////////  Check if the ball will hit a brick  ///////////////
        ////////////////////////////////////////////////////////////////////////
        if (new_y < BRICK_BOTTOM_BOUNDARY)
        {
          
          //If any bricks were hit, increment the score and redraw the display
          if (check_brick_collision(&current_score, bricks, ball_x, ball_y, &new_x,
          &new_y, &ball_hort_dir, &ball_vert_dir, ball_hort_speed, ball_vert_speed))
          {
            
            //Re-seed the randomizer
            srand(current_score);
            
            display_score(current_score, 104, 0, 0xffff, 0, false);
            
            //If the new current score surpassed the high score, update it.
            if (current_score > high_score && game_mode != DEMO_MODE)
            {
              high_score = current_score;
              display_score(high_score, 0, 0, 0xffff, 0, false);
            }
            
            //If all bricks are eliminated, redraw all the bricks and advance the
            //level.
            if (brick_count == 0)
            {
              
              //Increase the ball's speed
              ball_speed += 1;
              
              
              //Reset the ball to it's initial position, direction, and speed.
              new_x = PADDLE_CENTER;
              new_y = paddle_y - BALL_SIZE - 1;
              
              ball_hort_dir = BALL_LEFT;
              ball_vert_dir = BALL_UP;
              
              ball_hort_speed = ball_speed;
              ball_vert_speed = 1;
              
              
              video_paint_rect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, 0);
              video_paint_rect(new_x, new_y, BALL_SIZE, BALL_SIZE, BALL_COLOR);
              
              ball_x = new_x;
              ball_y = new_y;
              
              init_bricks(bricks);
              paint_bricks(bricks);
            }
            else
            {
              play_note(G3);
            }
            
          }
        }
        
        ////////////////////////////////////////////////////////////////////////
        ///////////////////  Bounce the ball off of the walls  /////////////////
        ////////////////////////////////////////////////////////////////////////
        
        //if the ball will run into the left boundary
        if (new_x < LEFT_BOUNDARY)
        {
          ball_hort_dir = BALL_RIGHT;
          
          new_x = LEFT_BOUNDARY + ((ball_hort_dir * ball_hort_speed)
          - (ball_x - LEFT_BOUNDARY));
          
          boundary_hit = true;
        }
        
        //Otherwise if the ball will run into the right boundary.
        else if ((new_x + BALL_SIZE) > RIGHT_BOUNDARY)
        {
          ball_hort_dir = BALL_LEFT;
          
          new_x = RIGHT_BOUNDARY + ((ball_hort_dir * ball_hort_speed)
          - (RIGHT_BOUNDARY - ball_x));
          boundary_hit = true;
        }
        
        //If the ball will run into the TOP boundary
        if (new_y < TOP_BOUNDARY)
        {
          ball_vert_dir = BALL_DOWN;
          
          new_y = TOP_BOUNDARY + ((ball_vert_dir * ball_vert_speed)
          - (ball_y - TOP_BOUNDARY));
          boundary_hit = true;
        }
        
        //Otherwise, if the ball will run into the bottom boundary
        else if ((new_y + BALL_SIZE) > BOTTOM_BOUNDARY)
        {
          //if the player is out of lives, then it's game over
          if (num_lives == 0)
          {
            game_over = true;
          }
          //otherwise, decrease the total amount of lives and update the display
          else if (game_mode != DEMO_MODE)
          {
            num_lives --;
            display_score(num_lives, 25, 0, 0xffff, 0, true);
          }
          
          //Reset the ball to it's initial position, direction, and speed.
          new_x = PADDLE_CENTER;
          new_y = paddle_y - BALL_SIZE - 1;
          
          ball_hort_dir = BALL_LEFT;
          ball_vert_dir = BALL_UP;
          
          ball_hort_speed = ball_speed;
          ball_vert_speed = 1;
          
          boundary_hit = true;
        }
        
        if (boundary_hit)
        {
          boundary_hit = false;
          play_note(D4);
        }
        
        ////////////////////////////////////////////////////////////////////////
        ///////////////////  Bounce the ball off of the paddle  ////////////////
        ////////////////////////////////////////////////////////////////////////
        
        //If the ball will run into the paddle, bounce it off the paddle.
        if (((new_x >= paddle_x - 5) && (new_x <= paddle_x + PADDLE_LENGTH))
        && ((new_y >= paddle_y) && (new_y <= paddle_y + PADDLE_HEIGHT)))
        {
          //Bounce the ball upwards
          ball_vert_dir = BALL_UP;
          
          //Calculate how far away from the center the ball is and use that to
          //determine the angle at which is bounces.
          if (ball_x >= PADDLE_CENTER)
          {
            ball_vert_speed = (SPEED_MAX + SPEED_MIN) - ((((SPEED_MAX - SPEED_MIN)
            * (ball_x - PADDLE_CENTER)) / ((paddle_x + PADDLE_LENGTH)
            - PADDLE_CENTER)) + SPEED_MIN);
          }
          else
          {
            ball_vert_speed = (((SPEED_MAX - SPEED_MIN) * (ball_x - paddle_x))
            / (PADDLE_CENTER - paddle_x)) + SPEED_MIN;
          }
          
          //Calculate how far up the ball needs to bounce
          new_y = paddle_y + ((ball_vert_dir * ball_vert_speed)
          - (paddle_y - new_y));
          
          //Play a sound for bouncing off the paddle
          play_note(G4);
        }
        
        //Move the ball based on its speed. Paint it's old position black and then
        // paint it at its new position
        video_paint_rect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, 0);
        video_paint_rect(new_x, new_y, BALL_SIZE, BALL_SIZE, BALL_COLOR);
        
        ball_x = new_x;
        ball_y = new_y;
        
        
        //////////////////////////////////////////////////////////////////////////
        ///////////////////  Move the paddle side to side  ///////////////////////
        //////////////////////////////////////////////////////////////////////////
        
        //if not in demo mode, the joystick will control the movement of the
        //paddle.
        if (game_mode != DEMO_MODE)
        {
          //Read the joystick position on the x-axis
          joy_x = joystick_get_X_Value();
          
          //If the joystick is to the left of the center, move the paddle left.
          if (joy_x < (JOYSTICK_CENTER))
          {
            new_x = paddle_x + PADDLE_SPEED;
          }
          
          //If the joystick is to the right of the center, move the paddle right.
          else if (joy_x > (JOYSTICK_CENTER + 150))
          {
            new_x = paddle_x - PADDLE_SPEED;
          }
          
          //Otherwise, if the joystick is in the dead zone, then don't move the
          //paddle
          else
          {
            new_x = paddle_x;
          }
          
          //If the game is in unlimited mode, move the paddle up and down using
          //the y-axis on the joystick.
          if (game_mode == UNLIMIT_MODE)
          {
            joy_y = joystick_get_Y_Value();
            
            //If the joystick is above the center, move the paddle up
            if (joy_y < (JOYSTICK_CENTER - 150))
            {
              new_y = paddle_y - PADDLE_SPEED;
            }
            
            //if the joystick is below the center, move the paddle down.
            else if (joy_y > (JOYSTICK_CENTER))
            {
              new_y = paddle_y + PADDLE_SPEED;
            }
            
            //Otherwise, the paddle can stay where it is.
            else
            {
              new_y = paddle_y;
            }
          }
          else
          {
            new_y = paddle_y;
          }
        }
        
        //Otherwise, if in demo mode, have the paddle follow the ball
        else
        {
          
          //Maintain the paddle's vertical position.
          new_y = paddle_y;
          
          //1 in 2 chance of the paddle not moving every frame.
          if (rand()%2 == 1)
          {
            //if the ball is to the right of the paddle, move the paddle to the right.
            if (ball_x > PADDLE_CENTER)
            {
              new_x = paddle_x + PADDLE_SPEED;
            }
            
            //Otherwise, if the ball is to the left, move the paddle to the left.
            else if (ball_x < PADDLE_CENTER)
            {
              new_x = paddle_x - PADDLE_SPEED;
            }
            
            //Otherwise, don't move the paddle.
            else
            {
              new_x = paddle_x;
            }
          }
          else
          {
            new_x = paddle_x;
          }
        }
        
        //Check to make sure that the paddle doesn't go off the end of the screen.
        //Check the left boundary
        if (new_x < LEFT_BOUNDARY)
        {
          //If the paddle would move past the left boundary, then just set it to
          //the edge.
          new_x = LEFT_BOUNDARY;
        }
        
        //Check the right boundary
        else if ((new_x + PADDLE_LENGTH) > RIGHT_BOUNDARY)
        {
          //If the paddle would move past the right boundary, then just set it to
          //the edge.
          new_x = (RIGHT_BOUNDARY - PADDLE_LENGTH);
        }
        
        //Make sure the paddle doesn't go off the bottom of the screen, or above
        // the paddle boundary
        if (new_y < PADDLE_TOP_BOUNDARY)
        {
          new_y = PADDLE_TOP_BOUNDARY;
        }
        else if ((new_y + PADDLE_HEIGHT) > BOTTOM_BOUNDARY)
        {
          new_y = (BOTTOM_BOUNDARY - PADDLE_HEIGHT);
        }
        
        //Redraw the paddle and update its position.
        video_paint_rect(paddle_x, paddle_y, PADDLE_LENGTH, PADDLE_HEIGHT, 0);
        video_paint_rect(new_x, new_y, PADDLE_LENGTH, PADDLE_HEIGHT,
        PADDLE_COLOR);
        
        paddle_x = new_x;
        paddle_y = new_y;
      }
    }
  }
}

//-----------------------------------------------------------------------------
//      __   __              ___  ___
//     |__) |__) | \  /  /\   |  |__
//     |    |  \ |  \/  /~~\  |  |___
//
//-----------------------------------------------------------------------------

//=============================================================================
void init_bricks(brick_t* bricks)
{
  brick_count = BRICKS_IN_ROW * NUM_ROWS;
  
  //For every every, and every brick in that row. Set the brick to its
  //initial state.
  for (int i = 0; i < NUM_ROWS; i++)
  {
    for (int j = 0; j < BRICKS_IN_ROW; j++)
    {
      bricks[((i * BRICKS_IN_ROW) + j)].x = 10 + (BRICK_WIDTH * j);
      bricks[((i * BRICKS_IN_ROW) + j)].y
      = (BRICK_BOTTOM_BOUNDARY - (BRICK_HEIGHT * (i + 1)));
      bricks[((i * BRICKS_IN_ROW) + j)].color = ROW_COLORS[i];
      bricks[((i * BRICKS_IN_ROW) + j)].hit = false;
    }
  }
}


//=============================================================================
void paint_bricks(brick_t* bricks)
{
  //For all the bricks
  for (int i = 0; i < (BRICKS_IN_ROW * NUM_ROWS); i++)
  {
    
    //If the brick hasn't been hit then paint it
    if (!bricks[i].hit)
    {
      video_paint_rect(bricks[i].x, bricks[i].y, BRICK_WIDTH, BRICK_HEIGHT,
      bricks[i].color);
    }
  }
}

//=============================================================================
bool check_brick_collision(uint16_t* score, brick_t* bricks, uint8_t ball_x, uint8_t ball_y,
uint8_t* new_x, uint8_t* new_y, int8_t* hort_dir, int8_t* vert_dir,
int8_t hort_speed, int8_t vert_speed)
{
  bool brick_hit = false;
  uint8_t current_level;
  
  //Iterate through all the bricks
  for (int i = 0; (i < (BRICKS_IN_ROW * NUM_ROWS)) && !brick_hit; i++)
  {
    
    //If the brick hasn't been hit, then check if there is a collision
    if (!bricks[i].hit)
    {
      
      //if any part of the ball passes through a brick, set the brick_hit flag
      if (
      ((((*new_x + BALL_SIZE) >= bricks[i].x)
      && ((*new_x + BALL_SIZE) <= (bricks[i].x + BRICK_WIDTH)))
      && (((*new_y + BALL_SIZE) >= bricks[i].y)
      && ((*new_y + BALL_SIZE) <= (bricks[i].y + BRICK_HEIGHT))))
      || (((*new_x <= (bricks[i].x + BRICK_WIDTH))
      && (*new_x >= bricks[i].x))
      && (*new_y <= (bricks[i].y + BRICK_HEIGHT))
      && (*new_y >= bricks[i].y))
      )
      {
        brick_hit = true;
      }
      
      //if a brick was hit then set the brick's hit flag
      if (brick_hit)
      {
        //Since a brick was hit, calculate how the ball should bounce.
        if ((ball_y >= (bricks[i].y + BRICK_HEIGHT)) && (*vert_dir == BALL_UP))
        {
          brick_hit = true;
          
          *vert_dir = BALL_DOWN;
          
          *new_y = (bricks[i].y + BRICK_HEIGHT);
        }
        else if ((ball_x <= bricks[i].x) && (*hort_dir == BALL_RIGHT))
        {
          brick_hit = true;
          
          *hort_dir = BALL_LEFT;
          
          *new_x = bricks[i].x - BALL_SIZE;
        }
        else if ((ball_y <= bricks[i].y) && (*vert_dir == BALL_DOWN))
        {
          brick_hit = true;
          
          *vert_dir = BALL_UP;
          
          *new_y = bricks[i].y - BALL_SIZE;
        }
        else if ((ball_x >= (bricks[i].x + BRICK_WIDTH))
        && (*hort_dir == BALL_LEFT))
        {
          brick_hit = true;
          
          *hort_dir = BALL_RIGHT;
          
          *new_x = (bricks[i].x + BRICK_WIDTH);
        }
        
        //Paint the brick that was hit black.
        bricks[i].hit = true;
        video_paint_rect(bricks[i].x, bricks[i].y, BRICK_WIDTH, BRICK_HEIGHT, 0);
        
        brick_count --;
        
        //Increase the score
        //Determine which row the destroyed brick was on and adjust the score 
        //accordingly.
        current_level = i / BRICKS_IN_ROW;
        if (current_level == LEVEL_1 || current_level == LEVEL_2)
        {
          *score += 1;
        }
        else if (current_level == LEVEL_3 || current_level == LEVEL_4)
        {
          *score += 4;
        }
        else if (current_level == LEVEL_5 || current_level == LEVEL_6)
        {
          *score += 7;
        }
      }
    }
  }
  
  return brick_hit;
}

//=============================================================================
void display_score(uint16_t score, uint8_t x, uint8_t y, uint16_t fg,
uint16_t bg, bool is_lives)
{
  int i = 0;
  uint8_t thousands_digit, hundreds_digit, tens_digit, ones_digit, digit;
  
  //Parse the tens and ones digit of the score
  thousands_digit = score % 10000 / 1000;
  hundreds_digit = score % 1000 / 100;
  tens_digit = score % 100 / 10;
  ones_digit = score % 10 / 1;
  
  //If displaying the number of lives only print the 1s digit
  if (is_lives)
  {
    i = 3;
    digit = ones_digit;
  }
  else
  {
    digit = thousands_digit;
  }
  
  for (; i < 4; i ++)
  {
    
    switch (digit)
    {
      case 0:
      video_paint_string(" _ ", font6x8, (x + (18 * i)), y, fg, bg);
      video_paint_string("| |", font6x8, (x + (18 * i)), (y + 8), fg, bg);
      video_paint_string("|_|", font6x8, (x + (18 * i)), (y + 16), fg, bg);
      break;
      
      case 1:
      video_paint_string("   ", font6x8, (x + (18 * i)), y, fg, bg);
      video_paint_string("  |", font6x8, (x + (18 * i)), (y + 8), fg, bg);
      video_paint_string("  |", font6x8, (x + (18 * i)), (y + 16), fg, bg);
      break;
      
      case 2:
      video_paint_string(" _ ", font6x8, (x + (18 * i)), y, fg, bg);
      video_paint_string(" _|", font6x8, (x + (18 * i)), (y + 8), fg, bg);
      video_paint_string("|_ ", font6x8, (x + (18 * i)), (y + 16), fg, bg);
      break;
      
      case 3:
      video_paint_string(" _ ", font6x8, (x + (18 * i)), y, fg, bg);
      video_paint_string(" _|", font6x8, (x + (18 * i)), (y + 8), fg, bg);
      video_paint_string(" _|", font6x8, (x + (18 * i)), (y + 16), fg, bg);
      break;
      
      case 4:
      video_paint_string("   ", font6x8, (x + (18 * i)), y, fg, bg);
      video_paint_string("|_|", font6x8, (x + (18 * i)), (y + 8), fg, bg);
      video_paint_string("  |", font6x8, (x + (18 * i)), (y + 16), fg, bg);
      break;
      
      case 5:
      video_paint_string(" _ ", font6x8, (x + (18 * i)), y, fg, bg);
      video_paint_string("|_ ", font6x8, (x + (18 * i)), (y + 8), fg, bg);
      video_paint_string(" _|", font6x8, (x + (18 * i)), (y + 16), fg, bg);
      break;
      
      case 6:
      video_paint_string(" _ ", font6x8, (x + (18 * i)), y, fg, bg);
      video_paint_string("|_ ", font6x8, (x + (18 * i)), (y + 8), fg, bg);
      video_paint_string("|_|", font6x8, (x + (18 * i)), (y + 16), fg, bg);
      break;
      
      case 7:
      video_paint_string(" _ ", font6x8, (x + (18 * i)), y, fg, bg);
      video_paint_string("  |", font6x8, (x + (18 * i)), (y + 8), fg, bg);
      video_paint_string("  |", font6x8, (x + (18 * i)), (y + 16), fg, bg);
      break;
      
      case 8:
      video_paint_string(" _ ", font6x8, (x + (18 * i)), y, fg, bg);
      video_paint_string("|_|", font6x8, (x + (18 * i)), (y + 8), fg, bg);
      video_paint_string("|_|", font6x8, (x + (18 * i)), (y + 16), fg, bg);
      break;
      
      case 9:
      video_paint_string(" _ ", font6x8, (x + (18 * i)), y, fg, bg);
      video_paint_string("|_|", font6x8, (x + (18 * i)), (y + 8), fg, bg);
      video_paint_string("  |", font6x8, (x + (18 * i)), (y + 16), fg, bg);
      break;
    }
    
    if (i == 0)
    {
      digit = hundreds_digit;
    }
    else if (i == 1)
    {
      digit = tens_digit;
    }
    else if (i == 2)
    {
      digit = ones_digit;
    }
  }
}

//=============================================================================
void reset_demo_timer()
{
  __disable_irq();
  demo_timer = 0;
  __enable_irq();
}

//=============================================================================
void wait_for_button_release()
{
  //Wait for the button to be released
  while (buttons_get() != NO_BUTTON)
  {
  }
}

//=============================================================================
void play_note(uint32_t note)
{
  speaker_enable();
  speaker_set(note);
  
  __disable_irq();
  note_timer = 0;
  __enable_irq();
}

//-----------------------------------------------------------------------------
//        __   __   __
//     | /__` |__) /__`
//     | .__/ |  \ .__/
//
//-----------------------------------------------------------------------------
void SysTick_Handler()
{
  millis ++;
  demo_timer++;
  note_timer++;
}
