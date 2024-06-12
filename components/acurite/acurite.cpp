#include "acurite.h"

namespace esphome {
namespace acurite {

static const char *const TAG = "acurite";

void AcuRiteDevice::wind_speed_value(float value) {
  if (this->wind_speed_sensor_) {
    // filter out crc false positives by confirming any large changes in value
    if (fabsf(value - this->wind_speed_last_) < 1.0) {
      this->wind_speed_sensor_->publish_state(value);
    }
    this->wind_speed_last_ = value;
  }
}

void AcuRiteDevice::temperature_value(float value) {
  if (this->temperature_sensor_) {
    // filter out crc false positives by confirming any large changes in value
    if (fabsf(value - this->temperature_last_) < 1.0) {
      this->temperature_sensor_->publish_state(value);
    }
    this->temperature_last_ = value;
  }
}

void AcuRiteDevice::humidity_value(float value) {
  if (this->humidity_sensor_) {
    // filter out crc false positives by confirming any large changes in value
    if (fabsf(value - this->humidity_last_) < 1.0) {
      this->humidity_sensor_->publish_state(value);
    }
    this->humidity_last_ = value;
  }
}

void AcuRiteDevice::distance_value(float value) {
  if (this->distance_sensor_) {
    // filter out crc false positives by confirming any large changes in value
    if (fabsf(value - this->distance_last_) < 1.0) {
      this->distance_sensor_->publish_state(value);
    }
    this->distance_last_ = value;
  }
}

void AcuRiteDevice::lightning_count(uint32_t count) {
  if (this->lightning_sensor_) {
    // filter out crc false positives by confirming any change in value
    if (count == this->lightning_last_) {
      this->lightning_sensor_->publish_state(count);
    }
    this->lightning_last_ = count;
  }
}

void AcuRiteDevice::rainfall_count(uint32_t count) {
  if (this->rainfall_sensor_) {
    // filter out crc false positives by confirming any change in value
    if (count == this->rainfall_last_) {
      // update rainfall state and save to nvm
      if (count != this->rainfall_state_.device) {
        if (count >= this->rainfall_state_.device) {
          this->rainfall_state_.total += count - this->rainfall_state_.device;
        } else {
          this->rainfall_state_.total += count;
        }
        this->rainfall_state_.device = count;
        this->preferences_.save(&this->rainfall_state_);
      }
      this->rainfall_sensor_->publish_state(this->rainfall_state_.total * 0.254);
    }
    this->rainfall_last_ = count;
  }
}

void AcuRiteDevice::setup() {
  if (this->rainfall_sensor_) {
    // load state from nvm, count needs to increase even after a reset
    uint32_t type = fnv1_hash(std::string("AcuRite: ") + format_hex(this->id_));
    this->preferences_ = global_preferences->make_preference<RainfallState>(type);
    this->preferences_.load(&this->rainfall_state_);
  }
}

void AcuRiteDevice::dump_config() {
  ESP_LOGCONFIG(TAG, "  0x%04x:", this->id_);
  LOG_SENSOR("    ", "Wind Speed", this->wind_speed_sensor_);
  LOG_SENSOR("    ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("    ", "Humidity", this->humidity_sensor_);
  LOG_SENSOR("    ", "Rainfall", this->rainfall_sensor_);
  LOG_SENSOR("    ", "Distance", this->distance_sensor_);
  LOG_SENSOR("    ", "Lightning", this->lightning_sensor_);
}

bool AcuRite::validate_(uint8_t *data, uint8_t len, bool parity) {
  ESP_LOGV(TAG, "Validating data: %s", format_hex(data, len).c_str());

  // checksum
  uint8_t sum = 0;
  for (int32_t i = 0; i < len - 1; i++) {
    sum += data[i];
  }
  if (sum != data[len - 1]) {
    ESP_LOGV(TAG, "Checksum failure %02x vs %02x", sum, data[len - 1]);
    return false;
  }

  // parity (excludes id and crc)
  if (parity) {
    for (int32_t i = 2; i < len - 1; i++) {
      uint8_t sum = 0;
      for (int32_t b = 0; b < 8; b++) {
        sum ^= data[i] >> b;
      }
      if ((sum & 1) != 0) {
        ESP_LOGV(TAG, "Parity failure on byte %d", i);
        return false;
      }
    }
  }
  return true;
}

void AcuRite::decode_temperature_(uint8_t *data, uint8_t len) {
  if (len == 7 && (data[2] & 0x3F) == 0x04 && this->validate_(data, 7, true)) {
    static const char channel_lut[4] = {'C', 'X', 'B', 'A'};
    char channel = channel_lut[data[0] >> 6];
    uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
    uint16_t battery = (data[2] >> 6) & 1;
    float humidity = data[3] & 0x7F;
    float temperature = ((float)(((data[4] & 0x0F) << 7) | (data[5] & 0x7F)) - 1000) / 10.0;
    ESP_LOGD(TAG, "Temperature:  ch %c, id %04x, bat %d, temp %.1f, rh %.1f",
             channel, id, battery, temperature, humidity);
    if (this->devices_.count(id) > 0) {
      this->devices_[id]->temperature_value(temperature);
      this->devices_[id]->humidity_value(humidity);
    }
  }
}

void AcuRite::decode_rainfall_(uint8_t *data, uint8_t len) {
  if (len == 8 && (data[2] & 0x3F) == 0x30 && this->validate_(data, 8, true)) {
    static const char channel_lut[4] = {'A', 'B', 'C', 'X'};
    char channel = channel_lut[data[0] >> 6];
    uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
    uint16_t battery = (data[2] >> 6) & 1;
    uint32_t count = ((data[4] & 0x7F) << 14) | ((data[5] & 0x7F) << 7) | ((data[6] & 0x7F) << 0);
    ESP_LOGD(TAG, "Rainfall:     ch %c, id %04x, bat %d, count %d", 
             channel, id, battery, count);
    if (this->devices_.count(id) > 0) {
      this->devices_[id]->rainfall_count(count);
    }
  }
}

void AcuRite::decode_lightning_(uint8_t *data, uint8_t len) {
  if (len == 9 && (data[2] & 0x3F) == 0x2F && this->validate_(data, 9, true)) {
    static const int8_t distance_lut[32] = {2, 2, 2, 2, 5, 6, 6, 8, 10, 10, 12, 12, 
                                            14, 14, 14, 17, 17, 20, 20, 20, 24, 24, 
                                            27, 27, 31, 31, 31, 34, 37, 37, 40, 40};
    static const char channel_lut[4] = {'C', 'X', 'B', 'A'};
    char channel = channel_lut[data[0] >> 6];
    uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
    uint16_t battery = (data[2] >> 6) & 1;
    float humidity = data[3] & 0x7F;
    float temperature = ((float)(((data[4] & 0x1F) << 7) | (data[5] & 0x7F)) - 1800) * 0.1 * 5.0 / 9.0;
    float distance = distance_lut[data[7] & 0x1F];
    uint16_t count = ((data[6] & 0x7F) << 1) | ((data[7] >> 6) & 1);
    uint16_t rfi = (data[7] >> 5) & 1;
    ESP_LOGD(TAG, "Lightning:    ch %c, id %04x, bat %d, temp %.1f, rh %.1f, count %d, dist %.1f, rfi %d",
             channel, id, battery, temperature, humidity, count, distance, rfi);
    if (this->devices_.count(id) > 0) {
      this->devices_[id]->temperature_value(temperature);
      this->devices_[id]->humidity_value(humidity);
      this->devices_[id]->lightning_count(count);
      this->devices_[id]->distance_value(distance);
    }
  }
}

void AcuRite::decode_notos_(uint8_t *data, uint8_t len) {
  if (len == 8 && (data[2] & 0x3F) == 0x20 && this->validate_(data, 8, false)) {
    static const char channel_lut[4] = {'C', 'X', 'B', 'A'};
    char channel = channel_lut[data[0] >> 6];
    uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
    uint16_t battery = (data[2] >> 6) & 1;
    float humidity = data[3] & 0x7F;
    float temperature = ((float)(((data[4] & 0x1F) << 7) | (data[5] & 0x7F)) - 1800) * 0.1 * 5.0 / 9.0;
    float speed = (float)(data[6] & 0x7F) * 2.54902f;  // doesn't match My AcuRite exactly but its close 
    ESP_LOGD(TAG, "Notos 3-in-1: ch %c, id %04x, bat %d, temp %.1f, rh %.1f, speed %.1f",
             channel, id, battery, temperature, humidity, speed);
    if (this->devices_.count(id) > 0) {
      this->devices_[id]->temperature_value(temperature);
      this->devices_[id]->humidity_value(humidity);
      this->devices_[id]->wind_speed_value(speed);
    }
  }
}

bool AcuRite::on_receive(remote_base::RemoteReceiveData data) {
  uint32_t syncs = 0;
  uint32_t bits = 0;
  uint8_t bytes[9];

  ESP_LOGV(TAG, "Received raw data with length %d", data.size());

  // demodulate AcuRite OOK data
  for(auto i : data.get_raw_data()) {
    bool isSync = std::abs(600 - std::abs(i)) < 100;
    bool isZero = std::abs(200 - std::abs(i)) < 100;
    bool isOne = std::abs(400 - std::abs(i)) < 100;
    bool level = (i >= 0);
    if ((isOne || isZero) && syncs > 2) {
      if (level == true) {
        // detect bits using on state
        bytes[bits / 8] <<= 1;
        bytes[bits / 8] |= isOne ? 1 : 0;
        bits += 1;

        // try to decode on whole bytes
        if ((bits & 7) == 0) {
          this->decode_temperature_(bytes, bits / 8);
          this->decode_rainfall_(bytes, bits / 8);
          this->decode_lightning_(bytes, bits / 8);
          this->decode_notos_(bytes, bits / 8);
        }

        // reset if buffer is full
        if (bits >= sizeof(bytes) * 8) {
          bits = 0;
          syncs = 0;
        }
      }
    } else if (isSync && bits == 0) {
      // count sync using off state
      if (level == false) {
        syncs++;
      }
    } else {
      // reset if state is invalid
      bits = 0;
      syncs = 0;
    }
  }
  return true;
}

void AcuRite::setup() {
  ESP_LOGCONFIG(TAG, "Setting up AcuRite...");
  for (auto const& device : this->devices_) {
    device.second->setup();
  }
  this->remote_receiver_->register_listener(this);
}

void AcuRite::dump_config() {
  ESP_LOGCONFIG(TAG, "AcuRite:");
  for (auto const& device : this->devices_) {
    device.second->dump_config();
  }
}

}  // namespace acurite
}  // namespace esphome
