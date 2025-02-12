void jas_matrix_bindsub(jas_matrix_t *mat0, jas_matrix_t *mat1, int r0, int c0,
  int r1, int c1)
{
	int i;

	if (mat0->data_) {
		if (!(mat0->flags_ & JAS_MATRIX_REF)) {
			jas_free(mat0->data_);
		}
		mat0->data_ = 0;
		mat0->datasize_ = 0;
	}
	if (mat0->rows_) {
		jas_free(mat0->rows_);
		mat0->rows_ = 0;
	}
	mat0->flags_ |= JAS_MATRIX_REF;
	mat0->numrows_ = r1 - r0 + 1;
	mat0->numcols_ = c1 - c0 + 1;
	mat0->maxrows_ = mat0->numrows_;
	mat0->rows_ = jas_malloc(mat0->maxrows_ * sizeof(jas_seqent_t *));
	for (i = 0; i < mat0->numrows_; ++i) {
		mat0->rows_[i] = mat1->rows_[r0 + i] + c0;
	}

	mat0->xstart_ = mat1->xstart_ + c0;
	mat0->ystart_ = mat1->ystart_ + r0;
	mat0->xend_ = mat0->xstart_ + mat0->numcols_;
	mat0->yend_ = mat0->ystart_ + mat0->numrows_;
}