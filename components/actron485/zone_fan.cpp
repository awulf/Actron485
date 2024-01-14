
#include "utilities.h"
#include "zone_fan.h"

namespace esphome {
namespace actron485 {

Actron485ZoneFan::Actron485ZoneFan() = default;

void Actron485ZoneFan::update_status() {
    if ((max(command_last_sent_, actron_controller_->dataLastSentTime) + DEBOUNCE_MILLIS) >= millis()) {
        // debounce our commands
        return;
    }

    bool has_changed = false;

    // Action Mode
    auto zone_on = actron_controller_->getZoneOn(number_);
    update_property(this->state, zone_on, has_changed);

    if (has_changed) {
        ESP_LOGD(TAG, "Zone Fan Changed, Publishing State");
        this->publish_state();
    }
}

void Actron485ZoneFan::control(const fan::FanCall &call) {
    command_last_sent_ = millis();

    if (call.get_state().has_value()) {
        bool on = call.get_state().value();
        actron_controller_->setZoneOn(number_, on);
        this->state = on;
        this->publish_state();
    }
}

void Actron485ZoneFan::dump_config() { 
    
}

}
}