PHP_FUNCTION(imagegrabwindow)
{
	HWND window;
	long client_area = 0;
	RECT rc = {0};
	RECT rc_win = {0};
	int Width, Height;
	HDC		hdc;
	HDC memDC;
	HBITMAP memBM;
	HBITMAP hOld;
	HINSTANCE handle;
	long lwindow_handle;
	typedef BOOL (WINAPI *tPrintWindow)(HWND, HDC,UINT);
	tPrintWindow pPrintWindow = 0;
	gdImagePtr im;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|l", &lwindow_handle, &client_area) == FAILURE) {
		RETURN_FALSE;
	}

	window = (HWND) lwindow_handle;

	if (!IsWindow(window)) {
		php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Invalid window handle");
		RETURN_FALSE;
	}

	hdc		= GetDC(0);

	if (client_area) {
		GetClientRect(window, &rc);
		Width = rc.right;
		Height = rc.bottom;
	} else {
		GetWindowRect(window, &rc);
		Width	= rc.right - rc.left;
		Height	= rc.bottom - rc.top;
	}

	Width		= (Width/4)*4;

	memDC	= CreateCompatibleDC(hdc);
	memBM	= CreateCompatibleBitmap(hdc, Width, Height);
	hOld	= (HBITMAP) SelectObject (memDC, memBM);


	handle = LoadLibrary("User32.dll");
	if ( handle == 0 ) {
		goto clean;
	}
	pPrintWindow = (tPrintWindow) GetProcAddress(handle, "PrintWindow");  

	if ( pPrintWindow )  {
		pPrintWindow(window, memDC, (UINT) client_area);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Windows API too old");
		goto clean;
	}

	FreeLibrary(handle);

	im = gdImageCreateTrueColor(Width, Height);
	if (im) {
		int x,y;
		for (y=0; y <= Height; y++) {
			for (x=0; x <= Width; x++) {
				int c = GetPixel(memDC, x,y);
				gdImageSetPixel(im, x, y, gdTrueColor(GetRValue(c), GetGValue(c), GetBValue(c)));
			}
		}
	}

clean:
	SelectObject(memDC,hOld);
	DeleteObject(memBM);
	DeleteDC(memDC);
	ReleaseDC( 0, hdc );

	if (!im) {
		RETURN_FALSE;
	} else {
		ZEND_REGISTER_RESOURCE(return_value, im, le_gd);
	}
}