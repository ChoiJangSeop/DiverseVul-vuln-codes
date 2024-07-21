i_alloc_byte_array(gs_memory_t * mem, uint num_elements, uint elt_size,
                   client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    obj_header_t *obj;

#ifdef MEMENTO
    if (Memento_failThisEvent())
        return NULL;
#endif

    obj = alloc_obj(imem, (ulong) num_elements * elt_size,
                    &st_bytes, ALLOC_DIRECT, cname);

    if_debug6m('A', mem, "[a%d:+b.]%s -bytes-*(%lu=%u*%u) = 0x%lx\n",
               alloc_trace_space(imem), client_name_string(cname),
               (ulong) num_elements * elt_size,
               num_elements, elt_size, (ulong) obj);
    return (byte *) obj;
}