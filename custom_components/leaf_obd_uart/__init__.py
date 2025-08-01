import esphome.codegen as cg
import esphome.config_validation as cv
# from esphome.components import uart, sensor, text_sensor # Старая строка
from esphome.components import uart, sensor, text_sensor, binary_sensor # Новая строка
from esphome.const import (
    CONF_ID,
    CONF_UART_ID,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_DISTANCE,
    DEVICE_CLASS_POWER,        # Добавлено
    DEVICE_CLASS_CURRENT,      # Добавлено
    DEVICE_CLASS_ENERGY,       # Добавлено (для счетчиков зарядок)
    STATE_CLASS_MEASUREMENT,   # Добавлено
    STATE_CLASS_TOTAL_INCREASING, # Добавлено (для счетчиков)
    UNIT_PERCENT,
    UNIT_VOLT,
    UNIT_CELSIUS,
    UNIT_KILOMETER,
    UNIT_WATT,          # Добавлено
    UNIT_AMPERE,        # Добавлено
    UNIT_EMPTY,         # Добавлено (для текстовых сенсоров)
    ICON_BATTERY,
    ICON_FLASH,
    ICON_THERMOMETER,
    ICON_GAUGE,         # Добавлено
    CONF_NAME,          # Добавлено для текстовых сенсоров
    CONF_UPDATE_INTERVAL
)

# Определяем пространство имен и класс
leaf_obd_uart_ns = cg.esphome_ns.namespace("leaf_obd_uart")
LeafObdComponent = leaf_obd_uart_ns.class_("LeafObdComponent", cg.PollingComponent, uart.UARTDevice)

DEPENDENCIES = ["uart", "sensor"]
# Добавляем text_sensor в зависимости и автозагрузку
#AUTO_LOAD = ["sensor", "text_sensor"] 
AUTO_LOAD = ["sensor", "text_sensor", "binary_sensor"] # <-- Добавлено

# --- Определяем константы для новых сенсоров ---
# Существующие
CONF_SOC = "soc"
CONF_HV = "hv" # HV Voltage
CONF_TEMP = "temp" # Battery Temp Avg (или первый, если упрощено)
CONF_SOH = "soh"
CONF_AHR = "ahr"
CONF_ODOMETER = "odometer"
# Новые числовые сенсоры
CONF_BAT_12V_VOLTAGE = "bat_12v_voltage"
CONF_BAT_12V_CURRENT = "bat_12v_current"
CONF_QUICK_CHARGES = "quick_charges"
CONF_L1_L2_CHARGES = "l1_l2_charges"
CONF_AMBIENT_TEMP = "ambient_temp"
CONF_ESTIMATED_AC_POWER = "estimated_ac_power"
CONF_AUX_POWER = "aux_power"
CONF_AC_POWER = "ac_power"
CONF_POWER_SWITCH = "power_switch" # Логический сенсор
# Новые текстовые сенсоры
CONF_PLUG_STATE = "plug_state"
CONF_CHARGE_MODE = "charge_mode"
# --- Возможно, добавить температуры отдельно, если нужно ---
CONF_BATTERY_TEMP_1 = "battery_temp_1"
CONF_BATTERY_TEMP_2 = "battery_temp_2"
CONF_BATTERY_TEMP_3 = "battery_temp_3"
CONF_BATTERY_TEMP_4 = "battery_temp_4"

