//------------------------------------------------------------------------------
//             __             __   ___  __
//     | |\ | /  ` |    |  | |  \ |__  /__`
//     | | \| \__, |___ \__/ |__/ |___ .__/
//
//------------------------------------------------------------------------------

#include "joystick.h"
#include <sam.h>
#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
//      __   ___  ___         ___  __
//     |  \ |__  |__  | |\ | |__  /__`
//     |__/ |___ |    | | \| |___ .__/
//
//------------------------------------------------------------------------------

#define JOY_Y (PORT_PB08)
#define JOY_Y_GROUP (1)
#define JOY_Y_PIN (PIN_PB08%32)

#define JOY_X (PORT_PA02)
#define JOY_X_GROUP (0)
#define JOY_X_PIN (PIN_PA02%32)

#define JOY_PRESS (PORT_PA20)
#define JOY_PRESS_GROUP (0)
#define JOY_PRESS_PIN (PIN_PA20%32)

//------------------------------------------------------------------------------
//     ___      __   ___  __   ___  ___  __
//      |  \ / |__) |__  |  \ |__  |__  /__`
//      |   |  |    |___ |__/ |___ |    .__/
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//                __          __        ___  __
//     \  /  /\  |__) |  /\  |__) |    |__  /__`
//      \/  /~~\ |  \ | /~~\ |__) |___ |___ .__/
//
//------------------------------------------------------------------------------

//indicates whether or not the Y axis is being used as input
bool using_y = true;

//------------------------------------------------------------------------------
//      __   __   __  ___  __  ___      __   ___  __
//     |__) |__) /  \  |  /  \  |  \ / |__) |__  /__`
//     |    |  \ \__/  |  \__/  |   |  |    |___ .__/
//
//------------------------------------------------------------------------------

void joystick_sync();

//------------------------------------------------------------------------------
//      __        __          __
//     |__) |  | |__) |    | /  `
//     |    \__/ |__) |___ | \__,
//
//------------------------------------------------------------------------------

//==============================================================================
void joystick_init()
{
  //////////// Init the Joystick button /////////////
  PORT->Group[JOY_PRESS_GROUP].PINCFG[JOY_PRESS_PIN].bit.INEN = 1;
  PORT->Group[JOY_PRESS_GROUP].DIRCLR.reg = JOY_PRESS;
  PORT->Group[JOY_PRESS_GROUP].CTRL.bit.SAMPLING = 1;
  joystick_sync();
  
  //////////// Init the Joystick ADC /////////////
  // set the ADC clock to be the 48 MHz source
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_ADC) |// Generic Clock ADC
  GCLK_CLKCTRL_GEN_GCLK0     | // Generic Clock Generator 0 is source
  GCLK_CLKCTRL_CLKEN;  // enable
  joystick_sync();

  // enable the peripheral functions
  PORT->Group[JOY_Y_GROUP].PINCFG[JOY_Y_PIN].bit.PMUXEN = 1;
  PORT->Group[JOY_X_GROUP].PINCFG[JOY_X_PIN].bit.PMUXEN = 1;
  joystick_sync();

  // select peripheral function B
  PORT->Group[JOY_Y_GROUP].PMUX[JOY_Y_PIN / 2].bit.PMUXE = 0x1;
  PORT->Group[JOY_X_GROUP].PMUX[JOY_X_PIN / 2].bit.PMUXE = 0x1;
  joystick_sync();
  
  // set the ADC input pin (PB08 / AIN2) as an input
  PORT->Group[JOY_Y_GROUP].DIRSET.reg = JOY_Y;
  PORT->Group[JOY_X_GROUP].DIRSET.reg = JOY_X;
  joystick_sync();
  

  // set the ADC to constantly update
  ADC->CTRLB.bit.FREERUN = 1;
  joystick_sync();

  // set prescaler to 1/16
  ADC->CTRLB.bit.PRESCALER = 0x2;
  joystick_sync();
  
  // set the reference voltage
  // 1/2 VDDANA
  ADC->REFCTRL.bit.REFSEL = 0x2;
  joystick_sync();

  //Set the gain to 1/2
  ADC->INPUTCTRL.bit.GAIN = 0xf;
  joystick_sync();

  // set the input pins
  // pin AIN2 -- Analog Y-Axis
  ADC->INPUTCTRL.bit.MUXPOS = 0x02;
  joystick_sync();

  // ground
  ADC->INPUTCTRL.bit.MUXNEG = 0x19;
  joystick_sync();

  //turn on 16-bit output
  ADC->CTRLB.bit.RESSEL = 0;
  joystick_sync();

  // enable the ADC
  ADC->CTRLA.bit.ENABLE = 1;
  joystick_sync();

  //start the ADC
  ADC->SWTRIG.bit.START = 1;
  joystick_sync();
  
}


