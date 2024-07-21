static int create_qp_common(struct mlx5_ib_dev *dev, struct ib_pd *pd,
			    struct ib_qp_init_attr *init_attr,
			    struct ib_udata *udata, struct mlx5_ib_qp *qp)
{
	struct mlx5_ib_resources *devr = &dev->devr;
	int inlen = MLX5_ST_SZ_BYTES(create_qp_in);
	struct mlx5_core_dev *mdev = dev->mdev;
	struct mlx5_ib_create_qp_resp resp;
	struct mlx5_ib_cq *send_cq;
	struct mlx5_ib_cq *recv_cq;
	unsigned long flags;
	u32 uidx = MLX5_IB_DEFAULT_UIDX;
	struct mlx5_ib_create_qp ucmd;
	struct mlx5_ib_qp_base *base;
	int mlx5_st;
	void *qpc;
	u32 *in;
	int err;

	mutex_init(&qp->mutex);
	spin_lock_init(&qp->sq.lock);
	spin_lock_init(&qp->rq.lock);

	mlx5_st = to_mlx5_st(init_attr->qp_type);
	if (mlx5_st < 0)
		return -EINVAL;

	if (init_attr->rwq_ind_tbl) {
		if (!udata)
			return -ENOSYS;

		err = create_rss_raw_qp_tir(dev, qp, pd, init_attr, udata);
		return err;
	}

	if (init_attr->create_flags & IB_QP_CREATE_BLOCK_MULTICAST_LOOPBACK) {
		if (!MLX5_CAP_GEN(mdev, block_lb_mc)) {
			mlx5_ib_dbg(dev, "block multicast loopback isn't supported\n");
			return -EINVAL;
		} else {
			qp->flags |= MLX5_IB_QP_BLOCK_MULTICAST_LOOPBACK;
		}
	}

	if (init_attr->create_flags &
			(IB_QP_CREATE_CROSS_CHANNEL |
			 IB_QP_CREATE_MANAGED_SEND |
			 IB_QP_CREATE_MANAGED_RECV)) {
		if (!MLX5_CAP_GEN(mdev, cd)) {
			mlx5_ib_dbg(dev, "cross-channel isn't supported\n");
			return -EINVAL;
		}
		if (init_attr->create_flags & IB_QP_CREATE_CROSS_CHANNEL)
			qp->flags |= MLX5_IB_QP_CROSS_CHANNEL;
		if (init_attr->create_flags & IB_QP_CREATE_MANAGED_SEND)
			qp->flags |= MLX5_IB_QP_MANAGED_SEND;
		if (init_attr->create_flags & IB_QP_CREATE_MANAGED_RECV)
			qp->flags |= MLX5_IB_QP_MANAGED_RECV;
	}

	if (init_attr->qp_type == IB_QPT_UD &&
	    (init_attr->create_flags & IB_QP_CREATE_IPOIB_UD_LSO))
		if (!MLX5_CAP_GEN(mdev, ipoib_basic_offloads)) {
			mlx5_ib_dbg(dev, "ipoib UD lso qp isn't supported\n");
			return -EOPNOTSUPP;
		}

	if (init_attr->create_flags & IB_QP_CREATE_SCATTER_FCS) {
		if (init_attr->qp_type != IB_QPT_RAW_PACKET) {
			mlx5_ib_dbg(dev, "Scatter FCS is supported only for Raw Packet QPs");
			return -EOPNOTSUPP;
		}
		if (!MLX5_CAP_GEN(dev->mdev, eth_net_offloads) ||
		    !MLX5_CAP_ETH(dev->mdev, scatter_fcs)) {
			mlx5_ib_dbg(dev, "Scatter FCS isn't supported\n");
			return -EOPNOTSUPP;
		}
		qp->flags |= MLX5_IB_QP_CAP_SCATTER_FCS;
	}

	if (init_attr->sq_sig_type == IB_SIGNAL_ALL_WR)
		qp->sq_signal_bits = MLX5_WQE_CTRL_CQ_UPDATE;

	if (init_attr->create_flags & IB_QP_CREATE_CVLAN_STRIPPING) {
		if (!(MLX5_CAP_GEN(dev->mdev, eth_net_offloads) &&
		      MLX5_CAP_ETH(dev->mdev, vlan_cap)) ||
		    (init_attr->qp_type != IB_QPT_RAW_PACKET))
			return -EOPNOTSUPP;
		qp->flags |= MLX5_IB_QP_CVLAN_STRIPPING;
	}

