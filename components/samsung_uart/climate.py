import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart", "wifi"]
ns = cg.esphome_ns.namespace("samsung_uart")
SamsungClimate = ns.class_("SamsungClimate", climate.Climate, cg.Component, uart.UARTDevice)
CONFIG_SCHEMA = climate.climate_schema(SamsungClimate).extend({
    cv.Required("rs485_uart_id"): cv.use_id(uart.UARTComponent),
    cv.Optional("factory_uart_id"): cv.use_id(uart.UARTComponent),
    cv.Optional("modbus_unit", default=1): cv.int_range(min=1, max=247),
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    cg.add_library("WiFi", None)
    cg.add_library("Network", None)
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)
    rs485 = await cg.get_variable(config["rs485_uart_id"])
    cg.add(var.set_rs485(rs485))
    if "factory_uart_id" in config:
        factory = await cg.get_variable(config["factory_uart_id"])
        cg.add(var.set_factory_uart(factory))
    cg.add(var.set_unit(config["modbus_unit"]))
