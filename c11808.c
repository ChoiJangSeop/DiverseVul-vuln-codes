mlx5_rx_err_handle(struct mlx5_rxq_data *rxq, uint8_t vec)
{
	const uint16_t cqe_n = 1 << rxq->cqe_n;
	const uint16_t cqe_mask = cqe_n - 1;
	const uint16_t wqe_n = 1 << rxq->elts_n;
	const uint16_t strd_n = RTE_BIT32(rxq->log_strd_num);
	struct mlx5_rxq_ctrl *rxq_ctrl =
			container_of(rxq, struct mlx5_rxq_ctrl, rxq);
	union {
		volatile struct mlx5_cqe *cqe;
		volatile struct mlx5_err_cqe *err_cqe;
	} u = {
		.cqe = &(*rxq->cqes)[rxq->cq_ci & cqe_mask],
	};
	struct mlx5_mp_arg_queue_state_modify sm;
	int ret;

	switch (rxq->err_state) {
	case MLX5_RXQ_ERR_STATE_NO_ERROR:
		rxq->err_state = MLX5_RXQ_ERR_STATE_NEED_RESET;
		/* Fall-through */
	case MLX5_RXQ_ERR_STATE_NEED_RESET:
		sm.is_wq = 1;
		sm.queue_id = rxq->idx;
		sm.state = IBV_WQS_RESET;
		if (mlx5_queue_state_modify(RXQ_DEV(rxq_ctrl), &sm))
			return -1;
		if (rxq_ctrl->dump_file_n <
		    RXQ_PORT(rxq_ctrl)->config.max_dump_files_num) {
			MKSTR(err_str, "Unexpected CQE error syndrome "
			      "0x%02x CQN = %u RQN = %u wqe_counter = %u"
			      " rq_ci = %u cq_ci = %u", u.err_cqe->syndrome,
			      rxq->cqn, rxq_ctrl->wqn,
			      rte_be_to_cpu_16(u.err_cqe->wqe_counter),
			      rxq->rq_ci << rxq->sges_n, rxq->cq_ci);
			MKSTR(name, "dpdk_mlx5_port_%u_rxq_%u_%u",
			      rxq->port_id, rxq->idx, (uint32_t)rte_rdtsc());
			mlx5_dump_debug_information(name, NULL, err_str, 0);
			mlx5_dump_debug_information(name, "MLX5 Error CQ:",
						    (const void *)((uintptr_t)
								    rxq->cqes),
						    sizeof(*u.cqe) * cqe_n);
			mlx5_dump_debug_information(name, "MLX5 Error RQ:",
						    (const void *)((uintptr_t)
								    rxq->wqes),
						    16 * wqe_n);
			rxq_ctrl->dump_file_n++;
		}
		rxq->err_state = MLX5_RXQ_ERR_STATE_NEED_READY;
		/* Fall-through */
	case MLX5_RXQ_ERR_STATE_NEED_READY:
		ret = check_cqe(u.cqe, cqe_n, rxq->cq_ci);
		if (ret == MLX5_CQE_STATUS_HW_OWN) {
			rte_io_wmb();
			*rxq->cq_db = rte_cpu_to_be_32(rxq->cq_ci);
			rte_io_wmb();
			/*
			 * The RQ consumer index must be zeroed while moving
			 * from RESET state to RDY state.
			 */
			*rxq->rq_db = rte_cpu_to_be_32(0);
			rte_io_wmb();
			sm.is_wq = 1;
			sm.queue_id = rxq->idx;
			sm.state = IBV_WQS_RDY;
			if (mlx5_queue_state_modify(RXQ_DEV(rxq_ctrl), &sm))
				return -1;
			if (vec) {
				const uint32_t elts_n =
					mlx5_rxq_mprq_enabled(rxq) ?
					wqe_n * strd_n : wqe_n;
				const uint32_t e_mask = elts_n - 1;
				uint32_t elts_ci =
					mlx5_rxq_mprq_enabled(rxq) ?
					rxq->elts_ci : rxq->rq_ci;
				uint32_t elt_idx;
				struct rte_mbuf **elt;
				int i;
				unsigned int n = elts_n - (elts_ci -
							  rxq->rq_pi);

				for (i = 0; i < (int)n; ++i) {
					elt_idx = (elts_ci + i) & e_mask;
					elt = &(*rxq->elts)[elt_idx];
					*elt = rte_mbuf_raw_alloc(rxq->mp);
					if (!*elt) {
						for (i--; i >= 0; --i) {
							elt_idx = (elts_ci +
								   i) & elts_n;
							elt = &(*rxq->elts)
								[elt_idx];
							rte_pktmbuf_free_seg
								(*elt);
						}
						return -1;
					}
				}
				for (i = 0; i < (int)elts_n; ++i) {
					elt = &(*rxq->elts)[i];
					DATA_LEN(*elt) =
						(uint16_t)((*elt)->buf_len -
						rte_pktmbuf_headroom(*elt));
				}
				/* Padding with a fake mbuf for vec Rx. */
				for (i = 0; i < MLX5_VPMD_DESCS_PER_LOOP; ++i)
					(*rxq->elts)[elts_n + i] =
								&rxq->fake_mbuf;
			}
			mlx5_rxq_initialize(rxq);
			rxq->err_state = MLX5_RXQ_ERR_STATE_NO_ERROR;
		}
		return ret;
	default:
		return -1;
	}
}