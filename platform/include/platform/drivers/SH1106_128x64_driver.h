#pragma once
#include <stdint.h>
#include <string.h>

#define OLED_CS_ACTIVE   LOW
#define OLED_CS_INACTIVE HIGH

struct SH1106_128x64_Driver {
  static constexpr size_t kFrameSize = 128 * 64 / 8;
  static constexpr size_t kNumPages  = 8;
  static constexpr size_t kPageSize  = kFrameSize / kNumPages;
  static constexpr uint8_t kDefaultOffset = 2;

  static void Init(uint8_t dc, uint8_t rst, uint8_t cs);
  static void Clear();
  static void Flush();
  static void SendPage(uint_fast8_t index, const uint8_t *data);
  static void SPI_send(void *bufr, size_t n);
  static void AdjustOffset(uint8_t offset);

  // Pin state stored at Init() time.
  static uint8_t kDC;
  static uint8_t kRST;
  static uint8_t kCS;
};