	if (pd && pd->uobject) {
		if (ib_copy_from_udata(&ucmd, udata, sizeof(ucmd))) {
			mlx5_ib_dbg(dev, "copy failed\n");
			return -EFAULT;
		}

		err = get_qp_user_index(to_mucontext(pd->uobject->context),
					&ucmd, udata->inlen, &uidx);
		if (err)
			return err;

		qp->wq_sig = !!(ucmd.flags & MLX5_QP_FLAG_SIGNATURE);
		qp->scat_cqe = !!(ucmd.flags & MLX5_QP_FLAG_SCATTER_CQE);
		if (ucmd.flags & MLX5_QP_FLAG_TUNNEL_OFFLOADS) {
			if (init_attr->qp_type != IB_QPT_RAW_PACKET ||
			    !tunnel_offload_supported(mdev)) {
				mlx5_ib_dbg(dev, "Tunnel offload isn't supported\n");
				return -EOPNOTSUPP;
			}
			qp->tunnel_offload_en = true;
		}

		if (init_attr->create_flags & IB_QP_CREATE_SOURCE_QPN) {
			if (init_attr->qp_type != IB_QPT_UD ||
			    (MLX5_CAP_GEN(dev->mdev, port_type) !=
			     MLX5_CAP_PORT_TYPE_IB) ||
			    !mlx5_get_flow_namespace(dev->mdev, MLX5_FLOW_NAMESPACE_BYPASS)) {
				mlx5_ib_dbg(dev, "Source QP option isn't supported\n");
				return -EOPNOTSUPP;
			}

			qp->flags |= MLX5_IB_QP_UNDERLAY;
			qp->underlay_qpn = init_attr->source_qpn;
		}
	} else {
		qp->wq_sig = !!wq_signature;
	}

	base = (init_attr->qp_type == IB_QPT_RAW_PACKET ||
		qp->flags & MLX5_IB_QP_UNDERLAY) ?
	       &qp->raw_packet_qp.rq.base :
	       &qp->trans_qp.base;

	qp->has_rq = qp_has_rq(init_attr);
	err = set_rq_size(dev, &init_attr->cap, qp->has_rq,
			  qp, (pd && pd->uobject) ? &ucmd : NULL);
	if (err) {
		mlx5_ib_dbg(dev, "err %d\n", err);
		return err;
	}

	if (pd) {
		if (pd->uobject) {
			__u32 max_wqes =
				1 << MLX5_CAP_GEN(mdev, log_max_qp_sz);
			mlx5_ib_dbg(dev, "requested sq_wqe_count (%d)\n", ucmd.sq_wqe_count);
			if (ucmd.rq_wqe_shift != qp->rq.wqe_shift ||
			    ucmd.rq_wqe_count != qp->rq.wqe_cnt) {
				mlx5_ib_dbg(dev, "invalid rq params\n");
				return -EINVAL;
			}
			if (ucmd.sq_wqe_count > max_wqes) {
				mlx5_ib_dbg(dev, "requested sq_wqe_count (%d) > max allowed (%d)\n",
					    ucmd.sq_wqe_count, max_wqes);
				return -EINVAL;
			}
			if (init_attr->create_flags &
			    mlx5_ib_create_qp_sqpn_qp1()) {
				mlx5_ib_dbg(dev, "user-space is not allowed to create UD QPs spoofing as QP1\n");
				return -EINVAL;
			}
			err = create_user_qp(dev, pd, qp, udata, init_attr, &in,
					     &resp, &inlen, base);
			if (err)
				mlx5_ib_dbg(dev, "err %d\n", err);
		} else {
			err = create_kernel_qp(dev, init_attr, qp, &in, &inlen,
					       base);
			if (err)
				mlx5_ib_dbg(dev, "err %d\n", err);
		}

		if (err)
			return err;
	} else {
		in = kvzalloc(inlen, GFP_KERNEL);
		if (!in)
			return -ENOMEM;

		qp->create_type = MLX5_QP_EMPTY;
	}

