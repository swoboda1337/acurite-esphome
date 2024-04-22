#include "acurite.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace acurite {

static const char *const TAG = "acurite";

// time before rainfall sensor is marked as dry
static const int32_t RAINFALL_TIMEOUT = 15 * 60 * 1000;

// acurite ook params
static const int32_t ACURITE_SYNC  = 600;
static const int32_t ACURITE_ONE   = 400;
static const int32_t ACURITE_ZERO  = 200;
static const int32_t ACURITE_DELTA = 100;

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
    if (fabsf(value - this->humidity_last_) < 2.0) {
      this->humidity_sensor_->publish_state(value);
    }
    this->humidity_last_ = value;
  }
}

bool AcuRiteDevice::rainfall_count(uint32_t count, ESPTime now) {
  uint32_t delta = 0;

  // if the new value changed significantly wait for it to be confirmed a 
  // second time in case it was corrupted by random bit flips
  if ((count - this->rainfall_count_last_) < 16) {
    uint8_t hour = this->rainfall_count_time_.hour;
    uint8_t minute = this->rainfall_count_time_.minute;

    // check for device reset and calculate delta
    if (count < this->rainfall_count_device_) {
      this->rainfall_count_device_ = count;
    }
    delta = count - this->rainfall_count_device_;
    this->rainfall_count_device_ = count;

    // zero expired data, update time and increment counts
    if (now.is_valid() && now > this->rainfall_count_time_) {
      while (minute != now.minute || hour != now.hour) {
        minute += 1;
        if (minute >= 60) {
          minute = 0;
          hour += 1;
          if (hour >= 24) {
            hour = 0;
          }
        }
        this->rainfall_count_buffer_[hour * 60 + minute] = 0;
      }
      this->rainfall_count_time_ = now;
    }
    this->rainfall_count_buffer_[hour * 60 + minute] += delta;

    // update daily sensor
    if (this->rainfall_sensor_daily_) {
      this->rainfall_count_daily_ += delta;
      this->rainfall_sensor_daily_->publish_state(this->rainfall_count_daily_ * 0.254);
    }

    // update 1 hour sensor
    if (this->rainfall_sensor_1hr_) {
      uint32_t total = 0;
      for (auto i = 0; i < 1 * 60; i++) {
        total += this->rainfall_count_buffer_[(24 * 60 + hour * 60 + minute - i) % (24 * 60)];
      }
      this->rainfall_sensor_1hr_->publish_state(total * 0.254);
    }

    // update 24 hour sensor
    if (this->rainfall_sensor_24hr_) {
      uint32_t total = 0;
      for (auto i = 0; i < 24 * 60; i++) {
        total += this->rainfall_count_buffer_[i];
      }
      this->rainfall_sensor_24hr_->publish_state(total * 0.254);
    }
  }

  this->rainfall_count_last_ = count;

  return delta > 0;
}

void AcuRiteDevice::reset_daily() {
  // reset daily count and publish
  if (this->rainfall_sensor_daily_) {
    this->rainfall_sensor_daily_->publish_state(0.0);
    this->rainfall_count_daily_ = 0;
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
  ESP_LOGD(TAG, "Temp sensor: channel %c, id %04x, bat %d, temp %.1f°C, rh %.1f%%",
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
  ESP_LOGD(TAG, "Rain gauge:  channel %c, id %04x, bat %d, count %d", 
           channel, id, battery, count);
#ifdef USE_SENSOR
  if (this->devices_.count(id) > 0) {
    ESPTime now = this->srctime_->utcnow();
    if (!now.is_valid()) {
      // if time is not synchronized daily rain count won't make sense
      ESP_LOGW(TAG, "Waiting for system time to be synchronized");
    } else {
      // use UTC time here to avoid any DST changes
      bool raining = this->devices_[id]->rainfall_count(count, now);
#ifdef USE_BINARY_SENSOR
      if (raining && this->rainfall_sensor_) {
        // if raining mark sensor as wet and schedule timeout to mark as dry
        this->rainfall_sensor_->publish_state(true);
        set_timeout("AcuRite::rainfall_timeout", RAINFALL_TIMEOUT, [this]() {
          this->rainfall_sensor_->publish_state(false);
        });
      }
#endif
    }
  }
#endif
  return true;
}

bool AcuRite::on_receive(remote_base::RemoteReceiveData data)
{
  uint32_t bits{0};
  uint32_t syncs{0};
  uint8_t bytes[8];

  if (data.size() < ((7 * 8) * 2 + 4)) {
    return false;
  }

  for(auto i : data.get_raw_data()) {
    bool isSync = std::abs(ACURITE_SYNC - std::abs(i)) < ACURITE_DELTA;
    bool isZero = std::abs(ACURITE_ZERO - std::abs(i)) < ACURITE_DELTA;
    bool isOne = std::abs(ACURITE_ONE - std::abs(i)) < ACURITE_DELTA;
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
          ESP_LOGVV(TAG, "AcuRite data: %02x%02x%02x%02x%02x%02x%02x%02x", 
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

void AcuRite::loop() {
#ifdef USE_SENSOR
  // check for midnight to reset daily rainfall
  ESPTime now = this->srctime_->now();
  if (now.is_valid()) {
    if (now.hour == 0) {
      if (this->midnight_ == false) {
        ESP_LOGI(TAG, "Resetting daily totals");
        for (auto const& item : this->devices_) {
          item.second->reset_daily();
        }
        this->midnight_ = true;
      }
    } else if (now.hour > 2) {
      this->midnight_ = false;
    }
  }
#endif
}

void AcuRite::setup() {
  ESP_LOGI(TAG, "AcuRite Setup");

#ifdef USE_BINARY_SENSOR
  // init rainfall binary sensor
  if (this->rainfall_sensor_) {
    this->rainfall_sensor_->publish_state(false);
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
