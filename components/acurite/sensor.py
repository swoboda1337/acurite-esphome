import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
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

CONF_RAIN = 'rain'
UNIT_MILLIMETER = "mm"
CONF_PIN = 'pin'

acurite_ns = cg.esphome_ns.namespace("acurite")
AcuRite = acurite_ns.class_("AcuRite", cg.Component)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AcuRite),
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
            cv.Required(CONF_PIN): pins.internal_gpio_input_pin_schema,
        }
    )
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if temperature_config := config.get(CONF_TEMPERATURE):
        sens = await sensor.new_sensor(temperature_config)
        cg.add(var.set_temperature_sensor(sens))
    if humidity_config := config.get(CONF_HUMIDITY):
        sens = await sensor.new_sensor(humidity_config)
        cg.add(var.set_humidity_sensor(sens))
    if rain_config := config.get(CONF_RAIN):
        sens = await sensor.new_sensor(rain_config)
        cg.add(var.set_rain_sensor(sens))
    if pin_cfg := config.get(CONF_PIN):
        pin = await cg.gpio_pin_expression(pin_cfg)
        cg.add(var.set_pin(pin))


