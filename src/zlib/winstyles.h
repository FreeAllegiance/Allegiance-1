#pragma once

#include "mask.h"

// broke these out into another file so this doesn't kill Source Insight
class StyleBaseType {};
typedef TBitMask<StyleBaseType, uint32_t> Style;

class StyleChild        : public Style { public: StyleChild       () : Style(WS_CHILD       ) {} };
class StylePopup        : public Style { public: StylePopup       () : Style(WS_POPUP       ) {} };
class StyleCaption      : public Style { public: StyleCaption     () : Style(WS_CAPTION     ) {} };
class StyleMaximizeBox  : public Style { public: StyleMaximizeBox () : Style(WS_MAXIMIZEBOX ) {} };
class StyleMinimizeBox  : public Style { public: StyleMinimizeBox () : Style(WS_MINIMIZEBOX ) {} };
class StyleSysMenu      : public Style { public: StyleSysMenu     () : Style(WS_SYSMENU     ) {} };
class StyleThickFrame   : public Style { public: StyleThickFrame  () : Style(WS_THICKFRAME  ) {} };
class StyleVisible      : public Style { public: StyleVisible     () : Style(WS_VISIBLE     ) {} };
class StyleClipChildren : public Style { public: StyleClipChildren() : Style(WS_CLIPCHILDREN) {} };
class StyleClipSiblings : public Style { public: StyleClipSiblings() : Style(WS_CLIPSIBLINGS) {} };
class StyleBorder       : public Style { public: StyleBorder      () : Style(WS_BORDER      ) {} };
class StyleTabStop      : public Style { public: StyleTabStop     () : Style(WS_TABSTOP     ) {} };
class StyleOverlapped   : public Style { public: StyleOverlapped  () : Style(WS_OVERLAPPEDWINDOW) {} };

class StyleBSPushButton : public Style { public: StyleBSPushButton() : Style(BS_PUSHBUTTON  ) {} };
class StyleBSCheckBox 	: public Style { public: StyleBSCheckBox  () : Style(BS_CHECKBOX    ) {} };
class StyleBSRadioButton: public Style { public: StyleBSRadioButton() : Style(BS_RADIOBUTTON) {} };
class StyleBSOwnerDraw  : public Style { public: StyleBSOwnerDraw () : Style(BS_OWNERDRAW   ) {} };
class StyleESLeft		: public Style { public: StyleESLeft	  () : Style(ES_LEFT		) {} };

class StyleEXBaseType {};
typedef TBitMask<StyleEXBaseType, uint32_t> StyleEX;

class StyleEXTopMost : public StyleEX { public: StyleEXTopMost() : StyleEX(WS_EX_TOPMOST) {} };
class StyleEXTransparent : public StyleEX { public: StyleEXTransparent() : StyleEX(WS_EX_TRANSPARENT) {} };
