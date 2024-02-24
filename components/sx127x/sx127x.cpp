#include "sx127x.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace sx127x {

const uint8_t REG_OP_MODE         = 0x01;
const uint8_t REG_FRF_MSB         = 0x06;
const uint8_t REG_FRF_MID         = 0x07;
const uint8_t REG_FRF_LSB         = 0x08;
const uint8_t REG_RX_BW           = 0x12;
const uint8_t REG_OOK_PEAK        = 0x14;
const uint8_t REG_SYNC_CONFIG     = 0x27;
const uint8_t REG_PACKET_CONFIG_1 = 0x30;
const uint8_t REG_PACKET_CONFIG_2 = 0x31;
const uint8_t REG_IRQ_FLAGS_1     = 0x3e;
const uint8_t REG_IRQ_FLAGS_2     = 0x3f;
const uint8_t REG_VERSION         = 0x42;

const uint8_t MODE_LF_ON = 0x08;
const uint8_t MODE_SLEEP = 0x00;
const uint8_t MODE_STDBY = 0x01;
const uint8_t MODE_RX_FS = 0x04;
const uint8_t MODE_RX    = 0x05;

const uint8_t OOK_TRESHOLD_PEAK = 0x08;

static const char *const TAG = "sx127x";

void SX127X::set_frequency_(uint64_t frequency)
{
  uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
  write_register_(REG_FRF_MSB, (uint8_t)((frf >> 16) & 0xFF));
  write_register_(REG_FRF_MID, (uint8_t)((frf >> 8) & 0xFF));
  write_register_(REG_FRF_LSB, (uint8_t)((frf >> 0) & 0xFF));
}

uint8_t SX127X::read_register_(uint8_t reg)
{
  return single_transfer_((uint8_t)reg & 0x7f, 0x00);
}

void SX127X::write_register_(uint8_t reg, uint8_t value)
{
  single_transfer_((uint8_t)reg | 0x80, value);
}

uint8_t SX127X::single_transfer_(uint8_t address, uint8_t value)
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

void SX127X::sx127x_setup_() {
  ESP_LOGD(TAG, "SX127X setup");

  // setup dio2
  dio2_->setup();

  // setup nss and set high
  nss_->setup();
  nss_->digital_write(true);

  // setup reset and toggle to reset chip
  rst_->setup();
  rst_->digital_write(false);
  delay(1);
  rst_->digital_write(true);
  delay(10);

  // start spi
  spi_setup();

  // check silicon version to make sure hw is ok
  uint8_t version = read_register_(REG_VERSION);
  ESP_LOGD(TAG, "SemTech ID: %0x", version);
  if (version != 0x12) {
    mark_failed();
    return;
  }

  // need to sleep before changing some settings
  write_register_(REG_OP_MODE, MODE_LF_ON | MODE_SLEEP);
  delay(20); 
}

void SX127X::rx_stop_() {
  ESP_LOGD(TAG, "SX127X stopping rx");
  
  // disable rx mode and sleep
  write_register_(REG_OP_MODE, MODE_LF_ON | MODE_STDBY);
  delay(1);
  write_register_(REG_OP_MODE, MODE_LF_ON | MODE_SLEEP);
  delay(1);
}

void SX127X::rx_start_(uint64_t frequency, SX127XRxBw bandwidth, SX127XRxMod modulation) {
  ESP_LOGD(TAG, "SX127X starting rx");

  // write modulation
  write_register_(REG_OP_MODE, modulation | MODE_LF_ON | MODE_SLEEP);
  delay(1);

  // set freq
  set_frequency_(frequency);

  // set the channel bw, this will determine the sample rate, make sure it is 
  // appropriate for the data you wish to decode, if the bandwidth is 10khz 
  // that will result in a sample every 100 usecs, that doesn't mean the data 
  // isr will trigger that often its just how responsive it will be to changes in 
  // the signal
  write_register_(REG_RX_BW, bandwidth);

  // disable packet mode
  write_register_(REG_PACKET_CONFIG_1, 0x00);
  write_register_(REG_PACKET_CONFIG_2, 0x00);

  // disable bit synchronizer and sync generation
  write_register_(REG_SYNC_CONFIG, 0x00);
  write_register_(REG_OOK_PEAK, OOK_TRESHOLD_PEAK);

  // enable rx mode  
  write_register_(REG_OP_MODE, modulation | MODE_LF_ON | MODE_STDBY);
  delay(1);
  write_register_(REG_OP_MODE, modulation | MODE_LF_ON | MODE_RX_FS);
  delay(1);
  write_register_(REG_OP_MODE, modulation | MODE_LF_ON | MODE_RX);
}

}  // namespace sx127x
}  // namespace esphome
