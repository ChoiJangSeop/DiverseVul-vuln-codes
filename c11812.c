mlx5_rx_burst_mprq(void *dpdk_rxq, struct rte_mbuf **pkts, uint16_t pkts_n)
{
	struct mlx5_rxq_data *rxq = dpdk_rxq;
	const uint32_t strd_n = RTE_BIT32(rxq->log_strd_num);
	const uint32_t strd_sz = RTE_BIT32(rxq->log_strd_sz);
	const uint32_t cq_mask = (1 << rxq->cqe_n) - 1;
	const uint32_t wq_mask = (1 << rxq->elts_n) - 1;
	volatile struct mlx5_cqe *cqe = &(*rxq->cqes)[rxq->cq_ci & cq_mask];
	unsigned int i = 0;
	uint32_t rq_ci = rxq->rq_ci;
	uint16_t consumed_strd = rxq->consumed_strd;
	struct mlx5_mprq_buf *buf = (*rxq->mprq_bufs)[rq_ci & wq_mask];

	while (i < pkts_n) {
		struct rte_mbuf *pkt;
		int ret;
		uint32_t len;
		uint16_t strd_cnt;
		uint16_t strd_idx;
		uint32_t byte_cnt;
		volatile struct mlx5_mini_cqe8 *mcqe = NULL;
		enum mlx5_rqx_code rxq_code;

		if (consumed_strd == strd_n) {
			/* Replace WQE if the buffer is still in use. */
			mprq_buf_replace(rxq, rq_ci & wq_mask);
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
		len = (byte_cnt & MLX5_MPRQ_LEN_MASK) >> MLX5_MPRQ_LEN_SHIFT;
		MLX5_ASSERT((int)len >= (rxq->crc_present << 2));
		if (rxq->crc_present)
			len -= RTE_ETHER_CRC_LEN;
		if (mcqe &&
		    rxq->mcqe_format == MLX5_CQE_RESP_FORMAT_FTAG_STRIDX)
			strd_cnt = (len / strd_sz) + !!(len % strd_sz);
		else
			strd_cnt = (byte_cnt & MLX5_MPRQ_STRIDE_NUM_MASK) >>
				   MLX5_MPRQ_STRIDE_NUM_SHIFT;
		MLX5_ASSERT(strd_cnt);
		consumed_strd += strd_cnt;
		if (byte_cnt & MLX5_MPRQ_FILLER_MASK)
			continue;
		strd_idx = rte_be_to_cpu_16(mcqe == NULL ?
					cqe->wqe_counter :
					mcqe->stride_idx);
		MLX5_ASSERT(strd_idx < strd_n);
		MLX5_ASSERT(!((rte_be_to_cpu_16(cqe->wqe_id) ^ rq_ci) &
			    wq_mask));
		pkt = rte_pktmbuf_alloc(rxq->mp);
		if (unlikely(pkt == NULL)) {
			++rxq->stats.rx_nombuf;
			break;
		}
		len = (byte_cnt & MLX5_MPRQ_LEN_MASK) >> MLX5_MPRQ_LEN_SHIFT;
		MLX5_ASSERT((int)len >= (rxq->crc_present << 2));
		if (rxq->crc_present)
			len -= RTE_ETHER_CRC_LEN;
		rxq_code = mprq_buf_to_pkt(rxq, pkt, len, buf,
					   strd_idx, strd_cnt);
		if (unlikely(rxq_code != MLX5_RXQ_CODE_EXIT)) {
			rte_pktmbuf_free_seg(pkt);
			if (rxq_code == MLX5_RXQ_CODE_DROPPED) {
				++rxq->stats.idropped;
				continue;
			}
			if (rxq_code == MLX5_RXQ_CODE_NOMBUF) {
				++rxq->stats.rx_nombuf;
				break;
			}
		}
		rxq_cq_to_mbuf(rxq, pkt, cqe, mcqe);
		if (cqe->lro_num_seg > 1) {
			mlx5_lro_update_hdr(rte_pktmbuf_mtod(pkt, uint8_t *),
					    cqe, mcqe, rxq, len);
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
	/* Update the consumer indexes. */
	rxq->consumed_strd = consumed_strd;
	rte_io_wmb();
	*rxq->cq_db = rte_cpu_to_be_32(rxq->cq_ci);
	if (rq_ci != rxq->rq_ci) {
		rxq->rq_ci = rq_ci;
		rte_io_wmb();
		*rxq->rq_db = rte_cpu_to_be_32(rxq->rq_ci);
	}
#ifdef MLX5_PMD_SOFT_COUNTERS
	/* Increment packets counter. */
	rxq->stats.ipackets += i;
#endif
	return i;
}