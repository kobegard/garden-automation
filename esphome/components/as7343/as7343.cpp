#include "as7343.h"
#include "esphome/core/log.h"

#include <algorithm>
#include <cmath>

namespace esphome::as7343 {

static const char *const TAG = "as7343";

static const uint8_t REG_ID = 0x5A;  // bank 1
static const uint8_t CHIP_ID = 0x81;
static const uint8_t REG_ENABLE = 0x80;
static const uint8_t REG_ATIME = 0x81;
static const uint8_t REG_STATUS2 = 0x90;
static const uint8_t REG_STATUS = 0x93;
static const uint8_t REG_ASTATUS = 0x94;
static const uint8_t REG_DATA_0_L = 0x95;
static const uint8_t REG_CFG0 = 0xBF;
static const uint8_t REG_CFG1 = 0xC6;
static const uint8_t REG_LED = 0xCD;
static const uint8_t REG_ASTEP_L = 0xD4;
static const uint8_t REG_CFG20 = 0xD6;

static const uint8_t ENABLE_PON = 1 << 0;
static const uint8_t ENABLE_SP_EN = 1 << 1;
static const uint8_t STATUS2_AVALID = 1 << 6;
static const uint8_t STATUS2_SAT = (1 << 4) | (1 << 3);  // digital | analog
static const uint8_t CFG0_REG_BANK = 1 << 4;
static const uint8_t GAIN_CODE_MAX = 12;  // 2048x

// Channels inside 400-700 nm (indices into the 18-value block).
static const uint8_t PAR_CHANNELS[] = {12, 6, 0, 7, 8, 15, 1, 2, 9, 13};
// Clear/VIS readings, one pair per SMUX cycle.
static const uint8_t VIS_CHANNELS[] = {4, 5, 10, 11, 16, 17};

static float gain_of(uint8_t code) { return code == 0 ? 0.5f : float(1u << (code - 1)); }

bool AS7343Component::set_bits_(uint8_t reg, uint8_t mask, uint8_t value) {
  uint8_t v;
  if (!this->read_byte(reg, &v))
    return false;
  return this->write_byte(reg, (v & ~mask) | (value & mask));
}

bool AS7343Component::write_gain_() { return this->set_bits_(REG_CFG1, 0x1F, this->gain_code_); }

void AS7343Component::setup() {
  uint8_t id = 0;
  if (!this->set_bits_(REG_CFG0, CFG0_REG_BANK, CFG0_REG_BANK) || !this->read_byte(REG_ID, &id) ||
      !this->set_bits_(REG_CFG0, CFG0_REG_BANK, 0) || id != CHIP_ID) {
    ESP_LOGE(TAG, "No AS7343 at 0x%02X (id 0x%02X)", this->address_, id);
    this->mark_failed();
    return;
  }
  uint8_t astep[2] = {uint8_t(this->astep_), uint8_t(this->astep_ >> 8)};
  if (!this->write_byte(REG_ENABLE, ENABLE_PON) || !this->write_byte(REG_ATIME, this->atime_) ||
      !this->write_bytes(REG_ASTEP_L, astep, 2) || !this->write_gain_() ||
      !this->set_bits_(REG_CFG20, 0x3 << 5, 0x3 << 5) ||  // auto-SMUX, 18 channels
      !this->set_bits_(REG_LED, 1 << 7, 0)) {               // on-board LED off
    this->mark_failed();
  }
}

void AS7343Component::update() {
  if (this->measuring_)
    return;
  uint8_t st;
  // Stop, clear status, start one measurement (all three SMUX cycles).
  if (!this->write_byte(REG_ENABLE, ENABLE_PON) || !this->read_byte(REG_STATUS, &st) ||
      !this->write_byte(REG_STATUS, st) || !this->read_byte(REG_ASTATUS, &st) ||
      !this->write_byte(REG_ENABLE, ENABLE_PON | ENABLE_SP_EN)) {
    this->status_set_warning();
    return;
  }
  this->measuring_ = true;
  float t_int_ms = (this->atime_ + 1) * (this->astep_ + 1) * 0.00278f;
  this->set_timeout("read", uint32_t(t_int_ms * 3) + 30, [this]() { this->poll_(0); });
}

void AS7343Component::poll_(uint8_t attempt) {
  uint8_t s2;
  if (this->read_byte(REG_STATUS2, &s2) && (s2 & STATUS2_AVALID)) {
    this->finish_();
    return;
  }
  if (attempt >= 20) {
    ESP_LOGW(TAG, "Timed out waiting for data");
    this->write_byte(REG_ENABLE, ENABLE_PON);
    this->measuring_ = false;
    this->status_set_warning();
    return;
  }
  this->set_timeout("read", 20, [this, attempt]() { this->poll_(attempt + 1); });
}

void AS7343Component::finish_() {
  this->measuring_ = false;
  uint8_t s2 = 0, astatus;
  uint8_t raw[36];
  bool ok = this->read_byte(REG_STATUS2, &s2) && this->read_byte(REG_ASTATUS, &astatus) &&
            this->read_bytes(REG_DATA_0_L, raw, sizeof(raw));
  this->write_byte(REG_ENABLE, ENABLE_PON);
  if (!ok) {
    this->status_set_warning();
    return;
  }
  this->status_clear_warning();

  uint16_t counts[18];
  uint16_t max_count = 0;
  for (int i = 0; i < 18; i++) {
    counts[i] = uint16_t(raw[2 * i]) | (uint16_t(raw[2 * i + 1]) << 8);
    max_count = std::max(max_count, counts[i]);
  }
  uint32_t full_scale = std::min<uint32_t>(65535u, uint32_t(this->atime_ + 1) * (this->astep_ + 1));

  // Auto-gain: saturated -> step down and drop this sample; dim -> step up.
  if ((s2 & STATUS2_SAT) || max_count > full_scale * 0.9f) {
    if (this->gain_code_ > 0) {
      this->gain_code_--;
      this->write_gain_();
      ESP_LOGD(TAG, "Saturated, gain -> %.1fx", gain_of(this->gain_code_));
      return;
    }
    ESP_LOGW(TAG, "Saturated at minimum gain; shorten atime/astep");
  }
  float gain = gain_of(this->gain_code_);
  float t_int_ms = (this->atime_ + 1) * (this->astep_ + 1) * 0.00278f;
  float scale = 1.0f / (gain * t_int_ms);
  if (max_count < full_scale * 0.1f && this->gain_code_ < GAIN_CODE_MAX) {
    this->gain_code_++;
    this->write_gain_();
    ESP_LOGD(TAG, "Dim, gain -> %.1fx for next sample", gain_of(this->gain_code_));
  }

  for (int i = 0; i < 18; i++) {
    if (this->channels_[i] != nullptr)
      this->channels_[i]->publish_state(counts[i] * scale);
  }
  if (this->clear_sensor_ != nullptr) {
    float sum = 0;
    for (uint8_t i : VIS_CHANNELS)
      sum += counts[i];
    this->clear_sensor_->publish_state(sum / 6.0f * scale);
  }
  if (this->par_sensor_ != nullptr) {
    float sum = 0;
    for (uint8_t i : PAR_CHANNELS)
      sum += counts[i];
    this->par_sensor_->publish_state(sum * scale * this->par_factor_);
  }
}

void AS7343Component::dump_config() {
  ESP_LOGCONFIG(TAG, "AS7343:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication failed");
    return;
  }
  ESP_LOGCONFIG(TAG, "  Integration: %.1f ms\n  PAR factor: %.4f (uncalibrated if 1.0)",
                (this->atime_ + 1) * (this->astep_ + 1) * 0.00278f, this->par_factor_);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "PAR", this->par_sensor_);
  LOG_SENSOR("  ", "Clear", this->clear_sensor_);
}

}  // namespace esphome::as7343
