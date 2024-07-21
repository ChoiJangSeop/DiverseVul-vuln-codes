  ResponseWrapper(const Http::ResponseHeaderMap* headers, const Http::ResponseTrailerMap* trailers,
                  const StreamInfo::StreamInfo& info)
      : headers_(headers), trailers_(trailers), info_(info) {}