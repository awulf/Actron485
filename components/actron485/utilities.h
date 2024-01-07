#pragma once

#include "Actron485.h"
#include "actron485_climate.h"
#include "esphome/components/climate/climate_traits.h"

namespace esphome {
namespace actron485 {

class Converter {
    public:
        static const std::string FAN_STANDARD;
        static const std::string FAN_CONTINUOUS;

        static ClimateMode to_climate_mode(Actron485::OperatingMode mode);
        static ClimateAction to_climate_action(Actron485::CompressorMode compressorMode, Actron485::OperatingMode operatingMode);
        static Actron485::OperatingMode to_actron_operating_mode(ClimateMode mode);

        static ClimateFanMode to_fan_mode(Actron485::FanMode mode);
        static Actron485::FanMode to_actron_fan_mode(ClimateFanMode mode);

        static const std::string to_preset(bool continuous_mode);
        static bool to_continuous_mode(const std::string preset);

};

}
}