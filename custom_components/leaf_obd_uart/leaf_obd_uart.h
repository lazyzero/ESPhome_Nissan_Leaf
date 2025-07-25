#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace leaf_obd_uart {

enum ELM327Status { DISCONNECTED = 0, OBD_CONNECTED = 1, CAR_CONNECTED = 2 };

class UARTStreamAdapter : public Stream {
 public:
  explicit UARTStreamAdapter(uart::UARTComponent *parent) : parent_(parent) {}
  int available() override { return parent_->available(); }
  int read() override {
    uint8_t byte;
    return parent_->read_byte(&byte) ? byte : -1;
  }
  int peek() override {
    uint8_t byte;
    return parent_->peek_byte(&byte) ? byte : -1;
  }
  size_t write(uint8_t data) override {
    parent_->write_byte(data);
    return 1;
  }
  size_t write(const uint8_t *data, size_t size) override {
    parent_->write_array(data, size);
    return size;
  }
  void flush() override { parent_->flush(); }

 protected:
  uart::UARTComponent *parent_;
};

class ELM327 {
 public:
  bool begin(Stream &stream);
  bool queryUDS(const char *ecu, const char *pid, int retries = 3);
  bool setECU(const char *ecu);
  bool connected();
  bool isCarResponsive();
  ELM327Status get_status() { return status_; }
  void set_status(ELM327Status new_status) { this->status_ = new_status; }
  const char* get_response_buffer() const { return response_buffer_; }

 protected:
  bool sendCommand(const char *cmd);
  bool readResponse(uint32_t timeout_ms = 500);
  Stream *elm_stream_{nullptr};
  bool is_connected_{false};
  ELM327Status status_{DISCONNECTED};
  char response_buffer_[1024]; // Увеличено для многофреймовых ответов
};

class LeafObdComponent : public PollingComponent {
 public:
  explicit LeafObdComponent(uint32_t update_interval = 30000) : PollingComponent(update_interval) {}
  void setup() override;
  void update() override;
  void dump_config() override;
  void set_uart_parent(uart::UARTComponent *parent) {
    this->parent_ = parent;
    this->stream_ = new UARTStreamAdapter(parent);
  }
  void set_soc_sensor(sensor::Sensor *soc) { this->soc_ = soc; }
  void set_hv_sensor(sensor::Sensor *hv) { this->hv_ = hv; }
  void set_temp_sensor(sensor::Sensor *temp) { this->temp_ = temp; }
  void set_soh_sensor(sensor::Sensor *soh) { this->soh_ = soh; }
  void set_ahr_sensor(sensor::Sensor *ahr) { this->ahr_ = ahr; }
  void set_odometer_sensor(sensor::Sensor *odometer) { this->odometer_ = odometer; }
  void flush_uart() {
    if (this->parent_) {
      uint8_t byte;
      while (this->parent_->available()) {
        this->parent_->read_byte(&byte);
      }
      this->parent_->flush();
    }
  }

 protected:
  void publish_nan();
  uart::UARTComponent *parent_{nullptr};
  UARTStreamAdapter *stream_{nullptr};
  sensor::Sensor *soc_{nullptr};
  sensor::Sensor *hv_{nullptr};
  sensor::Sensor *temp_{nullptr};
  sensor::Sensor *soh_{nullptr};
  sensor::Sensor *ahr_{nullptr};
  sensor::Sensor *odometer_{nullptr};
  ELM327 elm_;
};

} // namespace leaf_obd_uart
} // namespace esphome