#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/sx127x/sx127x.h"

namespace esphome {
namespace acurite {

struct OokStore {
  static void gpio_intr(OokStore *arg);
  volatile uint32_t *buffer{nullptr};
  volatile uint32_t write{0};
  volatile uint32_t read{0};
  volatile uint32_t prev{0};
  volatile bool filtered{false};
  volatile bool overflow{false};
  uint32_t size{8192};
  uint8_t filter{50};
  ISRInternalGPIOPin pin;
};

class AcuRite : public Component { 
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override;

  void add_temperature_sensor(sensor::Sensor *temperature_sensor, uint16_t id) { temperature_sensors_[id] = temperature_sensor; }
  void add_humidity_sensor(sensor::Sensor *humidity_sensor, uint16_t id) { humidity_sensors_[id] = humidity_sensor; }
  void add_rain_sensor(sensor::Sensor *rain_sensor, uint16_t id) { rain_sensors_[id] = rain_sensor; }
  void set_pin(InternalGPIOPin *pin) { this->pin_ = pin; }
  void zero_rain_totals();

 protected:
  std::map<uint16_t, sensor::Sensor*> temperature_sensors_;
  std::map<uint16_t, sensor::Sensor*> humidity_sensors_;
  std::map<uint16_t, sensor::Sensor*> rain_sensors_;
  std::map<uint16_t, uint32_t> rain_counters_;
  InternalGPIOPin *pin_{nullptr};
  bool decode_6002rm_(uint8_t *data, uint8_t len);
  bool decode_899_(uint8_t *data, uint8_t len);
  OokStore store_;
};

}  // namespace acurite
}  // namespace esphome