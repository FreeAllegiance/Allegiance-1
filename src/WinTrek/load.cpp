#include "pch.h"

#include <button.h>
#include <controls.h>
#include <geometry.h>
#include <image.h>
#include <paneimage.h>
#include "valuetransform.h"

const Money moneyLots = 0x7fffffff;

//////////////////////////////////////////////////////////////////////////////
//
// Loadout
//
//////////////////////////////////////////////////////////////////////////////

static  const char* GetBuyableName(IbuyableIGC*   ppt)
{
    const char* name = ppt->GetName();
    Money       cost = ppt->GetPrice();

    if (cost == 0)
        return name;
    else
    {
        static char bfr[c_cbName + 10];
        strcpy(bfr, name);
        strcat(bfr, " $(");
        _itoa(cost, bfr + strlen(bfr), 10);
        strcat(bfr, ")");

        return bfr;
    }


}

bool g_bTest = false;
const int c_midOtherParts = -2;
const int ET_Turret = -2;

class LoadoutImpl :
    public Loadout,
    public EventTargetContainer<LoadoutImpl>,
    public TrekClientEventSink,
    public ISubmenuEventSink,
    public IMenuCommandSink,
    public IKeyboardInput //AaronMoore 1/10
{
public:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Slot
    //
    //////////////////////////////////////////////////////////////////////////////

    class Slot : public Pane, public TrekClientEventSink {
    private:
        LoadoutImpl*   m_ploadout;
        TRef<Image>    m_pimageDefault;
        TRef<Image>    m_pimage;
        EquipmentType  m_equipmentType;
        Mount          m_mount;
        TRef<IpartIGC> m_ppart;
        ZString        m_strPrefix;

    public:
        Slot(
            LoadoutImpl*   ploadout,
            EquipmentType  equipmentType,
            Mount          mount,
            Image*         pimageDefault,
            const ZString& strPrefix
        ) :
            m_equipmentType(equipmentType),
            m_mount(mount),
            m_ploadout(ploadout),
            m_pimageDefault(pimageDefault),
            m_pimage(pimageDefault),
            m_strPrefix(strPrefix)
        {
        }

        //////////////////////////////////////////////////////////////////////////////
        //
        // Methods
        //
        //////////////////////////////////////////////////////////////////////////////

        IpartIGC* GetPart()
        {
            return m_ppart;
        }

        EquipmentType GetEquipmentType()
        {
            if (m_equipmentType == ET_Turret)
                return ET_Weapon;
            else
                return m_equipmentType;
        }

        Mount GetMount()
        {
            if (m_equipmentType == ET_Turret)
                return m_mount + m_ploadout->m_phullType->GetMaxFixedWeapons();
            else
                return m_mount;
        }

        static bool SlotValidForHull(const IhullTypeIGC* pht, EquipmentType equipmentType, Mount mount)
        {
            if (equipmentType == NA)
                return true;
            else if (equipmentType == ET_Weapon)
                return mount < pht->GetMaxFixedWeapons();
            else if (equipmentType == ET_Turret)
                return mount < pht->GetMaxWeapons() - pht->GetMaxFixedWeapons();
            else
                return pht->GetPartMask(equipmentType, mount) != 0;
        }

        void SetPart(IpartIGC* ppart)
        {
            m_ppart = ppart;

            if (ppart) {
                IpartTypeIGC* ppartType = m_ppart->GetPartType();

                //
                // Try to load the bitmap that fits this slot.
                //

                ZString str = (m_strPrefix + ZString(ppartType->GetModelName()) + "bmp").ToLower();
                m_pimage = GetModeler()->LoadImage(str, false, false);

                //
                // If we can't find the right bitmap load the default bitmap.
                //

                if (m_pimage == NULL) {
                    ZString str = (ZString("l") + ZString(ppartType->GetModelName()) + "bmp").ToLower();
                    m_pimage = GetModeler()->LoadImage(str, false);
                }
            } else {
                m_pimage = m_pimageDefault;
            }

            NeedPaint();
        }

        //////////////////////////////////////////////////////////////////////////////
        //
        // Pane Methods
        //
        //////////////////////////////////////////////////////////////////////////////

        void UpdateLayout()
        {
            m_pimageDefault->Update();
            InternalSetSize(
                WinPoint(133, 37)
                //::Cast(m_pimageDefault->GetBounds().GetRect().Size())
            );
            Pane::UpdateLayout();
        }

        void Paint(Surface* psurface)
        {
            psurface->BitBlt(WinPoint(0, 0), m_pimage->GetSurface());

            if (m_ppart)
            {
                // if this is a turret with a mounted weapon
                if (m_equipmentType == ET_Turret)
                {
                    IhullTypeIGC*   pht = trekClient.GetShip()->GetBaseHullType();
                    if (pht)
                    {
                        // display the current gunner for that turret
                        IshipIGC* pshipGunner 
                            = trekClient.GetShip()->GetGunner(GetMount() + pht->GetMaxFixedWeapons() -
                                                              m_ppart->GetShip()->GetBaseHullType()->GetMaxFixedWeapons());

                        if (pshipGunner)
                        {
                            psurface->DrawString(
                                TrekResources::SmallFont(),
                                MakeColorFromCOLORREF(TrekResources::WhiteColor()),
                                WinPoint(3,24), 
                                pshipGunner->GetName()
                                );
                        }
                        else
                        {
                            psurface->DrawString(
                                TrekResources::SmallFont(),
                                MakeColorFromCOLORREF(TrekResources::WhiteColor()),
                                WinPoint(3,24), 
                                "<open>"
                                );
                        }
                    }
                }
                // otherwise, if this is a launcher
                else
                {
                    ObjectType  type = m_ppart->GetObjectType();
                    if ((type == OT_pack) || IlauncherIGC::IsLauncher(m_ppart->GetObjectType()))
                    {
                        ZString strText((int)m_ppart->GetAmount());
                        int nTextWidth = TrekResources::SmallFont()->GetTextExtent(strText).X();

                        // display the count of expendibles
                        psurface->DrawString(
                            TrekResources::SmallFont(),
                            MakeColorFromCOLORREF(TrekResources::WhiteColor()),
                            WinPoint(130 - nTextWidth, 24), 
                            strText
                            );
                    }
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////////
        //
        // IMouseInput Methods
        //
        //////////////////////////////////////////////////////////////////////////////

        void MouseEnter(IInputProvider* pprovider, const Point& point)
        {
            m_ploadout->SlotEntered(this);
            trekClient.PlaySoundEffect(mouseoverSound);
        }

        void MouseLeave(IInputProvider* pprovider)
        {
            m_ploadout->SlotLeft(this);
        }

        MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
        {
            if (bDown) {
                //m_ploadout->SlotEntered(this);
                m_ploadout->SlotClicked(this, button == 0, WinPoint::Cast(point));
            }

            return MouseResult();
        }

        void OnBoardShip(IshipIGC* pshipChild, IshipIGC* pshipParent)
        {
            if (pshipParent == trekClient.GetShip() || pshipParent == NULL)
            {
                NeedPaint();
            }
        }
    };

    typedef TList<TRef<Slot> > SlotList;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Members
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<Modeler>           m_pmodeler;
    TRef<Number>            m_ptime;
    TRef<Pane>              m_ppane;
    TRef<Pane>              m_ppaneGeo;
    TRef<Image>             m_pBlankImage;
    TRef<INameSpace>        m_pnsLoadout;
    TRef<INameSpace>        m_pnsLoadoutPane;
    TRef<INameSpace>        m_pnsDefaultLoadout;

    SlotList                m_listSlots;
    TRef<Slot>              m_pslotMenu;
    TRef<IMenu>             m_pmenu;
    TRef<IMenu>             m_phullMenu;
    TRef<ISubmenuEventSink> m_psubmenuEventSink;

    TRef<IKeyboardInput> m_keyboardDelegate; //AaronMoore 1/10

    //
    // IGC stuff
    //

    IshipIGC*               m_pship;
    const IhullTypeIGC*     m_phullType;
    IhullTypeIGC*           m_phullTypeBase;

    float                   m_fSpecSignature;
    float                   m_fSpecMass;

    //
    // Geo Pane
    //

    TRef<Camera>            m_pcamera;
    TRef<Viewport>          m_pviewport;
    TRef<WrapGeo>           m_pwrapGeo;
    TRef<WrapImage>         m_pwrapImageGeo;

    /*
    TRef<Material>          m_pmaterialCone;
    TRef<VisibleGeo>        m_pvisibleGeoCone;
    TRef<MatrixTransform>   m_ptransformCone;
    TRef<ModifiableNumber>  m_pnumberAngleCone;
    float                   m_scaleCone;
    */

    TRef<StringPane>        m_pstringPartMask;
    //
    // Buttons
    //

    TRef<ButtonPane> m_pbuttonDefault;
    TRef<ButtonPane> m_pbuttonNext;
    TRef<ButtonPane> m_pbuttonPrevious;
    TRef<ButtonPane> m_pbuttonLaunch;
    TRef<ButtonPane> m_pbuttonBuy;
    TRef<ButtonPane> m_pbuttonConfirm;
    TRef<ButtonPane> m_pbuttonBack;
    TRef<ButtonPane> m_pbuttonHullList;

    TRef<Pane>       m_ppaneHoverBuy;
    TRef<Pane>       m_ppaneHoverConfirm;

    TRef<PointValue> m_ppointBuyButton;

    //
    // Exported Values
    //

	TRef<WrapImage>		   m_pwrapImagePart;
	TRef<WrapImage>		   m_pwrapImageCiv;
	TRef<WrapImage>		   m_pwrapImageHullCiv;
    TRef<ModifiableString> m_pstringGunners[c_maxMountedWeapons];

    TRef<ModifiableString> m_pstringTitle          ;
    TRef<ModifiableString> m_pstringPartName       ;
    TRef<ModifiableString> m_pstringPartDescription;
    TRef<ModifiableString> m_pstringHullDescription;
    
    TRef<ModifiableNumber> m_pnumberMoney;
    TRef<ModifiableNumber> m_pnumberOldShipCost;
    TRef<ModifiableNumber> m_pnumberNewShipCost;

    TRef<ModifiableNumber> m_pnumberLoadoutShipState;


    // parts

    TRef<ModifiableNumber> m_pnumberPartID         ;
    TRef<ModifiableNumber> m_pnumberPartMask       ;
    TRef<ModifiableNumber> m_pnumberEquipmentType  ;
    TRef<ModifiableNumber> m_pnumberSignature      ;
    TRef<ModifiableNumber> m_pnumberMass           ;
    TRef<ModifiableNumber> m_pnumberSignatureNumber     ;
    TRef<ModifiableNumber> m_pnumberMassNumber          ;
    TRef<ModifiableNumber> m_pnumberWeaponDamage   ;
    TRef<ModifiableNumber> m_pnumberWeaponRate     ;
    TRef<ModifiableNumber> m_pnumberWeaponRange    ;
    TRef<ModifiableNumber> m_pnumberWeaponSpeed    ;
    TRef<ModifiableNumber> m_pnumberWeaponDamageNumber  ;
    TRef<ModifiableNumber> m_pnumberWeaponRateNumber    ;
    TRef<ModifiableNumber> m_pnumberWeaponRangeNumber   ;
    TRef<ModifiableNumber> m_pnumberWeaponSpeedNumber   ;

    TRef<ModifiableNumber> m_pnumberShieldRegen    ;
    TRef<ModifiableNumber> m_pnumberShieldHitPts   ;
    TRef<ModifiableNumber> m_pnumberShieldRegenNumber    ;
    TRef<ModifiableNumber> m_pnumberShieldHitPtsNumber   ;

    TRef<ModifiableNumber> m_pnumberCloakCloaking  ;
    TRef<ModifiableNumber> m_pnumberCloakDuration  ;
    TRef<ModifiableNumber> m_pnumberCloakCloakingNumber  ;
    TRef<ModifiableNumber> m_pnumberCloakDurationNumber  ;
    
    TRef<ModifiableNumber> m_pnumberMagazineDamage  ;
    TRef<ModifiableNumber> m_pnumberMagazineRange       ;
    TRef<ModifiableNumber> m_pnumberMagazineLockTime    ;
    TRef<ModifiableNumber> m_pnumberMagazineMaxLock     ;
    TRef<ModifiableNumber> m_pnumberMagazineDamageNumber    ;
    TRef<ModifiableNumber> m_pnumberMagazineRangeNumber     ;
    TRef<ModifiableNumber> m_pnumberMagazineLockTimeNumber  ;
    TRef<ModifiableNumber> m_pnumberMagazineMaxLockNumber   ;

    TRef<ModifiableNumber> m_pnumberAfterburnerMaxSpeed ;
    TRef<ModifiableNumber> m_pnumberAfterburnerBurnRate     ;
    TRef<ModifiableNumber> m_pnumberAfterburnerMaxSpeedNumber   ;
    TRef<ModifiableNumber> m_pnumberAfterburnerBurnRateNumber   ;

    TRef<ModifiableNumber> m_pnumberChaffStrength   ;
    TRef<ModifiableNumber> m_pnumberChaffStrengthNumber ;

    TRef<ModifiableNumber> m_pnumberPackQuantityNumber  ;

    TRef<ModifiableNumber> m_pnumberProbeScanRange  ;
    TRef<ModifiableNumber> m_pnumberProbeVisibility  ;
    TRef<ModifiableNumber> m_pnumberProbeHitPts     ;
    TRef<ModifiableNumber> m_pnumberProbeLifespan   ;
    TRef<ModifiableNumber> m_pnumberProbeScanRangeNumber    ;
    TRef<ModifiableNumber> m_pnumberProbeVisibilityNumber    ;
    TRef<ModifiableNumber> m_pnumberProbeHitPtsNumber   ;
    TRef<ModifiableNumber> m_pnumberProbeLifespanNumber ;

    TRef<ModifiableNumber> m_pnumberMineVisibility  ;
    TRef<ModifiableNumber> m_pnumberMineVisibilityNumber  ;
    TRef<ModifiableNumber> m_pnumberMineDamage    ;
    TRef<ModifiableNumber> m_pnumberMineRadius    ;
    TRef<ModifiableNumber> m_pnumberMineLifespan  ;
    TRef<ModifiableNumber> m_pnumberMineDamageNumber  ;
    TRef<ModifiableNumber> m_pnumberMineRadiusNumber  ;
    TRef<ModifiableNumber> m_pnumberMineLifespanNumber    ;

    // hull

    TRef<ModifiableNumber> m_pnumberHullMaxSpeed    ;
    TRef<ModifiableNumber> m_pnumberHullAcceleration;
    TRef<ModifiableNumber> m_pnumberHullTurnRate    ;
    TRef<ModifiableNumber> m_pnumberHullHitPoints   ;
    TRef<ModifiableNumber> m_pnumberHullEnergy      ;
    TRef<ModifiableNumber> m_pnumberHullScanRange   ;
    TRef<ModifiableNumber> m_pnumberHullSignature   ;
    TRef<ModifiableNumber> m_pnumberHullMass        ;

    TRef<ModifiableNumber> m_pnumberHullMaxSpeedNumber    ;
    TRef<ModifiableNumber> m_pnumberHullAccelerationNumber;
    TRef<ModifiableNumber> m_pnumberHullTurnRateNumber    ;
    TRef<ModifiableNumber> m_pnumberHullHitPointsNumber   ;
    TRef<ModifiableNumber> m_pnumberHullEnergyNumber      ;
    TRef<ModifiableNumber> m_pnumberHullScanRangeNumber   ;
    TRef<ModifiableNumber> m_pnumberHullSignatureNumber   ;
    TRef<ModifiableNumber> m_pnumberHullMassNumber        ;

    TRef<ModifiableNumber> m_pnumberBaseHullMaxSpeed    ;
    TRef<ModifiableNumber> m_pnumberBaseHullAcceleration;
    TRef<ModifiableNumber> m_pnumberBaseHullTurnRate    ;
    TRef<ModifiableNumber> m_pnumberBaseHullHitPoints   ;
    TRef<ModifiableNumber> m_pnumberBaseHullEnergy      ;
    TRef<ModifiableNumber> m_pnumberBaseHullScanRange   ;
    TRef<ModifiableNumber> m_pnumberBaseHullSignature   ;
    TRef<ModifiableNumber> m_pnumberBaseHullMass        ;

    TRef<ModifiableNumber> m_pnumberBaseHullMaxSpeedNumber    ;
    TRef<ModifiableNumber> m_pnumberBaseHullAccelerationNumber;
    TRef<ModifiableNumber> m_pnumberBaseHullTurnRateNumber    ;
    TRef<ModifiableNumber> m_pnumberBaseHullHitPointsNumber   ;
    TRef<ModifiableNumber> m_pnumberBaseHullEnergyNumber      ;
    TRef<ModifiableNumber> m_pnumberBaseHullScanRangeNumber   ;
    TRef<ModifiableNumber> m_pnumberBaseHullSignatureNumber   ;
    TRef<ModifiableNumber> m_pnumberBaseHullMassNumber        ;

    TRef<ModifiableNumber> m_pnumberHullDefenseClass        ;
    TRef<ModifiableNumber> m_pnumberBaseHullDefenseClass    ;
    TRef<ModifiableNumber> m_pnumberShieldDefenseClass  ;
    TRef<ModifiableNumber> m_pnumberWeaponDamageBonus       ;
    TRef<ModifiableNumber> m_pnumberMagazineDamageBonus     ;

    //
    // Imported Values
    //

    float m_maxWeaponRate    ;
    float m_maxWeaponPower   ;
    float m_maxWeaponRange   ;
    float m_maxWeaponSpeed   ;
    float m_maxShieldParticle;
    float m_maxShieldEnergy  ;
    float m_maxShieldRegen   ;
    float m_maxShieldHitPts  ;
    float m_maxCloakEnergy   ;
    float m_maxPartMass      ;
    float m_maxPartSignature ;

    float m_maxMagazineHitDamage    ;
    float m_maxMagazineAreaDamage   ;
    float m_maxMagazineRange        ;
    float m_maxMagazineLockTime     ;
    float m_maxMagazineMaxLock      ;

    float m_maxAfterburnerMaxThrust;
    float m_maxAfterburnerBurnRate;
    float m_maxAfterburnerOnRate;

    float m_maxChaffStrength;

    float m_maxProbeScanRange;
    float m_maxProbeHitPts;
    float m_maxProbeLifespan;

    float m_maxMineDamage ;
    float m_maxMineRadius ;
    float m_maxMineLifespan   ;

    float m_maxMaxSpeed     ;
    float m_maxAcceleration ;
    float m_maxTurnRate     ;
    float m_maxHitPoints    ;
    float m_maxEnergy       ;
    float m_maxScanRange    ;
    float m_maxHullSignature;
    float m_maxHullMass     ;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Constructor
    //
    //////////////////////////////////////////////////////////////////////////////

    LoadoutImpl(Modeler* pmodeler, Number* ptime) :
        m_pmodeler(pmodeler),
        m_ptime(ptime),
        m_phullType(NULL),
        m_phullTypeBase(NULL)
    {
        // calculate the mass and signature of the default loadout
        IshipIGC*   pshipSource = trekClient.GetShip()->GetSourceShip();

        Money budget = moneyLots;
        m_pship = trekClient.CreateEmptyShip();

        trekClient.BuyDefaultLoadout(m_pship, trekClient.GetShip()->GetStation(), 
            pshipSource->GetBaseHullType(), &budget);
        m_fSpecMass = m_pship->GetMass();
        m_fSpecSignature = m_pship->GetSignature();
        m_pship->Terminate();
        m_pship->Release();

        // copy their current loadout, upgrading if we can.
        m_pship = trekClient.CreateEmptyShip();
        trekClient.RestoreLoadout(pshipSource, m_pship, trekClient.GetShip()->GetStation(),
                                  trekClient.GetMoney() + pshipSource->GetValue());

        InitializeMDL();
        InitializePanes();
        UpdateHullType();
        UpdateCivBitmap();
        UpdatePartInfo(NULL, NULL, NULL);
        UpdateNextAndPrevButtons();
        m_psubmenuEventSink = ISubmenuEventSink::CreateDelegate(this);
		
		//AaronMoore 1/10
        m_keyboardDelegate = IKeyboardInput::CreateDelegate(this);
        GetEngineWindow()->AddKeyboardInputFilter(m_keyboardDelegate);
    }

    ~LoadoutImpl()
    {
        CloseMenu();

        trekClient.SaveLoadout(m_pship);
        m_pship->Terminate();
        m_pship->Release();

        m_pmodeler->UnloadNameSpace("defaultloadout");
        m_pmodeler->UnloadNameSpace("loadoutpane");
        m_pmodeler->UnloadNameSpace("loadout");
        m_pmodeler->UnloadNameSpace("loadoutdata");
        m_pmodeler->UnloadNameSpace("loadoutinclude");
		
		//AaronMoore 1/10
        GetEngineWindow()->RemoveKeyboardInputFilter(m_keyboardDelegate);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Initialization
    //
    //////////////////////////////////////////////////////////////////////////////

    void InitializeMDL()
    {
        //
        // exports
        //

        TRef<INameSpace> pnsLoadoutData = m_pmodeler->CreateNameSpace("loadoutdata", m_pmodeler->GetNameSpace("effect"));

        for (int index = 0; index < c_maxMountedWeapons; index++) {
            pnsLoadoutData->AddMember(
                "gunner" + ZString(index),  
                m_pstringGunners[index] = new ModifiableString("unmanned")
            );
        }
        
        pnsLoadoutData->AddMember("titleString",           m_pstringTitle           = new ModifiableString(ZString()));
        pnsLoadoutData->AddMember("hullDescriptionString", m_pstringHullDescription = new ModifiableString(ZString()));
        pnsLoadoutData->AddMember("partNameString",        m_pstringPartName        = new ModifiableString(ZString()));
        pnsLoadoutData->AddMember("partDescriptionString", m_pstringPartDescription = new ModifiableString(ZString()));
    
        pnsLoadoutData->AddMember("money",             m_pnumberMoney            = new ModifiableNumber((float)trekClient.GetMoney()));
        pnsLoadoutData->AddMember("oldShipCost",       m_pnumberOldShipCost      = new ModifiableNumber((float)trekClient.GetShip()->GetValue()));
        pnsLoadoutData->AddMember("newShipCost",       m_pnumberNewShipCost      = new ModifiableNumber((float)m_pship->GetValue()));
        pnsLoadoutData->AddMember("loadoutShipState",  m_pnumberLoadoutShipState = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("partID",            m_pnumberPartID           = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("partMask",          m_pnumberPartMask         = new ModifiableNumber(0));
        //pnsLoadoutData->AddMember("partEquipmentType", m_pnumberEquipmentType    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("partSignature",     m_pnumberSignature        = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("partMass",          m_pnumberMass             = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("partSignatureNumber",     m_pnumberSignatureNumber        = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("partMassNumber",          m_pnumberMassNumber             = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("weaponDamage",      m_pnumberWeaponDamage     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("weaponRate",        m_pnumberWeaponRate       = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("weaponRange",       m_pnumberWeaponRange      = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("weaponSpeed",       m_pnumberWeaponSpeed      = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("weaponDamageNumber",      m_pnumberWeaponDamageNumber     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("weaponRateNumber",        m_pnumberWeaponRateNumber       = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("weaponRangeNumber",       m_pnumberWeaponRangeNumber      = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("weaponSpeedNumber",       m_pnumberWeaponSpeedNumber      = new ModifiableNumber(0));
        
        pnsLoadoutData->AddMember("shieldRegen",       m_pnumberShieldRegen      = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("shieldHitPts",      m_pnumberShieldHitPts     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("shieldRegenNumber",       m_pnumberShieldRegenNumber      = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("shieldHitPtsNumber",    m_pnumberShieldHitPtsNumber     = new ModifiableNumber(0));
        
        pnsLoadoutData->AddMember("equipmentType",     m_pnumberEquipmentType    = new ModifiableNumber(0));
        
        pnsLoadoutData->AddMember("cloakCloaking",     m_pnumberCloakCloaking    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("cloakDuration",     m_pnumberCloakDuration    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("cloakCloakingNumber",     m_pnumberCloakCloakingNumber    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("cloakDurationNumber",     m_pnumberCloakDurationNumber    = new ModifiableNumber(0));
        
        pnsLoadoutData->AddMember("magazineDamage",      m_pnumberMagazineDamage     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("magazineRange",          m_pnumberMagazineRange         = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("magazineLockTime",       m_pnumberMagazineLockTime      = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("magazineMaxLock",        m_pnumberMagazineMaxLock       = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("magazineDamageNumber",      m_pnumberMagazineDamageNumber     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("magazineRangeNumber",             m_pnumberMagazineRangeNumber        = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("magazineLockTimeNumber",       m_pnumberMagazineLockTimeNumber        = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("magazineMaxLockNumber",       m_pnumberMagazineMaxLockNumber      = new ModifiableNumber(0));

        pnsLoadoutData->AddMember("afterburnerMaxSpeed",      m_pnumberAfterburnerMaxSpeed     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("afterburnerBurnRate",       m_pnumberAfterburnerBurnRate      = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("afterburnerMaxSpeedNumber",      m_pnumberAfterburnerMaxSpeedNumber   = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("afterburnerBurnRateNumber",     m_pnumberAfterburnerBurnRateNumber    = new ModifiableNumber(0));
        
        pnsLoadoutData->AddMember("chaffStrength",          m_pnumberChaffStrength    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("chaffStrengthNumber",    m_pnumberChaffStrengthNumber  = new ModifiableNumber(0));

        pnsLoadoutData->AddMember("packQuantityNumber",    m_pnumberPackQuantityNumber   = new ModifiableNumber(0));
        
        pnsLoadoutData->AddMember("probeScanRange",     m_pnumberProbeScanRange     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("probeVisibility",    m_pnumberProbeVisibility    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("probeHitPts",        m_pnumberProbeHitPts        = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("probeLifespan",      m_pnumberProbeLifespan      = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("probeScanRangeNumber",    m_pnumberProbeScanRangeNumber   = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("probeVisibilityNumber",    m_pnumberProbeVisibilityNumber   = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("probeHitPtsNumber",      m_pnumberProbeHitPtsNumber      = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("probeLifespanNumber",        m_pnumberProbeLifespanNumber        = new ModifiableNumber(0));
        
        pnsLoadoutData->AddMember("mineVisibility",    m_pnumberMineVisibility    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("mineVisibilityNumber",    m_pnumberMineVisibilityNumber    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("mineDamage",     m_pnumberMineDamage     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("mineRadius",     m_pnumberMineRadius     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("mineLifespan",   m_pnumberMineLifespan   = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("mineDamageNumber",     m_pnumberMineDamageNumber     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("mineRadiusNumber",     m_pnumberMineRadiusNumber     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("mineLifespanNumber",   m_pnumberMineLifespanNumber   = new ModifiableNumber(0));

        pnsLoadoutData->AddMember("hullMaxSpeed",      m_pnumberHullMaxSpeed     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullAcceleration",  m_pnumberHullAcceleration = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullTurnRate",      m_pnumberHullTurnRate     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullHitPoints",     m_pnumberHullHitPoints    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullMaxEnergy",     m_pnumberHullEnergy       = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullScanRange",     m_pnumberHullScanRange    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullSignature",     m_pnumberHullSignature    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullMass",          m_pnumberHullMass         = new ModifiableNumber(0));

        pnsLoadoutData->AddMember("hullMaxSpeedNumber",      m_pnumberHullMaxSpeedNumber     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullAccelerationNumber",  m_pnumberHullAccelerationNumber = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullTurnRateNumber",      m_pnumberHullTurnRateNumber     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullHitPointsNumber",     m_pnumberHullHitPointsNumber    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullMaxEnergyNumber",     m_pnumberHullEnergyNumber       = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullScanRangeNumber",     m_pnumberHullScanRangeNumber    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullSignatureNumber",     m_pnumberHullSignatureNumber    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("hullMassNumber",          m_pnumberHullMassNumber         = new ModifiableNumber(0));

        pnsLoadoutData->AddMember("baseHullMaxSpeed",      m_pnumberBaseHullMaxSpeed     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullAcceleration",  m_pnumberBaseHullAcceleration = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullTurnRate",      m_pnumberBaseHullTurnRate     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullHitPoints",     m_pnumberBaseHullHitPoints    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullMaxEnergy",     m_pnumberBaseHullEnergy       = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullScanRange",     m_pnumberBaseHullScanRange    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullSignature",     m_pnumberBaseHullSignature    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullMass",          m_pnumberBaseHullMass         = new ModifiableNumber(0));

        pnsLoadoutData->AddMember("baseHullMaxSpeedNumber",      m_pnumberBaseHullMaxSpeedNumber     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullAccelerationNumber",  m_pnumberBaseHullAccelerationNumber = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullTurnRateNumber",      m_pnumberBaseHullTurnRateNumber     = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullHitPointsNumber",     m_pnumberBaseHullHitPointsNumber    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullMaxEnergyNumber",     m_pnumberBaseHullEnergyNumber       = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullScanRangeNumber",     m_pnumberBaseHullScanRangeNumber    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullSignatureNumber",     m_pnumberBaseHullSignatureNumber    = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullMassNumber",          m_pnumberBaseHullMassNumber         = new ModifiableNumber(0));
        
        pnsLoadoutData->AddMember("partImage",                  (Value*)(m_pwrapImagePart = new WrapImage(Image::GetEmpty())));
        pnsLoadoutData->AddMember("civImage",                   (Value*)(m_pwrapImageCiv  = new WrapImage(Image::GetEmpty())));
        pnsLoadoutData->AddMember("civHullImage",               (Value*)(m_pwrapImageHullCiv  = new WrapImage(Image::GetEmpty())));

        pnsLoadoutData->AddMember("hullDefenseClassNumber",     m_pnumberHullDefenseClass       = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("baseHullDefenseClassNumber", m_pnumberBaseHullDefenseClass   = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("shieldDefenseClassNumber", m_pnumberShieldDefenseClass   = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("weaponDamageClassNumber",    m_pnumberWeaponDamageBonus      = new ModifiableNumber(0));
        pnsLoadoutData->AddMember("magazineDamageClassNumber",  m_pnumberMagazineDamageBonus    = new ModifiableNumber(0));

        //
        // imports
        //

        TRef<INameSpace> pnsPartInfo = m_pmodeler->GetNameSpace("partinfo");

        m_maxWeaponRate     = pnsPartInfo->FindNumber("maxWeaponRate");
        m_maxWeaponPower    = pnsPartInfo->FindNumber("maxWeaponPower");
        m_maxWeaponRange    = pnsPartInfo->FindNumber("maxWeaponRange");
        m_maxWeaponSpeed    = pnsPartInfo->FindNumber("maxWeaponSpeed");
        m_maxShieldRegen    = pnsPartInfo->FindNumber("maxShieldRegen");
        m_maxShieldHitPts   = pnsPartInfo->FindNumber("maxShieldHitPts");
        m_maxCloakEnergy    = pnsPartInfo->FindNumber("maxCloakEnergy");
        m_maxPartMass       = pnsPartInfo->FindNumber("maxPartMass");
        m_maxPartSignature  = pnsPartInfo->FindNumber("maxPartSignature");

        m_maxMagazineHitDamage  = pnsPartInfo->FindNumber("maxMagazineHitDamage");
        m_maxMagazineAreaDamage = pnsPartInfo->FindNumber("maxMagazineAreaDamage");
        m_maxMagazineRange      = pnsPartInfo->FindNumber("maxMagazineRange");
        m_maxMagazineLockTime   = pnsPartInfo->FindNumber("maxMagazineLockTime");
        m_maxMagazineMaxLock    = pnsPartInfo->FindNumber("maxMagazineMaxLock");

        m_maxAfterburnerMaxThrust = pnsPartInfo->FindNumber("maxAfterburnerMaxThrust");
        m_maxAfterburnerBurnRate  = pnsPartInfo->FindNumber("maxAfterburnerBurnRate");
        m_maxAfterburnerOnRate    = pnsPartInfo->FindNumber("maxAfterburnerOnRate");
        
        m_maxChaffStrength        = pnsPartInfo->FindNumber("maxChaffStrength");

        m_maxProbeScanRange       = pnsPartInfo->FindNumber("maxProbeScanRange");
        m_maxProbeHitPts          = pnsPartInfo->FindNumber("maxProbeHitPts");
        m_maxProbeLifespan        = pnsPartInfo->FindNumber("maxProbeLifespan");

        m_maxMineDamage             = pnsPartInfo->FindNumber("maxMineDamage");
        m_maxMineRadius             = pnsPartInfo->FindNumber("maxMineRadius");
        m_maxMineLifespan       = pnsPartInfo->FindNumber("maxMineLifespan");

        m_pmodeler->UnloadNameSpace("partinfo");

        TRef<INameSpace> pnsHullInfo = m_pmodeler->GetNameSpace("hullinfo");

        m_maxMaxSpeed      = pnsHullInfo->FindNumber("maxHullMaxSpeed"    );
        m_maxAcceleration  = pnsHullInfo->FindNumber("maxHullAcceleration");
        m_maxTurnRate      = pnsHullInfo->FindNumber("maxHullTurnRate"    );
        m_maxHitPoints     = pnsHullInfo->FindNumber("maxHullHitPoints"   );
        m_maxEnergy        = pnsHullInfo->FindNumber("maxHullMaxEnergy"   );
        m_maxScanRange     = pnsHullInfo->FindNumber("maxHullScanRange"   );
        m_maxHullSignature = pnsHullInfo->FindNumber("maxHullSignature"   );
        m_maxHullMass      = pnsHullInfo->FindNumber("maxHullMass"        );

        m_pmodeler->UnloadNameSpace("hullinfo");

        //
        // we use these namespaces
        //

        m_pnsLoadout        = m_pmodeler->GetNameSpace("loadout");
        m_pnsLoadoutPane    = m_pmodeler->GetNameSpace("loadoutpane");
        m_pnsDefaultLoadout = m_pmodeler->GetNameSpace("defaultloadout");

    }

    void InitializePanes()
    {
        //
        // Get the pane
        //

        CastTo(m_ppane, m_pnsLoadoutPane->FindMember("loadoutPane"));

        //
        // Add the Geo Pane
        //

        m_ppaneGeo = CreateGeoPane(
            m_pnsLoadoutPane->FindPoint("geoSize"),
            m_pnsLoadoutPane->FindPoint("geoPosition")
        );
        m_ppaneGeo->SetOffset(m_pnsLoadoutPane->FindWinPoint("geoPosition"));
        m_ppane->InsertAtTop(m_ppaneGeo);
        
        
        //
        // The Buttons
        //

        CastTo(m_pbuttonDefault , m_pnsLoadoutPane->FindMember("defaultButtonPane" ));
        CastTo(m_pbuttonNext    , m_pnsLoadoutPane->FindMember("nextButtonPane"    ));
        CastTo(m_pbuttonPrevious, m_pnsLoadoutPane->FindMember("previousButtonPane"));
        CastTo(m_pbuttonLaunch  , m_pnsLoadoutPane->FindMember("launchButtonPane"  ));
        CastTo(m_pbuttonBuy     , m_pnsLoadoutPane->FindMember("buyButtonPane"     ));
        CastTo(m_pbuttonConfirm , m_pnsLoadoutPane->FindMember("confirmButtonPane" ));
        CastTo(m_pbuttonBack    , m_pnsLoadoutPane->FindMember("backButtonPane"    ));
        CastTo(m_pbuttonHullList, m_pnsLoadoutPane->FindMember("hullListButtonPane"));

        CastTo(m_ppaneHoverBuy    , m_pnsLoadoutPane->FindMember("buyHoverPane"    ));
        CastTo(m_ppaneHoverConfirm, m_pnsLoadoutPane->FindMember("confirmHoverPane"));
        CastTo(m_ppointBuyButton  , m_pnsLoadoutPane->FindMember("buyBtnPosition"  ));

        m_pbuttonHullList->SetDownTrigger(true);

		// mdvalley: A pointer and some class.
        AddEventTarget(&LoadoutImpl::OnButtonDefault , m_pbuttonDefault ->GetEventSource());
        AddEventTarget(&LoadoutImpl::OnButtonNext    , m_pbuttonNext    ->GetEventSource());
        AddEventTarget(&LoadoutImpl::OnButtonPrevious, m_pbuttonPrevious->GetEventSource());
        AddEventTarget(&LoadoutImpl::OnButtonLaunch  , m_pbuttonLaunch  ->GetEventSource());
        AddEventTarget(&LoadoutImpl::OnButtonBuy     , m_pbuttonBuy     ->GetEventSource());
        AddEventTarget(&LoadoutImpl::OnButtonBuy     , m_pbuttonConfirm ->GetEventSource());
        AddEventTarget(&LoadoutImpl::OnButtonBack    , m_pbuttonBack    ->GetEventSource());
        AddEventTarget(&LoadoutImpl::OnButtonHullList, m_pbuttonHullList->GetEventSource());
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Screen public methods
    //
    //////////////////////////////////////////////////////////////////////////////

    Pane* GetPane()
    {
        return m_ppane;
    }

    void OnFrame()
    {
        UpdateButtons();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Methods
    //
    //////////////////////////////////////////////////////////////////////////////
    
    void UpdateCivBitmap()
    {
        IsideIGC* pside = trekClient.GetSide();

        if (pside) {
            ZString str = ZString(pside->GetCivilization()->GetIconName()).ToLower() + "loadoutpanebmp";
            m_pwrapImageCiv->SetImage(GetModeler()->LoadImage(str, false));
            ZString str2 = ZString(pside->GetCivilization()->GetIconName()).ToLower() + "hullinfobmp";
            m_pwrapImageHullCiv->SetImage(GetModeler()->LoadImage(str2, false));
        } else {
            m_pwrapImageCiv->SetImage(GetModeler()->LoadImage("loadoutpanemp", false));
            m_pwrapImageHullCiv->SetImage(GetModeler()->LoadImage("hullinfomp", false));
        }
    }

    TRef<Pane> CreateGeoPane(const Point& size, const Point& offset)
    {
        m_pcamera = new Camera();
        m_pcamera->SetZClip(1, 10000);
        m_pcamera->SetFOV(RadiansFromDegrees(50));

        Rect rect(Point(0, 0), size);
        TRef<RectValue> prect = new RectValue(rect);

        m_pviewport = new Viewport(m_pcamera, prect);

        m_pwrapGeo  = new WrapGeo(Geo::GetEmpty());

        TRef<Image> pimage =
            new GroupImage(
                m_pwrapImageGeo = new WrapImage(Image::GetEmpty()),
                new GeoImage(                
                    new TransformGeo(
                        new TransformGeo(
                            m_pwrapGeo,
                            new AnimateRotateTransform(
                                new VectorValue(Vector(0, 1, 0)),
                                NumberTransform::Multiply(m_ptime, new Number(1.0))
                            )
                        ),
                        new RotateTransform(Vector(1, 0, 0), pi/8)
                    ),
                    m_pviewport,
                    true
                ),
                new TranslateImage(
                    m_pwrapImageCiv,
                    Point(-offset.X(), -(600 - size.Y() - offset.Y()))    
                )
            );

        //
        // Give this guy a zbuffer
        //

        return
            new AnimatedImagePane(
                CreatePaneImage( 
                    GetEngine(),
                    false,
                    new AnimatedImagePane(
                        pimage,
                        rect
                    )
                )
            );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetGeo(Geo* pgeo)
    {
        m_pwrapGeo->SetGeo(pgeo);

        float radius = pgeo->GetRadius(Matrix::GetIdentity());

        //m_scaleCone = 0.75f * radius;

        m_pcamera->SetPosition(
            Vector(
                0,
                0,
                2.4f * radius
            )
        );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // m_pship->SetBaseHullType(phullType);
    //
    //////////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////////
    //
    // 
    //
    //////////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////////
    //
    // Gunner Info
    //
    //////////////////////////////////////////////////////////////////////////////

    void UpdateGunnerInfo() 
    {
        for (int index = 0; index < c_maxMountedWeapons; index++) {
            IweaponIGC* pweapon;
            CastTo(pweapon, m_pship->GetMountedPart(ET_Weapon, index));

            if (pweapon) {
                IshipIGC* pshipGunner = pweapon->GetGunner();

                if (pshipGunner) {
                    m_pstringGunners[index]->SetValue(pshipGunner->GetName());
                } else {
                    m_pstringGunners[index]->SetValue("unmanned");
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Hull Info
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetHullTitle()
    {
        m_pstringTitle->SetValue(ZString(m_phullType->GetName()).ToUpper());
        m_pstringHullDescription->SetValue(m_phullType->GetDescription());
    }

    void UpdatePerformanceInfo()
    {
        SetHullTitle();

        float acceleration = m_phullType->GetThrust() / m_pship->GetMass();
        float turnRate =
              (
                  m_phullType->GetMaxTurnRate(c_axisYaw)
                + m_phullType->GetMaxTurnRate(c_axisPitch)
                + m_phullType->GetMaxTurnRate(c_axisRoll)
              )
              * (180.0f / (3.0f * pi));

        float accelerationBase = m_phullTypeBase->GetThrust() / m_fSpecMass;
        float turnRateBase =
              (
                  m_phullTypeBase->GetMaxTurnRate(c_axisYaw)
                + m_phullTypeBase->GetMaxTurnRate(c_axisPitch)
                + m_phullTypeBase->GetMaxTurnRate(c_axisRoll)
              )
            * (180.0f / (3.0f * pi));

        const GlobalAttributeSet&   ga = trekClient.GetSide()->GetGlobalAttributeSet();

        float   maxSpeed = m_phullType->GetMaxSpeed();
        float   hitPoints = m_phullType->GetHitPoints();
        float   maxEnergy = m_phullType->GetMaxEnergy();
        float   scannerRange = m_phullType->GetScannerRange();
        float   signature = m_pship->GetSignature() / ga.GetAttribute(c_gaSignature);
        float   specSignature = m_fSpecSignature / ga.GetAttribute(c_gaSignature);

        m_pnumberHullMaxSpeed    ->SetValue(maxSpeed     / m_maxMaxSpeed     );
        m_pnumberHullAcceleration->SetValue(acceleration                   / m_maxAcceleration );
        m_pnumberHullTurnRate    ->SetValue(turnRate                       / m_maxTurnRate     );
        m_pnumberHullHitPoints   ->SetValue(hitPoints    / m_maxHitPoints    );
        m_pnumberHullEnergy      ->SetValue(maxEnergy    / m_maxEnergy       );
        m_pnumberHullScanRange   ->SetValue(scannerRange / m_maxScanRange    );
        m_pnumberHullSignature   ->SetValue((1/signature)    / m_maxHullSignature);
        m_pnumberHullMass        ->SetValue(m_pship->GetMass()             / m_maxHullMass     );

        m_pnumberHullMaxSpeedNumber    ->SetValue(maxSpeed);
        m_pnumberHullAccelerationNumber->SetValue(acceleration);
        m_pnumberHullTurnRateNumber    ->SetValue(turnRate);
        m_pnumberHullHitPointsNumber   ->SetValue(hitPoints);
        m_pnumberHullEnergyNumber      ->SetValue(maxEnergy);
        m_pnumberHullScanRangeNumber   ->SetValue(scannerRange);
        m_pnumberHullSignatureNumber   ->SetValue(signature * 100);
        m_pnumberHullMassNumber        ->SetValue(m_pship->GetMass());

        m_pnumberBaseHullMaxSpeed    ->SetValue(m_phullTypeBase->GetMaxSpeed() / m_maxMaxSpeed);
        m_pnumberBaseHullAcceleration->SetValue(accelerationBase / m_maxAcceleration);
        m_pnumberBaseHullTurnRate    ->SetValue(turnRateBase / m_maxTurnRate);
        m_pnumberBaseHullHitPoints   ->SetValue(m_phullTypeBase->GetHitPoints() / m_maxHitPoints);
        m_pnumberBaseHullEnergy      ->SetValue(m_phullTypeBase->GetMaxEnergy() / m_maxEnergy);
        m_pnumberBaseHullScanRange   ->SetValue(m_phullTypeBase->GetScannerRange() / m_maxScanRange);
        m_pnumberBaseHullSignature   ->SetValue((1/specSignature) / m_maxHullSignature);
        m_pnumberBaseHullMass        ->SetValue(m_fSpecMass / m_maxHullMass);

        m_pnumberBaseHullMaxSpeedNumber    ->SetValue(m_phullTypeBase->GetMaxSpeed());
        m_pnumberBaseHullAccelerationNumber->SetValue(accelerationBase);
        m_pnumberBaseHullTurnRateNumber    ->SetValue(turnRateBase);
        m_pnumberBaseHullHitPointsNumber   ->SetValue(m_phullTypeBase->GetHitPoints());
        m_pnumberBaseHullEnergyNumber      ->SetValue(m_phullTypeBase->GetMaxEnergy());
        m_pnumberBaseHullScanRangeNumber   ->SetValue(m_phullTypeBase->GetScannerRange());
        m_pnumberBaseHullSignatureNumber   ->SetValue(specSignature * 100);
        m_pnumberBaseHullMassNumber        ->SetValue(m_fSpecMass);

        m_pnumberHullDefenseClass       ->SetValue(m_phullType->GetDefenseType());
        m_pnumberBaseHullDefenseClass   ->SetValue(m_phullTypeBase->GetDefenseType());


    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Part Info
    //
    //////////////////////////////////////////////////////////////////////////////

    void UpdateWeaponPartInfo(IpartTypeIGC* ppartType)
    {
        const GlobalAttributeSet&   ga = trekClient.GetSide()->GetGlobalAttributeSet();

        DataWeaponTypeIGC*  pdwt = (DataWeaponTypeIGC*)ppartType->GetData();
        IprojectileTypeIGC* ppt  = trekClient.GetCore()->GetProjectileType(pdwt->projectileTypeID);

        float   rate  = 1.0f / pdwt->dtimeBurst;
        float   power = (ppt->GetPower() + ppt->GetBlastPower()) * rate * ga.GetAttribute(c_gaDamageGuns);
        bool    bAmmo = pdwt->cAmmoPerShot != 0;
        float   speed = ppt->GetSpeed();
        if (bAmmo)
            speed *= ga.GetAttribute(c_gaSpeedAmmo);

        float   range = speed * ppt->GetLifespan();
        if (!bAmmo)
            range *= ga.GetAttribute(c_gaLifespanEnergy);

        m_pnumberWeaponDamage->SetValue(power / m_maxWeaponPower);
        m_pnumberWeaponRange ->SetValue(range / m_maxWeaponRange);
        m_pnumberWeaponSpeed ->SetValue(speed / m_maxWeaponSpeed);
        m_pnumberWeaponRate  ->SetValue(rate  / m_maxWeaponRate);

        m_pnumberWeaponDamageNumber->SetValue(power);
        m_pnumberWeaponRangeNumber ->SetValue(range);               // !!! should get a 'm' suffix
        m_pnumberWeaponSpeedNumber ->SetValue(speed);               // !!! should get a 'mps' suffix
        m_pnumberWeaponRateNumber  ->SetValue(rate);                // !!! should get a '/s' suffix

        m_pnumberWeaponDamageBonus ->SetValue(ppt->GetDamageType());

    }

    void UpdateMagazinePartInfo(IpartTypeIGC* ppartType)
    {
        const GlobalAttributeSet&   ga = trekClient.GetSide()->GetGlobalAttributeSet();

        ImissileTypeIGC* pmt = (ImissileTypeIGC*)((IlauncherTypeIGC*)ppartType)->GetExpendableType();
        
        float power    = (pmt->GetPower() + pmt->GetBlastPower()) * ga.GetAttribute(c_gaDamageMissiles);
        float lifespan = pmt->GetLifespan();
        float range    = pmt->GetInitialSpeed() * lifespan + 0.5f * (pmt->GetAcceleration() * lifespan * lifespan);
        float lockTime = pmt->GetLockTime();
        float lock     = pmt->GetMaxLock();

        m_pnumberMagazineDamage  ->SetValue(power / (m_maxMagazineHitDamage + m_maxMagazineAreaDamage));
        m_pnumberMagazineRange   ->SetValue(range / m_maxMagazineRange);
        m_pnumberMagazineLockTime->SetValue(lockTime / m_maxMagazineLockTime);
        m_pnumberMagazineMaxLock ->SetValue(lock);

        m_pnumberMagazineDamageNumber  ->SetValue(power);
        m_pnumberMagazineRangeNumber   ->SetValue(range);               // !!! Should get a 'm' suffix
        m_pnumberMagazineLockTimeNumber->SetValue(lockTime);            // !!! Should get a 's' suffix
        m_pnumberMagazineMaxLockNumber ->SetValue(lock * 100.0f);       // !!! Should get a '%' suffix

        m_pnumberMagazineDamageBonus ->SetValue(pmt->GetDamageType());
    }

    void UpdateDispenserPartInfo(IpartTypeIGC* ppartType)
    {
        const GlobalAttributeSet&   ga = trekClient.GetSide()->GetGlobalAttributeSet();

        IexpendableTypeIGC*     pet = ((IlauncherTypeIGC*)ppartType)->GetExpendableType();

        if (pet->GetObjectType() == OT_probeType)
        {
            DataProbeTypeIGC* pdprt = (DataProbeTypeIGC*)(pet->GetData());
    
            float   scanRange;
            {
                IprojectileTypeIGC* ppt = ((IprobeTypeIGC*)pet)->GetProjectileType();
                if (ppt)
                {
                    //Scan range for something with guns is the gun range
                    scanRange = ppt->GetLifespan() * ppt->GetSpeed() / 1.2f;
                    if (((IprobeTypeIGC*)pet)->GetAmmo() != 0)
                        scanRange *= trekClient.GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaSpeedAmmo);
                }
                else
                    scanRange = pdprt->scannerRange * ga.GetAttribute(c_gaScanRange);
            }
            float   signature = pdprt->signature / ga.GetAttribute(c_gaSignature);

            m_pnumberProbeScanRange         ->SetValue(scanRange / m_maxProbeScanRange);
            m_pnumberProbeVisibility        ->SetValue(signature);
            m_pnumberProbeHitPts            ->SetValue(pdprt->hitPoints / m_maxProbeHitPts);
            m_pnumberProbeLifespan          ->SetValue(pdprt->lifespan / m_maxProbeLifespan);
        
            m_pnumberProbeScanRangeNumber   ->SetValue(scanRange);        //Should get a 'm' suffix
            m_pnumberProbeVisibilityNumber  ->SetValue(signature * 100.0f);  //Should get a '%' suffix
            m_pnumberProbeHitPtsNumber      ->SetValue(pdprt->hitPoints);
            m_pnumberProbeLifespanNumber    ->SetValue(pdprt->lifespan);            //Should get a 's' suffix
        }
        else
        {
            DataMineTypeIGC* pdmt = (DataMineTypeIGC*)(pet->GetData());

            float   signature = pdmt->signature / ga.GetAttribute(c_gaSignature);
    
            m_pnumberMineDamage          ->SetValue(pdmt->power / m_maxMineDamage);
            m_pnumberMineVisibility      ->SetValue(signature);
            m_pnumberMineRadius          ->SetValue(pdmt->radius / m_maxMineRadius);
            m_pnumberMineLifespan        ->SetValue(pdmt->lifespan / m_maxMineLifespan);
        
            m_pnumberMineDamageNumber    ->SetValue(pdmt->power);
            m_pnumberMineVisibilityNumber->SetValue(signature * 100.0f);  //Should get a '%' suffix
            m_pnumberMineRadiusNumber    ->SetValue(pdmt->radius);              //Should get a 'm' suffix
            m_pnumberMineLifespanNumber  ->SetValue(pdmt->lifespan);            //Should get a 's' suffix
        }
    }

    void UpdateChaffLauncherPartInfo(IpartTypeIGC* ppartType)
    {
        DataChaffTypeIGC* pdct = (DataChaffTypeIGC*)
                                 ((((IlauncherTypeIGC*)ppartType)->GetExpendableType())->GetData());

        float effectiveness = (pdct->chaffStrength <= 1.0f) ?
                               pdct->chaffStrength / 2.0f : 1.0f - 1.0f/(2.0f * pdct->chaffStrength);

        m_pnumberChaffStrength ->SetValue(effectiveness / m_maxChaffStrength);
        m_pnumberChaffStrengthNumber->SetValue(effectiveness * 100);            //Should get a % suffix
    }

    void UpdateShieldPartInfo(IpartTypeIGC* ppartType)
    {
        const GlobalAttributeSet&   ga = trekClient.GetSide()->GetGlobalAttributeSet();

        DataShieldTypeIGC* pdst = (DataShieldTypeIGC*)ppartType->GetData();

        float maxStrength = pdst->maxStrength * ga.GetAttribute(c_gaMaxShieldShip);
        float regen = maxStrength / (pdst->rateRegen * ga.GetAttribute(c_gaShieldRegenerationShip));

        m_pnumberShieldRegen   ->SetValue(regen / m_maxShieldRegen);
        m_pnumberShieldHitPts  ->SetValue(maxStrength / m_maxShieldHitPts);

        m_pnumberShieldRegenNumber   ->SetValue(regen);       //Should have a 's' suffix
        m_pnumberShieldHitPtsNumber  ->SetValue(maxStrength);

        m_pnumberShieldDefenseClass     ->SetValue(pdst->defenseType);
    }

    void UpdateCloakPartInfo(IpartTypeIGC* ppartType)
    {
        DataCloakTypeIGC* pdct = (DataCloakTypeIGC*)ppartType->GetData();

        m_pnumberCloakCloaking  ->SetValue(pdct->maxCloaking);
        m_pnumberCloakCloakingNumber ->SetValue(pdct->maxCloaking * 100);       //Should get a '%' suffix

        const IhullTypeIGC* pht = m_pship->GetHullType();
        float recharge  = pht->GetRechargeRate();
        float maxEnergy = pht->GetMaxEnergy();

        float duration = pdct->energyConsumption <= recharge ?
                         -1.0f : maxEnergy / (pdct->energyConsumption - recharge);

        float maxDuration = m_maxCloakEnergy <= recharge ?
                            -1.0 : maxEnergy / (m_maxCloakEnergy - recharge);

        m_pnumberCloakDuration  ->SetValue(duration/maxDuration);
        m_pnumberCloakDurationNumber ->SetValue(duration);                      //Should get a 's' suffix
    }

    void UpdatePackPartInfo(IpartIGC* ppart, IpartTypeIGC* ppartType)
    {
    }

    void UpdateAfterburnerPartInfo(IpartTypeIGC* ppartType)
    {
        DataAfterburnerTypeIGC* pdat = (DataAfterburnerTypeIGC*)ppartType->GetData();

        float   hullSpeed = m_phullType->GetMaxSpeed();
        float   hullThrust = m_phullType->GetThrust();
        float   hullThrust_noga = ((DataHullTypeIGC*)m_phullType->GetData())->thrust;

        float   maxSpeed         = hullSpeed * (1.0f + (pdat->maxThrust / hullThrust_noga)); //Imago 8/14/09
        float   absoluteMaxSpeed = hullSpeed * (1.0f + (m_maxAfterburnerMaxThrust / hullThrust_noga));

        float   endurance    = m_phullType->GetMaxFuel() / (pdat->maxThrust * pdat->fuelConsumption);
        
        m_pnumberAfterburnerMaxSpeed->SetValue(maxSpeed / absoluteMaxSpeed);
        m_pnumberAfterburnerBurnRate    ->SetValue(endurance    / m_maxAfterburnerBurnRate);
        
        m_pnumberAfterburnerMaxSpeedNumber->SetValue(maxSpeed);       //Should have a 'mps' suffix
        m_pnumberAfterburnerBurnRateNumber    ->SetValue(endurance);          //Should have a 's' suffix
    }

    void UpdatePartInfo(Slot* pslot, IpartIGC* ppart, IpartTypeIGC* ppartType)
    {
        if (ppartType) {
           
            m_pnumberPartID         ->SetValue(ppartType->GetObjectID()            );
            m_pnumberPartMask       ->SetValue(ppartType->GetPartMask()            );

            if (ppartType->GetEquipmentType() == ET_Dispenser)
            {
                if (((IlauncherTypeIGC*)ppartType)->GetExpendableType()->GetObjectType() == OT_probeType)
                    m_pnumberEquipmentType->SetValue(ppartType->GetEquipmentType());
                else
                    m_pnumberEquipmentType->SetValue(-4); // equipmentTypeMine
            }
            else
            {
                m_pnumberEquipmentType->SetValue(ppartType->GetEquipmentType());
            }

            m_pstringPartDescription->SetValue(ppartType->GetDescription()         );

            float   signature = ppartType->GetSignature() / trekClient.GetSide()->GetGlobalAttributeSet().GetAttribute(c_gaSignature);
            m_pnumberSignature      ->SetValue(signature / m_maxPartSignature);
            m_pnumberMass           ->SetValue(ppartType->GetMass() / m_maxPartMass);
            
            m_pnumberSignatureNumber      ->SetValue(signature * 100);
            m_pnumberMassNumber           ->SetValue(ppartType->GetMass());
            
            
            //m_pstringPartDescription->SetValue("Part desc");
            m_pstringPartName->SetValue(GetBuyableName(ppartType));
            
            ZString str = ZString("l") + ZString(ppartType->GetModelName()) + ZString("bmp").ToLower();
            TRef<Image> pimage = GetModeler()->LoadImage(str, false);
            m_pwrapImagePart->SetImage(pimage); 
            
        
            switch (ppartType->GetEquipmentType()) {
                case ET_Weapon:        UpdateWeaponPartInfo(ppartType);         break;
                case ET_Magazine:      UpdateMagazinePartInfo(ppartType);       break;
                case ET_ChaffLauncher: UpdateChaffLauncherPartInfo(ppartType);  break;
                case ET_Dispenser:     UpdateDispenserPartInfo(ppartType);      break;
                case ET_Shield:        UpdateShieldPartInfo(ppartType);         break;
                case ET_Cloak:         UpdateCloakPartInfo(ppartType);          break;
                case ET_Pack:          UpdatePackPartInfo(ppart, ppartType);    break;
                case ET_Afterburner:   UpdateAfterburnerPartInfo(ppartType);    break;
            }
        } else {
            m_pnumberEquipmentType->SetValue(-3); // equipmentTypeNone
            ZString str = ZString("ldefaultbmp");
            TRef<Image> pimage = GetModeler()->LoadImage(str, false);
            m_pwrapImagePart->SetImage(pimage);
            if (pslot) {
                EquipmentType equipmentType = pslot->GetEquipmentType();

                if (equipmentType == -1) {
                    m_pnumberPartID->SetValue(-1.0f);
                } else {
                    m_pnumberPartID->SetValue((float)(-(10 + pslot->GetEquipmentType())));
                }
            } else {
                m_pnumberPartID->SetValue(0);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Cone of fire
    //
    //////////////////////////////////////////////////////////////////////////////
    /*
    void UpdateCone(Slot* pslot)
    {
        //m_pmaterialCone;

        if (
               pslot != NULL
            && pslot->GetEquipmentType() == ET_Weapon
        ) {
            Mount mount = pslot->GetMount();
            const HardpointData& hd = m_phullType->GetHardpointData(mount);

            if ((hd.bFixed != true) && (hd.minDot >= -0.75f)) {
                m_pvisibleGeoCone->SetVisible(true);


                m_pnumberAngleCone->SetValue(acos(hd.minDot));

                const Vector&      position = m_phullType->GetWeaponPosition(mount) * (1 / m_phullType->GetScale());
                const Orientation& orient   = m_phullType->GetWeaponOrientation(mount);

                m_ptransformCone->SetValue(
                    Matrix(
                        Orientation(orient.GetBackward()),
                        position,
                        m_scaleCone
                    )
                );
                return;
            }
        }

        m_pvisibleGeoCone->SetVisible(false);
    }
    */
    //////////////////////////////////////////////////////////////////////////////
    //
    // Slot callbacks
    //
    //////////////////////////////////////////////////////////////////////////////

    void SlotEntered(Slot* pslot)
    {
        IpartIGC* ppart = pslot->GetPart();

        UpdatePartInfo(
            pslot, ppart,
            ppart ? ppart->GetPartType() : NULL
        );
        //UpdateCone(pslot);
    }

    void SlotLeft(Slot* pslot)
    {   
        //CloseMenu();
        UpdatePartInfo(NULL, NULL, NULL);
        //UpdateCone(NULL);
    }

    void SlotClicked(Slot* pslot, bool bNext, const WinPoint& point)
    {
        assert (trekClient.GetShip()->GetStation() != NULL);
        BuyPartMenu(pslot, point);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Slot finder
    //
    //////////////////////////////////////////////////////////////////////////////

    Slot* FindSlot(EquipmentType equipmentType, Mount mount)
    {
        SlotList::Iterator iter(m_listSlots);

        while (!iter.End()) {
            Slot* pslot = iter.Value();

            if ((pslot->GetMount() == mount) &&
                ((mount < 0) || (pslot->GetEquipmentType() == equipmentType)))
            {
                return pslot;
            }

            iter.Next();
        }

        return NULL;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Buy and sell parts
    //
    //////////////////////////////////////////////////////////////////////////////

    void OnMoneyChange(PlayerInfo* pPlayerInfo)
    {
        if (pPlayerInfo == trekClient.MyPlayerInfo()) {
            m_pnumberMoney->SetValue((float)trekClient.GetMoney());
        }
    }

    void SellPart(Slot* pslot)
    {
        IpartIGC* ppart = pslot->GetPart();
        if (ppart != NULL)
        {
            ppart->Terminate();
            pslot->SetPart(NULL);
        }

        UpdateHullType();
    }

    void BuyPart(Slot* pslot, IpartTypeIGC* ppartType)
    {
        SellPart(pslot);
        
        short   amount;
        if (IlauncherTypeIGC::IsLauncherType(ppartType->GetEquipmentType()))
            amount = ((IlauncherTypeIGC*)ppartType)->GetAmount(m_pship);
        else
            amount = 0x7fff;

        IpartIGC*   ppart = m_pship->CreateAndAddPart(ppartType, pslot->GetMount(), amount);
        assert (ppart);
        pslot->SetPart(ppart);

        UpdateHullType();
    }
	//AaronMoore 1/10
    void RefreshLoadout(CachedLoadoutList & loadoutList, IhullTypeIGC* phullType)
    {
        // remove all of the current parts
        {
            SlotList::Iterator iter(m_listSlots);

            while (!iter.End())
            {
                Slot* pslot = iter.Value();
                if (pslot->GetPart())
                {
                    pslot->GetPart()->Terminate();
                    pslot->SetPart(NULL);
                }

                iter.Next();
            }
            assert (m_pship->GetParts()->n() == 0);
        }
	//AaronMoore 1/10
        trekClient.ReplaceLoadout(m_pship, trekClient.GetShip()->GetStation(), phullType, moneyLots, loadoutList);
    }

    void BuyHull(IhullTypeIGC* phullType)
    { 
        trekClient.SaveLoadout(m_pship);
        
        // give it the default loadout for this hull
        RefreshLoadout(trekClient.m_customLoadouts[0], phullType); //AaronMoore 1/10 Imago #146 7/10

        // recalculate the mass and signature of the default loadout
        m_fSpecMass = m_pship->GetMass();
        m_fSpecSignature = m_pship->GetSignature();

        trekClient.PlaySoundEffect(mountSound);

        UpdateHullType();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // buttons
    //
    //////////////////////////////////////////////////////////////////////////////

    bool OnButtonLaunch()
    {
        if ((trekClient.GetShip()->GetValue() + trekClient.GetMoney()) >= m_pship->GetValue())
        {
            if (trekClient.GetShip()->GetParentShip() != NULL)
                trekClient.DisembarkAndBuy(m_pship, true);
            else
                trekClient.BuyLoadout(m_pship, true);
        }

        return true;
    }

    bool OnButtonBuy()
    {
        Money   cost = m_pship->GetValue() - (trekClient.GetShip()->GetValue() + trekClient.GetMoney());
        if (cost > 0)
        {
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmRequest, CS, REQUEST_MONEY)
            END_PFM_CREATE
            pfmRequest->amount = cost;
            pfmRequest->hidFor = m_pship->GetBaseHullType()->GetObjectID();
        }
        else if (trekClient.GetShip()->GetParentShip() != NULL) {
            trekClient.DisembarkAndBuy(m_pship, false);
			trekClient.SaveCustomLoadout(m_pship, 0); //#158 Imago 7/10                
		} else {
            trekClient.BuyLoadout(m_pship, false);
			trekClient.SaveCustomLoadout(m_pship, 0); //#158 Imago 7/10 ...lazy
		}

        return true;
    }

    bool OnButtonBack()
    {
        //GetWindow()->ToggleOverlayFlags(ofLoadout);
        GetWindow()->SetViewMode(TrekWindow::vmHangar);
        return true;
    }

    bool OnButtonHullList()
    {
        assert(trekClient.GetShip()->GetStation() != NULL);
        HullMenu();

        return true;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Button Events
    //
    //////////////////////////////////////////////////////////////////////////////

    void OnTechTreeChanged(SideID sideID)
    {
        if (trekClient.GetSideID() == sideID)
            UpdateNextAndPrevButtons();
    }

    void UpdateNextAndPrevButtons()
    {
        bool bEnableNextAndPrev = (GetNextHullType(true) != m_pship->GetBaseHullType());

        m_pbuttonNext->SetEnabled(bEnableNextAndPrev);
        m_pbuttonPrevious->SetEnabled(bEnableNextAndPrev);
    }

    IhullTypeIGC* GetNextHullType(bool bForward)
    {
        IhullTypeIGC* phtCurrent = m_pship->GetBaseHullType();
        IstationIGC* pstation = trekClient.GetShip()->GetStation();
        if (pstation != NULL)
        {
            const HullTypeListIGC* phullList         = trekClient.m_pCoreIGC->GetHullTypes();

            //
            // Seach for the next buyable hulltype
            //

            HullTypeLinkIGC*    phtlStart = phullList->find(phtCurrent);
            assert (phtlStart);
            HullTypeLinkIGC*    phtlNow = phtlStart;

            while (true)
            {
                assert (phtlNow);
                if (bForward)
                {
                    phtlNow = phtlNow->next();
                    if (phtlNow == NULL)
                        phtlNow = phullList->first();
                }
                else
                {
                    phtlNow = phtlNow->txen();
                    if (phtlNow == NULL)
                        phtlNow = phullList->last();
                }

                IhullTypeIGC*   pht = phtlNow->data();

                // if we can buy this hull or if we've come full circle
                if ((pht == phtCurrent)
                    || (pht->GetGroupID() >= 0)
                        && pstation->CanBuy(pht) 
                        && (!pstation->IsObsolete(pht)))
                {
                    return pht;
                }
            }
        }
        else
        {
            return phtCurrent;
        }
    }

    void NextHull(bool bForward)
    {
        IhullTypeIGC* pht = GetNextHullType(bForward);
        IhullTypeIGC* phtCurrent = m_pship->GetBaseHullType();

        if (pht != phtCurrent)
            BuyHull(pht);
    }

    bool OnButtonNext()
    {
        NextHull(true);
        return true;
    }

    bool OnButtonPrevious()
    {
        NextHull(false);
        return true;
    }

    bool OnButtonDefault()
    {
        Money budget = moneyLots;
        trekClient.BuyDefaultLoadout (m_pship, trekClient.GetShip()->GetStation(), m_phullTypeBase, &budget);
		trekClient.SaveCustomLoadout(m_pship, 0); //#158 Imago 7/10

        UpdateHullType();

        return true;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    bool CanMount(IpartTypeIGC* pparttype)
    {
        if (pparttype->GetEquipmentType() == ET_Pack)
        {
            return true;
        }
        else if (pparttype->GetEquipmentType() == ET_Weapon)
        {
            Mount mountMax = m_phullType->GetMaxWeapons();

            for (Mount mount = 0; mount < mountMax; mount++)
            {
                if (m_phullType->CanMount(pparttype, mount))
                    return true;
            }

            return false;
        }
        else
        {
            return m_phullType->CanMount(pparttype, 0);
        }
    }

    void BuyPartMenu(Slot* pslot, const WinPoint& pointLocal)
    {
        m_pslotMenu = pslot;

        if (m_phullMenu) {
            GetWindow()->GetPopupContainer()->ClosePopup(m_phullMenu);
            m_phullMenu = NULL;
            
        }
        m_pmenu =
            CreateMenu(
                GetModeler(),
                TrekResources::SmallFont(),
                IMenuCommandSink::CreateDelegate(this)
            );

        bool         bAnyItems = false;

        if (pslot->GetEquipmentType() == NA)
        {
            m_pmenu->AddMenuItem(c_midOtherParts, "Parts For Other Ships", 0, m_psubmenuEventSink);
            bAnyItems = true;
        }

        m_pmenu->AddMenuItem(-1, "-Make Slot Empty-");

        IstationIGC* pstation  = trekClient.GetShip()->GetStation();
        assert(pstation);

        const PartTypeListIGC* plist = trekClient.m_pCoreIGC->GetPartTypes();
        for (PartTypeLinkIGC* ppartTypeNode = plist->first();
             ppartTypeNode != NULL; 
             ppartTypeNode = ppartTypeNode->next())
        {
            IpartTypeIGC* ppartType     = ppartTypeNode->data();
            if ((ppartType->GetGroupID() >= 0) &&
                pstation->CanBuy(ppartType) &&
                !pstation->IsObsolete(ppartType))
            {
                EquipmentType equipmentType = ppartType->GetEquipmentType();

                if (pslot->GetEquipmentType() == NA)
                {
                    if (CanMount(ppartType))
                    {
                        bAnyItems = true;
                        m_pmenu->AddMenuItem(
                            ppartType->GetObjectID(),
                            GetBuyableName(ppartType)
                        );
                    }
                }
                else
                {
                    if ((equipmentType == pslot->GetEquipmentType()) &&
                        m_phullType->CanMount(ppartType, pslot->GetMount()))
                    {
                        bAnyItems = true;
                        m_pmenu->AddMenuItem(
                            ppartType->GetObjectID(),
                            GetBuyableName(ppartType)
                        );
                    }
                }
            }
        }

        if (bAnyItems) {
            Point popupPosition = GetEngineWindow()->GetMousePosition();
            GetWindow()->GetPopupContainer()->OpenPopup(m_pmenu, Rect(popupPosition, popupPosition), true, false);
        }
    }

    TRef<IPopup> GetSubMenu(IMenuItem* pitem)
    {
        assert(pitem->GetID() == c_midOtherParts);

        TRef<IMenu> pmenu =
            CreateMenu(
                GetModeler(),
                TrekResources::SmallFont(),
                IMenuCommandSink::CreateDelegate(this)
            );

        // create a menu with every part we can't mount

        IstationIGC* pstation  = trekClient.GetShip()->GetStation();
        const PartTypeListIGC* plist = trekClient.m_pCoreIGC->GetPartTypes();
        for (PartTypeLinkIGC* ppartTypeNode = plist->first();
             ppartTypeNode != NULL; 
             ppartTypeNode = ppartTypeNode->next())
        {
            IpartTypeIGC* ppartType     = ppartTypeNode->data();
            if ((ppartType->GetGroupID() >= 0) &&
                pstation->CanBuy(ppartType) &&
                !pstation->IsObsolete(ppartType))
            {
                if (!CanMount(ppartType))
                {
                    pmenu->AddMenuItem(
                        ppartType->GetObjectID(),
                        GetBuyableName(ppartType)
                    );
                }
            }
        }

        return pmenu;
    }

    void HullMenu(void)
    {
        IhullTypeIGC* phullTypeCurrent = m_pship->GetBaseHullType();
        IstationIGC* pstation = trekClient.GetShip()->GetStation();

        if (m_pmenu) {
            GetWindow()->GetPopupContainer()->ClosePopup(m_pmenu);
            m_pmenu     = NULL;
            m_pslotMenu = NULL;
        }
        m_phullMenu = 
            CreateMenu(
                GetModeler(),
				TrekResources::SmallFont(),
                IMenuCommandSink::CreateDelegate(this)
            );
        
        const HullTypeListIGC* phullList = trekClient.m_pCoreIGC->GetHullTypes();
        
        for (
            HullTypeLinkIGC* phullTypeNode = phullList->first();
            phullTypeNode != NULL; 
            phullTypeNode = phullTypeNode->next()
        ) {
            IhullTypeIGC* phullType = phullTypeNode->data();
            
            if ((phullType == phullTypeCurrent) ||
                ((phullType->GetGroupID() >= 0) &&
                 (pstation->CanBuy(phullType)) &&
                 (!pstation->IsObsolete(phullType))))
            {
                    m_phullMenu->AddMenuItem(
                        phullType->GetObjectID(),
                        GetBuyableName(phullType)
                    );
            }

#if 0
            {
                const GlobalAttributeSet&   ga = trekClient.GetSide()->GetGlobalAttributeSet();
                if (pstation->CanBuy(phullType))
                {
                    debugf("%s\n", phullType->GetName());
                    debugf("\tTop speed\t\t%8.2f mps\n", phullType->GetMaxSpeed() * ga.GetAttribute(c_gaMaxSpeed));
                    debugf("\tAcceleration\t%8.2f mpss\n", phullType->GetThrust() * ga.GetAttribute(c_gaThrust) /
                                                   phullType->GetMass());
                    debugf("\tTurn rate\t\t%8.2f degrees/second\n", 180.0f * phullType->GetMaxTurnRate(c_axisYaw) * ga.GetAttribute(c_gaTurnRate) / pi);

                    debugf("\tHit points\t\t%8.2f\n", phullType->GetHitPoints() * ga.GetAttribute(c_gaMaxArmorShip));
                    debugf("\tScan range\t\t%8.2f m\n", phullType->GetScannerRange() * ga.GetAttribute(c_gaScanRange));
                }
            }
#endif
        }

        Point popupPosition = GetEngineWindow()->GetMousePosition();
        GetWindow()->GetPopupContainer()->OpenPopup(m_phullMenu, Rect(popupPosition, popupPosition), true, false);
    }
            
    void CloseMenu()
    {
        if (m_pmenu) {
            GetWindow()->GetPopupContainer()->ClosePopup(m_pmenu);
            m_pmenu     = NULL;
            m_pslotMenu = NULL;
        }

        if (m_phullMenu) {
            GetWindow()->GetPopupContainer()->ClosePopup(m_phullMenu);
            m_phullMenu = NULL;
        }
    }

    void OnMenuCommand(IMenuItem* pitem)
    {
        //  , I don't know why this is getting called twice for the same
        //             menu

        if (m_pslotMenu != NULL) {
            SellPart(m_pslotMenu);
            IpartTypeIGC* ppartType =
                    trekClient.m_pCoreIGC->GetPartType(
                        pitem->GetID()
                    );
            if (pitem->GetID() != -1) {
                IpartTypeIGC* ppartType =
                    trekClient.m_pCoreIGC->GetPartType(
                        pitem->GetID()
                    );

                //
                // !!! make sure the part is available
                //

                if (ppartType) {
                    BuyPart(m_pslotMenu, ppartType);
                }
                trekClient.PlaySoundEffect(mountSound);
            } else {
                trekClient.PlaySoundEffect(unmountSound);
            }
            UpdatePartInfo(NULL, NULL, ppartType);
            CloseMenu();
            
        } else if (m_phullMenu !=NULL) {
            IhullTypeIGC* phullType = trekClient.m_pCoreIGC->GetHullType(pitem->GetID());
            if (phullType != m_pship->GetBaseHullType()) {
                BuyHull(phullType);
            }
            CloseMenu();
        } else {
            ZAssert(m_pmenu == NULL);
        }
    }

    void OnMenuSelect(IMenuItem* pitem)
    {
        if (m_pslotMenu != NULL) {
            if (pitem->GetID() == -1) {
                UpdatePartInfo(NULL, NULL, NULL);
            } else {
                IpartTypeIGC* ppartType =
                    trekClient.m_pCoreIGC->GetPartType(
                        pitem->GetID()
                    );

                UpdatePartInfo(NULL, NULL, ppartType);
            }
        }
    }

    void OnMenuClose(IMenu* pmenu)
    {
        UpdatePartInfo(NULL, NULL, NULL);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Update Hull Type
    //
    // This is called whenever something external has changed the hull,
    // the game starts, or when a player docks.
    //
    //////////////////////////////////////////////////////////////////////////////

    void HullSwitched()
    {
        //
        // Fix the hull info
        //

        UpdatePerformanceInfo();


        //
        // Remove the old slots
        //
        SlotList::Iterator iter(m_listSlots);

        while (!iter.End()) {
            Slot* pslot = iter.Value();
            pslot->RemoveSelf();
            iter.Remove();
        }

        //
        // Load the Geo
        //

        {
            TRef<INameSpace> pns = m_pmodeler->GetNameSpace(m_phullType->GetModelName());

            if (pns) {
                TRef<ThingGeo> pthing = ThingGeo::Create(m_pmodeler, m_ptime);
                pthing->LoadMDL(0, pns, NULL);
                pthing->SetShadeAlways(true);

                SetGeo(pthing->GetGeo());
            
            }
        }

        //
        // Update the geo image overlay
        //

        {
            ZString str =
                  ZString("loadout")
                + ZString(m_phullType->GetObjectID())
                + ZString("overlay");
            TRef<Image> pimage = m_pnsLoadout->FindImage(str);

            if (pimage != NULL) {
                m_pwrapImageGeo->SetImage(pimage);
            } else {
                m_pwrapImageGeo->SetImage(Image::GetEmpty());
            }
        }

        //
        // Load the slots
        //
        {
            //
            // Find the loadout definition
            //

            ZString str = ZString("loadout") + ZString(m_phullType->GetObjectID());
            TRef<IObjectList> plist = m_pnsLoadout->FindList(str);

            if (plist == NULL) {
                plist = m_pnsLoadout->FindList("loadoutDefault");
            }

            //
            // Parse the slot info
            //

            plist->GetFirst();
            while (plist->GetCurrent() != NULL) {
                TRef<IObjectPair> ppair; CastTo(ppair, plist->GetCurrent());

                EquipmentType     equipmentType = (EquipmentType)(int)GetNumber(ppair->GetNth(0));
                Mount             mount         =              (Mount)GetNumber(ppair->GetNth(1));
                TRef<StringValue> pstrPrefix    =     StringValue::Cast((Value*)ppair->GetNth(2));
                TRef<Image>       pimageDefault =           Image::Cast((Value*)ppair->GetNth(3));
                TRef<PointValue>  ppointOffset  =      PointValue::Cast(ppair->GetLastNth(4));
                TRef<RowPane>     pRowPane      =       new RowPane();
                //
                // Does the ship support the slot?
                //

                bool bMount = Slot::SlotValidForHull(m_phullType, equipmentType, mount);

                //
                // Add the slot if the ship supports it
                //

                if (bMount) {
                    TRef<Slot> pslot =
                        new Slot(
                            this,
                            equipmentType,
                            equipmentType == NA ? (mount - c_maxCargo) : mount,
                            pimageDefault,
                            pstrPrefix->GetValue()
                        );


                    pslot->SetOffset(WinPoint::Cast(ppointOffset->GetValue()));
                    m_ppane->InsertAtTop(pslot);
                    m_listSlots.PushEnd(pslot);

                }

                plist->GetNext();
            }
        }

        PartsSwitched();
    }

    void PartsSwitched()
    {
        //
        // Update all of the slots to the stuff that is actually on the ship
        //

        const PartListIGC* plist = m_pship->GetParts();
        for(
            const PartLinkIGC* ppartLink = plist->first(); 
            ppartLink != NULL; 
            ppartLink = ppartLink->next()
        ) {                   
            IpartIGC*     ppart         = ppartLink->data();
            IpartTypeIGC* ppartType     = ppart->GetPartType();
            EquipmentType equipmentType = ppart->GetEquipmentType();
            Mount         mount         = ppart->GetMountID();

            //
            // Find the slot
            //

            Slot* pslot = FindSlot(equipmentType, mount);
            if (pslot) {
                pslot->SetPart(ppart);
            }
        }
    }

    void UpdateHullType(void)
    {
        UpdateButtons();
    
        {
            //
            // Close the menu
            //
            //CloseMenu();

            //
            // Grab the new hull type
            //

            bool bNewHull = (m_phullTypeBase != m_pship->GetBaseHullType());

            m_phullType     = m_pship->GetHullType();
            m_phullTypeBase = m_pship->GetBaseHullType();

            if (m_phullTypeBase == NULL)
            {
                //
                // Don't have a hull the game must be exiting
                //

                return;

            }

            //
            // Update the slots
            //

            if (bNewHull)
                HullSwitched();
            else
            {
                //
                // Empty all of the slots
                //
                SlotList::Iterator iter(m_listSlots);

                while (!iter.End()) {
                    Slot* pslot = iter.Value();
                    pslot->SetPart(NULL);
                    iter.Next();
                }

                PartsSwitched();
                //
                // Update the performance info
                // 

                UpdatePerformanceInfo();
            }
        }
    }

    void UpdateButtons()
    {
        Money cost = m_pship->GetValue();
        Money shortfall;
        if (cost <= trekClient.GetMoney())
            shortfall = 0;
        else
            shortfall = cost - (trekClient.GetShip()->GetValue() + trekClient.GetMoney());

        bool bAlreadyOwn = m_pship->EquivalentShip(trekClient.GetShip());

        bool bCanAfford = shortfall <= 0;
        bool bCanBuy = bCanAfford;
        if (!bCanBuy)
        {
            //Can anyone on the team afford it?
            for (ShipLinkIGC*   psl = trekClient.GetSide()->GetShips()->first(); (psl != NULL); psl = psl->next())
            {
                if (psl->data() != trekClient.GetShip())
                {
                    PlayerInfo* ppi = (PlayerInfo*)(psl->data()->GetPrivateData());
                    if (ppi->GetMoney() >= shortfall)
                    {
                        bCanBuy = true;
                        break;
                    }
                }
            }
        }

        m_pnumberNewShipCost->SetValue(cost);

        if (bAlreadyOwn)
            m_pnumberLoadoutShipState->SetValue(0);
        else if (cost <= 0)
            m_pnumberLoadoutShipState->SetValue(1);
        else if (bCanAfford)
            m_pnumberLoadoutShipState->SetValue(2);
        else
            m_pnumberLoadoutShipState->SetValue(3);

        m_pbuttonLaunch->SetEnabled(bAlreadyOwn || bCanAfford);
        m_pbuttonBuy->SetEnabled(!bAlreadyOwn && bCanBuy);
        m_pbuttonConfirm->SetEnabled(!bAlreadyOwn);

        if (m_pship->GetValue() != 0){
            m_ppaneHoverBuy->SetOffset(WinPoint::Cast(m_ppointBuyButton->GetValue()));
            m_ppaneHoverConfirm->SetOffset(WinPoint(-100, -100));
        } else {
            m_ppaneHoverBuy->SetOffset(WinPoint(-100, -100));
            m_ppaneHoverConfirm->SetOffset(WinPoint::Cast(m_ppointBuyButton->GetValue()));
        }
    }

    void OnPurchaseCompleted(bool bAllPartsBought)
    {
        if (!bAllPartsBought)
        {
            TRef<IMessageBox> pmsgBox = CreateMessageBox(
                "Some of the parts you requested are no longer available");
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);

            // copy their new loadout
            m_pship->Terminate();
            m_pship->Release();
            m_pship = trekClient.CopyCurrentShip();
        }

        m_pnumberOldShipCost->SetValue(trekClient.GetShip()->GetValue());

        UpdateHullType();
    }
	
	//AaronMoore 1/10
    //////////////////////////////////////////////////////////////////////////////
    //
    // IKeyboardInput Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    {
        if (ks.bDown && m_pship->GetParentShip() == NULL && !GetWindow()->GetConsoleImage()->GetConsoleData()->IsTakingKeystrokes()) //imago #118
        {
            int num = (ks.vk - 48);

            if (num >= 0 && num < trekClient.MAX_CUSTOM_LOADOUTS)
            {
                if (ks.bControl)
                {
                    // Save the current loadout of this ship into the Custom Loadout array
                    trekClient.SaveCustomLoadout(m_pship, num);
                    
                    trekClient.PostText(false, "Saved %s loadout %d", m_pship->GetBaseHullType()->GetName(), num);

                    if (trekClient.SaveCustomLoadoutFile())
                        trekClient.PlaySoundEffect(positiveButtonClickSound);
                    
                }
                else if (!ks.bShift && !ks.bAlt)
                {
                    // Restore the current loadout from the Custom Loadout array
                    RefreshLoadout(trekClient.m_customLoadouts[num], m_pship->GetBaseHullType());

                    UpdateHullType();

                    trekClient.PostText(false, "Loaded %s loadout %d", m_pship->GetBaseHullType()->GetName(), num);

                    trekClient.PlaySoundEffect(mountSound);
                }
            }

        }

        return false;
        
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Loadout> CreateLoadout(Modeler* pmodeler, Number* ptime)
{
    return new LoadoutImpl(pmodeler, ptime);
}
