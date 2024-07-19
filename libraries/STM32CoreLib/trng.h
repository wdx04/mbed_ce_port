#pragma once

#include "mbed.h"
#include "trng_api.h"

class TRNG
{
public:
    TRNG()
    {
        trng_init(&trng_obj);
    }

    ~TRNG()
    {
        trng_free(&trng_obj);
    }

    size_t get_bytes(uint8_t *output, size_t length)
    {
        size_t output_length = 0;
        trng_get_bytes(&trng_obj, output, length, &output_length);
        return output_length;
    }

    template<typename ValueType>
    ValueType get()
    {
        ValueType result = ValueType(0);
        get_bytes(reinterpret_cast<uint8_t*>(&result), sizeof(result));
        return result;
    }

private:
    trng_t trng_obj;
};
