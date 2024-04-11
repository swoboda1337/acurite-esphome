import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi
from esphome.const import CONF_ID

CODEOWNERS = ["@swoboda1337"]

DEPENDENCIES = ["spi"]

CONF_NSS_PIN = 'nss_pin'
CONF_RST_PIN = 'rst_pin'
CONF_OOK_FLOOR = "ook_floor"
CONF_FREQUENCY = "frequency"
CONF_MODULATION = "modulation"
CONF_BANDWIDTH = "bandwidth"

sx127x_ns = cg.esphome_ns.namespace("sx127x")
SX127x = sx127x_ns.class_("SX127x", cg.Component, spi.SPIDevice)
Modulation = sx127x_ns.enum("SX127xMod")
Bandwidth = sx127x_ns.enum("SX127xRxBw")

MODULATION = {
    "FSK": Modulation.MODULATION_FSK,
    "OOK": Modulation.MODULATION_OOK,
}

BANDWIDTH = {
    "2_6kHz": Bandwidth.RX_BANDWIDTH_2_6,
    "3_1kHz": Bandwidth.RX_BANDWIDTH_3_1,
    "3_9kHz": Bandwidth.RX_BANDWIDTH_3_9,
    "5_2kHz": Bandwidth.RX_BANDWIDTH_5_2,
    "6_3kHz": Bandwidth.RX_BANDWIDTH_6_3,
    "7_8kHz": Bandwidth.RX_BANDWIDTH_7_8,
    "10_4kHz": Bandwidth.RX_BANDWIDTH_10_4,
    "12_5kHz": Bandwidth.RX_BANDWIDTH_12_5,
    "15_6kHz": Bandwidth.RX_BANDWIDTH_15_6,
    "20_8kHz": Bandwidth.RX_BANDWIDTH_20_8,
    "25_0kHz": Bandwidth.RX_BANDWIDTH_25_0,
    "31_3kHz": Bandwidth.RX_BANDWIDTH_31_3,
    "41_7kHz": Bandwidth.RX_BANDWIDTH_41_7,
    "50_0kHz": Bandwidth.RX_BANDWIDTH_50_0,
    "62_5kHz": Bandwidth.RX_BANDWIDTH_62_5,
    "83_3kHz": Bandwidth.RX_BANDWIDTH_83_3,
    "100_0kHz": Bandwidth.RX_BANDWIDTH_100_0,
    "125_0kHz": Bandwidth.RX_BANDWIDTH_125_0,
    "166_7kHz": Bandwidth.RX_BANDWIDTH_166_7,
    "200_0kHz": Bandwidth.RX_BANDWIDTH_200_0,
    "250_0kHz": Bandwidth.RX_BANDWIDTH_250_0
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SX127x),
            cv.Required(CONF_RST_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_NSS_PIN): pins.internal_gpio_output_pin_schema,
            cv.Optional(CONF_OOK_FLOOR, default=-122): cv.float_range(min=-128, max=0),
            cv.Required(CONF_FREQUENCY): cv.int_range(min=137000000, max=1020000000),
            cv.Required(CONF_MODULATION): cv.enum(MODULATION),
            cv.Required(CONF_BANDWIDTH): cv.enum(BANDWIDTH),
        }
    ).extend(spi.spi_device_schema(False, 8e6, 'mode0'))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
    rst_pin = await cg.gpio_pin_expression(config[CONF_RST_PIN])
    cg.add(var.set_rst_pin(rst_pin))
    nss_pin = await cg.gpio_pin_expression(config[CONF_NSS_PIN])
    cg.add(var.set_nss_pin(nss_pin))
    cg.add(var.set_ook_floor(config.get(CONF_OOK_FLOOR)))
    cg.add(var.set_frequency(config[CONF_FREQUENCY]))
    cg.add(var.set_rx_modulation(config[CONF_MODULATION]))
    cg.add(var.set_rx_bandwidth(config[CONF_BANDWIDTH]))
