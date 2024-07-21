QPDFPageObjectHelper::getMatrixForTransformations(bool invert)
{
    QPDFObjectHandle::Matrix matrix(1, 0, 0, 1, 0, 0);
    QPDFObjectHandle bbox = getTrimBox(false);
    if (! bbox.isRectangle())
    {
        return matrix;
    }
    QPDFObjectHandle rotate_obj = getAttribute("/Rotate", false);
    QPDFObjectHandle scale_obj = getAttribute("/UserUnit", false);
    if (! (rotate_obj.isNull() && scale_obj.isNull()))
    {
        QPDFObjectHandle::Rectangle rect = bbox.getArrayAsRectangle();
        double width = rect.urx - rect.llx;
        double height = rect.ury - rect.lly;
        double scale = (scale_obj.isNumber()
                        ? scale_obj.getNumericValue()
                        : 1.0);
        int rotate = (rotate_obj.isInteger()
                      ? rotate_obj.getIntValue()
                      : 0);
        if (invert)
        {
            if (scale == 0.0)
            {
                return matrix;
            }
            scale = 1.0 / scale;
            rotate = 360 - rotate;
        }

        // Ignore invalid rotation angle
        switch (rotate)
        {
          case 90:
            matrix = QPDFObjectHandle::Matrix(
                0, -scale, scale, 0, 0, width * scale);
            break;
          case 180:
            matrix = QPDFObjectHandle::Matrix(
                -scale, 0, 0, -scale, width * scale, height * scale);
            break;
          case 270:
            matrix = QPDFObjectHandle::Matrix(
                0, scale, -scale, 0, height * scale, 0);
            break;
          default:
            matrix = QPDFObjectHandle::Matrix(
                scale, 0, 0, scale, 0, 0);
            break;
        }
    }
    return matrix;
}