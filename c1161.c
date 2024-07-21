static void ioport_write(void *opaque, uint32_t addr, uint32_t val)
{
    PCIQXLDevice *d = opaque;
    uint32_t io_port = addr - d->io_base;

    switch (io_port) {
    case QXL_IO_RESET:
    case QXL_IO_SET_MODE:
    case QXL_IO_MEMSLOT_ADD:
    case QXL_IO_MEMSLOT_DEL:
    case QXL_IO_CREATE_PRIMARY:
    case QXL_IO_UPDATE_IRQ:
    case QXL_IO_LOG:
        break;
    default:
        if (d->mode != QXL_MODE_VGA) {
            break;
        }
        dprint(d, 1, "%s: unexpected port 0x%x (%s) in vga mode\n",
            __func__, io_port, io_port_to_string(io_port));
        return;
    }

    switch (io_port) {
    case QXL_IO_UPDATE_AREA:
    {
        QXLRect update = d->ram->update_area;
        qxl_spice_update_area(d, d->ram->update_surface,
                              &update, NULL, 0, 0);
        break;
    }
    case QXL_IO_NOTIFY_CMD:
        qemu_spice_wakeup(&d->ssd);
        break;
    case QXL_IO_NOTIFY_CURSOR:
        qemu_spice_wakeup(&d->ssd);
        break;
    case QXL_IO_UPDATE_IRQ:
        qxl_set_irq(d);
        break;
    case QXL_IO_NOTIFY_OOM:
        if (!SPICE_RING_IS_EMPTY(&d->ram->release_ring)) {
            break;
        }
        pthread_yield();
        if (!SPICE_RING_IS_EMPTY(&d->ram->release_ring)) {
            break;
        }
        d->oom_running = 1;
        qxl_spice_oom(d);
        d->oom_running = 0;
        break;
    case QXL_IO_SET_MODE:
        dprint(d, 1, "QXL_SET_MODE %d\n", val);
        qxl_set_mode(d, val, 0);
        break;
    case QXL_IO_LOG:
        if (d->guestdebug) {
            fprintf(stderr, "qxl/guest-%d: %ld: %s", d->id,
                    qemu_get_clock_ns(vm_clock), d->ram->log_buf);
        }
        break;
    case QXL_IO_RESET:
        dprint(d, 1, "QXL_IO_RESET\n");
        qxl_hard_reset(d, 0);
        break;
    case QXL_IO_MEMSLOT_ADD:
        if (val >= NUM_MEMSLOTS) {
            qxl_guest_bug(d, "QXL_IO_MEMSLOT_ADD: val out of range");
            break;
        }
        if (d->guest_slots[val].active) {
            qxl_guest_bug(d, "QXL_IO_MEMSLOT_ADD: memory slot already active");
            break;
        }
        d->guest_slots[val].slot = d->ram->mem_slot;
        qxl_add_memslot(d, val, 0);
        break;
    case QXL_IO_MEMSLOT_DEL:
        if (val >= NUM_MEMSLOTS) {
            qxl_guest_bug(d, "QXL_IO_MEMSLOT_DEL: val out of range");
            break;
        }
        qxl_del_memslot(d, val);
        break;
    case QXL_IO_CREATE_PRIMARY:
        if (val != 0) {
            qxl_guest_bug(d, "QXL_IO_CREATE_PRIMARY: val != 0");
            break;
        }
        dprint(d, 1, "QXL_IO_CREATE_PRIMARY\n");
        d->guest_primary.surface = d->ram->create_surface;
        qxl_create_guest_primary(d, 0);
        break;
    case QXL_IO_DESTROY_PRIMARY:
        if (val != 0) {
            qxl_guest_bug(d, "QXL_IO_DESTROY_PRIMARY: val != 0");
            break;
        }
        dprint(d, 1, "QXL_IO_DESTROY_PRIMARY (%s)\n", qxl_mode_to_string(d->mode));
        qxl_destroy_primary(d);
        break;
    case QXL_IO_DESTROY_SURFACE_WAIT:
        qxl_spice_destroy_surface_wait(d, val);
        break;
    case QXL_IO_DESTROY_ALL_SURFACES:
        qxl_spice_destroy_surfaces(d);
        break;
    default:
        fprintf(stderr, "%s: ioport=0x%x, abort()\n", __FUNCTION__, io_port);
        abort();
    }
}