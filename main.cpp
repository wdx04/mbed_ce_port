#include "mbed.h"
#include "EventQueue.h"

EventQueue *event_queue = mbed_event_queue();

DigitalOut led(LED1);

void reverse_led()
{
    led = !led;
}

int main()
{
    sleep_manager_lock_deep_sleep();
    printf("Application started and running!\n");
    led  = 0;
    event_queue->call_every(200ms, &reverse_led);
    event_queue->dispatch_forever();
}
