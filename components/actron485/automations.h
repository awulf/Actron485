#pragma once

#include "esphome/core/automation.h"
#include "actron485_climate.h"

namespace esphome {
namespace actron485 {

template<typename... Ts> class ActionBase : public Action<Ts...> {
    protected:
        Actron485Climate *parent_;
};

template<typename... Ts> class PowerOnAction : public ActionBase<Ts...> {
    public:
        PowerOnAction(Actron485Climate *parent) { this->parent_ = parent; }
        void play(Ts... x) override { this->parent_->power_on(); }
};

template<typename... Ts> class PowerOffAction : public ActionBase<Ts...> {
    public:
        PowerOffAction(Actron485Climate *parent) { this->parent_ = parent; }
        void play(Ts... x) override { this->parent_->power_off(); }
};

template<typename... Ts> class PowerToggleAction : public ActionBase<Ts...> {
    public:
        PowerToggleAction(Actron485Climate *parent) { this->parent_ = parent; }
        void play(Ts... x) override { this->parent_->power_toggle(); }
};

}
}