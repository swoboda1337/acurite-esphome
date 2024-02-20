import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi, sensor
from esphome.const import (
    CONF_HUMIDITY,
    CONF_ID,
    CONF_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

CONF_NSS_PIN = 'nss_pin'
CONF_RST_PIN = 'rst_pin'
CONF_DIO2_PIN = 'dio2_pin'

DEPENDENCIES = ["spi"]

acurite_ns = cg.esphome_ns.namespace("acurite")
AcuRite = acurite_ns.class_(
    "AcuRite", cg.Component, spi.SPIDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AcuRite),
            cv.Required(CONF_RST_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_NSS_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_DIO2_PIN): pins.internal_gpio_input_pin_schema,
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
        }
    )
    .extend(spi.spi_device_schema(False, 8e6, 'mode0'))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await spi.register_spi_device(var, config)
    await cg.register_component(var, config)

    rst_pin = await cg.gpio_pin_expression(config[CONF_RST_PIN])
    cg.add(var.set_rst_pin(rst_pin))

    nss_pin = await cg.gpio_pin_expression(config[CONF_NSS_PIN])
    cg.add(var.set_nss_pin(nss_pin))

    dio2_pin = await cg.gpio_pin_expression(config[CONF_DIO2_PIN])
    cg.add(var.set_dio2_pin(dio2_pin))

    if temperature_config := config.get(CONF_TEMPERATURE):
        sens = await sensor.new_sensor(temperature_config)
        cg.add(var.set_temperature_sensor(sens))

    if humidity_config := config.get(CONF_HUMIDITY):
        sens = await sensor.new_sensor(humidity_config)
        cg.add(var.set_humidity_sensor(sens))
