/// Shared SPI0 bus initialization for Teensy 3.2.
///
/// MOSI is always on pin 11 (PTD6). SCK differs between modules:
///   OC  → pin 13 (PTC5, SPI0_SCK alt2)  — OC PCB trace
///   T_U → pin 14 (PTD1, SPI0_SCK alt2)  — T_U PCB trace; pin 13 = encoder button
///
/// Traits must provide:
///   static volatile uint32_t& sck_config()  — returns the CORE_PINx_CONFIG register ref

#pragma once

#include <Arduino.h>
#include <kinetis.h>

namespace platform {

static constexpr uint32_t kSpiClock =
    SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) | SPI_CTAR_DBR;

template <typename SpiTraits>
inline void spi0_init() {
    SIM_SCGC6 |= SIM_SCGC6_SPI0;

    CORE_PIN11_CONFIG       = PORT_PCR_DSE | PORT_PCR_MUX(2);  // MOSI = PTD6
    SpiTraits::sck_config() = PORT_PCR_DSE | PORT_PCR_MUX(2);  // SCK  = module-specific

    const uint32_t ctar0 = kSpiClock | SPI_CTAR_FMSZ(7);
    const uint32_t ctar1 = kSpiClock | SPI_CTAR_FMSZ(15);

    SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_PCSIS(0x1F);
    SPI0_MCR |= SPI_MCR_CLR_RXF | SPI_MCR_CLR_TXF;

    uint32_t mcr = SPI0_MCR;
    if (mcr & SPI_MCR_MDIS) {
        SPI0_CTAR0 = ctar0;
        SPI0_CTAR1 = ctar1;
    } else {
        SPI0_MCR = mcr | SPI_MCR_MDIS | SPI_MCR_HALT;
        SPI0_CTAR0 = ctar0;
        SPI0_CTAR1 = ctar1;
        SPI0_MCR = mcr;
    }
}

} // namespace platform
