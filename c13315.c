void recompute_ideal_colors_1plane(
	const image_block& blk,
	const error_weight_block& ewb,
	const partition_info& pi,
	const decimation_info& di,
	int weight_quant_mode,
	const uint8_t* dec_weights_quant_pvalue,
	endpoints& ep,
	vfloat4 rgbs_vectors[BLOCK_MAX_PARTITIONS],
	vfloat4 rgbo_vectors[BLOCK_MAX_PARTITIONS]
) {
	int weight_count = di.weight_count;
	int partition_count = pi.partition_count;
	bool is_decimated = di.weight_count != di.texel_count;

	promise(weight_count > 0);
	promise(partition_count > 0);

	const quantization_and_transfer_table& qat = quant_and_xfer_tables[weight_quant_mode];

	float dec_weight_quant_uvalue[BLOCK_MAX_WEIGHTS];
	for (int i = 0; i < weight_count; i++)
	{
		dec_weight_quant_uvalue[i] = qat.unquantized_value[dec_weights_quant_pvalue[i]] * (1.0f / 64.0f);
	}

	for (int i = 0; i < partition_count; i++)
	{
		vfloat4 rgba_sum(1e-17f);
		vfloat4 rgba_weight_sum(1e-17f);

		int texel_count = pi.partition_texel_count[i];
		const uint8_t *texel_indexes = pi.texels_of_partition[i];

		promise(texel_count > 0);
		for (int j = 0; j < texel_count; j++)
		{
			int tix = texel_indexes[j];

			vfloat4 rgba = blk.texel(tix);
			vfloat4 error_weight = ewb.error_weights[tix];

			rgba_sum += rgba * error_weight;
			rgba_weight_sum += error_weight;
		}

		vfloat4 scale_direction = normalize((rgba_sum * (1.0f / rgba_weight_sum)).swz<0, 1, 2>());

		float scale_max = 0.0f;
		float scale_min = 1e10f;

		float wmin1 = 1.0f;
		float wmax1 = 0.0f;

		vfloat4 left_sum    = vfloat4::zero();
		vfloat4 middle_sum  = vfloat4::zero();
		vfloat4 right_sum   = vfloat4::zero();
		vfloat4 lmrs_sum    = vfloat4::zero();

		vfloat4 color_vec_x = vfloat4::zero();
		vfloat4 color_vec_y = vfloat4::zero();

		vfloat4 scale_vec = vfloat4::zero();

		vfloat4 weight_weight_sum = vfloat4(1e-17f);
		float psum = 1e-17f;

		for (int j = 0; j < texel_count; j++)
		{
			int tix = texel_indexes[j];

			vfloat4 rgba = blk.texel(tix);
			vfloat4 color_weight = ewb.error_weights[tix];

			// TODO: Move this calculation out to the color block?
			float ls_weight = hadd_rgb_s(color_weight);

			float idx0 = dec_weight_quant_uvalue[tix];
			if (is_decimated)
			{
				idx0 = bilinear_infill(di, dec_weight_quant_uvalue, tix);
			}

			float om_idx0 = 1.0f - idx0;
			wmin1 = astc::min(idx0, wmin1);
			wmax1 = astc::max(idx0, wmax1);

			float scale = dot3_s(scale_direction, rgba);
			scale_min = astc::min(scale, scale_min);
			scale_max = astc::max(scale, scale_max);

			vfloat4 left   = color_weight * (om_idx0 * om_idx0);
			vfloat4 middle = color_weight * (om_idx0 * idx0);
			vfloat4 right  = color_weight * (idx0 * idx0);

			vfloat4 lmrs = vfloat3(om_idx0 * om_idx0,
			                       om_idx0 * idx0,
			                       idx0 * idx0) * ls_weight;

			left_sum   += left;
			middle_sum += middle;
			right_sum  += right;
			lmrs_sum   += lmrs;

			vfloat4 color_idx(idx0);
			vfloat4 cwprod = color_weight * rgba;
			vfloat4 cwiprod = cwprod * color_idx;

			color_vec_y += cwiprod;
			color_vec_x += cwprod - cwiprod;

			scale_vec += vfloat2(om_idx0, idx0) * (ls_weight * scale);
			weight_weight_sum += color_weight * color_idx;
			psum += dot3_s(color_weight * color_idx, color_idx);
		}

		// Calculations specific to mode #7, the HDR RGB-scale mode
		vfloat4 rgbq_sum = color_vec_x + color_vec_y;
		rgbq_sum.set_lane<3>(hadd_rgb_s(color_vec_y));

		vfloat4 rgbovec = compute_rgbo_vector(rgba_weight_sum, weight_weight_sum,
		                                  rgbq_sum, psum);
		rgbo_vectors[i] = rgbovec;

		// We will occasionally get a failure due to the use of a singular (non-invertible) matrix.
		// Record whether such a failure has taken place; if it did, compute rgbo_vectors[] with a
		// different method later
		float chkval = dot_s(rgbovec, rgbovec);
		int rgbo_fail = chkval != chkval;

		// Initialize the luminance and scale vectors with a reasonable default
		float scalediv = scale_min * (1.0f / astc::max(scale_max, 1e-10f));
		scalediv = astc::clamp1f(scalediv);

		vfloat4 sds = scale_direction * scale_max;

		rgbs_vectors[i] = vfloat4(sds.lane<0>(), sds.lane<1>(), sds.lane<2>(), scalediv);

		if (wmin1 >= wmax1 * 0.999f)
		{
			// If all weights in the partition were equal, then just take average of all colors in
			// the partition and use that as both endpoint colors
			vfloat4 avg = (color_vec_x + color_vec_y) * (1.0f / rgba_weight_sum);

			vmask4 notnan_mask = avg == avg;
			ep.endpt0[i] = select(ep.endpt0[i], avg, notnan_mask);
			ep.endpt1[i] = select(ep.endpt1[i], avg, notnan_mask);

			rgbs_vectors[i] = vfloat4(sds.lane<0>(), sds.lane<1>(), sds.lane<2>(), 1.0f);
		}
		else
		{
			// Otherwise, complete the analytic calculation of ideal-endpoint-values for the given
			// set of texel weights and pixel colors
			vfloat4 color_det1 = (left_sum * right_sum) - (middle_sum * middle_sum);
			vfloat4 color_rdet1 = 1.0f / color_det1;

			float ls_det1  = (lmrs_sum.lane<0>() * lmrs_sum.lane<2>()) - (lmrs_sum.lane<1>() * lmrs_sum.lane<1>());
			float ls_rdet1 = 1.0f / ls_det1;

			vfloat4 color_mss1 = (left_sum * left_sum)
			                   + (2.0f * middle_sum * middle_sum)
			                   + (right_sum * right_sum);

			float ls_mss1 = (lmrs_sum.lane<0>() * lmrs_sum.lane<0>())
			              + (2.0f * lmrs_sum.lane<1>() * lmrs_sum.lane<1>())
			              + (lmrs_sum.lane<2>() * lmrs_sum.lane<2>());

			vfloat4 ep0 = (right_sum * color_vec_x - middle_sum * color_vec_y) * color_rdet1;
			vfloat4 ep1 = (left_sum * color_vec_y - middle_sum * color_vec_x) * color_rdet1;

			vmask4 det_mask = abs(color_det1) > (color_mss1 * 1e-4f);
			vmask4 notnan_mask = (ep0 == ep0) & (ep1 == ep1);
			vmask4 full_mask = det_mask & notnan_mask;

			ep.endpt0[i] = select(ep.endpt0[i], ep0, full_mask);
			ep.endpt1[i] = select(ep.endpt1[i], ep1, full_mask);

			float scale_ep0 = (lmrs_sum.lane<2>() * scale_vec.lane<0>() - lmrs_sum.lane<1>() * scale_vec.lane<1>()) * ls_rdet1;
			float scale_ep1 = (lmrs_sum.lane<0>() * scale_vec.lane<1>() - lmrs_sum.lane<1>() * scale_vec.lane<0>()) * ls_rdet1;

			if (fabsf(ls_det1) > (ls_mss1 * 1e-4f) && scale_ep0 == scale_ep0 && scale_ep1 == scale_ep1 && scale_ep0 < scale_ep1)
			{
				float scalediv2 = scale_ep0 * (1.0f / scale_ep1);
				vfloat4 sdsm = scale_direction * scale_ep1;
				rgbs_vectors[i] = vfloat4(sdsm.lane<0>(), sdsm.lane<1>(), sdsm.lane<2>(), scalediv2);
			}
		}

		// If the calculation of an RGB-offset vector failed, try to compute a value another way
		if (rgbo_fail)
		{
			vfloat4 v0 = ep.endpt0[i];
			vfloat4 v1 = ep.endpt1[i];

			float avgdif = hadd_rgb_s(v1 - v0) * (1.0f / 3.0f);
			avgdif = astc::max(avgdif, 0.0f);

			vfloat4 avg = (v0 + v1) * 0.5f;
			vfloat4 ep0 = avg - vfloat4(avgdif) * 0.5f;

			rgbo_vectors[i] = vfloat4(ep0.lane<0>(), ep0.lane<1>(), ep0.lane<2>(), avgdif);
		}
	}
}