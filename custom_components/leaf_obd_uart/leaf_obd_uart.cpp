#include "leaf_obd_uart.h"
#include <cstdio>
#include <cctype>
#include <cstdlib>

namespace esphome {
namespace leaf_obd_uart {

static const char *const TAG = "leaf_obd_uart";
/*
LeafObdComponent::~LeafObdComponent() {
  if (stream_) {
    delete stream_;
    stream_ = nullptr;
    ESP_LOGD(TAG, "UARTStreamAdapter deleted");
  }
}
*/
/*
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

  // Очистка буфера с таймаутом
  unsigned long start = millis();
  while (elm_stream_->available() && millis() - start < 200) {
    elm_stream_->read();
  }
  ESP_LOGD(TAG, "UART buffer cleared before initialization");

  if (!sendCommand("ATZ")) {
    ESP_LOGE(TAG, "Failed to send ATZ command");
    return false;
  }
  start = millis();
  while (millis() - start < 1500) { // Увеличен таймаут для ATZ
    yield();
  }

  if (!readResponse(2000)) { // Увеличен таймаут для ответа ATZ
    ESP_LOGE(TAG, "No response to ATZ command");
    return false;
  }
  ESP_LOGD(TAG, "Raw ATZ response: %s", response_buffer_);
  if (strstr(response_buffer_, "ELM327") == nullptr) {
    ESP_LOGE(TAG, "Invalid ATZ response: %s", response_buffer_);
    return false;
  }

  const char *init_commands[] = {
      "ATE0", "ATL0", "ATSP6", "ATH1", "ATS0", "ATCAF0", "ATFCSD300000", "ATFCSM1"
  };
  for (const char *cmd : init_commands) {
    if (!sendCommand(cmd)) {
      ESP_LOGE(TAG, "Failed to send command: %s", cmd);
      return false;
    }
    if (!readResponse(500)) {
      ESP_LOGE(TAG, "No response to command: %s", cmd);
      return false;
    }
    ESP_LOGD(TAG, "Raw %s response: '%s'", cmd, response_buffer_);
    if (strstr(response_buffer_, "OK") == nullptr && strcmp(cmd, "ATFCSM1") != 0) {
      ESP_LOGE(TAG, "Invalid response to %s: %s", cmd, response_buffer_);
      return false;
    }
    if (strcmp(cmd, "ATFCSM1") == 0 && strstr(response_buffer_, "?")) {
      ESP_LOGW(TAG, "ATFCSM1 not supported by this adapter, continuing...");
      // Продолжаем, так как ATFCSM1 не критично
    }
    start = millis();
    while (millis() - start < 50) {
      yield();
    }
  }

  if (!sendCommand("ATDP") || !readResponse(500)) {
    ESP_LOGE(TAG, "Failed to check active protocol");
    return false;
  }
  ESP_LOGD(TAG, "Active protocol: %s", response_buffer_);

  is_connected_ = true;
  status_ = OBD_CONNECTED;
  ESP_LOGI(TAG, "ELM327 initialized successfully");
  return true;
}
*/
// Альтернативная инициализация, как в лифспае
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

  // Очистка буфера с таймаутом
  unsigned long start = millis();
  while (elm_stream_->available() && millis() - start < 200) {
    elm_stream_->read();
  }
  ESP_LOGD(TAG, "UART buffer cleared before initialization");

  // Полная инициализация как в LeafSpy
  if (!sendCommand("ATZ") || !readResponse(1300)) return false; // Сброс
  if (!sendCommand("ATE0") || !readResponse(250)) return false;  // Отключить эхо
  if (!sendCommand("ATL0") || !readResponse(250)) return false;  // Отключить перевод строк
  if (!sendCommand("ATSP6") || !readResponse(250)) return false; // CAN 500 кбит/с
  if (!sendCommand("ATH1") || !readResponse(250)) return false;  // Включить заголовки
  if (!sendCommand("ATS0") || !readResponse(250)) return false;  // Отключить пробелы
  if (!sendCommand("ATCAF0") || !readResponse(250)) return false; // Отключить автоформатирование
  if (!sendCommand("ATSH797") || !readResponse(250)) return false;  // Set Header to 
  if (!sendCommand("ATFCSH797") || !readResponse(250)) return false;  // FC, Set the Header to 
  if (!sendCommand("ATFCSD300000") || !readResponse(250)) return false;  // FC, Set Data to [...]
// команды ATFCSH и ATFCSD обязательно должны идти перед ATFCSM1
  if (!sendCommand("ATFCSM1") || !readResponse(300)) return false;  // Flow Control, Set the Mode to 
  if (!sendCommand("0210C0") || !readResponse(500)) return false; // Mystery command
  if (!sendCommand("ATDP") || !readResponse(500)) {  // Describe the current Protocol
    ESP_LOGE(TAG, "Failed to check active protocol");
    return false;
  }
  ESP_LOGD(TAG, "Active protocol: %s", response_buffer_);

