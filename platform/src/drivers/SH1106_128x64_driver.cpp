/*
 * Low-level SH1106 OLED driver for oc-core / Teensy 3.2.
 * Adapted from ArticCircle/src/drivers/SH1106_128x64_driver.cpp
 * Original author: Patrick Dowling (pld@gurkenkiste.com)
 * License: GPLv3 with additional permissions (see original file).
 *
 * Changes from original:
 *   - Removed dependency on OC_gpio.h and OC_options.h
 *   - Pin defines moved to SH1106_128x64_driver.h
 *   - FLIP_180 / INVERT_DISPLAY not supported (not needed for oc-core)
 */

#include <Arduino.h>
#include <DMAChannel.h>
#include "platform/drivers/SH1106_128x64_driver.h"

#ifndef SPI_SR_RXCTR
#define SPI_SR_RXCTR 0xF0
#endif

// Static pin storage — set by Init().
uint8_t SH1106_128x64_Driver::kDC  = 0;
uint8_t SH1106_128x64_Driver::kRST = 0;
uint8_t SH1106_128x64_Driver::kCS  = 0;

#define DMA_PAGE_TRANSFER
static DMAChannel page_dma;

static uint8_t SH1106_data_start_seq[] = {
  0x10,   // set upper 4 bits of col addr to 0
  0x02,   // set lower 4 bits of col addr to 0 (+offset)
  0x00    // 0xb0 | page — filled in by SendPage/Clear
};

static uint8_t SH1106_init_seq[] = {
  0xae,         // display off
  0xd5, 0x80,   // clock divide ratio / oscillator frequency
  0xa8, 0x3f,   // multiplex ratio (1/64)
  0xd3, 0x00,   // display offset: 0
  0x40,         // start line: 0
  0x8d, 0x14,   // charge pump: enable
  0x20, 0x00,   // page addressing mode
  0xa1,         // segment remap (normal)
  0xc8,         // COM scan direction: reverse
  0xda, 0x12,   // COM pin HW config
  0x81, 0xcf,   // contrast
  0xd9, 0xf1,   // pre-charge period
  0xdb, 0x40,   // VCOMH deselect level
  0x2e,         // deactivate scroll
  0xa4,         // output RAM to display
  0xa6,         // normal (non-inverted) display
};

static uint8_t SH1106_display_on_seq[] = { 0xaf };

/*static*/
void SH1106_128x64_Driver::Init(uint8_t dc, uint8_t rst, uint8_t cs) {
  kDC  = dc;
  kRST = rst;
  kCS  = cs;

  pinMode(kCS,  OUTPUT);
  pinMode(kRST, OUTPUT);
  pinMode(kDC,  OUTPUT);

  digitalWriteFast(kRST, HIGH);  delay(1);
  digitalWriteFast(kRST, LOW);   delay(10);
  digitalWriteFast(kRST, HIGH);  delay(20);

  digitalWriteFast(kCS, OLED_CS_INACTIVE);
  digitalWriteFast(kDC, LOW);

  digitalWriteFast(kRST, LOW);  delay(20);
  digitalWriteFast(kRST, HIGH); delay(20);

  digitalWriteFast(kCS, OLED_CS_ACTIVE);
  SPI_send(SH1106_init_seq, sizeof(SH1106_init_seq));
  digitalWriteFast(kCS, OLED_CS_INACTIVE);

  // SH1106 has 132 physical columns; the visible 128 start at column 2 by default.
  // Some OLEDs are wired differently and need offset 0.
  // SH1106 has 132 physical columns; visible 128 start at column 2.
  AdjustOffset(2);

#ifdef DMA_PAGE_TRANSFER
  page_dma.destination((volatile uint8_t&)SPI0_PUSHR);
  page_dma.transferSize(1);
  page_dma.transferCount(kPageSize);
  page_dma.disableOnCompletion();
  page_dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_TX);
  page_dma.disable();
#endif

  Clear();
}

