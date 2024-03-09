import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_TEMPERATURE,
    CONF_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_PRECIPITATION,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

CONF_DEVICES = 'devices'
CONF_DEVICE_ID = 'device_id'
CONF_RAIN = 'rain'
UNIT_MILLIMETER = "mm"
CONF_PIN = 'pin'

acurite_ns = cg.esphome_ns.namespace("acurite")
AcuRite = acurite_ns.class_("AcuRite", cg.Component)
AcuRiteResetRainfallAction = acurite_ns.class_("AcuRiteResetRainfallAction", automation.Action)

DEVICE_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_DEVICE_ID): cv.hex_int_range(max=0x3FFF),
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
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_PRECIPITATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AcuRite),
            cv.Required(CONF_PIN): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_DEVICES): cv.ensure_list(DEVICE_SCHEMA),
        }
    )
)

CALIBRATION_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(AcuRite),
    }
)

@automation.register_action(
    "acurite.reset_rainfall", AcuRiteResetRainfallAction, CALIBRATION_ACTION_SCHEMA
)

async def acurite_reset_rain_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if pin_cfg := config.get(CONF_PIN):
        pin = await cg.gpio_pin_expression(pin_cfg)
        cg.add(var.set_pin(pin))
    for device_conf in config[CONF_DEVICES]:
        if CONF_TEMPERATURE in device_conf:
            sens = await sensor.new_sensor(device_conf[CONF_TEMPERATURE])
            cg.add(var.add_temperature_sensor(sens, device_conf[CONF_DEVICE_ID]))
        if CONF_HUMIDITY in device_conf:
            sens = await sensor.new_sensor(device_conf[CONF_HUMIDITY])
            cg.add(var.add_humidity_sensor(sens, device_conf[CONF_DEVICE_ID]))
        if CONF_RAIN in device_conf:
            sens = await sensor.new_sensor(device_conf[CONF_RAIN])
            cg.add(var.add_rain_sensor(sens, device_conf[CONF_DEVICE_ID]))
