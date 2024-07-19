#include "mbed.h"
#include "Stepper28BYJ_48.h"

static uint8_t wave_drive_values[] = { 0x01, 0x02, 0x04, 0x08 };
static uint8_t full_step_values[] = { 0x03, 0x06, 0x0C, 0x09 };
static uint8_t half_step_values[] = { 0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09 };

static int non_negative_mod(int x, int y)
{
    int r = x % y; // assuming y > 0
    return r >= 0 ? r: r + y;
}

Stepper28BYJ_48::Stepper28BYJ_48(PinName p1, PinName p2, PinName  p3, PinName p4)
    : bo(p1, p2, p3, p4)
{
    update_output();
}

int Stepper28BYJ_48::get_current_step() const
{
    if(mode < StepperMode::HALF_STEP)
    {
        return current_half_step / 2;
    }
    else
    {
        return current_half_step;
    }
}

void Stepper28BYJ_48::update_output()
{
    switch(mode)
    {
    case StepperMode::WAVE_DRIVE:
        bo.write(wave_drive_values[non_negative_mod(current_half_step / 2,  4)]);
        break;    
    case StepperMode::FULL_STEP:
        bo.write(full_step_values[non_negative_mod(current_half_step / 2,  4)]);
        break;    
    case StepperMode::HALF_STEP:
        bo.write(half_step_values[non_negative_mod(current_half_step,  8)]);
        break;    
    }
}

void Stepper28BYJ_48::one_step(bool direction_cw)
{
    ticker.detach();
    switch(mode)
    {
    case StepperMode::WAVE_DRIVE:
        current_half_step += (direction_cw ? 2: -2);
        break;    
    case StepperMode::FULL_STEP:
        current_half_step += (direction_cw ? 2: -2);
        break;    
    case StepperMode::HALF_STEP:
        current_half_step += (direction_cw ? 1: -1);
        break;    
    }
    update_output();
}

void Stepper28BYJ_48::steps(int count, chrono::microseconds delay_between_steps)
{
    ticker.detach();
    if(count != 0)
    {
        if(mode < StepperMode::HALF_STEP)
        {
            target_half_step = current_half_step + count * 2;
        }
        else
        {
            target_half_step = current_half_step + count;
        }
        ticker.attach(mbed::callback(this, &Stepper28BYJ_48::steps_callback), delay_between_steps);
    }
}

void Stepper28BYJ_48::steps_callback()
{
    if(current_half_step < target_half_step)
    {
        switch(mode)
        {
        case StepperMode::WAVE_DRIVE:
            current_half_step += 2;
            break;    
        case StepperMode::FULL_STEP:
            current_half_step += 2;
            break;    
        case StepperMode::HALF_STEP:
            current_half_step += 1;
            break;    
        }
        if(current_half_step >= target_half_step)
        {
            ticker.detach();
        }
    }
    else
    {
        switch(mode)
        {
        case StepperMode::WAVE_DRIVE:
            current_half_step += -2;
            break;    
        case StepperMode::FULL_STEP:
            current_half_step += -2;
            break;    
        case StepperMode::HALF_STEP:
            current_half_step += -1;
            break;    
        }
        if(current_half_step <= target_half_step)
        {
            ticker.detach();
        }
    }
    update_output();
}

StepperMode Stepper28BYJ_48::get_mode() const
{
    return mode;
}

void Stepper28BYJ_48::set_mode(StepperMode mode_)
{
    ticker.detach();
    if(mode != mode_)
    {
        mode = mode_;
        update_output();
    }
}

float Stepper28BYJ_48::steps_to_angle(int count) const
{
    if(mode < StepperMode::HALF_STEP)
    {
        return 11.25f / 64.0f * float(count);
    }
    else
    {
        return 5.625f / 64.0f * float(count);
    }
}

int Stepper28BYJ_48::angle_to_steps(float angle) const
{
    if(mode < StepperMode::HALF_STEP)
    {
        return angle * 64.0f / 11.25f;
    }
    else
    {
        return angle * 64.0f / 5.625f;
    }
}
