#ifndef _trekmdl_h_
#define _trekmdl_h_

void ExportPaneFactories(INameSpace* pns);

//
// buttons with sounds for mouse hover and mouse clicks
//
TRef<ButtonPane> CreateTrekButton(
    ButtonFacePane* ppane,
    bool    bToggle     = false,
    SoundID soundClick  = positiveButtonClickSound,
    float repeatRate    = 0.0f,
    float repeatDelay   = 0.0f
);

TRef<ButtonPane> CreateTrekButton(
	Pane* ppaneUp, 
	Pane* ppaneDown, 
    bool  bToggle       = false,
    SoundID soundClick  = positiveButtonClickSound,
    float repeatRate    = 0.0f,
    float repeatDelay   = 0.0f
);

class ValuePane : public Value, public Pane
{
public:
    ValuePane(){};
    ValuePane(Pane* ppaneChild) : Pane(ppaneChild) {};
    DefineSubclassMembers(ValuePane);
};

class SectorMapPaneFactory : public IFunction
{
public:
    TRef<IObject> Apply(ObjectStack& stack);
};

class SectorInfoPaneFactory : public IFunction
{
public:
    TRef<IObject> Apply(ObjectStack& stack);
};

class PurchasesPaneFactory : public IFunction
{
public:
    TRef<IObject> Apply(ObjectStack& stack);
};

class PartInfoPaneFactory : public IFunction
{
public:
    TRef<IObject> Apply(ObjectStack& stack);
};

class SelectionPaneFactory : public IFunction
{
public:
    TRef<IObject> Apply(ObjectStack& stack);
};

class SelectModelPaneFactory : public IFunction
{
public:
    TRef<IObject> Apply(ObjectStack& stack);
};

class ChatListPane : public ValuePane
{
public:
    virtual void AddChannel(ChatTarget channel)    = 0;
    virtual void RemoveChannel(ChatTarget channel) = 0;
    virtual void ResetChannels()                   = 0;
    virtual void PageUp()                          = 0;
    virtual void PageDown()                        = 0;
	virtual void SetChatLines(int lines)		   = 0; // #294 - Turkey
};

class ChatListPaneFactory : public IFunction
{
public:
    TRef<IObject> Apply(ObjectStack& stack);
};

class InventoryImageFactory : public IFunction
{
public:
    TRef<IObject> Apply(ObjectStack& stack);
};

#endif
