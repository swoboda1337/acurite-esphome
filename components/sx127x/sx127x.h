#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace sx127x {

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
  void sx127x_start();
  void rx_stop();
  void rx_start(uint64_t frequency);
  void setBitrate_(uint32_t duration);
  void setFrequency_(uint64_t frequency);
  void writeRegister_(uint8_t address, uint8_t value);
  uint8_t singleTransfer_(uint8_t address, uint8_t value);
  uint8_t readRegister_(uint8_t address);

  InternalGPIOPin *rst_{nullptr};
  InternalGPIOPin *nss_{nullptr};
  InternalGPIOPin *dio0_{nullptr};
  InternalGPIOPin *dio1_{nullptr};
  InternalGPIOPin *dio2_{nullptr};
};

}  // namespace sx127x
}  // namespace esphome