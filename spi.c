//------------------------------------------------------------------------------
//             __             __   ___  __
//     | |\ | /  ` |    |  | |  \ |__  /__`
//     | | \| \__, |___ \__/ |__/ |___ .__/
//
//------------------------------------------------------------------------------

#include "spi.h"
#include "sam.h"
//#include "counter.h"
#include <stdbool.h>


//------------------------------------------------------------------------------
//      __   ___  ___         ___  __
//     |  \ |__  |__  | |\ | |__  /__`
//     |__/ |___ |    | | \| |___ .__/
//
//------------------------------------------------------------------------------

#define SPI_MOSI (PORT_PB10)
#define SPI_MOSI_GROUP (1)
#define SPI_MOSI_PIN (PIN_PB10%32)
#define SPI_MOSI_PMUX (SPI_MOSI_PIN/2)

#define SPI_MISO (PORT_PA12)
#define SPI_MISO_GROUP (0)
#define SPI_MISO_PIN (PIN_PA12%32)
#define SPI_MISO_PMUX (SPI_MISO_PIN/2)

#define SPI_SCK (PORT_PB11)
#define SPI_SCK_GROUP (1)
#define SPI_SCK_PIN (PIN_PB11%32)
#define SPI_SCK_PMUX (SPI_SCK_PIN/2)

#define SPI_LAT (PORT_PA14)
#define SPI_LAT_GROUP (0)
#define SPI_LAT_PIN (PIN_PA14%32)
#define SPI_LAT_PMUX (SPI_LAT_PIN/2)

#define SPI_BLANK (PORT_PA07)
#define SPI_BLANK_GROUP (0)
#define SPI_BLANK_PIN (PIN_PA07%32)

#define SPI_LED_GSCLK (PORT_PA15)
#define SPI_LED_GSCLK_GROUP (0)
#define SPI_LED_GSCLK_PIN (PIN_PA15%32)


#define PACKET_SIZE (24)

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

static volatile uint8_t lock = NO_LOCK;
static volatile uint8_t DATA_PACKET[PACKET_SIZE];
static volatile uint8_t packet_position;

//------------------------------------------------------------------------------
//      __   __   __  ___  __  ___      __   ___  __
//     |__) |__) /  \  |  /  \  |  \ / |__) |__  /__`
//     |    |  \ \__/  |  \__/  |   |  |    |___ .__/
//
//------------------------------------------------------------------------------

void led_spi_handler();
void video_spi_handler();

//------------------------------------------------------------------------------
//      __        __          __
//     |__) |  | |__) |    | /  `
//     |    \__/ |__) |___ | \__,
//
//------------------------------------------------------------------------------

