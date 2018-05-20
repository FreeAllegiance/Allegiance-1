ADD_DEFINITIONS(-D__MODULE__="SoundEngine")
INCLUDE_DIRECTORIES(../Inc ../zlib)

SET(SoundEngine_SRC
    ds3dbuffer.cpp
    ds3dengine.cpp
    ds3dutil.cpp
    ds3dvirtualbuffer.cpp
    dummysound.cpp
    soundbase.cpp
    soundtemplates.cpp
    soundutil.cpp
   )

ADD_LIBRARY(SoundEngine ${SoundEngine_SRC})
SET_PROPERTY(TARGET SoundEngine PROPERTY CXX_STANDARD 14)
