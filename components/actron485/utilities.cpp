#include "utilities.h"
#include "Actron485.h"

namespace esphome {
namespace actron485 {

ClimateMode Converter::to_climate_mode(Actron485::OperatingMode mode) {
    switch (mode) {
        case Actron485::OperatingMode::Off:
        case Actron485::OperatingMode::OffAuto:
        case Actron485::OperatingMode::OffCool:
        case Actron485::OperatingMode::OffHeat:
            return ClimateMode::CLIMATE_MODE_OFF;
        case Actron485::OperatingMode::FanOnly:
            return ClimateMode::CLIMATE_MODE_FAN_ONLY;
        case Actron485::OperatingMode::Auto:
            return ClimateMode::CLIMATE_MODE_AUTO;
        case Actron485::OperatingMode::Cool:
            return ClimateMode::CLIMATE_MODE_COOL;
        case Actron485::OperatingMode::Heat:
            return ClimateMode::CLIMATE_MODE_HEAT;
    }
    return ClimateMode::CLIMATE_MODE_OFF;
}

ClimateAction Converter::to_climate_action(Actron485::CompressorMode compressorMode, Actron485::OperatingMode operatingMode) {
    switch (operatingMode) {
        case Actron485::OperatingMode::Off:
        case Actron485::OperatingMode::OffAuto:
        case Actron485::OperatingMode::OffCool:
        case Actron485::OperatingMode::OffHeat:
            return ClimateAction::CLIMATE_ACTION_OFF;
        case Actron485::OperatingMode::FanOnly:
            return ClimateAction::CLIMATE_ACTION_FAN;
    }

    switch (compressorMode) {
        case Actron485::CompressorMode::Idle:
            return ClimateAction::CLIMATE_ACTION_IDLE;
        case Actron485::CompressorMode::Cooling:
            return ClimateAction::CLIMATE_ACTION_COOLING;
        case Actron485::CompressorMode::Heating:
            return ClimateAction::CLIMATE_ACTION_HEATING;
    }
    return ClimateAction::CLIMATE_ACTION_OFF;
}

Actron485::OperatingMode Converter::to_actron_operating_mode(ClimateMode mode) {
    switch (mode) {
        case ClimateMode::CLIMATE_MODE_FAN_ONLY:
            return Actron485::OperatingMode::FanOnly;
        case ClimateMode::CLIMATE_MODE_AUTO:
            return Actron485::OperatingMode::Auto;
        case ClimateMode::CLIMATE_MODE_COOL:
            return Actron485::OperatingMode::Cool;
        case ClimateMode::CLIMATE_MODE_HEAT:
            return Actron485::OperatingMode::Heat;
    }
    return Actron485::OperatingMode::Off;
}

ClimateFanMode Converter::to_fan_mode(Actron485::FanMode mode) {
    switch (mode) {
        case Actron485::FanMode::Off:
            return ClimateFanMode::CLIMATE_FAN_OFF;
        case Actron485::FanMode::Low:
        case Actron485::FanMode::LowContinuous:
            return ClimateFanMode::CLIMATE_FAN_LOW;
        case Actron485::FanMode::Medium:
        case Actron485::FanMode::MediumContinuous:
            return ClimateFanMode::CLIMATE_FAN_MEDIUM;
        case Actron485::FanMode::High:
        case Actron485::FanMode::HighContinuous:
            return ClimateFanMode::CLIMATE_FAN_HIGH;
        case Actron485::FanMode::Esp:
        case Actron485::FanMode::EspContinuous:
            return ClimateFanMode::CLIMATE_FAN_AUTO;
    }
    return ClimateFanMode::CLIMATE_FAN_OFF;
}

Actron485::FanMode Converter::to_actron_fan_mode(ClimateFanMode mode) {
    switch (mode) {
        case ClimateFanMode::CLIMATE_FAN_AUTO:
            return Actron485::FanMode::Esp;
        case ClimateFanMode::CLIMATE_FAN_LOW:
            return Actron485::FanMode::Low;
        case ClimateFanMode::CLIMATE_FAN_MEDIUM:
            return Actron485::FanMode::Medium;
        case ClimateFanMode::CLIMATE_FAN_HIGH:
            return Actron485::FanMode::High;
    }
    return Actron485::FanMode::Off;
}

const char* Converter::to_preset(bool continuous_mode) {
    if (continuous_mode) {
        return Converter::FAN_CONTINUOUS;
    } else {
        return Converter::FAN_STANDARD;
    }
}

bool Converter::to_continuous_mode(const char *preset) {
    return preset == Converter::FAN_CONTINUOUS;
}

}
}