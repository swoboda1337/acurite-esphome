import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import sensor
from esphome.components import time
from esphome.const import (
    CONF_TEMPERATURE,
    CONF_HUMIDITY,
    CONF_DISTANCE,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_PRECIPITATION,
    DEVICE_CLASS_DISTANCE,
    DEVICE_CLASS_EMPTY,
    DEVICE_CLASS_WIND_SPEED,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    STATE_CLASS_TOTAL,
    UNIT_DEGREES,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_KILOMETER,
    UNIT_KILOMETER_PER_HOUR,
    UNIT_EMPTY,
)
from . import AcuRite

DEPENDENCIES = ["acurite"]

CONF_ACURITE_ID = 'acurite_id'
CONF_DEVICES = 'devices'
CONF_DEVICE = 'device'
CONF_RAIN = 'rain'
CONF_WIND_SPEED = 'wind_speed'
CONF_WIND_DIRECTION = 'wind_direction'
CONF_LIGHTNING = 'lightning'
UNIT_MILLIMETER = "mm"

DEVICE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DEVICE): cv.hex_int_range(max=0x3FFF),
        cv.Optional(CONF_WIND_SPEED): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOMETER_PER_HOUR,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_WIND_SPEED,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_WIND_DIRECTION): sensor.sensor_schema(
            unit_of_measurement=UNIT_DEGREES,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
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
        cv.Optional(CONF_RAIN): sensor.sensor_schema(
            unit_of_measurement=UNIT_MILLIMETER,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_PRECIPITATION,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_DISTANCE): sensor.sensor_schema(
            unit_of_measurement=UNIT_KILOMETER,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DISTANCE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_LIGHTNING): sensor.sensor_schema(
            unit_of_measurement=UNIT_EMPTY,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_EMPTY,
            state_class=STATE_CLASS_TOTAL,
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
            if CONF_WIND_SPEED in device_cfg:
                sens = await sensor.new_sensor(device_cfg[CONF_WIND_SPEED])
                cg.add(parent.add_wind_speed_sensor(device_cfg[CONF_DEVICE], sens))
            if CONF_TEMPERATURE in device_cfg:
                sens = await sensor.new_sensor(device_cfg[CONF_TEMPERATURE])
                cg.add(parent.add_temperature_sensor(device_cfg[CONF_DEVICE], sens))
            if CONF_HUMIDITY in device_cfg:
                sens = await sensor.new_sensor(device_cfg[CONF_HUMIDITY])
                cg.add(parent.add_humidity_sensor(device_cfg[CONF_DEVICE], sens))
            if CONF_RAIN in device_cfg:
                sens = await sensor.new_sensor(device_cfg[CONF_RAIN])
                cg.add(parent.add_rainfall_sensor(device_cfg[CONF_DEVICE], sens))
            if CONF_LIGHTNING in device_cfg:
                sens = await sensor.new_sensor(device_cfg[CONF_LIGHTNING])
                cg.add(parent.add_lightning_sensor(device_cfg[CONF_DEVICE], sens))
            if CONF_DISTANCE in device_cfg:
                sens = await sensor.new_sensor(device_cfg[CONF_DISTANCE])
                cg.add(parent.add_distance_sensor(device_cfg[CONF_DEVICE], sens))