	if (is_sqp(init_attr->qp_type))
		qp->port = init_attr->port_num;

	qpc = MLX5_ADDR_OF(create_qp_in, in, qpc);

	MLX5_SET(qpc, qpc, st, mlx5_st);
	MLX5_SET(qpc, qpc, pm_state, MLX5_QP_PM_MIGRATED);

	if (init_attr->qp_type != MLX5_IB_QPT_REG_UMR)
		MLX5_SET(qpc, qpc, pd, to_mpd(pd ? pd : devr->p0)->pdn);
	else
		MLX5_SET(qpc, qpc, latency_sensitive, 1);


	if (qp->wq_sig)
		MLX5_SET(qpc, qpc, wq_signature, 1);

	if (qp->flags & MLX5_IB_QP_BLOCK_MULTICAST_LOOPBACK)
		MLX5_SET(qpc, qpc, block_lb_mc, 1);

	if (qp->flags & MLX5_IB_QP_CROSS_CHANNEL)
		MLX5_SET(qpc, qpc, cd_master, 1);
	if (qp->flags & MLX5_IB_QP_MANAGED_SEND)
		MLX5_SET(qpc, qpc, cd_slave_send, 1);
	if (qp->flags & MLX5_IB_QP_MANAGED_RECV)
		MLX5_SET(qpc, qpc, cd_slave_receive, 1);

	if (qp->scat_cqe && is_connected(init_attr->qp_type)) {
		int rcqe_sz;
		int scqe_sz;

		rcqe_sz = mlx5_ib_get_cqe_size(dev, init_attr->recv_cq);
		scqe_sz = mlx5_ib_get_cqe_size(dev, init_attr->send_cq);

		if (rcqe_sz == 128)
			MLX5_SET(qpc, qpc, cs_res, MLX5_RES_SCAT_DATA64_CQE);
		else
			MLX5_SET(qpc, qpc, cs_res, MLX5_RES_SCAT_DATA32_CQE);

		if (init_attr->sq_sig_type == IB_SIGNAL_ALL_WR) {
			if (scqe_sz == 128)
				MLX5_SET(qpc, qpc, cs_req, MLX5_REQ_SCAT_DATA64_CQE);
			else
				MLX5_SET(qpc, qpc, cs_req, MLX5_REQ_SCAT_DATA32_CQE);
		}
	}

	if (qp->rq.wqe_cnt) {
		MLX5_SET(qpc, qpc, log_rq_stride, qp->rq.wqe_shift - 4);
		MLX5_SET(qpc, qpc, log_rq_size, ilog2(qp->rq.wqe_cnt));
	}

	MLX5_SET(qpc, qpc, rq_type, get_rx_type(qp, init_attr));

	if (qp->sq.wqe_cnt) {
		MLX5_SET(qpc, qpc, log_sq_size, ilog2(qp->sq.wqe_cnt));
	} else {
		MLX5_SET(qpc, qpc, no_sq, 1);
		if (init_attr->srq &&
		    init_attr->srq->srq_type == IB_SRQT_TM)
			MLX5_SET(qpc, qpc, offload_type,
				 MLX5_QPC_OFFLOAD_TYPE_RNDV);
	}

	/* Set default resources */
	switch (init_attr->qp_type) {
	case IB_QPT_XRC_TGT:
		MLX5_SET(qpc, qpc, cqn_rcv, to_mcq(devr->c0)->mcq.cqn);
		MLX5_SET(qpc, qpc, cqn_snd, to_mcq(devr->c0)->mcq.cqn);
		MLX5_SET(qpc, qpc, srqn_rmpn_xrqn, to_msrq(devr->s0)->msrq.srqn);
		MLX5_SET(qpc, qpc, xrcd, to_mxrcd(init_attr->xrcd)->xrcdn);
		break;
	case IB_QPT_XRC_INI:
		MLX5_SET(qpc, qpc, cqn_rcv, to_mcq(devr->c0)->mcq.cqn);
		MLX5_SET(qpc, qpc, xrcd, to_mxrcd(devr->x1)->xrcdn);
		MLX5_SET(qpc, qpc, srqn_rmpn_xrqn, to_msrq(devr->s0)->msrq.srqn);
		break;
	default:
		if (init_attr->srq) {
			MLX5_SET(qpc, qpc, xrcd, to_mxrcd(devr->x0)->xrcdn);
			MLX5_SET(qpc, qpc, srqn_rmpn_xrqn, to_msrq(init_attr->srq)->msrq.srqn);
		} else {
			MLX5_SET(qpc, qpc, xrcd, to_mxrcd(devr->x1)->xrcdn);
			MLX5_SET(qpc, qpc, srqn_rmpn_xrqn, to_msrq(devr->s1)->msrq.srqn);
		}
	}

