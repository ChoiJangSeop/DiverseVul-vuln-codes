mlx5_rx_burst_mprq(void *dpdk_rxq, struct rte_mbuf **pkts, uint16_t pkts_n)
{
	struct mlx5_rxq_data *rxq = dpdk_rxq;
	const unsigned int strd_n = 1 << rxq->log_strd_num;
	const unsigned int strd_sz = 1 << rxq->log_strd_sz;
	const unsigned int strd_shift =
		MLX5_MPRQ_STRIDE_SHIFT_BYTE * rxq->strd_shift_en;
	const unsigned int cq_mask = (1 << rxq->cqe_n) - 1;
	const unsigned int wq_mask = (1 << rxq->elts_n) - 1;
	volatile struct mlx5_cqe *cqe = &(*rxq->cqes)[rxq->cq_ci & cq_mask];
	unsigned int i = 0;
	uint32_t rq_ci = rxq->rq_ci;
	uint16_t consumed_strd = rxq->consumed_strd;
	struct mlx5_mprq_buf *buf = (*rxq->mprq_bufs)[rq_ci & wq_mask];

	while (i < pkts_n) {
		struct rte_mbuf *pkt;
		void *addr;
		int ret;
		uint32_t len;
		uint16_t strd_cnt;
		uint16_t strd_idx;
		uint32_t offset;
		uint32_t byte_cnt;
		int32_t hdrm_overlap;
		volatile struct mlx5_mini_cqe8 *mcqe = NULL;
		uint32_t rss_hash_res = 0;

		if (consumed_strd == strd_n) {
			/* Replace WQE only if the buffer is still in use. */
			if (rte_atomic16_read(&buf->refcnt) > 1) {
				mprq_buf_replace(rxq, rq_ci & wq_mask, strd_n);
				/* Release the old buffer. */
				mlx5_mprq_buf_free(buf);
			} else if (unlikely(rxq->mprq_repl == NULL)) {
				struct mlx5_mprq_buf *rep;

				/*
				 * Currently, the MPRQ mempool is out of buffer
				 * and doing memcpy regardless of the size of Rx
				 * packet. Retry allocation to get back to
				 * normal.
				 */
				if (!rte_mempool_get(rxq->mprq_mp,
						     (void **)&rep))
					rxq->mprq_repl = rep;
			}
			/* Advance to the next WQE. */
			consumed_strd = 0;
			++rq_ci;
			buf = (*rxq->mprq_bufs)[rq_ci & wq_mask];
		}
		cqe = &(*rxq->cqes)[rxq->cq_ci & cq_mask];
		ret = mlx5_rx_poll_len(rxq, cqe, cq_mask, &mcqe);
		if (!ret)
			break;
		byte_cnt = ret;
		strd_cnt = (byte_cnt & MLX5_MPRQ_STRIDE_NUM_MASK) >>
			   MLX5_MPRQ_STRIDE_NUM_SHIFT;
		assert(strd_cnt);
		consumed_strd += strd_cnt;
		if (byte_cnt & MLX5_MPRQ_FILLER_MASK)
			continue;
		if (mcqe == NULL) {
			rss_hash_res = rte_be_to_cpu_32(cqe->rx_hash_res);
			strd_idx = rte_be_to_cpu_16(cqe->wqe_counter);
		} else {
			/* mini-CQE for MPRQ doesn't have hash result. */
			strd_idx = rte_be_to_cpu_16(mcqe->stride_idx);
		}
		assert(strd_idx < strd_n);
		assert(!((rte_be_to_cpu_16(cqe->wqe_id) ^ rq_ci) & wq_mask));
		pkt = rte_pktmbuf_alloc(rxq->mp);
		if (unlikely(pkt == NULL)) {
			++rxq->stats.rx_nombuf;
			break;
		}
		len = (byte_cnt & MLX5_MPRQ_LEN_MASK) >> MLX5_MPRQ_LEN_SHIFT;
		assert((int)len >= (rxq->crc_present << 2));
		if (rxq->crc_present)
			len -= RTE_ETHER_CRC_LEN;
		offset = strd_idx * strd_sz + strd_shift;
		addr = RTE_PTR_ADD(mlx5_mprq_buf_addr(buf, strd_n), offset);
		hdrm_overlap = len + RTE_PKTMBUF_HEADROOM - strd_cnt * strd_sz;
		/*
		 * Memcpy packets to the target mbuf if:
		 * - The size of packet is smaller than mprq_max_memcpy_len.
		 * - Out of buffer in the Mempool for Multi-Packet RQ.
		 * - The packet's stride overlaps a headroom and scatter is off.
		 */
		if (len <= rxq->mprq_max_memcpy_len ||
		    rxq->mprq_repl == NULL ||
		    (hdrm_overlap > 0 && !rxq->strd_scatter_en)) {
			if (likely(rte_pktmbuf_tailroom(pkt) >= len)) {
				rte_memcpy(rte_pktmbuf_mtod(pkt, void *),
					   addr, len);
				DATA_LEN(pkt) = len;
			} else if (rxq->strd_scatter_en) {
				struct rte_mbuf *prev = pkt;
				uint32_t seg_len =
					RTE_MIN(rte_pktmbuf_tailroom(pkt), len);
				uint32_t rem_len = len - seg_len;

				rte_memcpy(rte_pktmbuf_mtod(pkt, void *),
					   addr, seg_len);
				DATA_LEN(pkt) = seg_len;
				while (rem_len) {
					struct rte_mbuf *next =
						rte_pktmbuf_alloc(rxq->mp);

					if (unlikely(next == NULL)) {
						rte_pktmbuf_free(pkt);
						++rxq->stats.rx_nombuf;
						goto out;
					}
					NEXT(prev) = next;
					SET_DATA_OFF(next, 0);
					addr = RTE_PTR_ADD(addr, seg_len);
					seg_len = RTE_MIN
						(rte_pktmbuf_tailroom(next),
						 rem_len);
					rte_memcpy
						(rte_pktmbuf_mtod(next, void *),
						 addr, seg_len);
					DATA_LEN(next) = seg_len;
					rem_len -= seg_len;
					prev = next;
					++NB_SEGS(pkt);
				}
			} else {
				rte_pktmbuf_free_seg(pkt);
				++rxq->stats.idropped;
				continue;
			}
		} else {
			rte_iova_t buf_iova;
			struct rte_mbuf_ext_shared_info *shinfo;
			uint16_t buf_len = strd_cnt * strd_sz;
			void *buf_addr;

			/* Increment the refcnt of the whole chunk. */
			rte_atomic16_add_return(&buf->refcnt, 1);
			assert((uint16_t)rte_atomic16_read(&buf->refcnt) <=
			       strd_n + 1);
			buf_addr = RTE_PTR_SUB(addr, RTE_PKTMBUF_HEADROOM);
			/*
			 * MLX5 device doesn't use iova but it is necessary in a
			 * case where the Rx packet is transmitted via a
			 * different PMD.
			 */
			buf_iova = rte_mempool_virt2iova(buf) +
				   RTE_PTR_DIFF(buf_addr, buf);
			shinfo = &buf->shinfos[strd_idx];
			rte_mbuf_ext_refcnt_set(shinfo, 1);
			/*
			 * EXT_ATTACHED_MBUF will be set to pkt->ol_flags when
			 * attaching the stride to mbuf and more offload flags
			 * will be added below by calling rxq_cq_to_mbuf().
			 * Other fields will be overwritten.
			 */
			rte_pktmbuf_attach_extbuf(pkt, buf_addr, buf_iova,
						  buf_len, shinfo);
			/* Set mbuf head-room. */
			SET_DATA_OFF(pkt, RTE_PKTMBUF_HEADROOM);
			assert(pkt->ol_flags == EXT_ATTACHED_MBUF);
			assert(rte_pktmbuf_tailroom(pkt) >=
				len - (hdrm_overlap > 0 ? hdrm_overlap : 0));
			DATA_LEN(pkt) = len;
			/*
			 * Copy the last fragment of a packet (up to headroom
			 * size bytes) in case there is a stride overlap with
			 * a next packet's headroom. Allocate a separate mbuf
			 * to store this fragment and link it. Scatter is on.
			 */
			if (hdrm_overlap > 0) {
				assert(rxq->strd_scatter_en);
				struct rte_mbuf *seg =
					rte_pktmbuf_alloc(rxq->mp);

				if (unlikely(seg == NULL)) {
					rte_pktmbuf_free_seg(pkt);
					++rxq->stats.rx_nombuf;
					break;
				}
				SET_DATA_OFF(seg, 0);
				rte_memcpy(rte_pktmbuf_mtod(seg, void *),
					RTE_PTR_ADD(addr, len - hdrm_overlap),
					hdrm_overlap);
				DATA_LEN(seg) = hdrm_overlap;
				DATA_LEN(pkt) = len - hdrm_overlap;
				NEXT(pkt) = seg;
				NB_SEGS(pkt) = 2;
			}
		}
		rxq_cq_to_mbuf(rxq, pkt, cqe, rss_hash_res);
		if (cqe->lro_num_seg > 1) {
			mlx5_lro_update_hdr(addr, cqe, len);
			pkt->ol_flags |= PKT_RX_LRO;
			pkt->tso_segsz = len / cqe->lro_num_seg;
		}
		PKT_LEN(pkt) = len;
		PORT(pkt) = rxq->port_id;
#ifdef MLX5_PMD_SOFT_COUNTERS
		/* Increment bytes counter. */
		rxq->stats.ibytes += PKT_LEN(pkt);
#endif
		/* Return packet. */
		*(pkts++) = pkt;
		++i;
	}
out:
	/* Update the consumer indexes. */
	rxq->consumed_strd = consumed_strd;
	rte_cio_wmb();
	*rxq->cq_db = rte_cpu_to_be_32(rxq->cq_ci);
	if (rq_ci != rxq->rq_ci) {
		rxq->rq_ci = rq_ci;
		rte_cio_wmb();
		*rxq->rq_db = rte_cpu_to_be_32(rxq->rq_ci);
	}
#ifdef MLX5_PMD_SOFT_COUNTERS
	/* Increment packets counter. */
	rxq->stats.ipackets += i;
#endif
	return i;
}