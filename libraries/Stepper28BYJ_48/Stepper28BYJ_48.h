#pragma once

#include "mbed.h"

enum class StepperMode
{
    WAVE_DRIVE = 0,
    FULL_STEP = 1,
    HALF_STEP = 2
};

class Stepper28BYJ_48
{
public:
    Stepper28BYJ_48(PinName p1, PinName p2, PinName  p3, PinName p4);

    StepperMode get_mode() const;

    void set_mode(StepperMode mode_);

    int get_current_step() const;

    void one_step(bool direction_cw);

    void steps(int count, chrono::microseconds delay_between_steps = 2000us);

    float steps_to_angle(int count) const;

    int angle_to_steps(float angle) const;

private:
    void update_output();

    void steps_callback();

    BusOut bo;
    Ticker ticker;
    int current_half_step = 0;
    int target_half_step = 0;
    StepperMode mode = StepperMode::WAVE_DRIVE;
};
