#include "mbed.h"
#include "necir.h"

NECIRReceive::NECIRReceive(PinName _pin, const Callback<void(uint32_t)>& _receive_callback)
    : infrared_in(_pin), receive_callback(_receive_callback)
{
    infrared_in.mode(PullUp);
    infrared_in.rise(callback(this, &NECIRReceive::infrared_rise));
    infrared_in.fall(callback(this, &NECIRReceive::infrared_fall));
    space_timer.start();
}

void NECIRReceive::infrared_rise()
{
    space_timer.reset();
    space_timer.start();
    pulse_timer.stop();
    pulse_length = (long) pulse_timer.elapsed_time().count();
}

void NECIRReceive::infrared_fall()
{
    pulse_timer.reset();
    pulse_timer.start();
    space_timer.stop();
    space_length = (long) space_timer.elapsed_time().count();
    if(pulse_length != 0)
    {
        if(is_start_signal(pulse_length, space_length))
        {
            infrared_message_started = true;
            infrared_value = 0;
            infrared_bit_count = 0;
        }
        else
        {
            if(infrared_message_started)
            {
                if(is_logic_1(pulse_length, space_length))
                {
                    infrared_value = (1U << infrared_bit_count) | infrared_value;
                    infrared_bit_count++;
                }
                else if(is_logic_0(pulse_length, space_length))
                {
                    infrared_bit_count++;
                }
                else
                {
                    infrared_message_started = false;
                }
                if(infrared_bit_count == 32)
                {
                    infrared_message_started = false;
                    receive_callback(infrared_value);
                }
            }
        }
    }
}

NECIRTransmit::NECIRTransmit(PinName _pin)
    : infrared_out(_pin, 1)
{
}

void NECIRTransmit::transmit(uint32_t _value)
{
    int total_time_us = 13500;
    uint32_t temp_mask = 1;
    for(int i = 0; i < 32; i++, temp_mask <<= 1)
    {
        if(_value & temp_mask)
        {
            total_time_us += 2250;
        }
        else
        {
            total_time_us += 1120;
        }
    }
    total_time_us += 560; // end signal
    value = _value;
    mask = 1;
    end_flag = false;
    infrared_out = 0;
    space_timeout.attach(callback(this, &NECIRTransmit::on_space), 9000us);
    pulse_timeout.attach(callback(this, &NECIRTransmit::on_pulse), 13500us);
    chrono::milliseconds sleep_time(total_time_us / 1000 + 2);
    ThisThread::sleep_for(sleep_time);
    pulse_timeout.detach();
    space_timeout.detach();
    infrared_out = 1;
}

void NECIRTransmit::on_space()
{
    infrared_out = 1;
}

void NECIRTransmit::on_pulse()
{
    // 注：在STM32L4中，attach自身有10us的延迟
    infrared_out = 0;
    space_timeout.attach(callback(this, &NECIRTransmit::on_space), 550us);
    if(!end_flag)
    {
        if(value & mask)
        {
            pulse_timeout.attach(callback(this, &NECIRTransmit::on_pulse), 2230us);
        }
        else
        {
            pulse_timeout.attach(callback(this, &NECIRTransmit::on_pulse), 1100us);
        }
        mask = mask << 1;
        if(mask == 0)
        {
            end_flag = true;
        }
    }
}
