#include "acurite.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace acurite {

static const char *const TAG = "acurite";

#ifdef USE_SENSOR
void AcuRiteDevice::temperature_value(float value) {
  if (this->temperature_sensor_) {
    // if the new value changed significantly wait for it to be confirmed a 
    // second time in case it was corrupted by random bit flips
    if (fabsf(value - this->temperature_last_) < 1.0) {
      this->temperature_sensor_->publish_state(value);
    }
    this->temperature_last_ = value;
  }
}

void AcuRiteDevice::humidity_value(float value) {
  if (this->humidity_sensor_) {
    // if the new value changed significantly wait for it to be confirmed a 
    // second time in case it was corrupted by random bit flips
    if (fabsf(value - this->humidity_last_) < 1.0) {
      this->humidity_sensor_->publish_state(value);
    }
    this->humidity_last_ = value;
  }
}

void AcuRiteDevice::rainfall_count(uint32_t count) {
  if (this->rainfall_sensor_) {
    // if the new value changed significantly or decreased wait for it to be 
    // confirmed a second time in case it was corrupted by random bit flips
    if (count >= this->rainfall_last_ && (count - this->rainfall_last_) < 16) {
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
#endif

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

bool AcuRite::decode_6002rm_(uint8_t *data, uint8_t len) {
  // check length and validate
  if (len < 7 || this->validate_(data, 7) == false) {
    return false;
  }

  // decode data and update sensor
  static const char channel_lut[4] = {'C', 'X', 'B', 'A'};
  char channel = channel_lut[data[0] >> 6];
  uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
  uint16_t battery = (data[2] >> 6) & 1;
  float humidity = data[3] & 0x7F;
  float temperature = ((data[4] & 0x0F) << 7) | (data[5] & 0x7F);
  temperature = (temperature - 1000) / 10.0;
  ESP_LOGD(TAG, "Temp sensor: channel %c, id %04x, bat %d, temp %.1f°C, humidity %.1f%%",
           channel, id, battery, temperature, humidity);
#ifdef USE_SENSOR
  if (this->devices_.count(id) > 0) {
    this->devices_[id]->temperature_value(temperature);
    this->devices_[id]->humidity_value(humidity);
  }
#endif
  return true;
}

bool AcuRite::decode_899_(uint8_t *data, uint8_t len) {
  // check length and validate
  if (len < 8 || this->validate_(data, 8) == false) {
    return false;
  }

  // decode data and update sensor
  static const char channel_lut[4] = {'A', 'B', 'C', 'X'};
  char channel = channel_lut[data[0] >> 6];
  uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
  uint16_t battery = (data[2] >> 6) & 1;
  uint32_t count = ((data[4] & 0x7F) << 14) | ((data[5] & 0x7F) << 7) | ((data[6] & 0x7F) << 0);
  ESP_LOGD(TAG, "Rain sensor: channel %c, id %04x, bat %d, count %d", 
           channel, id, battery, count);
#ifdef USE_SENSOR
  if (this->devices_.count(id) > 0) {
    this->devices_[id]->rainfall_count(count);
  }
#endif
  return true;
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

  if (data.size() < ((7 * 8) * 2 + 4)) {
    return false;
  }

  for(auto i : data.get_raw_data()) {
    bool isSync = std::abs(acurite_sync - std::abs(i)) < acurite_delta;
    bool isZero = std::abs(acurite_zero - std::abs(i)) < acurite_delta;
    bool isOne = std::abs(acurite_one - std::abs(i)) < acurite_delta;
    bool level = (i >= 0);

    // validate signal state, only look for two syncs
    // as the first one can be affected by noise until
    // the agc adjusts
    if ((isOne || isZero) && syncs > 2) {
      // detect bit after on is complete
      if (level == true) {
        uint8_t idx = bits / 8;
        uint8_t bit = 1 << (7 - (bits & 7));
        if (isOne) {
          bytes[idx] |=  bit;
        } else {
          bytes[idx] &= ~bit;
        }
        bits += 1;

        // try to decode and reset if needed, return after each
        // successful decode to avoid blocking too long 
        if (this->decode_899_(bytes, bits / 8) || 
            this->decode_6002rm_(bytes, bits / 8) || 
            bits >= sizeof(bytes) * 8) {
          ESP_LOGV(TAG, "AcuRite data: %02x%02x%02x%02x%02x%02x%02x%02x", 
                   bytes[0], bytes[1], bytes[2], bytes[3], 
                   bytes[4], bytes[5], bytes[6], bytes[7]);
          bits = 0;
          syncs = 0;
        }
      }
    } else if (isSync && bits == 0) {
      // count sync after off is complete
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

#ifdef USE_SENSOR
  // setup devices
  for (auto const& device : this->devices_) {
    device.second->setup();
  }
#endif

  // listen for data
  this->remote_receiver_->register_listener(this);
}

float AcuRite::get_setup_priority() const { 
  return setup_priority::LATE; 
}

}  // namespace acurite
}  // namespace esphome
