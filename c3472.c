void jas_matrix_setall(jas_matrix_t *matrix, jas_seqent_t val)
{
	int i;
	int j;
	jas_seqent_t *rowstart;
	int rowstep;
	jas_seqent_t *data;

	rowstep = jas_matrix_rowstep(matrix);
	for (i = matrix->numrows_, rowstart = matrix->rows_[0]; i > 0; --i,
	  rowstart += rowstep) {
		for (j = matrix->numcols_, data = rowstart; j > 0; --j,
		  ++data) {
			*data = val;
		}
	}
}