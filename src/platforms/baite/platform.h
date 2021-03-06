/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2017 Kevin Redon <kingkevin@cuvoodoo.info>
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

/* This file implements the platform specific functions for the STM32F103 
 * based ST-Link v2 clone from Baite (betemcu.cn)
 */
#ifndef __PLATFORM_H
#define __PLATFORM_H

#include "gpio.h"
#include "timing.h"
#include "timing_stm32.h"
#include "version.h"

#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/f1/memorymap.h>
#include <libopencm3/usb/usbd.h>

#ifdef ENABLE_DEBUG
# define PLATFORM_HAS_DEBUG
# define USBUART_DEBUG
#endif

#define PLATFORM_HAS_TRACESWO	1
#define NUM_TRACE_PACKETS		(128)		/* This is an 8K buffer */

#define BOARD_IDENT             "Black Magic Probe (Baite)"
#define BOARD_IDENT_DFU	        "Black Magic Probe (Upgrade) for Baite"
#define BOARD_IDENT_UPD	        "Black Magic Probe (DFU Upgrade) for Baite"
#define DFU_IDENT               "Black Magic Firmware Upgrade (Baite)"
#define UPD_IFACE_STRING        "@Internal Flash   /0x08000000/8*001Kg"

/* Important pin mappings for Bait platform:
 *
 * LED = PA9 (Red, active low, idle on)
 *
 * SRST = PB0 (output)
 * TDO/TRACESWO = PA6 (input)
 * TDI = PA7 (output)
 * TMS/SWDIO = PB12 (input/output)
 * TCK/SWCLK = PB13 (output)
 * TX = PB6 (output, USART1 remap)
 * RX = PB7 (input, USART1 remap, pulled high by external 620 ohms resistor)
 */

/* Hardware definitions... */
#define TDO_PORT	GPIOA
#define TDI_PORT	GPIOA
#define TMS_PORT	GPIOB
#define TCK_PORT	GPIOA
#define TDO_PIN		GPIO6
#define TDI_PIN		GPIO7
#define TMS_PIN		GPIO14
#define TCK_PIN		GPIO5

#define SWDIO_PORT 	TMS_PORT
#define SWCLK_PORT 	TCK_PORT
#define SWDIO_PIN	TMS_PIN
#define SWCLK_PIN	TCK_PIN

#define SRST_PORT	GPIOB
#define SRST_PIN	GPIO0

#define LED_PORT		GPIOA
#define LED_PORT_UART	LED_PORT
#define LED_PIN			GPIO9
#define LED_UART		LED_PIN
#define LED_IDLE_RUN	LED_PIN

# define SWD_CR   GPIO_CRH(SWDIO_PORT)
# define SWD_CR_MULT (1 << ((14 - 8) << 2))

#define TMS_SET_MODE() \
	gpio_set_mode(TMS_PORT, GPIO_MODE_OUTPUT_50_MHZ, \
	              GPIO_CNF_OUTPUT_PUSHPULL, TMS_PIN);
//Input, floating
#define SWDIO_MODE_FLOAT() 	do { \
	uint32_t cr = SWD_CR; \
	cr  &= ~(0xf * SWD_CR_MULT); \
	cr  |=  (0x4 * SWD_CR_MULT); \
	SWD_CR = cr; \
} while(0)
//Push-pull, 50Mhz
#define SWDIO_MODE_DRIVE() 	do { \
	uint32_t cr = SWD_CR; \
	cr  &= ~(0xf * SWD_CR_MULT); \
	cr  |=  (0x3 * SWD_CR_MULT); \
	SWD_CR = cr; \
} while(0)
#define UART_PIN_SETUP() \
	gpio_set_mode(USBUSART_PORT, GPIO_MODE_OUTPUT_2_MHZ,  \
	              GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, USBUSART_TX_PIN);
#define USB_DRIVER st_usbfs_v1_usb_driver
#define USB_IRQ    NVIC_USB_LP_CAN_RX0_IRQ
#define USB_ISR    usb_lp_can_rx0_isr
/* Interrupt priorities.  Low numbers are high priority.
 * For now USART1 preempts USB which may spin while buffer is drained.
 * TIM3 is used for traceswo capture and must be highest priority.
 */
#define IRQ_PRI_USB             (2 << 4)
#define IRQ_PRI_USBUSART        (1 << 4)
#define IRQ_PRI_USBUSART_TIM    (3 << 4)
#define IRQ_PRI_USB_VBUS        (14 << 4)
#define IRQ_PRI_TRACE           (0 << 4)

#define USBUSART USART1
#define USBUSART_CR1 USART1_CR1
#define USBUSART_IRQ NVIC_USART1_IRQ
#define USBUSART_CLK RCC_USART1
#define USBUSART_PORT GPIOB
#define USBUSART_TX_PIN GPIO6
#define USBUSART_RX_PIN GPIO7
#define USBUSART_ISR usart1_isr
#define USBUSART_TIM TIM4
#define USBUSART_TIM_CLK_EN() rcc_periph_clock_enable(RCC_TIM4)
#define USBUSART_TIM_IRQ NVIC_TIM4_IRQ
#define USBUSART_TIM_ISR tim4_isr

#define TRACE_TIM TIM3
#define TRACE_TIM_CLK_EN() rcc_periph_clock_enable(RCC_TIM3)
#define TRACE_IRQ   NVIC_TIM3_IRQ
#define TRACE_ISR   tim3_isr

#ifdef ENABLE_DEBUG
extern bool debug_bmp;
int usbuart_debug_write(const char *buf, size_t len);
# define DEBUG printf
#else
# define DEBUG(...)
#endif

#define SET_RUN_STATE(state)	{running_status = (state);}
#define SET_IDLE_STATE(state)	{gpio_set_val(LED_PORT, LED_PIN, !state);}
#define SET_ERROR_STATE(state)	{gpio_set_val(LED_PORT, LED_PIN, 1);}

/* Use newlib provided integer only stdio functions */
#define sscanf siscanf
#define sprintf siprintf
#define snprintf sniprintf
#define vasprintf vasiprintf

#endif
