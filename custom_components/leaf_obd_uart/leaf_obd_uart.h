#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h" // Добавлен text_sensor
#include "esphome/components/binary_sensor/binary_sensor.h" // Добавлен binary_sensor

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
  //bool isCarResponsive();
  float isCarResponsive(); // Возвращает напряжение (>0.0f) или 0.0f в случае ошибки/отсутствия данных
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
  void set_atrv_sensor(sensor::Sensor *atrv) { this->atrv_ = atrv; }
  void set_soh_sensor(sensor::Sensor *soh) { this->soh_ = soh; }
  void set_ahr_sensor(sensor::Sensor *ahr) { this->ahr_ = ahr; }
  void set_odometer_sensor(sensor::Sensor *odometer) { this->odometer_ = odometer; }


    // Новые
    void set_bat_12v_voltage_sensor(sensor::Sensor *bat_12v_voltage) { this->bat_12v_voltage_ = bat_12v_voltage; }
    void set_bat_12v_current_sensor(sensor::Sensor *bat_12v_current) { this->bat_12v_current_ = bat_12v_current; }
    void set_quick_charges_sensor(sensor::Sensor *quick_charges) { this->quick_charges_ = quick_charges; }
    void set_l1_l2_charges_sensor(sensor::Sensor *l1_l2_charges) { this->l1_l2_charges_ = l1_l2_charges; }
    void set_ambient_temp_sensor(sensor::Sensor *ambient_temp) { this->ambient_temp_ = ambient_temp; }
    void set_estimated_ac_power_sensor(sensor::Sensor *estimated_ac_power) { this->estimated_ac_power_ = estimated_ac_power; }
    void set_aux_power_sensor(sensor::Sensor *aux_power) { this->aux_power_ = aux_power; }
    void set_ac_power_sensor(sensor::Sensor *ac_power) { this->ac_power_ = ac_power; }
    // Новый вариант для бинарного сенсора:
    void set_power_switch_sensor(binary_sensor::BinarySensor *power_switch) { this->power_switch_ = power_switch; }
    void set_battery_temp_1_sensor(sensor::Sensor *battery_temp_1) { this->battery_temp_1_ = battery_temp_1; }
    void set_battery_temp_2_sensor(sensor::Sensor *battery_temp_2) { this->battery_temp_2_ = battery_temp_2; }
    void set_battery_temp_3_sensor(sensor::Sensor *battery_temp_3) { this->battery_temp_3_ = battery_temp_3; }
    void set_battery_temp_4_sensor(sensor::Sensor *battery_temp_4) { this->battery_temp_4_ = battery_temp_4; }
    // ... и так далее
    // --- Сеттеры для текстовых сенсоров ---
    void set_plug_state_sensor(text_sensor::TextSensor *plug_state) { this->plug_state_ = plug_state; }
    void set_charge_mode_sensor(text_sensor::TextSensor *charge_mode) { this->charge_mode_ = charge_mode; }



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
  sensor::Sensor *atrv_{nullptr};
  sensor::Sensor *soh_{nullptr};
  sensor::Sensor *ahr_{nullptr};
  sensor::Sensor *odometer_{nullptr};


    // Новые
    sensor::Sensor *bat_12v_voltage_{nullptr};
    sensor::Sensor *bat_12v_current_{nullptr};
    sensor::Sensor *quick_charges_{nullptr};
    sensor::Sensor *l1_l2_charges_{nullptr};
    sensor::Sensor *ambient_temp_{nullptr};
    sensor::Sensor *estimated_ac_power_{nullptr};
    sensor::Sensor *aux_power_{nullptr};
    sensor::Sensor *ac_power_{nullptr};
    sensor::Sensor *battery_temp_1_{nullptr};
    sensor::Sensor *battery_temp_2_{nullptr};
    sensor::Sensor *battery_temp_3_{nullptr};
    sensor::Sensor *battery_temp_4_{nullptr};
    // ... и так далее
    // --- Указатели на бинарные сенсоры ---
    binary_sensor::BinarySensor *power_switch_{nullptr}; // Добавьте эту

    // --- Указатели на текстовые сенсоры ---
    text_sensor::TextSensor *plug_state_{nullptr};
    text_sensor::TextSensor *charge_mode_{nullptr};


  ELM327 elm_;
};

} // namespace leaf_obd_uart
} // namespace esphome