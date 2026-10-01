#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace esphome::dfr0997 {

// DFRobot DFR0997 Gravity 2.0" 320x240 IPS "serial" display, I2C mode.
// Frame format and timing from DFRobot's DFRobot_LcdDisplay 2.0.0 library:
//   55 AA <len = total-3> <cmd> <payload...>, written in <=32-byte I2C
//   transactions with 50 ms between them. Text: cmd 0x18, payload
//   id, size, R, G, B, x(BE16), y(BE16), text. Re-sending an id updates it.
// Everything is queued and sent from loop() so nothing here blocks.
class DFR0997Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_background_color(uint32_t rgb) { this->background_ = rgb; }
  void add_line(uint16_t x, uint16_t y, uint8_t size, uint32_t color, std::function<std::string()> &&fn) {
    this->lines_.push_back(Line{x, y, size, color, std::move(fn), {}, false, false});
  }

 protected:
  struct Line {
    uint16_t x, y;
    uint8_t size;
    uint32_t color;
    std::function<std::string()> text_fn;
    std::string text;
    bool dirty;
    bool shown;
  };
  struct Frame {
    std::vector<uint8_t> data;
    uint32_t post_delay_ms;
  };

  static std::vector<uint8_t> make_frame_(uint8_t cmd, const std::vector<uint8_t> &payload);
  void queue_text_(uint8_t id, const Line &line);

  std::vector<Line> lines_;
  std::deque<Frame> tx_;
  size_t tx_offset_{0};
  uint32_t next_tx_ms_{0};
  uint32_t background_{0};
  bool last_write_ok_{true};
};

}  // namespace esphome::dfr0997
