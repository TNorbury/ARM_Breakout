//------------------------------------------------------------------------------
//             __             __   ___  __
//     | |\ | /  ` |    |  | |  \ |__  /__`
//     | | \| \__, |___ \__/ |__/ |___ .__/
//
//------------------------------------------------------------------------------

#include "video.h"
#include "spi.h"
#include "sam.h"
#include "delay.h"

#include <strings.h>

//------------------------------------------------------------------------------
//      __   ___  ___         ___  __
//     |  \ |__  |__  | |\ | |__  /__`
//     |__/ |___ |    | | \| |___ .__/
//
//------------------------------------------------------------------------------

#define VIDEO_RST (PORT_PA18)
#define VIDEO_RST_GROUP (0)
#define VIDEO_RST_PIN (PIN_PA18%32)

#define VIDEO_CS (PORT_PA21)
#define VIDEO_CS_GROUP (0)
#define VIDEO_CS_PIN (PIN_PA21%32)

#define NO_OP                           0x0000
#define DISPLAY_DUTY                    0x0001
#define	RGB_INTERFACE					0x0002
#define	ENTRY_MODE						0x0003
#define	DISPLAY_CONTROL1				0x0005
#define	STAND_BY						0x0010
#define	OSC_REG							0x0018
#define	POWER_CONTROL3					0x00F8
#define	POWER_CONTROL4					0x00F9
#define	GRAM_ADDRESS_SET_X				0x0020
#define	GRAM_ADDRESS_SET_Y				0x0021
#define	GRAM_DATA_WRITE					0x0022
#define	GRAM_DATA_READ					0x0022
#define	IF_8BIT_BUS						0x0024
#define	VER_WINDOW_BEG					0x0035
#define	VER_WINDOW_END					0x0036
#define	HOR_WINDOW_BEGEND				0x0037
#define	GAMMA_TOPBOT_R					0x0070
#define	GAMMA_TOPBOT_G					0x0071
#define	GAMMA_TOPBOT_B					0x0072
#define	GAMMA_R12						0x0073
#define	GAMMA_R34						0x0074
#define	GAMMA_G12						0x0075
#define	GAMMA_G34						0x0076
#define	GAMMA_B12						0x0077
#define	GAMMA_B34						0x0078
#define	GAMMA_Q							0x0080

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

static void video(uint16_t index, uint16_t parameter);
static void video_index(uint16_t val);
static void video_parameter(uint16_t val);
static inline void chipsel_on();
static inline void chipsel_off();
static void video_draw_test_screen();
static void write_to_spi(uint8_t data);

//------------------------------------------------------------------------------
//      __        __          __
//     |__) |  | |__) |    | /  `
//     |    \__/ |__) |___ | \__,
//
//------------------------------------------------------------------------------

