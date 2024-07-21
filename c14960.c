Error ImageLoaderTGA::convert_to_image(Ref<Image> p_image, const uint8_t *p_buffer, const tga_header_s &p_header, const uint8_t *p_palette, const bool p_is_monochrome) {
#define TGA_PUT_PIXEL(r, g, b, a)             \
	int image_data_ofs = ((y * width) + x);   \
	image_data_w[image_data_ofs * 4 + 0] = r; \
	image_data_w[image_data_ofs * 4 + 1] = g; \
	image_data_w[image_data_ofs * 4 + 2] = b; \
	image_data_w[image_data_ofs * 4 + 3] = a;

	uint32_t width = p_header.image_width;
	uint32_t height = p_header.image_height;
	tga_origin_e origin = static_cast<tga_origin_e>((p_header.image_descriptor & TGA_ORIGIN_MASK) >> TGA_ORIGIN_SHIFT);

	uint32_t x_start;
	int32_t x_step;
	uint32_t x_end;
	uint32_t y_start;
	int32_t y_step;
	uint32_t y_end;

	if (origin == TGA_ORIGIN_TOP_LEFT || origin == TGA_ORIGIN_TOP_RIGHT) {
		y_start = 0;
		y_step = 1;
		y_end = height;
	} else {
		y_start = height - 1;
		y_step = -1;
		y_end = -1;
	}

	if (origin == TGA_ORIGIN_TOP_LEFT || origin == TGA_ORIGIN_BOTTOM_LEFT) {
		x_start = 0;
		x_step = 1;
		x_end = width;
	} else {
		x_start = width - 1;
		x_step = -1;
		x_end = -1;
	}

	Vector<uint8_t> image_data;
	image_data.resize(width * height * sizeof(uint32_t));
	uint8_t *image_data_w = image_data.ptrw();

	size_t i = 0;
	uint32_t x = x_start;
	uint32_t y = y_start;

	if (p_header.pixel_depth == 8) {
		if (p_is_monochrome) {
			while (y != y_end) {
				while (x != x_end) {
					uint8_t shade = p_buffer[i];

					TGA_PUT_PIXEL(shade, shade, shade, 0xff)

					x += x_step;
					i += 1;
				}
				x = x_start;
				y += y_step;
			}
		} else {
			while (y != y_end) {
				while (x != x_end) {
					uint8_t index = p_buffer[i];
					uint8_t r = 0x00;
					uint8_t g = 0x00;
					uint8_t b = 0x00;
					uint8_t a = 0xff;

					if (p_header.color_map_depth == 24) {
						// Due to low-high byte order, the color table must be
						// read in the same order as image data (little endian)
						r = (p_palette[(index * 3) + 2]);
						g = (p_palette[(index * 3) + 1]);
						b = (p_palette[(index * 3) + 0]);
					} else {
						return ERR_INVALID_DATA;
					}

					TGA_PUT_PIXEL(r, g, b, a)

					x += x_step;
					i += 1;
				}
				x = x_start;
				y += y_step;
			}
		}
	} else if (p_header.pixel_depth == 24) {
		while (y != y_end) {
			while (x != x_end) {
				uint8_t r = p_buffer[i + 2];
				uint8_t g = p_buffer[i + 1];
				uint8_t b = p_buffer[i + 0];

				TGA_PUT_PIXEL(r, g, b, 0xff)

				x += x_step;
				i += 3;
			}
			x = x_start;
			y += y_step;
		}
	} else if (p_header.pixel_depth == 32) {
		while (y != y_end) {
			while (x != x_end) {
				uint8_t a = p_buffer[i + 3];
				uint8_t r = p_buffer[i + 2];
				uint8_t g = p_buffer[i + 1];
				uint8_t b = p_buffer[i + 0];

				TGA_PUT_PIXEL(r, g, b, a)

				x += x_step;
				i += 4;
			}
			x = x_start;
			y += y_step;
		}
	}

	p_image->create(width, height, false, Image::FORMAT_RGBA8, image_data);

	return OK;
}