# --- Определяем схему для числовых сенсоров ---
# Улучшаем схему с более точными параметрами
SENSOR_SCHEMA = cv.Schema({
    # Существующие
    cv.Optional(CONF_SOC): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        icon="mdi:battery",
        accuracy_decimals=2, # Более точный SOC
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_HV): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        icon=ICON_FLASH,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_TEMP): sensor.sensor_schema( # Battery Temp
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_SOH): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        icon=ICON_GAUGE,
        accuracy_decimals=2, # Более точный SOH
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_AHR): sensor.sensor_schema(
        unit_of_measurement="Ah",
        icon=ICON_BATTERY,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_ENERGY, # Или UNIT_ENERGY_STORAGE
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_ODOMETER): sensor.sensor_schema(
        unit_of_measurement=UNIT_KILOMETER,
        icon="mdi:counter", # Или mdi:map-marker-distance
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_DISTANCE,
        state_class=STATE_CLASS_TOTAL_INCREASING, # Счетчик
    ),
    # Новые
    cv.Optional(CONF_BAT_12V_VOLTAGE): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        icon="mdi:car-battery", # Или 
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_BAT_12V_CURRENT): sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        icon="mdi:current-dc", # Или 
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_QUICK_CHARGES): sensor.sensor_schema(
        unit_of_measurement=" ",
        icon="mdi:lightning-bolt-circle", # Или 
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_ENERGY, # UNIT_ENERGY лучше подходит для счетчиков?
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional(CONF_L1_L2_CHARGES): sensor.sensor_schema(
        unit_of_measurement=" ",
        icon="mdi:plug", # Или 
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_ENERGY,
        state_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional(CONF_AMBIENT_TEMP): sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        icon=ICON_THERMOMETER,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_ESTIMATED_AC_POWER): sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        icon=ICON_GAUGE,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_AUX_POWER): sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        icon=ICON_GAUGE,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_AC_POWER): sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        icon=ICON_GAUGE,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    #cv.Optional(CONF_POWER_SWITCH): sensor.sensor_schema(
    #    unit_of_measurement=UNIT_EMPTY, # Логический сенсор
    #    icon="mdi:power", # Или mdi:toggle-switch
    #    accuracy_decimals=0, # 0 или 1
    #    device_class=DEVICE_CLASS_EMPTY, # Нет стандартного класса для bool
    #    state_class=STATE_CLASS_MEASUREMENT,
    #),
    # cv.Optional(CONF_BATTERY_TEMP_1): sensor.sensor_schema(...),
    # cv.Optional(CONF_BATTERY_TEMP_2): sensor.sensor_schema(...),
    # cv.Optional(CONF_BATTERY_TEMP_3): sensor.sensor_schema(...),
    # cv.Optional(CONF_BATTERY_TEMP_4): sensor.sensor_schema(...),
    cv.Optional(CONF_BATTERY_TEMP_1): sensor.sensor_schema(
        unit_of_measurement="°C",
        icon="mdi:thermometer",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional(CONF_BATTERY_TEMP_2): sensor.sensor_schema(
        unit_of_measurement="°C",
        icon="mdi:thermometer",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional(CONF_BATTERY_TEMP_3): sensor.sensor_schema(
        unit_of_measurement="°C",
        icon="mdi:thermometer",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional(CONF_BATTERY_TEMP_4): sensor.sensor_schema(
        unit_of_measurement="°C",
        icon="mdi:thermometer",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
})

# --- Определяем схему для текстовых сенсоров ---
TEXT_SENSOR_SCHEMA = cv.Schema({
    cv.Optional(CONF_PLUG_STATE): text_sensor.TEXT_SENSOR_SCHEMA.extend({
        cv.Optional(CONF_NAME, default="Plug State"): cv.string_strict,
    }),
    cv.Optional(CONF_CHARGE_MODE): text_sensor.TEXT_SENSOR_SCHEMA.extend({
        cv.Optional(CONF_NAME, default="Charge Mode"): cv.string_strict,
    }),
})

# --- Определяем схему для бинарных сенсоров ---
# Добавляем импорт вверху: from esphome.components import ..., binary_sensor
BINARY_SENSOR_SCHEMA = cv.Schema({
    cv.Optional(CONF_POWER_SWITCH): binary_sensor.BINARY_SENSOR_SCHEMA.extend({
        # cv.Optional(CONF_NAME, default="Power Switch"): cv.string_strict, # Имя обычно из YAML
        cv.Optional("icon", default="mdi:power"): cv.icon, # Можно задать дефолт
        # device_class для binary_sensor обычно не указывается для таких кастомных флагов
        # или используется, если подходит (например, DEVICE_CLASS_POWER, но это редкость)
    }),
    # Добавьте другие бинарные сенсоры при необходимости
})

# --- Основная схема конфигурации ---
#CONFIG_SCHEMA = cv.Schema({
#    cv.GenerateID(): cv.declare_id(LeafObdComponent),
#    cv.Optional(CONF_UPDATE_INTERVAL, default="30s"): cv.positive_time_period_milliseconds,
#}).extend(cv.polling_component_schema("30s")).extend(SENSOR_SCHEMA).extend(TEXT_SENSOR_SCHEMA).extend(uart.UART_DEVICE_SCHEMA) # Добавлен TEXT_SENSOR_SCHEMA

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(LeafObdComponent),
    cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
    cv.Optional(CONF_UPDATE_INTERVAL, default="30s"): cv.positive_time_period_milliseconds,
}).extend(cv.polling_component_schema("30s")).extend(SENSOR_SCHEMA).extend(TEXT_SENSOR_SCHEMA).extend(BINARY_SENSOR_SCHEMA).extend(uart.UART_DEVICE_SCHEMA) # <-- Добавлен BINARY_SENSOR_SCHEMA



