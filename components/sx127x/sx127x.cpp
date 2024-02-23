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

const uint8_t MODE_OOK   = 0x20;
const uint8_t MODE_LF_ON = 0x08;
const uint8_t MODE_SLEEP = 0x00;
const uint8_t MODE_STDBY = 0x01;
const uint8_t MODE_RX_FS = 0x04;
const uint8_t MODE_RX    = 0x05;

static const char *const TAG = "sx127x";

void SX127X::setFrequency_(uint64_t frequency)
{
  uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
  writeRegister_(REG_FRF_MSB, (uint8_t)((frf >> 16) & 0xFF));
  writeRegister_(REG_FRF_MID, (uint8_t)((frf >> 8) & 0xFF));
  writeRegister_(REG_FRF_LSB, (uint8_t)((frf >> 0) & 0xFF));
}

uint8_t SX127X::readRegister_(uint8_t reg)
{
  return singleTransfer_((uint8_t)reg & 0x7f, 0x00);
}

void SX127X::writeRegister_(uint8_t reg, uint8_t value)
{
  singleTransfer_((uint8_t)reg | 0x80, value);
}

uint8_t SX127X::singleTransfer_(uint8_t address, uint8_t value)
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

void SX127X::sx127x_start() {
  ESP_LOGD(TAG, "SX127X starting");

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
  uint8_t version = readRegister_(REG_VERSION);
  ESP_LOGD(TAG, "SemTech ID: %0x", version);
  if (version != 0x12) {
    mark_failed();
    return;
  }

  // need to sleep before changing some settings
  writeRegister_(REG_OP_MODE, MODE_OOK | MODE_LF_ON | MODE_SLEEP);
  delay(20); 
}

void SX127X::rx_stop() {
  ESP_LOGD(TAG, "SX127X stopping rx");
  
  // disable rx mode and sleep
  writeRegister_(REG_OP_MODE, MODE_OOK | MODE_LF_ON | MODE_STDBY);
  delay(20);
  writeRegister_(REG_OP_MODE, MODE_OOK | MODE_LF_ON | MODE_SLEEP);
  delay(20); 
}

void SX127X::rx_start(uint64_t frequency) {
  ESP_LOGD(TAG, "SX127X starting rx");

  // dio2 is connected to the output of the ook demodulator, 
  // when the signal state is on the gpio will go high and when
  // it is off the gpio will go low, trigger on both edges in 
  // order to detect on duration  
  // dio2_->setup();
  // store.state = sx127x_INIT;
  // store.pin = dio2_->to_isr();
  // dio2_->attach_interrupt(sx127xStore::gpio_intr, &store, gpio::INTERRUPT_ANY_EDGE);


  // set freq
  setFrequency_(frequency);

  // set bw to 50 kHz as this should result in ~20 usecs between samples, 
  // ie a minimum of 20 usecs between ook demodulator / dio2 gpio state
  // changes, which is fast enough to detect the difference in duration 
  // between sx127x ook bits which have a minimum duration of ~200 usecs
  writeRegister_(REG_RX_BW, 0x0B);

  // disable crc check and enable continuous mode
  writeRegister_(REG_PACKET_CONFIG_1, 0x80);
  writeRegister_(REG_PACKET_CONFIG_2, 0x00);

  // disable bit synchronizer and sync generation
  writeRegister_(REG_SYNC_CONFIG, 0x00);
  writeRegister_(REG_OOK_PEAK, 0x08);

  // enable rx mode  
  writeRegister_(REG_OP_MODE, MODE_OOK | MODE_LF_ON | MODE_STDBY);
  delay(20);
  writeRegister_(REG_OP_MODE, MODE_OOK | MODE_LF_ON | MODE_RX_FS);
  delay(20);
  writeRegister_(REG_OP_MODE, MODE_OOK | MODE_LF_ON | MODE_RX);
  delay(20);
}

}  // namespace sx127x
}  // namespace esphome
