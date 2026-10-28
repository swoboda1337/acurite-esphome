#include "esphome/core/log.h"
#include "acurite_binary_sensor.h"

namespace esphome {
namespace acurite {

static const char *const TAG = "acurite";

void AcuRiteBinarySensor::update_battery(uint8_t value) {
  if (this->battery_level_binary_sensor_) {
    this->battery_level_binary_sensor_->publish_state(value == 0);
  }
}

void AcuRiteBinarySensor::update_rfi(bool value) {
  if (this->rfi_binary_sensor_) {
    this->rfi_binary_sensor_->publish_state(value);
  }
}

void AcuRiteBinarySensor::dump_config() {
  ESP_LOGCONFIG(TAG, "AcuRite Binary Sensor: 0x%04x", this->id_);
  LOG_BINARY_SENSOR("  ", "Battery", this->battery_level_binary_sensor_);
  LOG_BINARY_SENSOR("  ", "RFI", this->rfi_binary_sensor_);
}

}  // namespace acurite
}  // namespace esphome