//==============================================================================
void video_init()
{
	// Set up the Video pins as outputs
	PORT->Group[VIDEO_RST_GROUP].DIRSET.reg = VIDEO_RST;
	PORT->Group[VIDEO_CS_GROUP].DIRSET.reg = VIDEO_CS;

	// Turn the outputs "off"
	PORT->Group[VIDEO_RST_GROUP].OUTSET.reg = VIDEO_RST; // active low
	PORT->Group[VIDEO_CS_GROUP].OUTSET.reg = VIDEO_CS;   // active low

	// Enable the SPI
	spi_init();

	// Wait here for a bit until we know the reset pulse has been big enough.
	// PAMO-C0201QILK-C Application Note - 3. Power Sequence
	DelayMs(500);
	PORT->Group[VIDEO_RST_GROUP].OUTCLR.reg = VIDEO_RST; // turn it on
	DelayMs(20); // Leave it on for a bit.
	PORT->Group[VIDEO_RST_GROUP].OUTSET.reg = VIDEO_RST; // turn it off
	DelayMs(10);
	video(STAND_BY, 0x0000); // Cancel Standby Mode
	DelayMs(100);
	// The following are specified by page 12 of PAMO-C0201QILK-C.pdf
	video(POWER_CONTROL3, 0x000F);
	video(POWER_CONTROL4, 0x0019);
	DelayMs(32);

	// The following are specified by page 12 of PAMO-C0201QILK-C.pdf

	// Set FP and BP to 8 and the Number of Lines to be 240x224
	// Our display is 176x220. I don't know why they picked 224 here.
	// I guess it is just the first thing bigger than 220. That said, the
	// X direction doesn't begin until 0x20 (32) - which, if
	video(DISPLAY_DUTY, 0x881C);


	// The RGB Interface is unused - Set DM to 0.
	video(RGB_INTERFACE, 0x0000);
	// Set the SS bit in the Entry Mode to put 0,0 at the top  left
	// Set up the ID to auto increase the Address Counter after GRAM write
	// Increase X in the window until it hits the edge, then return and increment Y
	video(ENTRY_MODE, 0x0130);
	// Turn off the Display
	video(DISPLAY_CONTROL1, 0x0000); // Turn off display
	//video(STAND_BY, 0x0000);
	// Set the Internal Oscilllator to Max
	video(OSC_REG, 0x0028);
	// The X address starts at 0x20. Why?? No clue.
	// 176 + 32 is 198. So .. that's not it. However, 176 + 64 = 240.  I wonder
	// if the designer figured on a 240 wide and just slapped te 176 in the middle?
	// Again - no clue. But - we need to start the X at 0x0020 to get it to show up
	// at the left edge of the screen.
	video(GRAM_ADDRESS_SET_X, 0x0020);
	// The Vertical starts at 0 ... that makes sense.
	video(GRAM_ADDRESS_SET_Y, 0x0000); // Vertical starts at 0.
	// Set up the window to match the entire screen.
	video(VER_WINDOW_BEG, 0x0000); // Window vertical start (0)
	video(VER_WINDOW_END, 0x00DB); // Window vertical end (219)
	// Horizontal starts at 0x20 and last for 0xAF (176 - 1) - so the end is 0xCF.
	video(HOR_WINDOW_BEGEND, 0x20CF);
	
	// Gamma settings from the test doc
	video(GAMMA_TOPBOT_R, 0x3300);
	video(GAMMA_TOPBOT_G, 0x3900);
	video(GAMMA_TOPBOT_B, 0x3800);
	video(GAMMA_R12, 0x2924);
	video(GAMMA_R34, 0x261C);
	video(GAMMA_G12, 0x3125);
	video(GAMMA_G34, 0x271C);
	video(GAMMA_B12, 0x352A);
	video(GAMMA_B34, 0x2A1E);

	//video_draw_test_screen();

	// Turn on the display
	video(DISPLAY_CONTROL1, 0x0001);
}

//==============================================================================
void video_off()
{
	video(DISPLAY_CONTROL1, 0x0000);
}

//==============================================================================
void video_on()
{
	video(DISPLAY_CONTROL1, 0x0001);
}

//==============================================================================
void video_set_window(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
	uint16_t begend;
	
	// Do the 0x20 shift on X so that we can deal with a base 0 window system.
	x = x + 0x20;
	
	// Set up the vertical window
	video(VER_WINDOW_BEG, y); // Window vertical start
	video(VER_WINDOW_END, y + height -1); // Window vertical end (-1)
	
	// Set up the horizontal window.
	begend = (x << 8) | (x + width -1);
	video(HOR_WINDOW_BEGEND, begend);
}

//==============================================================================
void video_paint_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height,
uint16_t color)
{
	uint16_t i;
	video_set_window(x,y,width,height);
	video(GRAM_ADDRESS_SET_X, x + 0x0020);
	video(GRAM_ADDRESS_SET_Y, y);
	video_index(GRAM_DATA_WRITE);
	chipsel_on();
	write_to_spi(0x72);
	
	for (i = 0; i < width * height; i++)
	{
		//video_parameter(color);
		write_to_spi(color >> 8);
		write_to_spi(color & 0xFF);
	}
	chipsel_off();
	
	video(NO_OP, 0x0000);
}

