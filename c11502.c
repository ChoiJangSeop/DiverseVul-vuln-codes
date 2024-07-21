MemoryRegion *memory_map_ptr(struct uc_struct *uc, hwaddr begin, size_t size, uint32_t perms, void *ptr)
{
    MemoryRegion *ram = g_new(MemoryRegion, 1);

    memory_region_init_ram_ptr(uc, ram, size, ptr);
    ram->perms = perms;
    if (ram->addr == -1) {
        // out of memory
        return NULL;
    }

    memory_region_add_subregion(uc->system_memory, begin, ram);

    if (uc->cpu) {
        tlb_flush(uc->cpu);
    }

    return ram;
}