static ssize_t virtio_net_receive_rcu(NetClientState *nc, const uint8_t *buf,
                                      size_t size, bool no_rss)
{
    VirtIONet *n = qemu_get_nic_opaque(nc);
    VirtIONetQueue *q = virtio_net_get_subqueue(nc);
    VirtIODevice *vdev = VIRTIO_DEVICE(n);
    VirtQueueElement *elems[VIRTQUEUE_MAX_SIZE];
    size_t lens[VIRTQUEUE_MAX_SIZE];
    struct iovec mhdr_sg[VIRTQUEUE_MAX_SIZE];
    struct virtio_net_hdr_mrg_rxbuf mhdr;
    unsigned mhdr_cnt = 0;
    size_t offset, i, guest_offset, j;
    ssize_t err;

    if (!virtio_net_can_receive(nc)) {
        return -1;
    }

    if (!no_rss && n->rss_data.enabled && n->rss_data.enabled_software_rss) {
        int index = virtio_net_process_rss(nc, buf, size);
        if (index >= 0) {
            NetClientState *nc2 = qemu_get_subqueue(n->nic, index);
            return virtio_net_receive_rcu(nc2, buf, size, true);
        }
    }

    /* hdr_len refers to the header we supply to the guest */
    if (!virtio_net_has_buffers(q, size + n->guest_hdr_len - n->host_hdr_len)) {
        return 0;
    }

    if (!receive_filter(n, buf, size))
        return size;

    offset = i = 0;

    while (offset < size) {
        VirtQueueElement *elem;
        int len, total;
        const struct iovec *sg;

        total = 0;

        if (i == VIRTQUEUE_MAX_SIZE) {
            virtio_error(vdev, "virtio-net unexpected long buffer chain");
            err = size;
            goto err;
        }

        elem = virtqueue_pop(q->rx_vq, sizeof(VirtQueueElement));
        if (!elem) {
            if (i) {
                virtio_error(vdev, "virtio-net unexpected empty queue: "
                             "i %zd mergeable %d offset %zd, size %zd, "
                             "guest hdr len %zd, host hdr len %zd "
                             "guest features 0x%" PRIx64,
                             i, n->mergeable_rx_bufs, offset, size,
                             n->guest_hdr_len, n->host_hdr_len,
                             vdev->guest_features);
            }
            err = -1;
            goto err;
        }

        if (elem->in_num < 1) {
            virtio_error(vdev,
                         "virtio-net receive queue contains no in buffers");
            virtqueue_detach_element(q->rx_vq, elem, 0);
            g_free(elem);
            err = -1;
            goto err;
        }

        sg = elem->in_sg;
        if (i == 0) {
            assert(offset == 0);
            if (n->mergeable_rx_bufs) {
                mhdr_cnt = iov_copy(mhdr_sg, ARRAY_SIZE(mhdr_sg),
                                    sg, elem->in_num,
                                    offsetof(typeof(mhdr), num_buffers),
                                    sizeof(mhdr.num_buffers));
            }

            receive_header(n, sg, elem->in_num, buf, size);
            if (n->rss_data.populate_hash) {
                offset = sizeof(mhdr);
                iov_from_buf(sg, elem->in_num, offset,
                             buf + offset, n->host_hdr_len - sizeof(mhdr));
            }
            offset = n->host_hdr_len;
            total += n->guest_hdr_len;
            guest_offset = n->guest_hdr_len;
        } else {
            guest_offset = 0;
        }

        /* copy in packet.  ugh */
        len = iov_from_buf(sg, elem->in_num, guest_offset,
                           buf + offset, size - offset);
        total += len;
        offset += len;
        /* If buffers can't be merged, at this point we
         * must have consumed the complete packet.
         * Otherwise, drop it. */
        if (!n->mergeable_rx_bufs && offset < size) {
            virtqueue_unpop(q->rx_vq, elem, total);
            g_free(elem);
            err = size;
            goto err;
        }

        elems[i] = elem;
        lens[i] = total;
        i++;
    }

    if (mhdr_cnt) {
        virtio_stw_p(vdev, &mhdr.num_buffers, i);
        iov_from_buf(mhdr_sg, mhdr_cnt,
                     0,
                     &mhdr.num_buffers, sizeof mhdr.num_buffers);
    }

    for (j = 0; j < i; j++) {
        /* signal other side */
        virtqueue_fill(q->rx_vq, elems[j], lens[j], j);
        g_free(elems[j]);
    }

    virtqueue_flush(q->rx_vq, i);
    virtio_notify(vdev, q->rx_vq);

    return size;

err:
    for (j = 0; j < i; j++) {
        g_free(elems[j]);
    }

    return err;
}