  is_connected_ = true;
  status_ = OBD_CONNECTED;
  ESP_LOGI(TAG, "ELM327 initialized successfully");
  return true;
}


bool ELM327::isCarResponsive() {
  if (!elm_stream_) {
    ESP_LOGW(TAG, "Cannot check car responsiveness: Stream is null");
    return false;
  }

  // Очистка буфера с таймаутом
  unsigned long start = millis();
  while (elm_stream_->available() && millis() - start < 200) {
    elm_stream_->read();
  }

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
  if (voltage > 12.8) {
//  if (voltage > 11.8) {  // По напряжению определяем включена ли машина. Временно занижал, чтобы посмотреть, что там отвечает
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

    response_buffer_[0] = '\0';
    size_t idx = 0;
    unsigned long start = millis();

    while (millis() - start < timeout_ms) {
        if (elm_stream_->available()) {
            char c = elm_stream_->read();
            // ESP_LOGD(TAG, "Received char: %c (0x%02X)", isprint(c) ? c : '.', c); // Можно оставить для отладки, но много логов

            // Пропускаем символы окончания строки
            if (c == '\r' || c == '\n') {
                continue;
            }

            // Проверяем символ завершения ответа ELM327
            if (c == '>') {
                // Завершаем строку в буфере
                response_buffer_[idx] = '\0';
                ESP_LOGD(TAG, "Response complete: %s", response_buffer_);
                return true; // Успешно завершено
            }

            // Записываем обычный символ в буфер
            if (idx < sizeof(response_buffer_) - 1) {
                response_buffer_[idx++] = c;
            } else {
                ESP_LOGW(TAG, "Response buffer overflow at %d bytes", idx);
                response_buffer_[idx] = '\0'; // Все равно завершаем строку
                // Можно вернуть false здесь, если переполнение критично
                // Но лучше завершить чтение, сохранив то, что есть.
                // break; // или return false;
            }
        }
        yield();
    }

    // Таймаут
    response_buffer_[idx] = '\0'; // Завершаем строку на случай таймаута
    ESP_LOGW(TAG, "Response timeout after %dms: %s", timeout_ms, response_buffer_);
    // Возвращаем true, если что-то было получено, иначе false.
    // Это соответствует оригинальной логике.
    return idx > 0;
}

/*
bool ELM327::readResponse(uint32_t timeout_ms) {
  if (!elm_stream_) {
    ESP_LOGW(TAG, "Cannot read response: Stream is null");
    return false;
  }

  response_buffer_[0] = '\0';
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
          timeout_ms = 2000;
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
*/

bool ELM327::sendCommand(const char *cmd) {
  if (!elm_stream_) {
    ESP_LOGW(TAG, "Cannot send command: Stream is null");
    return false;
  }
  // Очистка буфера с таймаутом
  unsigned long start = millis();
  while (elm_stream_->available() && millis() - start < 200) {
    elm_stream_->read();
  }
  ESP_LOGD(TAG, "Attempting to send command: %s", cmd);
  elm_stream_->print(cmd);
  elm_stream_->print("\r");
  elm_stream_->flush();
  ESP_LOGD(TAG, "Command sent successfully: %s", cmd);
  return true;
}

bool ELM327::setECU(const char *ecu) {
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "ATSH%s", ecu);  // Set Header to
  if (!sendCommand(cmd) || !readResponse(500)) {
    ESP_LOGE(TAG, "Failed to set ECU: %s", cmd);
    return false;
  }
  if (strstr(response_buffer_, "OK") == nullptr) {
    ESP_LOGE(TAG, "Invalid response to %s: %s", cmd, response_buffer_);
    return false;
  }
  snprintf(cmd, sizeof(cmd), "ATFCSH%s", ecu);  // set the ID Mask to 
  if (!sendCommand(cmd) || !readResponse(500)) {
    ESP_LOGE(TAG, "Failed to set Flow Control: %s", cmd);
    return false;
  }
  if (strstr(response_buffer_, "OK") == nullptr) {
    ESP_LOGE(TAG, "Invalid response to %s: %s", cmd, response_buffer_);
    return false;
  }

  // --- Специальная инициализация для ECU 743 ---
  // Проверяем, является ли текущий ECU целевым (743)
  if (strcmp(ecu, "743") == 0) {
    ESP_LOGD(TAG, "Performing special initialization for ECU 743");
    if (!sendCommand("ATFCSD300100") || !readResponse(500)) {
      ESP_LOGW(TAG, "Failed to send/set Flow Control Data '300100' for ECU 743");
    }
    if (!sendCommand("0210C0") || !readResponse(1000)) { // Таймаут побольше для "mystery" команды
      ESP_LOGW(TAG, "No response to Mystery Command '0210C0' for ECU 743");
    }
  }
  // --- Специальная инициализация для ECU 79B ---
  // Проверяем, является ли текущий ECU целевым (79B)
  if (strcmp(ecu, "79B") == 0 || strcmp(ecu, "797") == 0) {
    ESP_LOGD(TAG, "Performing special initialization for ECU 79B");
    if (!sendCommand("ATFCSD300000") || !readResponse(500)) {
      ESP_LOGW(TAG, "Failed to send/set Flow Control Data '300000' for ECU 79B");
    }
    if (!sendCommand("0210C0") || !readResponse(1000)) { // Таймаут побольше для "mystery" команды
      ESP_LOGW(TAG, "No response to Mystery Command '0210C0' for ECU 79B");
    }
  }


  return true;
}

bool ELM327::connected() {
  return is_connected_ && elm_stream_ != nullptr;
}

bool ELM327::queryUDS(const char *ecu, const char *pid, int retries) {
//  for (int i = 0; i < retries; i++) {
// Убираем несколько переподключений, так как в коде это не используется, да и лишние задержки
    response_buffer_[0] = '\0';
/*    if (!setECU(ecu)) {
      ESP_LOGW(TAG, "Retry %d: Failed to set ECU: %s", ecu);
//      ESP_LOGW(TAG, "Retry %d: Failed to set ECU: %s", i + 1, ecu);
//      continue;
    }*/
    if (!sendCommand(pid) || !readResponse(1000)) {
      ESP_LOGW(TAG, "No response to PID: %s", pid);
//      ESP_LOGW(TAG, "Retry %d: No response to PID: %s", i + 1, pid);
//      continue;
    }
//    if (strstr(response_buffer_, "NO DATA") || strstr(response_buffer_, "7F") || strstr(response_buffer_, "CAN ERROR")) {
// ищет 7F, и естественно в многострочном ответе находит его. Надо проверять на конкретном месте. Потом переделаю, а пока убрал проверку.
    if (strstr(response_buffer_, "NO DATA") || strstr(response_buffer_, "CAN ERROR")) {
      ESP_LOGW(TAG, "Invalid response for PID: %s, response: %s", pid, response_buffer_);
//      ESP_LOGW(TAG, "Retry %d: Invalid response for PID: %s, response: %s", i + 1, pid, response_buffer_);
//      continue;
    }
    ESP_LOGD(TAG, "UDS response: %s", response_buffer_);
    return true;
//  }
//  ESP_LOGE(TAG, "Failed UDS query after %d retries: ECU=%s, PID=%s", retries, ecu, pid);
//  return false;
}

void LeafObdComponent::setup() {
  ESP_LOGI(TAG, "Entering LeafObdComponent::setup...");
  if (!parent_ || !stream_) {
    ESP_LOGE(TAG, "UART parent or stream is null!");
    return;
  }
  ESP_LOGD(TAG, "UART initialized by ESPHome"); // Заменяем parent_->setup()
  flush_uart();
  unsigned long start = millis();
  while (millis() - start < 200) {
    yield();
  }
  if (!elm_.begin(*stream_)) {
    ESP_LOGE(TAG, "ELM327 initial setup failed");
    elm_.set_status(DISCONNECTED);
  } else if (elm_.isCarResponsive()) {
    elm_.set_status(CAR_CONNECTED);
    ESP_LOGI(TAG, "ELM327 setup complete, car connected");
  } else {
    elm_.set_status(OBD_CONNECTED);
    ESP_LOGI(TAG, "ELM327 setup complete, no car response");
  }
}

void LeafObdComponent::update() {
  static int state = 0; // Есть 4 состояния: 0 - проверрка состояния, 1 - Запрос Voltage, Temp, SOH, AHr, 2 - Запрос Odometer, 3 - Запрос Number of quick charges / L1/L2 charges
const char* raw_response;
int response_len;
  ESP_LOGD(TAG, "Updating Leaf OBD data (state=%d)...", state);

  switch (state) {
    case 0: // Проверка состояния ELM327 и автомобиля
      if (elm_.get_status() == DISCONNECTED) {
        ESP_LOGI(TAG, "ELM is not initialized. Starting setup...");
        flush_uart();
        unsigned long start = millis();
        while (millis() - start < 100) {
          yield();
        }
        if (!elm_.begin(*stream_)) {
          ESP_LOGE(TAG, "ELM327 initialization failed. Retrying later.");
          publish_nan();
          state = 0;
          return;
        }
      }
      if (!elm_.isCarResponsive()) {
        ESP_LOGI(TAG, "Car is not responsive. Setting OBD_CONNECTED.");
        elm_.set_status(OBD_CONNECTED);
        publish_nan();
        state = 0;
        return;
      }
      elm_.set_status(CAR_CONNECTED);
      state = 1;
      break;

    case 1: // 1 - Запрос Number of quick charges / L1/L2 charges и т.д. 
// --- power_switch ---
// PID: 03221304, ECU: 797, Response example: 79A0762130480BA00000
// Decoders.py: d[3] & 0x80 == 0x80 -> true/false
      if (elm_.connected() && elm_.setECU("797")) {        // 797 - VCM


if (power_switch_ && elm_.connected()) {
    if (elm_.queryUDS("797", "03221304")) {
        // const char* raw_response = elm_.get_response_buffer();
        raw_response = elm_.get_response_buffer(); // уже объявлено
        // int response_len = strlen(raw_response);
        response_len = strlen(raw_response); // уже объявлено
        // Проверка длины и префикса
        if (response_len >= 14 && strncmp(raw_response, "79A", 3) == 0) {
            // Байт данных d[3] находится по индексу 9 в строке "79A07621304..."
                char ps_hex[3] = {raw_response[11], raw_response[12], '\0'};
                char* endptr;
                long ps_raw = strtol(ps_hex, &endptr, 16);
                if (*endptr == '\0') {
                    bool ps_val = (ps_raw & 0x80) == 0x80;
                    power_switch_->publish_state(ps_val);
                    ESP_LOGD(TAG, "Power Switch: %s (raw hex: %s)", ps_val ? "ON" : "OFF", ps_hex);
                } else {
                    ESP_LOGW(TAG, "Failed to parse Power Switch hex: %s", ps_hex);
                    power_switch_->publish_state(NAN); // Или false, в зависимости от типа сенсора
                }
        } else {
            ESP_LOGW(TAG, "Invalid Power Switch response format: %s", raw_response);
            power_switch_->publish_state(NAN);
        }
    } else {
        ESP_LOGW(TAG, "Failed to query Power Switch");
        power_switch_->publish_state(NAN);
    }
}

// --- bat_12v_voltage ---
// PID: 03221103, ECU: 797, Response example: 79A07621103A4030000 (A4 = 164 -> 164*0.08=13.12V)
// Decoders.py: d[3] * 0.08
if (bat_12v_voltage_ && elm_.connected()) {
    if (elm_.queryUDS("797", "03221103")) {
        // const char* raw_response = elm_.get_response_buffer();
        raw_response = elm_.get_response_buffer(); // уже объявлено
        // int response_len = strlen(raw_response);
        response_len = strlen(raw_response); // уже объявлено
        if (response_len >= 14 && strncmp(raw_response, "79A", 3) == 0) {
                char v12_hex[3] = {raw_response[11], raw_response[12], '\0'};
                char* endptr;
                long v12_raw = strtol(v12_hex, &endptr, 16);
                if (*endptr == '\0') {
                    float v12_val = static_cast<float>(v12_raw) * 0.08f;
                    bat_12v_voltage_->publish_state(v12_val);
                    ESP_LOGD(TAG, "12V Battery Voltage: %.2fV (raw hex: %s)", v12_val, v12_hex);
                } else {
                    ESP_LOGW(TAG, "Failed to parse 12V Voltage hex: %s", v12_hex);
                    bat_12v_voltage_->publish_state(NAN);
                }
        } else {
            ESP_LOGW(TAG, "Invalid 12V Voltage response format: %s", raw_response);
            bat_12v_voltage_->publish_state(NAN);
        }
    } else {
        ESP_LOGW(TAG, "Failed to query 12V Voltage");
        bat_12v_voltage_->publish_state(NAN);
    }
}

// --- bat_12v_current ---
// PID: 03221183, ECU: 797, Response example: 79A0762118388B8FFFD
//                                            79A07621183819BFFFF
// Decoders.py: struct.unpack("!h", d[3:5])[0] / 256
// "!h" означает signed short, big-endian. d[3]=0x88, d[4]=0xB8 -> 0x88B8 -> signed -30536 -> /256 = -119.28 A
if (bat_12v_current_ && elm_.connected()) {
    if (elm_.queryUDS("797", "03221183")) {
        // const char* raw_response = elm_.get_response_buffer();
        raw_response = elm_.get_response_buffer(); // уже объявлено
        // int response_len = strlen(raw_response);
        response_len = strlen(raw_response); // уже объявлено
        if (response_len >= 14 && strncmp(raw_response, "79A", 3) == 0) {
                char i12_hex[5] = {raw_response[11], raw_response[12], raw_response[13], raw_response[14], '\0'};
                char* endptr;
                // Используем strtol для преобразования 4 hex символов (2 байта) в long
                // Затем приводим к signed short (int16_t)
                long i12_raw_long = strtol(i12_hex, &endptr, 16);
                if (*endptr == '\0') {
                    // Приведение к int16_t для правильной обработки знака
                    int16_t i12_raw = static_cast<int16_t>(i12_raw_long);
                    float i12_val = static_cast<float>(i12_raw) / 256.0f;
                    bat_12v_current_->publish_state(i12_val);
                    ESP_LOGD(TAG, "12V Battery Current: %.2fA (raw hex: %s)", i12_val, i12_hex);
                } else {
                    ESP_LOGW(TAG, "Failed to parse 12V Current hex: %s", i12_hex);
                    bat_12v_current_->publish_state(NAN);
                }
        } else {
            ESP_LOGW(TAG, "Invalid 12V Current response format: %s", raw_response);
            bat_12v_current_->publish_state(NAN);
        }
    } else {
        ESP_LOGW(TAG, "Failed to query 12V Current");
        bat_12v_current_->publish_state(NAN);
    }
}

// --- quick_charges ---
// PID: 03221203, ECU: 797, Response example: 79A076212030013FFFF (0013 = 19)
// Decoders.py: int.from_bytes(d[3:5])
if (quick_charges_ && elm_.connected()) {
    if (elm_.queryUDS("797", "03221203")) {
        //const char* raw_response = elm_.get_response_buffer();
        raw_response = elm_.get_response_buffer(); // уже объявлено
        // int response_len = strlen(raw_response);
        response_len = strlen(raw_response); // уже объявлено
        if (response_len >= 15 && strncmp(raw_response, "79A", 3) == 0) {
                char qc_hex[5] = {raw_response[11], raw_response[12], raw_response[13], raw_response[14], '\0'};
                char* endptr;
                long qc_raw = strtol(qc_hex, &endptr, 16);
                if (*endptr == '\0' && qc_raw >= 0 && qc_raw <= 0xFFFF) {
                    // unsigned int 16 bit
                    uint16_t qc_val = static_cast<uint16_t>(qc_raw);
                    quick_charges_->publish_state(static_cast<float>(qc_val));
                    ESP_LOGD(TAG, "Quick Charges: %u (raw hex: %s)", qc_val, qc_hex);
                } else {
                    ESP_LOGW(TAG, "Failed to parse Quick Charges hex: %s", qc_hex);
                    quick_charges_->publish_state(NAN);
                }
        } else {
            ESP_LOGW(TAG, "Invalid Quick Charges response format: %s", raw_response);
            quick_charges_->publish_state(NAN);
        }
    } else {
        ESP_LOGW(TAG, "Failed to query Quick Charges");
        quick_charges_->publish_state(NAN);
    }
}

// --- l1_l2_charges ---
// PID: 03221205, ECU: 797, Response example: 79A076212050F00FFFF (0F00 = 3840)
// Decoders.py: int.from_bytes(d[3:5])
if (l1_l2_charges_ && elm_.connected()) {
    if (elm_.queryUDS("797", "03221205")) {
        // const char* raw_response = elm_.get_response_buffer();
        raw_response = elm_.get_response_buffer(); // уже объявлено
        // int response_len = strlen(raw_response);
        response_len = strlen(raw_response); // уже объявлено
        if (response_len >= 14 && strncmp(raw_response, "79A", 3) == 0) {
            if (response_len > 12) {
                char l12c_hex[5] = {raw_response[11], raw_response[12], raw_response[13], raw_response[14], '\0'};
                char* endptr;
                long l12c_raw = strtol(l12c_hex, &endptr, 16);
                if (*endptr == '\0' && l12c_raw >= 0 && l12c_raw <= 0xFFFF) {
                    uint16_t l12c_val = static_cast<uint16_t>(l12c_raw);
                    l1_l2_charges_->publish_state(static_cast<float>(l12c_val));
                    ESP_LOGD(TAG, "L1/L2 Charges: %u (raw hex: %s)", l12c_val, l12c_hex);
                } else {
                    ESP_LOGW(TAG, "Failed to parse L1/L2 Charges hex: %s", l12c_hex);
                    l1_l2_charges_->publish_state(NAN);
                }
            } else {
                ESP_LOGW(TAG, "Response too short for L1/L2 Charges");
                l1_l2_charges_->publish_state(NAN);
            }
        } else {
            ESP_LOGW(TAG, "Invalid L1/L2 Charges response format: %s", raw_response);
            l1_l2_charges_->publish_state(NAN);
        }
    } else {
        ESP_LOGW(TAG, "Failed to query L1/L2 Charges");
        l1_l2_charges_->publish_state(NAN);
    }
}

// --- ambient_temp ---
// PID: 0322115D, ECU: 797, Response example: 79A0762115D743A7400 (74 = 116 -> 116/2-40 = 18C)
// Decoders.py: d[3] / 2 - 40

if (ambient_temp_ && elm_.connected()) {
    if (elm_.queryUDS("797", "0322115D")) {
        // const char* raw_response = elm_.get_response_buffer();
        raw_response = elm_.get_response_buffer(); // уже объявлено
        // int response_len = strlen(raw_response);
        response_len = strlen(raw_response); // уже объявлено
        if (response_len >= 11 && strncmp(raw_response, "79A", 3) == 0) {
                char at_hex[3] = {raw_response[11], raw_response[12], '\0'};
                char* endptr;
                long at_raw = strtol(at_hex, &endptr, 16);
                if (*endptr == '\0') {
                    float at_val = static_cast<float>(at_raw) / 2.0f - 40.0f;
                    ambient_temp_->publish_state(at_val);
                    ESP_LOGD(TAG, "Ambient Temperature: %.1f°C (raw hex: %s)", at_val, at_hex);
                } else {
                    ESP_LOGW(TAG, "Failed to parse Ambient Temp hex: %s", at_hex);
                    ambient_temp_->publish_state(NAN);
                }
        } else {
            ESP_LOGW(TAG, "Invalid Ambient Temp response format: %s", raw_response);
            ambient_temp_->publish_state(NAN);
        }
    } else {
        ESP_LOGW(TAG, "Failed to query Ambient Temp");
        ambient_temp_->publish_state(NAN);
    }
}

// --- estimated_ac_power ---
// PID: 03221261, ECU: 797, Response example: 79A0762126100010003 (00 = 0 -> 0*50=0W)
// Decoders.py: d[3] * 50
if (estimated_ac_power_ && elm_.connected()) {
    if (elm_.queryUDS("797", "03221261")) {
        // const char* raw_response = elm_.get_response_buffer();
        raw_response = elm_.get_response_buffer(); // уже объявлено
        // int response_len = strlen(raw_response);
        response_len = strlen(raw_response); // уже объявлено
        if (response_len >= 14 && strncmp(raw_response, "79A", 3) == 0) {
                char eac_hex[3] = {raw_response[11], raw_response[12], '\0'};
                char* endptr;
                long eac_raw = strtol(eac_hex, &endptr, 16);
                if (*endptr == '\0') {
                    float eac_val = static_cast<float>(eac_raw) * 50.0f;
                    estimated_ac_power_->publish_state(eac_val);
                    ESP_LOGD(TAG, "Estimated AC Power: %.0fW (raw hex: %s)", eac_val, eac_hex);
                } else {
                    ESP_LOGW(TAG, "Failed to parse Estimated AC Power hex: %s", eac_hex);
                    estimated_ac_power_->publish_state(NAN);
                }
        } else {
            ESP_LOGW(TAG, "Invalid Estimated AC Power response format: %s", raw_response);
            estimated_ac_power_->publish_state(NAN);
        }
    } else {
        ESP_LOGW(TAG, "Failed to query Estimated AC Power");
        estimated_ac_power_->publish_state(NAN);
    }
}

// --- aux_power ---
// PID: 03221152, ECU: 797, Response example: 79A0762115202190101 (02 = 2 -> 2*100=200W)
// Decoders.py: d[3] * 100
if (aux_power_ && elm_.connected()) {
    if (elm_.queryUDS("797", "03221152")) {
        // const char* raw_response = elm_.get_response_buffer();
        raw_response = elm_.get_response_buffer(); // уже объявлено
        // int response_len = strlen(raw_response);
        response_len = strlen(raw_response); // уже объявлено
        if (response_len >= 14 && strncmp(raw_response, "79A", 3) == 0) {
                char aux_hex[3] = {raw_response[11], raw_response[12], '\0'};
                char* endptr;
                long aux_raw = strtol(aux_hex, &endptr, 16);
                if (*endptr == '\0') {
                    float aux_val = static_cast<float>(aux_raw) * 100.0f;
                    aux_power_->publish_state(aux_val);
                    ESP_LOGD(TAG, "Auxiliary Power: %.0fW (raw hex: %s)", aux_val, aux_hex);
                } else {
                    ESP_LOGW(TAG, "Failed to parse Auxiliary Power hex: %s", aux_hex);
                    aux_power_->publish_state(NAN);
                }
        } else {
            ESP_LOGW(TAG, "Invalid Auxiliary Power response format: %s", raw_response);
            aux_power_->publish_state(NAN);
        }
    } else {
        ESP_LOGW(TAG, "Failed to query Auxiliary Power");
        aux_power_->publish_state(NAN);
    }
}

// --- ac_power ---
// PID: 03221151, ECU: 797, Response example: 79A0762115100000037 (00 = 0 -> 0*250=0W)
// Decoders.py: d[3] * 250
if (ac_power_ && elm_.connected()) {
    if (elm_.queryUDS("797", "03221151")) {
        // const char* raw_response = elm_.get_response_buffer();
        raw_response = elm_.get_response_buffer(); // уже объявлено
        // int response_len = strlen(raw_response);
        response_len = strlen(raw_response); // уже объявлено
        if (response_len >= 14 && strncmp(raw_response, "79A", 3) == 0) {
                char acp_hex[3] = {raw_response[11], raw_response[12], '\0'};
                char* endptr;
                long acp_raw = strtol(acp_hex, &endptr, 16);
                if (*endptr == '\0' && acp_raw >= 0 && acp_raw <= 0xFF) {
                    float acp_val = static_cast<float>(acp_raw) * 250.0f;
                    ac_power_->publish_state(acp_val);
                    ESP_LOGD(TAG, "AC System Power: %.0fW (raw hex: %s)", acp_val, acp_hex);
                } else {
                    ESP_LOGW(TAG, "Failed to parse AC Power hex: %s", acp_hex);
                    ac_power_->publish_state(NAN);
                }
        } else {
            ESP_LOGW(TAG, "Invalid AC Power response format: %s", raw_response);
            ac_power_->publish_state(NAN);
        }
    } else {
        ESP_LOGW(TAG, "Failed to query AC Power");
        ac_power_->publish_state(NAN);
    }
}

// --- plug_state ---
// PID: 03221234, ECU: 797, Response example: 79A0762123400000A0A (00 = Not plugged)
// Decoders.py: match d[3]: case 0: "Not plugged", case 1: "Partial plugged", case 2: "Plugged"
// Предположим, что сенсор `plug_state_` типа `text_sensor::TextSensor`
if (plug_state_ && elm_.connected()) {
    if (elm_.queryUDS("797", "03221234")) {
        // const char* raw_response = elm_.get_response_buffer();
        raw_response = elm_.get_response_buffer(); // уже объявлено
        // int response_len = strlen(raw_response);
        response_len = strlen(raw_response); // уже объявлено
        if (response_len >= 11 && strncmp(raw_response, "79A", 3) == 0) {
                char ps_hex[3] = {raw_response[11], raw_response[12], '\0'};
                char* endptr;
                long ps_raw = strtol(ps_hex, &endptr, 16);
                if (*endptr == '\0' && ps_raw >= 0 && ps_raw <= 0xFF) {
                    std::string ps_val = "Unknown";
                    switch (ps_raw) {
                        case 0: ps_val = "Not plugged"; break;
                        case 1: ps_val = "Partial plugged"; break;
                        case 2: ps_val = "Plugged"; break;
                        default: ps_val = "Unknown"; break;
                    }
                    plug_state_->publish_state(ps_val);
                    ESP_LOGD(TAG, "Plug State: %s (raw hex: %s)", ps_val.c_str(), ps_hex);
                } else {
                    ESP_LOGW(TAG, "Failed to parse Plug State hex: %s", ps_hex);
                    plug_state_->publish_state("Error");
                }
        } else {
            ESP_LOGW(TAG, "Invalid Plug State response format: %s", raw_response);
            plug_state_->publish_state("Error");
        }
    } else {
        ESP_LOGW(TAG, "Failed to query Plug State");
        plug_state_->publish_state("Error");
    }
}

// --- charge_mode ---
// PID: 0322114E, ECU: 797, Response example: 79A0762114E00001F07 (00 = Not charging)
// Decoders.py: match d[3]: case 0: "Not charging", case 1: "L1", case 2: "L2", case 3: "L3"
// Предположим, что сенсор `charge_mode_` типа `text_sensor::TextSensor`
if (charge_mode_ && elm_.connected()) {
    if (elm_.queryUDS("797", "0322114E")) {
        // const char* raw_response = elm_.get_response_buffer();
        raw_response = elm_.get_response_buffer(); // уже объявлено
        // int response_len = strlen(raw_response);
        response_len = strlen(raw_response); // уже объявлено
        if (response_len >= 14 && strncmp(raw_response, "79A", 3) == 0) {
                char cm_hex[3] = {raw_response[11], raw_response[12], '\0'};
                char* endptr;
                long cm_raw = strtol(cm_hex, &endptr, 16);
                if (*endptr == '\0') {
                    std::string cm_val = "Unknown";
                    switch (cm_raw) {
                        case 0: cm_val = "Not charging"; break;
                        case 1: cm_val = "L1 charging"; break;
                        case 2: cm_val = "L2 charging"; break;
                        case 3: cm_val = "L3 charging"; break;
                        default: cm_val = "Unknown"; break;
                    }
                    charge_mode_->publish_state(cm_val);
                    ESP_LOGD(TAG, "Charge Mode: %s (raw hex: %s)", cm_val.c_str(), cm_hex);
                } else {
                    ESP_LOGW(TAG, "Failed to parse Charge Mode hex: %s", cm_hex);
                    charge_mode_->publish_state("Error");
                }
        } else {
            ESP_LOGW(TAG, "Invalid Charge Mode response format: %s", raw_response);
            charge_mode_->publish_state("Error");
        }
    } else {
        ESP_LOGW(TAG, "Failed to query Charge Mode");
        charge_mode_->publish_state("Error");
    }
}
/*
      if (soc_ && elm_.connected()) {
        if (elm_.queryUDS("797", "02215D")) { // 797 - VCM
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
*/
      } // от if setECU("797")...
      state = 2;
      break;

    case 3: // Запрос Voltage, Temp, SOH, AHr
      if (elm_.connected() && elm_.setECU("79B")) {       // 79B - LBC(BMS)

        if (elm_.queryUDS("79B", "022101")) {


// Предполагаем, что elm_.queryUDS("79B", "022101") уже был вызван.
// const char* raw_response = elm_.get_response_buffer(); // Или просто response_buffer_
        raw_response = elm_.get_response_buffer(); // уже объявлено
int response_len = strlen(raw_response);  // определяем длину полученного ответа

// --- Проверка полученного ответа ---
// Минимальная длина для First Frame с 6 байтами данных = 19 символов.
if (response_len < 114) {
    ESP_LOGE(TAG, "Response is too short for 022101: %s", raw_response);
    // publish_nan() для соответствующих сенсоров
    if (soc_) soc_->publish_state(NAN);
    if (soh_) soh_->publish_state(NAN);
    if (ahr_) ahr_->publish_state(NAN);
    if (hv_) hv_->publish_state(NAN);
    break;  // Если строка меньшей длины, значит выходим из case
    //return;
}

// --- Извлечение длины полезной нагрузки из First Frame ---
// Байты 5 и 6 (символы 5 и 6 в строке 0-based) содержат длину в hex.
// Пример: "...7BB1029..." -> длина = 0x29 = 41 байт.
// Извлекаем подстроку длиной 2 символа, начиная с индекса 5.
char length_hex_chars[3] = {raw_response[5], raw_response[6], '\0'};
char* endptr_len;
int total_expected_data_length = strtol(length_hex_chars, &endptr_len, 16);

// Проверяем результат преобразования длины
if (*endptr_len != '\0' || total_expected_data_length <= 0 || total_expected_data_length > 0xC6) {
     ESP_LOGW(TAG, "Failed to parse or invalid data length from First Frame header: %s. Using default 41.", length_hex_chars);
     total_expected_data_length = 41; // Значение по умолчанию или ошибка
     // Или return;
}

// --- Инициализация буфера для бинарных данных ---
uint8_t payload_data[0xC6]; // 0xC6 = 198
int payload_len = 0;

// --- Основной цикл преобразования ---
// Мы знаем структуру ответа ISO-TP для 022101:
// - First Frame: 7BB10XX + 12 hex символов данных + (возможные другие данные)
// - Consecutive Frames: 7BB2X + 14 hex символов данных + ...
// Данные в FF начинаются с индекса 7. Данные в CF начинаются с индекса 5 относительно начала фрейма.
// Длина FF данных = 12 символов. Длина CF данных = 14 символов.
int pos = 0; // Текущая позиция в строке raw_response

while (pos < response_len && payload_len < 0xC6) {
    // --- Определение типа фрейма по индексу и длине строки ---
    // Это логика без strncmp. Определяем тип фрейма на основе позиции и оставшейся длины.
    // First Frame всегда начинается с позиции 0.
    if (pos == 0) {
        // --- Обработка First Frame ---
        // Извлекаем 6 байт (12 hex символов) данных из First Frame, начиная с индекса 7.
        for (int i = 0; i < 12 && (pos + 7 + i) < response_len && payload_len < 0xC6; i += 2) {
             // --- Преобразование пары hex-символов в байт с помощью strtol ---
             // Создаем временную C-строку из двух символов и завершающего нуля.
             char byte_hex_chars[3] = {raw_response[pos + 7 + i], raw_response[pos + 7 + i + 1], '\0'};
             char* endptr;
             // Преобразуем. base=16 для hex.
             long val = strtol(byte_hex_chars, &endptr, 16);

             // --- Проверка результата преобразования ---
             // *endptr != '\0' означает, что в строке были недопустимые символы.
             // val > 0xFF означает, что число больше 255.
             // val < 0 означает, что число отрицательное (не должно быть для hex без знака, но strtol возвращает long).
             if (*endptr != '\0' || val > 0xFF || val < 0) {
                 ESP_LOGE(TAG, "Failed to convert hex byte pair '%s' from First Frame data at global pos %d", byte_hex_chars, pos + 7 + i);
                 // publish_nan() / return
                 if (soc_) soc_->publish_state(NAN);
                 if (soh_) soh_->publish_state(NAN);
                 if (ahr_) ahr_->publish_state(NAN);
                 if (hv_) hv_->publish_state(NAN);
                 return;
             }

             // --- Запись байта в буфер ---
             payload_data[payload_len++] = (uint8_t)val;
        }
        // Перемещаем pos на начало следующего потенциального фрейма.
        // First Frame: 7 (заголовок) + 12 (данные первого фрейма) = 19 символов.
        pos += 19;
    }
    // --- Обработка Consecutive Frames ---
    // CF идут после FF. Проверяем, остались ли символы и соответствует ли позиция началу CF.
    // CF имеют префикс 7BB2X (5 символов). Данные CF начинаются с позиции 5 относительно начала фрейма.
    // Проверяем, достаточно ли символов в строке для минимального CF (заголовок 5 + 2 символа данных = 7)
    else if ((response_len - pos) >= 7 && pos >= 19 && (pos - 19) % 19 == 0) {
        // Это условие пытается определить CF: позиция после FF (>=19) и кратна шагу фрейма (19).
        // Однако, оно может быть хрупким. Более надежно просто проверить префикс, но по запросу избегаем strncmp.
        // Альтернатива: просто обрабатываем оставшиеся данные блоками по 19, если они подходят.
        // Упростим: если pos >= 19 и осталось >= 7 символов, пробуем обработать как CF.

        // Проверка на минимальную длину CF (заголовок 5 + 2 символа данных = 7)
        // if (response_len - pos < 7) { break; } // Уже проверили выше

        // Извлекаем 7 байт (14 hex символов) данных из Consecutive Frame, начиная с индекса 5 относительно начала фрейма.
        for (int i = 0; i < 14 && (pos + 5 + i) < response_len && payload_len < 0xC6; i += 2) {
             if ((pos + 5 + i + 1) >= response_len) {
                 ESP_LOGE(TAG, "Hex string for Consecutive Frame data is truncated at global pos %d", pos + 5 + i);
                 // publish_nan() / return
                 if (soc_) soc_->publish_state(NAN);
                 if (soh_) soh_->publish_state(NAN);
                 if (ahr_) ahr_->publish_state(NAN);
                 if (hv_) hv_->publish_state(NAN);
                 return;
             }

             // --- Преобразование пары hex-символов в байт с помощью strtol ---
             char byte_hex_chars[3] = {raw_response[pos + 5 + i], raw_response[pos + 5 + i + 1], '\0'};
             char* endptr;
             long val = strtol(byte_hex_chars, &endptr, 16);

             if (*endptr != '\0' || val > 0xFF || val < 0) {
                 ESP_LOGE(TAG, "Failed to convert hex byte pair '%s' from Consecutive Frame data at global pos %d", byte_hex_chars, pos + 5 + i);
                 // publish_nan() / return
                 if (soc_) soc_->publish_state(NAN);
                 if (soh_) soh_->publish_state(NAN);
                 if (ahr_) ahr_->publish_state(NAN);
                 if (hv_) hv_->publish_state(NAN);
                 return;
             }

             // --- Запись байта в буфер ---
             payload_data[payload_len++] = (uint8_t)val;
        }
        // Перемещаем pos на начало следующего потенциального фрейма.
        // Consecutive Frame: 5 (заголовок) + 14 (данные последующих фреймов) = 19 символов.
        pos += 19;
    } else {
        // Если позиция не соответствует ожидаемому началу FF или CF, или осталось меньше 7 символов,
        // значит, мы либо обработали все фреймы, либо наткнулись на неполный/остаточный фрагмент.
        // Просто выходим из цикла.
        break;
    }
}

// --- Проверка итоговой длины ---
ESP_LOGD(TAG, "Finished parsing 022101 response. Extracted payload length: %d bytes (expected: %d)", payload_len, total_expected_data_length);
// Логирование первых нескольких байт для отладки
std::string debug_hex;
for(int i = 0; i < std::min(payload_len, 10); ++i) {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02X", payload_data[i]);
    debug_hex += buf;
}
ESP_LOGD(TAG, "Byte Payload (first 10 bytes hex): %s", debug_hex.c_str());

// Минимальные индексы, которые мы используем: HV(16,17), SOH(20,21), SOC(27,28,29), AHr(30,31,32,33).
// --- Извлечение параметров по известным смещениям ---
// payload_data теперь содержит байты данных.

// --- HV Voltage (байты 16, 17) ---
if (hv_) {
        uint16_t hv_raw = (static_cast<uint16_t>(payload_data[20]) << 8) | payload_data[21];
        float hv_val = static_cast<float>(hv_raw) / 100.0;
        hv_->publish_state(hv_val);
        ESP_LOGD(TAG, "Decoded HV Voltage: %.2fV", hv_val);
}

// --- SOH (байты 20, 21) ---
if (soh_) {
        uint16_t soh_raw = (static_cast<uint16_t>(payload_data[24]) << 8) | payload_data[25];
        float soh_val = static_cast<float>(soh_raw) / 15.671;
        soh_->publish_state(soh_val);
        ESP_LOGD(TAG, "Decoded SOH: %.4f%%", soh_val);
}

// --- SOC (байты 27, 28, 29) ---
if (soc_) {
        // Предполагаем 3 байта, как в предыдущих примерах и формулах.
        uint32_t soc_raw = (static_cast<uint32_t>(payload_data[31]) << 16) |
                           (static_cast<uint32_t>(payload_data[32]) << 8) |
                            static_cast<uint32_t>(payload_data[33]);
        float soc_val = static_cast<float>(soc_raw) / 10000.0;
        soc_->publish_state(soc_val);
        ESP_LOGD(TAG, "Decoded SOC: %.4f%%", soc_val);
}

// --- AHr (байты 30, 31, 32, 33) ---
if (ahr_) {
         // Предполагаем 4 байта, как в предыдущих примерах и формулах.
         uint32_t ahr_raw = (static_cast<uint32_t>(payload_data[34]) << 24) |
                            (static_cast<uint32_t>(payload_data[35]) << 16) |
                            (static_cast<uint32_t>(payload_data[36]) << 8) |
                             static_cast<uint32_t>(payload_data[37]);
         float ahr_val = static_cast<float>(ahr_raw) / 10000.0;
         ahr_->publish_state(ahr_val);
         ESP_LOGD(TAG, "Decoded AHr: %.4fAh", ahr_val);
}

// ... обработка других параметров ...
ESP_LOGD(TAG, "Finished processing 022101 response for ECU 79B");
        }


        if (elm_.queryUDS("79B", "022104")) {
// --- Упрощенная проверка структуры ---
// Проверяем, достаточно ли длинный ответ и содержит ли он ожидаемые фреймы.
// Минимальная длина для 3 фреймов:
// 7BB1010610401B61C017BB21B11C01BC1B01BD7BB221B1B00FFFFFFFF
// 7BB1010610401D019017BB21D01901D81801D97BB22181800FFFFFFFF
// 7BB10106104 01 F5 15 01 7BB21 FD 14 01 FF 14 02 03 7BB22 14 14 00 FFFFFFFF
// 7BB10106104 01 B6 1C 01 7BB21 B1 1C 01 BC 1B 01 BD 7BB22 1B 1B 00 FFFFFFFF
//                   ^^             ^^       ^^             ^^
//                 15,16           26,27    32,33          43,44

// FF (7BB10) + 12 hex символов данных + CF1 (7BB21) + 14 символов + CF2 (7BB22) + 14 символов = 
// (7+12) + (5+14) + (5+14) = 19 + 19 + 19 = 57 символов.
// Но лучше проверить наличие префиксов.

response_len = strlen(raw_response);  // определяем длину полученного ответа
if (response_len >= 57 && strncmp(raw_response, "7BB10", 5) == 0) {
    // --- Извлечение температур напрямую из строки raw_response ---
    
    // Температура 1: CF1, данные, байт 7 (индекс 6). 
    // Данные CF1 начинаются с символа 24 (19 заголовок FF + 5 заголовок CF1).
    // Индекс символа = 24 + 6 * 2 = 36. Нужны символы 36 и 37.
        char temp1_hex[3] = {raw_response[12], raw_response[13], '\0'};
        char* endptr1;
        long temp1_raw = strtol(temp1_hex, &endptr1, 16);
        if (*endptr1 == '\0' && temp1_raw >= 0 && temp1_raw <= 0xFF) {
            float temp1_val = static_cast<float>(temp1_raw);
            if (battery_temp_1_) {
                battery_temp_1_->publish_state(temp1_val);
                ESP_LOGD(TAG, "Battery Temperature 1 (simplified): %.1f°C (hex: %s)", temp1_val, temp1_hex);
            }
        } else {
            ESP_LOGW(TAG, "Failed to parse Battery Temperature 1 hex: %s", temp1_hex);
            if (battery_temp_1_) battery_temp_1_->publish_state(NAN);
        }

    // Температура 2: CF1, данные, байт 3 (индекс 2).
    // Индекс символа = 24 + 2 * 2 = 28. Нужны символы 28 и 29.
        char temp2_hex[3] = {raw_response[26], raw_response[27], '\0'};
        char* endptr2;
        long temp2_raw = strtol(temp2_hex, &endptr2, 16);
        if (*endptr2 == '\0' && temp2_raw >= 0 && temp2_raw <= 0xFF) {
            float temp2_val = static_cast<float>(temp2_raw);
            if (battery_temp_2_) {
                battery_temp_2_->publish_state(temp2_val);
                ESP_LOGD(TAG, "Battery Temperature 2 (simplified): %.1f°C (hex: %s)", temp2_val, temp2_hex);
            }
        } else {
            ESP_LOGW(TAG, "Failed to parse Battery Temperature 2 hex: %s", temp2_hex);
            if (battery_temp_2_) battery_temp_2_->publish_state(NAN);
        }

    // Температура 3: CF1, данные, байт 6 (индекс 5).
    // Индекс символа = 24 + 5 * 2 = 34. Нужны символы 34 и 35.
        char temp3_hex[3] = {raw_response[32], raw_response[33], '\0'};
        char* endptr3;
        long temp3_raw = strtol(temp3_hex, &endptr3, 16);
        if (*endptr3 == '\0' && temp3_raw >= 0 && temp3_raw <= 0xFF) {
            float temp3_val = static_cast<float>(temp3_raw);
            if (battery_temp_3_) {
                battery_temp_3_->publish_state(temp3_val);
                ESP_LOGD(TAG, "Battery Temperature 3 (simplified): %.1f°C (hex: %s)", temp3_val, temp3_hex);
            }
        } else {
            ESP_LOGW(TAG, "Failed to parse Battery Temperature 3 hex: %s", temp3_hex);
            if (battery_temp_3_) battery_temp_3_->publish_state(NAN);
        }

    // Температура 4: CF2, данные, байт 3 (индекс 2).
    // Данные CF2 начинаются с символа 43 (19 FF + 19 CF1 + 5 заголовок CF2).
    // Индекс символа = 43 + 2 * 2 = 47. Нужны символы 47 и 48.
        char temp4_hex[3] = {raw_response[43], raw_response[44], '\0'};
        char* endptr4;
        long temp4_raw = strtol(temp4_hex, &endptr4, 16);
        if (*endptr4 == '\0' && temp4_raw >= 0 && temp4_raw <= 0xFF) {
            float temp4_val = static_cast<float>(temp4_raw);
            if (battery_temp_4_) {
                battery_temp_4_->publish_state(temp4_val);
                ESP_LOGD(TAG, "Battery Temperature 4 (simplified): %.1f°C (hex: %s)", temp4_val, temp4_hex);
            }
        } else {
            ESP_LOGW(TAG, "Failed to parse Battery Temperature 4 hex: %s", temp4_hex);
            if (battery_temp_4_) battery_temp_4_->publish_state(NAN);
        }

} else {
    ESP_LOGW(TAG, "Invalid or incomplete Battery Temperature response structure for 022104: %s", raw_response);
    if (battery_temp_1_) battery_temp_1_->publish_state(NAN);
    if (battery_temp_2_) battery_temp_2_->publish_state(NAN);
    if (battery_temp_3_) battery_temp_3_->publish_state(NAN);
    if (battery_temp_4_) battery_temp_4_->publish_state(NAN);
}



        }
      }
      state = 0;
      break;

    case 2: // Запрос Odometer

if (odometer_ ) {
      if (elm_.connected() && elm_.setECU("743")) {       // 743 - M&A (Meter)

    if (elm_.queryUDS("743", "022101")) {  // 743 - M&A (Meter)
        raw_response = elm_.get_response_buffer(); // уже объявлено
        response_len = strlen(raw_response);

        ESP_LOGD(TAG, "Raw Odometer response: %s", raw_response);

        // --- Определение смещения данных одометра ---
        // Ответ на 022110 к 743 (ID 763) может быть многофреймовым.
        // Пример: "76310826101000000007632100000001A7BE00"
        // Структура:
        // 7631082 - First Frame header (ID 763, PCI 10, Length 82 hex = 130 dec? или 0x08 0x2?)
        //           Или 763 1 0 82 -> 763 (ID), 1 (First Frame), 08 (Length High), 2 (Length Low) -> Длина 0x82 = 130 байт.
        //           Но в примере длина данных в FF = 6 байт (12 hex символов). Пересмотр.
        // Правильнее: 763 10 82 -> 763 (ID), 10 (PCI: First Frame, Length Low 0), 8 (Length High 8) -> Длина 0x08 = 8 байт данных.
        // 610100000000 - Данные First Frame (Mode 61, PID 01, 6 байт данных: 00 00 00 00 00 00)
        // 7632100 - Consecutive Frame 1 header (ID 763, PCI 21, Sequence 0)
        // 00000001A7BE00 - Данные CF1 (7 байт: 00 00 00 01 A7 BE 00)
        //
        // Общая структура данных ответа: [61 01 00 00 00 00] [00 00 00 01 A7 BE 00]
        // Индексы данных:                [ 0  1  2  3  4  5] [ 6  7  8  9 10 11 12]
        // struct.unpack(">I", b'\x00\x01\xA7\xBE')[0] = 0x0001A7BE = 108478. Подходит!
        // Значит, нам нужны байты данных с индексами 7, 8, 9, 10.
        // В hex-строке "76310826101000000007632100000001A7BE00"
        //               76310826101000000007632100000001A7F000
        // Данные FF "00000000" (символы 13-20)
        // Данные CF1 "00000001A7BE00" (символы 29-42)
        // Байт d[7]=00 (символ 29,30), d[8]=01 (символ 31,32), d[9]=A7 (символ 33,34), d[10]=BE (символ 35,36)
        // Нам нужны символы 31,32,33,34,35,36,37,38 -> "0001A7BE" (8 hex символов)
        const int ODO_HEX_START_INDEX = 28; // Жестко заданное смещение для начала 4 байт данных
        const int ODO_HEX_LENGTH = 8;       // 4 байта = 8 hex символов

        if (response_len > (ODO_HEX_START_INDEX + ODO_HEX_LENGTH - 1)) {
            // --- Использование массива char вместо std::string ---
            // Создаем временный массив char для хранения 8 hex символов + завершающий ноль
            char odo_hex_chars[9]; // 8 символов + '\0'

            // Копируем 8 символов из raw_response, начиная с ODO_HEX_START_INDEX
            // strncpy копирует ровно n символов, не добавляя '\0' автоматически, если в источнике нет '\0'
            strncpy(odo_hex_chars, raw_response + ODO_HEX_START_INDEX, ODO_HEX_LENGTH);

            // Вручную завершаем строку нулевым символом
            odo_hex_chars[ODO_HEX_LENGTH] = '\0';

            ESP_LOGD(TAG, "Extracted Odometer hex chars (4 bytes): %s", odo_hex_chars);

            // --- Преобразование 4 байт hex-строки в число ---
            char* endptr;
            // Используем strtoul для преобразования 4-байтного hex значения
            // Проверяем, что вся строка была преобразована успешно
            unsigned long odo_raw = strtoul(odo_hex_chars, &endptr, 16);

            if (*endptr == '\0' && odo_raw <= 0xFFFFFFFF) { // Проверка корректности
                // odo_raw теперь содержит значение 0x0001A7BE = 108478
                float odometer_val = static_cast<float>(odo_raw); // 108478.0

                odometer_->publish_state(odometer_val);
                ESP_LOGD(TAG, "Odometer (4 bytes, char array): %.0f km", odometer_val);
            } else {
                ESP_LOGW(TAG, "Failed to convert Odometer hex chars '%s'", odo_hex_chars);
                odometer_->publish_state(NAN);
            }
        } else {
            ESP_LOGW(TAG, "Odometer response too short to extract 4-byte data at expected offset: %s", raw_response);
            odometer_->publish_state(NAN);
        }

    } else {
        ESP_LOGW(TAG, "Failed to query Odometer");
        odometer_->publish_state(NAN);
    }
}






/*
      if (odometer_ && elm_.connected()) {
        if (elm_.queryUDS("743", "022110")) {  // 743 - M&A (Meter)
          const char* buffer = elm_.get_response_buffer();
          if (strlen(buffer) >= 15 && strstr(buffer, "763") && !strstr(buffer, "7F")) {
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
*/
      } // от if setECU("743")
      state = 3;
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