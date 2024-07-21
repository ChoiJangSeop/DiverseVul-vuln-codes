void symbolic_to_physical(
	const block_size_descriptor& bsd,
	const symbolic_compressed_block& scb,
	physical_compressed_block& pcb
) {
	// Constant color block using UNORM16 colors
	if (scb.block_type == SYM_BTYPE_CONST_U16)
	{
		// There is currently no attempt to coalesce larger void-extents
		static const uint8_t cbytes[8] { 0xFC, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
		for (unsigned int i = 0; i < 8; i++)
		{
			pcb.data[i] = cbytes[i];
		}

		for (unsigned int i = 0; i < BLOCK_MAX_COMPONENTS; i++)
		{
			pcb.data[2 * i + 8] = scb.constant_color[i] & 0xFF;
			pcb.data[2 * i + 9] = (scb.constant_color[i] >> 8) & 0xFF;
		}

		return;
	}

	// Constant color block using FP16 colors
	if (scb.block_type == SYM_BTYPE_CONST_F16)
	{
		// There is currently no attempt to coalesce larger void-extents
		static const uint8_t cbytes[8]  { 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
		for (unsigned int i = 0; i < 8; i++)
		{
			pcb.data[i] = cbytes[i];
		}

		for (unsigned int i = 0; i < BLOCK_MAX_COMPONENTS; i++)
		{
			pcb.data[2 * i + 8] = scb.constant_color[i] & 0xFF;
			pcb.data[2 * i + 9] = (scb.constant_color[i] >> 8) & 0xFF;
		}

		return;
	}

	unsigned int partition_count = scb.partition_count;

	// Compress the weights.
	// They are encoded as an ordinary integer-sequence, then bit-reversed
	uint8_t weightbuf[16] { 0 };

	const auto& bm = bsd.get_block_mode(scb.block_mode);
	const auto& di = bsd.get_decimation_info(bm.decimation_mode);
	int weight_count = di.weight_count;
	quant_method weight_quant_method = bm.get_weight_quant_mode();
	int is_dual_plane = bm.is_dual_plane;

	int real_weight_count = is_dual_plane ? 2 * weight_count : weight_count;

	int bits_for_weights = get_ise_sequence_bitcount(real_weight_count, weight_quant_method);

	if (is_dual_plane)
	{
		uint8_t weights[64];
		for (int i = 0; i < weight_count; i++)
		{
			weights[2 * i] = scb.weights[i];
			weights[2 * i + 1] = scb.weights[i + WEIGHTS_PLANE2_OFFSET];
		}
		encode_ise(weight_quant_method, real_weight_count, weights, weightbuf, 0);
	}
	else
	{
		encode_ise(weight_quant_method, weight_count, scb.weights, weightbuf, 0);
	}

	for (int i = 0; i < 16; i++)
	{
		pcb.data[i] = static_cast<uint8_t>(bitrev8(weightbuf[15 - i]));
	}

	write_bits(scb.block_mode, 11, 0, pcb.data);
	write_bits(partition_count - 1, 2, 11, pcb.data);

	int below_weights_pos = 128 - bits_for_weights;

	// Encode partition index and color endpoint types for blocks with 2+ partitions
	if (partition_count > 1)
	{
		write_bits(scb.partition_index, 6, 13, pcb.data);
		write_bits(scb.partition_index >> 6, PARTITION_INDEX_BITS - 6, 19, pcb.data);

		if (scb.color_formats_matched)
		{
			write_bits(scb.color_formats[0] << 2, 6, 13 + PARTITION_INDEX_BITS, pcb.data);
		}
		else
		{
			// Check endpoint types for each partition to determine the lowest class present
			int low_class = 4;

			for (unsigned int i = 0; i < partition_count; i++)
			{
				int class_of_format = scb.color_formats[i] >> 2;
				low_class = astc::min(class_of_format, low_class);
			}

			if (low_class == 3)
			{
				low_class = 2;
			}

			int encoded_type = low_class + 1;
			int bitpos = 2;

			for (unsigned int i = 0; i < partition_count; i++)
			{
				int classbit_of_format = (scb.color_formats[i] >> 2) - low_class;
				encoded_type |= classbit_of_format << bitpos;
				bitpos++;
			}

			for (unsigned int i = 0; i < partition_count; i++)
			{
				int lowbits_of_format = scb.color_formats[i] & 3;
				encoded_type |= lowbits_of_format << bitpos;
				bitpos += 2;
			}

			int encoded_type_lowpart = encoded_type & 0x3F;
			int encoded_type_highpart = encoded_type >> 6;
			int encoded_type_highpart_size = (3 * partition_count) - 4;
			int encoded_type_highpart_pos = 128 - bits_for_weights - encoded_type_highpart_size;
			write_bits(encoded_type_lowpart, 6, 13 + PARTITION_INDEX_BITS, pcb.data);
			write_bits(encoded_type_highpart, encoded_type_highpart_size, encoded_type_highpart_pos, pcb.data);
			below_weights_pos -= encoded_type_highpart_size;
		}
	}
	else
	{
		write_bits(scb.color_formats[0], 4, 13, pcb.data);
	}

	// In dual-plane mode, encode the color component of the second plane of weights
	if (is_dual_plane)
	{
		write_bits(scb.plane2_component, 2, below_weights_pos - 2, pcb.data);
	}

	// Encode the color components
	uint8_t values_to_encode[32];
	int valuecount_to_encode = 0;
	for (unsigned int i = 0; i < scb.partition_count; i++)
	{
		int vals = 2 * (scb.color_formats[i] >> 2) + 2;
		assert(vals <= 8);
		for (int j = 0; j < vals; j++)
		{
			values_to_encode[j + valuecount_to_encode] = scb.color_values[i][j];
		}
		valuecount_to_encode += vals;
	}

	encode_ise(scb.get_color_quant_mode(), valuecount_to_encode, values_to_encode, pcb.data,
	           scb.partition_count == 1 ? 17 : 19 + PARTITION_INDEX_BITS);
}