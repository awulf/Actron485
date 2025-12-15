#pragma once

#include "Actron485.h"
#include "actron485_climate.h"
#include "esphome/components/climate/climate_traits.h"
#include "Arduino.h"

namespace esphome {
namespace actron485 {

static const char *const TAG = "actron485.climate";
static const unsigned long DEBOUNCE_MILLIS = 3000; //3seconds

template<typename T> void update_property(T &property, const T &value, bool &flag) {
    if (property != value) {
        property = value;
        flag = true;
    }
}

class Converter {
    public:

        static constexpr const char *FAN_STANDARD = "Standard Fan";
        static constexpr const char *FAN_CONTINUOUS = "Continuous Fan";

        static ClimateMode to_climate_mode(Actron485::OperatingMode mode);
        static ClimateAction to_climate_action(Actron485::CompressorMode compressorMode, Actron485::OperatingMode operatingMode);
        static Actron485::OperatingMode to_actron_operating_mode(ClimateMode mode);

        static ClimateFanMode to_fan_mode(Actron485::FanMode mode);
        static Actron485::FanMode to_actron_fan_mode(ClimateFanMode mode);

        static const char *to_preset(bool continuous_mode);
        static bool to_continuous_mode(const char *preset);

};

}
}