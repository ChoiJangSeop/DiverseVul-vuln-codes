gdImagePtr gdImageRotateInterpolated(const gdImagePtr src, const float angle, int bgcolor)
{
	const int angle_rounded = (int)floor(angle * 100);

	if (bgcolor < 0 || bgcolor >= gdMaxColors) {
		return NULL;
	}

	/* impact perf a bit, but not that much. Implementation for palette
	   images can be done at a later point.
	*/
	if (src->trueColor == 0) {
		if (bgcolor >= 0) {
			bgcolor =  gdTrueColorAlpha(src->red[bgcolor], src->green[bgcolor], src->blue[bgcolor], src->alpha[bgcolor]);
		}
		gdImagePaletteToTrueColor(src);
	}

	/* no interpolation needed here */
	switch (angle_rounded) {
		case 9000:
			return gdImageRotate90(src, 0);
		case 18000:
			return gdImageRotate180(src, 0);
		case 27000:
			return gdImageRotate270(src, 0);
	}

	if (src == NULL || src->interpolation_id < 1 || src->interpolation_id > GD_METHOD_COUNT) {
		return NULL;
	}

	switch (src->interpolation_id) {
		case GD_NEAREST_NEIGHBOUR:
			return gdImageRotateNearestNeighbour(src, angle, bgcolor);
			break;

		case GD_BILINEAR_FIXED:
			return gdImageRotateBilinear(src, angle, bgcolor);
			break;

		case GD_BICUBIC_FIXED:
			return gdImageRotateBicubicFixed(src, angle, bgcolor);
			break;

		default:
			return gdImageRotateGeneric(src, angle, bgcolor);
	}
	return NULL;
}