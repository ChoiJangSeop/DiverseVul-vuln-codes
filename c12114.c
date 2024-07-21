void WebContents::DoGetZoomLevel(
    electron::mojom::ElectronBrowser::DoGetZoomLevelCallback callback) {
  std::move(callback).Run(GetZoomLevel());
}