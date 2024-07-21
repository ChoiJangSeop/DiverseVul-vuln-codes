static void quantsmooth_block(JCOEFPTR coef, UINT16 *quantval,
		JSAMPLE *image, JSAMPLE *image2, int stride, int flags, float **tables, int luma) {
	int k, n = DCTSIZE, x, y, need_refresh = 1;
	JSAMPLE ALIGN(32) buf[DCTSIZE2 + DCTSIZE * 6], *border = buf + n * n;
#ifndef NO_SIMD
	int16_t ALIGN(32) temp[DCTSIZE2 * 4 + DCTSIZE * (4 - 2)];
#endif
#ifdef USE_JSIMD
	JSAMPROW output_buf[DCTSIZE]; int output_col = 0;
	for (k = 0; k < n; k++) output_buf[k] = buf + k * n;
#endif
	(void)x;

	if (image2) {
		float ALIGN(32) fbuf[DCTSIZE2];
#if 1 && defined(USE_NEON)
		for (y = 0; y < n; y++) {
			uint8x8_t h0, h1; uint16x8_t sumA, sumB, v0, v1;
			uint16x4_t h2, h3; float32x4_t v5, scale;
			uint32x4_t v4, sumAA1, sumAB1, sumAA2, sumAB2;
#define M1(xx, yy) \
	h0 = vld1_u8(&image2[(y + yy) * stride + xx]); \
	h1 = vld1_u8(&image[(y + yy) * stride + xx]); \
	sumA = vaddw_u8(sumA, h0); v0 = vmull_u8(h0, h0); \
	sumB = vaddw_u8(sumB, h1); v1 = vmull_u8(h0, h1); \
	sumAA1 = vaddw_u16(sumAA1, vget_low_u16(v0)); \
	sumAB1 = vaddw_u16(sumAB1, vget_low_u16(v1)); \
	sumAA2 = vaddw_u16(sumAA2, vget_high_u16(v0)); \
	sumAB2 = vaddw_u16(sumAB2, vget_high_u16(v1));
#define M2 \
	sumA = vaddq_u16(sumA, sumA); sumB = vaddq_u16(sumB, sumB); \
	sumAA1 = vaddq_u32(sumAA1, sumAA1); sumAA2 = vaddq_u32(sumAA2, sumAA2); \
	sumAB1 = vaddq_u32(sumAB1, sumAB1); sumAB2 = vaddq_u32(sumAB2, sumAB2);
			h0 = vld1_u8(&image2[y * stride]);
			h1 = vld1_u8(&image[y * stride]);
			sumA = vmovl_u8(h0); v0 = vmull_u8(h0, h0);
			sumB = vmovl_u8(h1); v1 = vmull_u8(h0, h1);
			sumAA1 = vmovl_u16(vget_low_u16(v0));
			sumAB1 = vmovl_u16(vget_low_u16(v1));
			sumAA2 = vmovl_u16(vget_high_u16(v0));
			sumAB2 = vmovl_u16(vget_high_u16(v1));
			M2 M1(0, -1) M1(-1, 0) M1(1, 0) M1(0, 1)
			M2 M1(-1, -1) M1(1, -1) M1(-1, 1) M1(1, 1)
#undef M2
#undef M1
			v0 = vmovl_u8(vld1_u8(&image2[y * stride]));
#define M1(low, sumAA, sumAB, x) \
	h2 = vget_##low##_u16(sumA); sumAA = vshlq_n_u32(sumAA, 4); \
	h3 = vget_##low##_u16(sumB); sumAB = vshlq_n_u32(sumAB, 4); \
	sumAA = vmlsl_u16(sumAA, h2, h2); sumAB = vmlsl_u16(sumAB, h2, h3); \
	v4 = vtstq_u32(sumAA, sumAA); \
	sumAB = vandq_u32(sumAB, v4); sumAA = vornq_u32(sumAA, v4); \
	scale = vdivq_f32(vcvtq_f32_s32(vreinterpretq_s32_u32(sumAB)), \
			vcvtq_f32_s32(vreinterpretq_s32_u32(sumAA))); \
	scale = vmaxq_f32(scale, vdupq_n_f32(-16.0f)); \
	scale = vminq_f32(scale, vdupq_n_f32(16.0f)); \
	v4 = vshll_n_u16(vget_##low##_u16(v0), 4); \
	v5 = vcvtq_n_f32_s32(vreinterpretq_s32_u32(vsubw_u16(v4, h2)), 4); \
	v5 = vmlaq_f32(vcvtq_n_f32_u32(vmovl_u16(h3), 4), v5, scale); \
	v5 = vmaxq_f32(v5, vdupq_n_f32(0)); \
	v5 = vsubq_f32(v5, vdupq_n_f32(CENTERJSAMPLE)); \
	v5 = vminq_f32(v5, vdupq_n_f32(CENTERJSAMPLE)); \
	vst1q_f32(fbuf + y * n + x, v5);
			M1(low, sumAA1, sumAB1, 0) M1(high, sumAA2, sumAB2, 4)
#undef M1
		}
#elif 1 && defined(USE_AVX2)
		for (y = 0; y < n; y++) {
			__m128i v0, v1; __m256i v2, v3, v4, sumA, sumB, sumAA, sumAB;
			__m256 v5, scale;
#define M1(x0, y0, x1, y1) \
	v0 = _mm_loadl_epi64((__m128i*)&image2[(y + y0) * stride + x0]); \
	v1 = _mm_loadl_epi64((__m128i*)&image2[(y + y1) * stride + x1]); \
	v2 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi8(v0, v1)); \
	v0 = _mm_loadl_epi64((__m128i*)&image[(y + y0) * stride + x0]); \
	v1 = _mm_loadl_epi64((__m128i*)&image[(y + y1) * stride + x1]); \
	v3 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi8(v0, v1)); \
	sumA = _mm256_add_epi16(sumA, v2); \
	sumB = _mm256_add_epi16(sumB, v3); \
	sumAA = _mm256_add_epi32(sumAA, _mm256_madd_epi16(v2, v2)); \
	sumAB = _mm256_add_epi32(sumAB, _mm256_madd_epi16(v2, v3));
			v0 = _mm_loadl_epi64((__m128i*)&image2[y * stride]);
			v1 = _mm_loadl_epi64((__m128i*)&image[y * stride]);
			sumA = _mm256_cvtepu8_epi16(_mm_unpacklo_epi8(v0, v0));
			sumB = _mm256_cvtepu8_epi16(_mm_unpacklo_epi8(v1, v1));
			sumAA = _mm256_madd_epi16(sumA, sumA);
			sumAB = _mm256_madd_epi16(sumA, sumB);
			M1(0, -1, -1, 0) M1(1, 0, 0, 1)
			sumA = _mm256_add_epi16(sumA, sumA); sumAA = _mm256_add_epi32(sumAA, sumAA);
			sumB = _mm256_add_epi16(sumB, sumB); sumAB = _mm256_add_epi32(sumAB, sumAB);
			M1(-1, -1, 1, -1) M1(-1, 1, 1, 1)
#undef M1
			v3 = _mm256_set1_epi16(1);
			v2 = _mm256_madd_epi16(sumA, v3); sumAA = _mm256_slli_epi32(sumAA, 4);
			v3 = _mm256_madd_epi16(sumB, v3); sumAB = _mm256_slli_epi32(sumAB, 4);
			sumAA = _mm256_sub_epi32(sumAA, _mm256_mullo_epi32(v2, v2));
			sumAB = _mm256_sub_epi32(sumAB, _mm256_mullo_epi32(v2, v3));
			v4 = _mm256_cmpeq_epi32(sumAA, _mm256_setzero_si256());
			sumAB = _mm256_andnot_si256(v4, sumAB);
			scale = _mm256_cvtepi32_ps(_mm256_or_si256(sumAA, v4));
			scale = _mm256_div_ps(_mm256_cvtepi32_ps(sumAB), scale);
			scale = _mm256_max_ps(scale, _mm256_set1_ps(-16.0f));
			scale = _mm256_min_ps(scale, _mm256_set1_ps(16.0f));
			v0 = _mm_loadl_epi64((__m128i*)&image2[y * stride]);
			v4 = _mm256_slli_epi32(_mm256_cvtepu8_epi32(v0), 4);
			v5 = _mm256_cvtepi32_ps(_mm256_sub_epi32(v4, v2));
			// v5 = _mm256_add_ps(_mm256_mul_ps(v5, scale), _mm256_cvtepi32_ps(v3));
			v5 = _mm256_fmadd_ps(v5, scale, _mm256_cvtepi32_ps(v3));
			v5 = _mm256_mul_ps(v5, _mm256_set1_ps(1.0f / 16));
			v5 = _mm256_max_ps(v5, _mm256_setzero_ps());
			v5 = _mm256_sub_ps(v5, _mm256_set1_ps(CENTERJSAMPLE));
			v5 = _mm256_min_ps(v5, _mm256_set1_ps(CENTERJSAMPLE));
			_mm256_storeu_ps(fbuf + y * n, v5);
		}
#elif 1 && defined(USE_SSE2)
		for (y = 0; y < n; y++) {
			__m128i v0, v1, v2, v3, v4, sumA, sumB, sumAA1, sumAB1, sumAA2, sumAB2;
			__m128 v5, scale;
#define M1(x0, y0, x1, y1) \
	v0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)&image2[(y + y0) * stride + x0])); \
	v1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)&image2[(y + y1) * stride + x1])); \
	v2 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)&image[(y + y0) * stride + x0])); \
	v3 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)&image[(y + y1) * stride + x1])); \
	sumA = _mm_add_epi16(_mm_add_epi16(sumA, v0), v1); \
	sumB = _mm_add_epi16(_mm_add_epi16(sumB, v2), v3); \
	v4 = _mm_unpacklo_epi16(v0, v1); sumAA1 = _mm_add_epi32(sumAA1, _mm_madd_epi16(v4, v4)); \
	v1 = _mm_unpackhi_epi16(v0, v1); sumAA2 = _mm_add_epi32(sumAA2, _mm_madd_epi16(v1, v1)); \
	sumAB1 = _mm_add_epi32(sumAB1, _mm_madd_epi16(v4, _mm_unpacklo_epi16(v2, v3))); \
	sumAB2 = _mm_add_epi32(sumAB2, _mm_madd_epi16(v1, _mm_unpackhi_epi16(v2, v3)));
			v0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)&image2[y * stride]));
			v1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)&image[y * stride]));
			v2 = _mm_unpacklo_epi16(v0, v0); sumAA1 = _mm_madd_epi16(v2, v2);
			v3 = _mm_unpacklo_epi16(v1, v1); sumAB1 = _mm_madd_epi16(v2, v3);
			v2 = _mm_unpackhi_epi16(v0, v0); sumAA2 = _mm_madd_epi16(v2, v2);
			v3 = _mm_unpackhi_epi16(v1, v1); sumAB2 = _mm_madd_epi16(v2, v3);
			sumA = _mm_add_epi16(v0, v0); sumB = _mm_add_epi16(v1, v1);
			M1(0, -1, -1, 0) M1(1, 0, 0, 1)
			sumA = _mm_add_epi16(sumA, sumA); sumB = _mm_add_epi16(sumB, sumB);
			sumAA1 = _mm_add_epi32(sumAA1, sumAA1); sumAA2 = _mm_add_epi32(sumAA2, sumAA2);
			sumAB1 = _mm_add_epi32(sumAB1, sumAB1); sumAB2 = _mm_add_epi32(sumAB2, sumAB2);
			M1(-1, -1, 1, -1) M1(-1, 1, 1, 1)
