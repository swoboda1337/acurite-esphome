#pragma once

#include <map>

#include "esphome/core/gpio.h"
#include "esphome/core/component.h"
#include "esphome/components/remote_receiver/remote_receiver.h"

#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

namespace esphome {
namespace acurite {

#ifdef USE_SENSOR
class AcuRiteDevice {
 public:
  AcuRiteDevice(uint16_t id) : id_(id) { }
  void setup();
  void add_temperature_sensor(sensor::Sensor *sensor) { temperature_sensor_ = sensor; }
  void add_humidity_sensor(sensor::Sensor *sensor) { humidity_sensor_ = sensor; }
  void add_rainfall_sensor(sensor::Sensor *sensor) { rainfall_sensor_ = sensor; }
  void temperature_value(float value);
  void humidity_value(float value);
  void rainfall_count(uint32_t count);

 protected:
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *humidity_sensor_{nullptr};
  sensor::Sensor *rainfall_sensor_{nullptr};
  float temperature_last_{1000};
  float humidity_last_{1000};
  uint32_t rainfall_last_{0xFFFFFFFF};
  uint16_t id_;
};
#endif

class AcuRite : public Component, public remote_base::RemoteReceiverListener { 
 public:
  float get_setup_priority() const override;
  void setup() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;
  void set_srcrecv(remote_receiver::RemoteReceiverComponent *srcrecv) { this->remote_receiver_ = srcrecv; }
#ifdef USE_SENSOR
  void add_device(uint16_t id) { devices_[id] = new AcuRiteDevice(id); }
  void add_temperature_sensor(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_temperature_sensor(sensor); }
  void add_humidity_sensor(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_humidity_sensor(sensor); }
  void add_rainfall_sensor(uint16_t id, sensor::Sensor *sensor) { devices_[id]->add_rainfall_sensor(sensor); }
#endif

 protected:
  remote_receiver::RemoteReceiverComponent *remote_receiver_{nullptr};
  bool decode_6002rm_(uint8_t *data, uint8_t len);
  bool decode_899_(uint8_t *data, uint8_t len);
  bool validate_(uint8_t *data, uint8_t len);
  bool midnight_{false};
#ifdef USE_SENSOR
  std::map<uint16_t, AcuRiteDevice*> devices_;
#endif
};

}  // namespace acurite
}  // namespace esphome