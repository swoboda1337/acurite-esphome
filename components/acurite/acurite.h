#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/sx127x/sx127x.h"

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
  volatile uint8_t data[8];
  ISRInternalGPIOPin pin;
};

class AcuRite : public sx127x::SX127X { 
 public:
  void setup() override;
  void loop() override;

  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
  void set_humidity_sensor(sensor::Sensor *humidity_sensor) { humidity_sensor_ = humidity_sensor; }
  void set_rain_sensor(sensor::Sensor *rain_sensor) { rain_sensor_ = rain_sensor; }

 protected:
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *humidity_sensor_{nullptr};
  sensor::Sensor *rain_sensor_{nullptr};
  AcuRiteStore store;
};

}  // namespace acurite
}  // namespace esphome