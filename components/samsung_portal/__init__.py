import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32, socket
from esphome.components.samsung_uart.climate import SamsungClimate
from esphome.const import CONF_ID
from esphome.components.esphome.ota import ESPHomeOTAComponent
DEPENDENCIES = ['esp32', 'mqtt', 'wifi']
ns=cg.esphome_ns.namespace('samsung_portal')
Portal=ns.class_('Portal',cg.Component)
CONFIG_SCHEMA=cv.All(cv.Schema({
 cv.GenerateID():cv.declare_id(Portal),
 cv.Required('climate_id'):cv.use_id(SamsungClimate),
 cv.Required('ota_id'):cv.use_id(ESPHomeOTAComponent),
 cv.Required('web_password'):cv.All(cv.string_strict,cv.Length(min=8,max=64)),
 cv.Required('ota_password'):cv.All(cv.string_strict,cv.Length(min=16,max=64)),
 cv.Required('setup_password'):cv.All(cv.string_strict,cv.Length(min=8,max=63)),
 cv.Optional('public_release',default=False):cv.boolean,
 cv.Optional('test_update_feed',default=False):cv.boolean,
}).extend(cv.COMPONENT_SCHEMA), socket.consume_sockets(4,"samsung_portal"), socket.consume_sockets(2,"samsung_portal",socket.SocketType.TCP_LISTEN), socket.consume_sockets(1,"samsung_portal",socket.SocketType.UDP))
async def to_code(config):
 if config['test_update_feed']:
  if config['public_release']:raise ValueError('Public OTA cannot use testing feed')
  cg.add_define('SAMSUNG_TEST_FEED')
 var=cg.new_Pvariable(config[CONF_ID]);await cg.register_component(var,config)
 cg.add(var.set_climate(await cg.get_variable(config['climate_id'])))
 cg.add(var.set_ota(await cg.get_variable(config['ota_id'])))
 cg.add(var.set_password(config['web_password']))
 cg.add(var.set_initial_ota(config['ota_password']))
 cg.add(var.set_initial_setup(config['setup_password']))
 cg.add(var.set_public_release(config['public_release']))
 for lib in ['WiFi','WebServer','Network','Preferences','FS']:
  cg.add_library(lib,None)
 for comp in ['esp_http_client','esp_https_ota']:
  esp32.include_builtin_idf_component(comp)

