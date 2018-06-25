#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// Password Dialog
//
//////////////////////////////////////////////////////////////////////////////


class PasswordDialogImpl : public IPopup, 
    public EventTargetContainer<PasswordDialogImpl>, 
    public TrekClientEventSink
{
private:
    TRef<Pane> m_ppane;
    TRef<ButtonPane> m_pbuttonOK;
    TRef<ButtonPane> m_pbuttonCancel;
    TRef<EditPane> m_peditPane;
    MissionInfo* m_pMission;

    PasswordDialogSink* m_pparent;
    TRef<IKeyboardInput> m_pkeyboardInputOldFocus;

public:
    
    PasswordDialogImpl(PasswordDialogSink* pparent, const ZString& strPassword, MissionInfo* pmission)
    {
        m_pMission = NULL;
        m_pparent = pparent;

        TRef<INameSpace> pns = GetModeler()->GetNameSpace("passworddialog");

        CastTo(m_ppane,                 pns->FindMember("PasswordDialog"));

        CastTo(m_pbuttonOK,             pns->FindMember("passwordOkButtonPane"));
        CastTo(m_pbuttonCancel,         pns->FindMember("passwordCancelButtonPane"));
        CastTo(m_peditPane,      (Pane*)pns->FindMember("passwordEditPane"));

        m_peditPane->SetType(EditPane::Password);
        m_peditPane->SetString(strPassword);
        m_peditPane->SetMaxLength(c_cbGamePassword - 1);
		// mdvalley: Need pointer and name.
        AddEventTarget(&PasswordDialogImpl::OnButtonOK, m_pbuttonOK->GetEventSource());
        AddEventTarget(&PasswordDialogImpl::OnButtonCancel, m_pbuttonCancel->GetEventSource());
    }

    void SetMission(MissionInfo* pmission)
    {
        ZAssert(m_pMission == NULL);
        m_pMission = pmission;
    }

    //
    // IPopup methods
    //

    Pane* GetPane()
    {
        return m_ppane;
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
        m_peditPane->SetString("");

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
            return ((IKeyboardInput*)m_peditPane)->OnChar(pprovider, ks);
    }

    bool OnButtonOK()
    {
        GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
        m_pkeyboardInputOldFocus = NULL;

        PasswordDialogSink* pparent = m_pparent;
        ZString strPassword = m_peditPane->GetString();
        m_pMission = NULL;

        if (m_ppopupOwner) {
            m_ppopupOwner->ClosePopup(this);
        } else {
            m_pcontainer->ClosePopup(this);
        }
        pparent->OnPassword(strPassword);

        return true;
    }

    bool OnButtonCancel()
    {
        GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
        m_pkeyboardInputOldFocus = NULL;

        m_pMission = NULL;
        PasswordDialogSink* pparent = m_pparent;

        if (m_ppopupOwner) {
            m_ppopupOwner->ClosePopup(this);
        } else {
            m_pcontainer->ClosePopup(this);
        }
        pparent->OnCancelPassword();

        return true;
    }

    void OnDelMission(MissionInfo* pmission)
    {
        if (m_pMission == pmission)
        {
            // our mission has ended - bail.
            GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
            m_pkeyboardInputOldFocus = NULL;

            if (m_ppopupOwner) {
                m_ppopupOwner->ClosePopup(this);
            } else {
                m_pcontainer->ClosePopup(this);
            }
            m_pMission = NULL;

            TRef<IMessageBox> pmsgBox = 
                CreateMessageBox("The mission you were trying to join has ended.");
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);

            m_pparent->OnCancelPassword();
        }
    }
};


TRef<IPopup> CreatePasswordPopup(PasswordDialogSink* psink, const ZString& strPassword, MissionInfo* pmission)
{
    return new PasswordDialogImpl(psink, strPassword, pmission);
}
