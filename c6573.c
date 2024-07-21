int gx_device_subclass(gx_device *dev_to_subclass, gx_device *new_prototype, unsigned int private_data_size)
{
    gx_device *child_dev;
    void *psubclass_data;
    gs_memory_struct_type_t *a_std;
    int dynamic = dev_to_subclass->stype_is_dynamic;
    char *ptr, *ptr1;

    /* If this happens we are stuffed, as there is no way to get hold
     * of the original device's stype structure, which means we cannot
     * allocate a replacement structure. Abort if so.
     * Also abort if the new_prototype device struct is too large.
     */
    if (!dev_to_subclass->stype ||
        dev_to_subclass->stype->ssize < new_prototype->params_size)
        return_error(gs_error_VMerror);

    /* We make a 'stype' structure for our new device, and copy the old stype into it
     * This means our new device will always have the 'stype_is_dynamic' flag set
     */
    a_std = (gs_memory_struct_type_t *)
        gs_alloc_bytes_immovable(dev_to_subclass->memory->non_gc_memory, sizeof(*a_std),
                                 "gs_device_subclass(stype)");
    if (!a_std)
        return_error(gs_error_VMerror);
    *a_std = *dev_to_subclass->stype;
    a_std->ssize = dev_to_subclass->params_size;

    /* Allocate a device structure for the new child device */
    child_dev = gs_alloc_struct_immovable(dev_to_subclass->memory->stable_memory, gx_device, a_std,
                                        "gs_device_subclass(device)");
    if (child_dev == 0) {
        gs_free_const_object(dev_to_subclass->memory->non_gc_memory, a_std, "gs_device_subclass(stype)");
        return_error(gs_error_VMerror);
    }

    /* Make sure all methods are filled in, note this won't work for a forwarding device
     * so forwarding devices will have to be filled in before being subclassed. This doesn't fill
     * in the fill_rectangle proc, that gets done in the ultimate device's open proc.
     */
    gx_device_fill_in_procs(dev_to_subclass);
    memcpy(child_dev, dev_to_subclass, dev_to_subclass->stype->ssize);
    child_dev->stype = a_std;
    child_dev->stype_is_dynamic = 1;

    psubclass_data = (void *)gs_alloc_bytes(dev_to_subclass->memory->non_gc_memory, private_data_size, "subclass memory for subclassing device");
    if (psubclass_data == 0){
        gs_free_const_object(dev_to_subclass->memory->non_gc_memory, a_std, "gs_device_subclass(stype)");
        gs_free_object(dev_to_subclass->memory->stable_memory, child_dev, "free subclass memory for subclassing device");
        return_error(gs_error_VMerror);
    }
    memset(psubclass_data, 0x00, private_data_size);

    gx_copy_device_procs(dev_to_subclass, child_dev, new_prototype);
    set_dev_proc(dev_to_subclass, fill_rectangle, dev_proc(new_prototype, fill_rectangle));
    set_dev_proc(dev_to_subclass, copy_planes, dev_proc(new_prototype, copy_planes));
    dev_to_subclass->finalize = new_prototype->finalize;
    dev_to_subclass->dname = new_prototype->dname;
    if (dev_to_subclass->icc_struct)
        rc_increment(dev_to_subclass->icc_struct);
    if (dev_to_subclass->PageList)
        rc_increment(dev_to_subclass->PageList);

    /* In case the new device we're creating has already been initialised, copy
     * its additional data.
     */
    ptr = ((char *)dev_to_subclass) + sizeof(gx_device);
    ptr1 = ((char *)new_prototype) + sizeof(gx_device);
    memcpy(ptr, ptr1, new_prototype->params_size - sizeof(gx_device));

    /* We have to patch up the "type" parameters that the memory manage/garbage
     * collector will use, as well.
     */
    gs_set_object_type(child_dev->memory, dev_to_subclass, new_prototype->stype);

    /* If the original device's stype structure was dynamically allocated, we need
     * to 'fixup' the contents, it's procs need to point to the new device's procs
     * for instance.
     */
    if (dynamic) {
        if (new_prototype->stype) {
            a_std = (gs_memory_struct_type_t *)dev_to_subclass->stype;
            *a_std = *new_prototype->stype;
        } else {
            gs_free_const_object(child_dev->memory->non_gc_memory, dev_to_subclass->stype,
                             "unsubclass");
            dev_to_subclass->stype = NULL;
            dev_to_subclass->stype_is_dynamic = 0;
        }
    }

    dev_to_subclass->subclass_data = psubclass_data;
    dev_to_subclass->child = child_dev;
    if (child_dev->parent) {
        dev_to_subclass->parent = child_dev->parent;
        child_dev->parent->child = dev_to_subclass;
    }
    if (child_dev->child) {
        child_dev->child->parent = child_dev;
    }
    child_dev->parent = dev_to_subclass;

    return 0;
}