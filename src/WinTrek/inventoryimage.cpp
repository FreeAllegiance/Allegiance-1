
#include "pch.h"
#include "consoledata.h"

namespace {
    class InventoryImage : public WrapImage, public TrekClientEventSink, public IKeyboardInput
    {
        // a search key for finding identical inventory slots
        class InventorySlotDesc
        {
            EquipmentType   et;
            Mount           mount;
            IpartIGC*       ppart;
            IshipIGC*       pgunner;
            const char*     szKey;
        public:
            InventorySlotDesc() {};
            InventorySlotDesc(EquipmentType etN, Mount mountN, IpartIGC* partN, IshipIGC* pgunnerN, const char* szKeyN)
                : et(etN), mount(mountN), ppart(partN), pgunner(pgunnerN), szKey(szKeyN) {}
            bool operator == (const InventorySlotDesc& desc) const
            {
                return (et == desc.et) && (mount == desc.mount) 
                    && (ppart == desc.ppart) && (pgunner == desc.pgunner) && (szKey == desc.szKey);
            }

            bool operator < (const InventorySlotDesc& desc) const
            {
                if (et != desc.et)
                    return et < desc.et;               
                else if (mount != desc.mount)
                    return mount < desc.mount;
                else if (ppart != desc.ppart)
                    return ppart < desc.ppart;
                else if (pgunner != desc.pgunner)
                    return pgunner < desc.pgunner;
                else
                    return szKey < desc.szKey;
            }
            bool operator > (const InventorySlotDesc& desc) const
                { return desc < (*this); }
            bool operator >= (const InventorySlotDesc& desc) const
                { return !((*this) < desc); }
        };

        typedef TMap<InventorySlotDesc, TRef<Pane> > LineMDLCache;
        LineMDLCache cache1, cache2;
        LineMDLCache *pcacheCurrent, *pcachePrevious;


    public:
        InventoryImage() :
            WrapImage(Image::GetEmpty()),
            pcacheCurrent(&cache1),
            pcachePrevious(&cache2)
        {
            Rebuild();
        };

        TRef<Pane> CreatePartPane(const ZString& strNamespace, IpartIGC* ppart, const char* szKey,
            IshipIGC* gunner = NULL, const char* szLocation = "")
        {
            TRef<PartWrapper> ppartWrapper = new PartWrapper();
            ppartWrapper->SetPart(ppart);
            TRef<ModelData> pmodelDataGunner = new ModelData();
            pmodelDataGunner->SetModel(gunner);

            TRef<Pane> ppane;
        
            TRef<INameSpace> pnsInventoryPart = GetModeler()->CreateNameSpace("inventorypart", 
                GetModeler()->GetNameSpace("console"));
            pnsInventoryPart->AddMember("part", ppartWrapper);
            pnsInventoryPart->AddMember("gunner", pmodelDataGunner);
            pnsInventoryPart->AddMember("partkey", new StringValue(szKey));
            pnsInventoryPart->AddMember("partlocation", new StringValue(szLocation));
            TRef<INameSpace> pns = GetModeler()->GetNameSpace(strNamespace);

            CastTo(ppane, pns->FindMember("InventoryLine"));

            GetModeler()->UnloadNameSpace(strNamespace);
            GetModeler()->UnloadNameSpace("inventorypart");

            return ppane;
        };

        bool ShipHasPartForMount(IshipIGC* pship, EquipmentType et, Mount mount)
        {
            if (mount < 0)
                return false;

            for (const PartLinkIGC* ppartLink = pship->GetParts()->first(); 
                ppartLink; ppartLink = ppartLink->next())
            {
                if (ppartLink->data()->GetEquipmentType() == et
                    && pship->GetHullType()->CanMount(ppartLink->data()->GetPartType(), mount))
                    return true;                
            }

            return false;
        }

        void ProcessPart(Pane* paneParent, IshipIGC* pship, EquipmentType et, Mount mount, const char* szKey,
            IshipIGC* gunner = NULL, const char* szLocation = "")
        {
            const IhullTypeIGC* pht = pship->GetHullType();

            // if the ship can mount the part in question (or if it's in inventory)...
            if (pht && (mount < 0 || pht->GetPartMask(et, mount) != 0))
            {
                // create/find an entry for this part.
                ZString strNamespace;
                IpartIGC* ppart = pship->GetMountedPart(et, mount);

                if (ppart || ShipHasPartForMount(pship, et, mount))
                {
                    TRef<Pane> ppane;

                    InventorySlotDesc desc(et, mount, ppart, gunner, szKey);

                    if (pcachePrevious->Find(desc, ppane))
                    {
                        // reuse the existing pane
                    }
                    else if (ppart)
                    {
                        strNamespace = ppart->GetPartType()->GetInventoryLineMDLName();
                        ppane = CreatePartPane(strNamespace, ppart, szKey, gunner, szLocation);
                    }
                    else
                    {                    
                        strNamespace = ZString("invEmpty") 
                            + IpartTypeIGC::GetEquipmentTypeName(et);
                        ppane = CreatePartPane(strNamespace, ppart, szKey, gunner, szLocation);
                    }

                    pcacheCurrent->Set(desc, ppane);
                    paneParent->InsertAtBottom(ppane);
                }
            };
        };

