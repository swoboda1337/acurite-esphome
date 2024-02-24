#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace sx127x {

enum SX127XRxBw : uint8_t {
  RX_BANDWIDTH_2_6 = 0x17,
  RX_BANDWIDTH_3_1 = 0x0F,
  RX_BANDWIDTH_3_9 = 0x07,
  RX_BANDWIDTH_5_2 = 0x16,
  RX_BANDWIDTH_6_3 = 0x0E,
  RX_BANDWIDTH_7_8 = 0x06,
  RX_BANDWIDTH_10_4 = 0x15,
  RX_BANDWIDTH_12_5 = 0x0D,
  RX_BANDWIDTH_15_6 = 0x05,
  RX_BANDWIDTH_20_8 = 0x14,
  RX_BANDWIDTH_25_0 = 0x0C,
  RX_BANDWIDTH_31_3 = 0x04,
  RX_BANDWIDTH_41_7 = 0x13,
  RX_BANDWIDTH_50_0 = 0x0B,
  RX_BANDWIDTH_62_5 = 0x03,
  RX_BANDWIDTH_83_3 = 0x12,
  RX_BANDWIDTH_100_0 = 0x0A,
  RX_BANDWIDTH_125_0 = 0x02,
  RX_BANDWIDTH_166_7 = 0x11,
  RX_BANDWIDTH_200_0 = 0x09,
  RX_BANDWIDTH_250_0 = 0x01
};

enum SX127XMod : uint8_t {
  MODULATION_FSK = 0x00,
  MODULATION_OOK = 0x20
};

class SX127X : public Component,
               public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, 
                                     spi::CLOCK_POLARITY_LOW, 
                                     spi::CLOCK_PHASE_LEADING,
                                     spi::DATA_RATE_8MHZ> { 
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void set_rst_pin(InternalGPIOPin *rst) { this->rst_ = rst; }
  void set_nss_pin(InternalGPIOPin *nss) { this->nss_ = nss; }
  void set_frequency(uint32_t frequency) { this->frequency_ = frequency; }
  void set_rx_modulation(SX127XMod modulation) { this->modulation_ = modulation; }
  void set_rx_bandwidth(SX127XRxBw bandwidth) { this->bandwidth_ = bandwidth; }

 protected:
  void write_register_(uint8_t address, uint8_t value);
  uint8_t single_transfer_(uint8_t address, uint8_t value);
  uint8_t read_register_(uint8_t address);
  InternalGPIOPin *rst_{nullptr};
  InternalGPIOPin *nss_{nullptr};
  SX127XRxBw bandwidth_; 
  SX127XMod modulation_;
  uint32_t frequency_;
};

}  // namespace sx127x
}  // namespace esphome