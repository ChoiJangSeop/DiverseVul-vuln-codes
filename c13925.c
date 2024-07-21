static void *pvrdma_map_to_pdir(PCIDevice *pdev, uint64_t pdir_dma,
                                uint32_t nchunks, size_t length)
{
    uint64_t *dir, *tbl;
    int tbl_idx, dir_idx, addr_idx;
    void *host_virt = NULL, *curr_page;

    if (!nchunks) {
        rdma_error_report("Got nchunks=0");
        return NULL;
    }

    dir = rdma_pci_dma_map(pdev, pdir_dma, TARGET_PAGE_SIZE);
    if (!dir) {
        rdma_error_report("Failed to map to page directory");
        return NULL;
    }

    tbl = rdma_pci_dma_map(pdev, dir[0], TARGET_PAGE_SIZE);
    if (!tbl) {
        rdma_error_report("Failed to map to page table 0");
        goto out_unmap_dir;
    }

    curr_page = rdma_pci_dma_map(pdev, (dma_addr_t)tbl[0], TARGET_PAGE_SIZE);
    if (!curr_page) {
        rdma_error_report("Failed to map the page 0");
        goto out_unmap_tbl;
    }

    host_virt = mremap(curr_page, 0, length, MREMAP_MAYMOVE);
    if (host_virt == MAP_FAILED) {
        host_virt = NULL;
        rdma_error_report("Failed to remap memory for host_virt");
        goto out_unmap_tbl;
    }
    trace_pvrdma_map_to_pdir_host_virt(curr_page, host_virt);

    rdma_pci_dma_unmap(pdev, curr_page, TARGET_PAGE_SIZE);

    dir_idx = 0;
    tbl_idx = 1;
    addr_idx = 1;
    while (addr_idx < nchunks) {
        if (tbl_idx == TARGET_PAGE_SIZE / sizeof(uint64_t)) {
            tbl_idx = 0;
            dir_idx++;
            rdma_pci_dma_unmap(pdev, tbl, TARGET_PAGE_SIZE);
            tbl = rdma_pci_dma_map(pdev, dir[dir_idx], TARGET_PAGE_SIZE);
            if (!tbl) {
                rdma_error_report("Failed to map to page table %d", dir_idx);
                goto out_unmap_host_virt;
            }
        }

        curr_page = rdma_pci_dma_map(pdev, (dma_addr_t)tbl[tbl_idx],
                                     TARGET_PAGE_SIZE);
        if (!curr_page) {
            rdma_error_report("Failed to map to page %d, dir %d", tbl_idx,
                              dir_idx);
            goto out_unmap_host_virt;
        }

        mremap(curr_page, 0, TARGET_PAGE_SIZE, MREMAP_MAYMOVE | MREMAP_FIXED,
               host_virt + TARGET_PAGE_SIZE * addr_idx);

        trace_pvrdma_map_to_pdir_next_page(addr_idx, curr_page, host_virt +
                                           TARGET_PAGE_SIZE * addr_idx);

        rdma_pci_dma_unmap(pdev, curr_page, TARGET_PAGE_SIZE);

        addr_idx++;

        tbl_idx++;
    }

    goto out_unmap_tbl;

out_unmap_host_virt:
    munmap(host_virt, length);
    host_virt = NULL;

out_unmap_tbl:
    rdma_pci_dma_unmap(pdev, tbl, TARGET_PAGE_SIZE);

out_unmap_dir:
    rdma_pci_dma_unmap(pdev, dir, TARGET_PAGE_SIZE);

    return host_virt;
}