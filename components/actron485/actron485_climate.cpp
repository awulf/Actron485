#include "actron485_climate.h"
#include "esphome/core/log.h"
#include "utilities.h"
#include "zone_fan.h"
#include "zone_climate.h"

namespace esphome {
namespace actron485 {

Actron485Climate::Actron485Climate() = default;

// Global Actron485 controller
static Actron485::Controller actron_controller = Actron485::Controller();
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

void Actron485ZoneFan::setup() {
}

void Actron485Climate::setup() {
    // Setup controller if we have the pins and the controller hasn't already been setup
    if (we_pin_ != NULL) {
        int we_pin = we_pin_->get_pin();
        we_pin_->pin_mode(gpio::FLAG_OUTPUT);

        actron_controller.configure(stream_, 25);
        logStream_ = LogStream();
        if (logging_mode_ > 0) {
            actron_controller.configureLogging(&logStream_);
            switch (logging_mode_) {
                case 1:
                    actron_controller.printOutMode = Actron485::PrintOutMode::StatusOnly;
                    break;
                case 2:
                    actron_controller.printOutMode = Actron485::PrintOutMode::ChangedMessages;
                    break;
                case 3:
                    actron_controller.printOutMode = Actron485::PrintOutMode::AllMessages;
            }
        }
    }
}

void Actron485Climate::loop() {
    actron_controller.loop();
    
    if (millis()-counter > 1000) {
        counter = millis();
        update_status();
    }
}

void Actron485Climate::power_on() { 
    actron_controller.setSystemOn(true);
}

void Actron485Climate::power_off() {
    actron_controller.setSystemOn(false);
}

void Actron485Climate::power_toggle() { 
    actron_controller.setSystemOn(!actron_controller.getSystemOn());
}

void Actron485Climate::add_zone(int number, Actron485ZoneFan *fan) {
    if (number < 1 && number > 8) {
        ESP_LOGE(TAG, "Zone out of bounds %d, 1-8 accepted", number);
        return;
    }
    fan->set_controller(&actron_controller);
    fan->set_zone_number(number);
    zones_[number-1] = fan;
}

void Actron485Climate::update_status() {
    if (last_command_sent_time_ < actron_controller.statusLastReceivedTime) {
        // Don't check until we received a new status message after sending a command
        // to debounce status changes 
        return;
    }

    bool has_changed = false;

    // Target/Setpoint Temperature
    update_property(this->target_temperature, (float)actron_controller.getMasterSetpoint(), has_changed);
    // Current Temperature
    update_property(this->current_temperature, (float)actron_controller.getMasterCurrentTemperature(), has_changed);

    // Continuous Fan Mode
    bool continuous_mode = actron_controller.getContinuousFanMode();
    has_changed = has_changed || (this->set_custom_preset_(Converter::to_preset(continuous_mode)));

    // Fan Speed Mode
    Actron485::FanMode fan_mode = actron_controller.getFanSpeed();
    has_changed = has_changed || (this->set_fan_mode_(Converter::to_fan_mode(fan_mode)));

    // Operating Mode
    auto mode = actron_controller.getSystemOn() ? Converter::to_climate_mode(actron_controller.getOperatingMode()) : ClimateMode::CLIMATE_MODE_OFF;
    update_property(this->mode, mode, has_changed);

    // Action Mode
    auto action = actron_controller.getSystemOn() ? Converter::to_climate_action(actron_controller.getCompressorMode(), actron_controller.getOperatingMode()) : ClimateAction::CLIMATE_ACTION_OFF;
    update_property(this->action, action, has_changed);

    if (has_changed) {
        ESP_LOGD(TAG, "Has Changed, Publishing State");
        this->publish_state();
    }

    // Zone updates
    for (int z=0; z<6; z++) {
        if (zones_[z]) {
            zones_[z]->update();
        }
    }
}

void Actron485Climate::control(const climate::ClimateCall &call) {
    if (call.get_mode().has_value()) {
        Actron485::OperatingMode operating_mode = Converter::to_actron_operating_mode(call.get_mode().value());
        actron_controller.setOperatingMode(operating_mode);
    }
    if (call.get_target_temperature().has_value()) {
        actron_controller.setMasterSetpoint(call.get_target_temperature().value());
    }
    if (call.get_custom_preset().has_value()) {
        bool continuous_mode = Converter::to_continuous_mode(call.get_custom_preset().value());
        if (actron_controller.getContinuousFanMode() != continuous_mode) {
            actron_controller.setContinuousFanMode(continuous_mode);
        }
    }
    if (call.get_fan_mode().has_value()) {
        Actron485::FanMode fan_mode = Converter::to_actron_fan_mode(call.get_fan_mode().value());
        actron_controller.setFanSpeed(fan_mode);
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

    return traits;
}

void Actron485Climate::dump_config() {
  ESP_LOGCONFIG(TAG, "Actron485 Status:");
  ESP_LOGCONFIG(TAG, "  Receiving Data: %s", actron_controller.receivingData() ? "YES" : "NO");
  this->dump_traits_(TAG);
}

}
}