        void ProcessObservers(Pane* paneParent, IshipIGC* pship, const char* szKey)
        {
            const IhullTypeIGC* pht = pship->GetHullType();

            int nObservers = 0;

            for (ShipLinkIGC*  psl = pship->GetChildShips()->first(); (psl != NULL); psl = psl->next())
            {
                IshipIGC*   pship = psl->data();
                if (pship->GetTurretID() == NA)
                {
                    nObservers++;
                }
            }

            if (nObservers > 0)
            {
                TRef<Pane> ppane;
                InventorySlotDesc desc(NA, NA, NULL, pship, szKey);

                if (!pcachePrevious->Find(desc, ppane))
                {
                    TRef<INameSpace> pns = GetModeler()->GetNameSpace("invObserver");
                    CastTo(ppane, pns->FindMember("InventoryLine"));
                    GetModeler()->UnloadNameSpace("invObserver");
                }
        
                pcacheCurrent->Set(desc, ppane);
                paneParent->InsertAtBottom(ppane);
            }
        };

        bool ShowCargoSlot(IshipIGC* pship, Mount mount)
        {
            const IhullTypeIGC* pht = pship->GetHullType();

            if (!pht || (pht->GetCapabilities() & c_habmLifepod))
                return false;

            IpartIGC* ppart = pship->GetMountedPart(NA, mount);
            IpartTypeIGC *ppartType = ppart ? ppart->GetPartType() : NULL;

            // If this is ammo/missiles/etc, only show the first instance of this part
            ObjectType ot = ppart ? ppart->GetObjectType() : NA;
            if (ppart == NULL || ot == OT_pack || ot == OT_magazine || ot == OT_dispenser)
            {
                for (Mount mountPrev = mount + 1; mountPrev < 0; ++mountPrev)
                {
                    IpartIGC *ppartPrev = pship->GetMountedPart(NA, mountPrev);
                    IpartTypeIGC *ppartTypePrev = ppartPrev ? ppartPrev->GetPartType() : NULL;

                    if (ppartType == ppartTypePrev)
                    {
                        return false;
                        break;
                    }
                }
            }

            return true;
        };

        void ProcessCargo(Pane* paneParent, IshipIGC* pship, Mount mount, const char* szKey)
        {
            if (ShowCargoSlot(pship, mount))
            {
                IpartIGC* ppart = pship->GetMountedPart(NA, mount);

                // create an empty slot
                TRef<Pane> ppane;
                InventorySlotDesc desc(NA, NA, ppart, NULL, szKey);

                if (pcachePrevious->Find(desc, ppane))
                {
                    // reuse the existing pane
                }
                else if (ppart)
                {
                    ppane = CreatePartPane(
                        ppart->GetPartType()->GetInventoryLineMDLName(), 
                        ppart, szKey, NULL, "Cargo"
                        );
                }
                else
                {                    
                    ppane = CreatePartPane("invEmptyCargo", ppart, szKey, NULL, "Cargo");
                }

                pcacheCurrent->Set(desc, ppane);
                paneParent->InsertAtBottom(ppane);
            }
        };

        /*
        void ProcessCargo(Pane* paneParent, IshipIGC* pship, const char* szKey)
        {
            IpartIGC* ppart = trekClient.GetCargoPart();
            TRef<Pane> ppane;
            InventorySlotDesc desc(NA, NA, ppart, NULL);

            if (pcachePrevious->Find(desc, ppane))
            {
                // reuse the existing pane
            }
            else if (ppart)
            {
                ppane = CreatePartPane(
                    ppart->GetPartType()->GetInventoryLineMDLName(), 
                    ppart, szKey, NULL, "Cargo"
                    );
            }
            else
            {                    
                ppane = CreatePartPane("invEmptyCargo", ppart, szKey, NULL, "Cargo");
            }

            pcacheCurrent->Set(desc, ppane);
            paneParent->InsertAtBottom(ppane);
        };
        */

        void ProcessTurret(Pane* ppaneParent, IshipIGC* pship, Mount mount, const char* szKey)
        {
            IshipIGC* pshipGunner = pship->GetGunner(mount);
            IpartIGC* ppart = pship->GetMountedPart(ET_Weapon, mount);
            const IhullTypeIGC* pht = pship->GetHullType();

            ProcessPart(ppaneParent, pship, ET_Weapon, mount, szKey,
                pshipGunner, pht->GetHardpointData(mount).locationAbreviation);

            if (pshipGunner != NULL)
            {
                TRef<Pane> ppaneTurret;
                InventorySlotDesc desc(NA, mount, ppart, pshipGunner, szKey);

                if (!pcachePrevious->Find(desc, ppaneTurret))
                {
                    ppaneTurret = CreatePartPane(pshipGunner ? "invturret" : "invemptyturret",
                        ppart, "", pshipGunner,
                        pht->GetHardpointData(mount).locationAbreviation);
                }

                pcacheCurrent->Set(desc, ppaneTurret);
                ppaneParent->InsertAtBottom(ppaneTurret);
            }
        }

