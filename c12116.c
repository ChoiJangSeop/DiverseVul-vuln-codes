void ElectronBrowserHandlerImpl::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->ReceivePostMessage(channel, std::move(message),
                                         GetRenderFrameHost());
  }
}