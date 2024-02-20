#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace acurite {

enum AcuRiteState {
  ACURITE_INIT,
  ACURITE_SYNC_ON,
  ACURITE_SYNC_OFF,
  ACURITE_BIT_ON,
  ACURITE_BIT_OFF,
  ACURITE_DONE
};

struct AcuRiteStore {
  static void gpio_intr(AcuRiteStore *arg);
  volatile AcuRiteState state;
  volatile uint32_t bits;
  volatile uint32_t syncs;
  volatile uint32_t prev;
  volatile uint8_t data[7];
  ISRInternalGPIOPin pin;
};

class AcuRite : public Component,
                public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, 
                                      spi::CLOCK_POLARITY_LOW, 
                                      spi::CLOCK_PHASE_LEADING,
                                      spi::DATA_RATE_8MHZ> { 
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void loop() override;

  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
  void set_humidity_sensor(sensor::Sensor *humidity_sensor) { humidity_sensor_ = humidity_sensor; }
  void set_rst_pin(InternalGPIOPin *rst) { this->rst_ = rst; }
  void set_nss_pin(InternalGPIOPin *nss) { this->nss_ = nss; }
  void set_dio2_pin(InternalGPIOPin *dio2) { this->dio2_ = dio2; }

 protected:
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *humidity_sensor_{nullptr};
  InternalGPIOPin *rst_{nullptr};
  InternalGPIOPin *nss_{nullptr};
  InternalGPIOPin *dio2_{nullptr};

 private:
  static void interrupt(uint32_t *arg);
  void setBitrate(uint32_t duration);
  void setFrequency(uint64_t frequency);
  uint8_t readRegister(uint8_t address);
  void writeRegister(uint8_t address, uint8_t value);
  uint8_t singleTransfer(uint8_t address, uint8_t value);
  AcuRiteStore store;
};

}  // namespace acurite
}  // namespace esphome