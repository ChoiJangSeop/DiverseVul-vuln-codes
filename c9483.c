  RequestWrapper(const Http::RequestHeaderMap* headers, const StreamInfo::StreamInfo& info)
      : headers_(headers), info_(info) {}