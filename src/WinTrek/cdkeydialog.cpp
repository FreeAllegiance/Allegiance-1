#include "pch.h"

#include <button.h>
#include <controls.h>

//////////////////////////////////////////////////////////////////////////////
//
// CD Key Dialog
//
//////////////////////////////////////////////////////////////////////////////


class CDKeyDialogImpl : public IPopup, 
    public EventTargetContainer<CDKeyDialogImpl>
{
private:
    TRef<Pane> m_ppane;
    TRef<ButtonPane> m_pbuttonOK;
    TRef<ButtonPane> m_pbuttonCancel;
    TRef<EditPane> m_peditPane;

    TRef<IKeyboardInput> m_pkeyboardInputOldFocus;

public:
    
    CDKeyDialogImpl()
    {

        TRef<INameSpace> pns = GetModeler()->GetNameSpace("cdkeydialog");

        CastTo(m_ppane,                 pns->FindMember("CDKeyDialog"));

        CastTo(m_pbuttonOK,             pns->FindMember("CDKeyOkButtonPane"));
        CastTo(m_pbuttonCancel,         pns->FindMember("CDKeyCancelButtonPane"));
        CastTo(m_peditPane,      (Pane*)pns->FindMember("CDKeyEditPane"));

        m_peditPane->SetMaxLength(c_cbCDKey - 1);
		// mdvalley: These button events need a pointer now, as well as the class spelled out.
        AddEventTarget(&CDKeyDialogImpl::OnButtonOK, m_pbuttonOK->GetEventSource());
        AddEventTarget(&CDKeyDialogImpl::OnButtonCancel, m_pbuttonCancel->GetEventSource());
    }

    //
    // IPopup methods
    //

    Pane* GetPane()
    {
        return m_ppane;
    }

    ZString InsertHyphen(ZString strSource, int nOffset)
    {
        if (strSource.GetLength() < nOffset)
            return strSource;
        else
            return strSource.Left(nOffset) + ZString("-") + strSource.RightOf(nOffset);
    }

    ZString FormatKey(const ZString& strSource)
    {
        ZString strResult = strSource.ToUpper(); 

        // remove all spaces
        strResult.RemoveAll(' ');

        return strResult;
    }

    virtual void OnClose()
    {
        if (m_pkeyboardInputOldFocus)
            GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);

        m_pkeyboardInputOldFocus = NULL;

        IPopup::OnClose();
    }

    virtual void SetContainer(IPopupContainer* pcontainer)
    {
        m_pkeyboardInputOldFocus = GetEngineWindow()->GetFocus();
        GetEngineWindow()->SetFocus(m_peditPane);
        m_peditPane->SetString(FormatKey(trekClient.GetCDKey()));

        IPopup::SetContainer(pcontainer);
    }

    bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    {
        // we need to make sure we get OnChar calls to pass on to the edit box
        fForceTranslate = true;
        return false;
    }

    bool OnChar(IInputProvider* pprovider, const KeyState& ks)
    {
        if (ks.vk == 13)
        {
            OnButtonOK();
            return true;
        }
        else
        {
            bool bResult = ((IKeyboardInput*)m_peditPane)->OnChar(pprovider, ks);

            ZString strResult = m_peditPane->GetString();
            
            // add or remove hyphens as appropriate
            if (ks.vk != 8) // let backspace still delete things, however
                strResult = FormatKey(strResult);

            m_peditPane->SetString(strResult);

            return bResult;
        }
    }

    bool OnButtonOK()
    {
        ZString strCDKey = m_peditPane->GetString();

        if (!CDKeyIsValid(strCDKey))
        {
            TRef<IMessageBox> pmsgBox = CreateMessageBox("\"" + strCDKey 
                + "\" is not a valid CD Key.  Please enter the CD Key from "
                "the back of your CD Case");
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        }
        else
        {
            GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
            m_pkeyboardInputOldFocus = NULL;

            trekClient.SetCDKey(strCDKey);

            if (m_ppopupOwner) {
                m_ppopupOwner->ClosePopup(this);
            } else {
                m_pcontainer->ClosePopup(this);
            }
        }

        return true;
    }

    bool OnButtonCancel()
    {
        GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
        m_pkeyboardInputOldFocus = NULL;

        if (m_ppopupOwner) {
            m_ppopupOwner->ClosePopup(this);
        } else {
            m_pcontainer->ClosePopup(this);
        }

        return true;
    }

    static bool CDKeyIsValid(const ZString& strCDKey)
    {
        // TODO: do a more sound check.
        return strCDKey.GetLength() > 0;
    }
};


TRef<IPopup> CreateCDKeyPopup()
{
    return new CDKeyDialogImpl();
}
