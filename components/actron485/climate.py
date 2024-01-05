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
CONF_ZONE_NAMES = "zones"
CONF_ESP_FAN_AVAILABLE = "esp_fan_available"
CONF_ULTIMA_AVAILABLE = "ultima_available"
CONF_LOGGING_MODE = "logging_mode"

CONF_ZONE_NUMBER = "number"
CONF_ZONE_NAME = "name"

ALLOWED_LOGGING_MODES = {
    "NONE": 0,
    "STATUS": 1,
    "CHANGE": 2,
    "ALL": 3,
}

zone_entry_parameter = {
    cv.Required(CONF_ZONE_NUMBER): cv.int_,
    cv.Required(CONF_ZONE_NAME): cv.string,
}

CODEOWNERS = ["@awulf"]
DEPENDENCIES = ["climate", "uart"]

actron485_ns = cg.esphome_ns.namespace("actron485")
Actron485Climate = actron485_ns.class_("Actron485Climate", climate.Climate, cg.Component)

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(Actron485Climate),
            cv.Required(CONF_WRITE_ENABLE_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_ZONE_NAMES): cv.All(
                cv.ensure_list(zone_entry_parameter), cv.Length(min=1, max=8)
            ),
            cv.Optional(CONF_LOGGING_MODE, default="STATUS"): cv.enum(ALLOWED_LOGGING_MODES, upper=True),  
            cv.Optional(CONF_ESP_FAN_AVAILABLE, default=False): cv.boolean,
            cv.Optional(CONF_ULTIMA_AVAILABLE, default=False): cv.boolean,
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

    has_esp = config[CONF_ESP_FAN_AVAILABLE]
    cg.add(var.set_has_esp(has_esp))

    has_ultima = config[CONF_ULTIMA_AVAILABLE]
    cg.add(var.set_has_ultima(has_ultima))

    logging_mode = ALLOWED_LOGGING_MODES[config[CONF_LOGGING_MODE]]
    cg.add(var.set_logging_mode(logging_mode))
