static void compute_angular_endpoints_for_quant_levels(
	unsigned int weight_count,
	const float* dec_weight_quant_uvalue,
	const float* dec_weight_quant_sig,
	unsigned int max_quant_level,
	float low_value[12],
	float high_value[12]
) {
	unsigned int max_quant_steps = quantization_steps_for_level[max_quant_level];

	alignas(ASTCENC_VECALIGN) float angular_offsets[ANGULAR_STEPS];
	unsigned int max_angular_steps = max_angular_steps_needed_for_quant_level[max_quant_level];
	compute_angular_offsets(weight_count, dec_weight_quant_uvalue, dec_weight_quant_sig,
	                        max_angular_steps, angular_offsets);

	alignas(ASTCENC_VECALIGN) int32_t lowest_weight[ANGULAR_STEPS];
	alignas(ASTCENC_VECALIGN) int32_t weight_span[ANGULAR_STEPS];
	alignas(ASTCENC_VECALIGN) float error[ANGULAR_STEPS];
	alignas(ASTCENC_VECALIGN) float cut_low_weight_error[ANGULAR_STEPS];
	alignas(ASTCENC_VECALIGN) float cut_high_weight_error[ANGULAR_STEPS];

	compute_lowest_and_highest_weight(weight_count, dec_weight_quant_uvalue, dec_weight_quant_sig,
	                                  max_angular_steps, max_quant_steps,
	                                  angular_offsets, lowest_weight, weight_span, error,
	                                  cut_low_weight_error, cut_high_weight_error);

	// For each quantization level, find the best error terms. Use packed vectors so data-dependent
	// branches can become selects. This involves some integer to float casts, but the values are
	// small enough so they never round the wrong way.
	vfloat4 best_results[40];

	// Initialize the array to some safe defaults
	promise(max_quant_steps > 0);
	for (unsigned int i = 0; i < (max_quant_steps + 4); i++)
	{
		// Lane<0> = Best error
		// Lane<1> = Best scale; -1 indicates no solution found
		// Lane<2> = Cut low weight
		best_results[i] = vfloat4(ERROR_CALC_DEFAULT, -1.0f, 0.0f, 0.0f);
	}

	promise(max_angular_steps > 0);
	for (unsigned int i = 0; i < max_angular_steps; i++)
	{
		int idx_span = weight_span[i];
		float error_cut_low = error[i] + cut_low_weight_error[i];
		float error_cut_high = error[i] + cut_high_weight_error[i];
		float error_cut_low_high = error[i] + cut_low_weight_error[i] + cut_high_weight_error[i];

		// Check best error against record N
		vfloat4 best_result = best_results[idx_span];
		vfloat4 new_result = vfloat4(error[i], (float)i, 0.0f, 0.0f);
		vmask4 mask1(best_result.lane<0>() > error[i]);
		best_results[idx_span] = select(best_result, new_result, mask1);

		// Check best error against record N-1 with either cut low or cut high
		best_result = best_results[idx_span - 1];

		new_result = vfloat4(error_cut_low, (float)i, 1.0f, 0.0f);
		vmask4 mask2(best_result.lane<0>() > error_cut_low);
		best_result = select(best_result, new_result, mask2);

		new_result = vfloat4(error_cut_high, (float)i, 0.0f, 0.0f);
		vmask4 mask3(best_result.lane<0>() > error_cut_high);
		best_results[idx_span - 1] = select(best_result, new_result, mask3);

		// Check best error against record N-2 with both cut low and high
		best_result = best_results[idx_span - 2];
		new_result = vfloat4(error_cut_low_high, (float)i, 1.0f, 0.0f);
		vmask4 mask4(best_result.lane<0>() > error_cut_low_high);
		best_results[idx_span - 2] = select(best_result, new_result, mask4);
	}

	for (unsigned int i = 0; i <= max_quant_level; i++)
	{
		unsigned int q = quantization_steps_for_level[i];
		int bsi = (int)best_results[q].lane<1>();

		// Did we find anything?
#if !defined(NDEBUG)
		if ((bsi < 0) && print_once)
		{
			print_once = false;
			printf("INFO: Unable to find full encoding within search error limit\n\n");
		}
#endif

		bsi = astc::max(0, bsi);

		float stepsize = 1.0f / (1.0f + (float)bsi);
		int lwi = lowest_weight[bsi] + (int)best_results[q].lane<2>();
		int hwi = lwi + q - 1;

		float offset = angular_offsets[bsi] * stepsize;
		low_value[i] = offset + static_cast<float>(lwi) * stepsize;
		high_value[i] = offset + static_cast<float>(hwi) * stepsize;
	}
}