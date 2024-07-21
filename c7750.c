void del_aubio_onset (aubio_onset_t *o)
{
  del_aubio_spectral_whitening(o->spectral_whitening);
  del_aubio_specdesc(o->od);
  del_aubio_peakpicker(o->pp);
  del_aubio_pvoc(o->pv);
  del_fvec(o->desc);
  del_cvec(o->fftgrain);
  AUBIO_FREE(o);
}