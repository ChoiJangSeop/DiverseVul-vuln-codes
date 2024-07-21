static int _hostresolver_getaddrinfo(
    oe_resolver_t* resolver,
    const char* node,
    const char* service,
    const struct oe_addrinfo* hints,
    struct oe_addrinfo** res)
{
    int ret = OE_EAI_FAIL;
    uint64_t handle = 0;
    struct oe_addrinfo* head = NULL;
    struct oe_addrinfo* tail = NULL;
    struct oe_addrinfo* p = NULL;

    OE_UNUSED(resolver);

    if (res)
        *res = NULL;

    if (!res)
    {
        ret = OE_EAI_SYSTEM;
        OE_RAISE_ERRNO(OE_EINVAL);
    }

    /* Get the handle for enumerating addrinfo structures. */
    {
        int retval = OE_EAI_FAIL;

        if (oe_syscall_getaddrinfo_open_ocall(
                &retval, node, service, hints, &handle) != OE_OK)
        {
            ret = OE_EAI_SYSTEM;
            OE_RAISE_ERRNO(OE_EINVAL);
        }

        if (!handle)
        {
            ret = retval;
            goto done;
        }
    }

    /* Enumerate addrinfo structures. */
    for (;;)
    {
        int retval = 0;
        size_t canonnamelen = 0;

        if (!(p = oe_calloc(1, sizeof(struct oe_addrinfo))))
        {
            ret = OE_EAI_MEMORY;
            goto done;
        }

        /* Determine required size ai_addr and ai_canonname buffers. */
        if (oe_syscall_getaddrinfo_read_ocall(
                &retval,
                handle,
                &p->ai_flags,
                &p->ai_family,
                &p->ai_socktype,
                &p->ai_protocol,
                p->ai_addrlen,
                &p->ai_addrlen,
                NULL,
                canonnamelen,
                &canonnamelen,
                NULL) != OE_OK)
        {
            ret = OE_EAI_SYSTEM;
            OE_RAISE_ERRNO(OE_EINVAL);
        }

        /* If this is the final element in the enumeration. */
        if (retval == 1)
            break;

        /* Expecting that addr and canonname buffers were too small. */
        if (retval != -1 || oe_errno != OE_ENAMETOOLONG)
        {
            ret = OE_EAI_SYSTEM;
            OE_RAISE_ERRNO(oe_errno);
        }

        if (p->ai_addrlen && !(p->ai_addr = oe_calloc(1, p->ai_addrlen)))
        {
            ret = OE_EAI_MEMORY;
            goto done;
        }

        if (canonnamelen && !(p->ai_canonname = oe_calloc(1, canonnamelen)))
        {
            ret = OE_EAI_MEMORY;
            goto done;
        }

        if (oe_syscall_getaddrinfo_read_ocall(
                &retval,
                handle,
                &p->ai_flags,
                &p->ai_family,
                &p->ai_socktype,
                &p->ai_protocol,
                p->ai_addrlen,
                &p->ai_addrlen,
                p->ai_addr,
                canonnamelen,
                &canonnamelen,
                p->ai_canonname) != OE_OK)
        {
            ret = OE_EAI_SYSTEM;
            OE_RAISE_ERRNO(OE_EINVAL);
        }

        /* Append to the list. */
        if (tail)
        {
            tail->ai_next = p;
            tail = p;
        }
        else
        {
            head = p;
            tail = p;
        }

        p = NULL;
    }

    /* Close the enumeration. */
    if (handle)
    {
        int retval = -1;

        if (oe_syscall_getaddrinfo_close_ocall(&retval, handle) != OE_OK)
        {
            ret = OE_EAI_SYSTEM;
            OE_RAISE_ERRNO(OE_EINVAL);
        }

        handle = 0;

        if (retval != 0)
        {
            ret = OE_EAI_SYSTEM;
            OE_RAISE_ERRNO(oe_errno);
        }
    }

    /* If the list is empty. */
    if (!head)
    {
        ret = OE_EAI_SYSTEM;
        OE_RAISE_ERRNO(OE_EINVAL);
    }

    *res = head;
    head = NULL;
    tail = NULL;
    ret = 0;

done:

    if (handle)
    {
        int retval;
        oe_syscall_getaddrinfo_close_ocall(&retval, handle);
    }

    if (head)
        oe_freeaddrinfo(head);

    if (p)
        oe_freeaddrinfo(p);

    return ret;
}