        void Rebuild()
        {
            IshipIGC* pship = trekClient.GetShip()->GetSourceShip();
            const IhullTypeIGC* pht = pship->GetHullType();
            bool bIsPilot = trekClient.GetShip()->GetParentShip() == NULL;

            // swap the old and the new cache
            LineMDLCache *pcacheTemp = pcachePrevious;
            pcachePrevious = pcacheCurrent;
            pcacheCurrent = pcacheTemp;
            
            TRef<ColumnPane> pColumnPane = new ColumnPane();
			//TRef<RowPane> pRowPane = new RowPane();

			//TRef<Pane> pTab = new ImagePane(GetModeler()->LoadImage(
			//	"inventorytabbmp", false));
			
			TRef<Pane> pTop = new ImagePane(GetModeler()->LoadImage(
              "inventorytopbmp", false));
			

            TRef<Pane> pBottom = new ImagePane(GetModeler()->LoadImage(
              "inventorybottombmp", false));

			//pRowPane->InsertAtBottom(pTab);
			pColumnPane->InsertAtBottom(pTop);

            static const char* const vszWeaponHotKeys[] = { "1.", "2.", "3.", "4." };
            static const char* const vszTurretHotKeys[] = { "Y.", "U.", "I.", "O.", "", "", "", "" };
            ZAssert(c_maxUnmannedWeapons <= sizeof(vszWeaponHotKeys)/sizeof(const char* const));
            ZAssert(c_maxMountedWeapons <= sizeof(vszTurretHotKeys)/sizeof(const char* const));

            Mount mount;
            int nFixedWeapons = pht->GetMaxFixedWeapons();
            for (mount = 0; mount < nFixedWeapons; mount++)
            {
                ProcessPart(pColumnPane, pship, ET_Weapon, mount, 
                    bIsPilot ? vszWeaponHotKeys[mount] : "");
            };
            ProcessObservers(pColumnPane, pship, "");
            for (; mount < pht->GetMaxWeapons(); mount++)
            {
                ProcessTurret(pColumnPane, pship, mount, 
                    bIsPilot ? vszTurretHotKeys[mount - nFixedWeapons] : "");
            };

            ProcessPart(pColumnPane, pship, ET_Magazine, 0, "5.");
            ProcessPart(pColumnPane, pship, ET_ChaffLauncher, 0, "6.");
            ProcessPart(pColumnPane, pship, ET_Shield, 0, "7.");
            ProcessPart(pColumnPane, pship, ET_Cloak, 0, "8.");
            ProcessPart(pColumnPane, pship, ET_Afterburner, 0, "9.");
            ProcessPart(pColumnPane, pship, ET_Dispenser, 0, "0.");

            ProcessCargo(pColumnPane, pship, trekClient.GetSelectedCargoMount(), "S.");
            for (int nCargoOffset = 1; nCargoOffset < c_maxCargo; nCargoOffset++)
            {
                Mount mountCargo = trekClient.GetSelectedCargoMount() - nCargoOffset;
                if (mountCargo < -c_maxCargo)
                    mountCargo += c_maxCargo;
                ProcessCargo(pColumnPane, pship, mountCargo, "");
            }

            //ProcessCargo(pColumnPane, pship, "S.");
            SetImage(CreatePaneImage(GetEngine(), true, pColumnPane));
			pColumnPane->InsertAtBottom(pBottom);
			//pRowPane->InsertAtBottom(pColumnPane);
            pcachePrevious->SetEmpty();
        }

        void OnLoadoutChanged(IpartIGC* ppart, LoadoutChange lc)
        {
            switch (lc)
            {
            case c_lcMounted:
            case c_lcDismounted:
            case c_lcAdded:
            case c_lcRemoved:
            case c_lcHullChange:
            case c_lcAddPassenger:
            case c_lcRemovePassenger:
            case c_lcPassengerMove:
            case c_lcTurretChange:
            case c_lcCargoSelectionChanged:
                // don't bother updating the inventory while in the hangar
                if (trekClient.GetShip()->GetCluster())
                {
                    Rebuild();
                }
                else
                {
                    // we do need to make sure we don't hang on to parts that 
                    // have changed, however, so destroy the inventory pane.
                    SetImage(Image::GetEmpty());
                    pcacheCurrent->SetEmpty();
                }

                break;

            case c_lcQuantityChange:
            case c_lcDisembark:
                // don't try to handle this
                break;

            default:
                ZError("Unknown loadout change");
            }
        }

        void OnClusterChanged(IclusterIGC* pcluster)
        {
            if (pcluster != NULL)
            {
                // this should be safe even if we are just flying from aleph 
                // to aleph because nothing should have changed.
                Rebuild();
            }
        }
    };

}

TRef<IObject> InventoryImageFactory::Apply(ObjectStack& stack)
{
    return (Value*)(new InventoryImage());
};
