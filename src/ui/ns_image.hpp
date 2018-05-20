
#pragma once

#include "ui.h"
#include "items.hpp"
#include "D3DDevice9.h"

TRef<ConstantImage> LoadImageFile(LuaScriptContext& context, std::string path) {
    std::string fullpath = context.FindPath(path);
    Engine* pEngine = context.GetEngine();

    if (fullpath == "") {
        throw std::runtime_error("Path not found: " + path);
    }

    const char* charString = fullpath.c_str();

    PathString pathString = PathString(ZString(charString));

    TRef<ZFile> zf = new ZFile(pathString, OF_READ | OF_SHARE_DENY_WRITE);
    ZFile * pFile = (ZFile*)zf;

    D3DXIMAGE_INFO fileInfo;
    if (D3DXGetImageInfoFromFileInMemory(pFile->GetPointer(),
        pFile->GetLength(),
        &fileInfo) == D3D_OK)
    {
        _ASSERT(fileInfo.ResourceType == D3DRTYPE_TEXTURE);

        // We can resize non-UI textures.
        WinPoint targetSize(fileInfo.Width, fileInfo.Height);

        // For D3D9, we only allow black colour keys.
        TRef<Surface> psurface =
            pEngine->CreateSurfaceD3DX(
                &fileInfo,
                &targetSize,
                zf,
                false,
                Color(0, 0, 0),
                pathString);

        return new ConstantImage(psurface, pathString);
    }
    else
    {
        throw std::runtime_error("Failed to load image: " + pathString);
    }
}

