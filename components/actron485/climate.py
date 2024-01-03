import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import climate, sensor, uart

from esphome.const import (
    CONF_ID,
    CONF_READ_PIN,
    CONF_WRITE_PIN,
)

CONF_WRITE_ENABLE_PIN = "write_enable_pin"

CODEOWNERS = ["@awulf"]
DEPENDENCIES = ["climate", "uart"]

actron485_ns = cg.esphome_ns.namespace("actron485")
Actron485Climate = actron485_ns.class_("Actron485Climate", climate.Climate, cg.Component)

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(Actron485Climate),
            cv.Required(CONF_WRITE_ENABLE_PIN): pins.gpio_output_pin_schema,
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    await climate.register_climate(var, config)
    
    we_pin = await cg.gpio_pin_expression(config[CONF_WRITE_ENABLE_PIN])
    cg.add(var.set_we_pin(we_pin))