#undef M1
			v0 = _mm_setzero_si128();
			v1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)&image2[y * stride]));
#define M1(lo, sumAA, sumAB, x) \
	v2 = _mm_unpack##lo##_epi16(sumA, v0); sumAA = _mm_slli_epi32(sumAA, 4); \
	v3 = _mm_unpack##lo##_epi16(sumB, v0); sumAB = _mm_slli_epi32(sumAB, 4); \
	sumAA = _mm_sub_epi32(sumAA, _mm_mullo_epi32(v2, v2)); \
	sumAB = _mm_sub_epi32(sumAB, _mm_mullo_epi32(v2, v3)); \
	v4 = _mm_cmpeq_epi32(sumAA, v0); sumAB = _mm_andnot_si128(v4, sumAB); \
	scale = _mm_cvtepi32_ps(_mm_or_si128(sumAA, v4)); \
	scale = _mm_div_ps(_mm_cvtepi32_ps(sumAB), scale); \
	scale = _mm_max_ps(scale, _mm_set1_ps(-16.0f)); \
	scale = _mm_min_ps(scale, _mm_set1_ps(16.0f)); \
	v4 = _mm_slli_epi32(_mm_unpack##lo##_epi16(v1, v0), 4); \
	v5 = _mm_cvtepi32_ps(_mm_sub_epi32(v4, v2)); \
	v5 = _mm_add_ps(_mm_mul_ps(v5, scale), _mm_cvtepi32_ps(v3)); \
	v5 = _mm_mul_ps(v5, _mm_set1_ps(1.0f / 16)); \
	v5 = _mm_max_ps(v5, _mm_setzero_ps()); \
	v5 = _mm_sub_ps(v5, _mm_set1_ps(CENTERJSAMPLE)); \
	v5 = _mm_min_ps(v5, _mm_set1_ps(CENTERJSAMPLE)); \
	_mm_storeu_ps(fbuf + y * n + x, v5);
			M1(lo, sumAA1, sumAB1, 0) M1(hi, sumAA2, sumAB2, 4)
#undef M1
		}
