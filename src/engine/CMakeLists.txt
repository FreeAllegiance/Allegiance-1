ADD_DEFINITIONS(-D__MODULE__="Engine" -D_ALLEGIANCE_PROD_)

IF(WIN32)
    ADD_DEFINITIONS(-DDIRECT3D_VERSION=0x0900 -DEnablePerformanceCounters)
ENDIF()

INCLUDE_DIRECTORIES(../Inc ../zlib ../Lib/AllegianceSecurity)

SET(Engine_SRC
    D3DDevice9.cpp
    DeviceModesDX9.cpp
    HashTable.cpp
    ImageTransfer.cpp
    LogFile.cpp
    RasterD3D9.cpp
    VBIBManager.cpp
    VRAMManager.cpp
    VertexGenerator.cpp
    VideoSettingsDX9.cpp
    bounds.cpp
    bspgeo.cpp
    button.cpp
    camera.cpp
    caption.cpp
    context.cpp
    controls.cpp
    dderror.cpp
    device3d.cpp
    dinput.cpp
    engine.cpp
    engineapp.cpp
    enginewindow.cpp
    font.cpp
    frameimage.cpp
    geometry.cpp
    image.cpp
    keyframe.cpp
    material.cpp
    mdl.cpp
    mdlbin.cpp
    menu.cpp
    modeler.cpp
    namespace.cpp
    pane.cpp
    paneimage.cpp
    pixelformat.cpp
    popup.cpp
    surface.cpp
    toppane.cpp
    transform.cpp
    transform2.cpp
    value.cpp
    viewport.cpp
    xfile.cpp
   )

ADD_LIBRARY(Engine ${Engine_SRC})
SET_PROPERTY(TARGET Engine PROPERTY CXX_STANDARD 14)
