mlx5_rx_burst(void *dpdk_rxq, struct rte_mbuf **pkts, uint16_t pkts_n)
{
	struct mlx5_rxq_data *rxq = dpdk_rxq;
	const unsigned int wqe_cnt = (1 << rxq->elts_n) - 1;
	const unsigned int cqe_cnt = (1 << rxq->cqe_n) - 1;
	const unsigned int sges_n = rxq->sges_n;
	struct rte_mbuf *pkt = NULL;
	struct rte_mbuf *seg = NULL;
	volatile struct mlx5_cqe *cqe =
		&(*rxq->cqes)[rxq->cq_ci & cqe_cnt];
	unsigned int i = 0;
	unsigned int rq_ci = rxq->rq_ci << sges_n;
	int len = 0; /* keep its value across iterations. */

	while (pkts_n) {
		unsigned int idx = rq_ci & wqe_cnt;
		volatile struct mlx5_wqe_data_seg *wqe =
			&((volatile struct mlx5_wqe_data_seg *)rxq->wqes)[idx];
		struct rte_mbuf *rep = (*rxq->elts)[idx];
		volatile struct mlx5_mini_cqe8 *mcqe = NULL;

		if (pkt)
			NEXT(seg) = rep;
		seg = rep;
		rte_prefetch0(seg);
		rte_prefetch0(cqe);
		rte_prefetch0(wqe);
		/* Allocate the buf from the same pool. */
		rep = rte_mbuf_raw_alloc(seg->pool);
		if (unlikely(rep == NULL)) {
			++rxq->stats.rx_nombuf;
			if (!pkt) {
				/*
				 * no buffers before we even started,
				 * bail out silently.
				 */
				break;
			}
			while (pkt != seg) {
				MLX5_ASSERT(pkt != (*rxq->elts)[idx]);
				rep = NEXT(pkt);
				NEXT(pkt) = NULL;
				NB_SEGS(pkt) = 1;
				rte_mbuf_raw_free(pkt);
				pkt = rep;
			}
			rq_ci >>= sges_n;
			++rq_ci;
			rq_ci <<= sges_n;
			break;
		}
		if (!pkt) {
			cqe = &(*rxq->cqes)[rxq->cq_ci & cqe_cnt];
			len = mlx5_rx_poll_len(rxq, cqe, cqe_cnt, &mcqe);
			if (!len) {
				rte_mbuf_raw_free(rep);
				break;
			}
			pkt = seg;
			MLX5_ASSERT(len >= (rxq->crc_present << 2));
			pkt->ol_flags &= EXT_ATTACHED_MBUF;
			rxq_cq_to_mbuf(rxq, pkt, cqe, mcqe);
			if (rxq->crc_present)
				len -= RTE_ETHER_CRC_LEN;
			PKT_LEN(pkt) = len;
			if (cqe->lro_num_seg > 1) {
				mlx5_lro_update_hdr
					(rte_pktmbuf_mtod(pkt, uint8_t *), cqe,
					 mcqe, rxq, len);
				pkt->ol_flags |= PKT_RX_LRO;
				pkt->tso_segsz = len / cqe->lro_num_seg;
			}
		}
		DATA_LEN(rep) = DATA_LEN(seg);
		PKT_LEN(rep) = PKT_LEN(seg);
		SET_DATA_OFF(rep, DATA_OFF(seg));
		PORT(rep) = PORT(seg);
		(*rxq->elts)[idx] = rep;
		/*
		 * Fill NIC descriptor with the new buffer.  The lkey and size
		 * of the buffers are already known, only the buffer address
		 * changes.
		 */
		wqe->addr = rte_cpu_to_be_64(rte_pktmbuf_mtod(rep, uintptr_t));
		/* If there's only one MR, no need to replace LKey in WQE. */
		if (unlikely(mlx5_mr_btree_len(&rxq->mr_ctrl.cache_bh) > 1))
			wqe->lkey = mlx5_rx_mb2mr(rxq, rep);
		if (len > DATA_LEN(seg)) {
			len -= DATA_LEN(seg);
			++NB_SEGS(pkt);
			++rq_ci;
			continue;
		}
		DATA_LEN(seg) = len;
#ifdef MLX5_PMD_SOFT_COUNTERS
		/* Increment bytes counter. */
		rxq->stats.ibytes += PKT_LEN(pkt);
#endif
		/* Return packet. */
		*(pkts++) = pkt;
		pkt = NULL;
		--pkts_n;
		++i;
		/* Align consumer index to the next stride. */
		rq_ci >>= sges_n;
		++rq_ci;
		rq_ci <<= sges_n;
	}
	if (unlikely((i == 0) && ((rq_ci >> sges_n) == rxq->rq_ci)))
		return 0;
	/* Update the consumer index. */
	rxq->rq_ci = rq_ci >> sges_n;
	rte_io_wmb();
	*rxq->cq_db = rte_cpu_to_be_32(rxq->cq_ci);
	rte_io_wmb();
	*rxq->rq_db = rte_cpu_to_be_32(rxq->rq_ci);
#ifdef MLX5_PMD_SOFT_COUNTERS
	/* Increment packets counter. */
	rxq->stats.ipackets += i;
#endif
	return i;
}