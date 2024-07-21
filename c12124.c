void ElectronBrowserHandlerImpl::MessageSync(bool internal,
                                             const std::string& channel,
                                             blink::CloneableMessage arguments,
                                             MessageSyncCallback callback) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->MessageSync(internal, channel, std::move(arguments),
                                  std::move(callback), GetRenderFrameHost());
  }
}