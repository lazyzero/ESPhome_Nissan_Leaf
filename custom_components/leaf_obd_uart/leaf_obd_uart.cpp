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
  if (!sendCommand("ATL0") || !readResponse(200)) return false;  // Отключить перевод строк
  if (!sendCommand("ATSP6") || !readResponse(250)) return false; // CAN 500 кбит/с
  if (!sendCommand("ATH1") || !readResponse(200)) return false;  // Включить заголовки
  if (!sendCommand("ATS0") || !readResponse(200)) return false;  // Отключить пробелы
  if (!sendCommand("ATCAF0") || !readResponse(200)) return false; // Отключить автоформатирование
  if (!sendCommand("ATSH797") || !readResponse(200)) return false;  // Set Header to 
  if (!sendCommand("ATFCSH797") || !readResponse(200)) return false;  // FC, Set the Header to 
  if (!sendCommand("ATFCSD300000") || !readResponse(200)) return false;  // FC, Set Data to [...]
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
  return true;
}

bool ELM327::connected() {
  return is_connected_ && elm_stream_ != nullptr;
}

bool ELM327::queryUDS(const char *ecu, const char *pid, int retries) {
//  for (int i = 0; i < retries; i++) {
// Убираем несколько переподключений, так как в коде это не используется, да и лишние задержки
    response_buffer_[0] = '\0';
    if (!setECU(ecu)) {
      ESP_LOGW(TAG, "Retry %d: Failed to set ECU: %s", ecu);
//      ESP_LOGW(TAG, "Retry %d: Failed to set ECU: %s", i + 1, ecu);
//      continue;
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
    if (!sendCommand(pid) || !readResponse(1000)) {
      ESP_LOGW(TAG, "Retry %d: No response to PID: %s", pid);
//      ESP_LOGW(TAG, "Retry %d: No response to PID: %s", i + 1, pid);
//      continue;
    }
  // --- Специальная инициализация для ECU 743 ---
  // Проверяем, является ли текущий ECU целевым (743)
  if (strcmp(ecu, "79B") == 0) {
    ESP_LOGD(TAG, "Performing special initialization for ECU 79B");
    if (!sendCommand("ATFCSD300000") || !readResponse(500)) {
      ESP_LOGW(TAG, "Failed to send/set Flow Control Data '300000' for ECU 79B");
    }
    if (!sendCommand("0210C0") || !readResponse(1000)) { // Таймаут побольше для "mystery" команды
      ESP_LOGW(TAG, "No response to Mystery Command '0210C0' for ECU 79B");
    }
  }
    if (!sendCommand(pid) || !readResponse(1000)) {
      ESP_LOGW(TAG, "Retry %d: No response to PID: %s", pid);
//      ESP_LOGW(TAG, "Retry %d: No response to PID: %s", i + 1, pid);
//      continue;
    }
//    if (strstr(response_buffer_, "NO DATA") || strstr(response_buffer_, "7F") || strstr(response_buffer_, "CAN ERROR")) {
// ищет 7F, и естественно в многострочном ответе находит его. Надо проверять на конкретном месте. Потом переделаю, а пока убрал проверку.
    if (strstr(response_buffer_, "NO DATA") || strstr(response_buffer_, "CAN ERROR")) {
      ESP_LOGW(TAG, "Retry %d: Invalid response for PID: %s, response: %s", pid, response_buffer_);
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

    case 3: // 3 - Запрос Number of quick charges / L1/L2 charges
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
      state = 0;
      break;

    case 1: // Запрос Voltage, Temp, SOH, AHr
      if (elm_.connected() && elm_.setECU("79B")) {       // 79B - LBC(BMS)

        if (elm_.queryUDS("79B", "022101")) {


// Предполагаем, что elm_.queryUDS("79B", "022101") уже был вызван.
const char* raw_response = elm_.get_response_buffer(); // Или просто response_buffer_
int response_len = strlen(raw_response);

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

      }
      state = 2;
      break;

    case 2: // Запрос Odometer

if (odometer_ && elm_.connected()) {
    if (elm_.queryUDS("743", "022101")) {  // 743 - M&A (Meter)
        const char* raw_response = elm_.get_response_buffer();
        int response_len = strlen(raw_response);

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