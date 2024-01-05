#include "utilities.h"
#include "Actron485.h"

namespace esphome {
namespace actron485 {

const std::string Converter::FAN_NORMAL = "Normal Fan";
const std::string Converter::FAN_CONTINUOUS = "Continuous Fan";

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

const std::string Converter::to_preset(bool continuous_mode) {
    if (continuous_mode) {
        return Converter::FAN_CONTINUOUS;
    } else {
        return Converter::FAN_NORMAL;
    }
}

}
}