// SPDX-License-Identifier: zlib-acknowledgement

// QUESTIONS:
// 1. How to connect JTAG via external board if remove SWD from ST-Link? (go into pinout for breadboard connection)
// 2. (only required if not using ST-Link?) Provide power to board (JLinkExe -CommandFile .vscode/jlink/power-on/off.jlink:
//    - power on/off
//    - exit
// 3. Outside of Ozone, use probe debugger tools, e.g. JLinkGDBServer
//    - have one configuration for flashing/stopping and other for attaching

#include <libopencm3/stm32/rcc.h>

#define LED_PORT (GPIOA)
#define LED_PIN (GPIO5)

static void
rcc_setup(void)
{
  // RCC takes source and feeds to PLL
  // PLL is feedback hardware device that can take one signal and output one/many signals of different frequencies and phases
  // output of one will go to CPU
  rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

static void
gpio_setup(void)
{
  // in MCUs all peripherals off by default.
  // effectively 'turn them on' by enabling clock signal
  rcc_periph_clock_enable(RCC_GPIOA);

  // as hardware involved, the atomicity provided by ORing pins is good
  gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_PIN);
}

// 4.2billion enough
static void
delay_cycles(u32 cycles)
{
  for (u32 i = 0; i < cycles; i += 1)
  {
    __asm__("nop");
  }
}

// IMPORTANT: recognise that the datasheet is written for diverse group of people with different requirements
// e.g, if firmware engineer working on dev-board, not concerned with hardware engineer building board CPU-die package size, electrical characteristics for power supply etc.
// more concerned with peripheral capabilities
// Reference manual is more targeted for firmware developers

// ARM is owns intellectual property
// cortex-m4 TRM useful to have so can know what is common across all chips, and what is specific to yours 
// also cortex-m4 programming manual

int
main(void)
{
  rcc_setup();
  gpio_setup();

  while (1)
  {
    gpio_toggle(LED_PORT, LED_PIN);  
    // 4 to roughly account for the for-loop cmp and add
    delay_cycles(84000000 / 4);
  }

  return 0;
}
