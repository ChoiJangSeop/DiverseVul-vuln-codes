void SNC_io_parser<EW>::read_items(int plus01) {

  typename std::vector<Vertex_iterator>::iterator vi;
  for(vi=Vertex_of.begin(); vi!=Vertex_of.end(); ++vi) {
    if (!read_vertex<K>(*vi))
      CGAL_error_msg("SNC_io_parser::read: error in node line");
  }

  typename std::vector<Halfedge_iterator>::iterator ei;
  for(ei=Edge_of.begin(); ei!=Edge_of.end(); ++ei) {
    if (!read_edge<K>(*ei))
      CGAL_error_msg("SNC_io_parser::read: error in edge line");
  }

  typedef typename std::vector<Halffacet_iterator>::iterator vhf_iterator;
  vhf_iterator fi;
  for(fi=Halffacet_of.begin(); fi!=Halffacet_of.end(); ++fi) {
    if (!read_facet<K>(*fi))
      CGAL_error_msg("SNC_io_parser::read: error in facet line");
  }
  typename std::vector<Volume_iterator>::iterator ci;
  for(ci=Volume_of.begin()+plus01; ci!=Volume_of.end(); ++ci) {
    if (!read_volume(*ci))
      CGAL_error_msg("SNC_io_parser::read: error in volume line");
  }
  typename std::vector<SHalfedge_iterator>::iterator sei;
  for(sei=SEdge_of.begin(); sei!=SEdge_of.end(); ++sei) {
    if (!read_sedge<K>(*sei))
      CGAL_error_msg("SNC_io_parser::read: error in sedge line");
  }
  typename std::vector<SHalfloop_iterator>::iterator sli;
  for(sli=SLoop_of.begin(); sli!=SLoop_of.end(); ++sli) {
    if (!read_sloop<K>(*sli))
      CGAL_error_msg("SNC_io_parser::read: error in sloop line");
  }
  typename std::vector<SFace_iterator>::iterator sfi;
  for(sfi=SFace_of.begin(); sfi!=SFace_of.end(); ++sfi) {
    if (!read_sface(*sfi))
      CGAL_error_msg("SNC_io_parser::read: error in sface line");
  }

  SNC_constructor C(*this->sncp());
  C.assign_indices();
}