//==============================================================================
uint16_t joystick_get_Y_Value()
{
  uint16_t result;
  
  if (!using_y)
  {
    // set the input pins
    // pin AIN2 -- Analog Y-Axis
    ADC->INPUTCTRL.bit.MUXPOS = 0x02;
    joystick_sync();
    using_y = true;
    

    //Wait for a result to be ready
    while(!ADC->INTFLAG.bit.RESRDY);
    joystick_sync();
    
    //clear the result ready flag.
    ADC->INTFLAG.bit.RESRDY = 1;
    
    //Get the conversion, but don't use it.
    result = ADC->RESULT.reg;
    joystick_sync();
  }
  
  //Flush the current result
  ADC->SWTRIG.bit.FLUSH = 1;
  joystick_sync();
  
  //clear the result ready flag.
  ADC->INTFLAG.bit.RESRDY = 1;  
  
  //Wait for a result to be ready
  while(!ADC->INTFLAG.bit.RESRDY);
  joystick_sync();
  
  //clear the result ready flag.
  ADC->INTFLAG.bit.RESRDY = 1;

  result = ADC->RESULT.reg;
  joystick_sync();

  return result;
}

//==============================================================================
uint16_t joystick_get_X_Value()
{
  uint16_t result;
  
  if (using_y)
  {
    //Set the input pin to be the X-axis (PA02)
    ADC->INPUTCTRL.bit.MUXPOS = 0x00;
    joystick_sync();
    using_y = false;
    
    //Wait for a result to be ready
    while(!ADC->INTFLAG.bit.RESRDY);
    joystick_sync();
    
    //clear the result ready flag.
    ADC->INTFLAG.bit.RESRDY = 1;
    
    //Get the conversion, but don't use it
    result = ADC->RESULT.reg;
    joystick_sync();
  }
  
  //Flush the current result
  ADC->SWTRIG.bit.FLUSH = 1;
  joystick_sync();
  
  //clear the result ready flag.
  ADC->INTFLAG.bit.RESRDY = 1;
    
  //Wait for a result to be ready
  while(!ADC->INTFLAG.bit.RESRDY);
  joystick_sync();
  
  //clear the result ready flag.
  ADC->INTFLAG.bit.RESRDY = 1;
  
  result = ADC->RESULT.reg;
  joystick_sync();
  
  return result;
}

//==============================================================================
bool joystick_get_press()
{

  //Return true if the button is pressed.
  return (((PORT->Group[JOY_PRESS_GROUP].IN.reg & JOY_PRESS) >> 20) == 0);
}

//------------------------------------------------------------------------------
//      __   __              ___  ___
//     |__) |__) | \  /  /\   |  |__
//     |    |  \ |  \/  /~~\  |  |___
//
//------------------------------------------------------------------------------

//==============================================================================
void joystick_sync()
{
  // wait for the register to sync
  while(ADC->STATUS.bit.SYNCBUSY)
  {
    ;
  }
}

//------------------------------------------------------------------------------
//      __                  __        __        __
//     /  `  /\  |    |    |__)  /\  /  ` |__/ /__`
//     \__, /~~\ |___ |___ |__) /~~\ \__, |  \ .__/
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//        __   __  , __
//     | /__` |__)  /__`
//     | .__/ |  \  .__/
//
//------------------------------------------------------------------------------
