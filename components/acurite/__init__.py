import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import time
from esphome.components import remote_receiver
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
)

DEPENDENCIES = ["time", "remote_receiver"]

CODEOWNERS = ["@swoboda1337"]

CONF_RECEIVER_ID = 'receiver_id'

acurite_ns = cg.esphome_ns.namespace("acurite")
AcuRite = acurite_ns.class_("AcuRite", cg.Component)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AcuRite),
            cv.GenerateID(CONF_RECEIVER_ID): cv.use_id(remote_receiver.RemoteReceiverComponent),
        }
    )
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    receiver = await cg.get_variable(config[CONF_RECEIVER_ID])
    cg.add(var.set_srcrecv(receiver))
