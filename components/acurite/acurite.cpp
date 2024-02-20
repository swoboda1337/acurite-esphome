#include "acurite.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace acurite {

// registers
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_RX_BW                0x12
#define REG_OOK_PEAK             0x14
#define REG_SYNC_CONFIG          0x27
#define REG_PACKET_CONFIG_1      0x30
#define REG_PACKET_CONFIG_2      0x31
#define REG_IRQ_FLAGS_1          0x3e
#define REG_IRQ_FLAGS_2          0x3f
#define REG_VERSION              0x42

// modes
#define MODE_MOD_OOK             0x20
#define MODE_LF_ON               0x08
#define MODE_SLEEP               0x00
#define MODE_STDBY               0x01
#define MODE_RX_FS               0x04
#define MODE_RX                  0x05

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
      ESP_LOGD(TAG, "ID: %04x", ((data[0] & 0x3F) << 7) | (data[1] & 0x7F));
      ESP_LOGD(TAG, "Channel: %c",  channel[data[0] >> 6]);
      ESP_LOGD(TAG, "Signature: %02x", data[2] & 0x7F);
        
      // data
      if (valid) {
        float humidity = data[3] & 0x7F;
        float temperature = ((data[4] & 0x0F) << 7) | (data[5] & 0x7F);
        temperature = (temperature - 1000) / 10.0;
        ESP_LOGD(TAG, "Got temperature=%.1f°C humidity=%.1f%%", temperature, humidity);
        if (temperature_sensor_ != nullptr) {
          temperature_sensor_->publish_state(temperature);
        }
        if (humidity_sensor_ != nullptr) {
          humidity_sensor_->publish_state(humidity);
        }
        status_clear_warning();
      }
    } else if (len == 8 && valid) {
      int32_t counter = ((data[4] & 0x7F) << 14) | ((data[5] & 0x7F) << 7) | ((data[6] & 0x7F) << 0);
      float rainfall = (float)counter * 0.01 * 25.4;
      ESP_LOGD(TAG, "Got rainfall=%.1fmm", rainfall);
      if (rain_sensor_ != nullptr) {
        rain_sensor_->publish_state(rainfall);
      }
    }
  }
}

void AcuRite::setFrequency(uint64_t frequency)
{
  uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
  writeRegister(REG_FRF_MSB, (uint8_t)((frf >> 16) & 0xFF));
  writeRegister(REG_FRF_MID, (uint8_t)((frf >> 8) & 0xFF));
  writeRegister(REG_FRF_LSB, (uint8_t)((frf >> 0) & 0xFF));
}

uint8_t AcuRite::readRegister(uint8_t address)
{
  return singleTransfer(address & 0x7f, 0x00);
}

void AcuRite::writeRegister(uint8_t address, uint8_t value)
{
  singleTransfer(address | 0x80, value);
}

uint8_t AcuRite::singleTransfer(uint8_t address, uint8_t value)
{
  uint8_t response;
  delegate_->begin_transaction();
  nss_->digital_write(false);
  delegate_->transfer(address);
  response = delegate_->transfer(value);
  nss_->digital_write(true);
  delegate_->end_transaction();
  return response;
}

void AcuRite::setup() {
  ESP_LOGD(TAG, "AcuRite Setup");

  // dio2 is connected to the output of the ook demodulator, 
  // when the signal state is on the gpio will go high and when
  // it is off the gpio will go low, trigger on both edges in 
  // order to detect on duration  
  dio2_->setup();
  store.state = ACURITE_INIT;
  store.pin = dio2_->to_isr();
  dio2_->attach_interrupt(AcuRiteStore::gpio_intr, &store, gpio::INTERRUPT_ANY_EDGE);

  // init nss and set high
  nss_->setup();
  nss_->digital_write(true);

  // init reset and toggle to reset chip
  rst_->setup();
  rst_->digital_write(false);
  delay(1);
  rst_->digital_write(true);
  delay(10);

  // start spi
  spi_setup();

  // check silicon version to make sure hw is ok
  uint8_t version = readRegister(REG_VERSION);
  ESP_LOGD(TAG, "SemTech ID: %0x", version);
  if (version != 0x12) {
    mark_failed();
    return;
  }

  // need to sleep before changing some settings
  writeRegister(REG_OP_MODE, MODE_MOD_OOK | MODE_LF_ON | MODE_SLEEP);
  delay(20);

  // set freq
  setFrequency(433920000);

  // set bw to 50 kHz as this should result in ~20 usecs between samples, 
  // ie a minimum of 20 usecs between ook demodulator / dio2 gpio state
  // changes, which is fast enough to detect the difference in duration 
  // between acurite ook bits which have a minimum duration of ~200 usecs
  writeRegister(REG_RX_BW, 0x0B);

  // disable crc check and enable continuous mode
  writeRegister(REG_PACKET_CONFIG_1, 0x80);
  writeRegister(REG_PACKET_CONFIG_2, 0x00);

  // disable bit synchronizer and sync generation
  writeRegister(REG_SYNC_CONFIG, 0x00);
  writeRegister(REG_OOK_PEAK, 0x08);

  // enable rx mode  
  writeRegister(REG_OP_MODE, MODE_MOD_OOK | MODE_LF_ON | MODE_STDBY);
  delay(20);
  writeRegister(REG_OP_MODE, MODE_MOD_OOK | MODE_LF_ON | MODE_RX_FS);
  delay(20);
  writeRegister(REG_OP_MODE, MODE_MOD_OOK | MODE_LF_ON | MODE_RX);
  delay(20);
}

}  // namespace acurite
}  // namespace esphome
