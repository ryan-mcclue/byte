// SPDX-License-Identifier: zlib-acknowledgement

// QUESTIONS:
// 1. How to connect JTAG via external board if remove SWD from ST-Link? (go into pinout for breadboard connection)
// 2. (only required if not using ST-Link?) Provide power to board (JLinkExe -CommandFile .vscode/jlink/power-on/off.jlink:
//    - power on/off
//    - exit
// 3. Outside of Ozone, use probe debugger tools, e.g. JLinkGDBServer
//    - have one configuration for flashing/stopping and other for attaching

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/systick.h>
// IVT is a data structure; the last of which is NVIC for peripheral interrupts
#include <libopencm3/cm3/vector.h>

#define LED_PORT (GPIOA)
#define LED_PIN (GPIO5)

// interrupt handlers:
// void null_handler(void) {}
// void blocking_handler(void) { while(1){} } 

volatile u64 ticks = 0;
void
systick_handler(void)
{
  // non-atomic as will require 2 instructions
  ticks++;
}

static u64 get_ticks(void)
{
  return ticks;
}

// assembly: .if 0 .endif

void timer_setup(void)
{
  rcc_periph_clock_enable(RCC_TIM2);

  timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

  // channel specific
  timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_PWM1);

  timer_enable_counter(TIM2);
  timer_enable_oc_output(TIM2, TIM_OC1);

  // subtract 1 as cannot have 0, but don't want to waste the 0 value
  // freq = system_freq / ((prescaler - 1) * (arr - 1))
  // want, 1KHz, with 1000 possible places, i.e. resolution
  timer_set_prescaler(TIM2, 84 - 1);
  timer_set_period(TIM2, 1000 - 1);
}

void set_duty_cycle(f32 duty_cycle)
{
  // ccr = arr * (duty-cycle / 100)
  f32 raw_value = ARR_VALUE * (duty_cycle / 100);
  timer_set_oc_value(TIM2, TIM_OC1, (u32)raw_value);
}

// referred to as 'advanced' bus as highly configurable (speed and transfer) and connects different peripherals 

static void
rcc_setup(void)
{
  // RCC takes source and feeds to PLL
  // PLL is feedback hardware device that can take one signal and output one/many signals of different frequencies and phases
  // output of one will go to CPU
  rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_3V3_84MHZ]);
}

// systick peripheral is wall clock
static void
systick_setup(void)
{
  // 84MHz gives 84cycles/ns. However, rarely operate at this granularity in code; would be in a peripheral
  // us common, ms often
  // 1000 ticks per second, so ms
  systick_set_frequency(1000, 84000000);
  systick_counter_enable();
  // lets us know via maskable interrupt
  systick_interrupt_enable();

  // an exception like TRAP is synchronous in nature
}

static void
gpio_setup(void)
{
  // in MCUs all peripherals off by default.
  // effectively 'turn them on' by enabling clock signal
  rcc_periph_clock_enable(RCC_GPIOA);

  // the ports (ORing pins) provide atomicity, e.g. can read/write to 16pins simultaneously
  gpio_mode_setup(LED_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, LED_PIN);
  gpio_set_af(LED_PORT, GPIO_AF1, LED_PIN);
}

// #define MMIO_32(addr) (*(volatile u32 *)addr)
// memory mapped address hierarchy typically 'bus + peripheral + register' 

// 4.2billion enough
// 4 to roughly account for the for-loop cmp and add
// delay_cycles(84000000 / 4);
static void
delay_cycles(u32 cycles)
{
  for (u32 i = 0; i < cycles; i += 1)
  {
    __asm__("nop");
  }
}


// IMPORTANT: web docs with hyperlinks are not present for embedded. finding information can be more involved

// IMPORTANT: recognise that the datasheet is written for diverse group of people with different requirements
// e.g, if firmware engineer working on dev-board, not concerned with hardware engineer building board CPU-die package size, electrical characteristics for power supply etc.
// more concerned with peripheral capabilities
// Reference manual is more targeted for firmware developers

// ARM is owns intellectual property
// cortex-m4 TRM useful to have so can know what is common across all chips, and what is specific to yours 
// also cortex-m4 programming manual


// counter-reg, auto-reload (freq.), compare-reg (duty.)
// pwm is a 'rectangle' wave (internally is a saw-tooth, from comparator turns to rectangle)
// duty cycle is how long is it high for
// duty cycle independent of frequency
// signal can interface with motors that extract freq. and duty. information
// also used to drive a MOSFETs (electronic switch to activate a circuit of higher voltage/load?)
// in this case, could have multiple MOSFETs, driving each at different pulses? motor H-bridge?
// PWM dead-timing to ensure circuit settles?
// PWM also to create music (may require amplifier for speaker). RC circuit used for say a low-pass filter that could perform waveform shaping, e.g. square to sine
// timers flexible (channels, peripheral control, interrupts, inputs, outputs, etc.)
//
// TODO: links for MOSFET?

int
main(void)
{
  rcc_setup();
  gpio_setup();
  systick_setup();
  timer_setup();

  while (1)
  {
    u64 start_time = get_ticks();
    f32 duty_cycle = 0.0f;

    timer_set_duty_cycle(duty_cycle);

    // blink led at 1Hz
    if (get_ticks() - start_time >= 10)
    {
      duty_cycle += 1.0f;
      if (duty_cycle > 100.0f) duty_cycle = 0.0f;
      timer_set_duty_cycle(duty_cycle);

      start_time = get_ticks();
    }
  }

  return 0;
}
