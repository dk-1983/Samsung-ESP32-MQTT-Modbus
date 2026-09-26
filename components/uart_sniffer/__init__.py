import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID
DEPENDENCIES = ["uart", "wifi"]
ns = cg.esphome_ns.namespace("uart_sniffer")
Sniffer = ns.class_("Sniffer", cg.Component)
CONFIG_SCHEMA = cv.Schema({cv.GenerateID(): cv.declare_id(Sniffer),cv.Required("rx_a"):cv.use_id(uart.UARTComponent),cv.Required("rx_b"):cv.use_id(uart.UARTComponent),cv.Optional("rx_b_gpio"):cv.one_of(8,9,17,int=True),cv.Optional("rx_c"):cv.use_id(uart.UARTComponent),cv.Required("password"):cv.string,cv.Optional("bridge", default=False):cv.boolean,cv.Optional("monitor_only", default=False):cv.boolean}).extend(cv.COMPONENT_SCHEMA)
def validate_observer(config):
    if "rx_b_gpio" in config and not config["monitor_only"]:
        raise cv.Invalid("rx_b_gpio override is only allowed in monitor-only diagnostics")
    if "rx_c" in config and not (config["bridge"] and config["monitor_only"]):
        raise cv.Invalid("rx_c is only allowed in monitor-only bridge diagnostics")
    return config
CONFIG_SCHEMA = cv.All(CONFIG_SCHEMA, validate_observer)

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
    if "rx_c" in config:
        bus=await cg.get_variable(config["rx_c"])
        cg.add(v.set_rx_c(bus))
    if "rx_b_gpio" in config:
        cg.add(v.set_rx_b_gpio(config["rx_b_gpio"]))
    cg.add(v.set_password(config["password"]))
    cg.add(v.set_bridge(config["bridge"]))
    cg.add(v.set_monitor_only(config["monitor_only"]))
