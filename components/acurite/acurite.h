#pragma once

#include <map>

#include "esphome/core/gpio.h"
#include "esphome/core/component.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/remote_receiver/remote_receiver.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

namespace esphome {
namespace acurite {

#ifdef USE_SENSOR
class AcuRiteDevice {
 public:
  void add_temperature_sensor(sensor::Sensor *sensor) { temperature_sensor_ = sensor; }
  void add_humidity_sensor(sensor::Sensor *sensor) { humidity_sensor_ = sensor; }
  void add_rainfall_sensor_1hr(sensor::Sensor *sensor) { rainfall_sensor_1hr_ = sensor; }
  void add_rainfall_sensor_24hr(sensor::Sensor *sensor) { rainfall_sensor_24hr_ = sensor; }
  void add_rainfall_sensor_daily(sensor::Sensor *sensor) { rainfall_sensor_daily_ = sensor; }
  void temperature_value(float value);
  void humidity_value(float value);
  bool rainfall_count(uint32_t count, ESPTime now);
  void reset_daily();

 protected:
  sensor::Sensor *temperature_sensor_{nullptr};
  float temperature_last_{1000};

  sensor::Sensor *humidity_sensor_{nullptr};
  float humidity_last_{1000};

  sensor::Sensor *rainfall_sensor_daily_{nullptr};
  sensor::Sensor *rainfall_sensor_24hr_{nullptr};
  sensor::Sensor *rainfall_sensor_1hr_{nullptr};
  uint32_t rainfall_count_device_{0xFFFFFFFF};
  uint32_t rainfall_count_last_{0xFFFFFFFF};
  uint32_t rainfall_count_daily_{0};
  uint16_t rainfall_count_buffer_[24 * 60]{0};
  ESPTime rainfall_count_time_{0};
};
#endif

class AcuRite : public Component, public remote_base::RemoteReceiverListener { 
 public:
  bool on_receive(remote_base::RemoteReceiveData data) override;
  void setup() override;
  void loop() override;
  float get_setup_priority() const override;

#ifdef USE_SENSOR
  void add_device(uint16_t id) { devices_[id] = new AcuRiteDevice(); }
  void add_temperature_sensor(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_temperature_sensor(sensor); }
  void add_humidity_sensor(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_humidity_sensor(sensor); }
  void add_rainfall_sensor_1hr(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_rainfall_sensor_1hr(sensor); }
  void add_rainfall_sensor_24hr(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_rainfall_sensor_24hr(sensor); }
  void add_rainfall_sensor_daily(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_rainfall_sensor_daily(sensor); }
#endif
#ifdef USE_BINARY_SENSOR
  void set_rainfall_sensor(binary_sensor::BinarySensor *sensor) { rainfall_sensor_ = sensor; }
#endif
  void set_srctime(time::RealTimeClock *srctime) { this->srctime_ = srctime; }
  void set_srcrecv(remote_receiver::RemoteReceiverComponent *srcrecv) { this->remote_receiver_ = srcrecv; }

 protected:
  bool decode_6002rm_(uint8_t *data, uint8_t len);
  bool decode_899_(uint8_t *data, uint8_t len);
  bool midnight_{false};
#ifdef USE_SENSOR
  std::map<uint16_t, AcuRiteDevice*> devices_;
#endif
#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *rainfall_sensor_{nullptr};
#endif
  time::RealTimeClock *srctime_{nullptr};
  remote_receiver::RemoteReceiverComponent *remote_receiver_{nullptr};
};

}  // namespace acurite
}  // namespace esphome