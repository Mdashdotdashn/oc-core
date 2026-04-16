#pragma once
#include <stdint.h>
#include <string.h>

// OLED pin definitions (from OC_gpio.h)
#define OLED_DC   6
#define OLED_RST  7
#define OLED_CS   8
#define OLED_CS_ACTIVE   LOW
#define OLED_CS_INACTIVE HIGH

struct SH1106_128x64_Driver {
  static constexpr size_t kFrameSize = 128 * 64 / 8;
  static constexpr size_t kNumPages  = 8;
  static constexpr size_t kPageSize  = kFrameSize / kNumPages;
  static constexpr uint8_t kDefaultOffset = 2;

  static void Init();
  static void Clear();
  static void Flush();
  static void SendPage(uint_fast8_t index, const uint8_t *data);
  static void SPI_send(void *bufr, size_t n);
  static void AdjustOffset(uint8_t offset);
};