async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    # Устанавливаем интервал обновления
    if CONF_UPDATE_INTERVAL in config:
        cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))

    # --- Регистрируем числовые сенсоры ---
    # Существующие
    if CONF_SOC in config:
        soc_sensor = await sensor.new_sensor(config[CONF_SOC])
        cg.add(var.set_soc_sensor(soc_sensor))
    if CONF_HV in config:
        hv_sensor = await sensor.new_sensor(config[CONF_HV])
        cg.add(var.set_hv_sensor(hv_sensor))
    if CONF_TEMP in config: # Battery Temp
        temp_sensor = await sensor.new_sensor(config[CONF_TEMP])
        cg.add(var.set_temp_sensor(temp_sensor))
    if CONF_SOH in config:
        soh_sensor = await sensor.new_sensor(config[CONF_SOH])
        cg.add(var.set_soh_sensor(soh_sensor))
    if CONF_AHR in config:
        ahr_sensor = await sensor.new_sensor(config[CONF_AHR])
        cg.add(var.set_ahr_sensor(ahr_sensor))
    if CONF_ODOMETER in config:
        odometer_sensor = await sensor.new_sensor(config[CONF_ODOMETER])
        cg.add(var.set_odometer_sensor(odometer_sensor))
    # Новые
    if CONF_BAT_12V_VOLTAGE in config:
        bat_12v_voltage_sensor = await sensor.new_sensor(config[CONF_BAT_12V_VOLTAGE])
        cg.add(var.set_bat_12v_voltage_sensor(bat_12v_voltage_sensor))
    if CONF_BAT_12V_CURRENT in config:
        bat_12v_current_sensor = await sensor.new_sensor(config[CONF_BAT_12V_CURRENT])
        cg.add(var.set_bat_12v_current_sensor(bat_12v_current_sensor))
    if CONF_QUICK_CHARGES in config:
        quick_charges_sensor = await sensor.new_sensor(config[CONF_QUICK_CHARGES])
        cg.add(var.set_quick_charges_sensor(quick_charges_sensor))
    if CONF_L1_L2_CHARGES in config:
        l1_l2_charges_sensor = await sensor.new_sensor(config[CONF_L1_L2_CHARGES])
        cg.add(var.set_l1_l2_charges_sensor(l1_l2_charges_sensor))
    if CONF_AMBIENT_TEMP in config:
        ambient_temp_sensor = await sensor.new_sensor(config[CONF_AMBIENT_TEMP])
        cg.add(var.set_ambient_temp_sensor(ambient_temp_sensor))
    if CONF_ESTIMATED_AC_POWER in config:
        estimated_ac_power_sensor = await sensor.new_sensor(config[CONF_ESTIMATED_AC_POWER])
        cg.add(var.set_estimated_ac_power_sensor(estimated_ac_power_sensor))
    if CONF_AUX_POWER in config:
        aux_power_sensor = await sensor.new_sensor(config[CONF_AUX_POWER])
        cg.add(var.set_aux_power_sensor(aux_power_sensor))
    if CONF_AC_POWER in config:
        ac_power_sensor = await sensor.new_sensor(config[CONF_AC_POWER])
        cg.add(var.set_ac_power_sensor(ac_power_sensor))
    #if CONF_POWER_SWITCH in config:
    #    power_switch_sensor = await sensor.new_sensor(config[CONF_POWER_SWITCH])
    #    cg.add(var.set_power_switch_sensor(power_switch_sensor))
    # if CONF_BATTERY_TEMP_1 in config:
    #     battery_temp_1_sensor = await sensor.new_sensor(config[CONF_BATTERY_TEMP_1])
    #     cg.add(var.set_battery_temp_1_sensor(battery_temp_1_sensor))
    # ... и так далее для остальных температур
    if CONF_BATTERY_TEMP_1 in config:
        battery_temp_1_sensor = await sensor.new_sensor(config[CONF_BATTERY_TEMP_1])
        cg.add(var.set_battery_temp_1_sensor(battery_temp_1_sensor))
    if CONF_BATTERY_TEMP_2 in config:
        battery_temp_2_sensor = await sensor.new_sensor(config[CONF_BATTERY_TEMP_2])
        cg.add(var.set_battery_temp_2_sensor(battery_temp_2_sensor))
    if CONF_BATTERY_TEMP_3 in config:
        battery_temp_3_sensor = await sensor.new_sensor(config[CONF_BATTERY_TEMP_3])
        cg.add(var.set_battery_temp_3_sensor(battery_temp_3_sensor))
    if CONF_BATTERY_TEMP_4 in config:
        battery_temp_4_sensor = await sensor.new_sensor(config[CONF_BATTERY_TEMP_4])
        cg.add(var.set_battery_temp_4_sensor(battery_temp_4_sensor))

    # --- Регистрируем текстовые сенсоры ---
    if CONF_PLUG_STATE in config:
        plug_state_sensor = await text_sensor.new_text_sensor(config[CONF_PLUG_STATE])
        cg.add(var.set_plug_state_sensor(plug_state_sensor))
    if CONF_CHARGE_MODE in config:
        charge_mode_sensor = await text_sensor.new_text_sensor(config[CONF_CHARGE_MODE])
        cg.add(var.set_charge_mode_sensor(charge_mode_sensor))

    # --- Регистрируем бинарные сенсоры ---
    if CONF_POWER_SWITCH in config:
        power_switch_sensor = await binary_sensor.new_binary_sensor(config[CONF_POWER_SWITCH])
        cg.add(var.set_power_switch_sensor(power_switch_sensor)) # Убедитесь, что метод set_... в .h файле принимает binary_sensor::BinarySensor*
