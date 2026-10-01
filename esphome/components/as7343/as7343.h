#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome::as7343 {

// ams-OSRAM AS7343 14-channel spectral sensor, 18-channel auto-SMUX mode.
// Register map from Adafruit's CircuitPython driver (adafruit_as7343 1.0.3).
// Publishes "basic counts" (raw / (gain * t_int_ms)) so readings stay
// comparable while the auto-gain moves.
class AS7343Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_timing(uint8_t atime, uint16_t astep) {
    this->atime_ = atime;
    this->astep_ = astep;
  }
  void set_par_factor(float f) { this->par_factor_ = f; }
  void set_channel_sensor(uint8_t index, sensor::Sensor *s) { this->channels_[index] = s; }
  void set_clear_sensor(sensor::Sensor *s) { this->clear_sensor_ = s; }
  void set_par_sensor(sensor::Sensor *s) { this->par_sensor_ = s; }

 protected:
  bool set_bits_(uint8_t reg, uint8_t mask, uint8_t value);
  bool write_gain_();
  void poll_(uint8_t attempt);
  void finish_();

  uint8_t atime_{29};
  uint16_t astep_{599};
  uint8_t gain_code_{3};  // 4x to start; auto-ranged afterwards
  float par_factor_{1.0f};
  bool measuring_{false};
  sensor::Sensor *channels_[18]{};
  sensor::Sensor *clear_sensor_{nullptr};
  sensor::Sensor *par_sensor_{nullptr};
};

}  // namespace esphome::as7343