//==============================================================================
void video_paint_string(char* string, font_t* font, uint8_t x, uint8_t y,
 uint16_t fg, uint16_t bg)
{
	uint16_t i;
	uint16_t string_width;
	uint8_t character;
	uint8_t byte_size = 8;
  uint8_t row_height;
	uint8_t char_row;

  if (font->height == 16)
  {
    row_height = font->height * 2;
  }
  else
  {
    row_height = font->height;
  }

	video_set_window(x,y, strlen(string) * font->width , font->height);
	video(GRAM_ADDRESS_SET_X, x + 0x0020);
	video(GRAM_ADDRESS_SET_Y, y);
	video_index(GRAM_DATA_WRITE);
	chipsel_on();
	write_to_spi(0x72);

	for (i = 0; i < font->height; i++)
	{
		for (int j = 0; j < strlen(string); j++)
		{
			character = string[j] - 32;

      if (font->height == 16)
      {
        char_row = font->ptr[(character * row_height) + (i * 2)];
        //byte_size = 8;
      }
      else if (font->width == 6)
      {
        byte_size = font->width;
        char_row = font->ptr[(character * row_height) + i];
      }
      else
      {
        char_row = font->ptr[(character * row_height) + i];        
      }
      
			for (int k = 0; k < byte_size; k++)
			{
				if (((char_row & 0x80) >> 7) == 1)
				{
						write_to_spi(fg >> 8);
						write_to_spi(fg & 0xFF);
				}
				else
				{
						write_to_spi(bg >> 8);
						write_to_spi(bg & 0xFF);
				}
				char_row = char_row << 1;
			}
      
      if (font->height == 16)
      {
        char_row = font->ptr[(character * row_height) + (i * 2) + 1];
        
        for (int k = 0; k < 4; k++)
        {
          if (((char_row & 0x80) >> 7) == 1)
          {
            write_to_spi(fg >> 8);
            write_to_spi(fg & 0xFF);
          }
          else
          {
            write_to_spi(bg >> 8);
            write_to_spi(bg & 0xFF);
          }
          char_row = char_row << 1;
        }
			}
		}
	}

	chipsel_off();
	
	video(NO_OP, 0x0000);
}


//------------------------------------------------------------------------------
//      __   __              ___  ___
//     |__) |__) | \  /  /\   |  |__
//     |    |  \ |  \/  /~~\  |  |___
//
//------------------------------------------------------------------------------

//==============================================================================
static void video(uint16_t index, uint16_t parameter)
{
	video_index(index);
	video_parameter(parameter);
}

//==============================================================================
static void video_index(uint16_t val)
{
	chipsel_on();
	write_to_spi(0x70);
	write_to_spi(0x00);
	write_to_spi(val & 0x00FF);
	chipsel_off();
}

//==============================================================================
static void video_parameter(uint16_t val)
{
	chipsel_on();
	write_to_spi(0x72);
	write_to_spi(val >> 8);
	write_to_spi(val & 0xFF);
	chipsel_off();
}

//==============================================================================
static void video_draw_test_screen()
{
	uint16_t i,j;
	uint16_t color;

	video_index(GRAM_DATA_WRITE);

	for (i = 0; i < 176*220; i++)
	{
		color = 0x7BEF;
		if (i >= 176*20) color = 0xF800; // Red
		if (i >= 176*40) color = 0x07E0; // Green
		if (i >= 176*60) color = 0x001F; // Blue
		if (i >= 176*80) color = 0x0000; // Black
		if (i >= 176*100) color = 0xFFFF; // White
		if (i >= 176*120) color = 0xF81F; // Magenta
		if (i >= 176*140) color = 0xFFE0; // Yellow
		if (i >= 176*160) color = 0x07FF; // Cyan
		if (i >= 176*180) color = 0xF3E0; // Orange
		if (i >= 176*200) color = 0x781F; // Purple

		video_parameter(color);
	}
	// Write address 0 to stop the cycle
	video(NO_OP, 0x0000);

	// Let's draw a square in the middle of the screen. Make it RED.
	color = 0xF800;
	for (i = 20; i < 120; i++)
	{
		video(GRAM_ADDRESS_SET_X, (uint16_t) (0x0020 + 10));
		video(GRAM_ADDRESS_SET_Y, (uint16_t) (i));
		video_index(GRAM_DATA_WRITE);

		for (j = 0; j < 10; j++)
		{
			video_parameter(color);
		}
		video(NO_OP, 0x0000);
	}
}

//==============================================================================
static void inline chipsel_on()
{
	PORT->Group[VIDEO_CS_GROUP].OUTCLR.reg = VIDEO_CS; // active low
}

//==============================================================================
static void inline chipsel_off()
{
	PORT->Group[VIDEO_CS_GROUP].OUTSET.reg = VIDEO_CS; // active low
}

//==============================================================================
static void write_to_spi(uint8_t data)
{
    spi_wait_for_unlock(VID_LOCK);
    spi_write(data);
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
