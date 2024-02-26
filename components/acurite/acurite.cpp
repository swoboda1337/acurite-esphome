#include "acurite.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace acurite {

// acurite ook params
#define ACURITE_SYNC   600
#define ACURITE_ONE    400
#define ACURITE_ZERO   200
#define ACURITE_DELTA  100

static const char *const TAG = "acurite";

void IRAM_ATTR AcuRiteStore::gpio_intr(AcuRiteStore *arg) {
  bool one, zero;
  uint32_t bit, idx;
  uint32_t curr = micros();
  int32_t diff = curr - arg->prev;
  arg->prev = curr;

  switch (arg->state) {
    case ACURITE_INIT:
      // reset state and move to sync detection at the start of 'on'
      arg->bits = 0;
      arg->syncs = 0;
      if (arg->pin.digital_read() == true) {
        arg->state = ACURITE_SYNC_ON;
      }
      break;
    
    case ACURITE_SYNC_ON:
      // confirm the duration of 'on' was correct
      if (std::abs(ACURITE_SYNC - diff) < ACURITE_DELTA) {
        arg->state = ACURITE_SYNC_OFF;
      } else {
        arg->state = ACURITE_INIT;
      }  
      break;
    
    case ACURITE_SYNC_OFF:
      // confirm the duration of 'off' was correct
      if (std::abs(ACURITE_SYNC - diff) < ACURITE_DELTA) {
        // sync sequence is 4 'on's followed by 4 'off's (same duration)
        // either go back to sync detection or move on to bit detection
        arg->syncs++;
        if (arg->syncs == 4) {
          arg->state = ACURITE_BIT_ON;
        } else {
          arg->state = ACURITE_SYNC_ON;
        }
      } else {
        arg->state = ACURITE_INIT;
      }
      break;

    case ACURITE_BIT_ON:
      // use the duration of 'on' to determine if this was a one or a zero
      one = std::abs(ACURITE_ONE - diff) < ACURITE_DELTA;
      zero = std::abs(ACURITE_ZERO - diff) < ACURITE_DELTA;
      if (one || zero) {
        idx = arg->bits / 8;
        bit = 1 << (7 - (arg->bits & 7));
        if (one) {
          arg->data[idx] |=  bit;
        } else {
          arg->data[idx] &= ~bit;
        }
        arg->bits++;
        arg->state = (arg->bits >= 8 * 8) ? ACURITE_DONE : ACURITE_BIT_OFF;
      } else {
        arg->state = (arg->bits >= 7 * 8) ? ACURITE_DONE : ACURITE_INIT;
      }
      break;
    
    case ACURITE_BIT_OFF:
      // confirm the duration of 'off' was not too large
      if (diff < ACURITE_SYNC) {
        arg->state = ACURITE_BIT_ON;
      } else {
        arg->state = (arg->bits >= 7 * 8) ? ACURITE_DONE : ACURITE_INIT;
      }
      break;
    
    default:
      break;
  }
}

void AcuRite::loop() {
  if (store.state == ACURITE_DONE)
  {
    static const char channel[4] = {'C', 'X', 'B', 'A'};
    bool valid = true;
    uint8_t data[8];
    uint8_t len;
    
    // copy data and reset state
    len = store.bits / 8;
    for (int32_t i = 0; i < len; i++) {
      data[i] = store.data[i];
    }
    store.state = ACURITE_INIT;

    // print data
    if (len == 8) {
      ESP_LOGD(TAG, "AcuRite data: %02x %02x %02x %02x %02x %02x %02x %02x", 
               data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    } else {
      ESP_LOGD(TAG, "AcuRite data: %02x %02x %02x %02x %02x %02x %02x", 
               data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
    }
    
    // check checksum
    uint8_t sum = 0;
    for (int32_t i = 0; i < (len - 1); i++) {
      sum += data[i];
    }
    if (sum != data[len - 1]) {
      ESP_LOGD(TAG, "Checksum failure %02x vs %02x", sum, data[len - 1]);
      valid = false;
    }

    // temperature sensor
    if (len == 7) {
      // check parity (bytes 1 to 5)
      for (int32_t i = 1; i <= 5; i++) {
        uint8_t parity = (data[i] >> 7) & 1;
        uint8_t sum = 0;
        for (int32_t b = 0; b <= 6; b++) {
          sum ^= (data[i] >> b) & 1;
        }
        if (parity != sum) {
          ESP_LOGD(TAG, "Parity failure on byte %d", i);
          valid = false;
        }
      }

      // device info
      uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
      ESP_LOGD(TAG, "ID: %04x", id);
      ESP_LOGD(TAG, "Channel: %c",  channel[data[0] >> 6]);
      ESP_LOGD(TAG, "Signature: %02x", data[2] & 0x7F);
        
      // data
      if (valid) {
        float humidity = data[3] & 0x7F;
        float temperature = ((data[4] & 0x0F) << 7) | (data[5] & 0x7F);
        temperature = (temperature - 1000) / 10.0;
        ESP_LOGD(TAG, "Got temperature=%.1f°C humidity=%.1f%%", temperature, humidity);
        if (temperature_sensors_.count(id) > 0) {
          temperature_sensors_[id]->publish_state(temperature);
        }
        if (humidity_sensors_.count(id) > 0) {
          humidity_sensors_[id]->publish_state(humidity);
        }
        status_clear_warning();
      }
    } else if (len == 8 && valid) {
      uint16_t id = ((data[0] & 0x3F) << 8) | (data[1] & 0xFF);
      uint32_t counter = ((data[4] & 0x7F) << 14) | ((data[5] & 0x7F) << 7) | ((data[6] & 0x7F) << 0);
      ESP_LOGD(TAG, "ID: %04x", id);
      ESP_LOGD(TAG, "Counter: %d", counter);
      if (rain_sensors_.count(id) > 0) {
        // check for new devices or device reset
        if (rain_counters_.count(id) == 0 || counter < rain_counters_[id]) {
          rain_counters_[id] = counter;
        }

        // update daily count
        float mm = (float)(counter - rain_counters_[id]) * 0.254;
        if (rain_sensors_[id]->has_state()) {
          mm += rain_sensors_[id]->get_raw_state();
        }
        rain_sensors_[id]->publish_state(mm);

        // update counter
        rain_counters_[id] = counter;
      }
    }
  }
}

void AcuRite::zero_rain_totals()
{
  // clear rain totals, called from yaml
  for (auto const& s : rain_sensors_) {
    s.second->publish_state(0.0);
  }
  ESP_LOGI(TAG, "Rain totals have been set to zero");
}

void AcuRite::setup() {
  ESP_LOGI(TAG, "AcuRite Setup");

  // the gpio is connected to the output of the ook demodulator, 
  // when the signal state is on the gpio will go high and when
  // it is off the gpio will go low, trigger on both edges in 
  // order to detect on duration  
  this->pin_->setup();
  store.state = ACURITE_INIT;
  store.pin = pin_->to_isr();
  this->pin_->attach_interrupt(AcuRiteStore::gpio_intr, &store, gpio::INTERRUPT_ANY_EDGE);
}

}  // namespace acurite
}  // namespace esphome
