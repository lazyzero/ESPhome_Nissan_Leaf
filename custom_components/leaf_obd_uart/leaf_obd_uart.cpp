#include "leaf_obd_uart.h"
#include <cstdio>
#include <cctype>
#include <cstdlib>

namespace esphome {
namespace leaf_obd_uart {

static const char *const TAG = "leaf_obd_uart";

bool ELM327::begin(Stream &stream) {
  elm_stream_ = &stream;
  is_connected_ = false;
  status_ = DISCONNECTED;
  response_buffer_[0] = '\0';
  ESP_LOGD(TAG, "ELM327 begin called with stream");

  if (!elm_stream_) {
    ESP_LOGE(TAG, "Stream is null!");
    return false;
  }

  while (elm_stream_->available()) {
    elm_stream_->read();
  }
  ESP_LOGD(TAG, "UART buffer cleared before initialization");

  if (!sendCommand("ATZ")) {
    ESP_LOGE(TAG, "Failed to send ATZ command");
    return false;
  }
  delay(750); // Увеличено для надежности на основе логов

  if (!readResponse()) {
    ESP_LOGE(TAG, "No response to ATZ command");
    return false;
  }
  ESP_LOGD(TAG, "Raw ATZ response: %s", response_buffer_);
  if (strstr(response_buffer_, "ELM327") == nullptr) {
    ESP_LOGE(TAG, "Invalid ATZ response: %s", response_buffer_);
    return false;
  }

  const char *init_commands[] = {
      "ATE0", "ATL0", "ATSP6", "ATH1", "ATS0", "ATCAF0",
      "ATFCSD300000", "ATFCSM1"
  };
  for (const char *cmd : init_commands) {
    if (!sendCommand(cmd) || !readResponse()) {
      ESP_LOGE(TAG, "Failed to initialize with command: %s", cmd);
      return false;
    }
    ESP_LOGD(TAG, "Raw %s response: '%s'", cmd, response_buffer_);
    if (strstr(response_buffer_, "OK") == nullptr) {
      ESP_LOGE(TAG, "Invalid response to %s: %s", cmd, response_buffer_);
      return false;
    }
    delay(50);
  }

  if (!sendCommand("ATDP") || !readResponse()) {
    ESP_LOGE(TAG, "Failed to check active protocol");
    return false;
  }
  ESP_LOGD(TAG, "Active protocol: %s", response_buffer_);

  is_connected_ = true;
  status_ = OBD_CONNECTED;
  if (isCarResponsive()) {
    status_ = CAR_CONNECTED;
    ESP_LOGI(TAG, "ELM327 initialized, connected to car");
  } else {
    ESP_LOGW(TAG, "No car response, OBD connected only");
  }
  return true;
}

bool ELM327::isCarResponsive() {
  if (!elm_stream_) return false;

  if (!sendCommand("ATRV")) {
    ESP_LOGW(TAG, "Failed to send ATRV command");
    return false;
  }

  if (!readResponse(500)) {
    ESP_LOGD(TAG, "No response to ATRV. Car is likely off.");
    return false;
  }

  if (strstr(response_buffer_, "?") || strstr(response_buffer_, "NO DATA")) {
    ESP_LOGD(TAG, "ATRV response: '%s'. Car is off.", response_buffer_);
    return false;
  }

  float voltage = atof(response_buffer_);
  if (voltage > 12.0) {
    ESP_LOGD(TAG, "ATRV voltage: %.2fV. Car is on.", voltage);
    return true;
  }
  ESP_LOGD(TAG, "ATRV voltage: %.2fV. Car is off.", voltage);
  return false;
}

bool ELM327::readResponse(uint32_t timeout_ms) {
  if (!elm_stream_) {
    ESP_LOGW(TAG, "Cannot read response: Stream is null");
    return false;
  }

  size_t idx = 0;
  unsigned long start = millis();
  bool multi_frame = false;
  while (millis() - start < timeout_ms) {
    if (elm_stream_->available()) {
      char c = elm_stream_->read();
      ESP_LOGD(TAG, "Received char: %c (0x%02X)", isprint(c) ? c : '.', c);
      if (c == '>') {
        response_buffer_[idx] = '\0';
        while (idx > 0 && (response_buffer_[idx - 1] == '\r' || response_buffer_[idx - 1] == '\n')) {
          response_buffer_[--idx] = '\0';
        }
        ESP_LOGD(TAG, "Response complete: %s", response_buffer_);
        return true;
      }
      if (idx < sizeof(response_buffer_) - 1) {
        response_buffer_[idx++] = c;
        if (!multi_frame && idx >= 3 && strncmp(response_buffer_, "7BB", 3) == 0) {
          multi_frame = true;
          timeout_ms = 2000; // Увеличить тайм-аут для многофреймовых ответов
        }
      } else {
        ESP_LOGW(TAG, "Response buffer overflow at %d bytes", idx);
        response_buffer_[idx] = '\0';
        return false;
      }
    }
    yield();
  }
  response_buffer_[idx] = '\0';
  while (idx > 0 && (response_buffer_[idx - 1] == '\r' || response_buffer_[idx - 1] == '\n')) {
    response_buffer_[--idx] = '\0';
  }
  ESP_LOGW(TAG, "Response timeout after %dms: %s", timeout_ms, response_buffer_);
  return idx > 0;
}

bool ELM327::sendCommand(const char *cmd) {
  if (!elm_stream_) {
    ESP_LOGW(TAG, "Cannot send command: Stream is null");
    return false;
  }
  while (elm_stream_->available()) {
    elm_stream_->read();
  }
  elm_stream_->print(cmd);
  elm_stream_->print("\r");
  elm_stream_->flush();
  ESP_LOGD(TAG, "Command sent: %s", cmd);
  return true;
}

bool ELM327::setECU(const char *ecu) {
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "ATSH%s", ecu);
  if (!sendCommand(cmd) || !readResponse()) {
    ESP_LOGE(TAG, "Failed to set ECU: %s", cmd);
    return false;
  }
  if (strstr(response_buffer_, "OK") == nullptr) {
    ESP_LOGE(TAG, "Invalid response to %s: %s", cmd, response_buffer_);
    return false;
  }
  snprintf(cmd, sizeof(cmd), "ATFCSH%s", ecu);
  if (!sendCommand(cmd) || !readResponse()) {
    ESP_LOGE(TAG, "Failed to set Flow Control: %s", cmd);
    return false;
  }
  if (strstr(response_buffer_, "OK") == nullptr) {
    ESP_LOGE(TAG, "Invalid response to %s: %s", cmd, response_buffer_);
    return false;
  }
  return true;
}

bool ELM327::connected() {
  return is_connected_ && elm_stream_ != nullptr;
}

bool ELM327::queryUDS(const char *ecu, const char *pid, int retries) {
  for (int i = 0; i < retries; i++) {
    response_buffer_[0] = '\0';
    if (!setECU(ecu)) {
      ESP_LOGW(TAG, "Retry %d: Failed to set ECU: %s", i + 1, ecu);
      continue;
    }
    if (!sendCommand(pid) || !readResponse()) {
      ESP_LOGW(TAG, "Retry %d: No response to PID: %s", i + 1, pid);
      continue;
    }
    if (strstr(response_buffer_, "NO DATA") || strstr(response_buffer_, "7F") || strstr(response_buffer_, "CAN ERROR")) {
      ESP_LOGW(TAG, "Retry %d: Invalid response for PID: %s, response: %s", i + 1, pid, response_buffer_);
      continue;
    }
    ESP_LOGD(TAG, "UDS response: %s", response_buffer_);
    return true;
  }
  ESP_LOGE(TAG, "Failed UDS query after %d retries: ECU=%s, PID=%s", retries, ecu, pid);
  return false;
}

void LeafObdComponent::setup() {
  ESP_LOGI(TAG, "Entering LeafObdComponent::setup...");
  if (!parent_ || !stream_) {
    ESP_LOGE(TAG, "UART parent or stream is null!");
    return;
  }
}

void LeafObdComponent::update() {
  static int state = 0;
  ESP_LOGD(TAG, "Updating Leaf OBD data (state=%d)...", state);

  switch (state) {
    case 0: // Проверка состояния автомобиля
      if (!elm_.isCarResponsive()) {
        ESP_LOGI(TAG, "Car is not responsive. Setting DISCONNECTED.");
        elm_.set_status(DISCONNECTED);
        publish_nan();
        state = 0;
        return;
      }
      if (elm_.get_status() == DISCONNECTED) {
        ESP_LOGI(TAG, "ELM is not initialized. Starting setup...");
        flush_uart();
        delay(100);
        if (!elm_.begin(*stream_)) {
          ESP_LOGE(TAG, "ELM327 initialization failed. Retrying later.");
          publish_nan();
          state = 0;
          return;
        }
      }
      state = 1;
      break;

    case 1: // Запрос SOC
      if (soc_ && elm_.connected()) {
        if (elm_.queryUDS("797", "02215D")) { // Заменен PID
          const char* buffer = elm_.get_response_buffer();
          if (strlen(buffer) >= 13 && strstr(buffer, "79A") && !strstr(buffer, "7F")) {
            char soc_hex[5];
            strncpy(soc_hex, &buffer[7], 4);
            soc_hex[4] = '\0';
            int soc_raw = strtol(soc_hex, nullptr, 16);
            if (soc_raw > 0) {
              float soc_val = soc_raw / 40.96; // Упрощено масштабирование
              soc_->publish_state(soc_val);
              ESP_LOGD(TAG, "SOC: %.1f%%", soc_val);
            } else {
              soc_->publish_state(NAN);
              ESP_LOGW(TAG, "Invalid SOC value: %d", soc_raw);
            }
          } else {
            soc_->publish_state(NAN);
            ESP_LOGW(TAG, "Invalid SOC response: %s", buffer);
          }
        } else {
          soc_->publish_state(NAN);
          ESP_LOGW(TAG, "Failed to query SOC");
        }
      }
      state = 2;
      break;

    case 2: // Запрос Voltage, Temp, SOH, AHr
      if (elm_.connected() && elm_.setECU("79B")) {
        if (hv_ && elm_.queryUDS("79B", "022102")) {
          const char* buffer = elm_.get_response_buffer();
          if (strlen(buffer) >= 13 && strstr(buffer, "7BB") && !strstr(buffer, "7F")) {
            char voltage_hex[5];
            strncpy(voltage_hex, &buffer[9], 4);
            voltage_hex[4] = '\0';
            int voltage_raw = strtol(voltage_hex, nullptr, 16);
            if (voltage_raw > 0) {
              float hv_val = voltage_raw / 10.0; // Уточнено масштабирование
              hv_->publish_state(hv_val);
              ESP_LOGD(TAG, "Voltage: %.2fV", hv_val);
            } else {
              hv_->publish_state(NAN);
              ESP_LOGW(TAG, "Invalid Voltage value: %d", voltage_raw);
            }
          } else {
            hv_->publish_state(NAN);
            ESP_LOGW(TAG, "Invalid Voltage response: %s", buffer);
          }
        }

        if (temp_ && elm_.queryUDS("79B", "022104")) {
          const char* buffer = elm_.get_response_buffer();
          if (strlen(buffer) >= 15 && strstr(buffer, "7BB") && !strstr(buffer, "7F")) {
            char temp_hex[3];
            strncpy(temp_hex, &buffer[9], 2);
            temp_hex[2] = '\0';
            int temp_raw = strtol(temp_hex, nullptr, 16);
            if (temp_raw > 0) {
              float temp_val = (temp_raw - 40) / 2.0; // Уточнено масштабирование
              temp_->publish_state(temp_val);
              ESP_LOGD(TAG, "Temperature: %.1f°C", temp_val);
            } else {
              temp_->publish_state(NAN);
              ESP_LOGW(TAG, "Invalid Temp value: %d", temp_raw);
            }
          } else {
            temp_->publish_state(NAN);
            ESP_LOGW(TAG, "Invalid Temp response: %s", buffer);
          }
        }

        if ((soh_ || ahr_) && elm_.queryUDS("79B", "022101")) {
          const char* buffer = elm_.get_response_buffer();
          if (strlen(buffer) >= 15 && strstr(buffer, "7BB") && !strstr(buffer, "7F")) {
            if (soh_) {
              char soh_hex[3];
              strncpy(soh_hex, &buffer[7], 2);
              soh_hex[2] = '\0';
              int soh_raw = strtol(soh_hex, nullptr, 16);
              if (soh_raw > 0) {
                float soh_val = soh_raw / 2.0; // Уточнено масштабирование
                soh_->publish_state(soh_val);
                ESP_LOGD(TAG, "SOH: %.1f%%", soh_val);
              } else {
                soh_->publish_state(NAN);
                ESP_LOGW(TAG, "Invalid SOH value: %d", soh_raw);
              }
            }
            if (ahr_) {
              char ahr_hex[4];
              strncpy(ahr_hex, &buffer[11], 3);
              ahr_hex[3] = '\0';
              int ahr_raw = strtol(ahr_hex, nullptr, 16);
              if (ahr_raw > 0) {
                float ahr_val = ahr_raw / 10.0; // Уточнено масштабирование
                ahr_->publish_state(ahr_val);
                ESP_LOGD(TAG, "AHr: %.2f Ah", ahr_val);
              } else {
                ahr_->publish_state(NAN);
                ESP_LOGW(TAG, "Invalid AHr value: %d", ahr_raw);
              }
            }
          } else {
            if (soh_) soh_->publish_state(NAN);
            if (ahr_) ahr_->publish_state(NAN);
            ESP_LOGW(TAG, "Invalid SOH/AHr response: %s", buffer);
          }
        }
      }
      state = 3;
      break;

    case 3: // Запрос Odometer
      if (odometer_ && elm_.connected()) {
        if (elm_.queryUDS("744", "022110")) {
          const char* buffer = elm_.get_response_buffer();
          if (strlen(buffer) >= 15 && strstr(buffer, "764") && !strstr(buffer, "7F")) {
            char odo_hex[5];
            strncpy(odo_hex, &buffer[9], 4);
            odo_hex[4] = '\0';
            int odo_raw = strtol(odo_hex, nullptr, 16);
            if (odo_raw > 0) {
              float odometer_val = odo_raw * 10.0; // Уточнено масштабирование
              odometer_->publish_state(odometer_val);
              ESP_LOGD(TAG, "Odometer: %.0f km", odometer_val);
            } else {
              odometer_->publish_state(NAN);
              ESP_LOGW(TAG, "Invalid Odometer value: %d", odo_raw);
            }
          } else {
            odometer_->publish_state(NAN);
            ESP_LOGW(TAG, "Invalid Odometer response: %s", buffer);
          }
        } else {
          odometer_->publish_state(NAN);
          ESP_LOGW(TAG, "Failed to query Odometer");
        }
      }
      state = 0;
      break;
  }
}

void LeafObdComponent::publish_nan() {
  if (soc_) soc_->publish_state(NAN);
  if (hv_) hv_->publish_state(NAN);
  if (temp_) temp_->publish_state(NAN);
  if (soh_) soh_->publish_state(NAN);
  if (ahr_) ahr_->publish_state(NAN);
  if (odometer_) odometer_->publish_state(NAN);
}

void LeafObdComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Leaf OBD UART:");
  ESP_LOGCONFIG(TAG, "  UART Parent: %s", parent_ ? "set" : "not set");
  ESP_LOGCONFIG(TAG, "  Stream: %s", stream_ ? "set" : "not set");
  ESP_LOGCONFIG(TAG, "  ELM327 Status: %s",
                elm_.get_status() == DISCONNECTED ? "DISCONNECTED" :
                elm_.get_status() == OBD_CONNECTED ? "OBD_CONNECTED" :
                "CAR_CONNECTED");
  ESP_LOGCONFIG(TAG, "  Update Interval: %u ms", PollingComponent::get_update_interval());
  ESP_LOGCONFIG(TAG, "  SOC Sensor: %s", soc_ ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  HV Sensor: %s", hv_ ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  Temp Sensor: %s", temp_ ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  SOH Sensor: %s", soh_ ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  AHr Sensor: %s", ahr_ ? "configured" : "not configured");
  ESP_LOGCONFIG(TAG, "  Odometer Sensor: %s", odometer_ ? "configured" : "not configured");
}

} // namespace leaf_obd_uart
} // namespace esphome