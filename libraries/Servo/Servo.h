#pragma once

#include <PinNames.h>
#include <drivers/Ticker.h>
#include <drivers/PwmOut.h>
#include <chrono>

namespace servo
{
  namespace chrono = std::chrono;
  using namespace std::literals::chrono_literals;

  class Servo
  {
  public:
    // setup the servo attached to the given pin
    Servo(PinName pin_, chrono::microseconds min_duty_ = 1000us, chrono::microseconds max_duty_ = 2000us, float min_angle_ = 0.0f, 
      float max_angle_ = 90.0f, chrono::microseconds period_ = 20000us);

    ~Servo();
  
    // attach the ticker handler and bring the servo to the start position
    // this function is automatically called by other functions that moves the servo
    void init(chrono::microseconds init_duty = 1500us);

    // bring the servo the target position by specifying pulse width
    void set_current_duty(chrono::nanoseconds new_duty);

    // bring the servo to the center as fast as possible
    void go_center();

    // bring the servo to the given angle as fast as possible
    void go_angle(float angle);

    // bring the servo to the center smoothly by the given trip time
    void single_trip_center(chrono::milliseconds trip_time);

    // bring the servo to the given angle smoothly by the given trip time
    void single_trip(float angle, chrono::milliseconds trip_time);

    // loop forever between two angles, until stop() is called, trip_time is the one-way trip time between start angle and end angle
    void start_looping(float start_angle, float end_angle, chrono::milliseconds trip_time);

    // stop the servo
    void stop();

    // convert angle to pulse width
    chrono::nanoseconds angle_to_duty(float angle) const;

    // convert pulse width to angle
    float duty_to_angle(chrono::nanoseconds duty) const;

    // get the number of micro steps in the given trip time
    int get_step_count(chrono::milliseconds trip_time) const;

  protected:
    void callback();

  private:
    mbed::PwmOut servo;
    mbed::Ticker ticker;
    const chrono::nanoseconds period, min_duty, max_duty;
    const float min_angle, max_angle;
    chrono::nanoseconds current_duty, start_duty, end_duty;
    enum class work_state { unmanaged, idle, single_trip, looping } current_state = work_state::unmanaged;
    float current_speed = 0.0f;
    bool is_initialized = false;
  };

}

using servo::Servo;
