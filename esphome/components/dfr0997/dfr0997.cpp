#include "dfr0997.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include <algorithm>

namespace esphome::dfr0997 {

static const char *const TAG = "dfr0997";

static const uint8_t CMD_DRAW_TEXT = 0x18;
static const uint8_t CMD_SET_BACKGROUND_COLOR = 0x19;
static const uint8_t CMD_CLEAN_SCREEN = 0x1D;
static const size_t CHUNK = 32;          // library's max bytes per I2C transaction
static const uint32_t GAP_MS = 50;       // library's delay after every chunk
static const uint32_t BOOT_MS = 3000;    // let the display firmware boot first
static const size_t MAX_TEXT = 242;      // library's limit (frame length is one byte)

std::vector<uint8_t> DFR0997Component::make_frame_(uint8_t cmd, const std::vector<uint8_t> &payload) {
  std::vector<uint8_t> f;
  f.reserve(payload.size() + 4);
  f.push_back(0x55);
  f.push_back(0xAA);
  f.push_back(uint8_t(payload.size() + 1));  // total length - 3
  f.push_back(cmd);
  f.insert(f.end(), payload.begin(), payload.end());
  return f;
}

void DFR0997Component::queue_text_(uint8_t id, const Line &line) {
  std::vector<uint8_t> p = {id,
                            line.size,
                            uint8_t(line.color >> 16),
                            uint8_t(line.color >> 8),
                            uint8_t(line.color),
                            uint8_t(line.x >> 8),
                            uint8_t(line.x),
                            uint8_t(line.y >> 8),
                            uint8_t(line.y)};
  // An empty string would leave the old text up; a space blanks the line.
  std::string text = line.text.empty() ? std::string(" ") : line.text.substr(0, MAX_TEXT);
  p.insert(p.end(), text.begin(), text.end());
  this->tx_.push_back(Frame{make_frame_(CMD_DRAW_TEXT, p), GAP_MS});
}

void DFR0997Component::setup() {
  this->next_tx_ms_ = millis() + BOOT_MS;
  this->tx_.push_back(Frame{make_frame_(CMD_CLEAN_SCREEN, {}), 1500});  // library waits 1.5 s
  this->tx_.push_back(Frame{make_frame_(CMD_SET_BACKGROUND_COLOR, {uint8_t(this->background_ >> 16),
                                                                     uint8_t(this->background_ >> 8),
                                                                     uint8_t(this->background_)}),
                            300});  // library waits 300 ms
}

void DFR0997Component::update() {
  for (auto &line : this->lines_) {
    std::string t = line.text_fn();
    if (t != line.text || !line.shown) {
      line.text = std::move(t);
      line.dirty = true;
    }
  }
}

void DFR0997Component::loop() {
  uint32_t now = millis();
  if (int32_t(now - this->next_tx_ms_) < 0)
    return;
  if (this->tx_.empty()) {
    // One changed line at a time, so a burst of updates never piles up.
    for (size_t i = 0; i < this->lines_.size(); i++) {
      if (this->lines_[i].dirty) {
        this->lines_[i].dirty = false;
        this->lines_[i].shown = true;
        this->queue_text_(uint8_t(i + 1), this->lines_[i]);
        break;
      }
    }
    if (this->tx_.empty())
      return;
  }
  Frame &f = this->tx_.front();
  size_t n = std::min(CHUNK, f.data.size() - this->tx_offset_);
  bool ok = this->write(f.data.data() + this->tx_offset_, n) == i2c::ERROR_OK;
  if (ok != this->last_write_ok_) {
    if (ok) {
      this->status_clear_warning();
    } else {
      ESP_LOGW(TAG, "Write failed; display unplugged or unpowered?");
      this->status_set_warning();
    }
    this->last_write_ok_ = ok;
  }
  if (!ok) {
    // Retry this frame from its start in 1 s, and redraw every line once the
    // display answers again (it may have power-cycled and lost its screen).
    for (auto &line : this->lines_)
      line.shown = false;
    this->tx_offset_ = 0;
    this->next_tx_ms_ = now + 1000;
    return;
  }
  this->tx_offset_ += n;
  if (this->tx_offset_ >= f.data.size()) {
    this->next_tx_ms_ = now + std::max(GAP_MS, f.post_delay_ms);
    this->tx_.pop_front();
    this->tx_offset_ = 0;
  } else {
    this->next_tx_ms_ = now + GAP_MS;
  }
}

void DFR0997Component::dump_config() {
  ESP_LOGCONFIG(TAG, "DFR0997 display:\n  Lines: %u", unsigned(this->lines_.size()));
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esphome::dfr0997
