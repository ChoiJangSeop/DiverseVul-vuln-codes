void compress_block(
	const astcenc_context& ctx,
	const astcenc_image& input_image,
	const image_block& blk,
	physical_compressed_block& pcb,
	compression_working_buffers& tmpbuf)
{
	astcenc_profile decode_mode = ctx.config.profile;
	symbolic_compressed_block scb;
	error_weight_block& ewb = tmpbuf.ewb;
	const block_size_descriptor* bsd = ctx.bsd;
	float lowest_correl;

	TRACE_NODE(node0, "block");
	trace_add_data("pos_x", blk.xpos);
	trace_add_data("pos_y", blk.ypos);
	trace_add_data("pos_z", blk.zpos);

	// Set stricter block targets for luminance data as we have more bits to play with
	bool block_is_l = blk.is_luminance();
	float block_is_l_scale = block_is_l ? 1.0f / 1.5f : 1.0f;

	// Set slightly stricter block targets for lumalpha data as we have more bits to play with
	bool block_is_la = blk.is_luminancealpha();
	float block_is_la_scale = block_is_la ? 1.0f / 1.05f : 1.0f;

	bool block_skip_two_plane = false;

	// Default max partition, but +1 if only have 1 or 2 active components
	int max_partitions = ctx.config.tune_partition_count_limit;
	if (block_is_l || block_is_la)
	{
		max_partitions = astc::min(max_partitions + 1, 4);
	}


#if defined(ASTCENC_DIAGNOSTICS)
	// Do this early in diagnostic builds so we can dump uniform metrics
	// for every block. Do it later in release builds to avoid redundant work!
	float error_weight_sum = prepare_error_weight_block(ctx, input_image, *bsd, blk, ewb);
	float error_threshold = ctx.config.tune_db_limit
	                      * error_weight_sum
	                      * block_is_l_scale
	                      * block_is_la_scale;

	lowest_correl = prepare_block_statistics(bsd->texel_count, blk, ewb);
	trace_add_data("lowest_correl", lowest_correl);
	trace_add_data("tune_error_threshold", error_threshold);
#endif

	// Detected a constant-color block
	if (all(blk.data_min == blk.data_max))
	{
		TRACE_NODE(node1, "pass");
		trace_add_data("partition_count", 0);
		trace_add_data("plane_count", 1);

		scb.partition_count = 0;

		// Encode as FP16 if using HDR
		if ((decode_mode == ASTCENC_PRF_HDR) ||
		    (decode_mode == ASTCENC_PRF_HDR_RGB_LDR_A))
		{
			scb.block_type = SYM_BTYPE_CONST_F16;
			vint4 color_f16 = float_to_float16(blk.origin_texel);
			store(color_f16, scb.constant_color);
		}
		// Encode as UNORM16 if NOT using HDR
		else
		{
			scb.block_type = SYM_BTYPE_CONST_U16;
			vfloat4 color_f32 = clamp(0.0f, 1.0f, blk.origin_texel) * 65535.0f;
			vint4 color_u16 = float_to_int_rtn(color_f32);
			store(color_u16, scb.constant_color);
		}

		trace_add_data("exit", "quality hit");

		symbolic_to_physical(*bsd, scb, pcb);
		return;
	}

#if !defined(ASTCENC_DIAGNOSTICS)
	float error_weight_sum = prepare_error_weight_block(ctx, input_image, *bsd, blk, ewb);
	float error_threshold = ctx.config.tune_db_limit
	                      * error_weight_sum
	                      * block_is_l_scale
	                      * block_is_la_scale;
#endif

	// Set SCB and mode errors to a very high error value
	scb.errorval = ERROR_CALC_DEFAULT;
	scb.block_type = SYM_BTYPE_ERROR;

	float best_errorvals_for_pcount[BLOCK_MAX_PARTITIONS] {
		ERROR_CALC_DEFAULT, ERROR_CALC_DEFAULT, ERROR_CALC_DEFAULT, ERROR_CALC_DEFAULT
	};

	float exit_thresholds_for_pcount[BLOCK_MAX_PARTITIONS] {
		0.0f,
		ctx.config.tune_2_partition_early_out_limit_factor,
		ctx.config.tune_3_partition_early_out_limit_factor,
		0.0f
	};

	// Trial using 1 plane of weights and 1 partition.

	// Most of the time we test it twice, first with a mode cutoff of 0 and then with the specified
	// mode cutoff. This causes an early-out that speeds up encoding of easy blocks. However, this
	// optimization is disabled for 4x4 and 5x4 blocks where it nearly always slows down the
	// compression and slightly reduces image quality.

	float errorval_mult[2] {
		1.0f / ctx.config.tune_mode0_mse_overshoot,
		1.0f
	};

	static const float errorval_overshoot = 1.0f / ctx.config.tune_refinement_mse_overshoot;

	// Only enable MODE0 fast path (trial 0) if 2D and more than 25 texels
	int start_trial = 1;
	if ((bsd->texel_count >= TUNE_MIN_TEXELS_MODE0_FASTPATH) && (bsd->zdim == 1))
	{
		start_trial = 0;
	}

	for (int i = start_trial; i < 2; i++)
	{
		TRACE_NODE(node1, "pass");
		trace_add_data("partition_count", 1);
		trace_add_data("plane_count", 1);
		trace_add_data("search_mode", i);

		float errorval = compress_symbolic_block_for_partition_1plane(
		    ctx.config, *bsd, blk, ewb, i == 0,
		    error_threshold * errorval_mult[i] * errorval_overshoot,
		    1, 0,  scb, tmpbuf);

		best_errorvals_for_pcount[0] = astc::min(best_errorvals_for_pcount[0], errorval);
		if (errorval < (error_threshold * errorval_mult[i]))
		{
			trace_add_data("exit", "quality hit");
			goto END_OF_TESTS;
		}
	}

#if !defined(ASTCENC_DIAGNOSTICS)
	lowest_correl = prepare_block_statistics(bsd->texel_count, blk, ewb);
#endif

	block_skip_two_plane = lowest_correl > ctx.config.tune_2_plane_early_out_limit_correlation;

	// Test the four possible 1-partition, 2-planes modes. Do this in reverse, as
	// alpha is the most likely to be non-correlated if it is present in the data.
	for (int i = BLOCK_MAX_COMPONENTS - 1; i >= 0; i--)
	{
		TRACE_NODE(node1, "pass");
		trace_add_data("partition_count", 1);
		trace_add_data("plane_count", 2);
		trace_add_data("plane_component", i);

		if (block_skip_two_plane)
		{
			trace_add_data("skip", "tune_2_plane_early_out_limit_correlation");
			continue;
		}

		if (blk.grayscale && i != 3)
		{
			trace_add_data("skip", "grayscale block");
			continue;
		}

		if (blk.is_constant_channel(i))
		{
			trace_add_data("skip", "constant component");
			continue;
		}

		float errorval = compress_symbolic_block_for_partition_2planes(
		    ctx.config, *bsd, blk, ewb,
		    error_threshold * errorval_overshoot,
		    i, scb, tmpbuf);

		// If attempting two planes is much worse than the best one plane result
		// then further two plane searches are unlikely to help so move on ...
		if (errorval > (best_errorvals_for_pcount[0] * 2.0f))
		{
			break;
		}

		if (errorval < error_threshold)
		{
			trace_add_data("exit", "quality hit");
			goto END_OF_TESTS;
		}
	}

	// Find best blocks for 2, 3 and 4 partitions
	for (int partition_count = 2; partition_count <= max_partitions; partition_count++)
	{
		unsigned int partition_indices_1plane[2] { 0, 0 };

		find_best_partition_candidates(*bsd, blk, ewb, partition_count,
		                               ctx.config.tune_partition_index_limit,
		                               partition_indices_1plane[0],
		                               partition_indices_1plane[1]);

		for (int i = 0; i < 2; i++)
		{
			TRACE_NODE(node1, "pass");
			trace_add_data("partition_count", partition_count);
			trace_add_data("partition_index", partition_indices_1plane[i]);
			trace_add_data("plane_count", 1);
			trace_add_data("search_mode", i);

			float errorval = compress_symbolic_block_for_partition_1plane(
			    ctx.config, *bsd, blk, ewb, false,
			    error_threshold * errorval_overshoot,
			    partition_count, partition_indices_1plane[i],
			    scb, tmpbuf);

			best_errorvals_for_pcount[partition_count - 1] = astc::min(best_errorvals_for_pcount[partition_count - 1], errorval);
			if (errorval < error_threshold)
			{
				trace_add_data("exit", "quality hit");
				goto END_OF_TESTS;
			}
		}

		// If using N partitions doesn't improve much over using N-1 partitions then skip trying N+1
		float best_error = best_errorvals_for_pcount[partition_count - 1];
		float best_error_in_prev = best_errorvals_for_pcount[partition_count - 2];
		float best_error_scale = exit_thresholds_for_pcount[partition_count - 1];
		if (best_error > (best_error_in_prev * best_error_scale))
		{
			trace_add_data("skip", "tune_partition_early_out_limit_factor");
			goto END_OF_TESTS;
		}
	}

	trace_add_data("exit", "quality not hit");

END_OF_TESTS:
	// Compress to a physical block
	symbolic_to_physical(*bsd, scb, pcb);
}