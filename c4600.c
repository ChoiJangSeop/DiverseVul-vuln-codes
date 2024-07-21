void kbd_put_keysym_console(QemuConsole *s, int keysym)
{
    uint8_t buf[16], *q;
    int c;

    if (!s || (s->console_type == GRAPHIC_CONSOLE))
        return;

    switch(keysym) {
    case QEMU_KEY_CTRL_UP:
        console_scroll(s, -1);
        break;
    case QEMU_KEY_CTRL_DOWN:
        console_scroll(s, 1);
        break;
    case QEMU_KEY_CTRL_PAGEUP:
        console_scroll(s, -10);
        break;
    case QEMU_KEY_CTRL_PAGEDOWN:
        console_scroll(s, 10);
        break;
    default:
        /* convert the QEMU keysym to VT100 key string */
        q = buf;
        if (keysym >= 0xe100 && keysym <= 0xe11f) {
            *q++ = '\033';
            *q++ = '[';
            c = keysym - 0xe100;
            if (c >= 10)
                *q++ = '0' + (c / 10);
            *q++ = '0' + (c % 10);
            *q++ = '~';
        } else if (keysym >= 0xe120 && keysym <= 0xe17f) {
            *q++ = '\033';
            *q++ = '[';
            *q++ = keysym & 0xff;
        } else if (s->echo && (keysym == '\r' || keysym == '\n')) {
            console_puts(s->chr, (const uint8_t *) "\r", 1);
            *q++ = '\n';
        } else {
            *q++ = keysym;
        }
        if (s->echo) {
            console_puts(s->chr, buf, q - buf);
        }
        if (s->chr->chr_read) {
            qemu_fifo_write(&s->out_fifo, buf, q - buf);
            kbd_send_chars(s);
        }
        break;
    }
}