void ElectronBrowserHandlerImpl::OnFirstNonEmptyLayout() {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->OnFirstNonEmptyLayout(GetRenderFrameHost());
  }
}