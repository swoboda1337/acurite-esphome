#include "acurite.h"
#include "esphome/core/log.h"

namespace esphome {
namespace acurite {

static const char *const TAG = "acurite";

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

void AcuRiteDevice::rainfall_count(uint32_t count) {
  if (this->rainfall_sensor_) {
    // filter out crc false positives by confirming any change in value
    if (count == this->rainfall_last_) {
      if (count != this->rainfall_state_.device) {
        // update rainfall state and save to nvm
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

void AcuRiteDevice::setup()
{
  if (this->rainfall_sensor_) {
    // load rainfall state from nvm, rainfall value needs to always increase even 
    // after a power reset on the esp or sensor
    uint32_t type = fnv1_hash(std::string("AcuRite: ") + format_hex(this->id_));
    this->preferences_ = global_preferences->make_preference<RainfallState>(type);
    this->preferences_.load(&this->rainfall_state_);
  }
}

void AcuRiteDevice::dump_config()
{
  if (this->rainfall_sensor_) {
    LOG_SENSOR("  ", "Rainfall", this->rainfall_sensor_);
  }
  if (this->temperature_sensor_) {
    LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  }
  if (this->humidity_sensor_) {
    LOG_SENSOR("  ", "Humidity", this->humidity_sensor_);
  }
}

bool AcuRite::validate_(uint8_t *data, uint8_t len)
{
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
  for (int32_t i = 2; i < len - 1; i++) {
    uint8_t sum = 0;
    for (int32_t b = 0; b < 8; b++) {
      sum ^= (data[i] >> b) & 1;
    }
    if (sum != 0) {
      ESP_LOGV(TAG, "Parity failure on byte %d", i);
      return false;
    }
  }
  return true;
}

void AcuRite::decode_6002rm_(uint8_t *data, uint8_t len) {
  if (len == 7 && this->validate_(data, 7)) {
    static const char channel_lut[4] = {'C', 'X', 'B', 'A'};
    char channel = channel_lut[data[0] >> 6];
    uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
    uint16_t battery = (data[2] >> 6) & 1;
    float humidity = data[3] & 0x7F;
    float temperature = ((float)(((data[4] & 0x0F) << 7) | (data[5] & 0x7F)) - 1000) / 10.0;
    ESP_LOGD(TAG, "Temp sensor: channel %c, id %04x, bat %d, temp %.1f°C, humidity %.1f%%",
             channel, id, battery, temperature, humidity);
    if (this->devices_.count(id) > 0) {
      this->devices_[id]->temperature_value(temperature);
      this->devices_[id]->humidity_value(humidity);
    }
  }
}

void AcuRite::decode_899_(uint8_t *data, uint8_t len) {
  if (len == 8 && this->validate_(data, 8)) {
    static const char channel_lut[4] = {'A', 'B', 'C', 'X'};
    char channel = channel_lut[data[0] >> 6];
    uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
    uint16_t battery = (data[2] >> 6) & 1;
    uint32_t count = ((data[4] & 0x7F) << 14) | ((data[5] & 0x7F) << 7) | ((data[6] & 0x7F) << 0);
    ESP_LOGD(TAG, "Rain sensor: channel %c, id %04x, bat %d, count %d", 
             channel, id, battery, count);
    if (this->devices_.count(id) > 0) {
      this->devices_[id]->rainfall_count(count);
    }
  }
}

bool AcuRite::on_receive(remote_base::RemoteReceiveData data)
{
  static const int32_t acurite_sync{600};
  static const int32_t acurite_one{400};
  static const int32_t acurite_zero{200};
  static const int32_t acurite_delta{100};
  uint32_t bits{0};
  uint32_t syncs{0};
  uint8_t bytes[8];

  // demodulate AcuRite OOK data
  for(auto i : data.get_raw_data()) {
    bool isSync = std::abs(acurite_sync - std::abs(i)) < acurite_delta;
    bool isZero = std::abs(acurite_zero - std::abs(i)) < acurite_delta;
    bool isOne = std::abs(acurite_one - std::abs(i)) < acurite_delta;
    bool level = (i >= 0);

    if ((isOne || isZero) && syncs > 2) {
      // detect bits using on state
      if (level == true) {
        uint8_t idx = bits / 8;
        uint8_t bit = 1 << (7 - (bits & 7));
        if (isOne) {
          bytes[idx] |=  bit;
        } else {
          bytes[idx] &= ~bit;
        }
        bits += 1;

        if ((bits & 7) == 0) {
          // try to decode on whole bytes
          this->decode_899_(bytes, bits / 8);
          this->decode_6002rm_(bytes, bits / 8);
        }

        if (bits >= sizeof(bytes) * 8) {
          // reset if buffer is full
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
  ESP_LOGI(TAG, "AcuRite Setup");

  // setup devices
  for (auto const& device : this->devices_) {
    device.second->setup();
  }

  // listen for data
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
