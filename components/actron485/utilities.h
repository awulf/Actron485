#pragma once

#include "Actron485.h"
#include "actron485_climate.h"
#include "esphome/components/climate/climate_traits.h"

namespace esphome {
namespace actron485 {

class Converter {
    public:
        static const std::string FAN_NORMAL;
        static const std::string FAN_CONTINUOUS;

        static ClimateMode to_climate_mode(Actron485::OperatingMode mode);

        static ClimateFanMode to_fan_mode(Actron485::FanMode mode);

        static const std::string to_preset(bool continuous_mode);

};

}
}