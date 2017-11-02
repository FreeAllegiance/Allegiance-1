#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class JoystickImageImpl : public JoystickImage {
private:
    TArray<TRef<ModifiableNumber> , 20> m_ppnumber; //imago increased 8/12/09, again 7/10
    TArray<TRef<ModifiableBoolean>, 20> m_ppboolButton; //imago increased 8/12/09, again 7/10
    bool                               m_bJoystickEnabled;
    bool                               m_bButtonsEnabled;
    bool                               m_bJustEnabled;
    float                              m_sensitivity;

public:
    JoystickImageImpl(float sensitivity) :
        m_sensitivity(sensitivity),
        m_bJoystickEnabled(false),
        m_bButtonsEnabled(false),
        m_bJustEnabled(false)
    {

		//Imago 7/10
        for (int index = 0; index < m_ppboolButton.GetCount(); index++) {
            m_ppnumber[index] = new ModifiableNumber(false);
        }

        for (int index = 0; index < m_ppboolButton.GetCount(); index++) {
            m_ppboolButton[index] = new ModifiableBoolean(false);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // JoystickImage members
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetEnabled(bool bJoystickEnabled, bool bButtonsEnabled)
    {
        if (m_bJoystickEnabled != bJoystickEnabled) {
            m_bJoystickEnabled = bJoystickEnabled;

            if (m_bJoystickEnabled) {
                m_bJustEnabled = true;
            } else {
                m_ppnumber[0]->SetValue(0);
                m_ppnumber[1]->SetValue(0);
                m_ppnumber[2]->SetValue(0);
            }
        }

        if (m_bButtonsEnabled != bButtonsEnabled) {
            m_bButtonsEnabled = bButtonsEnabled;

            if (!m_bButtonsEnabled) {
                for (int index = 0; index < m_ppboolButton.GetCount(); index++) {
                    m_ppboolButton[index]->SetValue(false);
                }
            }
        }
    }

    void SetSensitivity(float sensitivity)
    {
        m_sensitivity = sensitivity;
    }

    bool GetJoystickEnabled()
    {
        return m_bJoystickEnabled;
    }

    bool GetButtonsEnabled()
    {
        return m_bButtonsEnabled;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // JoystickInputStream members
    //
    //////////////////////////////////////////////////////////////////////////////

    bool HasForceFeedback()
    {
        return false;
    }
	//Imago 7/10 #187
	void SetRanges() {

	}
    void CreateEffects()
    {
    }

    void PlayFFEffect(short effectID, LONG lDirection)
    {
    }

    ZString GetShortDescription(int index)
    {
        return "Mouse";
    }

    ZString GetDescription()
    {
        return "Mouse";
    }

    ZString GetValueDescription(int id)
    {
        switch(id) {
            case 0: return "Left";
            case 1: return "Right";
            case 2: return "Middle";
            case 3: return "XButton1";
            case 4: return "XButton2";
            case 5: return "XButton3";
            case 6: return "XButton4";
            case 7: return "XButton5";
            case 8: return "Wheel Up";
            case 9: return "Wheel Down";
        }

        return ZString();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // InputStream members
    //
    //////////////////////////////////////////////////////////////////////////////

    int GetValueCount()
    {
        return 3; //Imago 8/13/09 was 2
    }

    int GetButtonCount()
    {
        return 10; //Imago 8/13/09 was 3
    }

    Boolean* IsDown(int id)
    {
        return m_ppboolButton[id];
    }

    Number* GetValue(int id)
    {
        return m_ppnumber[id];
    }

    ButtonEvent::Source* GetEventSource()
    {
        ZUnimplemented();
        return NULL;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // IMouseInput members
    //
    //////////////////////////////////////////////////////////////////////////////

    MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
    { 
        return (m_bJoystickEnabled || m_bButtonsEnabled) ? MouseResultHit() : MouseResult(); 
    }

    void MouseMove(IInputProvider* pprovider, const Point& pointMouse, bool bCaptured, bool bInside)
    {
        if (m_bJoystickEnabled) {
            if (m_bJustEnabled) {
                m_bJustEnabled = false;
            } else {
                float dx = (pointMouse.X() - 320) * m_sensitivity;
                float dy = (pointMouse.Y() - 240) * m_sensitivity;

                Point 
                    point(
                        m_ppnumber[0]->GetValue() - dx, 
                        m_ppnumber[1]->GetValue() - dy
                    );

                float length = point.Length();

                if (length > 1) {
                    point = point / length;
                }
                
                m_ppnumber[0]->SetValue(point.X());
                m_ppnumber[1]->SetValue(point.Y());
            }

            pprovider->SetCursorPos(Point(320, 240));
        }
    }

    //NYI WheelMove Imago m_ppnumber[2]

    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    {
        if (m_bButtonsEnabled) {
            if (button <= 20) { //was 3 Imago 8/13/09, 7/10
                m_ppboolButton[button]->SetValue(bDown);
            }
        }

        return MouseResult();
    }
};

TRef<JoystickImage> CreateJoystickImage(float sensitivity)
{
    return new JoystickImageImpl(sensitivity);
}

//////////////////////////////////////////////////////////////////////////////
//
// BlendImage
//
//////////////////////////////////////////////////////////////////////////////

class BlendColorImage : public WrapImage {
private:
    ColorValue* GetColor() { return ColorValue::Cast(GetChild(1)); }

public:
    BlendColorImage(Image* pimage, ColorValue* pcolor) :
        WrapImage(pimage, pcolor)
    {
    }

    void Render(Context* pcontext)
    {
        WrapImage::Render(pcontext);
        pcontext->SetBlendMode(BlendModeSourceAlpha);
        pcontext->FillRect(GetBounds().GetRect(), GetColor()->GetValue());
    }
};

TRef<Image> CreateBlendColorImage(Image* pimage, ColorValue* pcolor)
{
    return new BlendColorImage(pimage, pcolor);
}

//////////////////////////////////////////////////////////////////////////////
//
// String Grid Image
//
//////////////////////////////////////////////////////////////////////////////

class StringGridImageImpl : public StringGridImage {
private:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Data members
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<IEngineFont>          m_pfont;
    TVector<TVector<ZString> > m_vvstr;
    TVector<Color>             m_vcolor;
    TVector<float>             m_vColumn;
    float                      m_ysizeRow; 

public:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Constructor
    //
    //////////////////////////////////////////////////////////////////////////////

    StringGridImageImpl(int columns, int rows, IEngineFont* pfont) :
        m_pfont(pfont),
        m_vvstr(rows),
        m_vcolor(rows),
        m_vColumn(columns)
    {
        for (int index = 0; index < rows; index++) {
            m_vvstr.Get(index).SetCount(columns);
        }

        m_ysizeRow = (float)(m_pfont->GetTextExtent("W").Y());
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // StringGrid methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetString(int row, int column, const ZString& str)
    {
        if (m_vvstr.Get(row).Get(column) != str) {
            m_vvstr.Get(row).Set(column, str);
            Changed();
        }
    }

    void SetColor(int row, const Color& color)
    {
        if (m_vcolor.Get(row) != color) {
            m_vcolor.Set(row, color);
            Changed();
        }
    }

    void SetSize(int rows, int columns)
    {
        m_vvstr.SetCount(rows);
        m_vcolor.SetCount(rows);
        m_vColumn.SetCount(columns);

        for (int index = 0; index < rows; index++) {
            m_vvstr.Get(index).SetCount(columns);
        }

        Changed();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Image methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void CalcBounds()
    {
        int rows    = m_vvstr.GetCount();
        int columns = m_vvstr[0].GetCount();

        float xsize = 0;

        for (int column = 0; column < columns; column++) {
            m_vColumn.Set(column, xsize);

            float xsizeColumn = 0;
            for (int row = 0; row < rows; row++) {
                WinPoint size = m_pfont->GetTextExtent(m_vvstr[row][column]);
                if ((float)size.X() > xsizeColumn) {
                    xsizeColumn = (float)size.X();
                }
            }

            xsize += xsizeColumn + 4;
        }    

        m_bounds.SetRect(
            Rect(
                0, 
                (float)(-(rows * m_ysizeRow)), 
                (float)xsize, 
                0
            )
        );
    }

    void Render(Context* pcontext)
    {
        int rows    = m_vvstr.GetCount();
        int columns = m_vvstr[0].GetCount();
        
        for (int row = 0; row < rows; row++) {
            float y = -(m_ysizeRow * (float)(row + 1));

            for (int column = 0; column < columns; column++) {
                pcontext->DrawString(
                    m_pfont, 
                    m_vcolor[row],
                    Point(
                        m_vColumn[column],
                        y
                    ),
                    m_vvstr[row][column]
                );
            }
        }    
    }
};

TRef<StringGridImage> CreateStringGridImage(int columns, int rows, IEngineFont* pfont)
{
    return new StringGridImageImpl(columns, rows, pfont);
}

//////////////////////////////////////////////////////////////////////////////
//
// Muzzle Flare
//
//////////////////////////////////////////////////////////////////////////////

class MuzzleFlareImageImpl : public MuzzleFlareImage {
    class FlareData {
    public:
        float m_time;
        Point m_point;
        bool  m_bRendered;
    };

    TList<FlareData, DefaultNoEquals> m_list;
    TRef<Image>      m_pimage;
    bool             m_bVisible;

    Number* GetTime() { return Number::Cast(GetChild(0)); }

public:
    MuzzleFlareImageImpl(Modeler* pmodeler, Number* ptime) :
        MuzzleFlareImage(ptime),
        m_bVisible(true)
    {
        m_pimage = pmodeler->LoadImage(AWF_EFFECT_MUZZLE_FLARE, true);
    }

    void AddFlare(const Point& point, float duration)
    {
        m_list.PushFront();

        m_list.GetFront().m_point     = point;
        m_list.GetFront().m_time      = GetTime()->GetValue() + duration;
        m_list.GetFront().m_bRendered = false;
    }

    void SetVisible(bool bVisible)
    {
        m_bVisible = bVisible;
    }

    void Evaluate()
    {
        float time = GetTime()->GetValue();

        TList<FlareData, DefaultNoEquals>::Iterator iter(m_list);

        while (!iter.End()) {
            if (iter.Value().m_bRendered && time > iter.Value().m_time) {
                iter.Remove();
            } else {
                iter.Next();
            }
        }

        MuzzleFlareImage::Evaluate();
    }

    void Render(Context* pcontext)
    {
        /*
        if (m_bVisible) {
            pcontext->SetBlendMode(BlendModeAdd);
            pcontext->SetShadeMode(ShadeModeGouraud);

            TList<FlareData, DefaultNoEquals>::Iterator iter(m_list);

            while (!iter.End()) {
                iter.Value().m_bRendered = true;

                pcontext->PushState();
                pcontext->PreTranslate(pcontext->TransformNDCToImage(iter.Value().m_point));
                pcontext->PreScale(10);
                pcontext->DrawImage(m_pimage->GetSurface(), true);
                pcontext->PopState();

                iter.Next();
            }
        }
        */
    }
};

TRef<MuzzleFlareImage> CreateMuzzleFlareImage(Modeler* pmodeler, Number* ptime)
{
    return new MuzzleFlareImageImpl(pmodeler, ptime);
}

//////////////////////////////////////////////////////////////////////////////
//
// PosterImage
//
//////////////////////////////////////////////////////////////////////////////

class PosterImageImpl : public PosterImage {
    class PosterData {
    public:
        Vector      m_vec;
        TRef<Image> m_pimage;
        float       m_scale;
        Matrix      m_mat;
    };

    TList<PosterData, DefaultNoEquals> m_list;

    Viewport*  GetViewport() { return Viewport::Cast(GetChild(0));  }
    Camera*    GetCamera()   { return GetViewport()->GetCamera();   }
    RectValue* GetViewRect() { return GetViewport()->GetViewRect(); }

public:
    PosterImageImpl(Viewport* pviewport) :
        PosterImage(pviewport)
    {
    }

    void AddPoster(Image* pimage, const Vector& vec, float scale)
    {
        m_list.PushFront();

        m_list.GetFront().m_pimage   = pimage;
        m_list.GetFront().m_vec      = vec;
        m_list.GetFront().m_scale    = scale;

        m_list.GetFront().m_mat.SetScale(scale);
        m_list.GetFront().m_mat.LookAtFrom(Vector(0, 0, 0), vec * 400.0f, Vector(0, 1, 0));
    }

    void Render(Context* pcontext)
    {
        Rect rect = GetViewRect()->GetValue();
        pcontext->Translate(rect.Center());
        pcontext->Scale2(0.5f * rect.XSize());
        pcontext->Begin3DLayer(GetCamera(), false);

        #ifndef FixPermedia
            pcontext->SetShadeMode(ShadeModeCopy);
        #endif
        pcontext->SetShadeMode(ShadeModeGlobalColor);
		pcontext->SetGlobalColor( Color::White() );
		pcontext->SetBlendMode( BlendModeSourceAlpha );

        TList<PosterData, DefaultNoEquals>::Iterator iter(m_list);

        while (!iter.End()) {
            PosterData& data = iter.Value();

            Point point;
			if (pcontext->TransformDirectionToImage(data.m_vec, point)) {
                pcontext->SetTexture(data.m_pimage->GetSurface());
                pcontext->PushState();
                pcontext->Multiply(data.m_mat);
                #ifdef FixPermedia
                    Geo::GetWhiteEmissiveSquare()->Render(pcontext);
                #else
                    Geo::GetSquare()->Render(pcontext);
                #endif
                pcontext->PopState();
            }

            iter.Next();
        }

        pcontext->End3DLayer();
    }
};

TRef<PosterImage> CreatePosterImage(Viewport* pviewport)
{
    return new PosterImageImpl(pviewport);
}

//////////////////////////////////////////////////////////////////////////////
//
// LensFlareImage
//
//////////////////////////////////////////////////////////////////////////////

class LensFlareData {
public:
    float m_pos;
    float m_scale;
    Color m_color;
	int   m_index;

	LensFlareData(int index, float pos, float scale, const Color& color) :
		m_pos(pos),
		m_scale(scale),
		m_color(color),
		m_index(index)
	{
	}

	LensFlareData(const LensFlareData& data) :
		m_pos(data.m_pos),
		m_scale(data.m_scale),
		m_color(data.m_color),
		m_index(data.m_index)
	{
	}
};

LensFlareData g_lensFlareData[] =
    {
        LensFlareData(2, -1.00, 1.50, Color::White()),
        LensFlareData(0, -0.75, 1.00, Color::Blue()  ),
        LensFlareData(0, -0.50, 0.50, Color::Blue()  ),
        LensFlareData(0, -0.25, 0.25, Color::Blue()  ),

        LensFlareData(0,  0.25, 0.25, Color::Blue()  ),
        LensFlareData(0,  0.50, 0.50, Color::Blue()  ),
        LensFlareData(0,  0.75, 1.00, Color::Blue()  ),

        LensFlareData(1,  1.00, 1.50, Color::White() )
    };

class LensFlareImageImpl : public LensFlareImage {
    TRef<Surface> m_psurfaces[3];
    Vector        m_vec;

    Viewport*  GetViewport() { return Viewport::Cast(GetChild(0));  }
    Camera*    GetCamera()   { return GetViewport()->GetCamera();   }
    RectValue* GetViewRect() { return GetViewport()->GetViewRect(); }


public:
    LensFlareImageImpl(Modeler* pmodeler, Viewport* pviewport) :
        LensFlareImage(pviewport),
        m_vec(0, 0, 1)
    {
/*        m_psurfaces[0] =
            pmodeler
                ->LoadImage(AWF_EFFECT_LENS_FLARE_MID_RING, true)
                ->GetSurface();

        m_psurfaces[1] =
            pmodeler
                ->LoadImage(AWF_EFFECT_LENS_FLARE_STAR_AURA, true)
                ->GetSurface();

        m_psurfaces[2] =
            pmodeler
                ->LoadImage(AWF_EFFECT_LENS_FLARE_END_RING, true)
                ->GetSurface();*/
        m_psurfaces[0] =
            pmodeler
                ->LoadImage(AWF_EFFECT_LENS_FLARE_MID_RING, false)
                ->GetSurface();

        m_psurfaces[1] =
            pmodeler
                ->LoadImage(AWF_EFFECT_LENS_FLARE_STAR_AURA, false)
                ->GetSurface();

        m_psurfaces[2] =
            pmodeler
                ->LoadImage(AWF_EFFECT_LENS_FLARE_END_RING, false)
                ->GetSurface();
    }

    void SetLightDirection(const Vector& vec)
    {
        m_vec = vec;
    }

/*    void Render(Context* pcontext)
    {
        m_vec = Vector(0, 0, 1);

        const Rect& rect        = GetViewRect()->GetValue();
              Point pointCenter = rect.Center();
              float scale       = rect.XSize() / 800.0f;

        Point pointLight;
		if (GetCamera()->TransformDirectionToImage(m_vec, pointLight)) {
            pointLight = rect.TransformNDCToImage(pointLight);

            if (rect.Inside(pointLight)) {
                pointLight = pointLight - pointCenter;

				pcontext->SetBlendMode(BlendModeAdd);
				pcontext->SetShadeMode(ShadeModeFlat);
                pcontext->Translate(pointCenter);

				int count = ArrayCount(g_lensFlareData);

				for(int index = 0; index < count; index++) {
					pcontext->PushState();

					pcontext->Translate(pointLight * g_lensFlareData[index].m_pos);
                    pcontext->Scale2(scale * g_lensFlareData[index].m_scale);

					float angle = 0;
					int indexSurface = g_lensFlareData[index].m_index;

					if (indexSurface == 1) {
						pcontext->Rotate((pointLight.X() + pointLight.Y()) / 200.0f);
					}

                    pcontext->DrawImage3D(m_psurfaces[indexSurface], g_lensFlareData[index].m_color, true);

					pcontext->PopState();
				}
            }
		}
    }*/

	// Updated version of the lens flare render.
    void Render( Context * pcontext )
    {
        m_vec = Vector(0, 0, 1);

        const Rect& rect        = GetViewRect()->GetValue();
              Point pointCenter = rect.Center();
              float scale       = rect.XSize() / 800.0f;

        Point pointLight;
		if( GetCamera()->TransformDirectionToImage( m_vec, pointLight )) 
		{
            pointLight = rect.TransformNDCToImage(pointLight);

            if( rect.Inside( pointLight ) ) 
			{
                pointLight = pointLight - pointCenter;

				pcontext->SetBlendMode(BlendModeAdd);
				pcontext->SetShadeMode(ShadeModeFlat);
                pcontext->Translate(pointCenter);

				int count = ArrayCount(g_lensFlareData);

				for(int index = 0; index < count; index++) 
				{
					pcontext->PushState();

					pcontext->Translate(pointLight * g_lensFlareData[index].m_pos);
                    pcontext->Scale2(scale * g_lensFlareData[index].m_scale);

					float angle = 0;
					int indexSurface = g_lensFlareData[index].m_index;

					if (indexSurface == 1) 
					{
						pcontext->Rotate((pointLight.X() + pointLight.Y()) / 200.0f);
					}

                    pcontext->DrawImage3D(m_psurfaces[indexSurface], g_lensFlareData[index].m_color, true);
					pcontext->PopState();
				}
            }
		}
    }
};

TRef<LensFlareImage> CreateLensFlareImage(Modeler* pmodeler, Viewport* pviewport)
{
    return new LensFlareImageImpl(pmodeler, pviewport);
}
