#include "acurite.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace acurite {

static const char *const TAG = "acurite";

// acurite ook params
static const int32_t ACURITE_SYNC  = 600;
static const int32_t ACURITE_ONE   = 400;
static const int32_t ACURITE_ZERO  = 200;
static const int32_t ACURITE_DELTA = 100;

void IRAM_ATTR HOT OokStore::gpio_intr(OokStore *arg) {
  uint32_t now = micros();
  uint32_t next = (arg->write + 1) & (arg->size - 1);
  uint32_t delta = now - arg->prev;
  uint32_t level = arg->pin.digital_read() ?  1 : 0;
  arg->prev = now;

  // check for overflow 
  if (next == arg->read) {
    arg->overflow = true;
    return;
  }

  // ingore if less than filter length and mark the last 
  // entry to be overwritten as it should be filtered too
  if (delta < arg->filter) {
    arg->filtered = true;
    return;
  } 

  // write micros or skip if the state is wrong
  if (arg->filtered == false) {
    if (level != (next & 1)) {
      return;
    }
    arg->write = next;
    arg->buffer[arg->write] = now;
  } else {
    if (level != (arg->write & 1)) {
      return;
    }
    arg->filtered = false;
    arg->buffer[arg->write] = now;
  }
}

void TemperatureSensor::new_value(float value)
{
  // if the new value changed significantly wait for it to be confirmed a 
  // second time incase it was corrupted by random bit flips
  if (fabsf(value - this->value_last_) < 1.0 && fabsf(value - this->value_published_) > 0.01) {
    this->sensor_->publish_state(value);
    this->value_published_ = value;
  }
  this->value_last_ = value;
}

void HumiditySensor::new_value(float value)
{
  // if the new value changed significantly wait for it to be confirmed a 
  // second time incase it was corrupted by random bit flips
  if (fabsf(value - this->value_last_) < 2.0 && fabsf(value - this->value_published_) > 0.01) {
    this->sensor_->publish_state(value);
    this->value_published_ = value;
  }
  this->value_last_ = value;
}

void RainSensor::new_value(uint32_t count)
{
  // if the new value changed significantly wait for it to be confirmed a 
  // second time incase it was corrupted by random bit flips
  if (count >= this->count_last_ && (count - this->count_last_) < 16) {
     // check for device reset or first receive 
    if (count < this->count_device_) {
      this->count_device_ = count;
    }

    // update daily count and sensor
    this->count_period_ += count - this->count_device_;
    if (this->count_period_ != this->count_published_) {
      this->sensor_->publish_state(this->count_period_ * 0.254);
      this->count_published_ = this->count_period_;
    } 
  }
  this->count_last_ = count;
}

void RainSensor::reset_period()
{
  // reset period count and publish
  this->sensor_->publish_state(0.0);
  this->count_published_ = 0;
  this->count_period_ = 0;
}

bool AcuRite::decode_6002rm_(uint8_t *data, uint8_t len) {
  // needs to be 7 bytes
  if (len != 7 * 8) {
    return false;
  }
  
  // checksum
  uint8_t sum = 0;
  for (int32_t i = 0; i <= 5; i++) {
    sum += data[i];
  }
  if (sum != data[6]) {
    ESP_LOGV(TAG, "Checksum failure %02x vs %02x", sum, data[6]);
    return false;
  }

  // parity (bytes 2 to 5)
  for (int32_t i = 2; i <= 5; i++) {
    uint8_t parity = (data[i] >> 7) & 1;
    uint8_t sum = 0;
    for (int32_t b = 0; b <= 6; b++) {
      sum ^= (data[i] >> b) & 1;
    }
    if (parity != sum) {
      ESP_LOGV(TAG, "Parity failure on byte %d", i);
      return false;
    }
  }

  // decode data and update sensor
  static const char channel_lut[4] = {'C', 'X', 'B', 'A'};
  char channel = channel_lut[data[0] >> 6];
  uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
  float humidity = data[3] & 0x7F;
  float temperature = ((data[4] & 0x0F) << 7) | (data[5] & 0x7F);
  temperature = (temperature - 1000) / 10.0;
  ESP_LOGD(TAG, "Temp sensor: channel %c, id %04x, temperature %.1f°C, humidity %.1f%%", 
           channel, id, temperature, humidity);
  if (this->temperature_sensors_.count(id) > 0) {
    this->temperature_sensors_[id]->new_value(temperature);
  }
  if (this->humidity_sensors_.count(id) > 0) {
    this->humidity_sensors_[id]->new_value(humidity);
  }
  return true;
}

