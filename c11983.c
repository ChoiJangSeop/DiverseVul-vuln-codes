void qemu_input_event_send_key_delay(uint32_t delay_ms)
{
    if (!kbd_timer) {
        kbd_timer = timer_new_ms(QEMU_CLOCK_VIRTUAL, qemu_input_queue_process,
                                 &kbd_queue);
    }
    qemu_input_queue_delay(&kbd_queue, kbd_timer,
                           delay_ms ? delay_ms : kbd_default_delay_ms);
}