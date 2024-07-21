static int jas_image_growcmpts(jas_image_t *image, int maxcmpts)
{
	jas_image_cmpt_t **newcmpts;
	int cmptno;

	newcmpts = (!image->cmpts_) ? jas_malloc(maxcmpts * sizeof(jas_image_cmpt_t *)) :
	  jas_realloc(image->cmpts_, maxcmpts * sizeof(jas_image_cmpt_t *));
	if (!newcmpts) {
		return -1;
	}
	image->cmpts_ = newcmpts;
	image->maxcmpts_ = maxcmpts;
	for (cmptno = image->numcmpts_; cmptno < image->maxcmpts_; ++cmptno) {
		image->cmpts_[cmptno] = 0;
	}
	return 0;
}