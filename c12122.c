void ElectronBrowserHandlerImpl::Invoke(bool internal,
                                        const std::string& channel,
                                        blink::CloneableMessage arguments,
                                        InvokeCallback callback) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->Invoke(internal, channel, std::move(arguments),
                             std::move(callback), GetRenderFrameHost());
  }
}