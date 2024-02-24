#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
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

enum SX127XRxMod : uint8_t {
  RX_MODULATION_FSK = 0x00,
  RX_MODULATION_OOK = 0x20
};

class SX127X : public Component,
               public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, 
                                     spi::CLOCK_POLARITY_LOW, 
                                     spi::CLOCK_PHASE_LEADING,
                                     spi::DATA_RATE_8MHZ> { 
 public:
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_rst_pin(InternalGPIOPin *rst) { this->rst_ = rst; }
  void set_nss_pin(InternalGPIOPin *nss) { this->nss_ = nss; }
  void set_dio0_pin(InternalGPIOPin *dio0) { this->dio0_ = dio0; }
  void set_dio1_pin(InternalGPIOPin *dio1) { this->dio1_ = dio1; }
  void set_dio2_pin(InternalGPIOPin *dio2) { this->dio2_ = dio2; }

 protected:
  void sx127x_setup_();
  void rx_stop_();
  void rx_start_(uint64_t frequency, SX127XRxBw bandwidth, SX127XRxMod modulation);
  void set_frequency_(uint64_t frequency);
  void write_register_(uint8_t address, uint8_t value);
  uint8_t single_transfer_(uint8_t address, uint8_t value);
  uint8_t read_register_(uint8_t address);
  SX127XRxMod rx_mod_;
  InternalGPIOPin *rst_{nullptr};
  InternalGPIOPin *nss_{nullptr};
  InternalGPIOPin *dio0_{nullptr};
  InternalGPIOPin *dio1_{nullptr};
  InternalGPIOPin *dio2_{nullptr};
};

}  // namespace sx127x
}  // namespace esphome