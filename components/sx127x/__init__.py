import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi
from esphome.const import CONF_ID

CODEOWNERS = ["@swoboda1337"]

CONF_NSS_PIN = 'nss_pin'
CONF_RST_PIN = 'rst_pin'
CONF_DIO0_PIN = 'dio0_pin'
CONF_DIO1_PIN = 'dio1_pin'
CONF_DIO2_PIN = 'dio2_pin'

DEPENDENCIES = ["spi"]

sx127x_ns = cg.esphome_ns.namespace("sx127x")
SX127X = sx127x_ns.class_("SX127X", cg.Component, spi.SPIDevice)
SX127X_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_RST_PIN): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_NSS_PIN): pins.internal_gpio_output_pin_schema,
        cv.Optional(CONF_DIO0_PIN): pins.internal_gpio_input_pin_schema,
        cv.Optional(CONF_DIO1_PIN): pins.internal_gpio_input_pin_schema,
        cv.Required(CONF_DIO2_PIN): pins.internal_gpio_input_pin_schema,
    }
).extend(spi.spi_device_schema(False, 8e6, 'mode0'))

async def setup_sx127x(var, config):
    await spi.register_spi_device(var, config)
    rst_pin = await cg.gpio_pin_expression(config[CONF_RST_PIN])
    cg.add(var.set_rst_pin(rst_pin))
    nss_pin = await cg.gpio_pin_expression(config[CONF_NSS_PIN])
    cg.add(var.set_nss_pin(nss_pin))
    if dio0_cfg := config.get(CONF_DIO0_PIN):
        dio0_pin = await cg.gpio_pin_expression(dio0_cfg)
        cg.add(var.set_dio0_pin(dio2_pin))
    if dio1_cfg := config.get(CONF_DIO1_PIN):
        dio1_pin = await cg.gpio_pin_expression(dio1_cfg)
        cg.add(var.set_dio1_pin(dio2_pin))
    dio2_pin = await cg.gpio_pin_expression(config[CONF_DIO2_PIN])
    cg.add(var.set_dio2_pin(dio2_pin))
