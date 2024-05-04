#pragma once

#include <map>
#include "esphome/core/gpio.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/remote_receiver/remote_receiver.h"

namespace esphome {
namespace acurite {

class AcuRiteDevice {
 public:
  AcuRiteDevice(uint16_t id) : id_(id) { }
  void add_temperature_sensor(sensor::Sensor *sensor) { temperature_sensor_ = sensor; }
  void add_humidity_sensor(sensor::Sensor *sensor) { humidity_sensor_ = sensor; }
  void add_rainfall_sensor(sensor::Sensor *sensor) { rainfall_sensor_ = sensor; }
  void temperature_value(float value);
  void humidity_value(float value);
  void rainfall_count(uint32_t count);
  void setup();

 protected:
  struct RainfallState {
    uint32_t total{0};
    uint32_t device{0};
  };
  ESPPreferenceObject preferences_;
  RainfallState rainfall_state_;
  sensor::Sensor *rainfall_sensor_{nullptr};
  sensor::Sensor *humidity_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
  uint32_t rainfall_last_{0xFFFFFFFF};
  float humidity_last_{1000};
  float temperature_last_{1000};
  uint16_t id_{0};
};

class AcuRite : public Component, public remote_base::RemoteReceiverListener { 
 public:
  float get_setup_priority() const override { return setup_priority::LATE; }
  void setup() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;
  void add_device(uint16_t id) { this->devices_[id] = new AcuRiteDevice(id); }
  void add_temperature_sensor(uint16_t id, sensor::Sensor *sensor) { this->devices_[id]->add_temperature_sensor(sensor); }
  void add_humidity_sensor(uint16_t id, sensor::Sensor *sensor) { this->devices_[id]->add_humidity_sensor(sensor); }
  void add_rainfall_sensor(uint16_t id, sensor::Sensor *sensor) { this->devices_[id]->add_rainfall_sensor(sensor); }
  void set_srcrecv(remote_receiver::RemoteReceiverComponent *srcrecv) { this->remote_receiver_ = srcrecv; }

 protected:
  remote_receiver::RemoteReceiverComponent *remote_receiver_{nullptr};
  bool decode_6002rm_(uint8_t *data, uint8_t len);
  bool decode_899_(uint8_t *data, uint8_t len);
  bool validate_(uint8_t *data, uint8_t len);
  std::map<uint16_t, AcuRiteDevice*> devices_;
};

}  // namespace acurite
}  // namespace esphome