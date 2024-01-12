#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/climate/climate.h"
#include "esphome/core/defines.h"
#include "esphome/components/fan/fan.h"
#include "Actron485.h"

namespace esphome {
namespace actron485 {

class Actron485ZoneClimate : public climate::Climate, public Component {
    public:
        Actron485ZoneClimate();
        void setup() override { }

        void update();

        void dump_config() override;

        void set_ultima_adjusts_master_setpoint(bool enable) { ultima_adjusts_master_setpoint_ = enable; }
        void set_zone_number(int number) { number_ = number; }
        void set_controller(Actron485::Controller *controller) { actron_controller_ = controller; }

    protected:
        int number_;
        bool ultima_adjusts_master_setpoint_;
        Actron485::Controller *actron_controller_ = NULL;

        /// Override control to change settings of the climate device.
        void control(const climate::ClimateCall &call) override;

        /// Return the traits of this controller.
        climate::ClimateTraits traits() override;
};

}
}