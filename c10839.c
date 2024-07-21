static uint16_t nvme_map_addr(NvmeCtrl *n, NvmeSg *sg, hwaddr addr, size_t len)
{
    bool cmb = false, pmr = false;

    if (!len) {
        return NVME_SUCCESS;
    }

    trace_pci_nvme_map_addr(addr, len);

    if (nvme_addr_is_cmb(n, addr)) {
        cmb = true;
    } else if (nvme_addr_is_pmr(n, addr)) {
        pmr = true;
    }

    if (cmb || pmr) {
        if (sg->flags & NVME_SG_DMA) {
            return NVME_INVALID_USE_OF_CMB | NVME_DNR;
        }

        if (sg->iov.niov + 1 > IOV_MAX) {
            goto max_mappings_exceeded;
        }

        if (cmb) {
            return nvme_map_addr_cmb(n, &sg->iov, addr, len);
        } else {
            return nvme_map_addr_pmr(n, &sg->iov, addr, len);
        }
    }

    if (!(sg->flags & NVME_SG_DMA)) {
        return NVME_INVALID_USE_OF_CMB | NVME_DNR;
    }

    if (sg->qsg.nsg + 1 > IOV_MAX) {
        goto max_mappings_exceeded;
    }

    qemu_sglist_add(&sg->qsg, addr, len);

    return NVME_SUCCESS;

max_mappings_exceeded:
    NVME_GUEST_ERR(pci_nvme_ub_too_many_mappings,
                   "number of mappings exceed 1024");
    return NVME_INTERNAL_DEV_ERROR | NVME_DNR;
}