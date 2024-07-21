ActivationPtr createActivation(const StreamInfo::StreamInfo& info,
                               const Http::RequestHeaderMap* request_headers,
                               const Http::ResponseHeaderMap* response_headers,
                               const Http::ResponseTrailerMap* response_trailers) {
  auto activation = std::make_unique<Activation>();
  activation->InsertValueProducer(Request, std::make_unique<RequestWrapper>(request_headers, info));
  activation->InsertValueProducer(
      Response, std::make_unique<ResponseWrapper>(response_headers, response_trailers, info));
  activation->InsertValueProducer(Connection, std::make_unique<ConnectionWrapper>(info));
  activation->InsertValueProducer(Upstream, std::make_unique<UpstreamWrapper>(info));
  activation->InsertValueProducer(Source, std::make_unique<PeerWrapper>(info, false));
  activation->InsertValueProducer(Destination, std::make_unique<PeerWrapper>(info, true));
  activation->InsertValueProducer(Metadata,
                                  std::make_unique<MetadataProducer>(info.dynamicMetadata()));
  activation->InsertValueProducer(FilterState,
                                  std::make_unique<FilterStateWrapper>(info.filterState()));
  return activation;
}