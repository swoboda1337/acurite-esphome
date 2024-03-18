import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import sensor
from esphome.components import time
from esphome.const import (
    CONF_TEMPERATURE,
    CONF_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_PRECIPITATION,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)
from . import AcuRite

DEPENDENCIES = ["acurite"]

CONF_ACURITE_ID = 'acurite_id'
CONF_DEVICES = 'devices'
CONF_DEVICE = 'device'
CONF_RAIN_1HR = 'rain_1hr'
CONF_RAIN_24HR = 'rain_24hr'
CONF_RAIN_DAILY = 'rain_daily'
UNIT_MILLIMETER = "mm"

DEVICE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DEVICE): cv.hex_int_range(max=0x3FFF),
        cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_HUMIDITY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_RAIN_1HR): sensor.sensor_schema(
            unit_of_measurement=UNIT_MILLIMETER,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_PRECIPITATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_RAIN_24HR): sensor.sensor_schema(
            unit_of_measurement=UNIT_MILLIMETER,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_PRECIPITATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_RAIN_DAILY): sensor.sensor_schema(
            unit_of_measurement=UNIT_MILLIMETER,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_PRECIPITATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(CONF_ACURITE_ID): cv.use_id(AcuRite),
            cv.Required(CONF_DEVICES): cv.ensure_list(DEVICE_SCHEMA),
        }
    )
)

async def to_code(config):
    parent = await cg.get_variable(config[CONF_ACURITE_ID])
    if devices_cfg := config.get(CONF_DEVICES):
        for device_cfg in devices_cfg:
            cg.add(parent.add_device(device_cfg[CONF_DEVICE]))
            if CONF_TEMPERATURE in device_cfg:
                sens = await sensor.new_sensor(device_cfg[CONF_TEMPERATURE])
                cg.add(parent.add_temperature_sensor(device_cfg[CONF_DEVICE], sens))
            if CONF_HUMIDITY in device_cfg:
                sens = await sensor.new_sensor(device_cfg[CONF_HUMIDITY])
                cg.add(parent.add_humidity_sensor(device_cfg[CONF_DEVICE], sens))
            if CONF_RAIN_1HR in device_cfg:
                sens = await sensor.new_sensor(device_cfg[CONF_RAIN_1HR])
                cg.add(parent.add_rainfall_sensor_1hr(device_cfg[CONF_DEVICE], sens))
            if CONF_RAIN_24HR in device_cfg:
                sens = await sensor.new_sensor(device_cfg[CONF_RAIN_24HR])
                cg.add(parent.add_rainfall_sensor_24hr(device_cfg[CONF_DEVICE], sens))
            if CONF_RAIN_DAILY in device_cfg:
                sens = await sensor.new_sensor(device_cfg[CONF_RAIN_DAILY])
                cg.add(parent.add_rainfall_sensor_daily(device_cfg[CONF_DEVICE], sens))
