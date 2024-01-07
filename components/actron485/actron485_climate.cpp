#include "actron485_climate.h"
#include "esphome/core/log.h"
#include "utilities.h"

namespace esphome {
namespace actron485 {

static const char *const TAG = "actron485.climate";

Actron485Climate::Actron485Climate() = default;

// Global Actron485 controller
static Actron485::Controller actronController = Actron485::Controller();
static long counter = 0;

size_t LogStream::write(uint8_t data) {
    if (_bufferIndex >= bufferSize) {
        // Need to print contents
        _bufferIndex = 0;
        return 0;
    }

    if (data == '\r' || data == '\0') {
        // Ignore
    } else if (data == '\n') {
        _buffer[_bufferIndex] = '\0';
        ESP_LOGD(TAG, "%s", _buffer);
        _bufferIndex = 0;
    } else {
        _buffer[_bufferIndex] = data;
        _bufferIndex++;
    }

    return 1;
}
    
size_t LogStream::write(const uint8_t *data, size_t size) {
    for (int i=0; i<size; i++) {
        write(data[i]);
    }
    return size;
}

void LogStream::flush() {
    ESP_LOGD(TAG, "FLUSH");
}

void Actron485Climate::setup() {
    // Setup controller if we have the pins and the controller hasn't already been setup
    if (we_pin_ != NULL) {
        int we_pin = we_pin_->get_pin();
        we_pin_->pin_mode(gpio::FLAG_OUTPUT);

        actronController.configure(stream_, 25);
        logStream_ = LogStream();
        if (logging_mode_ > 0) {
            actronController.configureLogging(&logStream_);
            switch (logging_mode_) {
                case 1:
                    actronController.printOutMode = Actron485::PrintOutMode::StatusOnly;
                    break;
                case 2:
                    actronController.printOutMode = Actron485::PrintOutMode::ChangedMessages;
                    break;
                case 3:
                    actronController.printOutMode = Actron485::PrintOutMode::AllMessages;
            }
        }
    }
}

void Actron485Climate::loop() {
    actronController.loop();
    
    if (millis()-counter > 1000) {
        counter = millis();
        update_status();
    }
}

template<typename T> void update_property(T &property, const T &value, bool &flag) {
    if (property != value) {
        property = value;
        flag = true;
    }
}

void Actron485Climate::update_status() {
    bool has_changed = false;
    // Target/Setpoint Tempeature
    update_property(this->target_temperature, (float)actronController.getMasterSetpoint(), has_changed);
    // Current Temperature
    update_property(this->current_temperature, (float)actronController.getMasterCurrentTemperature(), has_changed);

    // Continuous Fan Mode
    bool continuous_mode = actronController.getContinuousFanMode();
    has_changed = has_changed || (this->set_custom_preset_(Converter::to_preset(continuous_mode)));

    // Fan Speed Mode
    Actron485::FanMode fan_mode = actronController.getFanSpeed();
    has_changed = has_changed || (this->set_fan_mode_(Converter::to_fan_mode(fan_mode)));

    // Operating Mode
    auto mode = actronController.getSystemOn() ? Converter::to_climate_mode(actronController.getOperatingMode()) : ClimateMode::CLIMATE_MODE_OFF;
    update_property(this->mode, mode, has_changed);

    // Action Mode
    auto action = actronController.getSystemOn() ? Converter::to_climate_action(actronController.getCompressorMode(), actronController.getOperatingMode()) : ClimateAction::CLIMATE_ACTION_OFF;
    update_property(this->action, action, has_changed);

    if (has_changed) {
        ESP_LOGD(TAG, "Has Changed, Publishing State");
        this->publish_state();
    }
}

// Actron485Climate::Actron485Climate() : idle_trigger_(new Trigger<>()), cool_trigger_(new Trigger<>()), heat_trigger_(new Trigger<>()) {}

// Trigger<> *Actron485Climate::get_idle_trigger() const { 
//     return this->idle_trigger_; 
// }

void Actron485Climate::control(const climate::ClimateCall &call) {
    if (call.get_mode().has_value()) {
        Actron485::OperatingMode operating_mode = Converter::to_actron_operating_mode(call.get_mode().value());
        actronController.setOperatingMode(operating_mode);
    }
    if (call.get_target_temperature().has_value()) {
        actronController.setMasterSetpoint(call.get_target_temperature().value());
    }
    if (call.get_custom_preset().has_value()) {
        bool continuous_mode = Converter::to_continuous_mode(call.get_custom_preset().value());
        if (actronController.getContinuousFanMode() != continuous_mode) {
            actronController.setContinuousFanMode(continuous_mode);
        }
    }
    if (call.get_fan_mode().has_value()) {
        Actron485::FanMode fan_mode = Converter::to_actron_fan_mode(call.get_fan_mode().value());
        actronController.setFanSpeed(fan_mode);
    }
}

climate::ClimateTraits Actron485Climate::traits() {
    auto traits = climate::ClimateTraits();
    traits.set_supports_current_temperature(true);
    traits.set_visual_min_temperature(16);
    traits.set_visual_max_temperature(30);
    traits.set_visual_temperature_step(0.5);
    traits.set_visual_current_temperature_step(0.1);
    traits.set_supported_modes({
        ClimateMode::CLIMATE_MODE_OFF,
        ClimateMode::CLIMATE_MODE_COOL,
        ClimateMode::CLIMATE_MODE_HEAT,
        ClimateMode::CLIMATE_MODE_AUTO,
        ClimateMode::CLIMATE_MODE_FAN_ONLY,
    });
    traits.set_supported_fan_modes({
        ClimateFanMode::CLIMATE_FAN_LOW,
        ClimateFanMode::CLIMATE_FAN_MEDIUM,
        ClimateFanMode::CLIMATE_FAN_HIGH,
    });
    traits.set_supports_action(true);
    traits.add_supported_custom_preset(Converter::FAN_STANDARD);
    traits.add_supported_custom_preset(Converter::FAN_CONTINUOUS);
    if (has_esp_auto_) {
        traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_AUTO);
    }



//   traits.set_supports_action(true);
    return traits;
}

void Actron485Climate::dump_config() {
  ESP_LOGCONFIG(TAG, "Actron485 Status:");
  ESP_LOGCONFIG(TAG, "  Receiving Data: %s", actronController.receivingData() ? "YES" : "NO");
  this->dump_traits_(TAG);
}

}
}