// SPDX-License-Identifier: zlib-acknowledgement

// bootloader term is overloaded; typically first bit of code that is run
// for firmware, will verify and update code
// for all intents and purposes, our bootloader is first (however, typically first is manufacturer bootloader in ROM) 

// TODO: pad application in python: open("app.bin", "wb").write(bytes([0xff for _ in range(num_bytes)]))
// IMPORTANT: comparing little-endian reset vector address in .map and .bin with okteta, will see .bin is not 4byte-aligned (not ending in 0,4,8,c) to indicate thumb 

#define BOOTLOADER_SIZE KB(32)
#define APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE)

// IMPORTANT(Ryan): looking at binary size and map file of bootloader to verify it 'makes sense' 
void jump_to_main(void)
{
  u32 *reset_vector_addr = (u32 *)((u8 *)APP_START_ADDRESS + 4);
  u32 *reset_vector = (u32 *)(*reset_vector_addr);
  
  typedef void (*void_fn)(void);
  void_fn jump_fn = (void_fn)reset_vector;

  jump_fn();
}

int bootloader(void)
{
  // stack pointer will be cleared in application
  jump_to_main();

  return 0;
}
