#include "pch.h"


//////////////////////////////////////////////////////////////////////////////
//
// Function defined in xfile.cpp
//
//////////////////////////////////////////////////////////////////////////////

TRef<Geo> ImportXFile(Modeler* pmodeler, ZFile* pfile, Number* pnumberFrame, bool& bAnimation);

//////////////////////////////////////////////////////////////////////////////
//
// ModelerSite
//
//////////////////////////////////////////////////////////////////////////////

class ModelerSiteImpl : public ModelerSite {
public:
    //
    // ModelerSite members
    //

    void Error(const ZString& str)
    {
        ZError(str);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Factories
//
//////////////////////////////////////////////////////////////////////////////

class AnimatedImagePaneRectFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>     pimage =     Image::Cast((Value*)(IObject*)stack.Pop());
        TRef<RectValue> prect  = RectValue::Cast(        (IObject*)stack.Pop());

        return (Pane*)
            new AnimatedImagePane(
                pimage,
                prect->GetValue()
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class AnimatedImagePaneFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>     pimage =     Image::Cast((Value*)(IObject*)stack.Pop());

        return (Pane*)new AnimatedImagePane(pimage);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class PaneImageFactory : public IFunction {
private:
    TRef<Engine> m_pengine;
	TRef<Modeler> m_pmodeler;

public:
    PaneImageFactory(
        Modeler* pmodeler
    ) :
		m_pmodeler(pmodeler),
        m_pengine(pmodeler->GetEngine())
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Pane>    ppane;             CastTo(ppane, (IObject*)stack.Pop());
        TRef<Boolean> pbooleanZBuffer  = Boolean::Cast((IObject*)stack.Pop());
        TRef<Boolean> pbooleanColorKey = Boolean::Cast((IObject*)stack.Pop());

        SurfaceType stype = SurfaceType2D() | SurfaceType3D();

        if (pbooleanZBuffer->GetValue()) {
            stype.Set(SurfaceTypeZBuffer());
        }

		bool bColorKey = pbooleanColorKey->GetValue() || m_pmodeler->GetColorKeyHint();

        return (Value*)
            CreatePaneImage(
                m_pengine,
				bColorKey,
                ppane
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class FrameImageButtonPaneFactory : public IFunction {
private:
    TRef<Modeler> m_pmodeler;
    TRef<Number>  m_ptime;

public:
    FrameImageButtonPaneFactory(
        Modeler* pmodeler,
        Number*  ptime
    ) :
        m_pmodeler(pmodeler),
        m_ptime(ptime)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        StringValue* pstring = StringValue::Cast((IObject*)stack.Pop());
        Number*      pnumber = Number::Cast((IObject*)stack.Pop());
        bool         bLoop   = GetBoolean((IObject*)stack.Pop());
        bool         bFinish = GetBoolean((IObject*)stack.Pop());

        TRef<INameSpace>  pns = m_pmodeler->GetNameSpace(pstring->GetValue());
        TRef<ModifiableNumber> pnumberFrame; CastTo(pnumberFrame, pns->FindMember("frame"));
        TRef<Image>            pimage = pns->FindImage("image");

        return 
            CreateFrameImageButtonPane(
                m_ptime,
                pimage,
                pnumberFrame,
                pnumber->GetValue(),
                bLoop,
                bFinish
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class PointX : public Number {
private:
    PointValue* GetPoint() { return PointValue::Cast(GetChild(0)); }

public:
    PointX(PointValue* ppoint) :
        Number(ppoint)
    {
    }

    void Evaluate()
    {
        GetValueInternal() = GetPoint()->GetValue().X();
    }
};

class PointXFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<PointValue> ppoint = PointValue::Cast((IObject*)stack.Pop());

        return new PointX(ppoint);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class PointY : public Number {
private:
    PointValue* GetPoint() { return PointValue::Cast(GetChild(0)); }

public:
    PointY(PointValue* ppoint) :
        Number(ppoint)
    {
    }

    void Evaluate()
    {
        GetValueInternal() = GetPoint()->GetValue().Y();
    }
};

class PointYFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<PointValue> ppoint = PointValue::Cast((IObject*)stack.Pop());

        return new PointY(ppoint);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

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

class ImageSizeFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image> pimage = Image::Cast((Value*)(IObject*)stack.Pop());

        return new ImageSize(pimage);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class MDLFileImageFactory : public IFunction {
private:
    TRef<Modeler> m_pmodeler;

public:
    MDLFileImageFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<StringValue> pstring  = StringValue::Cast((IObject*)stack.Pop());

        return (Value*)
            CreateMDLFileImage(
                m_pmodeler,
                pstring->GetValue()
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class StringImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>      pjustify =      Number::Cast((IObject*)stack.Pop());
        TRef<Number>      pwidth   =      Number::Cast((IObject*)stack.Pop());
        TRef<FontValue>   pfont    =   FontValue::Cast((IObject*)stack.Pop());
        TRef<ColorValue>  pcolor   =  ColorValue::Cast((IObject*)stack.Pop());
        TRef<StringValue> pstring  = StringValue::Cast((IObject*)stack.Pop());

        Justification justification;
        justification.SetWord((DWORD)pjustify->GetValue());

        return (Value*)
            CreateStringImage(
                justification,
                pfont->GetValue(),
                pcolor,
                (int)pwidth->GetValue(),
                pstring
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class TextFileImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>      pjustify =      Number::Cast((IObject*)stack.Pop());
        TRef<Number>      pwidth   =      Number::Cast((IObject*)stack.Pop());
        TRef<FontValue>   pfont    =   FontValue::Cast((IObject*)stack.Pop());
        TRef<ColorValue>  pcolor   =  ColorValue::Cast((IObject*)stack.Pop());
        TRef<StringValue> pstring  = StringValue::Cast((IObject*)stack.Pop());

        Justification justification;
        justification.SetWord((DWORD)pjustify->GetValue());

        TRef<ZFile> pfile = new ZFile(pstring->GetValue());

        ZString str;

        if (pfile->IsValid()) {
            str = ZString((PCC)pfile->GetPointer(), pfile->GetLength());
        } else {
            str = "Unable to open file " + pstring->GetValue();
        }

        //
        // remove all of the 0xd characters from the string
        //

        int index = 0;
        while (index < str.GetLength()) {
            if (str[index] == 0xd) {
                str = str.Left(index) + str.Right(str.GetLength() - index - 1);
            } else {
                index++;
            }
        }

        //
        //
        //

        return (Value*)
            CreateStringImage(
                justification,
                pfont->GetValue(),
                pcolor,
                (int)pwidth->GetValue(),
                new StringValue(str)
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class JustifyImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>  pimage   =  Image::Cast((Value*)(IObject*)stack.Pop());
        TRef<Number> pjustify = Number::Cast(        (IObject*)stack.Pop());

        Justification justification;
        justification.SetWord((DWORD)pjustify->GetValue());

        return (Value*)
            CreateJustifyImage(
                pimage,
                justification
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class GaugeImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pjustify = Number::Cast((IObject*)stack.Pop());
        TRef<Image>  pimage   = Image::Cast((Value*)(IObject*)stack.Pop());
        TRef<Number> pnumber  = Number::Cast((IObject*)stack.Pop());

        Justification justification;
        justification.SetWord((DWORD)pjustify->GetValue());

        return 
            (Value*)CreateGaugeImage(
                justification,
                pimage, 
                false,
                pnumber
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class GaugeImageRectFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>    pjustify =    Number::Cast(        (IObject*)stack.Pop());
        TRef<Image>     pimage   = Image::Cast((Value*)(IObject*)stack.Pop());
        TRef<RectValue> prect    = RectValue::Cast(        (IObject*)stack.Pop());
        TRef<Boolean>   pbool    =   Boolean::Cast(        (IObject*)stack.Pop());
        TRef<Number>    pnumber  =    Number::Cast(        (IObject*)stack.Pop());

        Justification justification;
        justification.SetWord((DWORD)pjustify->GetValue());

        return 
            (Value*)CreateGaugeImage(
                justification,
                pimage,
                prect->GetValue(), 
                pbool->GetValue(),
                pnumber
            );
    }
};
                      
//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class UndetectableImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>     pimage =     Image::Cast((Value*)(IObject*)stack.Pop());

        return (Value*)CreateUndetectableImage(pimage);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ClipImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>     pimage =     Image::Cast((Value*)(IObject*)stack.Pop());
        TRef<RectValue> prect  = RectValue::Cast(        (IObject*)stack.Pop());

        return (Value*)CreateClipImage(pimage, prect);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class PickImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image> pimage = Image::Cast((Value*)(IObject*)stack.Pop());

        return (Value*)CreatePickImage(pimage);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ColorImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<ColorValue> pcolor = ColorValue::Cast((IObject*)stack.Pop());

        return (Value*)CreateColorImage(pcolor);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ExtentImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<RectValue>  prect  =  RectValue::Cast((IObject*)stack.Pop());
        TRef<ColorValue> pcolor = ColorValue::Cast((IObject*)stack.Pop());

        return (Value*)CreateExtentImage(prect, pcolor);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

/* !!! remove
class EmptyImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        return (Value*)Image::GetEmpty();
    }
};
*/

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ImportXFileFactory : public IFunction {
private:
    TRef<Modeler> m_pmodeler;

public:
    ImportXFileFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        ZString      str     = GetString((IObject*)stack.Pop());
        TRef<Number> pnumber = Number::Cast((IObject*)stack.Pop());

        bool bAnimation;
        TRef<IObject> pobject = m_pmodeler->LoadXFile(str, pnumber, bAnimation);
        ZAssert(pobject != NULL);
        return pobject;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ImportMDLFactory : public IFunction {
private:
    TRef<Modeler> m_pmodeler;

public:
    ImportMDLFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        ZString str = GetString((IObject*)stack.Pop());

        TRef<INameSpace> pns = m_pmodeler->GetNameSpace(str);

        if (pns) {
            return pns->FindMember("object");
        }

        return NULL;
    }
};


class ZPackFile : public ZFile
{
protected:
	DWORD m_dwFileSize;
	PathString m_strPath;
public:
	ZPackFile(const PathString& strPath, void * pData, DWORD dwFileSize )
	{
		m_p = (BYTE*) pData;
		m_dwFileSize = dwFileSize;
		m_strPath = strPath;
	}
	~ZPackFile()
	{
		m_p = NULL;
		m_dwFileSize = 0;
	}
	bool  IsValid()
	{
		return ( m_p != NULL );
	}
	int   GetLength()
	{
		return (int) m_dwFileSize;
	}
	BYTE * GetPointer(bool bWrite = false, bool bCopyOnWrite = false)
	{
		_ASSERT( !bWrite && !bCopyOnWrite );
		return m_p;
	}
    DWORD Read(void* p, DWORD length)
	{
		_ASSERT( length <= m_dwFileSize );
		memcpy( p, m_p, length );
		return length;
	}
};


class ImportImageFromFileFactory : public IFunction {
private:
    TRef<Modeler> m_pmodeler;
    TRef<PrivateEngine> m_pengine;

public:
    ImportImageFromFileFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
        CastTo(m_pengine, m_pmodeler->GetEngine());
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        ZString  str    = GetString((IObject*)stack.Pop());
        bool     b      = GetBoolean((IObject*)stack.Pop());

		// BT - 10/17 - HighRes Textures
        TRef<ZFile> zf = m_pmodeler->GetFile(str,"",true, m_pmodeler->GetUseHighResTextures());
		ZFile * pFile = (ZFile*) zf;
		
		D3DXIMAGE_INFO fileInfo;
		if( D3DXGetImageInfoFromFileInMemory(	pFile->GetPointer(),
												pFile->GetLength(),
												&fileInfo ) == D3D_OK )
		{
			_ASSERT( fileInfo.ResourceType == D3DRTYPE_TEXTURE );
			
			// We can resize non-UI textures.
			WinPoint targetSize( fileInfo.Width, fileInfo.Height );
			bool bColourKey = m_pmodeler->GetColorKeyHint();

			if( m_pmodeler->GetUIImageUsageHint() == false )
			{
				DWORD dwMaxTextureSize = CD3DDevice9::Get()->GetMaxTextureSize();
				_ASSERT( dwMaxTextureSize >= 256 );
				while(	( targetSize.x > (LONG)dwMaxTextureSize ) ||
						( targetSize.y > (LONG)dwMaxTextureSize ) )
				{
					targetSize.x = targetSize.x >> 1;
					targetSize.y = targetSize.y >> 1;
				}
			}
			// For D3D9, we only allow black colour keys.
			TRef<Surface> psurface =
				m_pengine->CreateSurfaceD3DX(
					&fileInfo,
					&targetSize,
					zf,
					bColourKey,
					Color( 0, 0, 0 ),
					str );
			return (Value*)new ConstantImage(psurface, ZString());
		}
		else
		{
			_ASSERT( false && "Failed to load image." );
		}

		// Replace FreeImage stuff with D3DX calls.
//        FreeImageIO fio;
//        fio.read_proc = myReadProc;
//        fio.seek_proc = mySeekProc;
//        fio.tell_proc = myTellProc;
//
//        FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromHandle(&fio,zf);
//        if (fif != FIF_UNKNOWN)
//        {
//            FIBITMAP * dib = FreeImage_LoadFromHandle(fif,&fio,zf,PNG_IGNOREGAMMA);
//            if (dib)
//            {
//                int bpp = FreeImage_GetBPP(dib);
//                assert((bpp == 16) || (bpp==24) || (bpp==32));
//                debugf("%s = %d bpp\n",(const char *)str,bpp);
//                UINT redm = FreeImage_GetRedMask(dib);
//                UINT grnm = FreeImage_GetGreenMask(dib);
//                UINT blum = FreeImage_GetBlueMask(dib);
//                UINT alpm = (bpp==32)?0xFF000000:0;
//                PixelFormat* ppf = m_pengine->GetPixelFormat(
//                    bpp,
//                    redm,
//                    grnm,
//                    blum,
//                    alpm
//                );  
//
//                // engine handles bitmaps mirrored ... yeeee
//                FreeImage_FlipHorizontal(dib);
//                FreeImage_FlipVertical(dib);
//                FreeImage_FlipHorizontal(dib);
//
//				// For D3D9, we only allow black colour keys.
//                TRef<Surface> psurface =
//                    m_pengine->CreateSurface(
//                    WinPoint(FreeImage_GetWidth(dib),FreeImage_GetHeight(dib)),
//                    ppf,
////                    NULL,				// Remove palette.
//                    FreeImage_GetPitch(dib),
//                    FreeImage_GetBits(dib),
//					zf,
//					true,
//					Color( 0, 0, 0 ),
//					str );
//                //FreeImage_Unload(dib); never free 
//                
//                if (b) {
//                    // could use FreeImage_HasBackgroundColor/FreeImage_GetBackgroundColor here
//                    // or extend MDL syntax to pass the transp color
//                    psurface->SetColorKey(Color(0, 0, 0));
//                }
//
//                return (Value*)new ConstantImage(psurface, ZString());
//            }
//        }
        debugf("ImportImageFromFileFactory: error reading file %s\n",(const char *)str);
        return NULL;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ImportImageFactory : public IFunction {
private:
    TRef<Modeler>		m_pmodeler;
    TRef<PrivateEngine> m_pengine;

public:
    ImportImageFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
        CastTo(m_pengine, m_pmodeler->GetEngine());
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        ZString str = GetString((IObject*)stack.Pop());
        bool      b = GetBoolean((IObject*)stack.Pop());

        TRef<Image> pimage = m_pmodeler->LoadImage(str, b);
        ZAssert(pimage != NULL);
        return (Value*)pimage;
    }

    TRef<IObject> Read(IBinaryReaderSite* psite, ObjectStack& stack)
    {
		BinarySurfaceInfo* pbsi; psite->GetStructure(pbsi);

        PixelFormat* ppf = m_pengine->GetPixelFormat(	pbsi->m_bitCount,
														pbsi->m_redMask,
														pbsi->m_greenMask,
														pbsi->m_blueMask,
														pbsi->m_alphaMask );

        BYTE* pdata = psite->GetPointer();
        psite->MovePointer(pbsi->m_pitch * pbsi->m_size.Y());

		bool bColorKey = m_pmodeler->GetColorKeyHint() || pbsi->m_bColorKey;

        TRef<Surface> psurface = m_pengine->CreateSurface(	pbsi->m_size,
															ppf,
															pbsi->m_pitch,
															pdata,
															psite->GetMemoryObject(),
															bColorKey,
															Color( 0, 0, 0 ),
															psite->GetCurrentFile(),
															m_pmodeler->GetSystemMemoryHint());

		return (Value*)new ConstantImage(psurface, ZString());
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ImportFontFactory : public IFunction {
private:
    TRef<Modeler>       m_pmodeler;
    TRef<PrivateEngine> m_pengine;

public:
    ImportFontFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
        CastTo(m_pengine, m_pmodeler->GetEngine());
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        ZUnimplemented();
        return NULL;
    }

    TRef<IObject> Read(IBinaryReaderSite* psite, ObjectStack& stack)
    {
        return 
            new FontValue(
                CreateEngineFont(psite)
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ImportImageLRFactory : public IFunction {
private:
    TRef<Modeler>     m_pmodeler;
    TRef<PrivateEngine> m_pengine;

public:
    ImportImageLRFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
        CastTo(m_pengine, m_pmodeler->GetEngine());
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        ZString str = GetString((IObject*)stack.Pop());
        bool      b = GetBoolean((IObject*)stack.Pop());

        TRef<Image>    pimage = m_pmodeler->LoadImage(str, b);
        const Bounds2& bounds = pimage->GetBounds();

        return
           (Value*)new TranslateImage(
                pimage,
                Point(
                    0,
                    -bounds.GetRect().YSize()
                )
           );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ImportImage3DFactory : public IFunction {
private:
    TRef<Modeler>     m_pmodeler;
    TRef<PrivateEngine> m_pengine;

public:
    ImportImage3DFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
        CastTo(m_pengine, m_pmodeler->GetEngine());
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        ZString          str    = GetString((IObject*)stack.Pop());
        TRef<ColorValue> pcolor = ColorValue::Cast(        (IObject*)stack.Pop());
        bool             b      = GetBoolean((IObject*)stack.Pop());

        TRef<Surface> psurface = m_pmodeler->LoadSurface(str, b);

        return (Value*)CreateConstantImage3D(psurface, pcolor); 
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////




class FillImageFactory : public IFunction{
private:
	TRef<Modeler>		m_pmodeler;
	TRef<PrivateEngine> m_pengine;

public:
	FillImageFactory(Modeler* pmodeler) :
		m_pmodeler(pmodeler)
	{
		CastTo(m_pengine, m_pmodeler->GetEngine());
	}

	class FillImage : public WrapImage
	{
	private:
		TRef<Engine> m_pengine;
	public:
		FillImage::FillImage(TRef<Engine> pengine, TRef<PointValue> psize, TRef<ColorValue> pcolor)
			: WrapImage(Image::GetEmpty(), psize, pcolor),
			m_pengine(pengine)
		{
			
		}

		PointValue* GetSize() { return PointValue::Cast(GetChild(1)); }
		ColorValue* GetColor() { return ColorValue::Cast(GetChild(2)); }

		void FillImage::Evaluate()
		{
			PointValue* psize = GetSize();
			ColorValue* pcolor = GetColor();
			WinPoint sizeWinPoint = WinPoint(psize->GetValue().X(), psize->GetValue().Y());

			TRef<Surface> surface = m_pengine->CreateSurface(sizeWinPoint, SurfaceType2D());
			surface->FillSurface(pcolor->GetValue());

			TRef<Image> pimage = new ConstantImage(surface, "FillImage");

			SetImage(pimage);
		}

	};

	TRef<IObject> Apply(ObjectStack& stack)
	{
		TRef<PointValue> psize = PointValue::Cast((IObject*)stack.Pop());
		TRef<ColorValue> pcolor = ColorValue::Cast((IObject*)stack.Pop());

		//TRef<PointValue> pixel = new PointValue(Point(1, 1));

		//TRef<FillImage> smallImage = new FillImage(m_pengine, pixel, pcolor);

		//return (Value*)
		//	new TransformImage(
		//		smallImage,
		//		new ScaleTransform2(pixel / psize)
		//	);

		return (Value*)new FillImage(m_pengine, psize, pcolor);

		WinPoint sizeWinPoint = WinPoint(psize->GetValue().X(), psize->GetValue().Y());

		TRef<Surface> surface = //m_pengine->CreateRenderTargetSurface(sizeWinPoint);
			m_pengine->CreateSurface(sizeWinPoint, SurfaceType2D());
		surface->FillSurface(pcolor->GetValue());

		TRef<Image> pimage = new ConstantImage(surface, "FillImage");
		return (Value*)pimage;
	}
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class BlendImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image> pimage       =  Image::Cast((Value*)(IObject*)stack.Pop());
        TRef<Number>     pnumberBlend = Number::Cast(        (IObject*)stack.Pop());

        return 
            (Value*)CreateBlendImage(
                pimage, 
                (BlendMode)(int)pnumberBlend->GetValue()
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class TranslateImageFactory : public IFunction {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>      pimage =      Image::Cast((Value*)(IObject*)stack.Pop());
        TRef<PointValue> ppoint = PointValue::Cast(        (IObject*)stack.Pop());

        return
           (Value*)new TransformImage(
                pimage,
                new TranslateTransform2(ppoint)
           );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class VisibleImageFactory : public IFunction {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>              pimage = Image::Cast((Value*)(IObject*)stack.Pop());
        TRef<Boolean>  pbool; CastTo(pbool, (Value*)(IObject*)stack.Pop());

        return (Value*)CreateVisibleImage(pimage, pbool);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ScaleImageFactory : public IFunction {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>      pimage =      Image::Cast((Value*)(IObject*)stack.Pop());
        TRef<PointValue> ppoint = PointValue::Cast(        (IObject*)stack.Pop());

        return 
            (Value*)new TransformImage(
                pimage, 
                new ScaleTransform2(ppoint)
            );
    }
};

class PointV : public PointValue {
public:
	PointV(Number* px, Number* py) :
		PointValue(px, py)
	{
	}

	Number* Get0() { return Number::Cast(GetChild(0)); }
	Number* Get1() { return Number::Cast(GetChild(1)); }

	void Evaluate()
	{
		GetValueInternal() =
			Point(
				Get0()->GetValue(),
				Get1()->GetValue()
			);
	}
};

class FitImageFactory : public IFunction {
public:
	TRef<IObject> Apply(ObjectStack& stack)
	{
		TRef<Image>      pimage = Image::Cast((Value*)(IObject*)stack.Pop());
		TRef<PointValue> ppoint = ModifiablePointValue::Cast((IObject*)stack.Pop());

		TRef<ImageSize> sizeImage = new ImageSize(pimage);

		TRef<Number> scale = Min(
			Divide(new PointX(ppoint), new PointX(sizeImage)),
			Divide(new PointY(ppoint), new PointY(sizeImage))
		);

		PointV * p_pointVariable = new PointV(scale, scale);

		return
			(Value*)new TransformImage(
				pimage,
				new ScaleTransform2(p_pointVariable)
			);
	}
};

class FitImageXFactory : public IFunction {
public:
	TRef<IObject> Apply(ObjectStack& stack)
	{
		TRef<Image>      pimage = Image::Cast((Value*)(IObject*)stack.Pop());
		TRef<Number> pnumber = Number::Cast((IObject*)stack.Pop());

		TRef<ImageSize> sizeImage = new ImageSize(pimage);

		TRef<Number> scale = Divide(pnumber, new PointX(sizeImage));

		PointV * p_pointVariable = new PointV(scale, scale);

		return
			(Value*)new TransformImage(
				pimage,
				new ScaleTransform2(p_pointVariable)
			);
	}
};

class FitImageYFactory : public IFunction {
public:
	TRef<IObject> Apply(ObjectStack& stack)
	{
		TRef<Image>      pimage = Image::Cast((Value*)(IObject*)stack.Pop());
		TRef<Number> pnumber = Number::Cast((IObject*)stack.Pop());
		//TRef<Number> pjustify = Number::Cast((IObject*)stack.Pop());

		//Justification justification;
		//justification.SetWord((DWORD)pjustify->GetValue());

		TRef<ImageSize> sizeImage = new ImageSize(pimage);

		TRef<Number> scale = Divide(pnumber, new PointY(sizeImage));

		PointV * p_pointVariable = new PointV(scale, scale);

		return
			(Value*)new TransformImage(
				pimage,
				new ScaleTransform2(p_pointVariable)
			);
	}
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class RotateImageFactory : public IFunction {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>  pimage =  Image::Cast((Value*)(IObject*)stack.Pop());
        TRef<Number> pangle = Number::Cast(        (IObject*)stack.Pop());

        return 
            (Value*)new TransformImage(
                pimage, 
                new RotateTransform2(pangle)
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class GeoImageFactory : public IFunction {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Geo>       pgeo            =       Geo::Cast((Value*)(IObject*)stack.Pop());
        TRef<RectValue> prect           = RectValue::Cast(        (IObject*)stack.Pop());
        TRef<Camera>    pcamera         =    Camera::Cast(        (IObject*)stack.Pop());
        TRef<Boolean>   pbooleanZBuffer =   Boolean::Cast(        (IObject*)stack.Pop());
        TRef<Viewport>  pviewport       = new Viewport(pcamera, prect);

        return
           (Value*)new GeoImage(
                pgeo,
                pviewport,
                pbooleanZBuffer->GetValue()
           );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class GeoImage2DFactory : public IFunction {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Geo> pgeo =Geo::Cast((Value*)(IObject*)stack.Pop());

        return (Value*)CreateGeoImage2D(pgeo);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ModifiableNumberFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        float value = GetNumber((IObject*)stack.Pop());

        return new ModifiableNumber(value);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class WrapNumberFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pnumber = Number::Cast((IObject*)stack.Pop());

        return new WrapNumber(pnumber);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

typedef TRef<IObject> TRefIObject; //hack for VC5

template<class StaticType, class ValueType>
class TIFunctionBinary : public IFunction {
public:
    TRefIObject Read(IBinaryReaderSite* psite, ObjectStack& stack)
    {
        StaticType* pvalue = (StaticType*)psite->GetPointer();
        psite->MovePointer(sizeof(StaticType));

        return new ValueType(*pvalue);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class RGBColorFactory : public TIFunctionBinary<Color, ColorValue > {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        float r = GetNumber((IObject*)stack.Pop());
        float g = GetNumber((IObject*)stack.Pop());
        float b = GetNumber((IObject*)stack.Pop());

        return new ColorValue(Color(r, g, b));
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class RGBAColorFactory : public TIFunctionBinary<Color, ColorValue > {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        float r = GetNumber((IObject*)stack.Pop());
        float g = GetNumber((IObject*)stack.Pop());
        float b = GetNumber((IObject*)stack.Pop());
        float a = GetNumber((IObject*)stack.Pop());

        return new ColorValue(Color(r, g, b, a));
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class VectorFactory : public TIFunctionBinary<Vector, VectorValue > {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        float x = GetNumber((IObject*)stack.Pop());
        float y = GetNumber((IObject*)stack.Pop());
        float z = GetNumber((IObject*)stack.Pop());

        return new VectorValue(Vector(x, y, z));
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class PointFactory : public TIFunctionBinary<Point, PointValue > {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        float x = GetNumber((IObject*)stack.Pop());
        float y = GetNumber((IObject*)stack.Pop());

        return new PointValue(Point(x, y));
    }
};

class PointVFactory : public TIFunctionBinary<Point, PointValue > {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        Number* px; CastTo(px, (IObject*)stack.Pop());
        Number* py; CastTo(py, (IObject*)stack.Pop());

        return new PointV(px, py);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class RectFactory : public TIFunctionBinary<Rect, RectValue > {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        float xmin = GetNumber((IObject*)stack.Pop());
        float ymin = GetNumber((IObject*)stack.Pop());
        float xmax = GetNumber((IObject*)stack.Pop());
        float ymax = GetNumber((IObject*)stack.Pop());

        return new RectValue(Rect(xmin, ymin, xmax, ymax));
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class MatrixTransformFactory : public TIFunctionBinary<Matrix, Transform> {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        float m00 = GetNumber((IObject*)stack.Pop());
        float m01 = GetNumber((IObject*)stack.Pop());
        float m02 = GetNumber((IObject*)stack.Pop());
        float m03 = GetNumber((IObject*)stack.Pop());

        float m10 = GetNumber((IObject*)stack.Pop());
        float m11 = GetNumber((IObject*)stack.Pop());
        float m12 = GetNumber((IObject*)stack.Pop());
        float m13 = GetNumber((IObject*)stack.Pop());

        float m20 = GetNumber((IObject*)stack.Pop());
        float m21 = GetNumber((IObject*)stack.Pop());
        float m22 = GetNumber((IObject*)stack.Pop());
        float m23 = GetNumber((IObject*)stack.Pop());

        float m30 = GetNumber((IObject*)stack.Pop());
        float m31 = GetNumber((IObject*)stack.Pop());
        float m32 = GetNumber((IObject*)stack.Pop());
        float m33 = GetNumber((IObject*)stack.Pop());

        return
            new Transform(
                Matrix(
                    m00, m01, m02, m03,
                    m10, m11, m12, m13,
                    m20, m21, m22, m23,
                    m30, m31, m32, m33
                )
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class MaterialFactory : public IFunction {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<ColorValue> pcolorDiffuse  = ColorValue::Cast((IObject*)stack.Pop());
        TRef<ColorValue> pcolorSpecular = ColorValue::Cast((IObject*)stack.Pop());
        TRef<ColorValue> pcolorEmissive = ColorValue::Cast((IObject*)stack.Pop());
        TRef<Number>     ppower         =     Number::Cast((IObject*)stack.Pop());

        return
            CreateMaterial(
                pcolorDiffuse,
                pcolorSpecular,
                pcolorEmissive,
                ppower
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class TransformGeoFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Geo>       pgeo   =       Geo::Cast((IObject*)stack.Pop());
        TRef<Transform> ptrans = Transform::Cast((IObject*)stack.Pop());

        return new TransformGeo(pgeo, ptrans);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class TextureGeoFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Geo>   pgeo   =   Geo::Cast((IObject*)stack.Pop());
        TRef<Image> pimage = Image::Cast((Value*)(IObject*)stack.Pop());

        return new TextureGeo(pgeo, pimage);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class BlendGeoFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Geo>    pgeo         =    Geo::Cast((IObject*)stack.Pop());
        TRef<Number> pnumberBlend = Number::Cast((IObject*)stack.Pop());

        return 
            CreateBlendGeo(
                pgeo, 
                (BlendMode)(int)pnumberBlend->GetValue()
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class MaterialGeoFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Geo>      pgeo      =      Geo::Cast((IObject*)stack.Pop());
        TRef<Material> pmaterial = Material::Cast((IObject*)stack.Pop());

        return new MaterialGeo(pgeo, pmaterial);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Switch Value
//
//////////////////////////////////////////////////////////////////////////////

template<class StaticType>
class TSwitchValue : public TStaticValue<StaticType> {
protected:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Types
    //
    //////////////////////////////////////////////////////////////////////////////

    typedef TStaticValue<StaticType> ValueType;

    class Data {
    public:
        float           m_number;
        TRef<ValueType> m_pvalue;
    };

    typedef TList<Data, DefaultNoEquals> DataList;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Members
    //
    //////////////////////////////////////////////////////////////////////////////

    DataList        m_list;
    TRef<ValueType> m_pdefault;
    float           m_number;

    TStaticValue<StaticType>* GetWrappedValue() { return TStaticValue<StaticType>::Cast(GetChild(0)); }

public:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    TSwitchValue(ValueType* pdefault, Number* pnumber) :
        TStaticValue<StaticType>(pdefault, pnumber),
        m_pdefault(pdefault),
        m_number(pnumber->GetValue())
    {
    }

    Number* GetNumber() { return Number::Cast(GetChild(1)); }

    //////////////////////////////////////////////////////////////////////////////
    //
    // SwitchImage Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void AddValue(float number, ValueType* pvalue)
    {
        m_list.PushFront();
        m_list.GetFront().m_number = number;
        m_list.GetFront().m_pvalue = pvalue;

        if (m_number == number) {
            pvalue->Update();
            SetChild(0, pvalue);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Value Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void Evaluate()
    {
        float number = GetNumber()->GetValue();

        if (m_number != number) {
            m_number = number;
            DataList::Iterator iter(m_list);

            while (true) {
                if (iter.End()) {
                    m_pdefault->Update();
                    SetChild(0, m_pdefault);
                    break;
                }
                const Data& data = iter.Value();

                if (data.m_number == number) {
                    data.m_pvalue->Update();
                    SetChild(0, data.m_pvalue);
                    break;
                }

                iter.Next();
            }
        }

        GetValueInternal() = GetWrappedValue()->GetValue();
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Switch Value Factory
//
//////////////////////////////////////////////////////////////////////////////

template<class StaticType>
class TSwitchFactory : public IFunction {
private:
    typedef TStaticValue<StaticType> ValueType;
    typedef TSwitchValue<StaticType> SwitchType;

public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>      pnumber; CastTo(pnumber,         (IObject*)stack.Pop());
        TRef<ValueType>   pvalue;  CastTo(pvalue,  (Value*)(IObject*)stack.Pop());
        TRef<IObjectList> plist;   CastTo(plist,           (IObject*)stack.Pop());

        TRef<SwitchType> pswitch = new SwitchType(pvalue, pnumber);

        IObjectPair* ppair;  CastTo(ppair, plist->GetFirst());
        while (ppair) {
            TRef<Number>    pnumber; CastTo(pnumber, ppair->GetFirst() );
            TRef<ValueType> pimage;  CastTo(pvalue,  ppair->GetSecond());

            pswitch->AddValue(
                pnumber->GetValue(),
                pvalue
            );

            CastTo(ppair, plist->GetNext());
        }

        return (Value*)pswitch;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class SwitchImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>      pnumber; CastTo(pnumber,         (IObject*)stack.Pop());
        TRef<Image>       pimage;  CastTo(pimage,  (Value*)(IObject*)stack.Pop());
        TRef<IObjectList> plist;   CastTo(plist,           (IObject*)stack.Pop());

        TRef<SwitchImage> pswitch = CreateSwitchImage(pnumber, pimage);

        IObjectPair* ppair;  CastTo(ppair, plist->GetFirst());
        while (ppair) {
            float       number =   GetNumber(        ppair->GetFirst());
            TRef<Image> pimage = Image::Cast((Value*)ppair->GetSecond());

            pswitch->AddImage(number, pimage);

            CastTo(ppair, plist->GetNext());
        }

        return (Value*)pswitch;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class GroupImageFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<GroupImage> pgroup = new GroupImage();

        TRef<IObjectList> plist;  CastTo(plist, (IObject*)stack.Pop());

        plist->GetFirst();

        while (plist->GetCurrent() != NULL) {
            pgroup->AddImage(Image::Cast((Value*)plist->GetCurrent()));
            plist->GetNext();
        }

        return (Value*)pgroup;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class GroupGeoFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<GroupGeo> pgroup = GroupGeo::Create();

        TRef<IObjectList> plist;  CastTo(plist, (IObject*)stack.Pop());

        plist->GetFirst();

        while (plist->GetCurrent() != NULL) {
            pgroup->AddGeo(Geo::Cast(plist->GetCurrent()));
            plist->GetNext();
        }

        return pgroup;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class MeshGeoFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<IObjectList> plistVertices; CastTo(plistVertices, (IObject*)stack.Pop());
        TRef<IObjectList> plistIndices;  CastTo(plistIndices,  (IObject*)stack.Pop() );

        TVector<Vertex> vertices;
        TVector<WORD> indices;

        plistVertices->GetFirst();

        while (plistVertices->GetCurrent() != NULL) {
            float x = GetNumber(plistVertices->GetCurrent());
            float y = GetNumber(plistVertices->GetNext());
            float z = GetNumber(plistVertices->GetNext());

            float nx = GetNumber(plistVertices->GetNext());
            float ny = GetNumber(plistVertices->GetNext());
            float nz = GetNumber(plistVertices->GetNext());

            float tx = GetNumber(plistVertices->GetNext());
            float ty = GetNumber(plistVertices->GetNext());

            vertices.PushEnd(Vertex(x, y, z, nx, ny, nz, tx,ty));

            plistVertices->GetNext();
        }

        plistIndices->GetFirst();

        while (plistIndices->GetCurrent() != NULL) {
            indices.PushEnd((WORD)GetNumber(plistIndices->GetCurrent()));
            plistIndices->GetNext();
        }

        return Geo::CreateMesh(vertices, indices);
    }


TRef<IObject> Read(IBinaryReaderSite* psite, ObjectStack& stack)
{
    DWORD countVertices = psite->GetDWORD();
    DWORD countIndices  = psite->GetDWORD();
    Vertex* pvertex     = (Vertex*)psite->GetPointer();

    psite->MovePointer(countVertices * sizeof(Vertex));

    WORD* pindex        = (WORD*)psite->GetPointer();

    psite->MovePointer(countIndices * sizeof(WORD));

    return
        Geo::CreateMesh(
            pvertex,
            countVertices,
            pindex,
            countIndices,
            psite->GetMemoryObject()
        );
}
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class LODGeoFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Geo>         pgeo;     CastTo(pgeo,     (IObject*)stack.Pop());
        TRef<IObjectList> plistLOD; CastTo(plistLOD, (IObject*)stack.Pop());

        TRef<LODGeo> plodGeo  = LODGeo::Create(pgeo);

        IObjectPair* ppair;    CastTo(ppair, plistLOD->GetFirst());
        while (ppair) {
            float       pixels  = GetNumber(ppair->GetFirst());
            TRef<Geo>   pgeoLOD = Geo::Cast(ppair->GetSecond());

            plodGeo->AddGeo(pgeoLOD, pixels);

            CastTo(ppair, plistLOD->GetNext());
        }

        return plodGeo;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class KeyFramedTranslateFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<IObjectList> plist;  CastTo(plist,  (IObject*)stack.Pop());
        TRef<Number>      pframe; CastTo(pframe, (IObject*)stack.Pop());

        TRef<KeyFramedTranslateTransform> ptrans = CreateKeyFramedTranslateTransform(pframe);

        plist->GetFirst();
        while (plist->GetCurrent() != NULL) {
            float time = GetNumber(plist->GetCurrent());
            float x    = GetNumber(plist->GetNext());
            float y    = GetNumber(plist->GetNext());
            float z    = GetNumber(plist->GetNext());

            ptrans->AddKey(time, Vector(x, y, z));

            plist->GetNext();
        }

        return ptrans;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ScaleFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pscale = Number::Cast((IObject*)stack.Pop());

        return new ScaleTransform(pscale->GetValue());
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class TranslateFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<VectorValue> pvector = VectorValue::Cast((IObject*)stack.Pop());

        return new TranslateTransform(pvector->GetValue());
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class RotateFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<VectorValue> pvector = VectorValue::Cast((IObject*)stack.Pop());
        TRef<Number>      pangle  =      Number::Cast((IObject*)stack.Pop());

        return 
            new RotateTransform(
                pvector->GetValue(),
                pangle->GetValue()
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class KeyFramedScaleFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<IObjectList> plist;  CastTo(plist,  (IObject*)stack.Pop());
        TRef<Number>      pframe; CastTo(pframe, (IObject*)stack.Pop());

        TRef<KeyFramedScaleTransform> ptrans = CreateKeyFramedScaleTransform(pframe);

        plist->GetFirst();
        while (plist->GetCurrent() != NULL) {
            float time = GetNumber(plist->GetCurrent());
            float x    = GetNumber(plist->GetNext());
            float y    = GetNumber(plist->GetNext());
            float z    = GetNumber(plist->GetNext());

            ptrans->AddKey(time, Vector(x, y, z));

            plist->GetNext();
        }

        return ptrans;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class KeyFramedRotateFactory : public IFunction {
private:
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<IObjectList> plist;  CastTo(plist,  (IObject*)stack.Pop());
        TRef<Number>      pframe; CastTo(pframe, (IObject*)stack.Pop());

        TRef<KeyFramedRotateTransform> ptrans = CreateKeyFramedRotateTransform(pframe);

        plist->GetFirst();
        while (plist->GetCurrent() != NULL) {
            float time = GetNumber(plist->GetCurrent());
            float s    = GetNumber(plist->GetNext());
            float x    = GetNumber(plist->GetNext());
            float y    = GetNumber(plist->GetNext());
            float z    = GetNumber(plist->GetNext());

            ptrans->AddKey(time, Quaternion(s, x, y, z));

            plist->GetNext();
        }

        return ptrans;
    }
};

// Camera
// KGJV addition
class CameraFactory : public IFunction {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
		TRef<Number> znear    =  Number::Cast((IObject*)stack.Pop());
		TRef<Number> zfar     = Number::Cast((Value*)(IObject*)stack.Pop());
		TRef<Number> fov      = Number::Cast((Value*)(IObject*)stack.Pop());
		TRef<VectorValue> pvector = VectorValue::Cast((IObject*)stack.Pop());

		TRef<Camera> pcam = new Camera();
		pcam->SetZClip(znear->GetValue(),zfar->GetValue());
		pcam->SetFOV(RadiansFromDegrees(fov->GetValue()));
		pcam->SetPosition(pvector->GetValue());
        return pcam;
    }
};


//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class Win32FontFactory : public IFunction {
private:
    Engine* m_pengine;

public:
    Win32FontFactory(Engine* pengine) :
        m_pengine(pengine)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<StringValue> pstringName;		CastTo(pstringName,		(IObject*)stack.Pop());
        TRef<Number>      pnumberSize;		CastTo(pnumberSize,		(IObject*)stack.Pop());
        TRef<Boolean>     pboolBold;		CastTo(pboolBold,		(IObject*)stack.Pop());
		TRef<Number>	  pnumberStretch;   CastTo(pnumberStretch,	(IObject*)stack.Pop());

        return
            new FontValue(
                CreateEngineFont(
                    CreateFont(
                        (int)pnumberSize->GetValue(),
                        (int)pnumberStretch->GetValue(),0, 0,
                        pboolBold->GetValue() ? FW_BOLD : FW_DONTCARE, 
                        FALSE, FALSE, FALSE, ANSI_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN,
                        pstringName->GetValue()
                    )
                )
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Numbers
//
//////////////////////////////////////////////////////////////////////////////

class ModFactory : public IFunction {
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pnumber1; CastTo(pnumber1, (IObject*)stack.Pop());
        TRef<Number> pnumber2; CastTo(pnumber2, (IObject*)stack.Pop());

        return Mod(pnumber1, pnumber2);
    }
};

class MinFactory : public IFunction {
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pnumber1; CastTo(pnumber1, (IObject*)stack.Pop());
        TRef<Number> pnumber2; CastTo(pnumber2, (IObject*)stack.Pop());

        return Min(pnumber1, pnumber2);
    }
};

class MaxFactory : public IFunction {
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pnumber1; CastTo(pnumber1, (IObject*)stack.Pop());
        TRef<Number> pnumber2; CastTo(pnumber2, (IObject*)stack.Pop());

        return Max(pnumber1, pnumber2);
    }
};

class AddFactory : public IFunction {
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pnumber1; CastTo(pnumber1, (IObject*)stack.Pop());
        TRef<Number> pnumber2; CastTo(pnumber2, (IObject*)stack.Pop());

        return Add(pnumber1, pnumber2);
    }
};

class SubtractFactory : public IFunction {
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pnumber1; CastTo(pnumber1, (IObject*)stack.Pop());
        TRef<Number> pnumber2; CastTo(pnumber2, (IObject*)stack.Pop());

        return Subtract(pnumber1, pnumber2);
    }
};

class MultiplyFactory : public IFunction {
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pnumber1; CastTo(pnumber1, (IObject*)stack.Pop());
        TRef<Number> pnumber2; CastTo(pnumber2, (IObject*)stack.Pop());

        return Multiply(pnumber1, pnumber2);
    }
};

class DivideFactory : public IFunction {
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pnumber1; CastTo(pnumber1, (IObject*)stack.Pop());
        TRef<Number> pnumber2; CastTo(pnumber2, (IObject*)stack.Pop());

        return Divide(pnumber1, pnumber2);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// NumberString
//
//////////////////////////////////////////////////////////////////////////////

class NumberString : public StringValue {
public:
    NumberString(Number* pvalue) :
        StringValue(pvalue)
    {
    }

    Number* GetNumber() { return Number::Cast(GetChild(0)); }

    void Evaluate()
    {
        GetValueInternal() = (int)(GetNumber()->GetValue() + 0.5);
    }

    ZString GetFunctionName() { return "NumberString"; }
};

class NumberStringFactory : public IFunction {
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pnumber; CastTo(pnumber, (IObject*)stack.Pop());

        return new NumberString(pnumber);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// RealNumberString
//
//////////////////////////////////////////////////////////////////////////////

class RealNumberString : public StringValue {
public:
    RealNumberString(Number* pvalue) :
        StringValue(pvalue)
    {
    }

    Number* GetNumber() { return Number::Cast(GetChild(0)); }

    void Evaluate()
    {
        char cbTemp[80];
        sprintf_s(cbTemp, 80, "%.2g", (double)GetNumber()->GetValue());

        GetValueInternal() = cbTemp;
    }

    ZString GetFunctionName() { return "RealNumberString"; }
};

class RealNumberStringFactory : public IFunction {
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number> pnumber; CastTo(pnumber, (IObject*)stack.Pop());

        return new RealNumberString(pnumber);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// ConcatinatedString
//
//////////////////////////////////////////////////////////////////////////////

class ConcatinatedString : public StringValue {
public:
    ConcatinatedString(StringValue* pvalue1, StringValue* pvalue2) :
        StringValue(pvalue1, pvalue2)
    {
    }

    void Evaluate()
    {
        GetValueInternal() = ((StringValue*)GetChild(0))->GetValue()
            + ((StringValue*)GetChild(1))->GetValue();
    }

    ZString GetFunctionName() { return "ConcatinatedString"; }
};

class ConcatinatedStringFactory : public IFunction {
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<StringValue> pstring1; CastTo(pstring1, (IObject*)stack.Pop());
        TRef<StringValue> pstring2; CastTo(pstring2, (IObject*)stack.Pop());

        return new ConcatinatedString(pstring1, pstring2);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// GetResolution
//
//////////////////////////////////////////////////////////////////////////////

class PointFromWinPoint : public PointValue {
private:
	WinPointValue* GetWinPoint() { return WinPointValue::Cast(GetChild(0)); }

public:
	PointFromWinPoint(WinPointValue* ppoint)
		: PointValue(ppoint)
	{
	}

	void Evaluate()
	{
		WinPoint winpoint = GetWinPoint()->GetValue();
		GetValueInternal() = Point(winpoint.X(), winpoint.Y());
	}
};

class GetResolutionFactory : public IFunction {
private:
	Engine* m_pengine;

public:
	GetResolutionFactory(Engine* pengine) :
		m_pengine(pengine)
	{
	}

	TRef<IObject> Apply(ObjectStack& stack)
	{
		return new PointFromWinPoint(m_pengine->GetResolutionSizeModifiable());
	}
};

//////////////////////////////////////////////////////////////////////////////
//
// Modeler
//
//////////////////////////////////////////////////////////////////////////////

class ModelerImpl : public Modeler {
private:
    TRef<Engine>			m_pengine;
    TRef<ModelerSite>		m_psite;
    PathString				m_pathStr;
	ImportImageFactory *	m_pImageFactory;			// This allows us to pass extra parameters into the image factory.

	// BT - STEAM
	FileHashTable			m_fileHashTable;

    TMap<ZString, TRef<INameSpace> > m_mapNameSpace;

	// Hints. Modeler flags referenced during resource loading.
	bool					m_bHintColorKey;			// Surface requires colour keying.
	bool					m_bHintSystemMemory;
	bool					m_bHintUIImage;
	bool					m_bUseHighResTextures;

public:
    ModelerImpl(Engine* pengine) :
        m_pengine(pengine),
        m_pathStr("."),
		m_bUseHighResTextures(true) // BT - 10/17 - HighRes Textures
		{
        m_psite = new ModelerSiteImpl();
        InitializeNameSpace();
		m_bHintColorKey = false;
		m_bHintSystemMemory = false;
		m_bHintUIImage = true;				// Default to true at startup.
		}

    void SetSite(ModelerSite* psite)
    {
        m_psite = psite;
	}

    void SetArtPath(const PathString& pathStr)
    {
        m_pathStr = pathStr;
    }

    ZString GetArtPath()
    {
        return m_pathStr;
    }

    void Terminate()
    {
        m_mapNameSpace.SetEmpty();
        m_pengine = NULL;
        m_psite   = NULL;
    }

    void InitializeNameSpace()
	{
        INameSpace* pns = CreateNameSpace("model");

        //
        // Types
        //

        pns->AddType("Number"     , new TBaseMDLType<float      >("float"      , ZString()));
        pns->AddType("Boolean"    , new TBaseMDLType<bool       >("bool"       , "b"      ));
        pns->AddType("Color"      , new TBaseMDLType<Color      >("Color"      , "color"  ));
        pns->AddType("Point"      , new TBaseMDLType<Point      >("Point"      , "point"  ));
        pns->AddType("Vector"     , new TBaseMDLType<Vector     >("Vector"     , "vec"    ));
        pns->AddType("Rect"       , new TBaseMDLType<Rect       >("Rect"       , "rect"   ));
        pns->AddType("Orientation", new TBaseMDLType<Orientation>("Orientation", "orient" ));
        pns->AddType("String"     , CreateStringMDLType()                                  );
        pns->AddType("Image"      , CreateIObjectMDLType("Image", "image")                 );
        pns->AddType("Geo"        , CreateIObjectMDLType("Geo"  , "Geo"  )                 );

        //
        // built in values
        //

        TRef<Number> ptime = new ModifiableNumber(0); //Fix memory leak -Imago 8/2/09

        pns->AddMember("emptyGeo",           Geo::GetEmpty()                 );
        pns->AddMember("emptyImage",         (Value*)Image::GetEmpty()       );
        pns->AddMember("transparentImage",   (Value*)CreateTransparentImage());
        pns->AddMember("emptyString",        new StringValue(ZString())      );
        pns->AddMember("identityTransform",  GetIdentityTransform()          );
        pns->AddMember("time",               ptime                           );
        pns->AddMember("white",              new ColorValue(Color::White() ) );
        pns->AddMember("black",              new ColorValue(Color::Black() ) );
        pns->AddMember("red",                new ColorValue(Color::Red()   ) );
        pns->AddMember("green",              new ColorValue(Color::Green() ) );
        pns->AddMember("blue",               new ColorValue(Color::Blue()  ) );
        pns->AddMember("yellow",             new ColorValue(Color::Yellow()) );
		pns->AddMember("cyan",               new ColorValue(Color::Cyan()  ) );
        pns->AddMember("pi",                 new Number(pi)                  );

        pns->AddMember("defaultFont",        
            new FontValue(
                CreateEngineFont(
                    CreateFont(
                        11,
                        0, 0, 0,
                        FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN,
                        "tahoma"
                    )
                )
            )
        );

        //
        // Switches
        //

        pns->AddMember("SwitchString",       new TSwitchFactory<ZString>());

        //
        // Data type constructors
        //

        pns->AddMember("ModifiableNumber",   new ModifiableNumberFactory());
        pns->AddMember("WrapNumber",         new WrapNumberFactory());
        pns->AddMember("Color",              new RGBColorFactory());
        pns->AddMember("ColorA",             new RGBAColorFactory());
        pns->AddMember("Vector",             new VectorFactory());
        pns->AddMember("Point",              new PointFactory());
        pns->AddMember("Rect",               new RectFactory());
        pns->AddMember("Material",           new MaterialFactory());
        pns->AddMember("PointV",             new PointVFactory());

        //
        // Numbers
        //

        pns->AddMember("Min",                new MinFactory());
        pns->AddMember("Max",                new MaxFactory());
        pns->AddMember("Mod",                new ModFactory());
        pns->AddMember("Add",                new AddFactory());
        pns->AddMember("Subtract",           new SubtractFactory());
        pns->AddMember("Multiply",           new MultiplyFactory());
        pns->AddMember("Divide",             new DivideFactory());

        //
        // Strings
        //

        pns->AddMember("NumberString",       new NumberStringFactory());
        pns->AddMember("RealNumberString",   new RealNumberStringFactory());
        pns->AddMember("ConcatinatedString", new ConcatinatedStringFactory());

        //
        // Images
        //

		m_pImageFactory = new ImportImageFactory( this );

        pns->AddMember("ImportImage",        m_pImageFactory );
        pns->AddMember("ImportImageFromFile",new ImportImageFromFileFactory(this)); // KGJV 32B
        pns->AddMember("ImportImage3D",      new ImportImage3DFactory(this));
        pns->AddMember("ImportImageLR",      new ImportImageLRFactory(this));
		pns->AddMember("FillImage",			 new FillImageFactory(this));

        pns->AddMember("FrameImage",         CreateFrameImageFactory());

        pns->AddMember("GaugeImage",         new GaugeImageFactory());
        pns->AddMember("GaugeImageRect",     new GaugeImageRectFactory());
        pns->AddMember("GroupImage",         new GroupImageFactory());
        pns->AddMember("SwitchImage",        new SwitchImageFactory());
        pns->AddMember("GeoImage",           new GeoImageFactory());
        pns->AddMember("GeoImage2D",         new GeoImage2DFactory());
        pns->AddMember("ClipImage",          new ClipImageFactory());
        pns->AddMember("PickImage",          new PickImageFactory());
        pns->AddMember("UndetectableImage",  new UndetectableImageFactory());
        pns->AddMember("ColorImage",         new ColorImageFactory());
        pns->AddMember("ExtentImage",        new ExtentImageFactory());
        // !!! pns->AddMember("EmptyImage",         new EmptyImageFactory());

        pns->AddMember("StringImage",        new StringImageFactory());
        pns->AddMember("MDLFileImage",       new MDLFileImageFactory(this));
        pns->AddMember("TextFileImage",      new TextFileImageFactory());

        pns->AddMember("JustifyLeft",        new Number((float)JustifyLeft().GetWord()   ));
        pns->AddMember("JustifyRight",       new Number((float)JustifyRight().GetWord()  ));
        pns->AddMember("JustifyTop",         new Number((float)JustifyTop().GetWord()    ));
        pns->AddMember("JustifyBottom",      new Number((float)JustifyBottom().GetWord() ));
        pns->AddMember("JustifyXCenter",     new Number((float)JustifyXCenter().GetWord()));
        pns->AddMember("JustifyYCenter",     new Number((float)JustifyYCenter().GetWord()));
        pns->AddMember("JustifyCenter",      new Number((float)JustifyCenter().GetWord() ));

        pns->AddMember("JustifyImage",       new JustifyImageFactory());
        pns->AddMember("VisibleImage",       new VisibleImageFactory());
        pns->AddMember("TranslateImage",     new TranslateImageFactory());
        pns->AddMember("ScaleImage",         new ScaleImageFactory());
        pns->AddMember("RotateImage",        new RotateImageFactory());
        pns->AddMember("BlendImage",         new BlendImageFactory());
		pns->AddMember("FitImage",			 new FitImageFactory());
		pns->AddMember("FitImageX",			 new FitImageXFactory());
		pns->AddMember("FitImageY",			 new FitImageYFactory());


        //
        // Image Attributes
        //

        pns->AddMember("ImageSize",          new ImageSizeFactory());

        //
        // Point Attributes
        //

        pns->AddMember("PointY",             new PointYFactory());
        pns->AddMember("PointX",             new PointXFactory());

        //
        // Geos
        //

        pns->AddMember("ImportXFile",          new ImportXFileFactory(this));
        pns->AddMember("ImportMDL",            new ImportMDLFactory(this));

        pns->AddMember("MeshGeo",              new MeshGeoFactory());
        pns->AddMember("LODGeo",               new LODGeoFactory());
        pns->AddMember("GroupGeo",             new GroupGeoFactory());
        pns->AddMember("TransformGeo",         new TransformGeoFactory());
        pns->AddMember("MaterialGeo",          new MaterialGeoFactory());
        pns->AddMember("TextureGeo",           new TextureGeoFactory());
        pns->AddMember("Matrix",               new MatrixTransformFactory());

        pns->AddMember("BlendGeo",             new BlendGeoFactory());
        
        pns->AddMember("BlendModeSource",      new Number(BlendModeSource     ));
        pns->AddMember("BlendModeAdd",         new Number(BlendModeAdd        ));

        //
        // Transforms
        //

        pns->AddMember("KeyFramedTranslate",   new KeyFramedTranslateFactory());
        pns->AddMember("KeyFramedScale",       new KeyFramedScaleFactory());
        pns->AddMember("KeyFramedRotate",      new KeyFramedRotateFactory());
        pns->AddMember("Scale",                new ScaleFactory());
        pns->AddMember("Translate",            new TranslateFactory());
        pns->AddMember("Rotate",               new RotateFactory());

		// Camera (KGJV)
		//  Camera(zclip near,zclip far,FOV in degrees,position_vector)
		pns->AddMember("Camera",                new CameraFactory());

        //
        // Fonts
        //

        pns->AddMember("Win32Font",          new Win32FontFactory(m_pengine));
        pns->AddMember("ImportFont",         new ImportFontFactory(this));

        //
        // Panes
        //

        pns->AddMember("AnimatedImagePane",     new AnimatedImagePaneFactory());
        pns->AddMember("AnimatedImagePaneRect", new AnimatedImagePaneRectFactory());
        pns->AddMember("FrameImageButtonPane",  new FrameImageButtonPaneFactory(this, ptime));
        pns->AddMember("PaneImage",             new PaneImageFactory(this));

		//
		// Resolution
		//
		pns->AddMember("GetResolution", new GetResolutionFactory(m_pengine));
	}

    Engine* GetEngine()
    {
        return m_pengine;
	}

	// BT - 10/17 - HighRes Textures
	void SetHighResTextures(bool bUseHighResTextures)
	{
		m_bUseHighResTextures = bUseHighResTextures;
	}

	// BT - 10/17 - HighRes Textures
	bool GetUseHighResTextures()
	{
		return m_bUseHighResTextures;
	}

	TRef<ZFile> GetFile(const PathString& pathStr, const ZString& strExtensionArg, bool bError, bool getHighresVersion)
	{
		ZString strExtension = pathStr.GetExtension();
		ZString strToTryOpen;// yp Your_Persona October 7 2006 : TextureFolder Patch
		ZString strToTryOpenFromDev;// KGJV - 'dev' subfolder
		ZString strPackFile; // doofus - Filename for pack searching.
		ZString strToTryOpenFromSteamDLC; // BT - STEAM

		ZString strToOpen;
		TRef<ZFile> pfile = NULL;

		if (!strExtension.IsEmpty()) {
			if (!strExtensionArg.IsEmpty()) // KGJV 32B - ignore empty strExtensionArg
				if (strExtension.ToLower() != strExtensionArg.ToLower()) { // KGJV 32B - ignore case
					return NULL;
				}
			strPackFile = pathStr;
			strToOpen = m_pathStr + pathStr;
			strToTryOpenFromDev = m_pathStr + "dev/" + pathStr;
			strToTryOpen = m_pathStr + "Textures/" + pathStr;
			strToTryOpenFromSteamDLC = ZString(m_pathStr + "SteamDLC/") + ZString(pathStr);

		}
		else {
			strPackFile = ZString(pathStr) + ("." + strExtensionArg);
			strToOpen = ZString(m_pathStr + pathStr) + ("." + strExtensionArg);
			strToTryOpenFromDev = ZString(m_pathStr + "dev/" + pathStr) + ("." + strExtensionArg);
			strToTryOpen = ZString(m_pathStr + "Textures/" + pathStr) + ("." + strExtensionArg);
			strToTryOpenFromSteamDLC = ZString(m_pathStr + "SteamDLC/") + ZString(pathStr) + ("." + strExtensionArg);
		}
		DWORD dwFileSize;
		void * pPackFile;
		pPackFile = CDX9PackFile::LoadFile(&strPackFile[0], &dwFileSize);
		if ((pPackFile != NULL) && (dwFileSize > 0))
		{
			pfile = new ZPackFile(strPackFile, pPackFile, dwFileSize);
		}

		// BT - STEAM
		if (pfile == NULL &&
			(strToTryOpenFromSteamDLC.Right(17) != "newgamescreen.mdl")) //newgamescreen needs to be ACSS-protected, so don't open it from mods
		{
			pfile = new ZFile(strToTryOpenFromSteamDLC, OF_READ | OF_SHARE_DENY_WRITE);

			if (!pfile->IsValid())
				pfile = NULL;


#ifdef STEAMSECURE
			// When building release mode, then enforce the artwork checksums on any MDL that is loaded by the engine.
			else if (m_fileHashTable.DoesFileHaveHash(strToTryOpenFromSteamDLC) == true)
				pfile = NULL;
#endif
		}

		// yp Your_Persona October 7 2006 : TextureFolder Patch
		// BT - 10/17 - HighRes Textures
		if ((pfile == NULL) && getHighresVersion == true &&
			(strToTryOpen.Right(7) == "bmp.mdl")) // if its a texture, try loading from the strToTryOpen
		{
			pfile = new ZFile(strToTryOpen, OF_READ | OF_SHARE_DENY_WRITE);
			// mmf modified Y_P's logic
			if (!pfile->IsValid())
			{
				pfile = NULL;
			}
		}
		if (!pfile) // if we dont have a file here, then load regularly.
		{
			// mmf #if this out for release.  I left the strtoTryOpenFromDev code in above

			// pkk - Use same conditional compilation like on registry keys
#ifdef _ALLEGIANCE_PROD_
			pfile = new ZFile(strToOpen, OF_READ | OF_SHARE_DENY_WRITE);
#else
			// KGJV try dev folder 1st
			pfile = new ZFile(strToTryOpenFromDev, OF_READ | OF_SHARE_DENY_WRITE);
			if (!pfile->IsValid()) {
				pfile = new ZFile(strToOpen, OF_READ | OF_SHARE_DENY_WRITE);
		}
			else {
				if (g_bMDLLog) {
					ZDebugOutput("'dev' file found for " + pathStr + "\n");
				}
			}
#endif     

			// mmf added debugf but will still have it call assert
			if (!pfile->IsValid()) {
				ZDebugOutput("Could not open the artwork file " + strToOpen + "\n");
				// this may fail/crash if strToOpen is fubar, but we are about to ZRAssert anyway
			}
			else
			{

#ifdef STEAMSECURE

				// BT - STEAM - Do the security checksum  on the loaded file here. Steam DRM wrapper will ensure that the Allegiance exe is not
				// tampered with, so basic checksums are all that is required.
				if (m_fileHashTable.IsHashCorrect(strToOpen, pfile) == false)
				{
					// Cause the calls downward to fail out.
					pfile = new ZFile("failsauce.nope");
				}
#endif
			}
		}

		//Imago 11/09/09 - Provide a helpful message box for this common error
		if (bError && !pfile->IsValid() && m_psite) {
			PostMessageA(GetActiveWindow(), WM_SYSCOMMAND, SC_MINIMIZE, 0);

#ifdef STEAMSECURE
			// BT - STEAM - Queue up a full content re-verify in case the user has a corrupted file.
			if (SteamUser() != nullptr && SteamUser()->BLoggedOn() == true)
				SteamApps()->MarkContentCorrupt(false);
#endif 

			// BT - STEAM
			MessageBoxA(GetDesktopWindow(), "Artwork file failed to validate: " + strToOpen + ", we have queued up an installation reverification. Check your Steam App in the downloads section for details..", "Allegiance: Fatal modeler error", MB_ICONERROR);
			exit(0);
		}
		ZRetailAssert(!(bError && !pfile->IsValid() && m_psite));

		return pfile->IsValid() ? pfile : NULL;
	}

	// BT - 10/17 - HighRes Textures
    TRef<ZFile> LoadFile(const PathString& pathStr, const ZString& strExtensionArg, bool bError, bool highRes)
    {
        return GetFile(pathStr, strExtensionArg, bError, highRes);
    }


	////////////////////////////////////////////////////////////////////////////////////////////////
	// LoadSurface()
	// Added bSystemMem flag. If this is specified, we keep the image in system memory rather 
	// than creating a D3D resource for it. Reasons for doing this: we don't want to waste texture
	// memory creating a surface that is going to be subdivided up into smaller textures, such
	// as when the source image is loaded for an AnimatedImage object.
	////////////////////////////////////////////////////////////////////////////////////////////////
	TRef<Surface> LoadSurface(const ZString& str, bool bColorKey, bool bError, bool bSystemMem )
    {
        TRef<ConstantImage> pimage; 
		CastTo(pimage, LoadImage(str, bColorKey, bError, bSystemMem));

        if (pimage) {
            return pimage->GetSurface();
        }

        return NULL;
    }


	////////////////////////////////////////////////////////////////////////////////////////////////
	// LoadImage()
	//
	////////////////////////////////////////////////////////////////////////////////////////////////
	TRef<Image> LoadImage(const ZString& str, bool bColorKey, bool bError, bool bSystemMem)
    {
        ZAssert(str.ToLower() == str);
        ZAssert(str.Right(3) == "bmp");

        //
        // Is the image already loaded?
        //

		// BT - 10/17 - HighRes Textures
		ZString namespaceName = str;

		if (m_bUseHighResTextures == true)
			namespaceName += "-highres";

        TRef<INameSpace> pns = GetCachedNameSpace(namespaceName);

        if (pns) {
            TRef<ConstantImage> pimage; CastTo(pimage, (Value*)pns->FindMember(str));
            if (pimage) {
                TRef<Surface> psurface = pimage->GetSurface();
                
                // HACK: Need to uncomment and track down the bug that's 
                // triggering this when a weapon fires, but this hack should
                // keep the debug client testable.  
                //ZAssert(bColorKey == psurface->HasColorKey());

                return pimage;
            }     
            
            return NULL;       
        }

        //
        // Try to load it
        //

		if( bColorKey == true )
			SetColorKeyHint( bColorKey );

        pns = GetNameSpace(str, bError, bSystemMem);

        if (pns) {
            TRef<ConstantImage> pimage; CastTo(pimage, (Value*)pns->FindMember(str));
            if (pimage) {
                TRef<Surface> psurface = pimage->GetSurface();
                psurface->SetName(str);

                if (bColorKey) {
                    psurface->SetColorKey(Color(0, 0, 0));
                }

                return pimage;
            }
        }

        return NULL;
    }

    HBITMAP LoadBitmap(const PathString& pathStrArg, bool bError)
    {
        PathString pathStr = m_pathStr + pathStrArg;
        ZString strExtension = pathStr.GetExtension();

        if (strExtension == "bmp" || strExtension.IsEmpty()) {
            if (strExtension.IsEmpty()) {
                pathStr += ".bmp";
            }

            return
                (HBITMAP)::LoadImageA(
                    NULL,
                    pathStr,
                    IMAGE_BITMAP,
                    0,
                    0,
                    LR_LOADFROMFILE | LR_CREATEDIBSECTION
                );
        }

        return NULL;
    }

    TRef<Geo> LoadGeo(const ZString& str, bool bError)
    {
        ZAssert(str.ToLower() == str);
        TRef<INameSpace> pns = GetNameSpace(str, bError, false);

        if (pns) {
            TRef<Geo> pgeo; CastTo(pgeo, (Value*)pns->FindMember("object"));
            return pgeo;
        }

        return NULL;
    }

    TRef<Geo> LoadXFile(
        const PathString& pathStr,
        Number* pnumberFrame,
        bool& bAnimation,
        bool bError
    ) {
        TRef<ZFile> pfile = GetFile(pathStr, "x", bError, false); // BT - 10/17 - HighRes Textures

        if (pfile) {
            return ::ImportXFile(this, pfile, pnumberFrame, bAnimation);
        }

        return NULL;
    }

    INameSpace* CreateNameSpace(const ZString& str)
    {
         TRef<INameSpace> pns = ::CreateNameSpace(str);

         m_mapNameSpace.Set(str, pns);

         return pns;
    }

    INameSpace* CreateNameSpace(const ZString& str, INameSpace* pnsParent)
    {
         TRef<INameSpace> pns = ::CreateNameSpace(str, pnsParent);

         m_mapNameSpace.Set(str, pns);

         return pns;
    }

    INameSpace* GetCachedNameSpace(const ZString& str)
    {
        TRef<INameSpace> pns;

        if (m_mapNameSpace.Find(str, pns)) {
            return pns;
        }

        return NULL;
    }
	
    INameSpace* GetNameSpace(const ZString& str, bool bError, bool bSystemMem)
    {
		// BT - 10/17 - HighRes Textures
		TRef<INameSpace> pns;
		
		ZString namespaceName = str;

		// if it's something that could have a high-res version...
		if (str.Right(3) == "bmp")
		{
			if (m_bUseHighResTextures == true)
			{
				namespaceName += "-highres";
				pns = GetCachedNameSpace(str + "-highres");
			}
			else
			{
				pns = GetCachedNameSpace(str);
			}
		}
		else
		{
			pns = GetCachedNameSpace(str);
		}

		if (pns)
			return pns;


		TRef<ZFile> pfile = GetFile(str, "mdl", bError, m_bUseHighResTextures);


		if (pfile != NULL)
		{
			bool bOriginalValue = SetSystemMemoryHint(bSystemMem);
			if (*(DWORD*)pfile->GetPointer(false, false) == MDLMagic) {
				if (g_bMDLLog) {
					ZDebugOutput("Reading Binary MDL file '" + str + "'\n");
				}
				pns = CreateBinaryNameSpace(namespaceName, this, pfile);
			}
			else {
				if (g_bMDLLog) {
					ZDebugOutput("Reading Text MDL file '" + str + "'\n");
				}
				pns = ::CreateNameSpace(namespaceName, this, pfile);
			}

			SetSystemMemoryHint(bOriginalValue);
			m_mapNameSpace.Set(namespaceName, pns);
			return pns;
		}

		return NULL;

    }

    void UnloadNameSpace(const ZString& str)
    {
        m_mapNameSpace.Remove(str);
    }

    void UnloadNameSpace(INameSpace* pns)
    {
        m_mapNameSpace.Remove(TRef<INameSpace>(pns));
    }

	bool SetColorKeyHint( const bool bColorKey )
	{
		bool bOldValue = m_bHintColorKey;
		m_bHintColorKey = bColorKey;
		return bOldValue;
	}

	bool GetColorKeyHint( )
	{
		return m_bHintColorKey;
	}

	bool SetSystemMemoryHint( const bool bSysMem )
	{
		bool bOldValue = m_bHintSystemMemory;
		m_bHintSystemMemory = bSysMem;
		return bOldValue;
	}
	
	bool GetSystemMemoryHint( )
	{
		return m_bHintSystemMemory;
	}

	bool SetUIImageUsageHint( const bool bUIImageUsage )
	{
		bool bOldValue = m_bHintUIImage;
		m_bHintUIImage = bUIImageUsage;
		return bOldValue;
	}

	bool GetUIImageUsageHint( )
	{
		return m_bHintUIImage;
	}
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Modeler> Modeler::Create(Engine* pengine)
{
    return new ModelerImpl(pengine);
}
