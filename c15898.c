Http::FilterHeadersStatus OAuth2Filter::decodeHeaders(Http::RequestHeaderMap& headers, bool) {

  // The following 2 headers are guaranteed for regular requests. The asserts are helpful when
  // writing test code to not forget these important variables in mock requests
  const Http::HeaderEntry* host_header = headers.Host();
  ASSERT(host_header != nullptr);
  host_ = host_header->value().getStringView();

  const Http::HeaderEntry* path_header = headers.Path();
  ASSERT(path_header != nullptr);
  const absl::string_view path_str = path_header->value().getStringView();

  // We should check if this is a sign out request.
  if (config_->signoutPath().match(path_header->value().getStringView())) {
    return signOutUser(headers);
  }

  if (canSkipOAuth(headers)) {
    // Update the path header with the query string parameters after a successful OAuth login.
    // This is necessary if a website requests multiple resources which get redirected to the
    // auth server. A cached login on the authorization server side will set cookies
    // correctly but cause a race condition on future requests that have their location set
    // to the callback path.

    if (config_->redirectPathMatcher().match(path_str)) {
      Http::Utility::QueryParams query_parameters = Http::Utility::parseQueryString(path_str);

      const auto state =
          Http::Utility::PercentEncoding::decode(query_parameters.at(queryParamsState()));
      Http::Utility::Url state_url;
      if (!state_url.initialize(state, false)) {
        sendUnauthorizedResponse();
        return Http::FilterHeadersStatus::StopIteration;
      }
      // Avoid infinite redirect storm
      if (config_->redirectPathMatcher().match(state_url.pathAndQueryParams())) {
        sendUnauthorizedResponse();
        return Http::FilterHeadersStatus::StopIteration;
      }
      Http::ResponseHeaderMapPtr response_headers{
          Http::createHeaderMap<Http::ResponseHeaderMapImpl>(
              {{Http::Headers::get().Status, std::to_string(enumToInt(Http::Code::Found))},
               {Http::Headers::get().Location, state}})};
      decoder_callbacks_->encodeHeaders(std::move(response_headers), true, REDIRECT_RACE);
    }

    // Continue on with the filter stack.
    return Http::FilterHeadersStatus::Continue;
  }

  // Save the request headers for later modification if needed.
  if (config_->forwardBearerToken()) {
    request_headers_ = &headers;
  }

  // If a bearer token is supplied as a header or param, we ingest it here and kick off the
  // user resolution immediately. Note this comes after HMAC validation, so technically this
  // header is sanitized in a way, as the validation check forces the correct Bearer Cookie value.
  access_token_ = extractAccessToken(headers);
  if (!access_token_.empty()) {
    found_bearer_token_ = true;
    finishFlow();
    return Http::FilterHeadersStatus::Continue;
  }

  // If no access token and this isn't the callback URI, redirect to acquire credentials.
  //
  // The following conditional could be replaced with a regex pattern-match,
  // if we're concerned about strict matching against the callback path.
  if (!config_->redirectPathMatcher().match(path_str)) {
    Http::ResponseHeaderMapPtr response_headers{Http::createHeaderMap<Http::ResponseHeaderMapImpl>(
        {{Http::Headers::get().Status, std::to_string(enumToInt(Http::Code::Found))}})};

    // Construct the correct scheme. We default to https since this is a requirement for OAuth to
    // succeed. However, if a downstream client explicitly declares the "http" scheme for whatever
    // reason, we also use "http" when constructing our redirect uri to the authorization server.
    auto scheme = Http::Headers::get().SchemeValues.Https;

    const auto* scheme_header = headers.Scheme();
    if ((scheme_header != nullptr &&
         scheme_header->value().getStringView() == Http::Headers::get().SchemeValues.Http)) {
      scheme = Http::Headers::get().SchemeValues.Http;
    }

    const std::string base_path = absl::StrCat(scheme, "://", host_);
    const std::string state_path = absl::StrCat(base_path, headers.Path()->value().getStringView());
    const std::string escaped_state = Http::Utility::PercentEncoding::encode(state_path, ":/=&?");

    Formatter::FormatterImpl formatter(config_->redirectUri());
    const auto redirect_uri = formatter.format(headers, *Http::ResponseHeaderMapImpl::create(),
                                               *Http::ResponseTrailerMapImpl::create(),
                                               decoder_callbacks_->streamInfo(), "");
    const std::string escaped_redirect_uri =
        Http::Utility::PercentEncoding::encode(redirect_uri, ":/=&?");

    const std::string new_url = fmt::format(
        AuthorizationEndpointFormat, config_->authorizationEndpoint(), config_->clientId(),
        config_->encodedAuthScopes(), escaped_redirect_uri, escaped_state);

    response_headers->setLocation(new_url + config_->encodedResourceQueryParams());
    decoder_callbacks_->encodeHeaders(std::move(response_headers), true, REDIRECT_FOR_CREDENTIALS);

    config_->stats().oauth_unauthorized_rq_.inc();

    return Http::FilterHeadersStatus::StopIteration;
  }

  // At this point, we *are* on /_oauth. We believe this request comes from the authorization
  // server and we expect the query strings to contain the information required to get the access
  // token
  const auto query_parameters = Http::Utility::parseQueryString(path_str);
  if (query_parameters.find(queryParamsError()) != query_parameters.end()) {
    sendUnauthorizedResponse();
    return Http::FilterHeadersStatus::StopIteration;
  }

  // if the data we need is not present on the URL, stop execution
  if (query_parameters.find(queryParamsCode()) == query_parameters.end() ||
      query_parameters.find(queryParamsState()) == query_parameters.end()) {
    sendUnauthorizedResponse();
    return Http::FilterHeadersStatus::StopIteration;
  }

  auth_code_ = query_parameters.at(queryParamsCode());
  state_ = Http::Utility::PercentEncoding::decode(query_parameters.at(queryParamsState()));

  Http::Utility::Url state_url;
  if (!state_url.initialize(state_, false)) {
    sendUnauthorizedResponse();
    return Http::FilterHeadersStatus::StopIteration;
  }

  Formatter::FormatterImpl formatter(config_->redirectUri());
  const auto redirect_uri = formatter.format(headers, *Http::ResponseHeaderMapImpl::create(),
                                             *Http::ResponseTrailerMapImpl::create(),
                                             decoder_callbacks_->streamInfo(), "");
  oauth_client_->asyncGetAccessToken(auth_code_, config_->clientId(), config_->clientSecret(),
                                     redirect_uri);

  // pause while we await the next step from the OAuth server
  return Http::FilterHeadersStatus::StopAllIterationAndBuffer;
}