Error ImageLoaderTGA::decode_tga_rle(const uint8_t *p_compressed_buffer, size_t p_pixel_size, uint8_t *p_uncompressed_buffer, size_t p_output_size) {
	Error error;

	Vector<uint8_t> pixels;
	error = pixels.resize(p_pixel_size);
	if (error != OK) {
		return error;
	}

	uint8_t *pixels_w = pixels.ptrw();

	size_t compressed_pos = 0;
	size_t output_pos = 0;
	size_t c = 0;
	size_t count = 0;

	while (output_pos < p_output_size) {
		c = p_compressed_buffer[compressed_pos];
		compressed_pos += 1;
		count = (c & 0x7f) + 1;

		if (c & 0x80) {
			for (size_t i = 0; i < p_pixel_size; i++) {
				pixels_w[i] = p_compressed_buffer[compressed_pos];
				compressed_pos += 1;
			}
			for (size_t i = 0; i < count; i++) {
				for (size_t j = 0; j < p_pixel_size; j++) {
					p_uncompressed_buffer[output_pos + j] = pixels_w[j];
				}
				output_pos += p_pixel_size;
			}
		} else {
			count *= p_pixel_size;
			for (size_t i = 0; i < count; i++) {
				p_uncompressed_buffer[output_pos] = p_compressed_buffer[compressed_pos];
				compressed_pos += 1;
				output_pos += 1;
			}
		}
	}
	return OK;
}