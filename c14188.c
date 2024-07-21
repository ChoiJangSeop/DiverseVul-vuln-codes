bool SNC_io_parser<EW>::
read_sedge(SHalfedge_handle seh) {

  bool OK = true;
  int index;
#ifdef CGAL_NEF_NATURAL_COORDINATE_INPUT
  typename K::RT a,b,c,d;
#endif

  in >> index;
  OK = OK && test_string("{");

  in >> index;
  seh->twin() = SEdge_of[index];
  OK = OK && test_string(",");
  in >> index;
  seh->sprev() = SEdge_of[index];
  OK = OK && test_string(",");
  in >> index;
  seh->snext() = SEdge_of[index];
  OK = OK && test_string(",");
  in >> index;
  seh->source() = Edge_of[index];
  OK = OK && test_string(",");
  in >> index;
  seh->incident_sface() = SFace_of[index];
  OK = OK && test_string(",");
  in >> index;
  seh->prev() = SEdge_of[index];
  OK = OK && test_string(",");
  in >> index;
  seh->next() = SEdge_of[index];
  OK = OK && test_string(",");
  in >> index;
  seh->facet() = Halffacet_of[index];
  OK = OK && test_string("|");
#ifdef CGAL_NEF_NATURAL_COORDINATE_INPUT
  in >> a >> b >> c >> d;
  seh->circle() = Sphere_circle(Plane_3(a,b,c,d));
#else
  seh->circle() =
    Geometry_io<typename K::Kernel_tag, Kernel>::
    template read_plane<Kernel, K>(in);
#endif
  OK = OK && test_string("}");
  in >> seh->mark();

  return OK;
}