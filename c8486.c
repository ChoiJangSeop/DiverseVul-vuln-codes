VideoTrack::VideoTrack(Segment* pSegment, long long element_start,
                       long long element_size)
    : Track(pSegment, element_start, element_size),
      m_colour(NULL),
      m_projection(NULL) {}