#include "mlx90632.h"
#include "esphome/core/log.h"

#include <cmath>

namespace esphome::mlx90632 {

static const char *const TAG = "mlx90632";

static const uint16_t REG_EE_PRODUCT_CODE = 0x2409;
static const uint16_t REG_EE_P_R = 0x240C;
static const uint16_t REG_EE_P_G = 0x240E;
static const uint16_t REG_EE_P_T = 0x2410;
static const uint16_t REG_EE_P_O = 0x2412;
static const uint16_t REG_EE_EA = 0x2424;
static const uint16_t REG_EE_EB = 0x2426;
static const uint16_t REG_EE_FA = 0x2428;
static const uint16_t REG_EE_FB = 0x242A;
static const uint16_t REG_EE_GA = 0x242C;
static const uint16_t REG_EE_GB = 0x242E;
static const uint16_t REG_EE_KA = 0x242F;
static const uint16_t REG_EE_HA = 0x2481;
static const uint16_t REG_EE_HB = 0x2482;
static const uint16_t REG_CONTROL = 0x3001;
static const uint16_t REG_STATUS = 0x3FFF;
static const uint16_t REG_RAM_4 = 0x4003;  // RAM_n = 0x4000 + n - 1

static const uint16_t STATUS_NEW_DATA = 1 << 0;
static const uint16_t CONTROL_MODE_MASK = 0x3 << 1;
static const uint16_t CONTROL_MODE_CONTINUOUS = 0x3 << 1;
static const uint16_t CONTROL_MEAS_SELECT_MASK = 0x1F << 4;  // 0 = medical

bool MLX90632Component::read16_(uint16_t reg, uint16_t *value) {
  uint8_t buf[2];
  if (this->read_register16(reg, buf, 2) != i2c::ERROR_OK)
    return false;
  *value = (uint16_t(buf[0]) << 8) | buf[1];
  return true;
}

bool MLX90632Component::read16s_(uint16_t reg, int16_t *value) {
  uint16_t raw;
  if (!this->read16_(reg, &raw))
    return false;
  *value = static_cast<int16_t>(raw);
  return true;
}

bool MLX90632Component::read32s_(uint16_t lsw_reg, int32_t *value) {
  uint16_t lsw, msw;
  if (!this->read16_(lsw_reg, &lsw) || !this->read16_(lsw_reg + 1, &msw))
    return false;
  *value = static_cast<int32_t>((uint32_t(msw) << 16) | lsw);
  return true;
}

bool MLX90632Component::write16_(uint16_t reg, uint16_t value) {
  uint8_t buf[2] = {uint8_t(value >> 8), uint8_t(value)};
  return this->write_register16(reg, buf, 2) == i2c::ERROR_OK;
}

bool MLX90632Component::load_calibration_() {
  int32_t p_r, p_g, p_t, p_o, ea, eb, fa, fb, ga;
  int16_t gb, ka, ha, hb;
  if (!this->read32s_(REG_EE_P_R, &p_r) || !this->read32s_(REG_EE_P_G, &p_g) ||
      !this->read32s_(REG_EE_P_T, &p_t) || !this->read32s_(REG_EE_P_O, &p_o) ||
      !this->read32s_(REG_EE_EA, &ea) || !this->read32s_(REG_EE_EB, &eb) ||
      !this->read32s_(REG_EE_FA, &fa) || !this->read32s_(REG_EE_FB, &fb) ||
      !this->read32s_(REG_EE_GA, &ga) || !this->read16s_(REG_EE_GB, &gb) ||
      !this->read16s_(REG_EE_KA, &ka) || !this->read16s_(REG_EE_HA, &ha) ||
      !this->read16s_(REG_EE_HB, &hb))
    return false;
  this->p_r_ = std::ldexp(double(p_r), -8);
  this->p_g_ = std::ldexp(double(p_g), -20);
  this->p_t_ = std::ldexp(double(p_t), -44);
  this->p_o_ = std::ldexp(double(p_o), -8);
  this->ea_ = std::ldexp(double(ea), -16);
  this->eb_ = std::ldexp(double(eb), -8);
  this->fa_ = std::ldexp(double(fa), -46);
  this->fb_ = std::ldexp(double(fb), -36);
  this->ga_ = std::ldexp(double(ga), -36);
  this->gb_ = std::ldexp(double(gb), -10);
  this->ka_ = std::ldexp(double(ka), -10);
  this->ha_ = std::ldexp(double(ha), -14);
  this->hb_ = std::ldexp(double(hb), -10);
  return true;
}

void MLX90632Component::setup() {
  if (!this->read16_(REG_EE_PRODUCT_CODE, &this->product_code_) || this->product_code_ == 0x0000 ||
      this->product_code_ == 0xFFFF) {
    ESP_LOGE(TAG, "No MLX90632 at 0x%02X", this->address_);
    this->mark_failed();
    return;
  }
  if (!this->load_calibration_()) {
    ESP_LOGE(TAG, "Reading calibration EEPROM failed");
    this->mark_failed();
    return;
  }
  // Continuous, medical mode. CONTROL is a RAM register; EEPROM is not touched.
  uint16_t control;
  if (!this->read16_(REG_CONTROL, &control)) {
    this->mark_failed();
    return;
  }
  control = (control & ~(CONTROL_MODE_MASK | CONTROL_MEAS_SELECT_MASK)) | CONTROL_MODE_CONTINUOUS;
  if (!this->write16_(REG_CONTROL, control)) {
    this->mark_failed();
    return;
  }
}

void MLX90632Component::update() {
  uint16_t status;
  if (!this->read16_(REG_STATUS, &status)) {
    this->status_set_warning();
    return;
  }
  if (!(status & STATUS_NEW_DATA))
    return;  // next poll; the sensor refreshes at its EEPROM rate (2 Hz default)

  int16_t ram[6];  // RAM_4 .. RAM_9
  for (int i = 0; i < 6; i++) {
    if (!this->read16s_(REG_RAM_4 + i, &ram[i])) {
      this->status_set_warning();
      return;
    }
  }
  this->write16_(REG_STATUS, status & ~STATUS_NEW_DATA);
  this->status_clear_warning();

  const double ram6 = ram[2], ram9 = ram[5];
  // Ambient
  double vr_ta = ram9 + this->gb_ * (ram6 / 12.0);
  double amb = (ram6 / 12.0) / vr_ta * 524288.0;  // 2^19
  double amb_d = amb - this->p_r_;
  double ta = this->p_o_ + amb_d / this->p_g_ + this->p_t_ * amb_d * amb_d;
  if (this->ambient_sensor_ != nullptr)
    this->ambient_sensor_->publish_state(ta);

  if (this->object_sensor_ == nullptr)
    return;
  // Medical mode alternates two chopper phases (RAM_4/5 and RAM_7/8). Melexis'
  // reference library averages both phases, which also removes any dependence
  // on which phase the status cycle-position field points at.
  double s = (double(ram[0]) + ram[1] + ram[3] + ram[4]) / 4.0;
  double vr_to = ram9 + this->ka_ * (ram6 / 12.0);
  double sto = (s / 12.0) / vr_to * 524288.0;
  double ta_dut = (amb - this->eb_) / this->ea_ + 25.0;
  double tak4 = std::pow(ta_dut + 273.15, 4);
  double to = 25.0;
  for (int i = 0; i < 5; i++) {
    double denom = this->emissivity_ * this->fa_ * this->ha_ *
                   (1.0 + this->ga_ * (to - 25.0) + this->fb_ * (ta_dut - 25.0));
    double t4 = sto / denom + tak4;
    if (!(t4 > 0)) {
      ESP_LOGW(TAG, "Object temperature out of range");
      return;
    }
    to = std::pow(t4, 0.25) - 273.15 - this->hb_;
  }
  this->object_sensor_->publish_state(to);
}

void MLX90632Component::dump_config() {
  ESP_LOGCONFIG(TAG, "MLX90632:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication failed");
    return;
  }
  ESP_LOGCONFIG(TAG, "  Product code: 0x%04X\n  Emissivity: %.2f", this->product_code_, this->emissivity_);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Ambient", this->ambient_sensor_);
  LOG_SENSOR("  ", "Object", this->object_sensor_);
}

}  // namespace esphome::mlx90632
