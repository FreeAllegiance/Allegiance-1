
#include "imagetransform.h"
#include "image.h"

class ImageSize : public PointValue {
private:
    Image* GetImage() { return Image::Cast(GetChild(0)); }

public:
    ImageSize(Image* pimage) :
        PointValue(pimage)
    {
    }

    void Evaluate()
    {
        GetValueInternal() = Point::Cast(GetImage()->GetBounds().GetRect().Size());
    }
};

class ImageBounds : public RectValue {
private:
    Image* GetImage() { return Image::Cast(GetChild(0)); }

public:
    ImageBounds(Image* pimage) :
        RectValue(pimage)
    {
    }

    void Evaluate()
    {
        GetValueInternal() = GetImage()->GetBounds().GetRect();
    }
};

class CutImage : public WrapImage {
public:
    CutImage(Image* pimage, RectValue* prect) :
        WrapImage(pimage, prect)
    {
    }

    RectValue* GetRect() { return RectValue::Cast(GetChild(1)); }

    void CalcBounds()
    {
        m_bounds = Rect(Point(0, 0), GetRect()->GetValue().Size());
    }

    MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
    {
        Rect rect = GetRect()->GetValue();
        if (Rect(Point(0, 0), m_bounds.GetRect().Size()).Inside(point)) {
            return WrapImage::HitTest(pprovider, Point(point.X() + rect.XMin(), point.Y() + rect.YMin()), bCaptured);
        }

        return MouseResult();
    }

    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    {
        Rect rect = GetRect()->GetValue();
        if (Rect(Point(0, 0), m_bounds.GetRect().Size()).Inside(point)) {
            return WrapImage::Button(pprovider, Point(point.X() + rect.XMin(), point.Y() + rect.YMin()), button, bCaptured, bInside, bDown);
        }

        return MouseResult();
    }

    void Render(Context* pcontext)
    {
        pcontext->Clip(Rect(Point(0, 0), GetRect()->GetValue().Size()));
        pcontext->Translate(-GetRect()->GetValue().Min());
        WrapImage::Render(pcontext);
    }

    ZString GetFunctionName() { return "CutImage"; }

};

class StringImageCorrected : public Image {
private:
    Justification     m_justification;

    //////////////////////////////////////////////////////////////////////////////
    //
    // value accessors
    //
    //////////////////////////////////////////////////////////////////////////////

    StringValue* GetString() { return StringValue::Cast(GetChild(0)); }
    ColorValue*  GetColor() { return  ColorValue::Cast(GetChild(1)); }
    Number* GetWidth() { return Number::Cast(GetChild(2)); }
    Number* GetLineSeparation() { return Number::Cast(GetChild(3)); }
    FontValue* GetFont() { return FontValue::Cast(GetChild(4)); }

public:
    StringImageCorrected(
        Justification justification,
        FontValue*  pfont,
        ColorValue*   pcolor,
        Number*       pwidth,
        StringValue*  pstring,
        Number*       pSeparation
    ) :
        Image(pstring, pcolor, pwidth, pSeparation),
        m_justification(justification)
    {
        AddChild(pfont);
    }

