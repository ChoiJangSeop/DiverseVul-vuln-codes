bool Filter::convertRequestHeadersForInternalRedirect(Http::RequestHeaderMap& downstream_headers,
                                                      const Http::HeaderEntry& internal_redirect,
                                                      uint64_t status_code) {
  if (!downstream_headers.Path()) {
    ENVOY_STREAM_LOG(trace, "no path in downstream_headers", *callbacks_);
    return false;
  }

  // Make sure the redirect response contains a URL to redirect to.
  if (internal_redirect.value().getStringView().empty()) {
    config_.stats_.passthrough_internal_redirect_bad_location_.inc();
    return false;
  }
  Http::Utility::Url absolute_url;
  if (!absolute_url.initialize(internal_redirect.value().getStringView(), false)) {
    config_.stats_.passthrough_internal_redirect_bad_location_.inc();
    return false;
  }

  const auto& policy = route_entry_->internalRedirectPolicy();
  // Don't change the scheme from the original request
  const bool scheme_is_http = schemeIsHttp(downstream_headers, *callbacks_->connection());
  const bool target_is_http = absolute_url.scheme() == Http::Headers::get().SchemeValues.Http;
  if (!policy.isCrossSchemeRedirectAllowed() && scheme_is_http != target_is_http) {
    config_.stats_.passthrough_internal_redirect_unsafe_scheme_.inc();
    return false;
  }

  const StreamInfo::FilterStateSharedPtr& filter_state = callbacks_->streamInfo().filterState();
  // Make sure that performing the redirect won't result in exceeding the configured number of
  // redirects allowed for this route.
  if (!filter_state->hasData<StreamInfo::UInt32Accessor>(NumInternalRedirectsFilterStateName)) {
    filter_state->setData(
        NumInternalRedirectsFilterStateName, std::make_shared<StreamInfo::UInt32AccessorImpl>(0),
        StreamInfo::FilterState::StateType::Mutable, StreamInfo::FilterState::LifeSpan::Request);
  }
  StreamInfo::UInt32Accessor& num_internal_redirect =
      filter_state->getDataMutable<StreamInfo::UInt32Accessor>(NumInternalRedirectsFilterStateName);

  if (num_internal_redirect.value() >= policy.maxInternalRedirects()) {
    config_.stats_.passthrough_internal_redirect_too_many_redirects_.inc();
    return false;
  }
  // Copy the old values, so they can be restored if the redirect fails.
  const std::string original_host(downstream_headers.getHostValue());
  const std::string original_path(downstream_headers.getPathValue());
  const bool scheme_is_set = (downstream_headers.Scheme() != nullptr);
  Cleanup restore_original_headers(
      [&downstream_headers, original_host, original_path, scheme_is_set, scheme_is_http]() {
        downstream_headers.setHost(original_host);
        downstream_headers.setPath(original_path);
        if (scheme_is_set) {
          downstream_headers.setScheme(scheme_is_http ? Http::Headers::get().SchemeValues.Http
                                                      : Http::Headers::get().SchemeValues.Https);
        }
      });

  // Replace the original host, scheme and path.
  downstream_headers.setScheme(absolute_url.scheme());
  downstream_headers.setHost(absolute_url.hostAndPort());

  auto path_and_query = absolute_url.pathAndQueryParams();
  if (Runtime::runtimeFeatureEnabled("envoy.reloadable_features.http_reject_path_with_fragment")) {
    // Envoy treats internal redirect as a new request and will reject it if URI path
    // contains #fragment. However the Location header is allowed to have #fragment in URI path. To
    // prevent Envoy from rejecting internal redirect, strip the #fragment from Location URI if it
    // is present.
    auto fragment_pos = path_and_query.find('#');
    path_and_query = path_and_query.substr(0, fragment_pos);
  }
  downstream_headers.setPath(path_and_query);

  callbacks_->clearRouteCache();
  const auto route = callbacks_->route();
  // Don't allow a redirect to a non existing route.
  if (!route) {
    config_.stats_.passthrough_internal_redirect_no_route_.inc();
    return false;
  }

  const auto& route_name = route->routeEntry()->routeName();
  for (const auto& predicate : policy.predicates()) {
    if (!predicate->acceptTargetRoute(*filter_state, route_name, !scheme_is_http,
                                      !target_is_http)) {
      config_.stats_.passthrough_internal_redirect_predicate_.inc();
      ENVOY_STREAM_LOG(trace, "rejecting redirect targeting {}, by {} predicate", *callbacks_,
                       route_name, predicate->name());
      return false;
    }
  }

  // See https://tools.ietf.org/html/rfc7231#section-6.4.4.
  if (status_code == enumToInt(Http::Code::SeeOther) &&
      downstream_headers.getMethodValue() != Http::Headers::get().MethodValues.Get &&
      downstream_headers.getMethodValue() != Http::Headers::get().MethodValues.Head) {
    downstream_headers.setMethod(Http::Headers::get().MethodValues.Get);
    downstream_headers.remove(Http::Headers::get().ContentLength);
    callbacks_->modifyDecodingBuffer([](Buffer::Instance& data) { data.drain(data.length()); });
  }

  num_internal_redirect.increment();
  restore_original_headers.cancel();
  // Preserve the original request URL for the second pass.
  downstream_headers.setEnvoyOriginalUrl(absl::StrCat(scheme_is_http
                                                          ? Http::Headers::get().SchemeValues.Http
                                                          : Http::Headers::get().SchemeValues.Https,
                                                      "://", original_host, original_path));
  return true;
}