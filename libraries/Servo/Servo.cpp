#include "Servo.h"
#include <chrono>
#include <ratio>

namespace servo
{

  Servo::Servo(PinName pin_, chrono::microseconds min_duty_, chrono::microseconds max_duty_, float min_angle_, 
  float max_angle_, chrono::microseconds period_)
    : servo(pin_), min_duty(min_duty_), max_duty(max_duty_), min_angle(min_angle_), max_angle(max_angle_)
    , current_duty((min_duty_ + max_duty_) / 2), period(period_)
  {
  }

  Servo::~Servo()
  {
    ticker.detach();
  }

  void Servo::init(chrono::microseconds init_duty)
  {
    ticker.detach();
    servo.period_us(period.count() / 1000);
    servo.pulsewidth_us(init_duty.count());
    ticker.attach(mbed::callback(this, &Servo::callback), chrono::duration_cast<chrono::microseconds>(period));
    is_initialized = true;
    current_state = work_state::idle;
  }

  void Servo::go_center()
  {
    set_current_duty((min_duty + max_duty) / 2);
  }

  void Servo::go_angle(float angle)
  {
    set_current_duty(angle_to_duty(angle));
  }

  void Servo::set_current_duty(chrono::nanoseconds new_duty)
  {
    if(!is_initialized)
    {
        init(chrono::duration_cast<chrono::microseconds>(new_duty));
        return;
    }
    if(servo.read_period_us() != period.count() / 1000)
    {
      servo.period_us(period.count() / 1000);
    }
    servo.pulsewidth_us(new_duty.count() / 1000);
    current_duty = new_duty;
    current_state = work_state::idle;
  }

  void Servo::single_trip(float angle, chrono::milliseconds trip_time)
  {
    if(current_state == work_state::unmanaged)
    {
      go_angle(angle);
    }
    else
    {
      int step_count = int((trip_time.count() * 1000000L + period.count() - 1) / period.count());
      if(step_count <= 0)
      {
        go_angle(angle);
      }
      else
      {
        start_duty = current_duty;
        end_duty = angle_to_duty(angle);
        current_speed = float(end_duty.count() - start_duty.count()) / float(step_count);
        current_state = work_state::single_trip;
      }
    }
  }

  void Servo::single_trip_center(chrono::milliseconds trip_time)
  {
    single_trip((min_angle + max_angle) / 2.0f, trip_time);
  }

  void Servo::start_looping(float start_angle, float end_angle, chrono::milliseconds trip_time)
  {
    if(current_state == work_state::unmanaged)
    {
      go_angle(start_angle);
    }
    int step_count = int((trip_time.count() * 1000000L + period.count() - 1) / period.count());
    if(step_count <= 0)
    {
      step_count = 1;
    }
    start_duty = angle_to_duty(start_angle);
    end_duty = angle_to_duty(end_angle);
    current_speed = float(end_duty.count() - start_duty.count()) / float(step_count);
    current_state = work_state::looping;
  }

  void Servo::stop()
  {
    current_speed = 0.0f;
    current_state = work_state::idle;
  }

  chrono::nanoseconds Servo::angle_to_duty(float angle) const
  {
    float min_ratio = (max_angle - angle) / (max_angle - min_angle);
    float max_ratio = (angle - min_angle) / (max_angle - min_angle);
    chrono::nanoseconds duty { static_cast<long>(float(min_duty.count()) * min_ratio + float(max_duty.count()) * max_ratio) };
    if(duty < min_duty) duty = min_duty;
    if(duty > max_duty) duty = max_duty;
    return duty;
  }

  float Servo::duty_to_angle(chrono::nanoseconds duty) const
  {
    if(duty < min_duty) duty = min_duty;
    if(duty > max_duty) duty = max_duty;
    float min_ratio = float((max_duty - duty) .count()) / float((max_duty - min_duty).count());
    float max_ratio = float((duty - min_duty).count()) / float((max_duty - min_duty).count());
    float angle = min_angle * min_ratio + max_angle * max_ratio;
    return angle;
  }

  int Servo::get_step_count(chrono::milliseconds trip_time) const
  {
    int step_time_ns = period.count();
    return (trip_time.count() * 1000000 + step_time_ns - 1) /  step_time_ns;
  }

  void Servo::callback()
  {
    if(current_state == work_state::single_trip || current_state == work_state::looping)
    {
      int current_step = int(float(current_duty.count() - start_duty.count()) / current_speed + 0.5f);
      current_duty = chrono::nanoseconds(start_duty.count() + int((current_step + 1) * current_speed + 0.5f));
      if(servo.read_period_us() != period.count() / 1000)
      {
        servo.period_us(period.count() / 1000);
      }
      servo.pulsewidth_us(current_duty.count() / 1000);
      if(std::abs((current_duty - end_duty).count()) < 500)
      {
        if(current_state == work_state::single_trip)
        {
          current_speed = 0.0f;
          current_state = work_state::idle;
        }
        else
        {
          std::swap(start_duty, end_duty);
          current_speed = -current_speed;
        }
      }
    }
  }

}
