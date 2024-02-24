import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi
from esphome.const import CONF_ID

CODEOWNERS = ["@swoboda1337"]

DEPENDENCIES = ["spi"]

CONF_NSS_PIN = 'nss_pin'
CONF_RST_PIN = 'rst_pin'
CONF_FREQUENCY = "frequency"
CONF_MODULATION = "modulation"
CONF_BANDWIDTH = "bandwidth"

sx127x_ns = cg.esphome_ns.namespace("sx127x")
SX127X = sx127x_ns.class_("SX127X", cg.Component, spi.SPIDevice)
Modulation = sx127x_ns.enum("SX127XMod")
Bandwidth = sx127x_ns.enum("SX127XRxBw")

MODULATION = {
    "fsk": Modulation.MODULATION_FSK,
    "ook": Modulation.MODULATION_OOK,
}

BANDWIDTH = {
    "2_6": Bandwidth.RX_BANDWIDTH_2_6,
    "3_1": Bandwidth.RX_BANDWIDTH_3_1,
    "3_9": Bandwidth.RX_BANDWIDTH_3_9,
    "5_2": Bandwidth.RX_BANDWIDTH_5_2,
    "6_3": Bandwidth.RX_BANDWIDTH_6_3,
    "7_8": Bandwidth.RX_BANDWIDTH_7_8,
    "10_4": Bandwidth.RX_BANDWIDTH_10_4,
    "12_5": Bandwidth.RX_BANDWIDTH_12_5,
    "15_6": Bandwidth.RX_BANDWIDTH_15_6,
    "20_8": Bandwidth.RX_BANDWIDTH_20_8,
    "25_0": Bandwidth.RX_BANDWIDTH_25_0,
    "31_3": Bandwidth.RX_BANDWIDTH_31_3,
    "41_7": Bandwidth.RX_BANDWIDTH_41_7,
    "50_0": Bandwidth.RX_BANDWIDTH_50_0,
    "62_5": Bandwidth.RX_BANDWIDTH_62_5,
    "83_3": Bandwidth.RX_BANDWIDTH_83_3,
    "100_0": Bandwidth.RX_BANDWIDTH_100_0,
    "125_0": Bandwidth.RX_BANDWIDTH_125_0,
    "166_7": Bandwidth.RX_BANDWIDTH_166_7,
    "200_0": Bandwidth.RX_BANDWIDTH_200_0,
    "250_0": Bandwidth.RX_BANDWIDTH_250_0
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SX127X),
            cv.Required(CONF_RST_PIN): pins.internal_gpio_output_pin_schema,
            cv.Required(CONF_NSS_PIN): pins.internal_gpio_output_pin_schema,
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
    cg.add(var.set_frequency(config[CONF_FREQUENCY]))
    cg.add(var.set_rx_modulation(config[CONF_MODULATION]))
    cg.add(var.set_rx_bandwidth(config[CONF_BANDWIDTH]))
