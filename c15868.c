static void compute_angular_endpoints_for_quant_levels_lwc(
	unsigned int weight_count,
	const float* dec_weight_quant_uvalue,
	const float* dec_weight_quant_sig,
	unsigned int max_quant_level,
	float low_value[12],
	float high_value[12]
) {
	unsigned int max_quant_steps = quantization_steps_for_level[max_quant_level];
	unsigned int max_angular_steps = max_angular_steps_needed_for_quant_level[max_quant_level];

	alignas(ASTCENC_VECALIGN) float angular_offsets[ANGULAR_STEPS];
	alignas(ASTCENC_VECALIGN) int32_t lowest_weight[ANGULAR_STEPS];
	alignas(ASTCENC_VECALIGN) int32_t weight_span[ANGULAR_STEPS];
	alignas(ASTCENC_VECALIGN) float error[ANGULAR_STEPS];

	compute_angular_offsets(weight_count, dec_weight_quant_uvalue, dec_weight_quant_sig,
	                        max_angular_steps, angular_offsets);


	compute_lowest_and_highest_weight_lwc(weight_count, dec_weight_quant_uvalue, dec_weight_quant_sig,
	                                      max_angular_steps, max_quant_steps,
	                                      angular_offsets, lowest_weight, weight_span, error);

	// For each quantization level, find the best error terms. Use packed vectors so data-dependent
	// branches can become selects. This involves some integer to float casts, but the values are
	// small enough so they never round the wrong way.
	float best_error[ANGULAR_STEPS];
	int best_index[ANGULAR_STEPS];

	// Initialize the array to some safe defaults
	promise(max_quant_steps > 0);
	for (unsigned int i = 0; i < (max_quant_steps + 4); i++)
	{
		best_error[i] = ERROR_CALC_DEFAULT;
		best_index[i] = -1;
	}

	promise(max_angular_steps > 0);
	for (unsigned int i = 0; i < max_angular_steps; i++)
	{
		int idx_span = weight_span[i];

		// Check best error against record N
		float current_best = best_error[idx_span];
		if (error[i] < current_best)
		{
			best_error[idx_span] = error[i];
			best_index[idx_span] = i;
		}
	}

	for (unsigned int i = 0; i <= max_quant_level; i++)
	{
		unsigned int q = quantization_steps_for_level[i];
		int bsi = best_index[q];

		// Did we find anything?
#if !defined(NDEBUG)
		if ((bsi < 0) && print_once)
		{
			print_once = false;
			printf("INFO: Unable to find low weight encoding within search error limit\n\n");
		}
#endif

		bsi = astc::max(0, bsi);

		int lwi = lowest_weight[bsi];
		int hwi = lwi + q - 1;

		low_value[i]  = (angular_offsets[bsi] + static_cast<float>(lwi)) / (1.0f + (float)bsi);
		high_value[i] = (angular_offsets[bsi] + static_cast<float>(hwi)) / (1.0f + (float)bsi);
	}
}