bool AcuRite::decode_899_(uint8_t *data, uint8_t len) {
  // needs to be 8 bytes
  if (len != 8 * 8) {
    return false;
  }
  
  // checksum
  uint8_t sum = 0;
  for (int32_t i = 0; i <= 6; i++) {
    sum += data[i];
  }
  if (sum != data[7]) {
    ESP_LOGV(TAG, "Checksum failure %02x vs %02x", sum, data[7]);
    return false;
  }

  // parity (bytes 4 to 6)
  for (int32_t i = 4; i <= 6; i++) {
    uint8_t parity = (data[i] >> 7) & 1;
    uint8_t sum = 0;
    for (int32_t b = 0; b <= 6; b++) {
      sum ^= (data[i] >> b) & 1;
    }
    if (parity != sum) {
      ESP_LOGV(TAG, "Parity failure on byte %d", i);
      return false;
    }
  }

  // decode data and update sensor
  static const char channel_lut[4] = {'A', 'B', 'C', 'X'};
  char channel = channel_lut[data[0] >> 6];
  uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
  uint32_t count = ((data[4] & 0x7F) << 14) | ((data[5] & 0x7F) << 7) | ((data[6] & 0x7F) << 0);
  ESP_LOGD(TAG, "Rain gauge: channel %c, id %04x, count %d", channel, id, count);
  if (this->rain_sensors_.count(id) > 0) {
    this->rain_sensors_[id]->new_value(count);
  }
  return true;
}

void AcuRite::loop() {
  while (this->store_.read != this->store_.write) {
    uint8_t level = this->store_.read & 1;
    uint32_t micros = this->store_.buffer[this->store_.read];
    int32_t delta = micros - this->prev_;
    bool isSync = std::abs(ACURITE_SYNC - delta) < ACURITE_DELTA;
    bool isZero = std::abs(ACURITE_ZERO - delta) < ACURITE_DELTA;
    bool isOne = std::abs(ACURITE_ONE - delta) < ACURITE_DELTA;

    // update read index, prev micros and warn about overflow 
    this->prev_ = micros;
    this->store_.read = (this->store_.read + 1) & (this->store_.size - 1);
    if (this->store_.overflow) {
      ESP_LOGW(TAG, "Buffer overflow");
      this->store_.overflow = false;
    }

    // validate signal state, only look for two syncs
    // as the first one can be affected by noise until
    // the agc adjusts
    if ((isOne || isZero) && this->syncs_ > 2) {
      // detect bit after on is complete
      if (level == 0) {
        uint8_t idx = this->bits_ / 8;
        uint8_t bit = 1 << (7 - (this->bits_ & 7));
        if (isOne) {
          this->data_[idx] |=  bit;
        } else {
          this->data_[idx] &= ~bit;
        }
        this->bits_ += 1;

        // try to decode and reset if needed, return after each
        // successful decode to avoid blocking too long 
        if (decode_899_(this->data_, this->bits_) || 
            decode_6002rm_(this->data_, this->bits_) || 
            this->bits_ >= sizeof(this->data_) * 8) {
          ESP_LOGVV(TAG, "AcuRite data: %02x%02x%02x%02x%02x%02x%02x%02x", 
                    this->data_[0], this->data_[1], this->data_[2], this->data_[3], 
                    this->data_[4], this->data_[5], this->data_[6], this->data_[7]);
          this->bits_ = 0;
          this->syncs_ = 0;
          return;
        }
      }
    } else if (isSync && this->bits_ == 0) {
      // count sync after off is complete
      if (level == 1) {
        this->syncs_++;
      }
    } else {
      // reset if state is invalid
      this->bits_ = 0;
      this->syncs_ = 0;
    }
  }
}

void AcuRite::reset_rain_totals()
{
  // clear rain totals, called from yaml
  for (auto const& item : this->rain_sensors_) {
    item.second->reset_period();
  }
  ESP_LOGI(TAG, "Rain totals have been set to zero");
}

void AcuRite::setup() {
  ESP_LOGI(TAG, "AcuRite Setup");

  // init isr store
  this->store_.buffer = new uint32_t[store_.size];
  memset((uint8_t*)this->store_.buffer, 0, this->store_.size * sizeof(uint32_t));

  // the gpio is connected to the output of the ook demodulator, 
  // when the signal state is on the gpio will go high and when
  // it is off the gpio will go low, trigger on both edges in 
  // order to detect on duration  
  this->pin_->setup();
  this->store_.pin = pin_->to_isr();
  this->pin_->attach_interrupt(OokStore::gpio_intr, &this->store_, gpio::INTERRUPT_ANY_EDGE);
}

float AcuRite::get_setup_priority() const { return setup_priority::LATE; }

}  // namespace acurite
}  // namespace esphome