//==============================================================================
void spi_init()
{
  #if (SPI_MOSI_PIN % 2) // Odd Pin
  PORT->Group[SPI_MOSI_GROUP].PMUX[SPI_MOSI_PMUX].bit.PMUXO
  = PORT_PMUX_PMUXO_D_Val;
  #else                  // Even Pin
  PORT->Group[SPI_MOSI_GROUP].PMUX[SPI_MOSI_PMUX].bit.PMUXE
  = PORT_PMUX_PMUXE_D_Val;
  #endif
  // Enable the PMUX
  PORT->Group[SPI_MOSI_GROUP].PINCFG[SPI_MOSI_PIN].bit.PMUXEN = 1;

  // MISO
  // Configure the appropriate peripheral
  #if (SPI_MISO_PIN % 2) // Odd Pin
  PORT->Group[SPI_MISO_GROUP].PMUX[SPI_MISO_PMUX].bit.PMUXO
  = PORT_PMUX_PMUXO_D_Val;
  #else                  // Even Pin
  PORT->Group[SPI_MISO_GROUP].PMUX[SPI_MISO_PMUX].bit.PMUXE
  = PORT_PMUX_PMUXE_D_Val;
  #endif
  // Enable the PMUX
  PORT->Group[SPI_MISO_GROUP].PINCFG[SPI_MISO_PIN].bit.PMUXEN = 1;

  // SCK
  // Configure the appropriate peripheral
  #if (SPI_SCK_PIN % 2) // Odd Pin
  PORT->Group[SPI_SCK_GROUP].PMUX[SPI_SCK_PMUX].bit.PMUXO
  = PORT_PMUX_PMUXO_D_Val;
  #else                  // Even Pin
  PORT->Group[SPI_SCK_GROUP].PMUX[SPI_SCK_PMUX].bit.PMUXE
  = PORT_PMUX_PMUXE_D_Val;
  #endif
  // Enable the PMUX
  PORT->Group[SPI_SCK_GROUP].PINCFG[SPI_SCK_PIN].bit.PMUXEN = 1;


  //Disable the SPI
  SERCOM4->SPI.CTRLA.bit.ENABLE = 0;
  while (SERCOM4->SPI.CTRLA.bit.ENABLE)
  {
  }

  //////////////////////////////////////////////////////////////////////////////
  // Enable the Latch
  //////////////////////////////////////////////////////////////////////////////
  PORT->Group[SPI_LAT_GROUP].DIRSET.reg = SPI_LAT;

  //////////////////////////////////////////////////////////////////////////////
  // Enable the Blank
  //////////////////////////////////////////////////////////////////////////////
  PORT->Group[SPI_BLANK_GROUP].DIRSET.reg = SPI_BLANK;

  //////////////////////////////////////////////////////////////////////////////
  // Enable the GSCLK
  //////////////////////////////////////////////////////////////////////////////
  PORT->Group[SPI_LED_GSCLK_GROUP].DIRSET.reg = SPI_LED_GSCLK;

  PM->APBCMASK.reg |= PM_APBCMASK_SERCOM4;

  // Initialize the GCLK
  // Setting clock for the SERCOM4_CORE clock
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_SERCOM4_CORE) |
  GCLK_CLKCTRL_GEN_GCLK0 |
  GCLK_CLKCTRL_CLKEN ;

  // Wait for the GCLK to be synchronized
  while(GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

  //////////////////////////////////////////////////////////////////////////////
  // Initialize the SPI
  //////////////////////////////////////////////////////////////////////////////

  // Reset the SPI
  SERCOM4->SPI.CTRLA.bit.SWRST = 1;
  // Wait for it to complete
  while (SERCOM4->SPI.CTRLA.bit.SWRST || SERCOM4->SPI.SYNCBUSY.bit.SWRST);

  // Set up CTRLA
  SERCOM4->SPI.CTRLA.bit.DIPO = 0; // MISO on PAD0
  SERCOM4->SPI.CTRLA.bit.DOPO = 1; // MOSI on PAD2, SCK on PAD3, SS on PAD 1
  SERCOM4->SPI.CTRLA.bit.DORD = 0; // MSB Transferred first
  SERCOM4->SPI.CTRLA.bit.CPOL = 0; // SCK Low when Idle
  SERCOM4->SPI.CTRLA.bit.CPHA = 0; // Data sampled on leading edge and change
  // on trailing edge
  SERCOM4->SPI.CTRLA.bit.MODE = 3; // Set MODE as SPI Master

  // Set up CTRLB
  SERCOM4->SPI.CTRLB.bit.RXEN = 1; // Enable the receiver

  // Set up the BAUD rate
  SERCOM4->SPI.BAUD.reg = 3; //100KHz - too slow, but easy to see on the Logic
  // Analyzer

  //////////////////////////////////////////////////////////////////////////////
  // Enable Interrupts
  //////////////////////////////////////////////////////////////////////////////
  NVIC_DisableIRQ(SERCOM4_IRQn);
  NVIC_ClearPendingIRQ(SERCOM4_IRQn);
  NVIC_SetPriority(SERCOM4_IRQn, 0);
  NVIC_EnableIRQ(SERCOM4_IRQn);

  SERCOM4->SPI.INTENSET.bit.TXC = 1;



  //////////////////////////////////////////////////////////////////////////////
  // Enable the SPI
  //////////////////////////////////////////////////////////////////////////////
  SERCOM4->SPI.CTRLA.bit.ENABLE = 1;
  // Wait for it to complete
  while (SERCOM4->SPI.SYNCBUSY.bit.ENABLE);
}

//==============================================================================
void spi_write(uint8_t* data)
{
  
  if (LED_LOCK == lock)
  {
    SERCOM4->SPI.CTRLA.bit.ENABLE = 0;
    // Wait for it to complete
    while (SERCOM4->SPI.SYNCBUSY.bit.ENABLE);
    
    SERCOM4->SPI.BAUD.reg = 293;
    
    SERCOM4->SPI.CTRLA.bit.ENABLE = 1;
    // Wait for it to complete
    while (SERCOM4->SPI.SYNCBUSY.bit.ENABLE);
    
    NVIC_EnableIRQ(SERCOM4_IRQn);
    //Make a copy of the data packet
    for (int i = 0; i < PACKET_SIZE; i++)
    {
      DATA_PACKET[i] = data[i];
    }
    packet_position = 0;
    
    //Send the first item in the packet and then increment the packet position
    SERCOM4->SPI.DATA.bit.DATA = DATA_PACKET[packet_position];
    packet_position ++;
  }
  else if (VID_LOCK == lock)
  {
    SERCOM4->SPI.CTRLA.bit.ENABLE = 0;
    // Wait for it to complete
    while (SERCOM4->SPI.SYNCBUSY.bit.ENABLE);
    
    SERCOM4->SPI.BAUD.reg = 3;
    
    SERCOM4->SPI.CTRLA.bit.ENABLE = 1;
    // Wait for it to complete
    while (SERCOM4->SPI.SYNCBUSY.bit.ENABLE);
    
    NVIC_DisableIRQ(SERCOM4_IRQn);
    
    // Wait for the data register to be empty
    while (SERCOM4->SPI.INTFLAG.bit.DRE == 0);
    // Send the data
    SERCOM4->SPI.DATA.bit.DATA = data;
    // Wait for transfer complete
    while( SERCOM4->SPI.INTFLAG.bit.TXC == 0 || SERCOM4->SPI.INTFLAG.bit.DRE == 0 );
    spi_clear_lock();
  }
}

