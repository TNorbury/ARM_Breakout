//------------------------------------------------------------------------------
//             __             __   ___  __
//     | |\ | /  ` |    |  | |  \ |__  /__`
//     | | \| \__, |___ \__/ |__/ |___ .__/
//
//------------------------------------------------------------------------------

#include "sam.h"
#include "joystick.h"
#include "spi.h"
#include "video.h"
#include "font.h"

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

#define BRICK_WIDTH (12)
#define BRICK_HEIGHT (10)
#define BRICKS_IN_ROW (156 / BRICK_WIDTH)
#define NUM_ROWS (6)

#define BRICK_BOTTOM_BOUNDARY (124)

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
  level_1,
  level_2,
  level_3,
  level_4,
  level_5,
  level_6
  };

static volatile uint32_t millis;
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
bool check_brick_collision(brick_t* bricks, uint8_t ball_x, uint8_t ball_y, 
  uint8_t* new_x, uint8_t* new_y, int8_t* hort_dir, int8_t* vert_dir, 
  int8_t hort_speed, int8_t vert_speed);
void display_score(uint16_t score, uint8_t x, uint8_t y, uint16_t fg,
  uint16_t bg, bool is_lives);



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
  
  uint8_t paddle_x = 100;
  uint8_t paddle_y = 190;
  
  uint16_t joy_x;
  
  uint16_t high_score, current_score;
  uint8_t num_lives = 5;
  uint8_t current_level = level_1;
  
  bool game_over = false;
  
  brick_t bricks[(BRICKS_IN_ROW * NUM_ROWS)];

  /* Initialize the SAM system */
  SystemInit();

  //Initialize drivers
  joystick_init();
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

  //Paint the ball in its initial spot.
  video_paint_rect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, 0xffff);

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
      current_score = 0;
      current_level = level_1;
      num_lives = 5;
      display_score(num_lives, 25, 0, 0xffff, 0, true);
      display_score(current_score, 104, 0, 0xffff, 0, false);
      video_paint_rect(10, 44, 156, 210, 0);
      
      video_paint_string("GAME OVER!!!", font12x16, 17, 110, 0xffff, 0);
      video_paint_string("Press any button", font6x8, 40, 126, 0x07FE, 0);
      video_paint_string("to play again", font6x8, 45, 134, 0x07FE, 0);
      
      while (1)
      {
      }
      
      init_bricks(bricks);
      paint_bricks(bricks);
    }
    
    if (millis > 16)
    {
      __disable_irq();
      millis = 0;
      __enable_irq();

      new_y = ball_y + (ball_vert_dir * ball_vert_speed);
      new_x = ball_x + (ball_hort_dir * ball_hort_speed);
      
      //////////////////////////////////////////////////////////////////////////
      ///////////////////  Check if the ball will hit a brick  /////////////////
      //////////////////////////////////////////////////////////////////////////
      if (new_y < BRICK_BOTTOM_BOUNDARY)
      {
        
        //If any bricks were hit, increment the score and redraw the display
        if (check_brick_collision(bricks, ball_x, ball_y, &new_x, &new_y,
        &ball_hort_dir, &ball_vert_dir, ball_hort_speed, ball_vert_speed))
        {
          if (current_level == level_1 || current_level == level_2)
          {
            current_score++;
          }
          else if (current_level == level_3 || current_level == level_4)
          {
            current_score += 4;
          }
          else if (current_level == level_5 || current_level == level_6)
          {
            current_score += 7;
          }
          
          display_score(current_score, 104, 0, 0xffff, 0, false);
          
          //If the new current score surpassed the high score, update it.
          if (current_score > high_score)
          {
            high_score = current_score;
            display_score(high_score, 0, 0, 0xffff, 0, false);
          }
          
          //If all bricks are eliminated, redraw all the bricks and advance the 
          //level.
          if (brick_count == 0)
          {
            if (current_level != level_6)
            {
              current_level ++;
            }
            else
            {
              current_level = 0;
              
              //Increase the ball's speed
            }
            
            init_bricks(bricks);
            paint_bricks(bricks);
          }
          
        }
      }
      

      //if the ball will run into a boundary then calculate the difference
      //between the ball and the boundary and then offset the ball by that much

      //if the ball will run into the left boundary
      if (new_x < LEFT_BOUNDARY)
      {
        ball_hort_dir = BALL_RIGHT;

        new_x = LEFT_BOUNDARY + ((ball_hort_dir * ball_hort_speed)
        - (ball_x - LEFT_BOUNDARY));
      }

      //Otherwise if the ball will run into the right boundary.
      else if ((new_x + BALL_SIZE) > RIGHT_BOUNDARY)
      {
        ball_hort_dir = BALL_LEFT;

        new_x = RIGHT_BOUNDARY + ((ball_hort_dir * ball_hort_speed)
        - (RIGHT_BOUNDARY - ball_x));
      }


      //If the ball will run into the TOP boundary
      if (new_y < TOP_BOUNDARY)
      {
        ball_vert_dir = BALL_DOWN;

        new_y = TOP_BOUNDARY + ((ball_vert_dir * ball_vert_speed)
        - (ball_y - TOP_BOUNDARY));
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
        else
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
      }
      
      
      //////////////////////////////////////////////////////////////////////////
      ///////////////////  Bounce the ball off of the paddle  //////////////////
      //////////////////////////////////////////////////////////////////////////
      
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
      
      //Redraw the paddle and update its position.
      video_paint_rect(paddle_x, paddle_y, PADDLE_LENGTH, PADDLE_HEIGHT, 0);
      video_paint_rect(new_x, paddle_y, PADDLE_LENGTH, PADDLE_HEIGHT,
      PADDLE_COLOR);
      
      paddle_x = new_x;
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
bool check_brick_collision(brick_t* bricks, uint8_t ball_x, uint8_t ball_y,
uint8_t* new_x, uint8_t* new_y, int8_t* hort_dir, int8_t* vert_dir,
int8_t hort_speed, int8_t vert_speed)
{
  bool brick_hit = false;
  
  //Iterate through all the bricks
  for (int i = 0; (i < (BRICKS_IN_ROW * NUM_ROWS)) && !brick_hit; i++)
  {
    
    //If the brick hasn't been hit, then check if there is a collision
    if (!bricks[i].hit)
    {
      
      //if any part of the ball passes through a brick, set the brick_hit flag
      if (
      ((((*new_x + BALL_SIZE) > bricks[i].x)
      && ((*new_x + BALL_SIZE) < (bricks[i].x + BRICK_WIDTH)))
      && (((*new_y + BALL_SIZE) > bricks[i].y)
      && ((*new_y + BALL_SIZE) < (bricks[i].y + BRICK_HEIGHT))))
      || (((*new_x < (bricks[i].x + BRICK_WIDTH))
      && (*new_x > bricks[i].x))
      && (*new_y < (bricks[i].y + BRICK_HEIGHT))
      && (*new_y > bricks[i].y))
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

          *new_y = (bricks[i].y + BRICK_HEIGHT) + ((*vert_dir * vert_speed)
          - (ball_y - (bricks[i].y + BRICK_HEIGHT)));
        }
        else if ((ball_x <= bricks[i].x) && (*hort_dir == BALL_RIGHT))
        {
          brick_hit = true;
          
          *hort_dir = BALL_LEFT;

          *new_x = bricks[i].x + ((*hort_dir * hort_speed)
          - (ball_x - bricks[i].x));
        }
        else if ((ball_y <= bricks[i].y) && (*vert_dir == BALL_DOWN))
        {
          brick_hit = true;
          
          *vert_dir = BALL_UP;

          new_y = bricks[i].y + ((*vert_dir * vert_speed)
          - (bricks[i].y - *new_y));
        }
        else if ((ball_x >= (bricks[i].x + BRICK_WIDTH))
        && (*hort_dir == BALL_LEFT))
        {
          brick_hit = true;
          
          *hort_dir = BALL_RIGHT;

          *new_x = (bricks[i].x + BRICK_WIDTH) + ((*hort_dir * hort_speed)
          - ((bricks[i].x + BRICK_WIDTH) - ball_x));
        }
        
        //Paint the brick that was hit black.
        bricks[i].hit = true;
        video_paint_rect(bricks[i].x, bricks[i].y, BRICK_WIDTH, BRICK_HEIGHT, 0);
        
        brick_count --;
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

//-----------------------------------------------------------------------------
//        __   __   __
//     | /__` |__) /__`
//     | .__/ |  \ .__/
//
//-----------------------------------------------------------------------------
void SysTick_Handler()
{
  millis ++;
}
