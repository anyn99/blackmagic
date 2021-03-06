/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2013 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/scb.h>

#include "usbdfu.h"
#include "platform.h"

uint32_t app_address = 0x08002000;

void dfu_detach(void)
{
	/* Disconnect USB cable by resetting USB Device and pulling USB_DP low*/
	rcc_periph_reset_pulse(RST_USB);
	rcc_periph_clock_enable(RCC_USB);
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_clear(GPIOA, GPIO12);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
	              GPIO12);
	/* USB device must detach, we just reset... */
	scb_reset_system();
}

int main(void)
{
	if((GPIOA_CRL & 0x40) == 0x40) {
		dfu_jump_app_if_valid();
	}
	dfu_protect(DFU_MODE);

	rcc_clock_setup_in_hse_8mhz_out_72mhz();
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	systick_set_reload(900000);

	/* Configure USB related clocks and pins.
	 * Just in case: Disconnect USB cable by resetting USB Device and pulling
	 * USB_DP low. Device will reconnect automatically as pull-up is hard wired
	 */
	rcc_periph_reset_pulse(RST_USB);
	rcc_periph_clock_enable(RCC_USB);
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_clear(GPIOA, GPIO12);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
	              GPIO12);

	/* Configure the LED pins. */
	gpio_set_mode(LED_PORT, GPIO_MODE_OUTPUT_2_MHZ,	GPIO_CNF_OUTPUT_PUSHPULL,
	              LED_PIN);

	systick_interrupt_enable();
	systick_counter_enable();

	dfu_init(&st_usbfs_v1_usb_driver, DFU_MODE);

	dfu_main();
}

void dfu_event(void)
{
}

void sys_tick_handler(void)
{
	/* blink LED to show activity */
	gpio_toggle(LED_PORT, LED_PIN);
}