/*static*/
void SH1106_128x64_Driver::Flush() {
#ifdef DMA_PAGE_TRANSFER
  // Always unconditionally finalize: deassert CS, stop DMA, clear SPI flags.
  // This matches ArticCircle's original pattern. Called at the start of every
  // ISR to cleanly end the previous page's DMA before DAC or new DMA runs.
  digitalWriteFast(kCS, OLED_CS_INACTIVE);
  page_dma.disable();
  SPI0_RSER = 0;
  // Halt SPI0 and clear both FIFOs before returning control to the ISR.
  // Without this, bytes queued by the DMA but not yet shifted out remain in
  // the TX FIFO and intermix with the subsequent DAC SPIFIFO writes, producing
  // garbled OLED output.  The halt takes effect after the current frame
  // completes (a few cycles at 18 MHz); CLR_TXF/CLR_RXF are self-clearing.
  SPI0_MCR |= SPI_MCR_HALT;
  SPI0_MCR |= SPI_MCR_CLR_TXF | SPI_MCR_CLR_RXF;
  SPI0_MCR &= ~SPI_MCR_HALT;
  SPI0_SR   = 0xFF0F0000;
#endif
}

static uint8_t empty_page[SH1106_128x64_Driver::kPageSize];

/*static*/
void SH1106_128x64_Driver::Clear() {
  memset(empty_page, 0, kPageSize);

  // SH1106 page-addressing mode does NOT auto-advance pages after the last
  // column, so we must re-send the page-address command for every page.
  // Previously only page 0 was cleared; pages 1-7 kept GDDRAM power-on data.
  digitalWriteFast(kDC, HIGH);  // data mode for all page sends
  for (size_t p = 0; p < kNumPages; ++p) {
    SH1106_data_start_seq[2] = 0xb0 | p;

    digitalWriteFast(kDC, LOW);   // command mode: send column + page addr
    digitalWriteFast(kCS, OLED_CS_ACTIVE);
    SPI_send(SH1106_data_start_seq, sizeof(SH1106_data_start_seq));

    digitalWriteFast(kDC, HIGH);  // data mode: 128 zero bytes
    SPI_send(empty_page, kPageSize);
    digitalWriteFast(kCS, OLED_CS_INACTIVE);
  }

  digitalWriteFast(kDC, LOW);
  digitalWriteFast(kCS, OLED_CS_ACTIVE);
  SPI_send(SH1106_display_on_seq, sizeof(SH1106_display_on_seq));
  digitalWriteFast(kCS, OLED_CS_INACTIVE);
  digitalWriteFast(kDC, HIGH);
}

/*static*/
void SH1106_128x64_Driver::SendPage(uint_fast8_t index, const uint8_t *data) {
  SH1106_data_start_seq[2] = 0xb0 | index;

  digitalWriteFast(kDC, LOW);
  digitalWriteFast(kCS, OLED_CS_ACTIVE);
  SPI_send(SH1106_data_start_seq, sizeof(SH1106_data_start_seq));
  digitalWriteFast(kDC, HIGH);

#ifdef DMA_PAGE_TRANSFER
  SPI0_SR   = 0xFF0F0000;
  SPI0_RSER = SPI_RSER_RFDF_RE | SPI_RSER_RFDF_DIRS |
              SPI_RSER_TFFF_RE | SPI_RSER_TFFF_DIRS;
  page_dma.sourceBuffer(data, kPageSize);
  page_dma.enable();
#else
  SPI_send(const_cast<uint8_t*>(data), kPageSize);
  digitalWriteFast(kCS, OLED_CS_INACTIVE);
#endif
}

/*static*/
void SH1106_128x64_Driver::SPI_send(void *bufr, size_t n) {
  int nf;
  uint8_t *buf = (uint8_t *)bufr;

  if (n & 1) {
    uint8_t b = *buf++;
    SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_CLR_RXF | SPI_MCR_CLR_TXF | SPI_MCR_PCSIS(0x1F);
    SPI0_SR  = SPI_SR_TCF;
    SPI0_PUSHR = SPI_PUSHR_CONT | b;
    while (!(SPI0_SR & SPI_SR_TCF));
    n--;
  }
  SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_CLR_RXF | SPI_MCR_CLR_TXF | SPI_MCR_PCSIS(0x1F);
  nf = n / 2 < 3 ? n / 2 : 3;
  uint8_t *limit = buf + n;
  for (int i = 0; i < nf; i++) {
    uint16_t w = (*buf++) << 8;
    w |= *buf++;
    SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | w;
  }
  while (buf < limit) {
    uint16_t w = (*buf++) << 8;
    w |= *buf++;
    while (!(SPI0_SR & SPI_SR_RXCTR));
    SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | w;
    SPI0_POPR;
  }
  while (nf) {
    while (!(SPI0_SR & SPI_SR_RXCTR));
    SPI0_POPR;
    nf--;
  }
}

/*static*/
void SH1106_128x64_Driver::AdjustOffset(uint8_t offset) {
  SH1106_data_start_seq[1] = offset;
}
