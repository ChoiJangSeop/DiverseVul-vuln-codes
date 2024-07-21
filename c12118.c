void ElectronBrowserHandlerImpl::MessageHost(
    const std::string& channel,
    blink::CloneableMessage arguments) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->MessageHost(channel, std::move(arguments),
                                  GetRenderFrameHost());
  }
}