#pragma once

#include "mbed.h"
#include "stm32lib.h"

class NECIRReceive
{
public:
    NECIRReceive(PinName _pin, const Callback<void(uint32_t)>& _receive_callback);

    void infrared_rise();

    void infrared_fall();

    bool is_start_signal(long pulse_length, long space_length)
    {
        return pulse_length > 8100 && pulse_length < 9900 && space_length > 4050 && space_length < 4950;
    }

    bool is_logic_1(long pulse_length, long space_length)
    {
        long total_length = pulse_length + space_length;
        return total_length > 1800 && total_length < 2700;
    }

    bool is_logic_0(long pulse_length, long space_length)
    {
        long total_length = pulse_length + space_length;
        return total_length > 896 && total_length < 1344;
    }

private:
    InterruptIn infrared_in;
    Timer pulse_timer;
    Timer space_timer;
    __IO bool infrared_message_started = false;
    __IO uint32_t infrared_value = 0;
    __IO uint8_t infrared_bit_count = 0;
    __IO long pulse_length = 0;
    __IO long space_length = 0;
    Callback<void(uint32_t)> receive_callback;
};

class NECIRTransmit
{
public:
    NECIRTransmit(PinName _pin);

    void transmit(uint32_t _value);

    void on_space();

    void on_pulse();

private:
    DigitalOut infrared_out;
    uint32_t value;
    uint32_t mask;
    bool end_flag;
    Timeout pulse_timeout;
    Timeout space_timeout;
};
