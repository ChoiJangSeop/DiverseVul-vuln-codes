void qemu_spice_add_memslot(SimpleSpiceDisplay *ssd, QXLDevMemSlot *memslot)
{
    ssd->worker->add_memslot(ssd->worker, memslot);
}