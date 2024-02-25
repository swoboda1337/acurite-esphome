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

class AcuRite : public Component { 
 public:
  void setup() override;
  void loop() override;

  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void add_temperature_sensor(sensor::Sensor *temperature_sensor, uint16_t id) { temperature_sensors_[id] = temperature_sensor; }
  void add_humidity_sensor(sensor::Sensor *humidity_sensor, uint16_t id) { humidity_sensors_[id] = humidity_sensor; }
  void add_rain_sensor(sensor::Sensor *rain_sensor, uint16_t id) { rain_sensors_[id] = rain_sensor; }
  void set_pin(InternalGPIOPin *pin) { this->pin_ = pin; }

 protected:
  std::map<uint16_t, sensor::Sensor*> temperature_sensors_;
  std::map<uint16_t, sensor::Sensor*> humidity_sensors_;
  std::map<uint16_t, sensor::Sensor*> rain_sensors_;
  InternalGPIOPin *pin_{nullptr};
  AcuRiteStore store;
};

}  // namespace acurite
}  // namespace esphome