#ifndef _IndicatorImage_h_
#define _IndicatorImage_h_

//////////////////////////////////////////////////////////////////////////////
//
// Indicator Images
//
//////////////////////////////////////////////////////////////////////////////

TRef<Image> CreateIndicatorImage(Modeler* pmodeler, Viewport* pviewport, Number* ptime);

//////////////////////////////////////////////////////////////////////////////
//
// Training Overlay
//
//////////////////////////////////////////////////////////////////////////////
enum TrainingOverlay
{
    NoTrainingOverlay,
    SpeedTrainingOverlay,
    MotionIndicatorTrainingOverlay,
    ReticuleTrainingOverlay,
    ThrottleTrainingOverlay,
    FuelTrainingOverlay,
    EnergyTrainingOverlay,
    AmmoTrainingOverlay,
    HullTrainingOverlay,
    ShieldTrainingOverlay,
    ChatTrainingOverlay,
    SectorTrainingOverlay,
    InventoryTrainingOverlay,
    TargetTrainingOverlay,
    CommandTrainingOverlay,
    EyeballTrainingOverlay,
    KillBonusTrainingOverlay,
    RollTrainingOverlay,
    ShiftTabTrainingOverlay,
    TargetCenterTrainingOverlay,
    FireGunsTrainingOverlay,
    TargetEnemyTrainingOverlay,
    MissileTrainingOverlay,
    CloakTrainingOverlay
};
void        SetTrainingOverlay(TrainingOverlay overlay);
inline
void        ClearTrainingOverlay(void) { SetTrainingOverlay(NoTrainingOverlay); }

#endif
