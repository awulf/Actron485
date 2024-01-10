#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/climate/climate.h"
#include "esphome/core/defines.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/fan/fan.h"
#include "Actron485.h"

namespace esphome {
namespace actron485 {

class Actron485ZoneFan : public fan::Fan, public Component {
    public:
        Actron485ZoneFan();
        void setup() override;

        void update();

        void dump_config() override;
        float get_setup_priority() const override { return 0; }

        fan::FanTraits get_traits() override { return fan::FanTraits(false, false, false, 1); }

        void set_zone_number(int number) { number_ = number; }
        void set_controller(Actron485::Controller *controller) { actron_controller_ = controller; }

    protected:
        void control(const fan::FanCall &call);

        int number_;
        Actron485::Controller *actron_controller_ = NULL;
};

}
}