    ZString BreakLine(ZString& str, int width)
    {
        ZString strSave = str;

        //
        // Search forward for any new lines
        //

        int indexNewLine = str.Find("\n");
        if (indexNewLine == -1) {
            indexNewLine = str.GetLength();
        }

        //
        // Figure out how many characters fit on the line
        //

        int nchar = GetFont()->GetValue()->GetMaxTextLength(str, width, true);

        //
        // Chop off at the newline
        //

        if (nchar >= indexNewLine) {
            nchar = indexNewLine;
        }
        else {
            //
            // Search backwards for the first space
            //

            if (nchar != str.GetLength()) {
                for (int index = nchar - 1; index >= 0; index--) {
                    if (str[index] == ' ') {
                        while (index > 0) {
                            if (str[index] != ' ') {
                                nchar = index + 1;
                                break;
                            }
                            index--;
                        }
                        break;
                    }
                }
            }
        }

        if (nchar < str.GetLength()) {
            str = strSave.RightOf(nchar + 1);
            return strSave.Left(nchar);
        }
        else {
            //
            // remove any leading spaces from the next line
            //

            str = strSave.RightOf(nchar);

            int length = str.GetLength();
            int index = 0;

            while (index < length && str[index] == ' ') {
                index++;
            }

            if (index != 0) {
                str = str.RightOf(index);
            }

            return strSave.Left(nchar);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Image methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void CalcBounds()
    {
        ZString str = GetString()->GetValue();
        float fWidth = GetWidth()->GetValue();
        int     xsize = 0;
        int     ysize = 0;

        IEngineFont* pfont = GetFont()->GetValue();

        while (!str.IsEmpty()) {
            ZString strLine = BreakLine(str, fWidth);
            WinPoint size = pfont->GetTextExtent(strLine);

            xsize = std::max(xsize, size.X());
            ysize += size.Y();
        }

        m_bounds.SetRect(Rect(0, 0, xsize, (float)ysize));
    }

    void Render(Context* pcontext)
    {
        ZString     str = GetString()->GetValue();
        const Rect& rect = m_bounds.GetRect();
        float fWidth = rect.XMax();
        int         y = 0;

        float fLineSeparation = GetLineSeparation()->GetValue();

        ZAssert(!pcontext->GetYAxisInversion());

        const Color& color = GetColor()->GetValue();
        IEngineFont* pfont = GetFont()->GetValue();

        while (!str.IsEmpty()) {
            ZString  strLine = BreakLine(str, fWidth);
            WinPoint size = pfont->GetTextExtent(strLine);
            int      x;

            if (m_justification == JustifyLeft()) {
                x = 0;
            }
            else if (m_justification == JustifyRight()) {
                x = fWidth - size.X();
            }
            else if (m_justification == JustifyCenter()) {
                x = (fWidth - size.X()) / 2;
            }
            else {
                ZError("Invalid Justification");
            }

            pcontext->DrawString(
                pfont,
                color,
                Point(
                (float)x,
                    (float)y
                ),
                strLine
            );

            y += size.Y() + fLineSeparation;
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Value members
    //
    //////////////////////////////////////////////////////////////////////////////

    ZString GetFunctionName() { return "StringImage"; }
};

TRef<Image> ImageTransform::Translate(Image* pImage, PointValue* pPoint) {
    return new TransformImage(
        pImage,
        new TranslateTransform2(pPoint)
    );
};

TRef<Image> ImageTransform::Scale(Image* pImage, PointValue* pPoint) {
    return new TransformImage(
        pImage,
        new ScaleTransform2(pPoint)
    );
};

TRef<Image> ImageTransform::Rotate(Image* pImage, Number* pNumberRadians) {
    return new TransformImage(
        pImage,
        new RotateTransform2(pNumberRadians)
    );
}

TRef<PointValue> ImageTransform::Size(Image* pImage) {
    return new ImageSize(pImage);
};

TRef<RectValue> ImageTransform::Bounds(Image* pImage) {
    return new ImageBounds(pImage);
}

TRef<Image> ImageTransform::Justify(Image* pimage, PointValue* pSizeContainer, Justification justification) {
    TRef<PointValue> sizeImage = ImageTransform::Size(pimage);
    TRef<Number> pImageX = PointTransform::X(sizeImage);
    TRef<Number> pImageY = PointTransform::Y(sizeImage);

    TRef<Number> pContainerX = PointTransform::X(pSizeContainer);
    TRef<Number> pContainerY = PointTransform::Y(pSizeContainer);

    TRef<Number> pSpaceX = NumberTransform::Subtract(pContainerX, pImageX);
    TRef<Number> pSpaceY = NumberTransform::Subtract(pContainerY, pImageY);

    TRef<Number> pNumberZero = new Number(0.0f);
    TRef<Number> pNumberHalf = new Number(0.5f);

    TRef<Number> pOffsetX;
    TRef<Number> pOffsetY;

    if (justification.Test(JustifyLeft())) {
        pOffsetX = pNumberZero;
    }
    else if (justification.Test(JustifyRight())) {
        pOffsetX = pSpaceX;
    }
    else {
        pOffsetX = NumberTransform::Multiply(pNumberHalf, pSpaceX);
    }

    if (justification.Test(JustifyTop())) {
        pOffsetY = pNumberZero;
    }
    else if (justification.Test(JustifyBottom())) {
        pOffsetY = pSpaceY;
    }
    else {
        pOffsetY = NumberTransform::Multiply(pNumberHalf, pSpaceY);
    }

    TRef<RectValue> pBounds = ImageTransform::Bounds(pimage);

    TRef<PointValue> pPointTranslate = PointTransform::Create(
        NumberTransform::Subtract(pOffsetX, RectTransform::XMin(pBounds)),
        NumberTransform::Subtract(pOffsetY, RectTransform::YMin(pBounds))
    );

    return (TRef<Image>)new TransformImage(
        pimage,
        new TranslateTransform2(pPointTranslate)
    );
}

TRef<Image> ImageTransform::ScaleFit(Image* pimage, PointValue* pSizeContainer, Justification justification) {
    //this should maybe be redone to return a custom class instead of using so many wrappers, it's a cool example though

    TRef<PointValue> sizeImage = ImageTransform::Size(pimage);
    TRef<Number> pImageX = PointTransform::X(sizeImage);
    TRef<Number> pImageY = PointTransform::Y(sizeImage);

    TRef<Number> pContainerX = PointTransform::X(pSizeContainer);
    TRef<Number> pContainerY = PointTransform::Y(pSizeContainer);

    // What scaling factor would we need for each axis? Pick the smallest one
    TRef<Number> pScale = NumberTransform::Min(
        NumberTransform::Divide(pContainerX, pImageX),
        NumberTransform::Divide(pContainerY, pImageY)
    );

    TRef<PointValue> pPointScale = PointTransform::Create(pScale, pScale);

    return ImageTransform::Justify(
        new TransformImage(
            pimage,
            new ScaleTransform2(pPointScale)
        ),
        pSizeContainer,
        justification
    );
}

TRef<Image> ImageTransform::ScaleFill(Image* pimage, PointValue* pSizeContainer, Justification justification) {
    //this should maybe be redone to return a custom class instead of using so many wrappers, it's a cool example though

    TRef<PointValue> sizeImage = ImageTransform::Size(pimage);
    TRef<Number> pImageX = PointTransform::X(sizeImage);
    TRef<Number> pImageY = PointTransform::Y(sizeImage);

    TRef<Number> pContainerX = PointTransform::X(pSizeContainer);
    TRef<Number> pContainerY = PointTransform::Y(pSizeContainer);

    // What scaling factor would we need for each axis? Pick the largest one
    TRef<Number> pScale = NumberTransform::Max(
        NumberTransform::Divide(pContainerX, pImageX),
        NumberTransform::Divide(pContainerY, pImageY)
    );

    TRef<PointValue> pPointScale = PointTransform::Create(pScale, pScale);

    return ImageTransform::Justify(
        new TransformImage(
            pimage,
            new ScaleTransform2(pPointScale)
        ),
        pSizeContainer,
        justification
    );
}

TRef<Image> ImageTransform::Switch(Boolean* pValue, std::map<bool, TRef<Image>> mapOptions, TRef<Image> defaultOption) {
    if (defaultOption == nullptr) {
        defaultOption = Image::GetEmpty();
    }
    return new CallbackImage<bool>([mapOptions, defaultOption](bool value) {
        auto find = mapOptions.find(value);
        if (find == mapOptions.end()) {
            return defaultOption;
        }
        return find->second;
    }, pValue);
}

TRef<Image> ImageTransform::Switch(Number* pValue, std::map<int, TRef<Image>> mapOptions, TRef<Image> defaultOption) {
    if (defaultOption == nullptr) {
        defaultOption = Image::GetEmpty();
    }
    return new CallbackImage<float>([mapOptions, defaultOption](float value) {
        auto find = mapOptions.find((int)value); //convert to integer
        if (find == mapOptions.end()) {
            return defaultOption;
        }
        return find->second;
    }, pValue);
}

TRef<Image> ImageTransform::Switch(TStaticValue<ZString>* pValue, std::map<std::string, TRef<Image>> mapOptions, TRef<Image> defaultOption) {
    if (defaultOption == nullptr) {
        defaultOption = Image::GetEmpty();
    }
    return new CallbackImage<ZString>([mapOptions, defaultOption](ZString value) {
        auto find = mapOptions.find(std::string(value));
        if (find == mapOptions.end()) {
            return defaultOption;
        }
        return find->second;
    }, pValue);
}

TRef<Image> ImageTransform::Clip(Image* pImage, RectValue* pRect) {
    return CreateClipImage(pImage, pRect);
};

TRef<Image> ImageTransform::Cut(Image* pImage, RectValue* pRect) {
    return new CutImage(pImage, pRect);
};

TRef<Image> ImageTransform::Multiply(Image* pImage, ColorValue* pColor) {
    //ugly cast here
    ConstantImage* pConstantImage = (ConstantImage*)(pImage);

    return CreateConstantImage3D(pImage->GetSurface(), pColor);
};

class LazyImage : public WrapImage {

private:
    std::function<TRef<Image>()> m_callback;



public:
    LazyImage(std::function<TRef<Image>()> callback) : 
        WrapImage(Image::GetEmpty()),
        m_callback(callback)
    {

    }

protected:
    void OnNoParents() override {
        SetImage(Image::GetEmpty());
    }

    void Evaluate() override {
        if (GetImage() == Image::GetEmpty()) {
            TRef<Image> pLoadedImage;
            try {
                pLoadedImage = m_callback();
            }
            catch (const std::runtime_error& e) {
                pLoadedImage = Image::GetEmpty();
            }
            pLoadedImage->Update();
            SetChildSilently(0, pLoadedImage);
        }
        WrapImage::Evaluate();
    }

};

TRef<Image> ImageTransform::Lazy(std::function<TRef<Image>()> callback)
{
    return new LazyImage(callback);
}

TRef<Image> ImageTransform::String(FontValue * font, ColorValue * color, Number * width, StringValue * string, Justification justification, Number* pLineSeparation)
{
    return new StringImageCorrected(justification, font, color, width, string, pLineSeparation);
}