#else
		for (y = 0; y < n; y++)
		for (x = 0; x < n; x++) {
			float sumA = 0, sumB = 0, sumAA = 0, sumAB = 0;
			float divN = 1.0f / 16, scale, offset; float a;
#define M1(xx, yy) { \
	float a = image2[(y + yy) * stride + x + xx]; \
	float b = image[(y + yy) * stride + x + xx]; \
	sumA += a; sumAA += a * a; \
	sumB += b; sumAB += a * b; }
#define M2 sumA += sumA; sumB += sumB; \
	sumAA += sumAA; sumAB += sumAB;
			M1(0, 0) M2
			M1(0, -1) M1(-1, 0) M1(1, 0) M1(0, 1) M2
			M1(-1, -1) M1(1, -1) M1(-1, 1) M1(1, 1)
#undef M2
#undef M1
			scale = sumAA - sumA * divN * sumA;
			if (scale != 0.0f) scale = (sumAB - sumA * divN * sumB) / scale;
			scale = scale < -16.0f ? -16.0f : scale;
			scale = scale > 16.0f ? 16.0f : scale;
			offset = (sumB - scale * sumA) * divN;

			a = image2[y * stride + x] * scale + offset;
			a = a < 0 ? 0 : a > MAXJSAMPLE + 1 ? MAXJSAMPLE + 1 : a;
			fbuf[y * n + x] = a - CENTERJSAMPLE;
		}
#endif
		fdct_clamp(fbuf, coef, quantval);
	}

	if (flags & JPEGQS_LOW_QUALITY) {
		float ALIGN(32) fbuf[DCTSIZE2];
		float range = 0, c0 = 2, c1 = c0 * sqrtf(0.5f);

		if (image2) goto end;
		{
			int sum = 0;
			for (x = 1; x < n * n; x++) {
				int a = coef[x]; a = a < 0 ? -a : a;
				range += quantval[x] * a; sum += a;
			}
			if (sum) range *= 4.0f / sum;
			if (range > CENTERJSAMPLE) range = CENTERJSAMPLE;
			range = roundf(range);
		}

#if 1 && defined(USE_NEON)
		for (y = 0; y < n; y++) {
			int16x8_t v4, v5; uint16x8_t v6 = vdupq_n_u16((int)range);
			float32x2_t f4; uint8x8_t i0, i1;
			float32x4_t f0, f1, s0 = vdupq_n_f32(0), s1 = s0, s2 = s0, s3 = s0;
			f4 = vset_lane_f32(c1, vdup_n_f32(c0), 1);
			i0 = vld1_u8(&image[y * stride]);
#define M1(i, x, y) \
	i1 = vld1_u8(&image[(y) * stride + x]); \
	v4 = vreinterpretq_s16_u16(vsubl_u8(i0, i1)); \
	v5 = vreinterpretq_s16_u16(vqsubq_u16(v6, \
			vreinterpretq_u16_s16(vabsq_s16(v4)))); \
	M2(low, s0, s1, i) M2(high, s2, s3, i)
#define M2(low, s0, s1, i) \
	f0 = vcvtq_f32_s32(vmovl_s16(vget_##low##_s16(v5))); \
	f0 = vmulq_f32(f0, f0); f1 = vmulq_lane_f32(f0, f4, i); \
	f0 = vmulq_f32(f0, vcvtq_f32_s32(vmovl_s16(vget_##low##_s16(v4)))); \
	s0 = vmlaq_f32(s0, f0, f1); s1 = vmlaq_f32(s1, f1, f1);
			M1(1, -1, y-1) M1(0, 0, y-1) M1(1, 1, y-1)
			M1(0, -1, y)                 M1(0, 1, y)
			M1(1, -1, y+1) M1(0, 0, y+1) M1(1, 1, y+1)
#undef M1
#undef M2
			v4 = vreinterpretq_s16_u16(vmovl_u8(i0));
#define M1(low, s0, s1, x) \
	f1 = vbslq_f32(vceqq_f32(s1, vdupq_n_f32(0)), vdupq_n_f32(1.0f), s1); \
	f0 = vdivq_f32(s0, f1); \
	f1 = vcvtq_f32_s32(vmovl_s16(vget_##low##_s16(v4))); \
	f0 = vsubq_f32(f1, f0); \
	f0 = vsubq_f32(f0, vdupq_n_f32(CENTERJSAMPLE)); \
	vst1q_f32(fbuf + y * n + x, f0);
			M1(low, s0, s1, 0) M1(high, s2, s3, 4)
#undef M1
		}
#elif 1 && defined(USE_AVX512)
		for (y = 0; y < n; y += 2) {
			__m256i v0, v1, v4, v5, v6 = _mm256_set1_epi16((int)range);
			__m512 f0, f1, f4, f5, s0 = _mm512_setzero_ps(), s1 = s0; __mmask16 m0;
			f4 = _mm512_set1_ps(c0); f5 = _mm512_set1_ps(c1);
#define M2(v0, pos) \
	v0 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi64( \
			_mm_loadl_epi64((__m128i*)&image[pos]), \
			_mm_loadl_epi64((__m128i*)&image[pos + stride])));
#define M1(f4, x, y) M2(v1, (y) * stride + x) \
	v4 = _mm256_sub_epi16(v0, v1); v5 = _mm256_subs_epu16(v6, _mm256_abs_epi16(v4)); \
	f0 = _mm512_cvtepi32_ps(_mm512_cvtepi16_epi32(v5)); \
	f0 = _mm512_mul_ps(f0, f0); f1 = _mm512_mul_ps(f0, f4); \
	f0 = _mm512_mul_ps(f0, _mm512_cvtepi32_ps(_mm512_cvtepi16_epi32(v4))); \
	s0 = _mm512_fmadd_ps(f0, f1, s0); s1 = _mm512_fmadd_ps(f1, f1, s1);
			M2(v0, y * stride)
			M1(f5, -1, y-1) M1(f4, 0, y-1) M1(f5, 1, y-1)
			M1(f4, -1, y)                  M1(f4, 1, y)
			M1(f5, -1, y+1) M1(f4, 0, y+1) M1(f5, 1, y+1)
#undef M1
#undef M2
			m0 = _mm512_cmp_ps_mask(s1, _mm512_setzero_ps(), 0);
			s1 = _mm512_mask_blend_ps(m0, s1, _mm512_set1_ps(1.0f));
			f0 = _mm512_div_ps(s0, s1);
			f1 = _mm512_cvtepi32_ps(_mm512_cvtepi16_epi32(v0));
			f0 = _mm512_sub_ps(f1, f0);
			f0 = _mm512_sub_ps(f0, _mm512_set1_ps(CENTERJSAMPLE));
			_mm512_storeu_ps(fbuf + y * n, f0);
		}
#elif 1 && defined(USE_AVX2)
		for (y = 0; y < n; y++) {
			__m128i v0, v1, v4, v5, v6 = _mm_set1_epi16((int)range);
			__m256 f0, f1, f4, f5, s0 = _mm256_setzero_ps(), s1 = s0;
			f4 = _mm256_set1_ps(c0); f5 = _mm256_set1_ps(c1);
			v0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)&image[y * stride]));
#define M1(f4, x, y) \
	v1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)&image[(y) * stride + x])); \
	v4 = _mm_sub_epi16(v0, v1); v5 = _mm_subs_epu16(v6, _mm_abs_epi16(v4)); \
	f0 = _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(v5)); \
	f0 = _mm256_mul_ps(f0, f0); f1 = _mm256_mul_ps(f0, f4); \
	f0 = _mm256_mul_ps(f0, _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(v4))); \
	s0 = _mm256_fmadd_ps(f0, f1, s0); s1 = _mm256_fmadd_ps(f1, f1, s1);
			M1(f5, -1, y-1) M1(f4, 0, y-1) M1(f5, 1, y-1)
			M1(f4, -1, y)                  M1(f4, 1, y)
			M1(f5, -1, y+1) M1(f4, 0, y+1) M1(f5, 1, y+1)
