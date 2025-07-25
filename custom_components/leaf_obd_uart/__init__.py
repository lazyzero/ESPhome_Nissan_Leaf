import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import CONF_ID, CONF_UART_ID, DEVICE_CLASS_BATTERY, DEVICE_CLASS_VOLTAGE, DEVICE_CLASS_TEMPERATURE, DEVICE_CLASS_DISTANCE, CONF_UPDATE_INTERVAL

# Определяем пространство имен и класс
leaf_obd_uart_ns = cg.esphome_ns.namespace("leaf_obd_uart")
LeafObdComponent = leaf_obd_uart_ns.class_("LeafObdComponent", cg.PollingComponent, uart.UARTDevice)

DEPENDENCIES = ["uart", "sensor"]
AUTO_LOAD = ["sensor"]

CONF_SOC = "soc"
CONF_HV = "hv"
CONF_TEMP = "temp"
CONF_SOH = "soh"
CONF_AHR = "ahr"
CONF_ODOMETER = "odometer"

# Определяем схему для сенсоров
SENSOR_SCHEMA = cv.Schema({
    cv.Optional(CONF_SOC): sensor.sensor_schema(
        unit_of_measurement="%",
        icon="mdi:battery",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_BATTERY,
    ),
    cv.Optional(CONF_HV): sensor.sensor_schema(
        unit_of_measurement="V",
        icon="mdi:flash",
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_VOLTAGE,
    ),
    cv.Optional(CONF_TEMP): sensor.sensor_schema(
        unit_of_measurement="°C",
        icon="mdi:thermometer",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ),
    cv.Optional(CONF_SOH): sensor.sensor_schema(
        unit_of_measurement="%",
        icon="mdi:battery-heart",
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_BATTERY,
    ),
    cv.Optional(CONF_AHR): sensor.sensor_schema(
        unit_of_measurement="Ah",
        icon="mdi:battery-heart",
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_BATTERY,
    ),
    cv.Optional(CONF_ODOMETER): sensor.sensor_schema(
        unit_of_measurement="km",
        icon="mdi:counter",
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_DISTANCE,
    ),
})

# Определяем схему для компонента
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(LeafObdComponent),
    cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
    cv.Optional(CONF_UPDATE_INTERVAL, default="30s"): cv.positive_time_period_milliseconds,
}).extend(cv.polling_component_schema("30s")).extend(SENSOR_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    
    # Устанавливаем интервал обновления
    if CONF_UPDATE_INTERVAL in config:
        cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    
    # Регистрируем сенсоры
    if CONF_SOC in config:
        soc_sensor = await sensor.new_sensor(config[CONF_SOC])
        cg.add(var.set_soc_sensor(soc_sensor))
    if CONF_HV in config:
        hv_sensor = await sensor.new_sensor(config[CONF_HV])
        cg.add(var.set_hv_sensor(hv_sensor))
    if CONF_TEMP in config:
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