	if (init_attr->send_cq)
		MLX5_SET(qpc, qpc, cqn_snd, to_mcq(init_attr->send_cq)->mcq.cqn);

	if (init_attr->recv_cq)
		MLX5_SET(qpc, qpc, cqn_rcv, to_mcq(init_attr->recv_cq)->mcq.cqn);

	MLX5_SET64(qpc, qpc, dbr_addr, qp->db.dma);

	/* 0xffffff means we ask to work with cqe version 0 */
	if (MLX5_CAP_GEN(mdev, cqe_version) == MLX5_CQE_VERSION_V1)
		MLX5_SET(qpc, qpc, user_index, uidx);

	/* we use IB_QP_CREATE_IPOIB_UD_LSO to indicates ipoib qp */
	if (init_attr->qp_type == IB_QPT_UD &&
	    (init_attr->create_flags & IB_QP_CREATE_IPOIB_UD_LSO)) {
		MLX5_SET(qpc, qpc, ulp_stateless_offload_mode, 1);
		qp->flags |= MLX5_IB_QP_LSO;
	}

	if (init_attr->create_flags & IB_QP_CREATE_PCI_WRITE_END_PADDING) {
		if (!MLX5_CAP_GEN(dev->mdev, end_pad)) {
			mlx5_ib_dbg(dev, "scatter end padding is not supported\n");
			err = -EOPNOTSUPP;
			goto err;
		} else if (init_attr->qp_type != IB_QPT_RAW_PACKET) {
			MLX5_SET(qpc, qpc, end_padding_mode,
				 MLX5_WQ_END_PAD_MODE_ALIGN);
		} else {
			qp->flags |= MLX5_IB_QP_PCI_WRITE_END_PADDING;
		}
	}

	if (inlen < 0) {
		err = -EINVAL;
		goto err;
	}

	if (init_attr->qp_type == IB_QPT_RAW_PACKET ||
	    qp->flags & MLX5_IB_QP_UNDERLAY) {
		qp->raw_packet_qp.sq.ubuffer.buf_addr = ucmd.sq_buf_addr;
		raw_packet_qp_copy_info(qp, &qp->raw_packet_qp);
		err = create_raw_packet_qp(dev, qp, in, inlen, pd);
	} else {
		err = mlx5_core_create_qp(dev->mdev, &base->mqp, in, inlen);
	}

	if (err) {
		mlx5_ib_dbg(dev, "create qp failed\n");
		goto err_create;
	}

	kvfree(in);

	base->container_mibqp = qp;
	base->mqp.event = mlx5_ib_qp_event;

	get_cqs(init_attr->qp_type, init_attr->send_cq, init_attr->recv_cq,
		&send_cq, &recv_cq);
	spin_lock_irqsave(&dev->reset_flow_resource_lock, flags);
	mlx5_ib_lock_cqs(send_cq, recv_cq);
	/* Maintain device to QPs access, needed for further handling via reset
	 * flow
	 */
	list_add_tail(&qp->qps_list, &dev->qp_list);
	/* Maintain CQ to QPs access, needed for further handling via reset flow
	 */
	if (send_cq)
		list_add_tail(&qp->cq_send_list, &send_cq->list_send_qp);
	if (recv_cq)
		list_add_tail(&qp->cq_recv_list, &recv_cq->list_recv_qp);
	mlx5_ib_unlock_cqs(send_cq, recv_cq);
	spin_unlock_irqrestore(&dev->reset_flow_resource_lock, flags);

	return 0;

err_create:
	if (qp->create_type == MLX5_QP_USER)
		destroy_qp_user(dev, pd, qp, base);
	else if (qp->create_type == MLX5_QP_KERNEL)
		destroy_qp_kernel(dev, qp);

err:
	kvfree(in);
	return err;
}