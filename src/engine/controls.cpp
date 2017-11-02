#include "pch.h"

/////////////////////////////////////////////////////////////////////////////
//
// Justify Pane
//
/////////////////////////////////////////////////////////////////////////////

JustifyPane::JustifyPane(int layout, const WinPoint& size, Pane* pchild) :
    Pane(pchild),
    m_bigSize(size),
    m_layout(layout)
{
}

void JustifyPane::UpdateLayout()
{
    if (Child()) {
        InternalSetSize(
            WinPoint(
                max(XExpand(), m_bigSize.X()),
                max(YExpand(), m_bigSize.Y())
            )
        );

        InternalSetExpand(Child(), GetSize());

        Child()->UpdateLayout();

        InternalSetSize(WinPoint(
            max(XSize(), Child()->XSize()),
            max(YSize(), Child()->YSize())
        ));

        int dx = XSize() - Child()->XSize();
        int dy = YSize() - Child()->YSize();

        InternalSetOffset(
            Child(),
            WinPoint(
                m_layout & Left ? 0 : (m_layout & Right  ? dx : dx / 2),
                m_layout & Top  ? 0 : (m_layout & Bottom ? dy : dy / 2)
            )
        );
    } else {
        InternalSetSize(
            WinPoint(
                max(XExpand(), m_bigSize.X()),
                max(YExpand(), m_bigSize.Y())
        ));
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Fill Pane
//
/////////////////////////////////////////////////////////////////////////////

FillPane::FillPane(const WinPoint& sizeMin, const Color& color) :
    m_sizeMin(sizeMin),
    m_color(color)
{
	SetOpaque(true);
}

void FillPane::SetColor(const Color& color)
{
    m_color = color;
    NeedPaint();
}

void FillPane::UpdateLayout()
{
    InternalSetSize(
        WinPoint(
            max(m_sizeMin.X(), XExpand()),
            max(m_sizeMin.Y(), YExpand())
        )
    );
}

void FillPane::Paint(Surface* psurface)
{
    psurface->FillSurface(m_color);
}

/////////////////////////////////////////////////////////////////////////////
//
// Border Pane
//
/////////////////////////////////////////////////////////////////////////////

BorderPane::BorderPane(int width, const Color& color, Pane* pchild) :
    Pane(pchild),
    m_xborder(width),
    m_yborder(width),
    m_color(color),
    m_colorSelected(color)
{
	SetOpaque(true);
}

BorderPane::BorderPane(int xborder, int yborder, const Color& color, Pane* pchild) :
    Pane(pchild),
    m_xborder(xborder),
    m_yborder(yborder),
    m_color(color),
    m_colorSelected(color)
{
	SetOpaque(true);
}

void BorderPane::SetColor(const Color& color)
{
    m_color = color;
    NeedPaint();
}

void BorderPane::SetColorSelected(const Color& color)
{
    m_colorSelected = color;
    NeedPaint();
}

void BorderPane::UpdateLayout()
{
    if (Child()) {
        InternalSetExpand(
            Child(),
            WinPoint(
                XExpand() == 0 ? 0 : XExpand() - m_xborder * 2,
                YExpand() == 0 ? 0 : YExpand() - m_yborder * 2
            )
        );
        Child()->UpdateLayout();
        InternalSetSize(WinPoint(
            Child()->XSize() + m_xborder * 2,
            Child()->YSize() + m_yborder * 2
        ));
        InternalSetOffset(Child(), WinPoint(m_xborder, m_yborder));
    } else {
        InternalSetSize(
            WinPoint(
                max(m_xborder * 2, XExpand()),
                max(m_yborder * 2, YExpand())
            )
        );
    }
}

void BorderPane::Paint(Surface* psurface)
{
    if (IsSelected()) {
        psurface->FillSurface(m_colorSelected);
    } else {
        psurface->FillSurface(m_color);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Edge Pane
//
/////////////////////////////////////////////////////////////////////////////

EdgePane::EdgePane(Pane* pchild, bool bUp) :
    Pane(pchild),
    m_bUp(bUp)
{
}

void EdgePane::UpdateLayout()
{
    int width = 3; // m_bUp ? 3 : 4;

    if (Child()) {
        InternalSetExpand(
            Child(),
            WinPoint(
                XExpand() == 0 ? 0 : XExpand() - width,
                YExpand() == 0 ? 0 : YExpand() - width
            )
        );
        Child()->UpdateLayout();
        InternalSetSize(WinPoint(
            Child()->XSize() + width,
            Child()->YSize() + width
        ));
        InternalSetOffset(Child(), WinPoint(width - 2, width - 2));
    } else {
        InternalSetSize(WinPoint(width, width));
    }
}

void EdgePane::Paint(Surface* psurface)
{
    if (m_bUp) {
        // top

        psurface->FillRect(WinRect(0, 0, XSize() - 1, 1), GetSystemColor(SystemColor3DHighLight));

        // left

        psurface->FillRect(WinRect(0, 0, 1, YSize() - 1), GetSystemColor(SystemColor3DHighLight));

        // bottom

        psurface->FillRect(WinRect(1, YSize() - 2, XSize() - 1, YSize() - 1), GetSystemColor(SystemColor3DShadow));
        psurface->FillRect(WinRect(0, YSize() - 1, XSize(),     YSize()    ), GetSystemColor(SystemColor3DDKShadow));

        // right

        psurface->FillRect(WinRect(XSize() - 2, 1, XSize() - 1, YSize() - 1), GetSystemColor(SystemColor3DShadow));
        psurface->FillRect(WinRect(XSize() - 1, 0, XSize(),     YSize()    ), GetSystemColor(SystemColor3DDKShadow));
    } else {
        // top

        psurface->FillRect(WinRect(0, 0, XSize() - 1, 1), GetSystemColor(SystemColor3DDKShadow));
        psurface->FillRect(WinRect(1, 1, XSize() - 1, 2), GetSystemColor(SystemColor3DShadow));

        // left

        psurface->FillRect(WinRect(0, 0, 1, YSize() - 1), GetSystemColor(SystemColor3DDKShadow));
        psurface->FillRect(WinRect(1, 1, 2, YSize() - 2), GetSystemColor(SystemColor3DShadow));

        // bottom

        psurface->FillRect(WinRect(0, YSize() - 1, XSize(), YSize()), GetSystemColor(SystemColor3DHighLight));

        // right

        psurface->FillRect(WinRect(XSize() - 1, 0, XSize(), YSize()), GetSystemColor(SystemColor3DHighLight));
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Column Pane
//
/////////////////////////////////////////////////////////////////////////////

ColumnPane::ColumnPane(bool bShowGrid) :
    m_bShowGrid(bShowGrid)
{}

ColumnPane::ColumnPane(Pane* ppane0, bool bShowGrid) :
    m_bShowGrid(bShowGrid)
{
    InsertAtBottom(ppane0);
}

ColumnPane::ColumnPane(Pane* ppane0, Pane* ppane1, bool bShowGrid) :
    m_bShowGrid(bShowGrid)
{
    InsertAtBottom(ppane0);
    InsertAtBottom(ppane1);
}

ColumnPane::ColumnPane(Pane* ppane0, Pane* ppane1, Pane* ppane2, bool bShowGrid) :
    m_bShowGrid(bShowGrid)
{
    InsertAtBottom(ppane0);
    InsertAtBottom(ppane1);
    InsertAtBottom(ppane2);
}

void ColumnPane::ShowGrid(bool bShowGrid)
{
    m_bShowGrid = bShowGrid;
    NeedLayout();
    NeedPaint();
}

void ColumnPane::UpdateLayout()
{
    int xsize = 0;
    int ysize = 0;

    //
    // first calculate the non-y expanded size
    //

    Pane* ppane;
    for(ppane = Child(); ppane != NULL; ppane = ppane->Next()) {
        if (!ppane->IsHidden()) {
            InternalSetExpand(ppane, WinPoint(XExpand(), 0));
            ppane->UpdateLayout();
            InternalSetOffset(ppane, WinPoint(0, ysize));

            xsize = max(ppane->XSize(), xsize);
            ysize += ppane->YSize() + (m_bShowGrid ? 1 : 0);
        }
    }

    //
    // If the ysize is less than YExpand() try to expand the child panes
    //

    int yextra = YExpand() - ysize;

    if (yextra > 0 || ysize > YExpand()) {
        bool bExpandLeftFirst = true;

		if (yextra < 0) {
			yextra = 0;
		}

        ysize = 0;

        for(ppane = Child(); ppane != NULL; ppane = ppane->Next()) {
            if (!ppane->IsHidden()) {
                int ysizeOld = ppane->YSize();

                if (bExpandLeftFirst) {
                    InternalSetExpand(ppane, WinPoint(xsize, ysizeOld + yextra));
                } else {
                    // !!! expand right first
                }
                ppane->UpdateLayout();
                InternalSetOffset(ppane, WinPoint(0, ysize));

                yextra -= ppane->YSize() - ysizeOld;
                ysize += ppane->YSize() + (m_bShowGrid ? 1 : 0);
            }
        }
    }

    if (Child() && m_bShowGrid) {
        ysize--;
    }

    InternalSetSize(WinPoint(xsize, ysize));
}

void ColumnPane::Paint(Surface* psurface)
{
    if (m_bShowGrid) {
        int y = -1;

        for(Pane* ppane = Child();
            ppane != NULL && ppane->Next() != NULL;
            ppane = ppane->Next()
        ) {
            y += ppane->YSize() + (m_bShowGrid ? 1 : 0);

            psurface->FillRect(WinRect(0, y, XSize(), y + 1), Color::Black());
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Row Pane
//
/////////////////////////////////////////////////////////////////////////////

RowPane::RowPane(bool bShowGrid) :
    m_bShowGrid(bShowGrid)
{}

RowPane::RowPane(Pane* ppane0, bool bShowGrid) :
    m_bShowGrid(bShowGrid)
{
    InsertAtBottom(ppane0);
}

RowPane::RowPane(Pane* ppane0, Pane* ppane1, bool bShowGrid) :
    m_bShowGrid(bShowGrid)
{
    InsertAtBottom(ppane0);
    InsertAtBottom(ppane1);
}

void RowPane::ShowGrid(bool bShowGrid)
{
    m_bShowGrid = bShowGrid;
    NeedLayout();
    NeedPaint();
}

void RowPane::UpdateLayout()
{
    int xsize = 0;
    int ysize = 0;
    bool bEvenHeight = false;

    //
    // first calculate the non-x expanded size
    //

    Pane* ppane;
    for(ppane = Child(); ppane != NULL; ppane = ppane->Next()) {
        if (!ppane->IsHidden()) {
            InternalSetExpand(ppane, WinPoint(0, YExpand()));
            ppane->UpdateLayout();
            InternalSetOffset(ppane, WinPoint(xsize, 0));

            xsize += ppane->XSize() + (m_bShowGrid ? 1 : 0);
            ysize = max(ppane->YSize(), ysize);
            bEvenHeight |= ppane->NeedEvenHeight();
        }
    }

    if (bEvenHeight && ((ysize & 1) != 0)) {
        ysize++;
    }

    //
    // If the xsize is less than XExpand() try to expand the child panes
    //

    int xextra = XExpand() - xsize;

    if (xextra > 0 || ysize > YExpand()) {
        bool bExpandLeftFirst = true;

		if (xextra < 0) {
			xextra = 0;
		}

        xsize = 0;
        int ysizeNew = 0;

        for(ppane = Child(); ppane != NULL; ppane = ppane->Next()) {
            if (!ppane->IsHidden()) {
                int xsizeOld = ppane->XSize();

                if (bExpandLeftFirst) {
                    InternalSetExpand(ppane, WinPoint(xsizeOld + xextra, ysize));
                } else {
                    // !!! expand right first
                }
                ppane->UpdateLayout();
                InternalSetOffset(ppane, WinPoint(xsize, 0));

                xextra -= ppane->XSize() - xsizeOld;
                xsize += ppane->XSize() + (m_bShowGrid ? 1 : 0);
                ysizeNew = max(ppane->YSize(), ysizeNew);
            }
        }

        ysize = ysizeNew;
    }

    if (Child() && m_bShowGrid) {
        xsize--;
    }

    InternalSetSize(WinPoint(xsize, ysize));
}

void RowPane::Paint(Surface* psurface)
{
    if (m_bShowGrid) {
        int x = -1;

        for(Pane* ppane = Child();
            ppane != NULL && ppane->Next() != NULL;
            ppane = ppane->Next()
        ) {
            x += ppane->XSize() + 1;

            psurface->FillRect(WinRect(x, 0, x + 1, YSize()), Color::Black());
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// String Pane
//
/////////////////////////////////////////////////////////////////////////////

StringPane::StringPane(const ZString& str, IEngineFont* pfont) :
    m_str(str),
    m_textColor(GetSystemColor(SystemColorWindowText)),
    m_textColorSelected(GetSystemColor(SystemColorHighLightText)),
    m_pfont(pfont),
    m_bClipped(false),
    m_justification(JustifyLeft())
{
}

StringPane::StringPane(
    const ZString& str,
    IEngineFont*   pfont,
    WinPoint       ptSize, 
    Justification  justification
) :
    m_str(str),
    m_textColor(GetSystemColor(SystemColorWindowText)),
    m_textColorSelected(GetSystemColor(SystemColorHighLightText)),
    m_pfont(pfont),
    m_bClipped(true),
    m_ptSize(ptSize),
    m_justification(justification)
{
}

void StringPane::SetFont(IEngineFont* pfont)
{
    m_pfont = pfont;
}

void StringPane::SetTextColor(const Color& color)
{
    if (color != m_textColor) {
        m_textColor = color;

        if (!IsSelected()) {
            NeedPaint();
        }
    }
}

void StringPane::SetTextColorSelected(const Color& color)
{
    if (m_textColorSelected != color) {
        m_textColorSelected = color;

        if (IsSelected()) {
            NeedPaint();
        }
    }
}

void StringPane::Paint(Surface* psurface)
{
    ZString strClipped;

	if (m_bClipped) 
	{
		if (m_justification == JustifyRight() || m_justification == JustifyLeftClipRight())
		{
			strClipped = m_str.Right(GetFont()->GetMaxTextLength(m_str, m_ptSize.X(), false));
		}
		else
		{
			strClipped = m_str.Left(GetFont()->GetMaxTextLength(m_str, m_ptSize.X(), true ));
		}
	} 
	else 
	{
		strClipped = m_str;
	}

	int nXStart;

	if (m_justification == JustifyRight())
	{
		ZAssert(m_bClipped);
		nXStart = m_ptSize.X() - m_pfont->GetTextExtent(strClipped).X();
	} 
	else if (m_justification == JustifyCenter()) 
	{
		ZAssert(m_bClipped);
		nXStart = (m_ptSize.X() - m_pfont->GetTextExtent(strClipped).X())/2;
	}
	else 
	{
		ZAssert(m_justification == JustifyLeft() || m_justification == JustifyLeftClipRight());
		nXStart = 0;
	}

	WinPoint origin = WinPoint( nXStart, 0 ) + psurface->GetOffset();
	m_pfont->DrawString(	origin,
							strClipped, 
							IsSelected() ? m_textColorSelected : m_textColor );

//	psurface->DrawString( m_pfont, IsSelected() ? m_textColorSelected : m_textColor, WinPoint(nXStart, 0),  strClipped );
}

void StringPane::SetString(const ZString& str)
{
    if (str != m_str) {
        m_str = str;
        NeedLayout();
        NeedPaint();
    }
}

void StringPane::SetJustification(Justification justification)
{
    ZAssert(m_bClipped);
    m_justification = justification;
}

void StringPane::UpdateLayout()
{
    if (m_bClipped) {
        InternalSetSize(m_ptSize);
    } else {
        InternalSetSize(m_pfont->GetTextExtent(m_str));
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// EditPane
//
/////////////////////////////////////////////////////////////////////////////

class EditPaneImpl : 
    public EditPane, 
    public Value 
{
protected:
    ZString            m_str;
    ZString            m_strDisplay;
    ZString            m_strClipped;
    Color              m_textColor;
    TRef<IEngineFont>  m_pfont;
    int                m_xsizeCursor;
    bool               m_bFocus;
    bool               m_bReadOnly;
    bool               m_bCursorOn;
    EditPaneType       m_type;
    int                m_maxLength;
    float              m_timeLast;

    TRef<EventSourceImpl>       m_peventSource;
    TRef<StringEventSourceImpl> m_peventSourceZString;

    Number* GetTime() { return (Number*)GetChild(0); }

public:
    EditPaneImpl(const ZString& str, IEngineFont* pfont, Number* ptime) :
        Value(ptime),
        m_pfont(pfont),
        m_textColor(Color::White()),
        m_bFocus(false),
        m_type(Normal),
        m_bReadOnly(false),
        m_maxLength(-1),
        m_timeLast(ptime->GetValue())
    {
        m_xsizeCursor  = pfont->GetTextExtent("o").X();
        m_peventSource        = new EventSourceImpl();
        m_peventSourceZString = new StringEventSourceImpl();

        SetString(str);
    }

    void UpdateStrings()
    {
        if (m_type == Password) {
            m_strDisplay = ZString('*', m_str.GetLength());
        } else {
            m_strDisplay = m_str;
        }

        if (m_bFocus && !m_bReadOnly) {
            m_strClipped =
                m_strDisplay.Right(
                    m_pfont->GetMaxTextLength(
                        m_strDisplay,
                        GetSize().X() - m_xsizeCursor,
                        false
                    )
                );
        } else {
            m_strClipped =
                m_strDisplay.Left(
                    m_pfont->GetMaxTextLength(
                        m_strDisplay,
                        GetSize().X(),
                        true
                    )
                );
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // EditPane methods
    //
    /////////////////////////////////////////////////////////////////////////////

    TRef<IEventSource> GetClickEvent()
    {
        return m_peventSource;
    }

    TRef<IStringEventSource> GetChangeEvent()
    {
        return m_peventSourceZString;
    }

    const ZString& GetString()
    {
        return m_str;
    }

    void SetTextColor(const Color& color)
    {
        m_textColor = color;
        NeedPaint();
    }

    void SetFont(IEngineFont* pfont)
    {
        m_pfont = pfont;
        NeedPaint();
    }

    void SetReadOnly(bool bReadOnly)  
    {
        m_bReadOnly = bReadOnly;
        UpdateStrings();
    }

    bool IsReadOnly()  
    {
        return m_bReadOnly;
    }

    void SetString(const ZString& str)
    {
        if (str != m_str) {
            m_str = str;
            UpdateStrings();
            NeedPaint();
        }
    }

    void SetType(EditPaneType type)
    {
        m_type = type;
        UpdateStrings();
        NeedPaint();
    }

    void SetMaxLength(int length)
    {
        m_maxLength = length;

        if (m_str.GetLength() > m_maxLength) {
            SetString(m_str.Left(m_maxLength));
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // IKeyboardInput methods
    //
    /////////////////////////////////////////////////////////////////////////////

    bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    {
        fForceTranslate = true;
        return false;
    }

    bool OnChar(IInputProvider* pprovider, const KeyState& ks)
    {
        if (m_bReadOnly) 
            return true;

        if (ks.vk == 13) {
            m_peventSourceZString->Trigger(m_str);
        } else if (ks.vk == 8) {
            if (m_str.GetLength() > 0) {
                SetString(m_str.LeftOf(1));
                NeedPaint();
            }
        } else {
            char ch = ks.vk;

            //
            // Unblink the cursor
            //

            m_timeLast = GetTime()->GetValue();

            //
            // Only allow numbers in a Numeric edit pane
            //

            if (m_type == Numeric && (ch < '0' || ch > '9' )) {
                return true;
            }

            //
            // Limit the length
            //

            if (m_maxLength != -1 && m_str.GetLength() == m_maxLength) {
                return true;
            }

            //
            // Everythings ok add the character
            //

            SetString(m_str + ZString(ch, 1));
            NeedPaint();
        }

        return true;
    }

    void SetFocusState(bool bFocus)
    {
        m_bFocus = bFocus;
        UpdateStrings();
        NeedPaint();
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // IMouseInput methods
    //
    /////////////////////////////////////////////////////////////////////////////

    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    {
        if (!m_bReadOnly) 
        {
            if (button == 0) {
                if (bDown) {
                    m_peventSource->Trigger();
                }
            }
        }

        return MouseResult();
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // Pane methods
    //
    /////////////////////////////////////////////////////////////////////////////

    void Paint(Surface* psurface)
    {
//		OutputDebugString( "EditPaneImpl::Paint\n");
//		return;

		//psurface->DrawString(m_pfont, m_textColor, WinPoint(0, 0), m_strClipped);

		WinPoint origin = psurface->GetOffset();
		m_pfont->DrawString(	origin,
								m_strClipped, 
								m_textColor );								

        // Draw the cursor
        WinPoint size = m_pfont->GetTextExtent(m_strClipped);

        if (m_bFocus && m_bCursorOn) {
            psurface->FillRect(
                WinRect(
                    size.X() + 1,
                    1,
                    size.X() + m_xsizeCursor,
                    GetSize().Y() - 2
                ),
                m_textColor
            );
        }
    }

    void UpdateLayout()
    {
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // Value methods
    //
    /////////////////////////////////////////////////////////////////////////////

    void ChildChanged(Value* pvalue, Value* pvalueNew)
    {
        Value::ChildChanged(pvalue, pvalueNew);

        float time = GetTime()->GetValue();
        bool bOn = mod((time - m_timeLast), 1.0f) < 0.5f;
        if (m_bCursorOn != bOn) {
            m_bCursorOn = bOn;
            NeedPaint();
        }
    }
};

TRef<EditPane> CreateEditPane(const ZString& str, IEngineFont* pfont, Number* ptime)
{
    return new EditPaneImpl(str, pfont, ptime);
}

/////////////////////////////////////////////////////////////////////////////
//
// AnimatedImage Pane
//
/////////////////////////////////////////////////////////////////////////////

AnimatedImagePane::AnimatedImagePane(Image* pimage, const Rect& rect) :
    Value(pimage),
    m_rect(rect),
    m_bFixedRect(true),
    m_ysizeSurface(m_rect.YSize())
{
}

AnimatedImagePane::AnimatedImagePane(Image* pimage) :
    Value(pimage),
    m_bFixedRect(false)
{
}

void AnimatedImagePane::ChildChanged(Value* pvalue, Value* pvalueNew)
{
    Value::ChildChanged(pvalue, pvalueNew);

    NeedPaint();
    NeedLayout();
}

void AnimatedImagePane::UpdateLayout()
{
    Update();

    if (!m_bFixedRect) {
        m_rect         = GetImage()->GetBounds().GetRect();
        m_ysizeSurface = m_rect.YSize();
    }

    InternalSetSize(WinPoint::Cast(m_rect.Size()));

    Pane::UpdateLayout();
}

void AnimatedImagePane::Paint(Surface* psurface)
{
    if (XSize() > 0 && YSize() > 0) {
        Update();
        Point offset = Point::Cast(GetOffsetFrom(NULL));
        m_ysizeSurface = float(psurface->GetSize().Y());

        TRef<Context> pcontext = psurface->GetContext();

        if (pcontext) {
            pcontext->Clip(
                Rect(
                    offset.X(),
                    m_ysizeSurface - (offset.Y() + YSize()),
                    offset.X() + XSize(),
                    m_ysizeSurface - offset.Y()
                )
            );
            pcontext->Translate(
                Point(
                    offset.X(),
                    m_ysizeSurface - offset.Y() - m_rect.YMax()
                )
            );

            GetImage()->Render(pcontext);

            psurface->ReleaseContext(pcontext);
        }
    }
}

Point AnimatedImagePane::GetImagePoint(const Point& point)
{
    //
    // Flip the coordinates
    //

    float x = point.X();
    float y = (float)GetSize().Y() - 1.0f - point.Y();

    //
    // Translate
    //
    //  
    //     There is a hack here, we are using the saved surface size
    //     from the last time that we painted the window
    //


    //Point offset = Point::Cast(GetOffsetFrom(NULL));

    return m_rect.Min() + Point(x, y);
        /*
        Point(
            x + m_rect.XMin() + offset.X(),
            y + m_ysizeSurface - (float)offset.Y() - m_rect.YMax()
        );
        */
}

MouseResult AnimatedImagePane::HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
{
    Point pointImage = GetImagePoint(point);

    if (m_bFixedRect) {
        if (!m_rect.Inside(pointImage)) {
            return MouseResult();
        }
    } 

    return GetImage()->HitTest(pprovider, GetImagePoint(point), bCaptured);
}

void AnimatedImagePane::MouseMove(IInputProvider* pprovider, const Point& point, bool bCaptured, bool bInside)
{
    GetImage()->MouseMove(pprovider, GetImagePoint(point), bCaptured, bInside);
}

void AnimatedImagePane::MouseEnter(IInputProvider* pprovider, const Point& point)
{
    GetImage()->MouseEnter(pprovider, GetImagePoint(point));
}

void AnimatedImagePane::MouseLeave(IInputProvider* pprovider)
{
    GetImage()->MouseLeave(pprovider);
}

MouseResult AnimatedImagePane::Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
{
    return GetImage()->Button(pprovider, GetImagePoint(point), button, bCaptured, bInside, bDown);
}

/////////////////////////////////////////////////////////////////////////////
//
// Image Pane
//
/////////////////////////////////////////////////////////////////////////////

ImagePane::ImagePane(Image* pimage) :
    Value(pimage)
{
}

void ImagePane::ChildChanged(Value* pvalue, Value* pvalueNew)
{
    Value::ChildChanged(pvalue, pvalueNew);

    NeedPaint();
    NeedLayout();
}

void ImagePane::UpdateLayout()
{
    Update();

    InternalSetSize(
        WinPoint::Cast(GetImage()->GetBounds().GetRect().Size())
    );
    Pane::UpdateLayout();
}

void ImagePane::Paint(Surface* psurface)
{
    TRef<Surface> psurfaceSource = GetImage()->GetSurface();

    psurface->BitBlt(WinPoint(0, 0), psurfaceSource);
}

/////////////////////////////////////////////////////////////////////////////
//
// Surface Pane
//
/////////////////////////////////////////////////////////////////////////////

SurfacePane::SurfacePane(Surface* psurface) :
    m_psurface(psurface)
{
    InternalSetSize(psurface->GetSize());
}

void SurfacePane::Paint(Surface* psurface)
{
    psurface->BitBlt(WinPoint(0, 0), m_psurface);
}

void SurfacePane::SetSurface(Surface* psurface)
{
    m_psurface = psurface;
    if (m_psurface->GetSize() != GetSize()) {
        NeedLayout();
    }
    NeedPaint();
}

/////////////////////////////////////////////////////////////////////////////
//
// TroughPane
//
/////////////////////////////////////////////////////////////////////////////

typedef WinPoint (PFNMakePoint)(int x, int y);

WinPoint MakePoint(int x, int y)        
{ 
    return WinPoint(x, y); 
}

WinPoint MakePointFlipped(int x, int y) 
{ 
    return WinPoint(y, x); 
}

class TroughPane : public Pane, IEventSink {
private:
    TRef<IntegerEventSourceImpl> m_peventSource;
    TRef<Pane>                   m_ppaneThumb;
    TRef<ButtonPane>             m_ppaneLeft;
    TRef<ButtonPane>             m_ppaneRight;
    TRef<IEventSource>           m_peventSourceLeft;
    TRef<IEventSource>           m_peventSourceRight;
    TRef<IEventSink>             m_psinkLeft;
    TRef<IEventSink>             m_psinkRight;
    bool                         m_bHorizontal;
    bool                         m_bICaptured;
    int                          m_size;
    int                          m_sizePage;
    int                          m_sizeLine;
    int                          m_pos;
    int                          m_sizeThumb;
    int                          m_sizePixels;
    Point                        m_pointTrack;

public:
    /////////////////////////////////////////////////////////////////////////////
    //
    // Constructor
    //
    /////////////////////////////////////////////////////////////////////////////

    TroughPane(
        Pane* ppaneThumb,
        Pane* ppaneLeft,
        Pane* ppaneRight,
        bool bHorizontal,
        float repeatRate,
        int size,
        int sizePage,
        int sizeLine,
        int pos
    ) :
        m_ppaneThumb(ppaneThumb),
        m_bHorizontal(bHorizontal),
        m_size(size),
        m_sizePage(sizePage),
        m_sizeLine(sizeLine),
        m_pos(0),
        m_peventSource(new IntegerEventSourceImpl()),
        m_bICaptured(false)
    {
        //  , should pass the buttons in and have ScrollPane handle the events

        m_ppaneLeft  = CreateButton(ppaneLeft,  NULL, false, repeatRate);
        m_ppaneRight = CreateButton(ppaneRight, NULL, false, repeatRate);

        m_peventSourceLeft  = m_ppaneLeft->GetEventSource();
        m_peventSourceRight = m_ppaneRight->GetEventSource();

        m_peventSourceLeft->AddSink( IEventSink::CreateDelegate(this));
        m_peventSourceRight->AddSink(IEventSink::CreateDelegate(this));

        InsertAtBottom(m_ppaneLeft);
        InsertAtBottom(m_ppaneThumb);
        InsertAtBottom(m_ppaneRight);

        SetPos(pos);
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // Destructor
    //
    /////////////////////////////////////////////////////////////////////////////

    ~TroughPane()
    {
        m_peventSourceLeft->RemoveSink( m_psinkLeft  = IEventSink::CreateDelegate(this));
        m_peventSourceRight->RemoveSink(m_psinkRight = IEventSink::CreateDelegate(this));
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // TroughPane Methods
    //
    /////////////////////////////////////////////////////////////////////////////

    void SetSizes(int size, int sizePage)
    {
        ZAssert(size >= 0);
        ZAssert(sizePage >= 0);

        if (
               m_size != size
            || m_sizePage != sizePage
        ) {
            m_size     = size;
            m_sizePage = sizePage;

            //
            // ensure m_pos is still within proper bounds
            //

            SetPos(m_pos);

            NeedLayout();
            NeedPaint();
        }
    }

    void SetLineSize(int sizeLine)
    {
        if (m_sizeLine != sizeLine) {
            m_sizeLine = sizeLine;

            NeedLayout();
            NeedPaint();
        }
    }

    void SetPos(int pos)
    {
        if (pos < 0) {
		    pos = 0;
	    }

        if (pos > m_size - m_sizePage) {
		    pos = max(m_size - m_sizePage, 0);
	    }

        if (m_pos != pos) {
             m_pos = pos;
             NeedLayout();
             NeedPaint();
             m_peventSource->Trigger(m_pos);
        }
    }

    void SetMousePos(int x)
    {
        if (m_sizePixels - m_sizeThumb != 0) {
            SetPos(x * (m_size - m_sizePage) / (m_sizePixels - m_sizeThumb));
        } else {
            SetPos(0);
        }
    }

    void PageUp()
    {
        SetPos(m_pos - m_sizePage);
    }

    void PageDown()
    {
        SetPos(m_pos + m_sizePage);
    }

    void LineUp()
    {
        SetPos(m_pos - m_sizeLine);
    }

    void LineDown()
    {
        SetPos(m_pos + m_sizeLine);
    }

    IIntegerEventSource* GetEventSource()
    {
        return m_peventSource;
    }

    int  GetSize()     { return m_size;        }
    int  GetPageSize() { return m_sizePage;    }
    int  GetLineSize() { return m_sizeLine;    }
    int  GetPos()      { return m_pos;         }
    bool IsHorizontal(){ return m_bHorizontal; }

    /////////////////////////////////////////////////////////////////////////////
    //
    // Pane methods
    //
    /////////////////////////////////////////////////////////////////////////////

    void UpdateLayout()
    {
        InternalSetSize(GetExpand());

        //
        // Set the thumb size
        //

        PFNMakePoint* pfnMakePoint;
        int sizePixelsOther;

        if (m_bHorizontal) {
            pfnMakePoint    = MakePoint;
            m_sizePixels    = XSize();
            sizePixelsOther = YSize();
        } else {
            pfnMakePoint    = MakePointFlipped;
            m_sizePixels    = YSize();
            sizePixelsOther = XSize();
        }

        if (m_sizePage >= m_size) {
            m_sizeThumb = m_sizePixels;
        } else if (m_sizePage == 0) {
            m_sizeThumb = sizePixelsOther;
        } else {
            m_sizeThumb = m_sizePixels * m_sizePage / m_size;
        }

        InternalSetExpand(
            m_ppaneThumb,
            pfnMakePoint(m_sizeThumb, sizePixelsOther)
        );

        m_ppaneThumb->UpdateLayout();

        int otherThumb;

        if (m_bHorizontal) {
            m_sizeThumb  = m_ppaneThumb->XSize();
            otherThumb   = m_ppaneThumb->YSize();
        } else {
            m_sizeThumb  = m_ppaneThumb->YSize();
            otherThumb   = m_ppaneThumb->XSize();
        }

        //
        // Adjust the left and right trough
        //

        int left;
        int right;

        if (m_sizeThumb > m_sizePixels) {
            left  = 0;
            right = 0;
        } else {
            if (m_sizePage >= m_size) {
                left = 0;
            } else {
                left = m_pos * (m_sizePixels - m_sizeThumb) / (m_size - m_sizePage);
            }

            right = m_sizePixels - m_sizeThumb - left;
        }

        InternalSetOffset(m_ppaneThumb, pfnMakePoint(left, 0));

        //
        // left and right trough
        //

        if (left == 0) {
            InternalSetHidden(m_ppaneLeft, true);
        } else {
            InternalSetHidden(m_ppaneLeft, false);
            InternalSetExpand(m_ppaneLeft, pfnMakePoint(left, otherThumb));
            m_ppaneLeft->UpdateLayout();
        }

        if (right == 0) {
            InternalSetHidden(m_ppaneRight, true);
        } else {
            InternalSetHidden(m_ppaneRight, false);
            InternalSetExpand(m_ppaneRight, pfnMakePoint(right, otherThumb));
            m_ppaneRight->UpdateLayout();
            InternalSetOffset(m_ppaneRight, pfnMakePoint(left + m_sizeThumb, 0));
        }

        InternalSetSize(pfnMakePoint(left + m_sizeThumb + right, otherThumb));
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // IMouseInput methods
    //
    /////////////////////////////////////////////////////////////////////////////

    void MouseMove(IInputProvider* pprovider, const Point& point, bool bCaptured, bool bInside)
    {
        if (bCaptured && GetCapturePane() == NULL) {
            if (m_bHorizontal) {
                SetMousePos((int)(point.X() - m_pointTrack.X()));
            } else {
                SetMousePos((int)(point.Y() - m_pointTrack.Y()));
            }
        }
    }

    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    {
        if (button == 0) {
            if (bDown) {
                Pane* ppaneHit = GetHitPane();

                if (ppaneHit == m_ppaneThumb) {
                    m_pointTrack = point - Point::Cast(ppaneHit->GetOffset());
                    return MouseResultCapture();
                }
            } else if (bCaptured && GetCapturePane() == NULL) {
                return MouseResultRelease();
            }
        }

        return Pane::Button(pprovider, point, button, bCaptured, bInside, bDown);
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // IEventSink methods
    //
    /////////////////////////////////////////////////////////////////////////////

    bool OnEvent(IEventSource* pevent)
    {
        if (pevent == m_peventSourceLeft) {
            PageUp();
        } else if (pevent == m_peventSourceRight) {
            PageDown();
        }

        return true;
    }
};

/////////////////////////////////////////////////////////////////////////////
//
// ScrollPane
//
/////////////////////////////////////////////////////////////////////////////

class ScrollPaneImpl : 
    public ScrollPane,
    public IEventSink
{
public:
    TRef<IEventSource> m_peventLeft;
    TRef<IEventSource> m_peventRight;
    TRef<TroughPane>   m_ptroughPane;
    TRef<IEventSink>   m_psinkLeft;
    TRef<IEventSink>   m_psinkRight;
    WinPoint           m_sizeMin;

    ScrollPaneImpl(
        Pane* ppaneBackground,
        Pane* ppaneThumb,
        Pane* ppaneLeft,
        IEventSource* peventLeft,
        Pane* ppaneRight,
        IEventSource* peventRight,
        bool bHorizontal,
        float repeatRatePage,
        int size,
        int sizePage,
        int sizeLine,
        int pos,
        const WinPoint sizeMin
    ) :
        m_peventLeft(peventLeft),
        m_peventRight(peventRight),
        m_sizeMin(sizeMin)
    {
        m_peventLeft->AddSink( m_psinkLeft  = IEventSink::CreateDelegate(this));
        m_peventRight->AddSink(m_psinkRight = IEventSink::CreateDelegate(this));

        //
        // Create the trough
        //

        ppaneBackground->InsertAtTop(
            m_ptroughPane = new TroughPane(
                ppaneThumb,
                //new BorderPane(0, RGB(255, 0, 0)),
                //new BorderPane(0, RGB(0, 255, 0)),
                new JustifyPane(),
                new JustifyPane(),
                bHorizontal,
                repeatRatePage,
                size,
                sizePage,
                sizeLine,
                pos
            )
        );

        //
        // stick the buttons and the trough together
        //

        TRef<Pane> prow;

        if (bHorizontal) {
            prow = new RowPane();
        } else {
            prow = new ColumnPane();
        }

        prow->InsertAtBottom(ppaneLeft);
        prow->InsertAtBottom(ppaneBackground);
        prow->InsertAtBottom(ppaneRight);

        InsertAtTop(prow);
    }

    ~ScrollPaneImpl()
    {
        m_peventLeft-> RemoveSink(m_psinkLeft );
        m_peventRight->RemoveSink(m_psinkRight);
    }

    IIntegerEventSource* GetEventSource()
    {
        return m_ptroughPane->GetEventSource();
    }

    int  GetSize()     { return m_ptroughPane->GetSize();     }
    int  GetPageSize() { return m_ptroughPane->GetPageSize(); }
    int  GetLineSize() { return m_ptroughPane->GetLineSize(); }
    int  GetPos()      { return m_ptroughPane->GetPos();      }
    bool IsHorizontal(){ return m_ptroughPane->IsHorizontal();}

    void SetSizes(int size, int sizePage)
    {
        m_ptroughPane->SetSizes(size, sizePage);
    }

    void SetSize(int size)
    {
        m_ptroughPane->SetSizes(size, m_ptroughPane->GetPageSize());
    }

    void SetPageSize(int sizePage)
    {
        m_ptroughPane->SetSizes(m_ptroughPane->GetSize(), sizePage);
    }

    void SetLineSize(int sizeLine)
    {
        m_ptroughPane->SetLineSize(sizeLine);
    }

    void SetPos(int pos)
    {
        m_ptroughPane->SetPos(pos);
    }

    void PageUp()
    {
        m_ptroughPane->PageUp();
    }

    void PageDown()
    {
        m_ptroughPane->PageDown();
    }

    void LineUp()
    {
        m_ptroughPane->LineUp();
    }

    void LineDown()
    {
        m_ptroughPane->LineDown();
    }

    //
    // IEventSink methods
    //

    bool OnEvent(IEventSource* pevent)
    {
        if (pevent == m_peventLeft) {
            LineUp();
        } else if (pevent == m_peventRight) {
            LineDown();
        }

        return true;
    }

    //
    // Value Methods
    //

    void UpdateLayout()
    {
        InternalSetExpand(
            WinPoint(
                max(m_sizeMin.X(), GetExpand().X()),
                max(m_sizeMin.Y(), GetExpand().Y())
            )
        );

        DefaultUpdateLayout();
    }
};

/////////////////////////////////////////////////////////////////////////////
//
// Easy ScrollPane Constructor
//
/////////////////////////////////////////////////////////////////////////////

TRef<ScrollPane> CreateScrollPane(
    Pane* ppaneBackground,
    Pane* ppaneThumb,
    Pane* ppaneLeft,
    IEventSource* peventLeft,
    Pane* ppaneRight,
    IEventSource* peventRight,
    bool bHorizontal,
    float repeatRatePage,
    int size,
    int sizePage,
    int sizeLine,
    int pos,
    const WinPoint sizeMin
) {
    return
        new ScrollPaneImpl(
            ppaneBackground,
            ppaneThumb,
            ppaneLeft,
            peventLeft,
            ppaneRight,
            peventRight,
            bHorizontal,
            repeatRatePage,
            size,
            sizePage,
            sizeLine,
            pos,
            sizeMin
        );
}

TRef<Pane> CreateScrollPane(
    const WinPoint& point,
    int size,
    int pageSize,
    int lineSize,
    int pos,
    TRef<ScrollPane>& pscrollPane,
    TRef<IIntegerEventSource>& pevent
) {
    bool bHorizontal = true;
    int buttonSize = point.Y();

    if (point.X() < buttonSize) {
        bHorizontal = false;
        buttonSize = point.X();
    }

    TRef<ButtonPane> pbuttonUp   = CreateButton(buttonSize);
    TRef<ButtonPane> pbuttonDown = CreateButton(buttonSize);

    pscrollPane =
        CreateScrollPane(
            new JustifyPane(),
			new EdgePane(
				new BorderPane(0, Color(0.5, 0.5, 0.5))
			),
            pbuttonUp,
            pbuttonUp->GetEventSource(),
            pbuttonDown,
            pbuttonDown->GetEventSource(),
            bHorizontal,
            0.25f,
            size,
            pageSize,
            lineSize,
            pos
        );

    pevent = pscrollPane->GetEventSource();

    return
        new BorderPane(
            0,
            Color(0.5, 0.5, 0.5),
            new EdgePane(
                new JustifyPane(
                    JustifyPane::Center,
                    point,
                    pscrollPane
                )
            )
        );
}
