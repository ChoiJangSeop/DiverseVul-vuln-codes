bool SNC_io_parser<EW>::
read_vertex(Vertex_handle vh) {

  bool OK = true;
  int index;
  #ifdef CGAL_NEF_NATURAL_COORDINATE_INPUT
  typename K::RT hx, hy, hz, hw;
  #endif

  in >> index;
  OK = OK && test_string("{");
  vh->sncp() = this->sncp();

  in >> index;
  vh->svertices_begin() = (index >= 0 ? Edge_of[index] : this->svertices_end());
  in >> index;
  vh->svertices_last()  = index >= 0 ? Edge_of[index] : this->svertices_end();
  OK = OK && test_string(",");
  in >> index;
  vh->shalfedges_begin() = index >= 0 ? SEdge_of[index] : this->shalfedges_end();
  in >> index;
  vh->shalfedges_last()  = index >= 0 ? SEdge_of[index] : this->shalfedges_end();
  OK = OK && test_string(",");
  in >> index;
  vh->sfaces_begin() = index >= 0 ? SFace_of[index] : this->sfaces_end();
  in >> index;
  vh->sfaces_last()  = index >= 0 ? SFace_of[index] : this->sfaces_end();
  OK = OK && test_string(",");
  in >> index;
  vh->shalfloop() = index >= 0 ? SLoop_of[index] : this->shalfloops_end();
  OK = OK && test_string("|");
#ifdef CGAL_NEF_NATURAL_COORDINATE_INPUT
  in >> hx >> hy >> hz >> hw;
  vh->point() = Point_3(hx,hy,hz,hw);
#else
  vh->point() =
    Geometry_io<typename K::Kernel_tag, Kernel>::template read_point<Kernel, K>(in);
#endif
  OK = OK && test_string("}");
  in >> vh->mark();

  return OK;
}