#undef M1
			f1 = _mm256_cmp_ps(s1, _mm256_setzero_ps(), 0);
			s1 = _mm256_blendv_ps(s1, _mm256_set1_ps(1.0f), f1);
			f0 = _mm256_div_ps(s0, s1);
			f1 = _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(v0));
			f0 = _mm256_sub_ps(f1, f0);
			f0 = _mm256_sub_ps(f0, _mm256_set1_ps(CENTERJSAMPLE));
			_mm256_storeu_ps(fbuf + y * n, f0);
		}
#elif 1 && defined(USE_SSE2)
		for (y = 0; y < n; y++) {
			__m128i v0, v1, v3, v4, v5, v6 = _mm_set1_epi16((int)range), v7 = _mm_setzero_si128();
			__m128 f0, f1, f4, f5, s0 = _mm_setzero_ps(), s1 = s0, s2 = s0, s3 = s0;
			f4 = _mm_set1_ps(c0); f5 = _mm_set1_ps(c1);
			v0 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)&image[y * stride]));
#define M1(f4, x, y) \
	v1 = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)&image[(y) * stride + x])); \
	v4 = _mm_sub_epi16(v0, v1); v3 = _mm_srai_epi16(v4, 15); \
	v5 = _mm_subs_epu16(v6, _mm_abs_epi16(v4)); \
	M2(lo, s0, s1, f4) M2(hi, s2, s3, f4)
