/// Shared SPI0 bus initialization for Teensy 3.2.
///
/// Both the DAC8565 (CS=pin10) and SH1106 OLED (CS=pin8, GPIO) share SPI0.
/// This must be called ONCE before either device is initialized.
///
/// Adapted directly from ArticCircle/OC_DAC.cpp::SPI_init().
/// Sets MOSI (pin11) and SCK (pin13) with drive strength enable (DSE),
/// configures CTAR0 (8-bit) and CTAR1 (16-bit) at the correct bus rate.

#pragma once

#include <Arduino.h>
#include <kinetis.h>

namespace oc::platform {

// F_BUS = 36 MHz for Teensy 3.2 at 72 MHz (F_CPU/2).
// Rate = (36/2) * ((1+1)/2) = 18 MHz — within DAC8565 (30MHz) and SH1106 spec.
static constexpr uint32_t kSpiClock =
    SPI_CTAR_PBR(0) | SPI_CTAR_BR(0) | SPI_CTAR_DBR;

inline void spi0_init() {
    // Enable SPI0 peripheral clock
    SIM_SCGC6 |= SIM_SCGC6_SPI0;

    // Configure MOSI and SCK with drive strength enable for fast signal edges
    CORE_PIN11_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2);  // MOSI = PTD6, SPI0_SOUT
    CORE_PIN13_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2);  // SCK  = PTD1, SPI0_SCK

    // CTAR0 = 8-bit frames at kSpiClock rate
    const uint32_t ctar0 = kSpiClock | SPI_CTAR_FMSZ(7);
    // CTAR1 = 16-bit frames at same rate (used by OLED SPI_send 16-bit path)
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

} // namespace oc::platform
