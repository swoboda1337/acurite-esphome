#pragma once

#include <map>

#include "esphome/core/gpio.h"
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"

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

class AcuRiteDevice {
 public:
  void add_temperature_sensor(sensor::Sensor *sensor) { temperature_sensor_ = sensor; }
  void add_humidity_sensor(sensor::Sensor *sensor) { humidity_sensor_ = sensor; }
  void add_rainfall_sensor_1hr(sensor::Sensor *sensor) { rainfall_sensor_1hr_ = sensor; }
  void add_rainfall_sensor_24hr(sensor::Sensor *sensor) { rainfall_sensor_24hr_ = sensor; }
  void add_rainfall_sensor_daily(sensor::Sensor *sensor) { rainfall_sensor_daily_ = sensor; }
  void temperature_value(float value);
  void humidity_value(float value);
  void rainfall_count(uint32_t count, ESPTime now);
  void reset_daily();
  void mark_unknown();

 protected:
  sensor::Sensor *temperature_sensor_{nullptr};
  float temperature_published_{1000};
  float temperature_last_{1000};

  sensor::Sensor *humidity_sensor_{nullptr};
  float humidity_published_{1000};
  float humidity_last_{1000};

  sensor::Sensor *rainfall_sensor_daily_{nullptr};
  sensor::Sensor *rainfall_sensor_24hr_{nullptr};
  sensor::Sensor *rainfall_sensor_1hr_{nullptr};
  uint32_t rainfall_published_daily_{0xFFFFFFFF};
  uint32_t rainfall_published_24hr_{0xFFFFFFFF};
  uint32_t rainfall_published_1hr_{0xFFFFFFFF};
  uint32_t rainfall_count_device_{0xFFFFFFFF};
  uint32_t rainfall_count_last_{0xFFFFFFFF};
  uint32_t rainfall_count_daily_{0};
  uint16_t rainfall_count_buffer_[24 * 60]{0};
  ESPTime rainfall_count_time_{0};
};

class AcuRite : public Component { 
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override;

  void add_device(uint16_t id) { devices_[id] = new AcuRiteDevice(); }
  void add_temperature_sensor(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_temperature_sensor(sensor); }
  void add_humidity_sensor(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_humidity_sensor(sensor); }
  void add_rainfall_sensor_1hr(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_rainfall_sensor_1hr(sensor); }
  void add_rainfall_sensor_24hr(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_rainfall_sensor_24hr(sensor); }
  void add_rainfall_sensor_daily(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_rainfall_sensor_daily(sensor); }
  void set_srctime(time::RealTimeClock *srctime) { this->srctime_ = srctime; }
  void set_pin(InternalGPIOPin *pin) { this->pin_ = pin; }

 protected:
  bool decode_6002rm_(uint8_t *data, uint8_t len);
  bool decode_899_(uint8_t *data, uint8_t len);
  bool midnight_{false};
  time::RealTimeClock *srctime_{nullptr};
  std::map<uint16_t, AcuRiteDevice*> devices_;
  InternalGPIOPin *pin_{nullptr};
  OokStore store_;
  uint32_t bits_{0};
  uint32_t syncs_{0};
  uint32_t prev_{0};
  uint8_t data_[8];
};

}  // namespace acurite
}  // namespace esphome