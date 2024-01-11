
#include "utilities.h"
#include "zone_climate.h"

namespace esphome {
namespace actron485 {

Actron485ZoneClimate::Actron485ZoneClimate() = default;

void Actron485ZoneClimate::update() {
    bool has_changed = false;

    // Target/Setpoint Temperature
    update_property(this->target_temperature, (float)actron_controller_->getZoneSetpointTemperature(number_), has_changed);
    // Current Temperature
    update_property(this->current_temperature, (float)actron_controller_->getZoneCurrentTemperature(number_), has_changed);

    // Operating Mode
    auto zone_on = actron_controller_->getZoneOn(number_) ? ClimateMode::CLIMATE_MODE_AUTO : ClimateMode::CLIMATE_MODE_OFF;
    update_property(this->mode, zone_on, has_changed);

    // Action Mode
    // auto action = actron_controller.getSystemOn() ? Converter::to_climate_action(actron_controller.getCompressorMode(), actron_controller.getOperatingMode()) : ClimateAction::CLIMATE_ACTION_OFF;
    // update_property(this->action, action, has_changed);

    if (has_changed) {
        ESP_LOGD(TAG, "Zone Changed, Publishing State");
        this->publish_state();
    }
}

void Actron485ZoneClimate::control(const climate::ClimateCall &call) {
    if (call.get_mode().has_value()) {
        bool isOn = call.get_mode().value() != ClimateMode::CLIMATE_MODE_OFF;
        actron_controller_->setZoneOn(number_, isOn);
    }
    if (call.get_target_temperature().has_value()) {
        actron_controller_->setZoneSetpointTemperature(number_, call.get_target_temperature().value(), ultima_adjusts_master_setpoint_);
    }
}

climate::ClimateTraits Actron485ZoneClimate::traits() {
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_visual_min_temperature(16);
    traits.set_visual_max_temperature(30);
    traits.set_visual_temperature_step(0.5);
    traits.set_visual_current_temperature_step(0.1);
    traits.set_supported_modes({
        ClimateMode::CLIMATE_MODE_OFF,
        ClimateMode::CLIMATE_MODE_AUTO,
    });
    traits.set_supports_action(true);

    return traits;
}

void Actron485ZoneClimate::dump_config() { 
    
}

}
}