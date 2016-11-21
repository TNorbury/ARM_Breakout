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
#define BOTTOM_BOUNDARY (210)

#define SPEED_MIN (1)
#define SPEED_MAX (7)

#define BALL_SIZE (8)

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

uint8_t calculate_speed(uint16_t joystick_val);

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
  uint16_t color = 0;

  uint16_t joy_y, joy_x;

  /* Initialize the SAM system */
  SystemInit();

  //Initialize drivers
  joystick_init();
  video_init();

  SysTick_Config(48000); //  Configure the SysTick timer for a ms

  //Create a black screen with a white border
  video_paint_rect(0, 0, 176, 220, 0xffff);
  video_paint_rect(10, 10, 156, 200, 0);

  ball_x = 50;
  ball_y = 50;

  //Paint the ball in its initial spot.
  video_paint_rect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, 0xffff);

  ball_hort_dir = BALL_LEFT;
  ball_hort_speed = 1;

  ball_vert_dir = BALL_UP;
  ball_vert_speed = 1;

  while (1)
  {
    if (millis > 16)
    {
      __disable_irq();
      millis = 0;
      __enable_irq();

      ball_hort_speed = calculate_speed(joystick_get_X_Value());
      ball_vert_speed = calculate_speed(joystick_get_Y_Value());
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
        ball_vert_dir = BALL_UP;

        //Calculate the difference between the ball and the boundary and then
        //offset the ball by that much
        new_y = BOTTOM_BOUNDARY + ((ball_vert_dir * ball_vert_speed)
        - (BOTTOM_BOUNDARY - new_y));
      }

      //Move the ball based on its speed. Paint it's old position black and then
      // paint it at its new position
      video_paint_rect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, 0);
      video_paint_rect(new_x, new_y, BALL_SIZE, BALL_SIZE, 0xffff);

      ball_x = new_x;
      ball_y = new_y;
    }
  }
}

//-----------------------------------------------------------------------------
//      __   __              ___  ___
//     |__) |__) | \  /  /\   |  |__
//     |    |  \ |  \/  /~~\  |  |___
//
//-----------------------------------------------------------------------------

uint8_t calculate_speed(uint16_t joystick_val)
{
  uint8_t speed;

  //Take the adc value and convert it into the range of speed min to speed max
  speed = (SPEED_MAX + SPEED_MIN) -
  (
  (((SPEED_MAX - SPEED_MIN) * (joystick_val - JOYSTICK_MIN))
  / (JOYSTICK_MAX - JOYSTICK_MIN)) + SPEED_MIN
  );

  return speed;

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