//==============================================================================
uint16_t spi_read()
{
  // Wait for something to show up in the data register
  while(SERCOM4->SPI.INTFLAG.bit.DRE == 0 || SERCOM4->SPI.INTFLAG.bit.RXC == 0);

  // Read it and return it.
  return SERCOM4->SPI.DATA.bit.DATA;
}

//==============================================================================
uint16_t spi(uint16_t data)
{
  // Wait for the data register to be empty
  while (SERCOM4->SPI.INTFLAG.bit.DRE == 0);

  // Send the data
  SERCOM4->SPI.DATA.bit.DATA = data;

  // Wait for something to show up in the data register
  while(SERCOM4->SPI.INTFLAG.bit.DRE == 0 || SERCOM4->SPI.INTFLAG.bit.RXC == 0);

  // Read it and return it.
  return SERCOM4->SPI.DATA.bit.DATA;
}

//==============================================================================
void spi_pulse_latch()
{
  PORT->Group[SPI_LAT_GROUP].OUTSET.reg = SPI_LAT;
  PORT->Group[SPI_LAT_GROUP].OUTCLR.reg = SPI_LAT;
}

//==============================================================================
void spi_set_blank()
{
  PORT->Group[SPI_BLANK_GROUP].OUTSET.reg = SPI_BLANK;
}

//==============================================================================
void spi_clear_blank()
{
  PORT->Group[SPI_BLANK_GROUP].OUTCLR.reg = SPI_BLANK;
}

//==============================================================================
void spi_pulse_gsclk()
{
  PORT->Group[SPI_LED_GSCLK_GROUP].OUTTGL.reg = SPI_LED_GSCLK;
  //PORT->Group[SPI_LED_GSCLK_GROUP].OUTSET.reg = SPI_LED_GSCLK;
  //PORT->Group[SPI_LED_GSCLK_GROUP].OUTCLR.reg = SPI_LED_GSCLK;
}

//==============================================================================
bool spi_lock(uint8_t caller)
{
  bool retval = false;
  
  // See if the system is locked
  if (NO_LOCK == lock)
  {
    lock = caller;
    retval = true;
  }
  return retval;
}

//==============================================================================
void spi_clear_lock()
{
  lock = NO_LOCK;
}


//==============================================================================
void spi_pack_data(uint8_t* packed_data, uint16_t* pwm_values)
{
  for (uint8_t i = 0; i < 8; i++) {
    packed_data[0+3*i] = pwm_values[15-2*i] >> 4 & 0xff;
    packed_data[1+3*i] = (pwm_values[15-2*i] << 4 & 0xf0)
    | (pwm_values[14-2*i] >> 8 & 0x0f);
    packed_data[2+3*i] = pwm_values[14-2*i] & 0xff;
  }
}

//==============================================================================
void spi_wait_for_unlock(uint8_t caller)
{
  while(!spi_lock(caller))
  {
  }
}

//------------------------------------------------------------------------------
//      __   __              ___  ___
//     |__) |__) | \  /  /\   |  |__
//     |    |  \ |  \/  /~~\  |  |___
//
//------------------------------------------------------------------------------
//==============================================================================
void led_spi_handler()
{
  //If the end of the packet hasn't been reached, then send the next item in the
  //packet and increment the counter.
  if (24 > packet_position)
  {
    SERCOM4->SPI.DATA.bit.DATA = DATA_PACKET[packet_position];
    packet_position ++;
  }

  //Otherwise, reset the packet position and unlock the SPI
  else
  {
    //lock = false;
    packet_position = 0;
    
    //Let the counter know that there is data ready to be sent.
    //counter_queue_transer();
  }
}

//==============================================================================
void video_spi_handler()
{
  spi_clear_lock();
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
void SERCOM4_Handler()
{
  ////Clear the flag
  //SERCOM4->SPI.INTFLAG.bit.TXC = 1;
//
  //switch (lock)
  //{
    //case LED_LOCK:
    //led_spi_handler();
    //break;
    //
    //case VID_LOCK:
    //video_spi_handler();
    //break;
    //
    //default:
    //break;
  //}
}