#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome::mlx90632 {

// Melexis MLX90632 FIR thermometer, medical (default) measurement mode.
// Register map and EEPROM scaling from Adafruit's CircuitPython driver
// (adafruit_mlx90632 1.0.4). Object math follows Melexis' reference library:
// both chopper phases averaged, TO0 = TA0 = 25 C, TOdut iterated 5 times.
class MLX90632Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_ambient_sensor(sensor::Sensor *s) { this->ambient_sensor_ = s; }
  void set_object_sensor(sensor::Sensor *s) { this->object_sensor_ = s; }
  void set_emissivity(float e) { this->emissivity_ = e; }

 protected:
  bool read16_(uint16_t reg, uint16_t *value);
  bool read16s_(uint16_t reg, int16_t *value);
  bool read32s_(uint16_t lsw_reg, int32_t *value);
  bool write16_(uint16_t reg, uint16_t value);
  bool load_calibration_();

  sensor::Sensor *ambient_sensor_{nullptr};
  sensor::Sensor *object_sensor_{nullptr};
  float emissivity_{0.95f};

  // Calibration constants, already scaled.
  double p_r_, p_g_, p_t_, p_o_, ea_, eb_, fa_, fb_, ga_, gb_, ka_, ha_, hb_;
  uint16_t product_code_{0};
};

}  // namespace esphome::mlx90632
