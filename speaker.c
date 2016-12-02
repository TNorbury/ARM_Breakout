//------------------------------------------------------------------------------
//             __             __   ___  __
//     | |\ | /  ` |    |  | |  \ |__  /__`
//     | | \| \__, |___ \__/ |__/ |___ .__/
//
//------------------------------------------------------------------------------

#include "speaker.h"
#include "sam.h"

//------------------------------------------------------------------------------
//      __   ___  ___         ___  __
//     |  \ |__  |__  | |\ | |__  /__`
//     |__/ |___ |    | | \| |___ .__/
//
//------------------------------------------------------------------------------

#define SPEAKER (PORT_PA09)
#define SPEAKER_GROUP (0)
#define SPEAKER_PIN (PIN_PA09%32)

// Constants for Clock multiplexers
#define GENERIC_CLOCK_MULTIPLEXER_TCC0_TCC1 (0x1A)

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

//------------------------------------------------------------------------------
//      __   __   __  ___  __  ___      __   ___  __
//     |__) |__) /  \  |  /  \  |  \ / |__) |__  /__`
//     |    |  \ \__/  |  \__/  |   |  |    |___ .__/
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//      __        __          __
//     |__) |  | |__) |    | /  `
//     |    \__/ |__) |___ | \__,
//
//------------------------------------------------------------------------------

//==============================================================================
void speaker_init()
{
  //Put Generic Clock Generator 1 as source for Generic Clock Multiplexer 0x1A
  // Generic Clock Multiplexer 0
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID( GENERIC_CLOCK_MULTIPLEXER_TCC0_TCC1 ) |
  GCLK_CLKCTRL_GEN_GCLK1 | // Generic Clock Generator 1 is source
  GCLK_CLKCTRL_CLKEN ;
  
  // Enable Multiplexing
  PORT->Group[SPEAKER_GROUP].PINCFG[SPEAKER_PIN].bit.PMUXEN = 1;
  
  // Configure Multiplexing
  PORT->Group[SPEAKER_GROUP].PMUX[SPEAKER_PIN / 2].bit.PMUXO = 0x5;


  //Enable the TCC bus clock (CLK_TCCx_APB)
  PM->APBCMASK.bit.TCC1_ = 1;
  
  // Enable TCC1 for speaker
  TCC1->CTRLA.bit.PRESCALER   = 0;
  
  //Select Prescaler Synchronization setting in Control A register 
  TCC1->CTRLA.bit.PRESCSYNC   = 0;
  
  TCC1->CTRLA.bit.RESOLUTION  = 0;
  
  //  If down-counting operation must be enabled, write a one to the Counter
  //Direction bit in the Control B Set register (CTRLBSET.DIR)
  TCC1->CTRLBSET.bit.DIR      = 0;
  while ( TCC1->SYNCBUSY.bit.CTRLB )
  {
    ;
  }
  
  //   Select the Waveform Generation operation in WAVE register (WAVE.WAVEGEN)
  TCC1->WAVE.bit.WAVEGEN      = 0x1;      // Match frequency
  while ( TCC1->SYNCBUSY.bit.WAVE )
  {
    ;
  }
  
  //   Select the Waveform Output Polarity in the WAVE register (WAVE.POL)
  TCC1->WAVE.vec.POL          = 0;
  while ( TCC1->SYNCBUSY.bit.WAVE )
  {
    ;
  }
  
  //   The waveform output can be inverted for the individual channels using the
  // Waveform Output Invert Enable bit group in the Driver register
  //(DRVCTRL.INVEN)
  TCC1->DRVCTRL.vec.INVEN     = 0;        // No inversion
  
  // CC0 for all outputs
  TCC1->WEXCTRL.bit.OTMX      = 0x2;

  TCC1->CTRLBCLR.bit.LUPD = 1;
  while (TCC1->SYNCBUSY.bit.CC0)
  {
    ;
  }
  while(TCC1->SYNCBUSY.bit.CTRLB);
}

void speaker_set(uint32_t frequency)
{
  TCC1->CCB[0].bit.CCB = (frequency / 2);
  while (TCC1->SYNCBUSY.bit.CCB0)
  {
    ;
  }
}

void speaker_disable()
{
  while (TCC1 ->SYNCBUSY.bit.ENABLE)
  {
    ;
  }
  TCC1->CTRLA.bit.ENABLE = 0;
  while (TCC1 ->SYNCBUSY.bit.ENABLE)
  {
    ;
  }
}

void speaker_enable()
{
  TCC1->CTRLA.bit.ENABLE = 1;
}

//------------------------------------------------------------------------------
//      __   __              ___  ___
//     |__) |__) | \  /  /\   |  |__
//     |    |  \ |  \/  /~~\  |  |___
//
//------------------------------------------------------------------------------

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
