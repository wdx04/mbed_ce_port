#include "mbed.h"
#include "Stepper28BYJ_48.h"
#include <chrono>

Stepper28BYJ_48 stepper(PA_12/*IN1*/, PA_11/*IN2*/, PA_0/*IN3*/, PA_2/*IN4*/);

int main()
{
    constexpr auto step_delay_wave_drive = 2000us;
    constexpr auto step_delay_full_step = 1750us;
    constexpr auto step_delay_half_step = 900us;
    while(true)
    {
        puts("Stepper Motor running in Wave Drive mode...");
        stepper.set_mode(StepperMode::WAVE_DRIVE);
        stepper.steps(2048, step_delay_wave_drive);
        ThisThread::sleep_for(chrono::duration_cast<chrono::milliseconds>(step_delay_wave_drive * 2048));
        stepper.steps(-2048, step_delay_wave_drive);
        ThisThread::sleep_for(chrono::duration_cast<chrono::milliseconds>(step_delay_wave_drive * 2048));
        puts("Stepper Motor running in Full Step mode...");
        stepper.set_mode(StepperMode::FULL_STEP);
        stepper.steps(2048, step_delay_full_step);
        ThisThread::sleep_for(chrono::duration_cast<chrono::milliseconds>(step_delay_full_step * 2048));
        stepper.steps(-2048, step_delay_full_step);
        ThisThread::sleep_for(chrono::duration_cast<chrono::milliseconds>(step_delay_full_step * 2048));
        puts("Stepper Motor running in Half Step mode...");
        stepper.set_mode(StepperMode::HALF_STEP);
        stepper.steps(4096, step_delay_half_step);
        ThisThread::sleep_for(chrono::duration_cast<chrono::milliseconds>(step_delay_half_step * 4096));
        stepper.steps(-4096, step_delay_half_step);
        ThisThread::sleep_for(chrono::duration_cast<chrono::milliseconds>(step_delay_half_step * 4096));
    }
}
