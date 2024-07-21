void ElectronBrowserHandlerImpl::MessageTo(int32_t web_contents_id,
                                           const std::string& channel,
                                           blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->MessageTo(web_contents_id, channel, std::move(arguments));
  }
}