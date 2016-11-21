#ifndef SPI_H
#define SPI_H

//------------------------------------------------------------------------------
//             __             __   ___  __
//     | |\ | /  ` |    |  | |  \ |__  /__`
//     | | \| \__, |___ \__/ |__/ |___ .__/
//
//------------------------------------------------------------------------------

#include "sam.h"

#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
//      __   ___  ___         ___  __
//     |  \ |__  |__  | |\ | |__  /__`
//     |__/ |___ |    | | \| |___ .__/
//
//------------------------------------------------------------------------------

#define NO_LOCK (0)
#define LED_LOCK (1)
#define VID_LOCK (2)

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
void spi_init();
void spi_write(uint8_t* data_packet);
uint16_t spi_read();
uint16_t spi(uint16_t data);
void spi_pulse_latch();
void spi_set_blank();
void spi_clear_blank();
void spi_pulse_gsclk();
bool spi_lock(uint8_t caller);
void spi_clear_lock();
void spi_pack_data(uint8_t* packed_data, uint16_t* pwm_values);
void spi_wait_for_unlock(uint8_t caller);

#endif /* SPI_H */
