static void check_image(int pageno, QPDFObjectHandle page)
{
    QPDFObjectHandle image =
        page.getKey("/Resources").getKey("/XObject").getKey("/Im1");
    ImageChecker ic(pageno);
    image.pipeStreamData(&ic, 0, qpdf_dl_specialized);
}