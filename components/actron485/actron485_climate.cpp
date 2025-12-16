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

void Actron485Climate::uart_task(void *param) {
    Actron485Climate *self = static_cast<Actron485Climate*>(param);
    
    while (true) {
        // Check for timeout (>8ms since last byte means new packet)
        bool newPacket = (self->serial_received_last_byte_time_ == 0 || (millis() - self->serial_received_last_byte_time_) > 8);

        if (self->stream_.available()) {             
            // Been long enough for a new packet?
            if (newPacket) {
                // Start a new packet buffer
                if (!self->serial_receive_buffer_.empty()) {
                    self->serial_completed_packets_.push_back(self->serial_receive_buffer_);
                    self->serial_receive_buffer_.clear();
                }
            }
            
            // Read to current packet buffer
            self->serial_receive_buffer_.push_back(self->stream_.read());
            self->serial_received_last_byte_time_ = millis();

        } else if (newPacket) {
            // Finished reading the packet? And no new data, let ESPHome do it's thing
            vTaskDelay(1);
        }
    }
}

void Actron485Climate::setup() {
    uint8_t we_pin = 0;
    if (we_pin_ != NULL) {
        we_pin = we_pin_->get_pin();
        we_pin_->pin_mode(gpio::FLAG_OUTPUT);
    }
    actron_controller.configure(stream_, we_pin);
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
    
    xTaskCreate(uart_task, "uart_task", 2048, this, 10, nullptr);
}

void Actron485Climate::loop() {
    // Process any complete packets
    // In the future if ESPHome ever goes being more than 1 core we'll need to add a mutex here
    // but currently it slows things down unnecessarily
    for (const std::vector<uint8_t> &packet: serial_completed_packets_) {
        // Needs to be more than 2 bytes otherwise, it's nothing useful
        if (packet.size() > 1) {
            uint8_t *data = const_cast<uint8_t*>(packet.data());
            actron_controller.processMessage(data, packet.size());
        }
    }
    serial_completed_packets_.clear();
    unsigned long now = millis();
    unsigned long last_received = now - serial_received_last_byte_time_;
    // Has been more than 0.1s since last received, but less than 0.8s, so we don't have a potential clash
    // but also don't try multiple attempts times in this 1s period
    if (last_received  > 100 && last_received < 800 && (now - serial_send_attempt_last_time_) > 800) {
        actron_controller.attemptToSendQueuedCommand();
        serial_send_attempt_last_time_ = now;
    }

    if (now-counter > 1000) {
        counter = now;
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

void Actron485Climate::add_ultima_zone(int number, Actron485ZoneClimate *climate) {
    if (number < 1 && number > 8) {
        ESP_LOGE(TAG, "Zone out of bounds %d, 1-8 accepted", number);
        return;
    }
    climate->set_controller(&actron_controller);
    climate->set_zone_number(number);
    climate->set_ultima_adjusts_master_setpoint(ultima_adjusts_master_setpoint_);
    zone_climates_[number-1] = climate;
}

void Actron485Climate::update_status() {
    if (actron_controller.dataLastSentTime >= actron_controller.statusLastReceivedTime || actron_controller.totalPendingMainCommands() > 0) {
        // Don't check until we received a new status message after sending a command
        // to debounce status changes 
        return;
    }

    if ((max(command_last_sent_, actron_controller.dataLastSentTime) + DEBOUNCE_MILLIS) >= millis()) {
        // debounce our commands
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
    for (int z=0; z<8; z++) {
        if (zones_[z]) {
            zones_[z]->update_status();
        }
        if (zone_climates_[z]) {
            zone_climates_[z]->update_status();
        }
    }
}

void Actron485Climate::control(const climate::ClimateCall &call) {
    command_last_sent_ = millis();

    if (call.get_mode().has_value()) {
        Actron485::OperatingMode operating_mode = Converter::to_actron_operating_mode(call.get_mode().value());
        actron_controller.setOperatingMode(operating_mode);
        this->mode = call.get_mode().value();
    }
    if (call.get_target_temperature().has_value()) {
        actron_controller.setMasterSetpoint(call.get_target_temperature().value());
        this->target_temperature = call.get_target_temperature().value();
    }
    if (call.has_custom_preset()) {
        bool continuous_mode = Converter::to_continuous_mode(call.get_custom_preset());
        if (actron_controller.getContinuousFanMode() != continuous_mode) {
            actron_controller.setContinuousFanMode(continuous_mode);
        }
        this->set_custom_preset_(call.get_custom_preset());
    }
    if (call.get_fan_mode().has_value()) {
        Actron485::FanMode fan_mode = Converter::to_actron_fan_mode(call.get_fan_mode().value());
        actron_controller.setFanSpeed(fan_mode);
        this->fan_mode = call.get_fan_mode().value();
    }

    this->publish_state();
}

climate::ClimateTraits Actron485Climate::traits() {
    auto traits = climate::ClimateTraits();
    traits.add_feature_flags(
        climate::ClimateFeature::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE |
        climate::ClimateFeature::CLIMATE_SUPPORTS_ACTION
    );
    traits.set_visual_min_temperature(16);
    traits.set_visual_max_temperature(30);
    traits.set_visual_temperature_step(0.5);
    traits.set_visual_current_temperature_step(0.1);
    traits.set_supported_modes({
        ClimateMode::CLIMATE_MODE_OFF,
        ClimateMode::CLIMATE_MODE_COOL,
        ClimateMode::CLIMATE_MODE_HEAT,
        ClimateMode::CLIMATE_MODE_HEAT_COOL,
        ClimateMode::CLIMATE_MODE_FAN_ONLY,
    });
    traits.set_supported_fan_modes({
        ClimateFanMode::CLIMATE_FAN_LOW,
        ClimateFanMode::CLIMATE_FAN_MEDIUM,
        ClimateFanMode::CLIMATE_FAN_HIGH,
    });
    traits.set_supported_custom_presets({
        Converter::FAN_STANDARD,
        Converter::FAN_CONTINUOUS
    });
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