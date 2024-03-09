#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
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

class TemperatureSensor {
 public:
  TemperatureSensor(sensor::Sensor *sensor): sensor_(sensor) { }
  void new_value(float value);
  void mark_unknown();

 protected:
  sensor::Sensor *sensor_;
  float value_published_{1000};
  float value_last_{1000};
};

class HumiditySensor {
 public:
  HumiditySensor(sensor::Sensor *sensor): sensor_(sensor) { }
  void new_value(float value);
  void mark_unknown();

 protected:
  sensor::Sensor *sensor_;
  float value_published_{1000};
  float value_last_{1000};
};

class RainSensor {
public:
  RainSensor(sensor::Sensor *sensor): sensor_(sensor) { }
  void new_value(uint32_t count);
  void mark_unknown();
  void reset_period();

 protected:
  sensor::Sensor *sensor_;
  uint32_t count_device_{0xFFFFFFFF};
  uint32_t count_published_{0xFFFFFFFF};
  uint32_t count_last_{0xFFFFFFFF};
  uint32_t count_period_{0};
};

class AcuRite : public Component { 
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override;

  void add_temperature_sensor(sensor::Sensor *sensor, uint16_t id) { temperature_sensors_[id] = new TemperatureSensor(sensor); }
  void add_humidity_sensor(sensor::Sensor *sensor, uint16_t id) { humidity_sensors_[id] = new HumiditySensor(sensor); }
  void add_rain_sensor(sensor::Sensor *sensor, uint16_t id) { rain_sensors_[id] = new RainSensor(sensor); }
  void set_pin(InternalGPIOPin *pin) { this->pin_ = pin; }
  void reset_rainfall();

 protected:
  bool decode_6002rm_(uint8_t *data, uint8_t len);
  bool decode_899_(uint8_t *data, uint8_t len);
  std::map<uint16_t, TemperatureSensor*> temperature_sensors_;
  std::map<uint16_t, HumiditySensor*> humidity_sensors_;
  std::map<uint16_t, RainSensor*> rain_sensors_;
  InternalGPIOPin *pin_{nullptr};
  OokStore store_;
  uint32_t bits_{0};
  uint32_t syncs_{0};
  uint32_t prev_{0};
  uint8_t data_[8];
};

template<typename... Ts> class AcuRiteResetRainfallAction : public Action<Ts...> {
 public:
  AcuRiteResetRainfallAction(AcuRite *acurite) : acurite_(acurite) {}

  void play(Ts... x) override { this->acurite_->reset_rainfall(); }

 protected:
  AcuRite *acurite_;
};

}  // namespace acurite
}  // namespace esphome