class ImageNamespace {
public:
    static void AddNamespace(LuaScriptContext& context) {
        sol::table table = context.GetLua().create_table();


        context.GetLua().new_usertype<ConstantImage>("ConstantImage",
            sol::base_classes, sol::bases<Image>()
        );
        context.GetLua().new_usertype<Image>("Image"
            );
        context.GetLua().new_usertype<TRef<Image>>("TRef<Image>");
        context.GetLua().new_usertype<TRef<ConstantImage>>("TRef<ConstantImage>");


        table["Empty"] = []() {
            return (TRef<Image>)Image::GetEmpty();
        };

        table["Lazy"] = [&context](sol::function callback) {
            return ImageTransform::Lazy(context.WrapCallback<TRef<Image>>(callback, Image::GetEmpty()));
        };

        table["Extent"] = sol::overload(
            [](const TRef<RectValue>& rect, const TRef<ColorValue>& color) {
                return CreateExtentImage(rect, color);
            },
            [](const TRef<PointValue>& pPoint, const TRef<ColorValue>& color) {
                Number* zero = new Number(0.0f);

                return CreateExtentImage(
                    RectTransform::Create(
                        zero,
                        zero,
                        PointTransform::X(pPoint),
                        PointTransform::Y(pPoint)
                    ),
                    color
                );
            }
        );
        table["MouseEvent"] = [](const TRef<Image>& image) {
            return (TRef<Image>)new MouseEventImage(image);
        };
        table["File"] = [&context](std::string path) {
            return (TRef<ConstantImage>)LoadImageFile(context, path);
        };
        table["Group"] = [](sol::object list) {
            if (list.is<TRef<ImageList>>()) {
                TRef<ImageList> varlist = list.as<TRef<ImageList>>();

                return (TRef<Image>)new NonStaticCallbackImage<ImageList*>([](ImageList* varlist) {
                    TRef<GroupImage> pgroup = new GroupImage();
                    for (auto entry : varlist->GetList()) {
                        pgroup->AddImageToTop(entry);
                    }
                    return (TRef<Image>)pgroup;
                }, varlist);
            }
            else if (list.is<sol::table>()) {
                TRef<GroupImage> pgroup = new GroupImage();

                sol::table table_list = list;
                int count = table_list.size();

                TRef<Image> child;

                for (int i = 1; i <= count; ++i) {
                    child = table_list.get<const TRef<Image>&>(i);
                    if (!child) {
                        throw std::runtime_error("Element in group should not be null");
                    }
                    pgroup->AddImageToTop(child);
                }

                return (TRef<Image>)pgroup;
            }
            throw std::runtime_error("Expected value argument of Image.Group to be either a table of images or a result from List.MapToImages");
        };

        table["StackVertical"] = [](sol::table list, sol::optional<sol::object> separation) {
            TRef<GroupImage> pgroup = new GroupImage();

            sol::table table_list = list;
            int count = table_list.size();

            TRef<Image> child;

            TRef<Number> pZero = new Number(0.0f);
            TRef<Number> offset_y = pZero;

            for (int i = 1; i <= count; ++i) {
                child = table_list.get<const TRef<Image>&>(i);
                if (!child) {
                    throw std::runtime_error("Element in group should not be null");
                }
                pgroup->AddImageToTop(ImageTransform::Translate(child, PointTransform::Create(pZero, offset_y)));

                offset_y = NumberTransform::Add(offset_y, PointTransform::Y(ImageTransform::Size(child)));
                if (separation) {
                    offset_y = NumberTransform::Add(offset_y, wrapValue<float>(separation.value()));
                }
            }

            return (TRef<Image>)pgroup;
        };
        table["StackVertical"] = [](TRef<ImageList> list, sol::optional<sol::object> separation) {
            return (TRef<Image>)new NonStaticCallbackImage<ImageList*>([separation](ImageList* varlist) {
                TRef<GroupImage> pgroup = new GroupImage();

                TRef<Number> pZero = new Number(0.0f);
                TRef<Number> offset_y = pZero;

                for (TRef<Image> entry : varlist->GetList()) {
                    pgroup->AddImageToTop(ImageTransform::Translate(entry, PointTransform::Create(pZero, offset_y)));

                    offset_y = NumberTransform::Add(offset_y, PointTransform::Y(ImageTransform::Size(entry)));
                    if (separation) {
                        offset_y = NumberTransform::Add(offset_y, wrapValue<float>(separation.value()));
                    }
                }

                return (TRef<Image>)pgroup;
            }, list);
        };

        table["Switch"] = [&context](sol::object value, sol::table table) {
            int count = table.size();

            if (value.is<TRef<Number>>() || value.is<float>()) {
                std::map<int, TRef<Image>> mapOptions;

                table.for_each([&mapOptions](sol::object key, sol::object value) {
                    int fKey = (int)key.as<float>();
                    mapOptions[fKey] = value.as<const TRef<Image>&>();
                });

                return ImageTransform::Switch(wrapValue<float>(value), mapOptions);
            }
            else if (value.is<TRef<TStaticValue<ZString>>>() || value.is<std::string>()) {
                //the wrapped value is a ZString, the unwrapped value a std::string
                std::map<std::string, TRef<Image>> mapOptions;

                table.for_each([&mapOptions](sol::object key, sol::object value) {
                    std::string strKey = key.as<std::string>();
                    mapOptions[strKey] = value.as<const TRef<Image>&>();
                });

                return ImageTransform::Switch(wrapString(value), mapOptions);
            }
            else if (value.is<TRef<Boolean>>() || value.is<bool>()) {
                std::map<bool, TRef<Image>> mapOptions;

                table.for_each([&mapOptions](sol::object key, sol::object value) {
                    bool bKey = key.as<bool>();
                    mapOptions[bKey] = value.as<const TRef<Image>&>();
                });

                return ImageTransform::Switch(wrapValue<bool>(value), mapOptions);
            }
            else if (value.is<TRef<UiStateValue>>()) {
                std::map<std::string, sol::function> mapOptions;

                table.for_each([&mapOptions](sol::object key, sol::object value) {
                    std::string strKey = key.as<std::string>();
                    mapOptions[strKey] = value.as<sol::function>();
                });

                return (TRef<Image>)new CallbackImage<std::shared_ptr<UiState>>([&context, mapOptions](const std::shared_ptr<UiState>& state) {
                    auto find = mapOptions.find(state->GetName());
                    if (find == mapOptions.end()) {
                        return (TRef<Image>)Image::GetEmpty();
                    }
                    return (TRef<Image>)context.WrapCallback<TRef<Image>, const std::shared_ptr<UiState>&>(find->second, Image::GetEmpty())(state);
                }, value.as<TRef<UiStateValue>>());
            }
            throw std::runtime_error("Expected value argument of Image.Switch to be either a wrapped or unwrapped bool, int, or string");
        };
        table["String"] = sol::overload(
            [](const TRef<FontValue>& font, const TRef<ColorValue>& color, sol::object width, sol::object string, sol::optional<Justification> justification_arg) {
                return ImageTransform::String(font, color, wrapValue<float>(width), wrapString(string), justification_arg.value_or(JustifyLeft()), new Number(0.0f));
            },
            [](const TRef<FontValue>& font, const TRef<ColorValue>& color, sol::object string) {

                TRef<Number> width = new Number(10000); //something large as default
                Justification justification = JustifyLeft();
                TRef<Number> separation = new Number(0.0f);

                return ImageTransform::String(font, color, width, wrapString(string), justification, separation);
            },
            [](const TRef<FontValue>& font, const TRef<ColorValue>& color, sol::object string, sol::table object) {

                TRef<Number> width = new Number(10000); //something large as default
                Justification justification = JustifyLeft();
                TRef<Number> separation = new Number(0.0f);

                if (object) {
                    object.for_each([&width, &justification, &separation](sol::object key, sol::object value) {
                        std::string strKey = key.as<std::string>();
                        if (strKey == "Width") {
                            width = wrapValue<float>(value);
                        }
                        else if (strKey == "Line separation") {
                            separation = wrapValue<float>(value);
                        }
                        else if (strKey == "Justification") {
                            justification = value.as<Justification>();
                        }
                        else {
                            throw std::runtime_error("Unknown key.");
                        }
                    });
                }

                return ImageTransform::String(font, color, width, wrapString(string), justification, separation);
            }
        );
        table["Translate"] = [](const TRef<Image>& pimage, const TRef<PointValue>& pPoint) {
            return ImageTransform::Translate(pimage, pPoint);
        };
        table["Scale"] = [](const TRef<Image>& pimage, const TRef<PointValue>& pPoint) {
            return ImageTransform::Scale(pimage, pPoint);
        };
        table["Rotate"] = [](const TRef<Image>& pimage, sol::object radians) {
            return ImageTransform::Rotate(pimage, wrapValue<float>(radians));
        };
        table["Size"] = [](const TRef<Image>& pimage) {
            return ImageTransform::Size(pimage);
        };

        table["Justify"] = [](const TRef<Image>& pimage, TRef<PointValue>& pSizeContainer, Justification justification) {
            return ImageTransform::Justify(pimage, pSizeContainer, justification);
        };
        table["ScaleFit"] = [](const TRef<Image>& pimage, TRef<PointValue>& pSizeContainer, Justification justification) {
            return ImageTransform::ScaleFit(pimage, pSizeContainer, justification);
        };
        table["ScaleFill"] = [](const TRef<Image>& pimage, TRef<PointValue>& pSizeContainer, Justification justification) {
            return ImageTransform::ScaleFill(pimage, pSizeContainer, justification);
        };

        table["Clip"] = [](const TRef<Image>& pimage, const TRef<RectValue>& rect) {
            return ImageTransform::Clip(pimage, rect);
        };
        table["Cut"] = [](const TRef<Image>& pimage, const TRef<RectValue>& rect) {
            return ImageTransform::Cut(pimage, rect);
        };

        table["Multiply"] = [](const TRef<ConstantImage>& pimage, const TRef<ColorValue>& color) {
            return ImageTransform::Multiply(pimage, color);
        };
        context.GetLua().set("Image", table);

        sol::table tableJustification = context.GetLua().create_table();
        tableJustification["Left"] = (Justification)JustifyLeft();
        tableJustification["Right"] = (Justification)JustifyRight();
        tableJustification["Top"] = (Justification)JustifyTop();
        tableJustification["Bottom"] = (Justification)JustifyBottom();
        tableJustification["Center"] = (Justification)JustifyCenter();
        tableJustification["Topleft"] = (Justification)(JustifyTop() | JustifyLeft());
        tableJustification["Topright"] = (Justification)(JustifyTop() | JustifyRight());
        tableJustification["Bottomleft"] = (Justification)(JustifyBottom() | JustifyLeft());
        tableJustification["Bottomright"] = (Justification)(JustifyBottom() | JustifyRight());
        context.GetLua().set("Justify", tableJustification);
    }
};