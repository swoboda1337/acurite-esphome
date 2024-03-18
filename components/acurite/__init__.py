import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import time
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
)

DEPENDENCIES = ["time"]

CODEOWNERS = ["@swoboda1337"]

CONF_PIN = 'pin'

acurite_ns = cg.esphome_ns.namespace("acurite")
AcuRite = acurite_ns.class_("AcuRite", cg.Component)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AcuRite),
            cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            cv.Required(CONF_PIN): pins.internal_gpio_input_pin_schema,
        }
    )
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    clock = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_srctime(clock))
    if pin_cfg := config.get(CONF_PIN):
        pin = await cg.gpio_pin_expression(pin_cfg)
        cg.add(var.set_pin(pin))
