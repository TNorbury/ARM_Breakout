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
#define TOP_BOUNDARY (10)
#define BOTTOM_BOUNDARY (220)

#define SPEED_MIN (1)
#define SPEED_MAX (4)

#define BALL_SIZE (4)
#define BALL_COLOR (0xffff)

#define PADDLE_LENGTH (35)
#define PADDLE_HEIGHT (4)
#define PADDLE_COLOR (0x0C5A)
#define PADDLE_SPEED (2)

//-----------------------------------------------------------------------------
//     ___      __   ___  __   ___  ___  __
//      |  \ / |__) |__  |  \ |__  |__  /__`
//      |   |  |    |___ |__/ |___ |    .__/
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//                __          __        ___  __
//     \  /  /\  |__) |  /\  |__) |    |__  /__`
//      \/  /~~\ |  \ | /~~\ |__) |___ |___ .__/
//
//-----------------------------------------------------------------------------

static volatile uint32_t millis;

//-----------------------------------------------------------------------------
//      __   __   __  ___  __  ___      __   ___  __
//     |__) |__) /  \  |  /  \  |  \ / |__) |__  /__`
//     |    |  \ \__/  |  \__/  |   |  |    |___ .__/
//
//-----------------------------------------------------------------------------

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
  uint8_t paddle_center;
  
  uint8_t paddle_x = 50;
  uint8_t paddle_y = 190;
  
  uint16_t joy_x;

  /* Initialize the SAM system */
  SystemInit();

  //Initialize drivers
  joystick_init();
  video_init();

  SysTick_Config(48000); //  Configure the SysTick timer for a ms

  //Create a black screen with a white border on the left, right, and top
  video_paint_rect(0, 0, 176, 220, 0xffff);
  video_paint_rect(10, 10, 156, 210, 0);

  ball_x = 50;
  ball_y = 50;

  //Paint the ball in its initial spot.
  video_paint_rect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, 0xffff);

  ball_hort_dir = BALL_LEFT;

  ball_vert_dir = BALL_UP;

  ball_hort_speed = 2;
  ball_vert_speed = 1;
  
  while (1)
  {
    if (millis > 16)
    {
      __disable_irq();
      millis = 0;
      __enable_irq();

      //ball_hort_speed = calculate_speed(joystick_get_X_Value());
      //ball_vert_speed = calculate_speed(joystick_get_Y_Value());
      new_y = ball_y + (ball_vert_dir * ball_vert_speed);
      new_x = ball_x + (ball_hort_dir * ball_hort_speed);

      //if the ball will run into the left boundary
      if (new_x < LEFT_BOUNDARY)
      {
        ball_hort_dir = BALL_RIGHT;

        //Calculate the difference between the ball and the boundary and then
        //offset the ball by that much
        new_x = LEFT_BOUNDARY + ((ball_hort_dir * ball_hort_speed)
        - (ball_x - LEFT_BOUNDARY));
      }

      //Otherwise if the ball will run into the right boundary.
      else if ((new_x + BALL_SIZE) > RIGHT_BOUNDARY)
      {
        ball_hort_dir = BALL_LEFT;

        //Calculate the difference between the ball and the boundary and then
        //offset the ball by that much
        new_x = RIGHT_BOUNDARY + ((ball_hort_dir * ball_hort_speed)
        - (RIGHT_BOUNDARY - ball_x));
      }


      //If the ball will run into the TOP boundary
      if (new_y < TOP_BOUNDARY)
      {
        ball_vert_dir = BALL_DOWN;

        //Calculate the difference between the ball and the boundary and then
        //offset the ball by that much
        new_y = TOP_BOUNDARY + ((ball_vert_dir * ball_vert_speed)
        - (new_y - TOP_BOUNDARY));
      }

      //Otherwise, if the ball will run into the bottom boundary
      else if ((new_y + BALL_SIZE) > BOTTOM_BOUNDARY)
      {
        
        //Reset the ball to it's initial position, direction, and speed.
        new_x = 50;
        new_y = 50;

        ball_hort_dir = BALL_LEFT;
        ball_vert_dir = BALL_UP;
        
        ball_hort_speed = 2;
        ball_vert_speed = 2;
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
        
        //Calculate the angle at which the ball needs to bounce.
        paddle_center = (paddle_x + PADDLE_LENGTH) - (PADDLE_LENGTH / 2);
        
        //Calculate how far away from the center the ball is and use that to
        //determine the angle at which is bounces.
        if (ball_x >= paddle_center)
        {
          ball_vert_speed = (SPEED_MAX + SPEED_MIN) - ((((SPEED_MAX - SPEED_MIN)
          * (ball_x - paddle_center)) / ((paddle_x + PADDLE_LENGTH)
          - paddle_center)) + SPEED_MIN);
        }
        else
        {
          ball_vert_speed = (((SPEED_MAX - SPEED_MIN) * (ball_x - paddle_x))
          / (paddle_center - paddle_x)) + SPEED_MIN;
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
      
      //Otherwise, if the joystick is in the dead zone, then don't move the paddle
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
