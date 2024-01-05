#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/climate/climate.h"
#include "esphome/core/defines.h"
#include "esphome/components/uart/uart.h"
#include "Actron485.h"

namespace esphome {
namespace actron485 {

using climate::ClimateCall;
using climate::ClimatePreset;
using climate::ClimateTraits;
using climate::ClimateMode;
using climate::ClimateSwingMode;
using climate::ClimateFanMode;

/* Stream from UART component (copied from Midea component) */
class UARTStream : public Stream {
    public:
        void set_uart(uart::UARTComponent *uart) { this->uart_ = uart; }

        /* Stream interface implementation */

        int available() override { return this->uart_->available(); }
        int read() override {
            uint8_t data;
            this->uart_->read_byte(&data);
            return data;
        }
        int peek() override {
            uint8_t data;
            this->uart_->peek_byte(&data);
            return data;
        }
        size_t write(uint8_t data) override {
            this->uart_->write_byte(data);
            return 1;
        }
        size_t write(const uint8_t *data, size_t size) override {
            this->uart_->write_array(data, size);
            return size;
        }
        void flush() override { this->uart_->flush(); }

    protected:
        uart::UARTComponent *uart_;
};

class LogStream : public Stream {
    public:
        int available() override { return false; }
        int read() override {
            return 0;
        }
        int peek() override {
            return 0;
        }
        size_t write(uint8_t data) override;
        size_t write(const uint8_t *data, size_t size) override;
        void flush() override;
    protected:
        static const int bufferSize = 200;
        char _buffer[bufferSize];
        int _bufferIndex = 0;
};

class Actron485Climate : public climate::Climate, public Component {
        
    public:
        Actron485Climate();
        void setup() override;
        void loop() override;

        void set_we_pin(InternalGPIOPin *pin) { we_pin_ = pin; }
        void set_has_esp(bool available) { has_esp_auto_ = available; }
        void set_has_ultima(bool available) { has_ultima_ = available; }
        void set_logging_mode(int logging_mode) { logging_mode_ = logging_mode; }
        void set_uart_parent(uart::UARTComponent *parent) { this->stream_.set_uart(parent); }

    //     void dump_config() override;
        void update_status();

    //     Trigger<> *get_idle_trigger() const;

    protected:
        InternalGPIOPin *we_pin_;
        UARTStream stream_;
        LogStream logStream_;
        int logging_mode_;
        bool has_esp_auto_;
        bool has_ultima_;

        /// Override control to change settings of the climate device.
        void control(const climate::ClimateCall &call) override;
        /// Return the traits of this controller.
        climate::ClimateTraits traits() override;


    //     // Operating Modes
    //     bool supports_auto_{true};
    //     bool supports_cool_{true};
    //     bool supports_heat_{true};
    //     bool supports_fan_only_{true};

    //     // ESP Fan Speed
    //     bool supports_fan_mode_auto_{true};

    //     // Fan Speeds
    //     bool supports_fan_mode_low_{true};
    //     bool supports_fan_mode_medium_{true};
    //     bool supports_fan_mode_high_{true};

    //     // Continuous Fan Mode
    //     bool supports_fan_with_cooling_{true};
    //     bool supports_fan_with_heating_{true};
        
    //     Trigger<> *idle_trigger_;

    //     Trigger<> *cool_trigger_;
    //     Trigger<> *heat_trigger_;

};

}
}