#include "actron485_climate.h"
#include "esphome/core/log.h"

namespace esphome {
namespace actron485 {

static const char *const TAG = "actron485.climate";

Actron485Climate::Actron485Climate() = default;

// Global Actron485 controller
static Actron485::Controller actronController = Actron485::Controller();
static long counter = 0;

void Actron485Climate::setup() {
//     this->controller.getMasterSetpoint();
    ESP_LOGD(TAG, "SETUP");

    // Setup controller if we have the pins and the controller hasn't already been setup
    if (we_pin_ != NULL) {
        int we_pin = we_pin_->get_pin();

        we_pin_->pin_mode(gpio::FLAG_OUTPUT);

        ESP_LOGD(TAG, "WE %d", we_pin);

        actronController.configure(stream_, 25);
        actronController.printOutMode = Actron485::PrintOutMode::StatusOnly;
    }
}

void Actron485Climate::loop() {
    // if (actronController != NULL) {
        actronController.loop();
    // }
    
    if (millis()-counter > 1000) {
        ESP_LOGD(TAG, "Receiving Data: %s", actronController.receivingData() ? "YES" : "NO");
        counter = millis();
    }
}

// void Actron485Climate::switch_to_action_(climate::ClimateAction action) {

// }

// Actron485Climate::Actron485Climate() : idle_trigger_(new Trigger<>()), cool_trigger_(new Trigger<>()), heat_trigger_(new Trigger<>()) {}

// Trigger<> *Actron485Climate::get_idle_trigger() const { 
//     return this->idle_trigger_; 
// }

void Actron485Climate::control(const climate::ClimateCall &call) {
    // if (call.get_mode().has_value())
    //     this->mode = *call.get_mode();
    // if (call.get_target_temperature_low().has_value())
    //     this->target_temperature_low = *call.get_target_temperature_low();
    // if (call.get_target_temperature_high().has_value())
    //     this->target_temperature_high = *call.get_target_temperature_high();
    // if (call.get_preset().has_value())
    //     this->change_away_(*call.get_preset() == climate::CLIMATE_PRESET_AWAY);

    // this->compute_state_();
    // this->publish_state();
}

climate::ClimateTraits Actron485Climate::traits() {
    auto traits = climate::ClimateTraits();
//   traits.set_supports_current_temperature(true);
//   if (this->humidity_sensor_ != nullptr)
//     traits.set_supports_current_humidity(true);
//   traits.set_supported_modes({
//       climate::CLIMATE_MODE_OFF,
//   });
//   if (supports_cool_)
//     traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
//   if (supports_heat_)
//     traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);
//   if (supports_cool_ && supports_heat_)
//     traits.add_supported_mode(climate::CLIMATE_MODE_HEAT_COOL);
//   traits.set_supports_two_point_target_temperature(true);
//   if (supports_away_) {
//     traits.set_supported_presets({
//         climate::CLIMATE_PRESET_HOME,
//         climate::CLIMATE_PRESET_AWAY,
//     });
//   }
//   traits.set_supports_action(true);
    return traits;
}

}
}