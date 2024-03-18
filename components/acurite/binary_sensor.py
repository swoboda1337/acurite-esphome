import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import binary_sensor
from esphome.components import time
from esphome.const import (
    DEVICE_CLASS_MOISTURE
)
from . import AcuRite

DEPENDENCIES = ["acurite"]

CONF_ACURITE_ID = 'acurite_id'
CONF_RAIN = 'rain'

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(CONF_ACURITE_ID): cv.use_id(AcuRite),
            cv.Required(CONF_RAIN): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_MOISTURE,
            ),
        }
    )
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_ACURITE_ID])
    sens = await binary_sensor.new_binary_sensor(config.get(CONF_RAIN))
    cg.add(parent.set_rainfall_sensor(sens))
