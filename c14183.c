bool SNC_io_parser<EW>::
read_edge(Halfedge_handle eh) {

  bool OK = true;
  int index;
#ifdef CGAL_NEF_NATURAL_COORDINATE_INPUT
  typename K::RT hx,hy,hz,hw;
#endif
  in >> index;
  OK = OK && test_string("{");

  in >> index;
  eh->twin() = Edge_of[index];
  OK = OK && test_string(",");
  in >> index;
  eh->center_vertex() = Vertex_of[index];
  OK = OK && test_string(",");
  in >> index;
  if(index == 0) {
    in >> index;
    eh->out_sedge() = SEdge_of[index];
  } else {
    in >> index;
    eh->incident_sface() = SFace_of[index];
  }
  OK = OK && test_string("|");
#ifdef CGAL_NEF_NATURAL_COORDINATE_INPUT
  in >> hx >> hy >> hz >> hw;
  eh->point() = Sphere_point(hx,hy,hz);
#else
  eh->point() =
    Geometry_io<typename K::Kernel_tag, Kernel>::template read_point<Kernel,K>(in);
#endif
  OK = OK && test_string("}");
  in >> eh->mark();

  return OK;
}