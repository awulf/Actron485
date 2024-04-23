# Acknowledgement: Some parts of code in this component has been taken from the Midea Climate component and some others

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome import automation
from esphome.components import climate, sensor, uart, fan

from esphome.const import (
    CONF_ID,
    CONF_READ_PIN,
    CONF_WRITE_PIN,
    CONF_DISABLED_BY_DEFAULT,
    CONF_RESTORE_MODE
)

CONF_WRITE_ENABLE_PIN = "write_enable_pin"
CONF_ZONES = "zones"
CONF_ESP_FAN_AVAILABLE = "esp_fan_available"
CONF_ULTIMA = "ultima"
CONF_ULTIMA_AVAILABLE = "available"
CONF_ULTIMA_ZONES_ADJUSTS_MASTER = "adjust_master_target"
CONF_LOGGING_MODE = "logging_mode"

CONF_ZONE_NUMBER = "number"
CONF_ZONE_NAME = "name"

CONF_ZONE_FAN_ID = "zone_fan_id"
CONF_ZONE_CLIMATE_ID = "zone_climate_id"

ALLOWED_LOGGING_MODES = {
    "NONE": 0,
    "STATUS": 1,
    "CHANGE": 2,
    "ALL": 3,
}

ultima_config_parameter = {
    cv.Optional(CONF_ULTIMA_AVAILABLE, default=True): cv.boolean,
    cv.Optional(CONF_ULTIMA_ZONES_ADJUSTS_MASTER, default=False): cv.boolean,
}

CODEOWNERS = ["@awulf"]
DEPENDENCIES = ["climate", "uart", "fan"]

actron485_ns = cg.esphome_ns.namespace("actron485")
Actron485Climate = actron485_ns.class_("Actron485Climate", climate.Climate, cg.Component)
Actron485ZoneFan = actron485_ns.class_("Actron485ZoneFan", fan.Fan, cg.Component)
Actron485ZoneClimate = actron485_ns.class_("Actron485ZoneClimate", climate.Climate, cg.Component)

ZONE_ENTRY_PARAMETER = cv.All(
    cv.COMPONENT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_ZONE_FAN_ID): cv.declare_id(Actron485ZoneFan),
            cv.GenerateID(CONF_ZONE_CLIMATE_ID): cv.declare_id(Actron485ZoneClimate),
            cv.Required(CONF_ZONE_NUMBER): cv.int_,
            cv.Required(CONF_ZONE_NAME): cv.string,
        }
    )
    # .extend(fan.FAN_SCHEMA)
    # .extend(climate.CLIMATE_SCHEMA)
)

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(Actron485Climate),
            cv.Optional(CONF_WRITE_ENABLE_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_ZONES): cv.All(
                cv.ensure_list(ZONE_ENTRY_PARAMETER), cv.Length(min=1, max=8)
            ),
            cv.Optional(CONF_LOGGING_MODE, default="STATUS"): cv.enum(ALLOWED_LOGGING_MODES, upper=True),  
            cv.Optional(CONF_ESP_FAN_AVAILABLE, default=False): cv.boolean,
            cv.Optional(CONF_ULTIMA): cv.Schema(ultima_config_parameter),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)

# Actions
PowerOnAction = actron485_ns.class_("PowerOnAction", automation.Action)
PowerOffAction = actron485_ns.class_("PowerOffAction", automation.Action)
PowerToggleAction = actron485_ns.class_("PowerToggleAction", automation.Action)

ACTRON485_ACTION_BASE_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(Actron485Climate),
    }
)

# Power On action
@automation.register_action(
    "actron485.power_on", PowerOnAction, ACTRON485_ACTION_BASE_SCHEMA,
)
@automation.register_action(
    "actron485.power_off", PowerOffAction, ACTRON485_ACTION_BASE_SCHEMA,
)
@automation.register_action(
    "actron485.power_toggle", PowerToggleAction, ACTRON485_ACTION_BASE_SCHEMA,
)
async def power_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    return var

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    await climate.register_climate(var, config)
    
    if CONF_WRITE_ENABLE_PIN in config:
        we_pin = await cg.gpio_pin_expression(config[CONF_WRITE_ENABLE_PIN])
        cg.add(var.set_we_pin(we_pin))

    has_esp = config[CONF_ESP_FAN_AVAILABLE]
    cg.add(var.set_has_esp(has_esp))

    has_ultima = False
    if CONF_ULTIMA in config:
        ultima_config = config[CONF_ULTIMA]
        has_ultima = ultima_config[CONF_ULTIMA_AVAILABLE]
        adjusts_master = ultima_config[CONF_ULTIMA_ZONES_ADJUSTS_MASTER]
        cg.add(var.set_ultima_settings(has_ultima, adjusts_master))

    logging_mode = ALLOWED_LOGGING_MODES[config[CONF_LOGGING_MODE]]
    cg.add(var.set_logging_mode(logging_mode))

    if CONF_ZONES in config:
        zones = config[CONF_ZONES]
        for zone in zones:
            name = zone[CONF_ZONE_NAME]
            number = zone[CONF_ZONE_NUMBER]
            
            # This below is messy, surely there's a better way
            # to make two types of components from a single list

            zone_f = zone.copy()
            zone_f.pop(CONF_ZONE_NUMBER)
            zone_f.pop(CONF_ZONE_CLIMATE_ID)
            fan_id = zone_f.pop(CONF_ZONE_FAN_ID)
            zone_f[CONF_ID] = fan_id

            fanConfig = fan.FAN_SCHEMA(zone_f)
            fanConfig[CONF_ID] = fan_id
            
            zoneFan = cg.new_Pvariable(fanConfig[CONF_ID])
            await fan.register_fan(zoneFan, fanConfig)
            await cg.register_component(zoneFan, fanConfig)
            cg.add(var.add_zone(number, zoneFan))

            if has_ultima:
                zone_c = zone.copy()
                zone_c.pop(CONF_ZONE_NUMBER)
                climate_id = zone_c.pop(CONF_ZONE_CLIMATE_ID)
                zone_c.pop(CONF_ZONE_FAN_ID)
                zone_c[CONF_ID] = climate_id

                climateConfig = climate.CLIMATE_SCHEMA(zone_c)
                climateConfig[CONF_ID] = climate_id

                zoneClimate = cg.new_Pvariable(climateConfig[CONF_ID])
                await climate.register_climate(zoneClimate, climateConfig)
                await cg.register_component(zoneClimate, climateConfig)
                cg.add(var.add_ultima_zone(number, zoneClimate))

