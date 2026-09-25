import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID
DEPENDENCIES = ["uart", "wifi"]
ns = cg.esphome_ns.namespace("uart_sniffer")
Sniffer = ns.class_("Sniffer", cg.Component)
CONFIG_SCHEMA = cv.Schema({cv.GenerateID(): cv.declare_id(Sniffer),cv.Required("rx_a"):cv.use_id(uart.UARTComponent),cv.Required("rx_b"):cv.use_id(uart.UARTComponent),cv.Required("password"):cv.string}).extend(cv.COMPONENT_SCHEMA)
async def to_code(config):
    cg.add_library("WiFi", None)
    cg.add_library("Network", None)
    cg.add_library("WebServer", None)
    cg.add_library("FS", None)
    v=cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(v,config)
    for name in ("rx_a","rx_b"):
        bus=await cg.get_variable(config[name])
        cg.add(getattr(v,"set_"+name)(bus))
    cg.add(v.set_password(config["password"]))
