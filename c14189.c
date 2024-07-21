bool SNC_io_parser<EW>::
read_facet(Halffacet_handle fh) {

  bool OK = true;
  int index;
  char cc;
#ifdef CGAL_NEF_NATURAL_COORDINATE_INPUT
  typename K::RT a,b,c,d;
#endif

  in >> index;
  OK = OK && test_string("{");

  in >> index;
  fh->twin() = Halffacet_of[index];
  OK = OK && test_string(",");

  in >> cc;
  while(isdigit(cc)) {
    in.putback(cc);
    in >> index;
    fh->boundary_entry_objects().push_back(make_object(SEdge_of[index]));
    in >> cc;
  }

  in >> cc;
  while(isdigit(cc)) {
    in.putback(cc);
    in >> index;
    fh->boundary_entry_objects().push_back(make_object(SLoop_of[index]));
    in >> cc;
  }

  in >> index;
  fh->incident_volume() = Volume_of[index+addInfiBox];
  OK = OK && test_string("|");
#ifdef CGAL_NEF_NATURAL_COORDINATE_INPUT
  in >> a >> b >> c >> d;
  fh->plane() = Plane_3(a,b,c,d);
#else
  fh->plane() =
    Geometry_io<typename K::Kernel_tag, Kernel>::
    template read_plane<Kernel, K>(in);
#endif
  OK = OK && test_string("}");
  in >> fh->mark();

  return OK;
}