#define M2(lo, s0, s1, f4) \
	f0 = _mm_cvtepi32_ps(_mm_unpack##lo##_epi16(v5, v7)); \
	f0 = _mm_mul_ps(f0, f0); f1 = _mm_mul_ps(f0, f4); \
	f0 = _mm_mul_ps(f0, _mm_cvtepi32_ps(_mm_unpack##lo##_epi16(v4, v3))); \
	f0 = _mm_mul_ps(f0, f1); f1 = _mm_mul_ps(f1, f1); \
	s0 = _mm_add_ps(s0, f0); s1 = _mm_add_ps(s1, f1);
			M1(f5, -1, y-1) M1(f4, 0, y-1) M1(f5, 1, y-1)
			M1(f4, -1, y)                  M1(f4, 1, y)
			M1(f5, -1, y+1) M1(f4, 0, y+1) M1(f5, 1, y+1)
#undef M1
#undef M2
#define M1(lo, s0, s1, x) \
	f1 = _mm_cmpeq_ps(s1, _mm_setzero_ps()); \
	f1 = _mm_and_ps(f1, _mm_set1_ps(1.0f)); \
	f0 = _mm_div_ps(s0, _mm_or_ps(s1, f1)); \
	f1 = _mm_cvtepi32_ps(_mm_unpack##lo##_epi16(v0, v7)); \
	f0 = _mm_sub_ps(f1, f0); \
	f0 = _mm_sub_ps(f0, _mm_set1_ps(CENTERJSAMPLE)); \
	_mm_storeu_ps(fbuf + y * n + x, f0);
			M1(lo, s0, s1, 0) M1(hi, s2, s3, 4)
#undef M1
		}
#else
		for (y = 0; y < n; y++)
		for (x = 0; x < n; x++) {
#define M1(i, x, y) t0 = a - image[(y) * stride + x]; \
	t = range - fabsf(t0); if (t < 0) t = 0; t *= t; aw = c##i * t; \
	a0 += t0 * t * aw; an += aw * aw;
			int a = image[(y)*stride+(x)];
			float a0 = 0, an = 0, aw, t, t0;
			M1(1, x-1, y-1) M1(0, x, y-1) M1(1, x+1, y-1)
			M1(0, x-1, y)                 M1(0, x+1, y)
			M1(1, x-1, y+1) M1(0, x, y+1) M1(1, x+1, y+1)
#undef M1
			if (an > 0.0f) a -= a0 / an;
			fbuf[y * n + x] = a - CENTERJSAMPLE;
		}
#endif
		fdct_clamp(fbuf, coef, quantval);
		goto end;
	}

#if 1 && defined(USE_NEON)
#define VINITD uint8x8_t i0, i1, i2;
#define VDIFF(i) vst1q_u16((uint16_t*)temp + (i) * n, vsubl_u8(i0, i1));
#define VLDPIX(j, p) i##j = vld1_u8(p);
#define VRIGHT(a, b) i##a = vext_u8(i##b, i##b, 1);
#define VCOPY(a, b) i##a = i##b;

#define VINIT \
	int16x8_t v0, v5; uint16x8_t v6 = vdupq_n_u16(range); \
	float32x4_t f0, f1, s0 = vdupq_n_f32(0), s1 = s0, s2 = s0, s3 = s0;

#define VCORE \
	v0 = vld1q_s16(temp + y * n); \
	v5 = vreinterpretq_s16_u16(vqsubq_u16(v6, \
			vreinterpretq_u16_s16(vabsq_s16(v0)))); \
	VCORE1(low, s0, s1, tab) VCORE1(high, s2, s3, tab + 4)

#define VCORE1(low, s0, s1, tab) \
	f0 = vcvtq_f32_s32(vmovl_s16(vget_##low##_s16(v5))); \
	f0 = vmulq_f32(f0, f0); f1 = vmulq_f32(f0, vld1q_f32(tab + y * n)); \
	f0 = vmulq_f32(f0, vcvtq_f32_s32(vmovl_s16(vget_##low##_s16(v0)))); \
	s0 = vmlaq_f32(s0, f0, f1); s1 = vmlaq_f32(s1, f1, f1);

#ifdef __aarch64__
#define VFIN \
	a2 = vaddvq_f32(vaddq_f32(s0, s2)); \
	a3 = vaddvq_f32(vaddq_f32(s1, s3));
#else
#define VFIN { \
	float32x4x2_t p0; float32x2_t v0; \
	p0 = vzipq_f32(vaddq_f32(s0, s2), vaddq_f32(s1, s3)); \
	f0 = vaddq_f32(p0.val[0], p0.val[1]); \
	v0 = vadd_f32(vget_low_f32(f0), vget_high_f32(f0)); \
	a2 = vget_lane_f32(v0, 0); a3 = vget_lane_f32(v0, 1); \
}
#endif

#elif 1 && defined(USE_AVX512)
#define VINCR 2
#define VINIT \
	__m256i v4, v5, v6 = _mm256_set1_epi16(range); \
	__m512 f0, f1, f4, s0 = _mm512_setzero_ps(), s1 = s0;

#define VCORE \
	v4 = _mm256_loadu_si256((__m256i*)&temp[y * n]); \
	f4 = _mm512_load_ps(tab + y * n); \
	v5 = _mm256_subs_epu16(v6, _mm256_abs_epi16(v4)); \
	f0 = _mm512_cvtepi32_ps(_mm512_cvtepi16_epi32(v5)); \
	f0 = _mm512_mul_ps(f0, f0); f1 = _mm512_mul_ps(f0, f4); \
	f0 = _mm512_mul_ps(f0, _mm512_cvtepi32_ps(_mm512_cvtepi16_epi32(v4))); \
	s0 = _mm512_fmadd_ps(f0, f1, s0); s1 = _mm512_fmadd_ps(f1, f1, s1);

// "reduce_add" is not faster here, because it's a macro, not a single instruction
// a2 = _mm512_reduce_add_ps(s0); a3 = _mm512_reduce_add_ps(s1);
#define VFIN { __m256 s2, s3, f2; \
	f0 = _mm512_shuffle_f32x4(s0, s1, 0x44); \
	f1 = _mm512_shuffle_f32x4(s0, s1, 0xee); \
	f0 = _mm512_add_ps(f0, f1); s2 = _mm512_castps512_ps256(f0); \
	s3 = _mm256_castpd_ps(_mm512_extractf64x4_pd(_mm512_castps_pd(f0), 1)); \
	f2 = _mm256_permute2f128_ps(s2, s3, 0x20); \
	f2 = _mm256_add_ps(f2, _mm256_permute2f128_ps(s2, s3, 0x31)); \
	f2 = _mm256_add_ps(f2, _mm256_shuffle_ps(f2, f2, 0xee)); \
	f2 = _mm256_add_ps(f2, _mm256_shuffle_ps(f2, f2, 0x55)); \
	a2 = _mm256_cvtss_f32(f2); \
	a3 = _mm_cvtss_f32(_mm256_extractf128_ps(f2, 1)); }

#elif 1 && defined(USE_AVX2)
#define VINIT \
	__m128i v4, v5, v6 = _mm_set1_epi16(range); \
	__m256 f0, f1, f4, s0 = _mm256_setzero_ps(), s1 = s0;

#define VCORE \
	v4 = _mm_loadu_si128((__m128i*)&temp[y * n]); \
	f4 = _mm256_load_ps(tab + y * n); \
	v5 = _mm_subs_epu16(v6, _mm_abs_epi16(v4)); \
	f0 = _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(v5)); \
	f0 = _mm256_mul_ps(f0, f0); f1 = _mm256_mul_ps(f0, f4); \
	f0 = _mm256_mul_ps(f0, _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(v4))); \
	s0 = _mm256_fmadd_ps(f0, f1, s0); s1 = _mm256_fmadd_ps(f1, f1, s1);

#define VFIN \
	f0 = _mm256_permute2f128_ps(s0, s1, 0x20); \
	f1 = _mm256_permute2f128_ps(s0, s1, 0x31); \
	f0 = _mm256_add_ps(f0, f1); \
	f0 = _mm256_add_ps(f0, _mm256_shuffle_ps(f0, f0, 0xee)); \
	f0 = _mm256_add_ps(f0, _mm256_shuffle_ps(f0, f0, 0x55)); \
	a2 = _mm256_cvtss_f32(f0); \
	a3 = _mm_cvtss_f32(_mm256_extractf128_ps(f0, 1));

#elif 1 && defined(USE_SSE2)
#define VINIT \
	__m128i v3, v4, v5, v6 = _mm_set1_epi16(range), v7 = _mm_setzero_si128(); \
	__m128 f0, f1, s0 = _mm_setzero_ps(), s1 = s0, s2 = s0, s3 = s0;

#define VCORE \
	v4 = _mm_loadu_si128((__m128i*)&temp[y * n]); \
	v3 = _mm_srai_epi16(v4, 15); \
	v5 = _mm_subs_epu16(v6, _mm_abs_epi16(v4)); \
	VCORE1(lo, s0, s1, tab) VCORE1(hi, s2, s3, tab + 4)

#define VCORE1(lo, s0, s1, tab) \
	f0 = _mm_cvtepi32_ps(_mm_unpack##lo##_epi16(v5, v7)); \
	f0 = _mm_mul_ps(f0, f0); \
	f1 = _mm_mul_ps(f0, _mm_load_ps(tab + y * n)); \
	f0 = _mm_mul_ps(f0, _mm_cvtepi32_ps(_mm_unpack##lo##_epi16(v4, v3))); \
	f0 = _mm_mul_ps(f0, f1); f1 = _mm_mul_ps(f1, f1); \
	s0 = _mm_add_ps(s0, f0); s1 = _mm_add_ps(s1, f1);

#define VFIN \
	f0 = _mm_add_ps(s0, s2); f1 = _mm_add_ps(s1, s3); \
	f0 = _mm_add_ps(_mm_unpacklo_ps(f0, f1), _mm_unpackhi_ps(f0, f1)); \
	f0 = _mm_add_ps(f0, _mm_shuffle_ps(f0, f0, 0xee)); \
	a2 = _mm_cvtss_f32(f0); \
	a3 = _mm_cvtss_f32(_mm_shuffle_ps(f0, f0, 0x55));

#elif !defined(NO_SIMD) // vector code simulation
#define VINITD JSAMPLE *p0, *p1, *p2;
#define VDIFF(i) for (x = 0; x < n; x++) temp[(i) * n + x] = p0[x] - p1[x];
#define VLDPIX(i, a) p##i = a;
#define VRIGHT(a, b) p##a = p##b + 1;
#define VCOPY(a, b) p##a = p##b;

#define VINIT int j; float a0, a1, f0, sum[DCTSIZE * 2]; \
	for (j = 0; j < n * 2; j++) sum[j] = 0;

#define VCORE \
	for (j = 0; j < n; j++) { \
		a0 = temp[y * n + j]; a1 = tab[y * n + j]; \
		f0 = (float)range - fabsf(a0); if (f0 < 0) f0 = 0; f0 *= f0; \
		a0 *= f0; a1 *= f0; a0 *= a1; a1 *= a1; \
		sum[j] += a0; sum[j + n] += a1; \
	}

#define VCORE1(sum) \
	((sum[0] + sum[4]) + (sum[1] + sum[5])) + \
	((sum[2] + sum[6]) + (sum[3] + sum[7]));
#define VFIN a2 += VCORE1(sum) a3 += VCORE1((sum+8))
#endif

	for (y = 0; y < n; y++) {
		border[y + n * 2] = image[y - stride];
		border[y + n * 3] = image[y + stride * n];
		border[y + n * 4] = image[y * stride - 1];
		border[y + n * 5] = image[y * stride + n];
	}

	for (k = n * n - 1; k > 0; k--) {
		int i = jpegqs_natural_order[k];
		float *tab = tables[i], a2 = 0, a3 = 0;
		int range = quantval[i] * 2;
		if (need_refresh && zigzag_refresh[i]) {
			idct_islow(coef, buf, n);
			need_refresh = 0;
#ifdef VINIT
			for (y = 0; y < n; y++) {
				border[y] = buf[y * n];
				border[y + n] = buf[y * n + n - 1];
			}

#ifndef VINITD
// same for SSE2, AVX2, AVX512
#define VINITD __m128i v0, v1, v2;
#define VDIFF(i) _mm_storeu_si128((__m128i*)&temp[(i) * n], _mm_sub_epi16(v0, v1));
#define VLDPIX(i, p) v##i = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(p)));
#define VRIGHT(a, b) v##a = _mm_bsrli_si128(v##b, 2);
#define VCOPY(a, b) v##a = v##b;
#endif

			{
				VINITD
				VLDPIX(0, buf)
				VLDPIX(1, border + n * 2)
				VDIFF(n)
				VRIGHT(1, 0) VDIFF(0)
				for (y = 1; y < n; y++) {
					VLDPIX(1, buf + y * n)
					VDIFF(y + n + 3)
					VCOPY(0, 1)
					VRIGHT(1, 0) VDIFF(y)
				}
				VLDPIX(1, border + n * 3)
				VDIFF(n + 1)
				VLDPIX(0, border)
				VLDPIX(1, border + n * 4)
				VDIFF(n + 2)
				VLDPIX(0, border + n)
				VLDPIX(1, border + n * 5)
				VDIFF(n + 3)

				if (flags & JPEGQS_DIAGONALS) {
					VLDPIX(0, buf)
					for (y = 0; y < n - 1; y++) {
						VLDPIX(2, buf + y * n + n)
						VRIGHT(1, 2)
						VDIFF(n * 2 + 4 + y * 2)
						VRIGHT(0, 0)
						VCOPY(1, 2)
						VDIFF(n * 2 + 4 + y * 2 + 1)
						VCOPY(0, 2)
					}
				}
			}
#undef VINITD
#undef VLDPIX
#undef VRIGHT
#undef VCOPY
#undef VDIFF
#endif
		}

#ifdef VINIT
#ifndef VINCR
#define VINCR 1
#endif
		{
			int y0 = i & (n - 1) ? 0 : n;
			int y1 = (i >= n ? n - 1 : 0) + n + 4;
			VINIT

			for (y = y0; y < y1; y += VINCR) { VCORE }
			if (flags & JPEGQS_DIAGONALS) {
				y0 = n * 2 + 4; y1 = y0 + (n - 1) * 2;
				for (y = y0; y < y1; y += VINCR) { VCORE }
			}

			VFIN
		}

#undef VINCR
#undef VINIT
#undef VCORE
#ifdef VCORE1
#undef VCORE1
#endif
#undef VFIN
#else
		{
			int p; float a0, a1, t;
#define CORE t = (float)range - fabsf(a0); \
	if (t < 0) t = 0; t *= t; a0 *= t; a1 *= t; a2 += a0 * a1; a3 += a1 * a1;
#define M1(a, b) \
	for (y = 0; y < n - 1 + a; y++) \
	for (x = 0; x < n - 1 + b; x++) { p = y * n + x; \
	a0 = buf[p] - buf[(y + b) * n + x + a]; a1 = tab[p]; CORE }
#define M2(z, i) for (z = 0; z < n; z++) { p = y * n + x; \
	a0 = buf[p] - border[i * n + z]; a1 = *tab++; CORE }
			if (i & (n - 1)) M1(1, 0) tab += n * n;
			y = 0; M2(x, 2) y = n - 1; M2(x, 3)
			x = 0; M2(y, 4) x = n - 1; M2(y, 5)
			if (i > (n - 1)) M1(0, 1)

			if (flags & JPEGQS_DIAGONALS) {
				tab += n * n;
				for (y = 0; y < n - 1; y++, tab += n * 2)
				for (x = 0; x < n - 1; x++) {
					p = y * n + x;
					a0 = buf[p] - buf[p + n + 1]; a1 = tab[x]; CORE
					a0 = buf[p + 1] - buf[p + n]; a1 = tab[x + n]; CORE
				}
			}
#undef M2
#undef M1
#undef CORE
		}
#endif

		a2 = a2 / a3;
		range = roundf(a2);

		if (range) {
			int div = quantval[i], coef1 = coef[i], add;
			int dh, dl, d0 = (div - 1) >> 1, d1 = div >> 1;
			int a0 = (coef1 + (coef1 < 0 ? -d1 : d1)) / div * div;

			dh = a0 + (a0 < 0 ? d1 : d0);
			dl = a0 - (a0 > 0 ? d1 : d0);

			add = coef1 - range;
			if (add > dh) add = dh;
			if (add < dl) add = dl;
			coef[i] = add;
			need_refresh |= add ^ coef1;
		}
	}
end:
	if (flags & JPEGQS_NO_REBALANCE) return;
	if (!luma && flags & JPEGQS_NO_REBALANCE_UV) return;
#if 1 && defined(USE_NEON)
	if (sizeof(quantval[0]) == 2 && sizeof(quantval[0]) == sizeof(coef[0])) {
		JCOEF orig[DCTSIZE2]; int coef0 = coef[0];
		int32_t m0, m1; int32x4_t s0 = vdupq_n_s32(0), s1 = s0;
		coef[0] = 0;
		for (k = 0; k < DCTSIZE2; k += 8) {
			int16x8_t v0, v1, v2, v3; float32x4_t f0, f3, f4, f5; int32x4_t v4;
			v1 = vld1q_s16((int16_t*)&quantval[k]);
			v0 = vld1q_s16((int16_t*)&coef[k]); v3 = vshrq_n_s16(v0, 15);
			v2 = veorq_s16(vaddq_s16(vshrq_n_s16(v1, 1), v3), v3);
			v2 = vaddq_s16(v0, v2); f3 = vdupq_n_f32(0.5f); f5 = vnegq_f32(f3);
#define M1(low, f0) \
	v4 = vmovl_s16(vget_##low##_s16(v2)); \
	f0 = vbslq_f32(vreinterpretq_u32_s32(vshrq_n_s32(v4, 31)), f5, f3); \
	f4 = vcvtq_f32_s32(vmovl_s16(vget_##low##_s16(v1))); \
	f0 = vdivq_f32(vaddq_f32(vcvtq_f32_s32(v4), f0), f4);
			M1(low, f0) M1(high, f3)
#undef M1
			v2 = vcombine_s16(vmovn_s32(vcvtq_s32_f32(f0)), vmovn_s32(vcvtq_s32_f32(f3)));
			v2 = vmulq_s16(v2, v1);
			vst1q_s16((int16_t*)&orig[k], v2);
#define M1(low) \
	s0 = vmlal_s16(s0, vget_##low##_s16(v0), vget_##low##_s16(v2)); \
	s1 = vmlal_s16(s1, vget_##low##_s16(v2), vget_##low##_s16(v2));
			M1(low) M1(high)
#undef M1
		}
		{
#ifdef __aarch64__
			m0 = vaddvq_s32(s0); m1 = vaddvq_s32(s1);
#else
			int32x4x2_t v0 = vzipq_s32(s0, s1); int32x2_t v1;
			s0 = vaddq_s32(v0.val[0], v0.val[1]);
			v1 = vadd_s32(vget_low_s32(s0), vget_high_s32(s0));
			m0 = vget_lane_s32(v1, 0); m1 = vget_lane_s32(v1, 1);
#endif
		}
		if (m1 > m0) {
			int mul = (((int64_t)m1 << 13) + (m0 >> 1)) / m0;
			int16x8_t v4 = vdupq_n_s16(mul);
			for (k = 0; k < DCTSIZE2; k += 8) {
				int16x8_t v0, v1, v2, v3;
				v1 = vld1q_s16((int16_t*)&quantval[k]);
				v2 = vld1q_s16((int16_t*)&coef[k]);
				v2 = vqrdmulhq_s16(vshlq_n_s16(v2, 2), v4);
				v0 = vld1q_s16((int16_t*)&orig[k]);
				v3 = vaddq_s16(v1, vreinterpretq_s16_u16(vcgeq_s16(v0, vdupq_n_s16(0))));
				v2 = vminq_s16(v2, vaddq_s16(v0, vshrq_n_s16(v3, 1)));
				v3 = vaddq_s16(v1, vreinterpretq_s16_u16(vcleq_s16(v0, vdupq_n_s16(0))));
				v2 = vmaxq_s16(v2, vsubq_s16(v0, vshrq_n_s16(v3, 1)));
				vst1q_s16((int16_t*)&coef[k], v2);
			}
		}
		coef[0] = coef0;
	} else
#elif 1 && defined(USE_AVX2)
	if (sizeof(quantval[0]) == 2 && sizeof(quantval[0]) == sizeof(coef[0])) {
		JCOEF orig[DCTSIZE2]; int coef0 = coef[0];
		int32_t m0, m1; __m128i s0 = _mm_setzero_si128(), s1 = s0;
		coef[0] = 0;
		for (k = 0; k < DCTSIZE2; k += 8) {
			__m128i v0, v1, v2, v3; __m256i v4; __m256 f0;
			v1 = _mm_loadu_si128((__m128i*)&quantval[k]);
			v0 = _mm_loadu_si128((__m128i*)&coef[k]);
			v2 = _mm_srli_epi16(v1, 1); v3 = _mm_srai_epi16(v0, 15);
			v2 = _mm_xor_si128(_mm_add_epi16(v2, v3), v3);
			v2 = _mm_add_epi16(v0, v2);
			f0 = _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(v2));
			f0 = _mm256_div_ps(f0, _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(v1)));
			v4 = _mm256_cvttps_epi32(f0);
			v2 = _mm_packs_epi32(_mm256_castsi256_si128(v4), _mm256_extractf128_si256(v4, 1));
			v2 = _mm_mullo_epi16(v2, v1);
			_mm_storeu_si128((__m128i*)&orig[k], v2);
			s0 = _mm_add_epi32(s0, _mm_madd_epi16(v0, v2));
			s1 = _mm_add_epi32(s1, _mm_madd_epi16(v2, v2));
		}
		s0 = _mm_hadd_epi32(s0, s1); s0 = _mm_hadd_epi32(s0, s0);
		m0 = _mm_cvtsi128_si32(s0); m1 = _mm_extract_epi32(s0, 1);
		if (m1 > m0) {
			int mul = (((int64_t)m1 << 13) + (m0 >> 1)) / m0;
			__m256i v4 = _mm256_set1_epi16(mul);
			for (k = 0; k < DCTSIZE2; k += 16) {
				__m256i v0, v1, v2, v3;
				v1 = _mm256_loadu_si256((__m256i*)&quantval[k]);
				v2 = _mm256_loadu_si256((__m256i*)&coef[k]);
				v2 = _mm256_mulhrs_epi16(_mm256_slli_epi16(v2, 2), v4);
				v0 = _mm256_loadu_si256((__m256i*)&orig[k]);
				v1 = _mm256_add_epi16(v1, _mm256_set1_epi16(-1));
				v3 = _mm256_sub_epi16(v1, _mm256_srai_epi16(v0, 15));
				v2 = _mm256_min_epi16(v2, _mm256_add_epi16(v0, _mm256_srai_epi16(v3, 1)));
				v3 = _mm256_sub_epi16(v1, _mm256_cmpgt_epi16(v0, _mm256_setzero_si256()));
				v2 = _mm256_max_epi16(v2, _mm256_sub_epi16(v0, _mm256_srai_epi16(v3, 1)));
				_mm256_storeu_si256((__m256i*)&coef[k], v2);
			}
		}
		coef[0] = coef0;
	} else
#elif 1 && defined(USE_SSE2)
	if (sizeof(quantval[0]) == 2 && sizeof(quantval[0]) == sizeof(coef[0])) {
		JCOEF orig[DCTSIZE2]; int coef0 = coef[0];
		int32_t m0, m1; __m128i s0 = _mm_setzero_si128(), s1 = s0;
		coef[0] = 0;
		for (k = 0; k < DCTSIZE2; k += 8) {
			__m128i v0, v1, v2, v3, v7; __m128 f0, f2, f4;
			v1 = _mm_loadu_si128((__m128i*)&quantval[k]);
			v0 = _mm_loadu_si128((__m128i*)&coef[k]);
			v2 = _mm_srli_epi16(v1, 1); v3 = _mm_srai_epi16(v0, 15);
			v2 = _mm_xor_si128(_mm_add_epi16(v2, v3), v3);
			v2 = _mm_add_epi16(v0, v2);
			v7 = _mm_setzero_si128(); v3 = _mm_srai_epi16(v2, 15);
#define M1(lo, f0) \
	f4 = _mm_cvtepi32_ps(_mm_unpack##lo##_epi16(v1, v7)); \
	f0 = _mm_cvtepi32_ps(_mm_unpack##lo##_epi16(v2, v3)); \
	f0 = _mm_div_ps(f0, f4);
		M1(lo, f0) M1(hi, f2)
#undef M1
			v2 = _mm_packs_epi32(_mm_cvttps_epi32(f0), _mm_cvttps_epi32(f2));
			v2 = _mm_mullo_epi16(v2, v1);
			_mm_storeu_si128((__m128i*)&orig[k], v2);
			s0 = _mm_add_epi32(s0, _mm_madd_epi16(v0, v2));
			s1 = _mm_add_epi32(s1, _mm_madd_epi16(v2, v2));
		}
#ifdef USE_SSE4
		s0 = _mm_hadd_epi32(s0, s1); s0 = _mm_hadd_epi32(s0, s0);
		m0 = _mm_cvtsi128_si32(s0); m1 = _mm_extract_epi32(s0, 1);
#else
		s0 = _mm_add_epi32(_mm_unpacklo_epi32(s0, s1), _mm_unpackhi_epi32(s0, s1));
		s0 = _mm_add_epi32(s0, _mm_bsrli_si128(s0, 8));
		m0 = _mm_cvtsi128_si32(s0); m1 = _mm_cvtsi128_si32(_mm_bsrli_si128(s0, 4));
#endif
		if (m1 > m0) {
			int mul = (((int64_t)m1 << 13) + (m0 >> 1)) / m0;
			__m128i v4 = _mm_set1_epi16(mul);
			for (k = 0; k < DCTSIZE2; k += 8) {
				__m128i v0, v1, v2, v3 = _mm_set1_epi16(-1);
				v1 = _mm_loadu_si128((__m128i*)&quantval[k]);
				v2 = _mm_loadu_si128((__m128i*)&coef[k]);
#ifdef USE_SSE4
				v2 = _mm_mulhrs_epi16(_mm_slli_epi16(v2, 2), v4);
#else
				v2 = _mm_mulhi_epi16(_mm_slli_epi16(v2, 4), v4);
				v2 = _mm_srai_epi16(_mm_sub_epi16(v2, v3), 1);
#endif
				v0 = _mm_loadu_si128((__m128i*)&orig[k]);
				v1 = _mm_add_epi16(v1, v3);
				v3 = _mm_sub_epi16(v1, _mm_srai_epi16(v0, 15));
				v2 = _mm_min_epi16(v2, _mm_add_epi16(v0, _mm_srai_epi16(v3, 1)));
				v3 = _mm_sub_epi16(v1, _mm_cmpgt_epi16(v0, _mm_setzero_si128()));
				v2 = _mm_max_epi16(v2, _mm_sub_epi16(v0, _mm_srai_epi16(v3, 1)));
				_mm_storeu_si128((__m128i*)&coef[k], v2);
			}
		}
		coef[0] = coef0;
	} else
#endif
	{
		JCOEF orig[DCTSIZE2];
		int64_t m0 = 0, m1 = 0;
		for (k = 1; k < DCTSIZE2; k++) {
			int div = quantval[k], coef1 = coef[k], d1 = div >> 1;
			int a0 = (coef1 + (coef1 < 0 ? -d1 : d1)) / div * div;
			orig[k] = a0;
			m0 += coef1 * a0; m1 += a0 * a0;
		}
		if (m1 > m0) {
			int mul = ((m1 << 13) + (m0 >> 1)) / m0;
			for (k = 1; k < DCTSIZE2; k++) {
				int div = quantval[k], coef1 = coef[k], add;
				int dh, dl, d0 = (div - 1) >> 1, d1 = div >> 1;
				int a0 = orig[k];

				dh = a0 + (a0 < 0 ? d1 : d0);
				dl = a0 - (a0 > 0 ? d1 : d0);

				add = (coef1 * mul + 0x1000) >> 13;
				if (add > dh) add = dh;
				if (add < dl) add = dl;
				coef[k] = add;
			}
		}
	}
}