JPEGQS_ATTR int QS_NAME(j_decompress_ptr srcinfo, jvirt_barray_ptr *coef_arrays, jpegqs_control_t *opts) {
	JDIMENSION comp_width, comp_height, blk_y;
	int i, ci, stride, iter, stride1 = 0, need_downsample = 0;
	jpeg_component_info *compptr; int64_t size;
	JQUANT_TBL *qtbl; JSAMPLE *image, *image1 = NULL, *image2 = NULL;
	int num_iter = opts->niter, old_threads = -1;
	int prog_next = 0, prog_max = 0, prog_thr = 0, prog_prec = opts->progprec;
#ifdef PRECISE_PROGRESS
	volatile int stop = 0;
#else
	int stop = 0;
#endif
	jvirt_barray_ptr coef_up[2] = { NULL, NULL };
	float **tables = NULL;

#ifdef WITH_LOG
	int64_t time = 0;

	if (opts->flags & JPEGQS_INFO_COMP1)
	for (ci = 0; ci < srcinfo->num_components; ci++) {
		compptr = srcinfo->comp_info + ci;
		i = compptr->quant_tbl_no;
		logfmt("component[%i] : table %i, samp %ix%i\n", ci, i,
				compptr->h_samp_factor, compptr->v_samp_factor);
	}

	if (opts->flags & JPEGQS_INFO_QUANT)
	for (i = 0; i < NUM_QUANT_TBLS; i++) {
		int x, y;
		qtbl = srcinfo->quant_tbl_ptrs[i];
		if (!qtbl) continue;
		logfmt("quant[%i]:\n", i);

		for (y = 0; y < DCTSIZE; y++) {
			for (x = 0; x < DCTSIZE; x++)
				logfmt("%04x ", qtbl->quantval[y * DCTSIZE + x]);
			logfmt("\n");
		}
	}

	if (opts->flags & JPEGQS_INFO_TIME) time = get_time_usec();
#endif

	compptr = srcinfo->comp_info;
	if (opts->flags & (JPEGQS_JOINT_YUV | JPEGQS_UPSAMPLE_UV) &&
			srcinfo->jpeg_color_space == JCS_YCbCr &&
			!((compptr[1].h_samp_factor - 1) | (compptr[1].v_samp_factor - 1) |
			(compptr[2].h_samp_factor - 1) | (compptr[2].v_samp_factor - 1))) {
		need_downsample = 1;
	}

	if (num_iter < 0) num_iter = 0;
	if (num_iter > JPEGQS_ITER_MAX) num_iter = JPEGQS_ITER_MAX;

	if (num_iter <= 0 && !(opts->flags & JPEGQS_UPSAMPLE_UV && need_downsample)) return 0;

	range_limit_init();
	if (!(opts->flags & JPEGQS_LOW_QUALITY)) {
		tables = quantsmooth_init(opts->flags);
		if (!tables) return 0;
	}

	(void)old_threads;
#ifdef _OPENMP
	if (opts->threads >= 0) {
		old_threads = omp_get_max_threads();
		omp_set_num_threads(opts->threads ? opts->threads : omp_get_num_procs());
	}
#endif

	if (opts->progress) {
		for (ci = 0; ci < srcinfo->num_components; ci++) {
			compptr = srcinfo->comp_info + ci;
			prog_max += compptr->height_in_blocks * compptr->v_samp_factor * num_iter;
		}
		if (prog_prec == 0) prog_prec = 20;
		if (prog_prec < 0) prog_prec = prog_max;
		prog_thr = (unsigned)(prog_max + prog_prec - 1) / (unsigned)prog_prec;
	}

	for (ci = 0; ci < srcinfo->num_components; ci++) {
		UINT16 quantval[DCTSIZE2];
		int extra_refresh = 0, num_iter2 = num_iter;
		int prog_cur = prog_next, prog_inc;
		compptr = srcinfo->comp_info + ci;
		comp_width = compptr->width_in_blocks;
		comp_height = compptr->height_in_blocks;
		prog_inc = compptr->v_samp_factor;
		prog_next += comp_height * prog_inc * num_iter;
		if (!(qtbl = compptr->quant_table)) continue;

		if (image1 || (!ci && need_downsample)) extra_refresh = 1;

		// skip if already processed
		{
			int val = 0;
			for (i = 0; i < DCTSIZE2; i++) val |= (int)qtbl->quantval[i] - 1;
			if (val <= 0) num_iter2 = 0;

			// damaged JPEG files may contain multipliers equal to zero
			// replacing them with ones avoids division by zero
			for (i = 0; i < DCTSIZE2; i++) {
				val = qtbl->quantval[i];
				quantval[i] = val - ((val - 1) >> 16);
			}
		}

		if (num_iter2 + extra_refresh == 0) continue;
		image = NULL;
		if (!stop) {
			// keeping block pointers aligned
			stride = comp_width * DCTSIZE + 8;
			size = ((int64_t)(comp_height * DCTSIZE + 2) * stride + 8) * sizeof(JSAMPLE);
			if (size == (int64_t)(size_t)size)
				image = (JSAMPLE*)malloc(size);
		}
		if (!image) {
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
			for (blk_y = 0; blk_y < comp_height; blk_y++) {
				JDIMENSION blk_x;
				JBLOCKARRAY buffer = (*srcinfo->mem->access_virt_barray)
						((j_common_ptr)srcinfo, coef_arrays[ci], blk_y, 1, TRUE);

				for (blk_x = 0; blk_x < comp_width; blk_x++) {
					JCOEFPTR coef = buffer[0][blk_x]; int i;
					for (i = 0; i < DCTSIZE2; i++) coef[i] *= qtbl->quantval[i];
				}
			}
			continue;
		}
		image += 7;

#ifdef WITH_LOG
		if (opts->flags & JPEGQS_INFO_COMP2)
			logfmt("component[%i] : size %ix%i\n", ci, comp_width, comp_height);
#endif
#define IMAGEPTR (blk_y * DCTSIZE + 1) * stride + blk_x * DCTSIZE + 1

#ifdef USE_JSIMD
		JSAMPROW output_buf[DCTSIZE];
		for (i = 0; i < DCTSIZE; i++) output_buf[i] = image + i * stride;
#endif

		for (iter = 0; iter < num_iter2 + extra_refresh; iter++) {
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
			for (blk_y = 0; blk_y < comp_height; blk_y++) {
				JDIMENSION blk_x;
				JBLOCKARRAY buffer = (*srcinfo->mem->access_virt_barray)
						((j_common_ptr)srcinfo, coef_arrays[ci], blk_y, 1, TRUE);

				for (blk_x = 0; blk_x < comp_width; blk_x++) {
					JCOEFPTR coef = buffer[0][blk_x]; int i;
					if (!iter)
						for (i = 0; i < DCTSIZE2; i++) coef[i] *= qtbl->quantval[i];
#ifdef USE_JSIMD
					int output_col = IMAGEPTR;
#endif
					idct_islow(coef, image + IMAGEPTR, stride);
				}
			}

			{
				int y, w = comp_width * DCTSIZE, h = comp_height * DCTSIZE;
				for (y = 1; y < h + 1; y++) {
					image[y * stride] = image[y * stride + 1];
					image[y * stride + w + 1] = image[y * stride + w];
				}
				memcpy(image, image + stride, stride * sizeof(JSAMPLE));
				memcpy(image + (h + 1) * stride, image + h * stride, stride * sizeof(JSAMPLE));
			}

			if (iter == num_iter2) break;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
			for (blk_y = 0; blk_y < comp_height; blk_y++) {
				JDIMENSION blk_x;
				JBLOCKARRAY buffer = (*srcinfo->mem->access_virt_barray)
						((j_common_ptr)srcinfo, coef_arrays[ci], blk_y, 1, TRUE);

#ifdef PRECISE_PROGRESS
				if (stop) continue;
#endif
				for (blk_x = 0; blk_x < comp_width; blk_x++) {
					JSAMPLE *p2 = image2 && opts->flags & JPEGQS_JOINT_YUV ? image2 + IMAGEPTR : NULL;
					JCOEFPTR coef = buffer[0][blk_x];
					quantsmooth_block(coef, quantval, image + IMAGEPTR, p2, stride,
							opts->flags, tables, !ci || srcinfo->jpeg_color_space != JCS_YCbCr);
				}
#ifdef PRECISE_PROGRESS
				if (opts->progress) {
					int cur = __sync_add_and_fetch(&prog_cur, prog_inc);
					if (cur >= prog_thr && omp_get_thread_num() == 0) {
						cur = (int64_t)prog_prec * cur / prog_max;
						prog_thr = ((int64_t)(cur + 1) * prog_max + prog_prec - 1) / prog_prec;
						stop = PROGRESS_PTR(opts->userdata, cur, prog_prec);
					}
				}
#endif
			}

#ifdef PRECISE_PROGRESS
			if (stop) break;
#else
			if (opts->progress) {
				int cur = prog_cur += comp_height * prog_inc;
				if (cur >= prog_thr) {
					cur = (int64_t)prog_prec * cur / prog_max;
					prog_thr = ((int64_t)(cur + 1) * prog_max + prog_prec - 1) / prog_prec;
					stop = PROGRESS_PTR(opts->userdata, cur, prog_prec);
				}
				if (stop) break;
			}
#endif
		} // iter

		if (!stop && image1) {
			JSAMPLE *mem; int st, w1, h1, h2, ws, hs, ww, hh;
			compptr = srcinfo->comp_info;
			ws = compptr[0].h_samp_factor;
			hs = compptr[0].v_samp_factor;
			w1 = (srcinfo->image_width + ws - 1) / ws;
			h1 = (srcinfo->image_height + hs - 1) / hs;
			comp_width = compptr[0].width_in_blocks;
			comp_height = compptr[0].height_in_blocks;

			coef_up[ci - 1] = (*srcinfo->mem->request_virt_barray)
					((j_common_ptr)srcinfo, JPOOL_IMAGE, FALSE, comp_width, comp_height, 1);
			(*srcinfo->mem->realize_virt_arrays) ((j_common_ptr)srcinfo);

#ifdef _OPENMP
			// need to suppress JERR_BAD_VIRTUAL_ACCESS
			for (blk_y = 0; blk_y < comp_height; blk_y++) {
				(*srcinfo->mem->access_virt_barray)
						((j_common_ptr)srcinfo, coef_up[ci - 1], blk_y, 1, TRUE);
			}
#endif

			ww = comp_width * DCTSIZE;
			hh = comp_height * DCTSIZE;
			st = ((w1 + DCTSIZE) & -DCTSIZE) * ws;
			h2 = ((h1 + DCTSIZE) & -DCTSIZE) * hs;
			size = (int64_t)h2 * st * sizeof(JSAMPLE);
			mem = (JSAMPLE*)(size == (int64_t)(size_t)size ? malloc(size) : NULL);
			if (mem) {
				int y;
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
				for (y = 0; y < h1; y += DCTSIZE) {
					int y1 = y + DCTSIZE; y1 = y1 < h1 ? y1 : h1;
					upsample_row(w1, y, y1, image, image2, stride,
							image1, stride1, mem, st, ww, ws, hs);
				}
				for (y = h1 * hs; y < hh; y++)
					memcpy(mem + y * st, mem + (h1 * hs - 1) * st, st * sizeof(JSAMPLE));

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
				for (blk_y = 0; blk_y < comp_height; blk_y++) {
					JDIMENSION blk_x;
					JBLOCKARRAY buffer = (*srcinfo->mem->access_virt_barray)
							((j_common_ptr)srcinfo, coef_up[ci - 1], blk_y, 1, TRUE);

					for (blk_x = 0; blk_x < comp_width; blk_x++) {
						float ALIGN(32) buf[DCTSIZE2]; int x, y, n = DCTSIZE;
						JSAMPLE *p = mem + blk_y * n * st + blk_x * n;
						JCOEFPTR coef = buffer[0][blk_x];
						for (y = 0; y < n; y++)
						for (x = 0; x < n; x++)
							buf[y * n + x] = p[y * st + x] - CENTERJSAMPLE;
						fdct_float(buf, buf);
						for (x = 0; x < n * n; x++) coef[x] = roundf(buf[x]);
					}
				}
				free(mem);
			}
		} else if (!stop && !ci && need_downsample) do {
			// make downsampled copy of Y component
			int y, w, h, w1, h1, st, ws, hs;

			ws = compptr[0].h_samp_factor;
			hs = compptr[0].v_samp_factor;
			if ((ws - 1) | (hs - 1)) {
				if (opts->flags & JPEGQS_UPSAMPLE_UV) { image1 = image; stride1 = stride; }
			} else { image2 = image; break; }
			w = compptr[1].width_in_blocks * DCTSIZE;
			h = compptr[1].height_in_blocks * DCTSIZE;
			st = w + 8;
			size = ((int64_t)(h + 2) * st + 8) * sizeof(JSAMPLE);
			image2 = (JSAMPLE*)(size == (int64_t)(size_t)size ? malloc(size) : NULL);
			if (!image2) break;
			image2 += 7;

			w1 = (comp_width * DCTSIZE + ws - 1) / ws;
			h1 = (comp_height * DCTSIZE + hs - 1) / hs;

			// faster case for 4:2:0
			if (1 && !((ws - 2) | (hs - 2))) {
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
				for (y = 0; y < h1; y++) {
					int x;
					for (x = 0; x < w1; x++) {
						JSAMPLE *p = image + (y * 2 + 1) * stride + x * 2 + 1;
						int a = p[0] + p[1] + p[stride] + p[stride + 1];
						image2[(y + 1) * st + x + 1] = (a + 2) >> 2;
					}
				}
			} else {
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
				for (y = 0; y < h1; y++) {
					int x, h2 = comp_height * DCTSIZE - y * hs;
					h2 = h2 < hs ? h2 : hs;
					for (x = 0; x < w1; x++) {
						JSAMPLE *p = image + (y * hs + 1) * stride + x * ws + 1;
						int xx, yy, sum = 0, w2 = comp_width * DCTSIZE - x * ws, div;
						w2 = w2 < ws ? w2 : ws; div = w2 * h2;

						for (yy = 0; yy < h2; yy++)
						for (xx = 0; xx < w2; xx++) sum += p[yy * stride + xx];
						image2[(y + 1) * st + x + 1] = (sum + div / 2) / div;
					}
				}
			}

			for (y = 1; y < h1 + 1; y++) {
				int x; JSAMPLE a = image2[y * st + w1];
				image2[y * st] = image2[y * st + 1];
				for (x = w1 + 1; x < w + 2; x++)
					image2[y * st + x] = a;
			}
			memcpy(image2, image2 + st, st * sizeof(JSAMPLE));
			for (y = h1 + 1; y < h + 2; y++)
				memcpy(image2 + y * st, image2 + h1 * st, st * sizeof(JSAMPLE));

		} while (0);
#undef IMAGEPTR
		if (image != image1 && image != image2) free(image - 7);
	}

#ifdef WITH_LOG
	if (!stop && opts->flags & JPEGQS_INFO_TIME) {
		time = get_time_usec() - time;
		logfmt("quantsmooth: %.3fms\n", time * 0.001);
	}
#endif

#ifdef _OPENMP
	if (old_threads > 0) omp_set_num_threads(old_threads);
#endif

	if (tables) free(tables);

	if (image2 != image1 && image2) free(image2 - 7);
	if (image1) free(image1 - 7);
	if (stop) image1 = NULL;
	if (image1) {
		srcinfo->max_h_samp_factor = 1;
		srcinfo->max_v_samp_factor = 1;
		compptr = srcinfo->comp_info;
		compptr[0].h_samp_factor = 1;
		compptr[0].v_samp_factor = 1;
		comp_width = compptr[0].width_in_blocks;
		comp_height = compptr[0].height_in_blocks;
#define M1(i) coef_arrays[i] = coef_up[i - 1]; \
	compptr[i].width_in_blocks = comp_width; \
	compptr[i].height_in_blocks = comp_height;
		M1(1) M1(2)
#undef M1
	}

	for (ci = 0; ci < NUM_QUANT_TBLS; ci++) {
		qtbl = srcinfo->quant_tbl_ptrs[ci];
		if (qtbl) for (i = 0; i < DCTSIZE2; i++) qtbl->quantval[i] = 1;
	}

	for (ci = 0; ci < srcinfo->num_components; ci++) {
		qtbl = srcinfo->comp_info[ci].quant_table;
		if (qtbl) for (i = 0; i < DCTSIZE2; i++) qtbl->quantval[i] = 1;
	}

#ifndef TRANSCODE_ONLY
	if (!(opts->flags & JPEGQS_TRANSCODE)) {
		// things needed for jpeg_read_scanlines() to work correctly
		if (image1) {
#ifdef LIBJPEG_TURBO_VERSION
			srcinfo->master->last_MCU_col[1] = srcinfo->master->last_MCU_col[0];
			srcinfo->master->last_MCU_col[2] = srcinfo->master->last_MCU_col[0];
#endif
			jinit_color_deconverter(srcinfo);
			jinit_upsampler(srcinfo);
			jinit_d_main_controller(srcinfo, FALSE);
			srcinfo->input_iMCU_row = (srcinfo->output_height + DCTSIZE - 1) / DCTSIZE;
		}
		jinit_inverse_dct(srcinfo);
	}
#endif
	return stop;
}