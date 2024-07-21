Error ImageLoaderTGA::load_image(Ref<Image> p_image, FileAccess *f, bool p_force_linear, float p_scale) {
	Vector<uint8_t> src_image;
	int src_image_len = f->get_len();
	ERR_FAIL_COND_V(src_image_len == 0, ERR_FILE_CORRUPT);
	ERR_FAIL_COND_V(src_image_len < (int)sizeof(tga_header_s), ERR_FILE_CORRUPT);
	src_image.resize(src_image_len);

	Error err = OK;

	tga_header_s tga_header;
	tga_header.id_length = f->get_8();
	tga_header.color_map_type = f->get_8();
	tga_header.image_type = static_cast<tga_type_e>(f->get_8());

	tga_header.first_color_entry = f->get_16();
	tga_header.color_map_length = f->get_16();
	tga_header.color_map_depth = f->get_8();

	tga_header.x_origin = f->get_16();
	tga_header.y_origin = f->get_16();
	tga_header.image_width = f->get_16();
	tga_header.image_height = f->get_16();
	tga_header.pixel_depth = f->get_8();
	tga_header.image_descriptor = f->get_8();

	bool is_encoded = (tga_header.image_type == TGA_TYPE_RLE_INDEXED || tga_header.image_type == TGA_TYPE_RLE_RGB || tga_header.image_type == TGA_TYPE_RLE_MONOCHROME);
	bool has_color_map = (tga_header.image_type == TGA_TYPE_RLE_INDEXED || tga_header.image_type == TGA_TYPE_INDEXED);
	bool is_monochrome = (tga_header.image_type == TGA_TYPE_RLE_MONOCHROME || tga_header.image_type == TGA_TYPE_MONOCHROME);

	if (tga_header.image_type == TGA_TYPE_NO_DATA) {
		err = FAILED;
	}

	if (has_color_map) {
		if (tga_header.color_map_length > 256 || (tga_header.color_map_depth != 24) || tga_header.color_map_type != 1) {
			err = FAILED;
		}
	} else {
		if (tga_header.color_map_type) {
			err = FAILED;
		}
	}

	if (tga_header.image_width <= 0 || tga_header.image_height <= 0) {
		err = FAILED;
	}

	if (!(tga_header.pixel_depth == 8 || tga_header.pixel_depth == 24 || tga_header.pixel_depth == 32)) {
		err = FAILED;
	}

	if (err == OK) {
		f->seek(f->get_position() + tga_header.id_length);

		Vector<uint8_t> palette;

		if (has_color_map) {
			size_t color_map_size = tga_header.color_map_length * (tga_header.color_map_depth >> 3);
			err = palette.resize(color_map_size);
			if (err == OK) {
				uint8_t *palette_w = palette.ptrw();
				f->get_buffer(&palette_w[0], color_map_size);
			} else {
				return OK;
			}
		}

		uint8_t *src_image_w = src_image.ptrw();
		f->get_buffer(&src_image_w[0], src_image_len - f->get_position());

		const uint8_t *src_image_r = src_image.ptr();

		const size_t pixel_size = tga_header.pixel_depth >> 3;
		const size_t buffer_size = (tga_header.image_width * tga_header.image_height) * pixel_size;

		Vector<uint8_t> uncompressed_buffer;
		uncompressed_buffer.resize(buffer_size);
		uint8_t *uncompressed_buffer_w = uncompressed_buffer.ptrw();
		const uint8_t *uncompressed_buffer_r;

		const uint8_t *buffer = nullptr;

		if (is_encoded) {
			err = decode_tga_rle(src_image_r, pixel_size, uncompressed_buffer_w, buffer_size);

			if (err == OK) {
				uncompressed_buffer_r = uncompressed_buffer.ptr();
				buffer = uncompressed_buffer_r;
			}
		} else {
			buffer = src_image_r;
		};

		if (err == OK) {
			const uint8_t *palette_r = palette.ptr();
			err = convert_to_image(p_image, buffer, tga_header, palette_r, is_monochrome);
		}
	}

	f->close();
	return err;
}