mlx5_rx_poll_len(struct mlx5_rxq_data *rxq, volatile struct mlx5_cqe *cqe,
		 uint16_t cqe_cnt, volatile struct mlx5_mini_cqe8 **mcqe)
{
	struct rxq_zip *zip = &rxq->zip;
	uint16_t cqe_n = cqe_cnt + 1;
	int len;
	uint16_t idx, end;

	do {
		len = 0;
		/* Process compressed data in the CQE and mini arrays. */
		if (zip->ai) {
			volatile struct mlx5_mini_cqe8 (*mc)[8] =
				(volatile struct mlx5_mini_cqe8 (*)[8])
				(uintptr_t)(&(*rxq->cqes)[zip->ca &
							  cqe_cnt].pkt_info);
			len = rte_be_to_cpu_32((*mc)[zip->ai & 7].byte_cnt &
					       rxq->byte_mask);
			*mcqe = &(*mc)[zip->ai & 7];
			if ((++zip->ai & 7) == 0) {
				/* Invalidate consumed CQEs */
				idx = zip->ca;
				end = zip->na;
				while (idx != end) {
					(*rxq->cqes)[idx & cqe_cnt].op_own =
						MLX5_CQE_INVALIDATE;
					++idx;
				}
				/*
				 * Increment consumer index to skip the number
				 * of CQEs consumed. Hardware leaves holes in
				 * the CQ ring for software use.
				 */
				zip->ca = zip->na;
				zip->na += 8;
			}
			if (unlikely(rxq->zip.ai == rxq->zip.cqe_cnt)) {
				/* Invalidate the rest */
				idx = zip->ca;
				end = zip->cq_ci;

				while (idx != end) {
					(*rxq->cqes)[idx & cqe_cnt].op_own =
						MLX5_CQE_INVALIDATE;
					++idx;
				}
				rxq->cq_ci = zip->cq_ci;
				zip->ai = 0;
			}
		/*
		 * No compressed data, get next CQE and verify if it is
		 * compressed.
		 */
		} else {
			int ret;
			int8_t op_own;
			uint32_t cq_ci;

			ret = check_cqe(cqe, cqe_n, rxq->cq_ci);
			if (unlikely(ret != MLX5_CQE_STATUS_SW_OWN)) {
				if (unlikely(ret == MLX5_CQE_STATUS_ERR ||
					     rxq->err_state)) {
					ret = mlx5_rx_err_handle(rxq, 0);
					if (ret == MLX5_CQE_STATUS_HW_OWN ||
					    ret == -1)
						return 0;
				} else {
					return 0;
				}
			}
			/*
			 * Introduce the local variable to have queue cq_ci
			 * index in queue structure always consistent with
			 * actual CQE boundary (not pointing to the middle
			 * of compressed CQE session).
			 */
			cq_ci = rxq->cq_ci + 1;
			op_own = cqe->op_own;
			if (MLX5_CQE_FORMAT(op_own) == MLX5_COMPRESSED) {
				volatile struct mlx5_mini_cqe8 (*mc)[8] =
					(volatile struct mlx5_mini_cqe8 (*)[8])
					(uintptr_t)(&(*rxq->cqes)
						[cq_ci & cqe_cnt].pkt_info);

				/* Fix endianness. */
				zip->cqe_cnt = rte_be_to_cpu_32(cqe->byte_cnt);
				/*
				 * Current mini array position is the one
				 * returned by check_cqe64().
				 *
				 * If completion comprises several mini arrays,
				 * as a special case the second one is located
				 * 7 CQEs after the initial CQE instead of 8
				 * for subsequent ones.
				 */
				zip->ca = cq_ci;
				zip->na = zip->ca + 7;
				/* Compute the next non compressed CQE. */
				zip->cq_ci = rxq->cq_ci + zip->cqe_cnt;
				/* Get packet size to return. */
				len = rte_be_to_cpu_32((*mc)[0].byte_cnt &
						       rxq->byte_mask);
				*mcqe = &(*mc)[0];
				zip->ai = 1;
				/* Prefetch all to be invalidated */
				idx = zip->ca;
				end = zip->cq_ci;
				while (idx != end) {
					rte_prefetch0(&(*rxq->cqes)[(idx) &
								    cqe_cnt]);
					++idx;
				}
			} else {
				rxq->cq_ci = cq_ci;
				len = rte_be_to_cpu_32(cqe->byte_cnt);
			}
		}
		if (unlikely(rxq->err_state)) {
			cqe = &(*rxq->cqes)[rxq->cq_ci & cqe_cnt];
			++rxq->stats.idropped;
		} else {
			return len;
		}
	} while (1);
}