void ElectronBrowserHandlerImpl::DoGetZoomLevel(
    DoGetZoomLevelCallback callback) {
  api::WebContents* api_web_contents = api::WebContents::From(web_contents());
  if (api_web_contents) {
    api_web_contents->DoGetZoomLevel(std::move(callback));
  }
}