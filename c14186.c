bool SNC_io_parser<EW>::
read_sloop(SHalfloop_handle slh) {

  bool OK = true;
  int index;
#ifdef CGAL_NEF_NATURAL_COORDINATE_INPUT
  typename K::RT a,b,c,d;
#endif

  in >> index;
  OK = OK && test_string("{");

  in >> index;
  slh->twin() = SLoop_of[index];
  OK = OK && test_string(",");
  in >> index;
  slh->incident_sface() = SFace_of[index];
  OK = OK && test_string(",");
  in >> index;
  slh->facet() = Halffacet_of[index];
  OK = OK && test_string("|");
#ifdef CGAL_NEF_NATURAL_COORDINATE_INPUT
  in >> a >> b >> c >> d;
  slh->circle() = Sphere_circle(Plane_3(a,b,c,d));
#else
  slh->circle() =
    Geometry_io<typename K::Kernel_tag, Kernel>::
    template read_plane<Kernel, K>(in);
#endif
  OK = OK && test_string("}");
  in >> slh->mark();

  return OK;
}