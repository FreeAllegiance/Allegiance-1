/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:	missionIGC.cpp
**
**  Author: 
**
**  Description:
**      Implementation of the CmissionIGC class. This file was initially created by
**  the ATL wizard for the mission object.
**
**      mission is currently setup as an object that can be instantiated multiple times
**  (generating a unique instance each time). This should probably change to allowing
**  only a single instance ... but it doesn't matter much. All the state (except for the
**  ref count) used by mission is stored as a static in the igc class.
**
**  History:
*/
// missionIGC.cpp : Implementation of CmissionIGC
#include    "pch.h"
#include    "missionIGC.h"
#include    "clusterIGC.h"
#include    "warpIGC.h"
#include    "dispenserIGC.h"
#include    "buildingEffectIGC.h"
#include    "hullTypeIGC.h"
#include    "buoyIGC.h"
#include    "shipIGC.h"
#include    "chaffIGC.h"
#include    "shieldIGC.h"
#include    "cloakIGC.h"
#include    "projectileIGC.h"
#include    "projectileTypeIGC.h"
#include    "partTypeIGC.h"
#include    "launcherTypeIGC.h"
#include    "weaponIGC.h"
#include    "asteroidIGC.h"
#include    "stationIGC.h"
#include    "treasureIGC.h"
#include    "treasureSetIGC.h"
#include    "afterburnerIGC.h"
#include    "packIGC.h"
#include    "magazineIGC.h"
#include    "missileTypeIGC.h"
#include    "probeIGC.h"
#include    "probeTypeIGC.h"
#include    "mineTypeIGC.h"
#include    "chaffTypeIGC.h"
#include    "missileIGC.h"
#include    "mineIGC.h"
#include    "civilizationIGC.h"
#include    "sideIGC.h"
#include    "stationTypeIGC.h"
#include    "developmentIGC.h"
#include    "droneTypeIGC.h"
#include    "bucketigc.h"
#include    "mapmakerigc.h"
static void DoDecrypt(int size, char* pdata)
{
    DWORD encrypt = 0;
    //Do a rolling XOR to demunge the data
    for (int i = 0; (i < size); i += 4)
    {
        DWORD*  p = (DWORD*)(pdata + i);

        encrypt = *p = *p ^ encrypt;
    }
}

//------------------------------------------------------------------------------
void    DumpIGCFile (FILE* file, ImissionIGC* pMission, __int64 iMaskExportTypes,  void (*munge)(int size, char* data))
{
    int         iSize = pMission->Export(iMaskExportTypes, 0);
    char*       pData = new char[iSize+4];      //leave a little extra space for the encryption (which takes dword chunks)
    pMission->Export (iMaskExportTypes, pData);
    fwrite (&iSize, sizeof(int), 1, file);

    if (munge)
    {
        (*munge)(iSize, pData);
    }

    fwrite (pData, sizeof(char), iSize, file);
    delete [] pData;
}

//------------------------------------------------------------------------------
void    LoadIGCFile (FILE* file, ImissionIGC* pMission, void (*munge)(int size, char* data))
{
    Time            now = pMission->GetLastUpdate();
    int             iDatasize;
    int             iReadCount = fread (&iDatasize, sizeof(iDatasize), 1, file);
    assert (iReadCount == 1);
    char*           pData = new char[iDatasize+4];      //leave a little extra space for the encryption (which takes dword chunks)
    iReadCount = fread (pData, sizeof(char), iDatasize, file);
    assert (iReadCount == iDatasize);

    if (munge)
    {
        (*munge)(iDatasize, pData);
    }

    pMission->Import (now, -1, pData, iDatasize);
    delete [] pData;
}

//Imago
//moved from FedSrv.CPP and made data private on 7/28/08
int CmissionIGC::TechBitFromToken(const char * szToken)
{
  int itoken = 0;
  while (lstrcmpi(this->rgTechs[itoken], szToken))
  {
    if(itoken == cTechs)
      return NA;
    itoken++;
  }
  return itoken;
}

//moved from FedSrv.CPP and made data private on 7/28/08
void CmissionIGC::TechsListToBits(const char * szTechs, TechTreeBitMask & ttbm)
{
	ttbm.ClearAll();
	
	if(strcmp(szTechs,"0") == 0) {
		return;
	}

	//Imago added hack for the bit array string stored in the binary IGC 8/1/08
	if(isdigit(szTechs[0])) {
		ttbm.FromString(szTechs);
		return;
	}

  char * token;
  token = strtok((char *) szTechs, " ");
  while(token)
  {
    ttbm.SetBit(TechBitFromToken(token));
    token = strtok(NULL, " ");
  }
}

//gets a single flag names part mask the hard way
PartMask CmissionIGC::PartMaskFromToken(const char * szToken, EquipmentType et)
{
	if (isdigit(szToken[0]))
		return atoi(szToken);

	ZString strTemp = ZString(szToken);
	strTemp.RemoveAll(' ');	

	if(strTemp.IsEmpty())
		return 0;

	int itoken = 0;
	while (lstrcmpi(this->rgParts[et][itoken],(PCC)strTemp))
	{
		if(itoken == 16)
			return 0;
		itoken++;
	}

	// I know there is a better way (using math) please show me how! (i forget)
	if(itoken==0)
		return c_pbm1;
	if(itoken==1)
		return c_pbm2;
	if(itoken==2)
		return c_pbm3;
	if(itoken==3)
		return c_pbm4;
	if(itoken==4)
		return c_pbm5;
	if(itoken==5)
		return c_pbm6;
	if(itoken==6)
		return c_pbm7;
	if(itoken==7)
		return c_pbm8;
	if(itoken==8)
		return c_pbm9;
	if(itoken==9)
		return c_pbm10;
	if(itoken==10)
		return c_pbm11;
	if(itoken==11)
		return c_pbm12;
	if(itoken==12)
		return c_pbm13;
	if(itoken==13)
		return c_pbm14;
	if(itoken==14)
		return c_pbm15;
	if(itoken==15)
		return c_pbm16;

	return NA; // never happen
}

// flag name helper function, if any IGC.h ability mask changes, this must change as well.
AbilityBitMask AbilitiesListToMask(const char * szAbilities, AbilityType at) {
	ZString strCapabilities = ZString(szAbilities);
	if (strCapabilities.IsEmpty())
		return 0;

	switch(at) {
		case AT_Hull: {
			HullAbilityBitMask habm = 0;
			ZString strHullCap = strCapabilities.GetToken();
			while(!(strHullCap.IsEmpty())) {
				if(strHullCap.ReverseFind("CanBoard") == 0)
					habm |= c_habmBoard;
				if(strHullCap.ReverseFind("CanRescue") == 0)
					habm |= c_habmRescue;
				if(strHullCap.ReverseFind("IsPod") == 0)
					habm |= c_habmLifepod;
				if(strHullCap.ReverseFind("CanCapture") == 0)
					habm |= c_habmCaptureThreat;
				if(strHullCap.ReverseFind("CanLandOnCarrier") == 0)
					habm |= c_habmLandOnCarrier;
				if(strHullCap.ReverseFind("CanNotRip") == 0)
					habm |= c_habmNoRipcord;
				if(strHullCap.ReverseFind("IsRipcordTarget") == 0)
					habm |= c_habmIsRipcordTarget;
				if(strHullCap.ReverseFind("IsFighter") == 0)
					habm |= c_habmFighter;
				if(strHullCap.ReverseFind("IsRemoteLeadIndicator") == 0)
					habm |= c_habmRemoteLeadIndicator;
				if(strHullCap.ReverseFind("CanBomb") == 0)
					habm |= c_habmThreatToStation;
				if(strHullCap.ReverseFind("IsCarrier") == 0)
					habm |= c_habmCarrier;
				if(strHullCap.ReverseFind("CanLeadIndicator") == 0)
					habm |= c_habmLeadIndicator;
				if(strHullCap.ReverseFind("IsLtRip") == 0)
					habm |= c_habmIsLtRipcordTarget;
				if(strHullCap.ReverseFind("CanLtRip") == 0)
					habm |= c_habmCanLtRipcord;
				if(strHullCap.ReverseFind("IsMiner") == 0)
					habm |= c_habmMiner;
				if(strHullCap.ReverseFind("IsBuilder") == 0)
					habm |= c_habmBuilder;

				strHullCap = strCapabilities.GetToken();
			}
			return habm;
		}
		break;
		case AT_Station: {
			StationAbilityBitMask sabm = 0;
			ZString strStationCap = strCapabilities.GetToken();
			while(!(strStationCap.IsEmpty())) {	
				if(strStationCap.ReverseFind("CanBoard") == 0)
					sabm |= c_habmBoard;
				if(strStationCap.ReverseFind("CanOffloadMinerals") == 0)
					sabm |= c_sabmUnload;
				if(strStationCap.ReverseFind("CanStartHere") == 0)
					sabm |= c_sabmStart;
				if(strStationCap.ReverseFind("CanRestartHere") == 0)
					sabm |= c_sabmRestart;
				if(strStationCap.ReverseFind("CanRipHere") == 0)
					sabm |= c_sabmRipcord;
				if(strStationCap.ReverseFind("CanBeCaptured") == 0)
					sabm |= c_sabmCapture;
				if(strStationCap.ReverseFind("CanLandHere") == 0)
					sabm |= c_sabmLand;
				if(strStationCap.ReverseFind("CanRepair") == 0)
					sabm |= c_sabmRepair;
				if(strStationCap.ReverseFind("IsVisibleInLoadout") == 0)
					sabm |= c_sabmRemoteLeadIndicator;
				if(strStationCap.ReverseFind("CanReload") == 0)
					sabm |= c_sabmReload;
				if(strStationCap.ReverseFind("IsVictoryObjective") == 0)
					sabm |= c_sabmFlag;
				if(strStationCap.ReverseFind("IsFlagPedestal") == 0)
					sabm |= c_sabmPedestal;
				if(strStationCap.ReverseFind("CanTeleportOffloadMinerals") == 0)
					sabm |= c_sabmTeleportUnload;
				if(strStationCap.ReverseFind("CanLandCapitols") == 0)
					sabm |= c_sabmCapLand;
				if(strStationCap.ReverseFind("CanRescuePods") == 0)
					sabm |= c_sabmRescue;
				if(strStationCap.ReverseFind("CanRescueAnyPods") == 0)
					sabm |= c_sabmRescueAny;

				strStationCap = strCapabilities.GetToken();
			}
			return sabm;
		}
		break;
		case AT_Expendable: {
			ExpendableAbilityBitMask eabm = 0;
			ZString strExpCap = strCapabilities.GetToken();
			while(!(strExpCap.IsEmpty())) {
				if(strExpCap.ReverseFind("CanCaptureStations") == 0)
					eabm |= c_eabmCapture;
				if(strExpCap.ReverseFind("CanWarpBombDual") == 0)
					eabm |= c_eabmWarpBombDual;
				if(strExpCap.ReverseFind("CanWarpBombSingle") == 0)
					eabm |= c_eabmWarpBombSingle;
				if(strExpCap.ReverseFind("CanQuickReady") == 0)
					eabm |= c_eabmQuickReady;
				if(strExpCap.ReverseFind("CanRipcord") == 0)
					eabm |= c_eabmRipcord;
				if(strExpCap.ReverseFind("CanShootStations") == 0)
					eabm |= c_eabmShootStations;
				if(strExpCap.ReverseFind("CanShootShips") == 0)
					eabm |= c_eabmShootShips;
				if(strExpCap.ReverseFind("CanShootMissiles") == 0)
					eabm |= c_eabmShootMissiles;
				if(strExpCap.ReverseFind("CanShootTargetOnly") == 0)
					eabm |= c_eabmShootOnlyTarget;
				if(strExpCap.ReverseFind("CanRescue") == 0)
					eabm |= c_eabmRescue;
				if(strExpCap.ReverseFind("CanRescueAny") == 0)
					eabm |= c_eabmRescueAny;

				strExpCap = strCapabilities.GetToken();
			}
			return eabm;
		}
		break;
		case AT_Asteroid: {
			AsteroidAbilityBitMask aabm = 0;
			ZString strAsteroidCap = strCapabilities.GetToken();
			while(!(strAsteroidCap.IsEmpty())) {
				if(strAsteroidCap.ReverseFind("CanBeRegular") == 0)
					aabm |= c_aabmBuildable;
				if(strAsteroidCap.ReverseFind("CanBeUranium") == 0)
					aabm |= (c_aabmSpecial << 0);
				if(strAsteroidCap.ReverseFind("CanBeSilicon") == 0)
					aabm |= (c_aabmSpecial << 1);
				if(strAsteroidCap.ReverseFind("CanBeCarbon") == 0)
					aabm |= (c_aabmSpecial << 2);
				if(strAsteroidCap.ReverseFind("CanBeHelium") == 0)
					aabm |= c_aabmMineHe3;
				if(strAsteroidCap.ReverseFind("CanBeDenseHelium") == 0)
					aabm |= c_aabmMineLotsHe3;
				if(strAsteroidCap.ReverseFind("CanBeGold") == 0)
					aabm |= c_aabmMineGold;

				strAsteroidCap = strCapabilities.GetToken();
			}
			return aabm;
		}
		break;
	}
	return NA; // never happen
}


// for making an allowed hull/hardpoint equipment mask from bit flag names
PartMask CmissionIGC::PartsListToMask(const char * szParts, EquipmentType et)
{
	if (isdigit(szParts[0]))
		return atoi(szParts);

	char * token; PartMask pm = 0;
	token = strtok((char *) szParts, " ");
	while(token)
	{
		pm += this->PartMaskFromToken(token,et);
		token = strtok(NULL, " ");
	}
	return pm;
}

//looks up the flag names for a 'tech bit' from a zlib mask
ZString CmissionIGC::BitsToTechsList(TechTreeBitMask & ttbm)
{
	ZString strList;

	//use Zmask.ToString() if the new rgTechs isn't loaded
	if(strcmp((PCC)GetTechFlagName(0),"unavailable") != 0) {
		char  * pszTechs = new char[(c_ttbMax + 7) / 8 * 2 + 1];
		ttbm.ToString(pszTechs,(c_ttbMax + 7) / 8 * 2 + 1);
		pszTechs[100] = '\0';  //tidy
		return ZString(pszTechs);
	}

	for(int i=0;i<c_ttbMax;i++) {
		if(ttbm.GetBit(i)) {
			ZString strTemp = ZString(GetTechFlagName(i));
			if(strTemp.GetLength() != 0) {
				strList += strTemp;
				if(i < c_ttbMax - 1)
					strList += " ";
			} else {
				debugf("BitsToTechsList: got a nameless tech bit #%i\n",i);
			}
		} 
	}
	if(strList.GetLength() < 3) {
		if (!(ttbm.GetAllZero())) {
			//use Zmask.ToString() for an empty mask name list that isn't supposed to be
			char  * pszTechs = new char[(c_ttbMax + 7) / 8 * 2 + 1];
			ttbm.ToString(pszTechs,(c_ttbMax + 7) / 8 * 2 + 1);
			pszTechs[100] = '\0';  //tidy
			return ZString(pszTechs);
		} else {
			return "0";
		}
	}
	return strList;
}

//looks up the flag names for an equipment type's 'part bit' from a part mask
//this allows for named hull/weapon classes like the AZ database used.
ZString CmissionIGC::BitsToPartsList(PartMask & pm, EquipmentType et)
{
	ZString strList;

	//use short if the new rgParts isn't loaded
	if(ZString(GetPartFlagName(0,et)).GetLength() < 3) {
		return ZString(pm);
	}
	
	//there is a better way...I beg you please to show me the way
	#define ISSET(id) if(c_pbm##id & pm)\
		strList += ZString(GetPartFlagName(id-1,et)) + " ";

		ISSET(1)
		ISSET(2)
		ISSET(3)
		ISSET(4)
		ISSET(5)
		ISSET(6)
		ISSET(7)
		ISSET(8)
		ISSET(9)
		ISSET(10)
		ISSET(11)
		ISSET(12)
		ISSET(13)
		ISSET(14)
		ISSET(15)
		ISSET(16)

	#undef ISSET
		return (strList.GetLength() > 3) ? strList : ZString(pm);
}

bool CmissionIGC::LoadTechBitsList() {
	typedef std::vector< std::vector<std::string> > csvVector;
	//Load Techbits
	ZString strFile = ZString(UTL::artworkPath()) + ZString(this->GetMissionParams()->szIGCStaticFile) + "\\BitsOfTechs.csv";
	std::fstream file0((PCC)strFile, std::ios::in);
	if(file0.is_open()) {
		csvVector csvData0;
		readCSV(file0, csvData0);
		ZString strFlag; ZString strMask;
		for(csvVector::iterator i = csvData0.begin(); i != csvData0.end(); ++i)
		{
			if (i != csvData0.begin()) { //skip the header row
				int id = 0;
				int colnum = 1;
				for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
				{
					std::string sdata = *j;
					if(j == i->begin()) {
						id = atoi(sdata.c_str()); //aka BitID
					} else {
						if(colnum == 2)
							strFlag = ZString(sdata.c_str()); //aka short name
						if(colnum == 3) {
							strMask = ZString(sdata.c_str()); //aka long name
						}
					}
					colnum++;
				}
				SetTechFlagName(id,strFlag);
			}
		}
		file0.close();
		return true;
	}
	return false;
}
bool CmissionIGC::LoadPartsBitsList() {
	typedef std::vector< std::vector<std::string> > csvVector;
	//Load Partbits
	ZString strFile = ZString(UTL::artworkPath()) + ZString(this->GetMissionParams()->szIGCStaticFile) + "\\BitsOfParts.csv";
	std::fstream file0((PCC)strFile, std::ios::in);
	if(file0.is_open()) {
		csvVector csvData0;
		readCSV(file0, csvData0);
		int bitnum = 0;
		for(csvVector::iterator i = csvData0.begin(); i != csvData0.end(); ++i)
		{
			if (i != csvData0.begin()) { //skip the header row
				ZString strFlag;
				int  colnum = 1;  int et = 0;
				if(bitnum >= 16) {
					bitnum = 0;
					if(et == ET_MAX - 1)
						break;  //stop now no matter what
				}
				for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
				{
					std::string sdata = *j;
					if(j == i->begin()) {
						et = atoi(sdata.c_str());
						
					} else {
						if (colnum == 2)
							strFlag = ZString(sdata.c_str());
					}
					colnum++;
				}
				SetPartFlagName(bitnum,et,strFlag);
				bitnum++;
			}
		}
		file0.close();
		return true;
	}
	return false;
}


//Imago be lazy and read in a CSV format
void readCSV(std::istream &input, std::vector< std::vector<std::string> > &output)
{
	std::string csvLine;
	while( std::getline(input, csvLine) )
	{
		std::istringstream csvStream(csvLine);
		std::vector<std::string> csvColumn;
		std::string csvElement;

		while( std::getline(csvStream, csvElement, '\t') )
		{
			csvColumn.push_back(csvElement);
		}		
		output.push_back(csvColumn);
	}
}

void CmissionIGC::ImportStaticIGCObjs() //is opposite of ExportStaticIGCObjs()
{
	ImissionIGC* pMission = this;

	typedef std::vector< std::vector<std::string> > csvVector;

	Time now = pMission->GetLastUpdate();

	if (!(pMission->LoadTechBitsList()))
		debugf("Warning: Failed to load Tech mask bit flags! (\\artwork\\%s\\BitsOfTechs.csv)\n",pMission->GetMissionParams()->szIGCStaticFile);
	if (!(pMission->LoadPartsBitsList()))
		debugf("Warning: Failed to load Part mask bit flags! (\\artwork\\%s\\BitsOfParts.csv)\n",pMission->GetMissionParams()->szIGCStaticFile);

	//our running size
	int iDatasize = 0;


	//NYI read all the text at once to determine these sizes
	//static length objects
	//int				iSize = pMission->Export(c_maskStaticTypes, 0); //Constants
	/*
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataCivilizationIGC)) * 20; 
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataStationTypeIGC)) * 62; 
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataDroneTypeIGC)) * 27; 
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataDevelopmentIGC)) * 189; 
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataProjectileIGC)) * 36; 

	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataLauncherTypeIGC)) * 58; 
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataChaffTypeIGC)) * 6; 
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataMineTypeIGC)) * 6; 
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataMissileTypeIGC)) * 28; 
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataProbeTypeIGC)) * 22; 

	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataAfterburnerTypeIGC)) * 5;
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataCloakTypeIGC)) * 5;
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataPackTypeIGC)) * 2;
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataShieldTypeIGC)) * 9;
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataWeaponTypeIGC)) * 31;

	//variable length objects
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataHullTypeIGC) + (sizeof(HardpointData) * c_maxMountedWeapons)) * 189;  
	iSize += (sizeof(int) + sizeof(ObjectType) + sizeof(DataTreasureSetIGC)) * 1;
	iSize += sizeof(TreasureData) * 15;
	*/

	char* pData = new char[1024 * 1024];  // 1M max of IGC data

	//Load Constants
	ZString strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Constants.csv";
	std::fstream file((PCC)strFile, std::ios::in);
	assert(file.is_open());
	csvVector csvData;
	
	readCSV(file, csvData);
	Constants* pConstants = (Constants*)(new char [sizeof(Constants)]);
	int iLine = 0;
	int cnt = 0;
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) { //skip the header row
			int id = 0;
			// two different ordered row/column types (global constants, damage constants)
			if (iLine < c_fcidMax) {
				int colnum = 0;
				for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
				{
					colnum++;
					std::string sdata = *j;
					if(j == i->begin()) {
						id = atoi(sdata.c_str());
					} else {
						if(colnum==2)
							pConstants->floatConstants[id] = atof(sdata.c_str());
					}
				}
			} else {
				int colnum = 0;
				for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
				{
					colnum++;
					std::string sdata = *j;
					if(j == i->begin()) {
						id = atoi(sdata.c_str());
					} else {
						if(colnum==2)
							pConstants->damageConstants[id][cnt] = atof(sdata.c_str());
					}
				}
				cnt++;
				if (cnt == c_defidMax)
					cnt = 0;
			}
			iLine++;
		}
	}
	file.close();
	// delete file; KG- not a pointer anymore

	int size = sizeof(Constants);
	*((ObjectType*)pData) = OT_constants;
	*((int*)(pData + sizeof(ObjectType))) = size;
	iDatasize += sizeof(int) + sizeof(ObjectType) + size;
	memcpy(pData + sizeof(int) + sizeof(ObjectType), pConstants, iDatasize);

	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Expendables.csv";
	std::fstream file10((PCC)strFile, std::ios::in);
	assert(file10.is_open());
	csvVector csvData4;
	readCSV(file10, csvData4);

	// Load projectile types
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\TypesOfProjectiles.csv";
	std::fstream projfile((PCC)strFile, std::ios::in);
	assert(projfile.is_open());
	csvData.clear();
	readCSV(projfile, csvData);
	int iLines = 0; int id = 0;
	size = sizeof(DataProjectileTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataProjectileTypeIGC* pProj = (DataProjectileTypeIGC*)(new char [size]);
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					pProj->projectileTypeID = atoi(sdata.c_str());
				} else {
					if(colnum == 2)
						pProj->power = atof(sdata.c_str());
					if(colnum == 3)
						pProj->blastPower = atof(sdata.c_str());
					if(colnum == 4)
						pProj->blastRadius = atof(sdata.c_str());
					if(colnum == 5)
						pProj->speed = atof(sdata.c_str());
					if(colnum == 6)
						pProj->lifespan = atof(sdata.c_str());
					if(colnum == 7)
						pProj->damageType = ZString(sdata.c_str()).GetToken().GetInteger();
					if(colnum == 8)
						pProj->absoluteF = atoi(sdata.c_str());
					if(colnum == 9)
						pProj->bDirectional = atoi(sdata.c_str());
					if(colnum == 10)
						pProj->width = atof(sdata.c_str());;
					if(colnum == 11)
						pProj->ambientSound = atoi(sdata.c_str());
					if(colnum == 12) {
						ZString strColor = ZString(sdata.c_str());
						pProj->color.r = strColor.GetToken().GetInteger();
						pProj->color.g = strColor.GetToken().GetInteger();
						pProj->color.b = strColor.GetToken().GetInteger();
						pProj->color.a = strColor.GetToken().GetInteger();
					}
					if(colnum == 13)
						pProj->radius = atof(sdata.c_str());
					if(colnum == 14)
						pProj->rotation = atof(sdata.c_str());
					if(colnum == 15)
						Strcpy(pProj->modelName,sdata.c_str());
					if(colnum == 16)
						Strcpy(pProj->textureName,sdata.c_str());
				}
			}


			*((ObjectType*)(pData + iDatasize)) = OT_projectileType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pProj, size);

			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load missile expendable
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\ExpendableMissiles.csv";
	std::fstream file12((PCC)strFile, std::ios::in);
	assert(file12.is_open());
	csvData.clear();
	readCSV(file12, csvData);
	iLines = 0; id = 0;
	size = sizeof(DataMissileTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataMissileTypeIGC* pMissile = (DataMissileTypeIGC*)(new char [size]);
			DataLauncherTypeIGC * pPart = (DataLauncherTypeIGC*)(new char [sizeof(DataLauncherTypeIGC)]);
			pPart->partID = 0;
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					id = pMissile->expendabletypeID = atoi(sdata.c_str());
					for(csvVector::iterator m = csvData4.begin(); m != csvData4.end(); ++m)
					{
						if (m != csvData4.begin()) {
							int id2 = 0; int colnum2 = 0;
							for(std::vector<std::string>::iterator n = m->begin(); n != m->end(); ++n)
							{
								colnum2++;
								std::string sdata2 = *n;
								if(n == m->begin()) {
									id2 = atoi(sdata2.c_str());
									if (id == id2)
										pPart->expendabletypeID = id2;
									else 
										break;
								} else {
									if (id == id2) {
										if(colnum2 == 2)
											pPart->partID = atoi(sdata2.c_str());
										if(colnum2 == 3)
											pPart->amount = atoi(sdata2.c_str());
										if(colnum2 == 4)
											pPart->successorPartID = atoi(sdata2.c_str());
										if(colnum2 == 5)
											pPart->launchCount = atoi(sdata2.c_str());
										if(colnum2 == 6)
											Strcpy(pPart->inventoryLineMDL,sdata2.c_str());
										if(colnum2 == 7)
											pMissile->launcherDef.price = atoi(sdata2.c_str());
										if(colnum2 == 8)
											pMissile->launcherDef.timeToBuild = atoi(sdata2.c_str());
										if(colnum2 == 9)
											Strcpy(pMissile->launcherDef.modelName,sdata2.c_str());
										if(colnum2 == 10)
											Strcpy(pMissile->launcherDef.iconName,sdata2.c_str());
										if(colnum2 == 11)
											Strcpy(pMissile->launcherDef.name,sdata2.c_str());
										if(colnum2 == 12)
											Strcpy(pMissile->launcherDef.description,sdata2.c_str());
										if(colnum2 == 13)
											pMissile->launcherDef.groupID = atoi(sdata2.c_str());
										if(colnum2 == 14)
											pMission->TechsListToBits(sdata2.c_str(),pMissile->launcherDef.ttbmRequired);
										if(colnum2 == 15)
											pMission->TechsListToBits(sdata2.c_str(),pMissile->launcherDef.ttbmEffects);
									} else {
										break;
									}
								}
							}
						}
					}
				} else {
					if(colnum == 2)
						pMissile->acceleration = atof(sdata.c_str());
					if(colnum == 3)
						pMissile->turnRate = atof(sdata.c_str());
					if(colnum == 4)
						pMissile->initialSpeed = atof(sdata.c_str());
					if(colnum == 5)
						pMissile->lockTime = atof(sdata.c_str());
					if(colnum == 6)
						pMissile->readyTime = atof(sdata.c_str());
					if(colnum == 7)
						pMissile->maxLock = atof(sdata.c_str());
					if(colnum == 8)
						pMissile->chaffResistance = atof(sdata.c_str());
					if(colnum == 9)
						pMissile->dispersion = atof(sdata.c_str());
					if(colnum == 10)
						pMissile->lockAngle = atof(sdata.c_str());
					if(colnum == 11)
						pMissile->power = atof(sdata.c_str());
					if(colnum == 12)
						pMissile->blastPower = atof(sdata.c_str());
					if(colnum == 13)
						pMissile->blastRadius = atof(sdata.c_str());
					if(colnum == 14)
						pMissile->width = atof(sdata.c_str());
					if(colnum == 15)
						pMissile->damageType = ZString(sdata.c_str()).GetToken().GetInteger(); 
					if(colnum == 16)
						pMissile->bDirectional = atoi(sdata.c_str());
					if(colnum == 17)
						pMissile->launchSound = atoi(sdata.c_str());
					if(colnum == 18)
						pMissile->ambientSound = atoi(sdata.c_str());
					if(colnum == 19)
						pMissile->loadTime = atof(sdata.c_str());
					if(colnum == 20)
						pMissile->lifespan = atof(sdata.c_str());
					if(colnum == 21)
						pMissile->signature = atof(sdata.c_str());
					if(colnum == 22)
						pMissile->hitPoints = atof(sdata.c_str());
					if(colnum == 23)
						pMissile->defenseType = ZString(sdata.c_str()).GetToken().GetInteger(); 
					if(colnum == 24)
						pMissile->eabmCapabilities = AbilitiesListToMask(sdata.c_str(),AT_Expendable);
					if(colnum == 25)
						Strcpy(pMissile->iconName,sdata.c_str());
					if(colnum == 26) {
						ZString strColor = ZString(sdata.c_str());
						pMissile->color.r = strColor.GetToken().GetInteger();
						pMissile->color.g = strColor.GetToken().GetInteger();
						pMissile->color.b = strColor.GetToken().GetInteger();
						pMissile->color.a = strColor.GetToken().GetInteger();
					}
					if(colnum == 27)
						pMissile->radius = atof(sdata.c_str());
					if(colnum == 28)
						pMissile->rotation = atof(sdata.c_str());
					if(colnum == 29)
						Strcpy(pMissile->modelName,sdata.c_str());
					if(colnum == 30)
						Strcpy(pMissile->textureName,sdata.c_str());
					if(colnum == 31)
						pMissile->launcherDef.signature = atof(sdata.c_str());
					if(colnum == 32)
						pMissile->launcherDef.mass = atof(sdata.c_str());
					if(colnum == 33)
						pMissile->launcherDef.partMask = pMission->PartMaskFromToken(sdata.c_str(),ET_Magazine);
					if(colnum == 34)
						pMissile->launcherDef.expendableSize = atoi(sdata.c_str());
				}
			}

			if (pPart->partID ==0) {
				debugf("non-buyable magazine (for drones only) %i\n",pMissile->expendabletypeID);
			}

			*((ObjectType*)(pData + iDatasize)) = OT_missileType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pMissile, size);
			
			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load probes expendable
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\ExpendableProbes.csv";
	std::fstream file18((PCC)strFile, std::ios::in);
	assert(file18.is_open());
	csvData.clear();
	readCSV(file18, csvData);
	iLines = 0; id = 0;
	size = sizeof(DataProbeTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataProbeTypeIGC* pProbe = (DataProbeTypeIGC*)(new char [size]);
			DataLauncherTypeIGC * pPart = (DataLauncherTypeIGC*)(new char [sizeof(DataLauncherTypeIGC)]);
			pPart->partID = 0;

			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					id = pProbe->expendabletypeID = atoi(sdata.c_str());
					for(csvVector::iterator m = csvData4.begin(); m != csvData4.end(); ++m)
					{
						if (m != csvData4.begin()) {
							int id2 = 0; int colnum2 = 0;
							for(std::vector<std::string>::iterator n = m->begin(); n != m->end(); ++n)
							{
								colnum2++;
								std::string sdata2 = *n;
								if(n == m->begin()) {
									id2 = atoi(sdata2.c_str());
									if (id == id2)
										pPart->expendabletypeID = id2;
								} else {
									if (id == id2) {
										if(colnum2 == 2)
											pPart->partID = atoi(sdata2.c_str());
										if(colnum2 == 3)
											pPart->amount = atoi(sdata2.c_str());
										if(colnum2 == 4)
											pPart->successorPartID = atoi(sdata2.c_str());
										if(colnum2 == 5)
											pPart->launchCount = atoi(sdata2.c_str());
										if(colnum2 == 6)
											Strcpy(pPart->inventoryLineMDL,sdata2.c_str());
										if(colnum2 == 7)
											pProbe->launcherDef.price = atoi(sdata2.c_str());
										if(colnum2 == 8)
											pProbe->launcherDef.timeToBuild = atoi(sdata2.c_str());
										if(colnum2 == 9)
											Strcpy(pProbe->launcherDef.modelName,sdata2.c_str());
										if(colnum2 == 10)
											Strcpy(pProbe->launcherDef.iconName,sdata2.c_str());
										if(colnum2 == 11)
											Strcpy(pProbe->launcherDef.name,sdata2.c_str());
										if(colnum2 == 12)
											Strcpy(pProbe->launcherDef.description,sdata2.c_str());
										if(colnum2 == 13)
											pProbe->launcherDef.groupID = atoi(sdata2.c_str());
										if(colnum2 == 14)
											pMission->TechsListToBits(sdata2.c_str(),pProbe->launcherDef.ttbmRequired);
										if(colnum2 == 15)
											pMission->TechsListToBits(sdata2.c_str(),pProbe->launcherDef.ttbmEffects);
									}
								}
							}
						}
					}
				} else {
					if(colnum == 2)
						pProbe->scannerRange = atof(sdata.c_str());
					if(colnum == 3)
						pProbe->dtimeBurst = atof(sdata.c_str());
					if(colnum == 4)
						pProbe->dispersion = atof(sdata.c_str());
					if(colnum == 5)
						pProbe->accuracy = atof(sdata.c_str());
					if(colnum == 6)
						pProbe->ammo = atoi(sdata.c_str());
					if(colnum == 7)
						pProbe->projectileTypeID = atoi(sdata.c_str());
					if(colnum == 8)
						pProbe->ambientSound = atoi(sdata.c_str());
					if(colnum == 9)
						pProbe->dtRipcord = atof(sdata.c_str());
					if(colnum == 10)
						pProbe->loadTime = atof(sdata.c_str());					
					if(colnum == 11)
						pProbe->lifespan = atof(sdata.c_str());
					if(colnum == 12)
						pProbe->signature = atof(sdata.c_str());
					if(colnum == 13)
						pProbe->hitPoints = atof(sdata.c_str());
					if(colnum == 14)
						pProbe->defenseType = ZString(sdata.c_str()).GetToken().GetInteger(); 
					if(colnum == 15)
						pProbe->eabmCapabilities = AbilitiesListToMask(sdata.c_str(),AT_Expendable);
					if(colnum == 16)
						Strcpy(pProbe->iconName,sdata.c_str());
					if(colnum == 17) {
						ZString strColor = ZString(sdata.c_str());
						pProbe->color.r = strColor.GetToken().GetInteger();
						pProbe->color.g = strColor.GetToken().GetInteger();
						pProbe->color.b = strColor.GetToken().GetInteger();
						pProbe->color.a = strColor.GetToken().GetInteger();
					}
					if(colnum == 18)
						pProbe->radius = atof(sdata.c_str());
					if(colnum == 19)
						pProbe->rotation = atof(sdata.c_str());
					if(colnum == 20)
						Strcpy(pProbe->modelName,sdata.c_str());
					if(colnum == 21)
						Strcpy(pProbe->textureName,sdata.c_str());
					if(colnum == 22)
						pProbe->launcherDef.signature = atof(sdata.c_str());
					if(colnum == 23)
						pProbe->launcherDef.mass = atof(sdata.c_str());
					if(colnum==24)
						pProbe->launcherDef.partMask = pMission->PartMaskFromToken(sdata.c_str(),ET_Dispenser);
					if(colnum==25)
						pProbe->launcherDef.expendableSize = atoi(sdata.c_str());
				}
			}
			//NYI handle this  (the asserts were commented out anyways)
			if (pPart->partID ==0)
				debugf("ImportStaticIGCObjs: non-launchable probe/tower (for drone use only) %i\n",pProbe->expendabletypeID);

			*((ObjectType*)(pData + iDatasize)) = OT_probeType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pProbe, size);
			
			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load chaff expendable
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\ExpendableChaff.csv";
	std::fstream file9((PCC)strFile, std::ios::in);
	assert(file9.is_open());
	csvData.clear();
	readCSV(file9, csvData);
	iLines = 0; id = 0;
	size = sizeof(DataChaffTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataChaffTypeIGC* pChaff = (DataChaffTypeIGC*)(new char [size]);
			DataLauncherTypeIGC * pPart = (DataLauncherTypeIGC*)(new char [sizeof(DataLauncherTypeIGC)]);
			pPart->partID = 0;
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					id = pChaff->expendabletypeID = atoi(sdata.c_str());
					for(csvVector::iterator m = csvData4.begin(); m != csvData4.end(); ++m)
					{
						if (m != csvData4.begin()) {
							int id2 = 0; int colnum2 = 0;
							for(std::vector<std::string>::iterator n = m->begin(); n != m->end(); ++n)
							{
								colnum2++;
								std::string sdata2 = *n;
								if(n == m->begin()) {
									id2 = atoi(sdata2.c_str());
									if (id == id2)
										pPart->expendabletypeID = id2;
									else 
										break;
								} else {
									if (id == id2) {
										if(colnum2 == 2)
											pPart->partID = atoi(sdata2.c_str());
										if(colnum2 == 3)
											pPart->amount = atoi(sdata2.c_str());
										if(colnum2 == 4)
											pPart->successorPartID = atoi(sdata2.c_str());
										if(colnum2 == 5)
											pPart->launchCount = atoi(sdata2.c_str());
										if(colnum2 == 6)
											Strcpy(pPart->inventoryLineMDL,sdata2.c_str());
										if(colnum2 == 7)
											pChaff->launcherDef.price = atoi(sdata2.c_str());
										if(colnum2 == 8)
											pChaff->launcherDef.timeToBuild = atoi(sdata2.c_str());
										if(colnum2 == 9)
											Strcpy(pChaff->launcherDef.modelName,sdata2.c_str());
										if(colnum2 == 10)
											Strcpy(pChaff->launcherDef.iconName,sdata2.c_str());
										if(colnum2 == 11)
											Strcpy(pChaff->launcherDef.name,sdata2.c_str());
										if(colnum2 == 12)
											Strcpy(pChaff->launcherDef.description,sdata2.c_str());
										if(colnum2 == 13)
											pChaff->launcherDef.groupID = atoi(sdata2.c_str());
										if(colnum2 == 14)
											pMission->TechsListToBits(sdata2.c_str(),pChaff->launcherDef.ttbmRequired);
										if(colnum2 == 15)
											pMission->TechsListToBits(sdata2.c_str(),pChaff->launcherDef.ttbmEffects);
									} else {
										break;
									}
								}
							}
						}
					}
				} else {
					if(colnum == 2)
						pChaff->chaffStrength = atof(sdata.c_str());
					if(colnum == 3)
						pChaff->loadTime = atof(sdata.c_str());
					if(colnum == 4)
						pChaff->lifespan = atof(sdata.c_str());
					if(colnum == 5)
						pChaff->signature = atof(sdata.c_str());
					if(colnum == 6)
						pChaff->hitPoints = atof(sdata.c_str());
					if(colnum == 7)
						pChaff->defenseType = ZString(sdata.c_str()).GetToken().GetInteger(); 
					if(colnum == 8)
						pChaff->eabmCapabilities = AbilitiesListToMask(sdata.c_str(),AT_Expendable);
					if(colnum == 9)
						Strcpy(pChaff->iconName,sdata.c_str());
					if(colnum == 10) {
						ZString strColor = ZString(sdata.c_str());
						pChaff->color.r = strColor.GetToken().GetInteger();
						pChaff->color.g = strColor.GetToken().GetInteger();
						pChaff->color.b = strColor.GetToken().GetInteger();
						pChaff->color.a = strColor.GetToken().GetInteger();
					}
					if(colnum == 11)
						pChaff->radius = atof(sdata.c_str());
					if(colnum == 12)
						pChaff->rotation = atof(sdata.c_str());
					if(colnum == 13)
						Strcpy(pChaff->modelName,sdata.c_str());
					if(colnum == 14)
						Strcpy(pChaff->textureName,sdata.c_str());
					if(colnum == 15)
						pChaff->launcherDef.signature = atof(sdata.c_str());
					if(colnum == 16)
						pChaff->launcherDef.mass = atof(sdata.c_str());
					if(colnum==17)
						pChaff->launcherDef.partMask = pMission->PartMaskFromToken(sdata.c_str(),ET_ChaffLauncher);
					if(colnum==18)
						pChaff->launcherDef.expendableSize = atoi(sdata.c_str());

				}
			}

			//NYI handle this
			if (pPart->partID ==0)
				debugf("ImportStaticIGCObjs: non-launchable chaff (for drone use only) %i\n",pChaff->expendabletypeID);


			*((ObjectType*)(pData + iDatasize)) = OT_chaffType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pChaff, size);
			
			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load mines expendable
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\ExpendableMines.csv";
	std::fstream file17((PCC)strFile, std::ios::in);
	assert(file17.is_open());
	csvData.clear();
	readCSV(file17, csvData);
	iLines = 0; id = 0;
	size = sizeof(DataMineTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataMineTypeIGC* pMine = (DataMineTypeIGC*)(new char [size]);
			DataLauncherTypeIGC * pPart = (DataLauncherTypeIGC*)(new char [sizeof(DataLauncherTypeIGC)]);
			pPart->partID = 0;

			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					id = pMine->expendabletypeID = atoi(sdata.c_str());
					for(csvVector::iterator m = csvData4.begin(); m != csvData4.end(); ++m)
					{
						if (m != csvData4.begin()) {
							int id2 = 0; int colnum2 = 0;
							for(std::vector<std::string>::iterator n = m->begin(); n != m->end(); ++n)
							{
								colnum2++;
								std::string sdata2 = *n;
								if(n == m->begin()) {
									id2 = atoi(sdata2.c_str());
									if (id == id2)
										pPart->expendabletypeID = id2;
									else 
										break;
								} else {
									if (id == id2) {
										if(colnum2 == 2)
											pPart->partID = atoi(sdata2.c_str());
										if(colnum2 == 3)
											pPart->amount = atoi(sdata2.c_str());
										if(colnum2 == 4)
											pPart->successorPartID = atoi(sdata2.c_str());
										if(colnum2 == 5)
											pPart->launchCount = atoi(sdata2.c_str());
										if(colnum2 == 6)
											Strcpy(pPart->inventoryLineMDL,sdata2.c_str());
										if(colnum2 == 7)
											pMine->launcherDef.price = atoi(sdata2.c_str());
										if(colnum2 == 8)
											pMine->launcherDef.timeToBuild = atoi(sdata2.c_str());
										if(colnum2 == 9)
											Strcpy(pMine->launcherDef.modelName,sdata2.c_str());
										if(colnum2 == 10)
											Strcpy(pMine->launcherDef.iconName,sdata2.c_str());
										if(colnum2 == 11)
											Strcpy(pMine->launcherDef.name,sdata2.c_str());
										if(colnum2 == 12)
											Strcpy(pMine->launcherDef.description,sdata2.c_str());
										if(colnum2 == 13)
											pMine->launcherDef.groupID = atoi(sdata2.c_str());
										if(colnum2 == 14)
											pMission->TechsListToBits(sdata2.c_str(),pMine->launcherDef.ttbmRequired);
										if(colnum2 == 15)
											pMission->TechsListToBits(sdata2.c_str(),pMine->launcherDef.ttbmEffects);
									} else {
										break;
									}
								}
							}
						}
					}
				} else {
					if(colnum == 2)
						pMine->radius= atof(sdata.c_str());
					if(colnum == 3)
						pMine->power = atof(sdata.c_str());
					if(colnum == 4)
						pMine->endurance = atof(sdata.c_str());
					if(colnum == 5)
						pMine->damageType = ZString(sdata.c_str()).GetToken().GetInteger(); 
					if(colnum == 6)
						pMine->loadTime = atof(sdata.c_str());
					if(colnum == 7)
						pMine->lifespan = atof(sdata.c_str());
					if(colnum == 8)
						pMine->signature = atof(sdata.c_str());
					if(colnum == 9)
						pMine->hitPoints = atof(sdata.c_str());
					if(colnum == 10)
						pMine->defenseType = ZString(sdata.c_str()).GetToken().GetInteger(); 
					if(colnum == 11)
						pMine->eabmCapabilities = AbilitiesListToMask(sdata.c_str(),AT_Expendable);
					if(colnum == 12)
						Strcpy(pMine->iconName,sdata.c_str());
					if(colnum == 13) {
						ZString strColor = ZString(sdata.c_str());
						pMine->color.r = strColor.GetToken().GetInteger();
						pMine->color.g = strColor.GetToken().GetInteger();
						pMine->color.b = strColor.GetToken().GetInteger();
						pMine->color.a = strColor.GetToken().GetInteger();
					}
					//there can only be one radius - MineType Data or Object Data...
					//if(colnum == 14)
					//	pMine->radius = atof(sdata.c_str());
					if(colnum == 15)
						pMine->rotation = atof(sdata.c_str());
					if(colnum == 16)
						Strcpy(pMine->modelName,sdata.c_str());
					if(colnum == 17)
						Strcpy(pMine->textureName,sdata.c_str());
					if(colnum == 18)
						pMine->launcherDef.signature = atof(sdata.c_str());
					if(colnum == 19)
						pMine->launcherDef.mass = atof(sdata.c_str());
					if(colnum==20)
						pMine->launcherDef.partMask = pMission->PartMaskFromToken(sdata.c_str(),ET_Dispenser);
					if(colnum==21)
						pMine->launcherDef.expendableSize = atoi(sdata.c_str());
				}
			}
			//NYI handle this
			if (pPart->partID ==0)
				debugf("ImportStaticIGCObjs: non-launchable mine (for drone use only) %i\n",pMine->expendabletypeID);

			*((ObjectType*)(pData + iDatasize)) = OT_mineType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pMine, size);
			
			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load afterburner part types
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Afterburners.csv";
	std::fstream file7((PCC)strFile, std::ios::in);
	assert(file7.is_open());
	csvData.clear();
	readCSV(file7, csvData);
	iLines = 0;
	size = sizeof(DataAfterburnerTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataAfterburnerTypeIGC* pPart = (DataAfterburnerTypeIGC*)(new char [size]);
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					pPart->partID = atoi(sdata.c_str());
				} else {
					if(colnum == 2)
						pPart->fuelConsumption = atof(sdata.c_str());
					if(colnum == 3)
						pPart->maxThrust = atof(sdata.c_str());
					if(colnum == 4)
						pPart->onRate = atof(sdata.c_str());
					if(colnum == 5)
						pPart->offRate = atof(sdata.c_str());
					if(colnum == 6)
						pPart->interiorSound = atoi(sdata.c_str());
					if(colnum == 7)
						pPart->exteriorSound = atoi(sdata.c_str());
					if(colnum == 8)
						pPart->mass = atof(sdata.c_str());
					if(colnum == 9)
						pPart->signature = atof(sdata.c_str());
					if(colnum == 10)
						pPart->successorPartID = atoi(sdata.c_str());
					if(colnum == 11)
						pPart->partMask = pMission->PartMaskFromToken(sdata.c_str(),ET_Afterburner); //
					if(colnum == 12)
						Strcpy(pPart->inventoryLineMDL,sdata.c_str());
					if(colnum == 13)
						pPart->price = atoi(sdata.c_str());
					if(colnum == 14)
						pPart->timeToBuild = atoi(sdata.c_str());
					if(colnum == 15)
						Strcpy(pPart->modelName,sdata.c_str());
					if(colnum == 16)
						Strcpy(pPart->iconName,sdata.c_str());
					if(colnum == 17)
						Strcpy(pPart->name,sdata.c_str());
					if(colnum == 18)
						Strcpy(pPart->description,sdata.c_str());
					if(colnum == 19)
						pPart->groupID = atoi(sdata.c_str());
					if(colnum == 20)
						pMission->TechsListToBits(sdata.c_str(),pPart->ttbmRequired);
					if(colnum == 21)
						pMission->TechsListToBits(sdata.c_str(),pPart->ttbmEffects);
				}
			}
			pPart->equipmentType = ET_Afterburner;
			*((ObjectType*)(pData + iDatasize)) = OT_partType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pPart, size);
			
			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load weapon part types
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Weapons.csv";
	std::fstream file11((PCC)strFile, std::ios::in);
	assert(file11.is_open());
	csvData.clear();
	readCSV(file11, csvData);
	iLines = 0;
	size = sizeof(DataWeaponTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataWeaponTypeIGC* pPart = (DataWeaponTypeIGC*)(new char [size]);
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					pPart->partID = atoi(sdata.c_str());
				} else {
					if(colnum == 2)
						pPart->dtimeReady = atof(sdata.c_str());
					if(colnum == 3)
						pPart->dtimeBurst = atof(sdata.c_str());
					if(colnum == 4)
						pPart->energyPerShot = atof(sdata.c_str());
					if(colnum == 5)
						pPart->dispersion = atof(sdata.c_str());
					if(colnum == 6)
						pPart->cAmmoPerShot = atof(sdata.c_str());
					if(colnum == 7)
						pPart->projectileTypeID = atoi(sdata.c_str());
					if(colnum == 8)
						pPart->activateSound = atoi(sdata.c_str());
					if(colnum == 9)
						pPart->singleShotSound = atoi(sdata.c_str());
					if(colnum == 10)
						pPart->burstSound = atoi(sdata.c_str());
					if(colnum == 11)
						pPart->mass = atof(sdata.c_str());
					if(colnum == 12)
						pPart->signature = atof(sdata.c_str());
					if(colnum == 13)
						pPart->successorPartID = atoi(sdata.c_str());
					if(colnum == 14)
						pPart->partMask = pMission->PartMaskFromToken(sdata.c_str(),ET_Weapon);
					if(colnum == 15)
						Strcpy(pPart->inventoryLineMDL,sdata.c_str());
					if(colnum == 16)
						pPart->price = atoi(sdata.c_str());
					if(colnum == 17)
						pPart->timeToBuild = atoi(sdata.c_str());
					if(colnum == 18)
						Strcpy(pPart->modelName,sdata.c_str());
					if(colnum == 19)
						Strcpy(pPart->iconName,sdata.c_str());
					if(colnum == 20)
						Strcpy(pPart->name,sdata.c_str());
					if(colnum == 21)
						Strcpy(pPart->description,sdata.c_str());
					if(colnum == 22)
						pPart->groupID = atoi(sdata.c_str());
					if(colnum == 23)
						pMission->TechsListToBits(sdata.c_str(),pPart->ttbmRequired);
					if(colnum == 24)
						pMission->TechsListToBits(sdata.c_str(),pPart->ttbmEffects);
				}
			}
			pPart->equipmentType = ET_Weapon;
			*((ObjectType*)(pData + iDatasize)) = OT_partType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pPart, size);
			
			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load pack types
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Packs.csv";
	std::fstream packfile((PCC)strFile, std::ios::in);
	assert(packfile.is_open());
	csvData.clear();
	readCSV(packfile, csvData);
	iLines = 0;
	size = sizeof(DataPackTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataPackTypeIGC* pPack = (DataPackTypeIGC*)(new char [size]);
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					pPack->partID = atoi(sdata.c_str());
				} else {
					if(colnum == 2)
						pPack->packType = atoi(sdata.c_str());
					if(colnum == 3)
						pPack->amount = atoi(sdata.c_str());
					if(colnum == 4)
						pPack->mass = atof(sdata.c_str());
					if(colnum == 5)
						pPack->signature = atof(sdata.c_str());
					if(colnum == 6)
						pPack->successorPartID = atoi(sdata.c_str());
					if(colnum == 7)
						pPack->partMask = pMission->PartMaskFromToken(sdata.c_str(),ET_Cloak);
					if(colnum == 8)
						Strcpy(pPack->inventoryLineMDL,sdata.c_str());
					if(colnum == 9)
						pPack->price = atoi(sdata.c_str());
					if(colnum == 10)
						pPack->timeToBuild = atof(sdata.c_str());
					if(colnum == 11)
						Strcpy(pPack->modelName,sdata.c_str());
					if(colnum == 12)
						Strcpy(pPack->iconName,sdata.c_str());
					if(colnum == 13)
						Strcpy(pPack->name,sdata.c_str());
					if(colnum == 14)
						Strcpy(pPack->description,sdata.c_str());
					if(colnum == 15)
						pPack->groupID = atoi(sdata.c_str());
					if(colnum == 16)
						pMission->TechsListToBits(sdata.c_str(),pPack->ttbmRequired);
					if(colnum == 17)
						pMission->TechsListToBits(sdata.c_str(),pPack->ttbmEffects);

				}
			}
			pPack->equipmentType = ET_Pack;
			*((ObjectType*)(pData + iDatasize)) = OT_partType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pPack, size);

			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load shield types
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Shields.csv";
	std::fstream shieldfile((PCC)strFile, std::ios::in);
	assert(shieldfile.is_open());
	csvData.clear();
	readCSV(shieldfile, csvData);
	iLines = 0;
	size = sizeof(DataShieldTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataShieldTypeIGC* pShield = (DataShieldTypeIGC*)(new char [size]);
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					pShield->partID = atoi(sdata.c_str());
				} else {
					if(colnum == 2)
						pShield->rateRegen = atof(sdata.c_str());
					if(colnum == 3)
						pShield->maxStrength = atof(sdata.c_str());
					if(colnum == 4)
						pShield->defenseType = ZString(sdata.c_str()).GetToken().GetInteger(); ;
					if(colnum == 5)
						pShield->activateSound = atoi(sdata.c_str());
					if(colnum == 6)
						pShield->deactivateSound = atoi(sdata.c_str());
					if(colnum == 7)
						pShield->mass = atof(sdata.c_str());
					if(colnum == 8)
						pShield->signature = atof(sdata.c_str());
					if(colnum == 9)
						pShield->successorPartID = atoi(sdata.c_str());
					if(colnum == 10)
						pShield->partMask = pMission->PartMaskFromToken(sdata.c_str(),ET_Shield);
					if(colnum == 11)
						Strcpy(pShield->inventoryLineMDL,sdata.c_str());
					if(colnum == 12)
						pShield->price = atoi(sdata.c_str());
					if(colnum == 13)
						pShield->timeToBuild = atof(sdata.c_str());
					if(colnum == 14)
						Strcpy(pShield->modelName,sdata.c_str());
					if(colnum == 15)
						Strcpy(pShield->iconName,sdata.c_str());
					if(colnum == 16)
						Strcpy(pShield->name,sdata.c_str());
					if(colnum == 17)
						Strcpy(pShield->description,sdata.c_str());
					if(colnum == 18)
						pShield->groupID = atoi(sdata.c_str());
					if(colnum == 19)
						pMission->TechsListToBits(sdata.c_str(),pShield->ttbmRequired);
					if(colnum == 20)
						pMission->TechsListToBits(sdata.c_str(),pShield->ttbmEffects);
				}
			}
			pShield->equipmentType = ET_Shield;
			*((ObjectType*)(pData + iDatasize)) = OT_partType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pShield, size);

			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load cloaks types
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Cloaks.csv";
	std::fstream cloakfile((PCC)strFile, std::ios::in);
	assert(cloakfile.is_open());
	csvData.clear();
	readCSV(cloakfile, csvData);
	iLines = 0;
	size = sizeof(DataCloakTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataCloakTypeIGC* pCloak = (DataCloakTypeIGC*)(new char [size]);
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					pCloak->partID = atoi(sdata.c_str());
				} else {
					if(colnum == 2)
						pCloak->energyConsumption = atof(sdata.c_str());
					if(colnum == 3)
						pCloak->maxCloaking = atof(sdata.c_str());
					if(colnum == 4)
						pCloak->onRate = atof(sdata.c_str());
					if(colnum == 5)
						pCloak->offRate = atof(sdata.c_str());
					if(colnum == 6)
						pCloak->engageSound = atoi(sdata.c_str());
					if(colnum == 7)
						pCloak->disengageSound = atoi(sdata.c_str());
					if(colnum == 8)
						pCloak->mass = atof(sdata.c_str());
					if(colnum == 9)
						pCloak->signature = atof(sdata.c_str());
					if(colnum == 10)
						pCloak->successorPartID = atoi(sdata.c_str());
					if(colnum == 11)
						pCloak->partMask = pMission->PartMaskFromToken(sdata.c_str(),ET_Cloak);
					if(colnum == 12)
						Strcpy(pCloak->inventoryLineMDL,sdata.c_str());
					if(colnum == 13)
						pCloak->price = atoi(sdata.c_str());
					if(colnum == 14)
						pCloak->timeToBuild = atof(sdata.c_str());
					if(colnum == 15)
						Strcpy(pCloak->modelName,sdata.c_str());
					if(colnum == 16)
						Strcpy(pCloak->iconName,sdata.c_str());
					if(colnum == 17)
						Strcpy(pCloak->name,sdata.c_str());
					if(colnum == 18)
						Strcpy(pCloak->description,sdata.c_str());
					if(colnum == 19)
						pCloak->groupID = atoi(sdata.c_str());
					if(colnum == 20)
						pMission->TechsListToBits(sdata.c_str(),pCloak->ttbmRequired);
					if(colnum == 21)
						pMission->TechsListToBits(sdata.c_str(),pCloak->ttbmEffects);
				}
			}
			pCloak->equipmentType = ET_Cloak;
			*((ObjectType*)(pData + iDatasize)) = OT_partType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pCloak, size);

			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load expendable parts (launcher)
	csvData.clear();
	size = sizeof(DataLauncherTypeIGC);
	iLines = 0; id = 0;
	for(csvVector::iterator m = csvData4.begin(); m != csvData4.end(); ++m)
	{
		if (m != csvData4.begin()) {
			int colnum2 = 0;
			DataLauncherTypeIGC * pPart = (DataLauncherTypeIGC*)(new char[size]);
			for(std::vector<std::string>::iterator n = m->begin(); n != m->end(); ++n)
			{
				colnum2++;
				std::string sdata2 = *n;
				if(n == m->begin()) {
					pPart->expendabletypeID = atoi(sdata2.c_str());
				} else {
					if(colnum2 == 2)
						pPart->partID = atoi(sdata2.c_str());
					if(colnum2 == 3)
						pPart->amount = atoi(sdata2.c_str());
					if(colnum2 == 4)
						pPart->successorPartID = atoi(sdata2.c_str());
					if(colnum2 == 5)
						pPart->launchCount = atoi(sdata2.c_str());
					if(colnum2 == 6)
						Strcpy(pPart->inventoryLineMDL,sdata2.c_str());
				}
			}

			*((ObjectType*)(pData + iDatasize)) = OT_partType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pPart, size);

			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}



	// Load hull types & hardpoints
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\TypesOfShips.csv";
	std::fstream file5((PCC)strFile, std::ios::in);
	assert(file5.is_open());
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Hardpoints.csv";
	std::fstream file6((PCC)strFile, std::ios::in);
	assert(file6.is_open());
	csvVector csvData2;
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\ShipsSFX.csv";
	std::fstream file8((PCC)strFile, std::ios::in);
	assert(file8.is_open());
	csvVector csvData3;

	csvData.clear();
	readCSV(file5, csvData);
	readCSV(file6, csvData2);
	readCSV(file8, csvData3);
	iLines = 0;
	size = sizeof(DataHullTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataHullTypeIGC* pHull = (DataHullTypeIGC*)(new char [sizeof(DataHullTypeIGC) + (sizeof(HardpointData) * c_maxMountedWeapons)]); //
			int id = 0; int colnum = 0; 
			int iHardpoints = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) { // has sfx
					id = pHull->hullID = atoi(sdata.c_str());
					for(csvVector::iterator m = csvData3.begin(); m != csvData3.end(); ++m)
					{
						if (m != csvData3.begin()) {
							int id2 = 0; int colnum2 = 0;
							for(std::vector<std::string>::iterator n = m->begin(); n != m->end(); ++n)
							{
								colnum2++;
								std::string sdata2 = *n;
								if(n == m->begin()) {
									id2 = atoi(sdata2.c_str());
								} else {
									if (id == id2) {
										if(colnum2 == 2)
											pHull->interiorSound = atoi(sdata2.c_str());
										if(colnum2 == 3)
											pHull->exteriorSound = atoi(sdata2.c_str());
										if(colnum2 == 4)
											pHull->mainThrusterInteriorSound = atoi(sdata2.c_str());
										if(colnum2 == 5)
											pHull->mainThrusterExteriorSound = atoi(sdata2.c_str());
										if(colnum2 == 6)
											pHull->manuveringThrusterInteriorSound = atoi(sdata2.c_str());
										if(colnum2 == 7)
											pHull->manuveringThrusterExteriorSound = atoi(sdata2.c_str());
									}
								}
							}
						}
					}
				} else {
					if(colnum == 2)
						pHull->mass = atof(sdata.c_str());
					if(colnum == 3)
						pHull->signature = atof(sdata.c_str());
					if(colnum == 4)
						pHull->speed = atof(sdata.c_str());
					if(colnum == 5) {	
						ZString strMaxTurnRates = ZString(sdata.c_str());
						for (int i = 0; (i < 3); i++)
							pHull->maxTurnRates[i] = atof(strMaxTurnRates.GetToken());
					}
					if(colnum == 6) {	
						ZString strTurnTorques = ZString(sdata.c_str());
						for (int i = 0; (i < 3); i++)
							pHull->turnTorques[i] = atof(strTurnTorques.GetToken());
					}
					if(colnum == 7)	
						pHull->thrust = atof(sdata.c_str());
					if(colnum == 8)
						pHull->sideMultiplier = atof(sdata.c_str());
					if(colnum == 9)
						pHull->backMultiplier = atof(sdata.c_str());
					if(colnum == 10)
						pHull->scannerRange = atof(sdata.c_str());
					if(colnum == 11)
						pHull->maxFuel = atof(sdata.c_str());
					if(colnum == 12)
						pHull->ecm = atof(sdata.c_str());
					if(colnum == 13)
						pHull->length = atof(sdata.c_str());
					if(colnum == 14)
						pHull->maxEnergy = atof(sdata.c_str());
					if(colnum == 15)
						pHull->rechargeRate = atof(sdata.c_str());
					if(colnum == 16)
						pHull->ripcordSpeed = atof(sdata.c_str());
					if(colnum == 17)
						pHull->ripcordCost = atof(sdata.c_str());
					if(colnum == 18)
						pHull->maxAmmo = atoi(sdata.c_str());
					if(colnum == 19)
						pHull->successorHullID = atoi(sdata.c_str());
					if(colnum == 20) {
						pHull->maxWeapons = atoi(sdata.c_str());
						if (pHull->maxWeapons != 0) { //has hard points
							HardpointData * phpd = (HardpointData*)((char*)pHull + size);
							for(csvVector::iterator k = csvData2.begin(); k != csvData2.end(); ++k)
							{
								if (k != csvData2.begin()) {
									int id2 = 0; int colnum2 = 0;
									for(std::vector<std::string>::iterator l = k->begin(); l != k->end(); ++l)
									{
										colnum2++;
										std::string sdata2 = *l;
										if(l == k->begin()) {
											id2 = atoi(sdata2.c_str());
											if (id == id2)
												iHardpoints++;
										} else {
											if (id == id2) {
												if(colnum2 == 2)
													phpd->interiorSound = atoi(sdata2.c_str());
												if(colnum2 == 3)
													phpd->turnSound = atoi(sdata2.c_str());
												if(colnum2 == 4)
													Strcpy(phpd->frameName,sdata2.c_str());
												if(colnum2 == 5)
													Strcpy(phpd->locationAbreviation,sdata2.c_str());
												if(colnum2 == 6)
													phpd->partMask = pMission->PartsListToMask(sdata2.c_str(),ET_Weapon);
												if(colnum2 == 7)
													phpd->bFixed = atoi(sdata2.c_str());
											}
										}
									}
									if ( (id == id2) && (k != csvData2.end()) )
										phpd++;
								}
							}
						}
					}
					if(colnum == 21)
						pHull->maxFixedWeapons = atoi(sdata.c_str());
					if(colnum == 22)
						pHull->hitPoints = atof(sdata.c_str());
					if(colnum == 23)		
						pHull->defenseType = ZString(sdata.c_str()).GetToken().GetInteger();
					if(colnum == 24)
						pHull->capacityMagazine = atoi(sdata.c_str());
					if(colnum == 25)
						pHull->capacityDispenser = atoi(sdata.c_str());
					if(colnum == 26)
						pHull->capacityChaffLauncher = atoi(sdata.c_str());
					if(colnum == 27) {
						ZString strParts = ZString(sdata.c_str());
						for (int i = 0; (i < c_cMaxPreferredPartTypes); i++)
							pHull->preferredPartsTypes[i] = atoi(strParts.GetToken());
					}
					if(colnum == 28)
						pHull->habmCapabilities = AbilitiesListToMask(sdata.c_str(),AT_Hull);
					if(colnum == 29)
						Strcpy(pHull->textureName,sdata.c_str());
					if(colnum == 30)
						pHull->pmEquipment[ET_ChaffLauncher] = pMission->PartsListToMask(sdata.c_str(),ET_ChaffLauncher);
					if(colnum == 31)
						pHull->pmEquipment[ET_Magazine] = pMission->PartsListToMask(sdata.c_str(),ET_Magazine);
					if(colnum == 32)
						pHull->pmEquipment[ET_Dispenser] = pMission->PartsListToMask(sdata.c_str(),ET_Dispenser);
					if(colnum == 33)
						pHull->pmEquipment[ET_Shield] = pMission->PartsListToMask(sdata.c_str(),ET_Shield);
					if(colnum == 34)
						pHull->pmEquipment[ET_Cloak] = pMission->PartsListToMask(sdata.c_str(),ET_Cloak);
					if(colnum == 35)
						pHull->pmEquipment[ET_Afterburner] = pMission->PartsListToMask(sdata.c_str(),ET_Afterburner);
					if(colnum == 36)
						pHull->price = atoi(sdata.c_str());
					if(colnum == 37)
						pHull->timeToBuild = atoi(sdata.c_str());
					if(colnum == 38)
						Strcpy(pHull->modelName,sdata.c_str());
					if(colnum == 39)
						Strcpy(pHull->iconName,sdata.c_str());
					if(colnum == 40)
						Strcpy(pHull->name,sdata.c_str());
					if(colnum == 41)
						Strcpy(pHull->description,sdata.c_str());
					if(colnum == 42)
						pHull->groupID = atoi(sdata.c_str());
					if(colnum == 43)
						pMission->TechsListToBits(sdata.c_str(),pHull->ttbmRequired);
					if(colnum == 44)
						pMission->TechsListToBits(sdata.c_str(),pHull->ttbmEffects);
				}
			}
			pHull->hardpointOffset = sizeof(DataHullTypeIGC);
			pHull->pmEquipment[ET_Weapon] = 0; //used in hardpoint
			pHull->pmEquipment[ET_Pack] = 0; //NYI
						
			*((ObjectType*)(pData + iDatasize)) = OT_hullType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size + (sizeof(HardpointData) * iHardpoints);
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pHull, size + (sizeof(HardpointData) * iHardpoints));
			
			iDatasize += sizeof(int) + sizeof(ObjectType) + size + (sizeof(HardpointData) * iHardpoints);
			iLines++;
		}
	}


	// Load gas
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\GlobalAttributes.csv";
	std::fstream file16((PCC)strFile, std::ios::in);
	assert(file16.is_open());
	csvVector csvData6;
	readCSV(file16, csvData6);

	//load developments
	ZString wstr2File = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Developments.csv";
	std::fstream devfile((PCC)wstr2File, std::ios::in);
	assert(devfile.is_open());
	csvData.clear();
	readCSV(devfile, csvData);
	iLines = 0;
	size = sizeof(DataDevelopmentIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			float fgas[c_gaMax] = {-1.0f};
			DataDevelopmentIGC* pDev = (DataDevelopmentIGC*)(new char [size]);
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					pDev->developmentID = atoi(sdata.c_str());
					
					for(csvVector::iterator m = csvData6.begin(); m != csvData6.end(); ++m)
					{
						if (m != csvData6.begin()) {
							int id2 = 0; int colnum2 = 0; int gaid = 0;
							for(std::vector<std::string>::iterator n = m->begin(); n != m->end(); ++n)
							{
								colnum2++;
								std::string sdata2 = *n;
								if(n == m->begin()) {
									id2 = atoi(sdata2.c_str());
								} else {
									if (pDev->developmentID == id2 && colnum2 != 2) {
										fgas[gaid] = atof(sdata2.c_str());
										gaid++;
									}
								}
							}
						}
					}
				} else {
					if(colnum == 2)
						pDev->completionSound = atoi(sdata.c_str());
					if(colnum == 3)
						pDev->price = atoi(sdata.c_str());
					if(colnum == 4)
						pDev->timeToBuild = atof(sdata.c_str());
					if(colnum == 5)
						Strcpy(pDev->modelName,sdata.c_str());
					if(colnum == 6)
						Strcpy(pDev->iconName,sdata.c_str());
					if(colnum == 7)
						Strcpy(pDev->name,sdata.c_str());
					if(colnum == 8)
						Strcpy(pDev->description,sdata.c_str());					
					if(colnum == 9)
						pDev->groupID = atoi(sdata.c_str());
					if(colnum == 10)
						pMission->TechsListToBits(sdata.c_str(),pDev->ttbmRequired);
					if(colnum == 11)
						pMission->TechsListToBits(sdata.c_str(),pDev->ttbmEffects);
				}
			}
			if(fgas[0] == -1.0f) {
				pDev->gas.Initialize();
			} else {
				pDev->gas.Set(fgas);
			}
			*((ObjectType*)(pData + iDatasize)) = OT_development;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pDev, size);
			if(i != csvData.end())
				iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load drone types
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\TypesOfDrones.csv";
	std::fstream dronefile((PCC)strFile, std::ios::in);
	assert(dronefile.is_open());
	csvData.clear();
	readCSV(dronefile, csvData);
	iLines = 0;
	size = sizeof(DataDroneTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataDroneTypeIGC* pDrone = (DataDroneTypeIGC*)(new char [size]);
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					pDrone->droneTypeID = atoi(sdata.c_str());
				} else {
					if(colnum == 2)
						pDrone->shootSkill = atof(sdata.c_str());
					if(colnum == 3)
						pDrone->moveSkill = atof(sdata.c_str());
					if(colnum == 4)
						pDrone->bravery = atof(sdata.c_str());
					if(colnum == 5)
						pDrone->pilotType = atoi(sdata.c_str()); //add flag names
					if(colnum == 6)
						pDrone->hullTypeID = atoi(sdata.c_str());					
					if(colnum == 7)
						pDrone->etidLaid = atoi(sdata.c_str());
					if(colnum == 8)
						pDrone->price = atoi(sdata.c_str());
					if(colnum == 9)
						pDrone->timeToBuild = atoi(sdata.c_str());
					if(colnum == 10)
						Strcpy(pDrone->modelName,sdata.c_str());
					if(colnum == 11)
						Strcpy(pDrone->iconName,sdata.c_str());
					if(colnum == 12)
						Strcpy(pDrone->name,sdata.c_str());
					if(colnum == 13)
						Strcpy(pDrone->description,sdata.c_str());
					if(colnum == 14)
						pDrone->groupID = atoi(sdata.c_str());
					if(colnum == 15)
						pMission->TechsListToBits(sdata.c_str(),pDrone->ttbmRequired);
					if(colnum == 16)
						pMission->TechsListToBits(sdata.c_str(),pDrone->ttbmEffects);
				}
			}
			*((ObjectType*)(pData + iDatasize)) = OT_droneType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pDrone, size);

			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load station types
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\TypesOfStations.csv";
	std::fstream file3((PCC)strFile, std::ios::in);
	assert(file3.is_open());
	csvData.clear();
	readCSV(file3, csvData);

	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\StationsSFX.csv";
	std::fstream file15((PCC)strFile, std::ios::in);
	assert(file15.is_open());
	csvVector csvData5;
	readCSV(file15, csvData5);
	iLines = 0;
	size = sizeof(DataStationTypeIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataStationTypeIGC* pStation = (DataStationTypeIGC*)(new char [size]);
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					pStation->stationTypeID = atoi(sdata.c_str());
					for(csvVector::iterator m = csvData5.begin(); m != csvData5.end(); ++m)
					{
						if (m != csvData5.begin()) {
							int id2 = 0; int colnum2 = 0;
							for(std::vector<std::string>::iterator n = m->begin(); n != m->end(); ++n)
							{
								colnum2++;
								std::string sdata2 = *n;
								if(n == m->begin()) {
									id2 = atoi(sdata2.c_str());
								} else {
									if (pStation->stationTypeID == id2) {
										if(colnum2 == 2)
											pStation->constructorNeedRockSound = atoi(sdata2.c_str());
										if(colnum2 == 3)
											pStation->constructorUnderAttackSound = atoi(sdata2.c_str());
										if(colnum2 == 4)
											pStation->constructorDestroyedSound = atoi(sdata2.c_str());
										if(colnum2 == 5)
											pStation->completionSound = atoi(sdata2.c_str());
										if(colnum2 == 6)
											pStation->interiorSound = atoi(sdata2.c_str());
										if(colnum2 == 7)
											pStation->exteriorSound = atoi(sdata2.c_str());
										if(colnum2 == 8)
											pStation->interiorAlertSound = atoi(sdata2.c_str());
										if(colnum2 == 9)
											pStation->underAttackSound = atoi(sdata2.c_str());
										if(colnum2 == 10)
											pStation->criticalSound = atoi(sdata2.c_str());
										if(colnum2 == 11)
											pStation->destroyedSound = atoi(sdata2.c_str());
										if(colnum2 == 12)
											pStation->capturedSound = atoi(sdata2.c_str());
										if(colnum2 == 13)
											pStation->enemyCapturedSound = atoi(sdata2.c_str());
										if(colnum2 == 14)
											pStation->enemyDestroyedSound = atoi(sdata2.c_str());
									}
								}
							}
						}
					}
				} else {
					if(colnum == 2)
						pStation->signature = atof(sdata.c_str());
					if(colnum == 3)
						pStation->maxArmorHitPoints = atof(sdata.c_str());
					if(colnum == 4)
						pStation->maxShieldHitPoints = atof(sdata.c_str());
					if(colnum == 5)
						pStation->armorRegeneration = atof(sdata.c_str());
					if(colnum == 6)
						pStation->shieldRegeneration = atof(sdata.c_str());
					if(colnum == 7)
						pStation->scannerRange = atof(sdata.c_str());
					if(colnum == 8)
						pStation->income = atoi(sdata.c_str());
					if(colnum == 9)
						pStation->radius = atof(sdata.c_str());					
					if(colnum == 10)
						pStation->successorStationTypeID = atoi(sdata.c_str());
					if(colnum == 11)
						pStation->defenseTypeArmor = ZString(sdata.c_str()).GetToken().GetInteger(); 
					if(colnum == 12)
						pStation->defenseTypeShield = ZString(sdata.c_str()).GetToken().GetInteger(); 
					if(colnum == 13)
						pStation->sabmCapabilities = AbilitiesListToMask(sdata.c_str(),AT_Station);
					if(colnum == 14)
						pStation->aabmBuild = AbilitiesListToMask(sdata.c_str(),AT_Asteroid);
					if(colnum == 15)
						pStation->classID = atoi(sdata.c_str()); //add flag names
					if(colnum == 16)
						pStation->constructionDroneTypeID = atoi(sdata.c_str());
					if(colnum == 17)
						Strcpy(pStation->textureName,sdata.c_str());
					if(colnum == 18)
						Strcpy(pStation->builderName,sdata.c_str());
					if(colnum == 19)
						pStation->price = atoi(sdata.c_str());
					if(colnum == 20)
						pStation->timeToBuild = atoi(sdata.c_str());
					if(colnum == 21)
						Strcpy(pStation->modelName,sdata.c_str());
					if(colnum == 22)
						Strcpy(pStation->iconName,sdata.c_str());
					if(colnum == 23)
						Strcpy(pStation->name,sdata.c_str());
					if(colnum == 24)
						Strcpy(pStation->description,sdata.c_str());
					if(colnum == 25)
						pStation->groupID = atoi(sdata.c_str());
					if(colnum == 26)
						pMission->TechsListToBits(sdata.c_str(),pStation->ttbmRequired);
					if(colnum == 27)
						pMission->TechsListToBits(sdata.c_str(),pStation->ttbmEffects);
 					if(colnum == 28)
						pMission->TechsListToBits(sdata.c_str(),pStation->ttbmLocal);;
				}
			}

			*((ObjectType*)(pData + iDatasize)) = OT_stationType;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pStation, size);
			
			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}

	// Load treasure set (max of 64 treasures)
	strFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\TypesOfTreasures.csv";
	std::fstream tresfile((PCC)strFile, std::ios::in);
	assert(tresfile.is_open());
	csvData.clear();
	readCSV(tresfile, csvData);
	int iTreasures = 0;
	size = sizeof(DataTreasureSetIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			DataTreasureSetIGC* pSet = (DataTreasureSetIGC*)(new char [size + (sizeof(TreasureData) * 64)]);
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					pSet->treasureSetID = atoi(sdata.c_str());
				} else {
					if(colnum == 2)
						Strcpy(pSet->name,sdata.c_str());
					if(colnum == 3)
						pSet->nTreasureData = atoi(sdata.c_str());
					if(colnum == 4)
						pSet->bZoneOnly = atoi(sdata.c_str());
					if(colnum == 5) {
						if(pSet->nTreasureData != 0) {
							TreasureData* ptd =	(TreasureData*)((char*)pSet + size);
							ZString strTreasures = ZString(sdata.c_str());
							for(int i=0;i <= pSet->nTreasureData;i++){
							
								ptd->treasureID = strTreasures.GetToken().GetInteger();
								if(strTreasures.IsEmpty())
									break;

								ZString strCode = strTreasures.GetToken();
								if(strCode.ReverseFind("Part") == 0)
									ptd->treasureCode = c_tcPart;
								if(strCode.ReverseFind("Powerup") == 0)
									ptd->treasureCode = c_tcPowerup;
								if(strCode.ReverseFind("Development") == 0)
									ptd->treasureCode = c_tcDevelopment;
								if(strCode.ReverseFind("Cash") == 0)
									ptd->treasureCode = c_tcCash;
								if(strCode.ReverseFind("Flag") == 0)
									ptd->treasureCode = c_tcFlag;

								ptd->chance = strTreasures.GetToken().GetInteger();
								ptd++; iTreasures++;
							}
						}
					}
				}
			}

			*((ObjectType*)(pData + iDatasize)) = OT_treasureSet;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size + (sizeof(TreasureData) * iTreasures);
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pSet, size + (sizeof(TreasureData) * iTreasures));

			iDatasize += sizeof(int) + sizeof(ObjectType) + size + (sizeof(TreasureData) * iTreasures);
			iLines++;
		}
	}


	//load civs
	ZString wstrFile = ZString(UTL::artworkPath()) + ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Civilizations.csv";
	std::fstream file2((PCC)wstrFile, std::ios::in);
	assert(file2.is_open());
	csvData.clear();
	readCSV(file2, csvData);
	iLines = 0;
	size = sizeof(DataCivilizationIGC);
	for(csvVector::iterator i = csvData.begin(); i != csvData.end(); ++i)
	{
		if (i != csvData.begin()) {
			float fgas[c_gaMax];
			DataCivilizationIGC* pCiv = (DataCivilizationIGC*)(new char [size]);
			int colnum = 0;
			for(std::vector<std::string>::iterator j = i->begin(); j != i->end(); ++j)
			{
				colnum++;
				std::string sdata = *j;
				if(j == i->begin()) {
					pCiv->civilizationID = atoi(sdata.c_str());
					
					for(csvVector::iterator m = csvData6.begin(); m != csvData6.end(); ++m)
					{
						if (m != csvData6.begin()) {
							int id2 = 0; int colnum2 = 0; int gaid = 0;
							for(std::vector<std::string>::iterator n = m->begin(); n != m->end(); ++n)
							{
								colnum2++;
								std::string sdata2 = *n;
								if(n == m->begin()) {
									id2 = atoi(sdata2.c_str());
								} else {
									if (pCiv->civilizationID == id2 && colnum2 != 2) {
										fgas[gaid] = atof(sdata2.c_str());
										gaid++;
									}
								}
							}
						}
					}
				} else {
					if(colnum == 2)
						pCiv->incomeMoney = atof(sdata.c_str());
					if(colnum == 3)
						pCiv->bonusMoney = atof(sdata.c_str());
					if(colnum == 4)
						Strcpy(pCiv->name,sdata.c_str());
					if(colnum == 5)
						Strcpy(pCiv->iconName,sdata.c_str());
					if(colnum == 6)
						Strcpy(pCiv->hudName,sdata.c_str());
					if(colnum == 7)
						pCiv->lifepod = atoi(sdata.c_str());					
					if(colnum == 8)
						pCiv->initialStationTypeID = atoi(sdata.c_str());
					if(colnum == 9)
						pMission->TechsListToBits(sdata.c_str(),pCiv->ttbmBaseTechs);
					if(colnum == 10)
						pMission->TechsListToBits(sdata.c_str(),pCiv->ttbmNoDevTechs);
				}
			}
			pCiv->gasBaseAttributes.Set(fgas);
			*((ObjectType*)(pData + iDatasize)) = OT_civilization;
			*((int*)(pData + sizeof(ObjectType) + iDatasize)) = size;
			memcpy(pData + iDatasize + sizeof(int) + sizeof(ObjectType), pCiv, size);
			iDatasize += sizeof(int) + sizeof(ObjectType) + size;
			iLines++;
		}
	}


	//the beast is now loaded up with IGC, initialize the objects
    pMission->Import (now, c_maskStaticTypes, pData, iDatasize);
}

// End Imago

//------------------------------------------------------------------------------
bool    DumpIGCFile (const char* name, ImissionIGC* pMission, __int64 iMaskExportTypes, void (*munge)(int size, char* data))
{
    char        szFilename[MAX_PATH + 1];
    HRESULT     hr = UTL::getFile(name, ".igc", szFilename, true, true);
    FILE*       file = fopen (szFilename, "wb");
    if (file)
    {
        DumpIGCFile (file, pMission, iMaskExportTypes, munge);
        fclose (file);
        return true;
    }
    else
        return false;
}

//------------------------------------------------------------------------------
bool    LoadIGCFile (const char* name, ImissionIGC* pMission, void (*munge)(int size, char* data))
{
    char        szFilename[MAX_PATH + 1];
    HRESULT     hr = UTL::getFile(name, ".igc", szFilename, true, true);
    FILE*       file = fopen(szFilename, "rb");
    if (file)
    {
        LoadIGCFile (file, pMission, munge);
        fclose (file);

        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
#define IGC_FILE_VERSION_TYPE   int

//------------------------------------------------------------------------------
bool    DumpIGCStaticCore (const char* name, ImissionIGC* pMission, __int64 iMaskExportTypes, void (*munge)(int size, char* data))
{
    char        szFilename[MAX_PATH + 1];
    HRESULT     hr = UTL::getFile(name, ".igc", szFilename, true, true);
    FILE*       file = fopen (szFilename, "wb");
    if (file)
    {
        IGC_FILE_VERSION_TYPE   iStaticCoreVersion = static_cast<IGC_FILE_VERSION_TYPE> (time (0));
        fwrite (&iStaticCoreVersion, sizeof(iStaticCoreVersion), 1, file);
        DumpIGCFile (file, pMission, iMaskExportTypes, munge);
        fclose (file);
        
        return true;
    }
    else
    {
        assert ("Dump IGC Static Core failed." && false);
        return false;
    }
}

struct  CachedStaticCore
{
    CstaticIGC*             pstatic;
    IGC_FILE_VERSION_TYPE   version;
    int                     consumers;
    char                    name[32]; // was c_cbFileName - needed it longer
};

typedef Slist_utl<CachedStaticCore> CachedList;
typedef Slink_utl<CachedStaticCore> CachedLink;

static CachedList   s_cores;

int     LoadIGCStaticCore (const char* name, ImissionIGC* pMission, bool fGetVersionOnly, void (*munge)(int size, char* data))
{
    //See if we've already loaded the static core
    {
        for (CachedLink*    pcl = s_cores.first(); (pcl != NULL); pcl = pcl->next())
        {
            if (_stricmp(name, pcl->data().name) == 0)
            {
                if (!fGetVersionOnly)
                {
                    pMission->SetStaticCore(pcl->data().pstatic);
                    pcl->data().consumers++;
                }

                return pcl->data().version;
            }
        }
    }

    char        szFilename[MAX_PATH + 1];
    HRESULT     hr = UTL::getFile(name, ".igc", szFilename, true, true);
    FILE*       file = fopen(szFilename, "rb");
    if (file)
    {
        IGC_FILE_VERSION_TYPE   iStaticCoreVersion;
        int                     iReadCount = fread (&iStaticCoreVersion, sizeof(iStaticCoreVersion), 1, file);
        assert (iReadCount == 1);

        if (!fGetVersionOnly)
        {
            pMission->SetStaticCore(NULL);

            LoadIGCFile (file, pMission, munge);

            CachedLink* pcl = new CachedLink;

            pcl->data().pstatic = pMission->GetStaticCore();
            strcpy(pcl->data().name, name);
            pcl->data().version = iStaticCoreVersion;
            pcl->data().consumers = 1;

            s_cores.last(pcl);


#if 0
            {
                debugf("Weapons\n");
                for (PartTypeLinkIGC*   ppl = pMission->GetPartTypes()->first(); (ppl != NULL); ppl = ppl->next())
                {
                    IpartTypeIGC*   ppt = ppl->data();
                    if (ppt->GetEquipmentType() == ET_Weapon)
                    {
                        DataWeaponTypeIGC*  data = (DataWeaponTypeIGC*)ppt->GetData();
                        IprojectileTypeIGC* pt = pMission->GetProjectileType(data->projectileTypeID);
                        float   dps = (pt->GetBlastPower() + pt->GetPower()) / data->dtimeBurst;

                        debugf("Name: %s\n", ppt->GetName());
                        debugf("\tShots/second\t%f\n", (1.0f / data->dtimeBurst));
                        debugf("\tammo/second\t\t%f\n", float(data->cAmmoPerShot) / data->dtimeBurst);
                        debugf("\tenergy/second\t%f\n", data->energyPerShot / data->dtimeBurst);
                        debugf("\tTime to kill: (seconds)\t(target)\n");
                        {
                            for (PartTypeLinkIGC*   l2 = pMission->GetPartTypes()->first(); (l2 != NULL); l2 = l2->next())
                            {
                                IpartTypeIGC*   p2 = l2->data();
                                if (p2->GetEquipmentType() == ET_Shield)
                                {
                                    DataShieldTypeIGC*   d2 = (DataShieldTypeIGC*)(p2->GetData());
                                    float   dtm = pMission->GetDamageConstant(pt->GetDamageType(), d2->defenseType);
                                    float   d = dps * dtm; // - d2->rateRegen;
                                    if (d > 0.0f)
                                    {
                                        debugf("\t\t\t%8.2f", d2->maxStrength / d);

                                        debugf("\t%s\n", p2->GetName());
                                    }
                                }
                            }
                        }
                        {
                            for (HullTypeLinkIGC*   l2 = pMission->GetHullTypes()->first(); (l2 != NULL); l2 = l2->next())
                            {
                                IhullTypeIGC*   h2 = l2->data();
                                for (HullTypeLinkIGC*   l3 = l2->txen(); (l3 != NULL); l3 = l3->txen())
                                    if (strcmp(h2->GetName(), l3->data()->GetName()) == 0)
                                        break;

                                if (l3 == NULL)
                                {
                                    float   dtm = pMission->GetDamageConstant(pt->GetDamageType(), h2->GetDefenseType());
                                    float   d = dps * dtm;
                                    if (d > 0.0f)
                                    {
                                        debugf("\t\t\t%8.2f", h2->GetHitPoints() / d);

                                        debugf("\t%s\n", h2->GetName());
                                    }
                                }
                            }
                        }
                        {
                            for (StationTypeLinkIGC*   l2 = pMission->GetStationTypes()->first(); (l2 != NULL); l2 = l2->next())
                            {
                                IstationTypeIGC*   s2 = l2->data();
                                for (StationTypeLinkIGC*   l3 = l2->txen(); (l3 != NULL); l3 = l3->txen())
                                    if (strcmp(s2->GetName(), l3->data()->GetName()) == 0)
                                        break;

                                if (l3 == NULL)
                                {
                                    float   dtmA = pMission->GetDamageConstant(pt->GetDamageType(), s2->GetArmorDefenseType());
                                    float   dtmS = pMission->GetDamageConstant(pt->GetDamageType(), s2->GetShieldDefenseType());

                                    float   dS = dps * dtmS; // - s2->GetShieldRegeneration();
                                    if (dS > 0.0f)
                                    {
                                        debugf("\t\t\t%8.2f", s2->GetMaxShieldHitPoints() / dS);

                                        debugf("\t%s (shield)\n", s2->GetName());
                                    }

                                    float   dA = dps * dtmA; // - s2->GetArmorRegeneration();
                                    if (dA > 0.0f)
                                    {
                                        debugf("\t\t\t%8.2f", s2->GetMaxArmorHitPoints() / dA);

                                        debugf("\t%s (armor)\n", s2->GetName());
                                    }
                                }
                            }
                        }
                    }
                }
            }

            {
                debugf("Missiles\n");
                for (PartTypeLinkIGC*   ppl = pMission->GetPartTypes()->first(); (ppl != NULL); ppl = ppl->next())
                {
                    IpartTypeIGC*   ppt = ppl->data();
                    if (ppt->GetEquipmentType() == ET_Magazine)
                    {
                        ImissileTypeIGC* pmt = (ImissileTypeIGC*)(((IlauncherTypeIGC*)ppt)->GetExpendableType());
                        float   dps = (pmt->GetPower() + pmt->GetBlastPower());

                        debugf("Name: %s\n", pmt->GetName());
                        debugf("\tShots to kill: (shots)\t(target)\n");
                        {
                            for (PartTypeLinkIGC*   l2 = pMission->GetPartTypes()->first(); (l2 != NULL); l2 = l2->next())
                            {
                                IpartTypeIGC*   p2 = l2->data();
                                if (p2->GetEquipmentType() == ET_Shield)
                                {
                                    DataShieldTypeIGC*   d2 = (DataShieldTypeIGC*)(p2->GetData());
                                    float   dtm = pMission->GetDamageConstant(pmt->GetDamageType(), d2->defenseType);
                                    float   d = dps * dtm; // - d2->rateRegen;
                                    if (d > 0.0f)
                                    {
                                        debugf("\t\t\t%8.2f", d2->maxStrength / d);

                                        debugf("\t%s\n", p2->GetName());
                                    }
                                }
                            }
                        }
                        {
                            for (HullTypeLinkIGC*   l2 = pMission->GetHullTypes()->first(); (l2 != NULL); l2 = l2->next())
                            {
                                IhullTypeIGC*   h2 = l2->data();
                                for (HullTypeLinkIGC*   l3 = l2->txen(); (l3 != NULL); l3 = l3->txen())
                                    if (strcmp(h2->GetName(), l3->data()->GetName()) == 0)
                                        break;

                                if (l3 == NULL)
                                {
                                    float   dtm = pMission->GetDamageConstant(pmt->GetDamageType(), h2->GetDefenseType());
                                    float   d = dps * dtm;
                                    if (d > 0.0f)
                                    {
                                        debugf("\t\t\t%8.2f", h2->GetHitPoints() / d);

                                        debugf("\t%s\n", h2->GetName());
                                    }
                                }
                            }
                        }
                        {
                            for (StationTypeLinkIGC*   l2 = pMission->GetStationTypes()->first(); (l2 != NULL); l2 = l2->next())
                            {
                                IstationTypeIGC*   s2 = l2->data();
                                for (StationTypeLinkIGC*   l3 = l2->txen(); (l3 != NULL); l3 = l3->txen())
                                    if (strcmp(s2->GetName(), l3->data()->GetName()) == 0)
                                        break;

                                if (l3 == NULL)
                                {
                                    float   dtmA = pMission->GetDamageConstant(pmt->GetDamageType(), s2->GetArmorDefenseType());
                                    float   dtmS = pMission->GetDamageConstant(pmt->GetDamageType(), s2->GetShieldDefenseType());

                                    float   dS = dps * dtmS; // - s2->GetShieldRegeneration();
                                    if (dS > 0.0f)
                                    {
                                        debugf("\t\t\t%8.2f", s2->GetMaxShieldHitPoints() / dS);

                                        debugf("\t%s (shield)\n", s2->GetName());
                                    }

                                    float   dA = dps * dtmA; // - s2->GetArmorRegeneration();
                                    if (dA > 0.0f)
                                    {
                                        debugf("\t\t\t%8.2f", s2->GetMaxArmorHitPoints() / dA);

                                        debugf("\t%s (armor)\n", s2->GetName());
                                    }
                                }
                            }
                        }
                    }
                }
            }

#endif
        }

        fclose (file);

		// BT - STEAM - Ensure that the ICG Core files are not tampered with.
		// Have to let the static core info above load, even if it's no good or the client crashes out becuase it can't find a civ.
#ifdef STEAMSECURE
		if (fGetVersionOnly == true)
		{
			FileHashTable fileHashTable;
			if (fileHashTable.DoesFileHaveHash(szFilename) == true)
			{
				TRef<ZFile> coreFile = new ZFile(szFilename);
				if (fileHashTable.IsHashCorrect(szFilename, coreFile) == false)
				{
					iStaticCoreVersion = 0;
				}
			}
		}
#endif

        return iStaticCoreVersion;
    }
    else
    {
		// Imago try to load a text-based core if no .igc is present 
		ZString strName = ZString(name) + "\\Constants";
		char        szFilename[MAX_PATH + 1];
		HRESULT     hr = UTL::getFile(strName, ".csv", szFilename, true, true);
		if (hr == S_OK) {
            
            //NYI: Version
		    FILE*       file = fopen(szFilename, "rb");
            fclose (file);

			if (!fGetVersionOnly)
			{   
				pMission->SetStaticCore(NULL);

                //force new params /w core name if empty
                if (pMission->GetMissionParams()->szIGCStaticFile[0] == '\0') {
                    MissionParams mp;
                    Strcpy(mp.szIGCStaticFile, name);
			        pMission->SetMissionParams(&mp);
                }
                
                //Text2Core 
                pMission->ImportStaticIGCObjs();

				CachedLink* pcl = new CachedLink;
				pcl->data().pstatic = pMission->GetStaticCore();
				strcpy(pcl->data().name, name);
				pcl->data().version = 1337;
				pcl->data().consumers = 1;
				s_cores.last(pcl);
			}
			
			return 1337; 
            //NYI Version
		} else {
			// mmf added debugf
			debugf("Load IGC Static Core failed to load %s\n",name);
			assert ("Load IGC Static Core failed." && false);
			return NA;
		}
    }
}

ImissionIGC*    CreateMission(void)
{
    return new CmissionIGC;
}

const float c_zLensMultiplier = 0.2f; // DON'T CHANGE THIS, people get really pissy about
const float c_radiusUniverse = 5000.0f;
const float c_outOfBounds = 25.0f;
const float c_fExitWarpSpeed = 15.0f;
const float c_fExitStationSpeed = 50.0f;
const float c_downedShield = 0.1f;
const float c_dtBetweenPaydays = 60.0f;             //one minute
const float c_fCapacityHe3 = 90.0f;
const float c_fValueHe3 = 80.0f;
const float c_fAmountHe3 = 120000.0f / c_fValueHe3; //total amount of He3 in universe/team
const float c_mountRate = 1.0f / 5.0f;

const float c_fStartingMoney = 16000.0f;
const float c_fWinTheGame    = c_fAmountHe3 * c_fValueHe3 * 0.6f;

const float c_dtRipcord = 10.0f;

const float c_fHe3Regeneration = 0.01f/60.0f;     //1% of the asteroid's value/minute

const float c_pointsWarp        = 2.0f;
const float c_pointsAsteroid    = 1.0f;
const float c_pointsTech        = 5.0f;
const float c_pointsMiner       = 10.0f;
const float c_pointsBuilder     = 20.0f;
const float c_pointsLayer       = 10.0f;
const float c_pointsCarrier     = 10.0f;
const float c_pointsPlayer      = 10.0f;
const float c_pointsBaseKill    = 25.0f;
const float c_pointsBaseCapture = 25.0f;

const float c_pointsFlags       = 10.0f;
const float c_pointsArtifacts   = 10.0f;
const float c_pointsRescues     = 5.0f;
const float c_pointsProbeSpot   = 10.0f;
const float c_pointsRepair		= 0.02f; //1 point for every 50 health restored

const float c_crAdd             = 1.25f;
const float c_crDivide          = 100.0f;

const float c_fIncome           = 600.0f;

const float c_fLifepodEndurance = 300.0f;

const float c_fWarpBombDelay    = 15.0f;

const float c_fLifepodCost      = 0.5f;
const float c_fTurretCost       = 0.5f;
const float c_fDroneCost        = 0.5f;
const float c_fPlayerCost       = 1.0f;
const float c_fBaseClusterCost  = -36.0f;
const float c_fClusterDivisor   = 400.0f;

CstaticIGC::CstaticIGC(void)
:
    m_pptAmmoPack(NULL),
    m_pptFuelPack(NULL)
{
    m_constants.floatConstants[c_fcidLensMultiplier]        = c_zLensMultiplier;
    m_constants.floatConstants[c_fcidRadiusUniverse]        = c_radiusUniverse;
    m_constants.floatConstants[c_fcidOutOfBounds]           = c_outOfBounds;
    m_constants.floatConstants[c_fcidExitWarpSpeed]         = c_fExitWarpSpeed;
    m_constants.floatConstants[c_fcidExitStationSpeed]      = c_fExitStationSpeed;
    m_constants.floatConstants[c_fcidDownedShield]          = c_downedShield;
    m_constants.floatConstants[c_fcidSecondsBetweenPaydays] = c_dtBetweenPaydays;
    m_constants.floatConstants[c_fcidCapacityHe3]           = c_fCapacityHe3;
    m_constants.floatConstants[c_fcidValueHe3]              = c_fValueHe3;
    m_constants.floatConstants[c_fcidAmountHe3]             = c_fAmountHe3;
    m_constants.floatConstants[c_fcidMountRate]             = c_mountRate;

    m_constants.floatConstants[c_fcidStartingMoney]         = c_fStartingMoney;
    m_constants.floatConstants[c_fcidWinTheGameMoney]       = c_fWinTheGame;
    m_constants.floatConstants[c_fcidRipcordTime]           = c_dtRipcord;
    m_constants.floatConstants[c_fcidHe3Regeneration]       = c_fHe3Regeneration;

    m_constants.floatConstants[c_fcidPointsWarp]            = c_pointsWarp;
    m_constants.floatConstants[c_fcidPointsAsteroid]        = c_pointsAsteroid;
    m_constants.floatConstants[c_fcidPointsTech]            = c_pointsTech;
    m_constants.floatConstants[c_fcidPointsMiner]           = c_pointsMiner;
    m_constants.floatConstants[c_fcidPointsBuilder]         = c_pointsBuilder;
    m_constants.floatConstants[c_fcidPointsLayer]           = c_pointsLayer;
    m_constants.floatConstants[c_fcidPointsCarrier]         = c_pointsCarrier;
    m_constants.floatConstants[c_fcidPointsPlayer]          = c_pointsPlayer;
    m_constants.floatConstants[c_fcidPointsBaseKill]        = c_pointsBaseKill;
    m_constants.floatConstants[c_fcidPointsBaseCapture]     = c_pointsBaseCapture;
    m_constants.floatConstants[c_fcidPointsFlags]           = c_pointsFlags;
    m_constants.floatConstants[c_fcidPointsArtifacts]       = c_pointsArtifacts;
    m_constants.floatConstants[c_fcidPointsRescues]         = c_pointsRescues;
	//m_constants.floatConstants[c_fcidPointsProbeSpot]		= c_pointsProbeSpot;
	//m_constants.floatConstants[c_fcidPointsRepair]			= c_pointsRepair;
    m_constants.floatConstants[c_fcidRatingAdd]             = c_crAdd;
    m_constants.floatConstants[c_fcidRatingDivide]          = c_crDivide;

    m_constants.floatConstants[c_fcidIncome]                = c_fIncome;

    m_constants.floatConstants[c_fcidLifepodEndurance]      = c_fLifepodEndurance;

    m_constants.floatConstants[c_fcidWarpBombDelay]         = c_fWarpBombDelay;

    m_constants.floatConstants[c_fcidLifepodCost]           = c_fLifepodCost;
    m_constants.floatConstants[c_fcidTurretCost]            = c_fTurretCost;
    m_constants.floatConstants[c_fcidDroneCost]             = c_fDroneCost;
    m_constants.floatConstants[c_fcidPlayerCost]            = c_fPlayerCost;
    m_constants.floatConstants[c_fcidBaseClusterCost]       = c_fBaseClusterCost;
    m_constants.floatConstants[c_fcidClusterDivisor]        = c_fClusterDivisor;

    for (int i = 0; (i < c_dmgidMax); i++)
    {
        m_constants.damageConstants[i][0] = 0.0f;       //Everything vs. asteroid defaults to 0
        for (int j = 1; (j < c_defidMax); j++)
            m_constants.damageConstants[i][j] = 1.0f;
    }

    //NYI hard code the starting exceptions.
    m_constants.damageConstants[1][1] = 1.00f;        //Plasma rifle vs. lt ship
    m_constants.damageConstants[1][2] = 0.5f;         //Plasma rifle vs. md ship
    m_constants.damageConstants[1][3] = 0.5f;         //Plasma rifle vs. hv ship
    m_constants.damageConstants[1][4] = 0.25f;        //Plasma rifle vs. shv ship
    m_constants.damageConstants[1][5] = 2.00f;        //Plasma rifle vs. utility ships
    m_constants.damageConstants[1][6] = 0.0f;         //Plasma rifle vs. lt base
    m_constants.damageConstants[1][7] = 0.0f;         //Plasma rifle vs. hv base
    m_constants.damageConstants[1][8] = 0.75f;        //Plasma rifle vs. ship shields
    m_constants.damageConstants[1][9] = 0.0f;         //Plasma rifle vs. lt base shields
    m_constants.damageConstants[1][10]= 0.0f;         //Plasma rifle vs. hv base shields
    m_constants.damageConstants[1][11]= 1.0f;         //Plasma rifle vs. probe
    m_constants.damageConstants[1][12]= 0.25f;        //Plasma rifle vs. ultralt base armor
    m_constants.damageConstants[1][13]= 1.0f;         //Plasma rifle vs. ultralt base shield
    m_constants.damageConstants[1][14] = 0.75f;       //Plasma rifle vs. lg ship shields

    m_constants.damageConstants[2][1] = 1.00f;        //MG vs. lt ship
    m_constants.damageConstants[2][2] = 0.75f;        //MG vs. md ship
    m_constants.damageConstants[2][3] = 0.75f;        //MG vs. hv ship
    m_constants.damageConstants[2][4] = 0.5f;         //MG vs. shv ship
    m_constants.damageConstants[2][5] = 0.5f;         //MG vs. utility ships
    m_constants.damageConstants[2][6] = 0.0f;         //MG vs. lt base
    m_constants.damageConstants[2][7] = 0.0f;         //MG vs. hv base
    m_constants.damageConstants[2][8] = 0.5f;         //MG vs. ship shields
    m_constants.damageConstants[2][9] = 0.00f;        //MG vs. lt base shields
    m_constants.damageConstants[2][10]= 0.00f;        //MG vs. hv base shields
    m_constants.damageConstants[2][11]= 1.00f;        //MG vs. probe
    m_constants.damageConstants[2][12]= 0.5f;         //MG vs. ultralt base armor
    m_constants.damageConstants[2][13]= 0.5f;         //MG vs. ultralt base shield
    m_constants.damageConstants[2][14] = 0.5f;        //MG vs. lg ship shields

    m_constants.damageConstants[3][1] = 1.00f;        //Disruptors vs. lt ship
    m_constants.damageConstants[3][2] = 1.50f;        //Disruptors vs. md ship
    m_constants.damageConstants[3][3] = 4.00f;        //Disruptors vs. hv ship
    m_constants.damageConstants[3][4] = 4.00f;        //Disruptors vs. shv ship
    m_constants.damageConstants[3][5] = 0.50f;        //Disruptors vs. utility ships
    m_constants.damageConstants[3][6] = 0.00f;        //Disruptors vs. lt base
    m_constants.damageConstants[3][7] = 0.00f;        //Disruptors vs. hv base
    m_constants.damageConstants[3][8] = 3.0f;         //Disruptors vs. ship shields
    m_constants.damageConstants[3][9] = 0.0f;         //Disruptors vs. lt base shields
    m_constants.damageConstants[3][10]= 0.0f;         //Disruptors vs. hv base shields
    m_constants.damageConstants[3][11]= 1.0f;         //Disruptors vs. probe
    m_constants.damageConstants[3][12]= 4.0f;         //Disruptors vs. ultralt base armor
    m_constants.damageConstants[3][13]= 3.0f;         //Disruptors vs. ultralt base shield
    m_constants.damageConstants[3][14] = 3.0f;        //Disruptors vs. lg ship shields

    m_constants.damageConstants[4][1] = 1.00f;        //Sniper rifle vs. lt ship
    m_constants.damageConstants[4][2] = 1.00f;        //Sniper rifle vs. md ship
    m_constants.damageConstants[4][3] = 1.33f;        //Sniper rifle vs. hv ship
    m_constants.damageConstants[4][4] = 1.00f;        //Sniper rifle vs. shv ship
    m_constants.damageConstants[4][5] = 0.5f;         //Sniper rifle vs. utility ships
    m_constants.damageConstants[4][6] = 0.00f;        //Sniper rifle vs. lt base
    m_constants.damageConstants[4][7] = 0.00f;        //Sniper rifle vs. hv base
    m_constants.damageConstants[4][8] = 1.00f;        //Sniper rifle vs. ship shields
    m_constants.damageConstants[4][9] = 0.00f;        //Sniper rifle vs. lt base shields
    m_constants.damageConstants[4][10]= 0.00f;        //Sniper rifle vs. hv base shields
    m_constants.damageConstants[4][11]= 1.00f;        //Sniper rifle vs. probe
    m_constants.damageConstants[4][12]= 1.00f;        //Sniper rifle vs. ultralt base armor
    m_constants.damageConstants[4][13]= 1.0f;         //Sniper rifle vs. ultralt base shield
    m_constants.damageConstants[4][14] = 1.00f;       //Sniper rifle vs. lg ship shields

    m_constants.damageConstants[5][1] = 1.00f;        //Mini-gun vs. lt ship
    m_constants.damageConstants[5][2] = 0.75f;        //Mini-gun vs. md ship
    m_constants.damageConstants[5][3] = 0.50f;        //Mini-gun vs. hv ship
    m_constants.damageConstants[5][4] = 0.25f;        //Mini-gun vs. shv ship
    m_constants.damageConstants[5][5] = 0.25f;        //Mini-gun vs. utility ships
    m_constants.damageConstants[5][6] = 0.0f;         //Mini-gun vs. lt base
    m_constants.damageConstants[5][7] = 0.0f;         //Mini-gun vs. hv base
    m_constants.damageConstants[5][8] = 0.5f;         //Mini-gun vs. ship shields
    m_constants.damageConstants[5][9] = 0.0f;         //Mini-gun vs. lt base shields
    m_constants.damageConstants[5][10]= 0.0f;         //Mini-gun vs. hv base shields
    m_constants.damageConstants[5][11]= 1.0f;         //Mini-gun vs. probe
    m_constants.damageConstants[5][12]= 0.25f;        //Mini-gun vs. ultralt base armor
    m_constants.damageConstants[5][13]= 0.5f;         //Mini-gun vs. ultralt base shield
    m_constants.damageConstants[5][14] = 0.5f;        //Mini-gun vs. lg ship shields

    m_constants.damageConstants[6][1] = 1.00f;        //Cannon vs. lt ship
    m_constants.damageConstants[6][2] = 0.5f;         //Cannon vs. md ship
    m_constants.damageConstants[6][3] = 0.25f;        //Cannon vs. hv ship
    m_constants.damageConstants[6][4] = 0.25f;        //Cannon vs. shv ship
    m_constants.damageConstants[6][5] = 0.5f;         //Cannon vs. utility ships
    m_constants.damageConstants[6][6] = 0.0f;         //Cannon vs. lt base
    m_constants.damageConstants[6][7] = 0.0f;         //Cannon vs. hv base
    m_constants.damageConstants[6][8] = 1.0f;         //Cannon vs. ship shields
    m_constants.damageConstants[6][9] = 0.0f;         //Cannon vs. lt base shields
    m_constants.damageConstants[6][10]= 0.0f;         //Cannon vs. hv base shields
    m_constants.damageConstants[6][11]= 1.0f;         //Cannon vs. probe
    m_constants.damageConstants[6][12]= 0.25f;        //Cannon vs. ultralt base armor
    m_constants.damageConstants[6][13]= 1.0f;         //Cannon vs. ultralt base shield
    m_constants.damageConstants[6][14] = 1.0f;        //Cannon vs. lg ship shields

    m_constants.damageConstants[7][0] = 1.00f;        //Galvonic vs. asteroids
    m_constants.damageConstants[7][1] = 1.00f;        //Galvonic vs. lt ship
    m_constants.damageConstants[7][2] = 1.50f;        //Galvonic vs. md ship
    m_constants.damageConstants[7][3] = 4.00f;        //Galvonic vs. hv ship
    m_constants.damageConstants[7][4] = 4.00f;        //Galvonic vs. shv ship
    m_constants.damageConstants[7][5] = 0.50f;        //Galvonic vs. utility ships
    m_constants.damageConstants[7][6] = 10.00f;       //Galvonic vs. lt base
    m_constants.damageConstants[7][7] = 0.00f;        //Galvonic vs. hv base
    m_constants.damageConstants[7][8] = 3.0f;         //Galvonic vs. ship shields
    m_constants.damageConstants[7][9] = 4.0f;         //Galvonic vs. lt base shields
    m_constants.damageConstants[7][10]= 0.0f;         //Galvonic vs. hv base shields
    m_constants.damageConstants[7][11]= 1.0f;         //Galvonic vs. probe
    m_constants.damageConstants[7][12]= 10.00f;       //Galvonic vs. ultralt base armor
    m_constants.damageConstants[7][13]= 4.0f;         //Galvonic vs. ultralt base shield
    m_constants.damageConstants[7][14] = 3.0f;        //Galvonic vs. lg ship shields

    m_constants.damageConstants[8][0] = 1.0f;         //Anti-base weapons vs. asteroid
    m_constants.damageConstants[8][6] = 15.0f;        //Anti-base weapons vs. lt base
    m_constants.damageConstants[8][7] = 10.0f;        //Anti-base weapons vs. hv base
    m_constants.damageConstants[8][9] = 10.0f;        //Anti-base weapons vs. lt base shield
    m_constants.damageConstants[8][10]= 10.0f;        //Anti-base weapons vs. hv base shield
    m_constants.damageConstants[8][12]= 15.00f;       //Anti-base weapons vs. ultralt base armor
    m_constants.damageConstants[8][13]= 15.0f;        //Anti-base weapons vs. ultralt base shield

    m_constants.damageConstants[9][0] = 15.0f;        //Anti-rock weapons vs. asteroids

                                                      //Repair weapons

    m_constants.damageConstants[11][1] = 1.00f;       //Turret vs. lt ship
    m_constants.damageConstants[11][2] = 0.85f;       //Turret vs. md ship
    m_constants.damageConstants[11][3] = 0.50f;       //Turret vs. hv ship
    m_constants.damageConstants[11][4] = 0.25f;       //Turret vs. shv ship
    m_constants.damageConstants[11][5] = 0.50f;       //Turret vs. utility ships
    m_constants.damageConstants[11][6] = 0.0f;        //Turret vs. lt base
    m_constants.damageConstants[11][7] = 0.0f;        //Turret vs. hv base
    m_constants.damageConstants[11][8] = 0.75f;       //Turret vs. ship shields
    m_constants.damageConstants[11][9] = 0.0f;        //Turret vs. lt base shields
    m_constants.damageConstants[11][10]= 0.0f;        //Turret vs. hv base shields
    m_constants.damageConstants[11][11]= 1.0f;        //Turret vs. probe
    m_constants.damageConstants[11][12]= 0.25f;       //Turret vs. ultralt base armor
    m_constants.damageConstants[11][13]= 0.75f;       //Turret vs. ultralt base shield
    m_constants.damageConstants[11][14] = 0.75f;      //Turret vs. lg ship shields

    m_constants.damageConstants[12][0] = 0.00f;       //shield buster vs. asteroids
    m_constants.damageConstants[12][1] = 0.00f;       //shield buster vs. lt ship
    m_constants.damageConstants[12][2] = 0.00f;       //shield buster vs. md ship
    m_constants.damageConstants[12][3] = 0.00f;       //shield buster vs. hv ship
    m_constants.damageConstants[12][4] = 0.00f;       //shield buster vs. shv ship
    m_constants.damageConstants[12][5] = 0.00f;       //shield buster vs. utility ships
    m_constants.damageConstants[12][6] = 0.00f;       //shield buster vs. lt base
    m_constants.damageConstants[12][7] = 0.00f;       //shield buster vs. hv base
    m_constants.damageConstants[12][8] = 1.0f;        //shield buster vs. ship shields
    m_constants.damageConstants[12][9] = 1.0f;        //shield buster vs. base shields
    m_constants.damageConstants[12][10]= 1.0f;        //shield buster vs. base shields
    m_constants.damageConstants[12][11]= 0.0f;        //shield buster vs. probe
    m_constants.damageConstants[12][12]= 0.0f;        //shield buster vs. ultralt base armor
    m_constants.damageConstants[12][13]= 1.0f;        //shield buster vs. ultralt base shield
    m_constants.damageConstants[12][14] = 1.0f;       //shield buster vs. lg ship shields

    m_constants.damageConstants[13][0] = 0.00f;       //shield buster vs. asteroids
    m_constants.damageConstants[13][1] = 0.00f;       //shield buster vs. lt ship
    m_constants.damageConstants[13][2] = 0.00f;       //shield buster vs. md ship
    m_constants.damageConstants[13][3] = 0.00f;       //shield buster vs. hv ship
    m_constants.damageConstants[13][4] = 0.00f;       //shield buster vs. shv ship
    m_constants.damageConstants[13][5] = 0.00f;       //shield buster vs. utility ships
    m_constants.damageConstants[13][6] = 0.00f;       //shield buster vs. lt base
    m_constants.damageConstants[13][7] = 0.00f;       //shield buster vs. hv base
    m_constants.damageConstants[13][8] = 1.0f;        //shield buster vs. ship shields
    m_constants.damageConstants[13][9] = 0.5f;        //shield buster vs. base shields
    m_constants.damageConstants[13][10]= 0.0f;        //shield buster vs. base shields
    m_constants.damageConstants[13][11]= 0.0f;        //shield buster vs. probe
    m_constants.damageConstants[13][12]= 0.0f;        //shield buster vs. ultralt base armor
    m_constants.damageConstants[13][13]= 1.0f;        //shield buster vs. ultralt base shield
    m_constants.damageConstants[13][14] = 1.0f;       //shield buster vs. lg ship shields

                                                        //catpure weapon

    m_constants.damageConstants[15][0] = 1.00f;        //Skyripper vs. asteroid
    m_constants.damageConstants[15][1] = 1.00f;        //Skyripper vs. lt ship
    m_constants.damageConstants[15][2] = 1.00f;        //Skyripper vs. md ship
    m_constants.damageConstants[15][3] = 2.50f;        //Skyripper vs. hv ship
    m_constants.damageConstants[15][4] = 2.00f;        //Skyripper vs. shv ship
    m_constants.damageConstants[15][5] = 1.00f;        //Skyripper vs. utility ships
    m_constants.damageConstants[15][6] = 1.5f;         //Skyripper vs. lt base
    m_constants.damageConstants[15][7] = 1.00f;        //Skyripper vs. hv base
    m_constants.damageConstants[15][8] = 1.5f;         //Skyripper vs. ship shields
    m_constants.damageConstants[15][9] = 2.00f;        //Skyripper vs. lt base shields
    m_constants.damageConstants[15][10]= 2.00f;        //Skyripper vs. hv base shields
    m_constants.damageConstants[15][11]= 1.00f;        //Skyripper vs. probe
    m_constants.damageConstants[15][12]= 2.0f;         //Skyripper vs. ultralt base armor
    m_constants.damageConstants[15][13]= 2.0f;         //Skyripper vs. ultralt base shield
    m_constants.damageConstants[15][14] = 1.5f;        //Skyripper vs. lg ship shields

    m_constants.damageConstants[16][1] = 1.00f;        //Mine vs. lt ship
    m_constants.damageConstants[16][2] = 0.75f;        //Mine vs. md ship
    m_constants.damageConstants[16][3] = 0.75f;        //Mine vs. hv ship
    m_constants.damageConstants[16][4] = 0.50f;        //Mine vs. shv ship
    m_constants.damageConstants[16][5] = 0.25f;        //Mine vs. utility ships
    m_constants.damageConstants[16][6] = 0.0f;         //Mine vs. lt base
    m_constants.damageConstants[16][7] = 0.0f;         //Mine vs. hv base
    m_constants.damageConstants[16][8] = 0.5f;         //Mine vs. ship shields
    m_constants.damageConstants[16][9] = 0.0f;         //Mine vs. lt base shields
    m_constants.damageConstants[16][10]= 0.0f;         //Mine vs. hv base shields
    m_constants.damageConstants[16][11]= 1.0f;         //Mine vs. probe
    m_constants.damageConstants[16][12]= 0.25f;        //Mine vs. ultralt base armor
    m_constants.damageConstants[16][13]= 0.5f;         //Mine vs. ultralt base shield
    m_constants.damageConstants[16][14] = 0.5f;        //Mine vs. lg ship shields

    m_constants.damageConstants[17][0] = 1.0f;         //Stinger vs. asteroid
    m_constants.damageConstants[17][6] = 15.0f;        //Stinger vs. lt base
    m_constants.damageConstants[17][7] = 10.0f;        //Stinger vs. hv base
    m_constants.damageConstants[17][9] = 15.0f;        //Stinger vs. lt base shield
    m_constants.damageConstants[17][10]= 15.0f;        //Stinger vs. hv base shield
    m_constants.damageConstants[17][12]= 15.0f;        //Stinger vs. ultralt base armor
    m_constants.damageConstants[17][13]= 15.0f;        //Stinger vs. ultralt base shield

    m_constants.damageConstants[18][1] = 1.00f;        //Lancer vs. lt ship
    m_constants.damageConstants[18][2] = 0.75f;        //Lancer vs. md ship
    m_constants.damageConstants[18][3] = 0.50f;        //Lancer vs. hv ship
    m_constants.damageConstants[18][4] = 0.40f;        //Lancer vs. shv ship
    m_constants.damageConstants[18][5] = 0.25f;        //Lancer vs. utility ships
    m_constants.damageConstants[18][6] = 0.0f;         //Lancer vs. lt base
    m_constants.damageConstants[18][7] = 0.0f;         //Lancer vs. hv base
    m_constants.damageConstants[18][8] = 0.75f;        //Lancer vs. ship shields
    m_constants.damageConstants[18][9] = 0.0f;         //Lancer vs. lt base shields
    m_constants.damageConstants[18][10]= 0.0f;         //Lancer vs. hv base shields
    m_constants.damageConstants[18][11]= 0.05f;        //Lancer vs. probe
    m_constants.damageConstants[18][12]= 0.25f;        //Lancer vs. ultralt base armor
    m_constants.damageConstants[18][13]= 0.5f;         //Lancer vs. ultralt base shield
    m_constants.damageConstants[18][14] = 0.50f;       //Lancer vs. lg ship shields
}

void TerminateStatic(CstaticIGC*  pStatic)
{
	// BT - 9/17 - Fixing fedsrv crashes.
	__try
	{
		pStatic->Terminate();
	}
	__except (StackTracer::ExceptionFilter(GetExceptionInformation()))
	{
		StackTracer::OutputStackTraceToDebugF();
	}
}

CmissionIGC::~CmissionIGC(void)
{
    if (m_pStatic != NULL)
    {
        for (CachedLink*    pcl = s_cores.first(); (pcl != NULL); pcl = pcl->next())
        {
            if (m_pStatic == pcl->data().pstatic)
            {
                if ((pcl->data().consumers--) == 1)
                {
					// BT - 9/17 - Fixing fedsrv crashes.
					TerminateStatic(m_pStatic);

                    delete m_pStatic;
                    delete pcl;
                    break;
                }
            }
        }
    }
}

void CmissionIGC::Initialize(Time now, IIgcSite* pIgcSite)
{
    debugf("Mission Initialize id=%d this=%x now=%d\n", GetMissionID(), this, now.clock());
    m_pIgcSite = pIgcSite;
    m_lastUpdate = now;

    m_damageTracks.Initialize(now);

    //preload the convex hulls used for the various asteroids
    ZVerify(HitTest::Load("bgrnd50"));
    ZVerify(HitTest::Load("bgrnd51"));
    ZVerify(HitTest::Load("bgrnd52"));
    ZVerify(HitTest::Load("bgrnd53"));
    ZVerify(HitTest::Load("bgrnd03"));


    m_sideTeamLobby = NULL;
}

void    CmissionIGC::Terminate(void)
{
    debugf("Terminating mission id=%d, this=%x igccount=%x\n", GetMissionID(), this, GetIgcSite()->GetCount());
  
    m_pIgcSite->TerminateMissionEvent(this);

    {
        //Ships are not terminated ... they are mearly moved to the NULL mission
        //So do it before nuking the clusters since that would nuke the ships in the
        //cluster.
        ShipLinkIGC*  l;
        debugf("moving %x ships to NULL mission\n",m_ships.n());
        while ((l = m_ships.first()) != NULL)
        {
            l->data()->SetMission(NULL);
        }
    }
    {
        ClusterLinkIGC*  l;
        debugf("nuking %x clusters\n",m_clusters.n());
        while ((l = m_clusters.first()) != NULL)
        {
            l->data()->Terminate();
        }
    }
    {
        SideLinkIGC*  l;
        debugf("nuking %x sides\n",m_sides.n());
        while ((l = m_sides.first()) != NULL)
        {
            l->data()->Terminate();
        }
    }
    if (m_sideTeamLobby)
    {
        m_sideTeamLobby->Terminate();
        m_sideTeamLobby->Release();
        m_sideTeamLobby = NULL;
    };

    assert (m_clusters.n() == 0);
    assert (m_warps.n() == 0);
    assert (m_stations.n() == 0);
    assert (m_asteroids.n() == 0);
    assert (m_treasures.n() == 0);
    assert (m_ships.n() == 0);
    assert (m_sides.n() == 0);
}

void     CmissionIGC::Update(Time now)
{
    assert (now >= m_lastUpdate);

    if (now != m_lastUpdate)
    {
        if ((m_stageMission == STAGE_STARTED) ||
            (m_stageMission == STAGE_STARTING))
        {
            {
                //Update the various sides
                for (SideLinkIGC*   l = m_sides.first();
                     (l != NULL);
                     l = l->next())
                {
                    l->data()->Update(now);
                }
            }

            m_damageTracks.Update(now);

            assert (now - m_lastUpdate < 600.0f);   //never 10 minutes at a time.
            float   deltaT = now - m_lastUpdate;

            //Update the components in chunks of no more than 250ms each
            //if more than one chuck of time is required, evenly divide the
            //total time into nice, even chunks.
            assert (deltaT > 0.0f);

            int n = 1 + (int)(deltaT / c_fTimeIncrement);
            //ShouldBe(n == 1);

            Time    startTime = m_lastUpdate;

            for (int i = 1; (i <= n); i++)
            {
                Time    timeUpdate = startTime + (deltaT * (((float)i) / ((float)n)));

                {
                    //Update all clusters (which update all the models in clusters, etc.)
                    //Clusters can never be deleted on an update, so this is safe.
                    for (ClusterLinkIGC*  l = m_clusters.first();
                         (l != NULL);
                         l = l->next())
                    {
                        assert (l->data());
                        l->data()->Update(timeUpdate);
                    }
                }

                m_lastUpdate = timeUpdate;
            }
        }
        else
            m_lastUpdate = now;
    }

    assert (m_lastUpdate == now);
}

static int  ExportList(__int64              maskTypes,
                       const BaseListIGC*   plist,
                       char**               ppdata)
{
    int offset = 0;
    int size = 0;

    for (BaseLinkIGC*   pbl = plist->first();
         (pbl != NULL);
         pbl = pbl->next())
    {
        ObjectType  type = pbl->data()->GetObjectType();
        if (maskTypes & (__int64(1) << __int64(type)))
        {
            if (*ppdata)
            {
                int sizeItem = pbl->data()->Export(*ppdata + sizeof(int) + sizeof(ObjectType));
            
                *((ObjectType*)*ppdata) = type;
                *((int*)(*ppdata + sizeof(ObjectType))) = sizeItem;

                *ppdata += sizeItem + sizeof(int) + sizeof(ObjectType);
                size += sizeItem + sizeof(int) + sizeof(ObjectType);
            }
            else
                size += pbl->data()->Export(NULL) + sizeof(int) + sizeof(ObjectType);
        }
    }

    return size;
}

int                 CmissionIGC::Export(__int64  maskTypes,
                                        char*    pdata) const
{
    int datasize = 0;

    if (maskTypes & (__int64(1) << __int64(OT_constants)))
    {
        int  size = GetSizeOfConstants();
        if (pdata)
        {
            *((ObjectType*)pdata) = OT_constants;
            *((int*)(pdata + sizeof(ObjectType))) = size;
            memcpy(pdata + sizeof(int) + sizeof(ObjectType), GetConstants(), size);

            pdata += sizeof(int) + sizeof(ObjectType) + size;
        }
        datasize += size + sizeof(int) + sizeof(ObjectType);
    }

    datasize += ExportList(maskTypes, (BaseListIGC*)GetProjectileTypes(), &pdata);

    datasize += ExportList(maskTypes, (BaseListIGC*)GetExpendableTypes(), &pdata);

    datasize += ExportList(maskTypes, (BaseListIGC*)GetPartTypes(), &pdata);

    datasize += ExportList(maskTypes, (BaseListIGC*)GetHullTypes(), &pdata);

    datasize += ExportList(maskTypes, (BaseListIGC*)GetDevelopments(), &pdata);

    datasize += ExportList(maskTypes, (BaseListIGC*)GetDroneTypes(), &pdata);

    datasize += ExportList(maskTypes, (BaseListIGC*)GetStationTypes(), &pdata);

    datasize += ExportList(maskTypes, (BaseListIGC*)GetTreasureSets(), &pdata);

    datasize += ExportList(maskTypes, (BaseListIGC*)GetCivilizations(), &pdata);

    datasize += ExportList(maskTypes, (BaseListIGC*)&m_sides, &pdata);
    datasize += ExportList(maskTypes, (BaseListIGC*)&m_clusters, &pdata);
    datasize += ExportList(maskTypes, (BaseListIGC*)&m_warps, &pdata);
    datasize += ExportList(maskTypes, (BaseListIGC*)&m_asteroids, &pdata);
    datasize += ExportList(maskTypes, (BaseListIGC*)&m_stations, &pdata);
    datasize += ExportList(maskTypes, (BaseListIGC*)&m_probes, &pdata);
    datasize += ExportList(maskTypes, (BaseListIGC*)&m_mines, &pdata);
    datasize += ExportList(maskTypes, (BaseListIGC*)&m_treasures, &pdata);


    return datasize;
}

void                CmissionIGC::Import(Time    now,
                                        __int64 maskTypes,
                                        char*   pdata,
                                        int     datasize)
{
    while (datasize > 0)
    {
        ObjectType  type = *((ObjectType*)pdata);
        int size = *((int*)(pdata + sizeof(ObjectType)));

        if (maskTypes & (__int64(1) << __int64(type)))
        {
            if (type == OT_constants)
            {
                SetConstants((void*)(pdata + sizeof(int) + sizeof(ObjectType)));
            }
            else
            {
                IbaseIGC*   b = CreateObject(now, type, (void*)(pdata + sizeof(int) + sizeof(ObjectType)), size);

                assert (b);
                b->Release();
            }
            pdata += size + sizeof(int) + sizeof(ObjectType);
        }

        datasize -= (size + sizeof(int) + sizeof(ObjectType));
    }
}

IbaseIGC*           CmissionIGC::CreateObject(Time now, ObjectType objecttype,
                                              const void* data, int dataSize)
{
    #define OBJECT(CLS)   case OT_##CLS## :\
                                { pBase = new C##CLS##IGC; } break;

    IbaseIGC*   pBase;

    //Create an instance of the class
    switch (objecttype)
    {
        OBJECT(projectile)
        OBJECT(missile)
        OBJECT(mine)
        OBJECT(probe)
        OBJECT(ship)
        OBJECT(weapon)
        OBJECT(shield)
        OBJECT(cloak)
        OBJECT(chaff)
        OBJECT(afterburner)
        OBJECT(pack)
        OBJECT(magazine)
        OBJECT(dispenser)
        OBJECT(treasure)
        OBJECT(treasureSet)
        OBJECT(cluster)
        OBJECT(warp)
        OBJECT(buoy)
        OBJECT(buildingEffect)
        OBJECT(side)
        OBJECT(civilization)
        OBJECT(station)
        OBJECT(asteroid)
        OBJECT(projectileType)
        OBJECT(hullType)
        OBJECT(stationType)
        OBJECT(missileType)
        OBJECT(mineType)
        OBJECT(chaffType)
        OBJECT(probeType)
        OBJECT(droneType)
        OBJECT(development)
        OBJECT(bucket)

        case OT_partType:
        {
            //Part types and magazine part types get special treatment
            assert ((dataSize >= sizeof(DataLauncherTypeIGC)) ||
                    (dataSize >= sizeof(DataPartTypeIGC)));
            assert (sizeof(DataLauncherTypeIGC) < sizeof(DataPartTypeIGC));

            pBase = (dataSize == sizeof(DataLauncherTypeIGC))
                    ? (IbaseIGC*)(new ClauncherTypeIGC)
                    : (IbaseIGC*)(new CpartTypeIGC);
        }
        break;

        default:
        {
            ShouldBe(false);
            pBase = NULL;
        }
    }

    if (pBase)
    {
        //Increment the ref count because IGC objects should be created with a ref count of 1.
        pBase->AddRef();

		HRESULT hr = pBase->Initialize(this, now, data, dataSize);
        if (FAILED(hr) || (hr == S_FALSE))  // mmf added or check for S_FALSE to support destroyed TP
        {
			// debugf("mmf Initialize return failed or S_FALSE in missionigc, S_FALSE added to support TP destroy\n");
            if (hr != E_ABORT)
                pBase->Terminate();

            pBase->Release();
            pBase = NULL;
        }
    }

    #undef  OBJECT

    return pBase;
}

ImodelIGC*  CmissionIGC::GetModel(ObjectType type, ObjectID id) const
{
    switch (type)
    {
        case OT_station:
            return GetStation(id);

        case OT_warp:
            return GetWarp(id);

        case OT_ship:
            return GetShip(id);

        case OT_asteroid:
            return GetAsteroid(id);

        case OT_treasure:
            return GetTreasure(id);

        case OT_probe:
            return GetProbe(id);

        case OT_mine:
            return GetMine(id);

        case OT_buoy:
            return GetBuoy(id);
    }

    return NULL;
}

//
// Note: Some objects aren't supported by this.  This returns NULL in such
//       cases.
//
IbaseIGC*  CmissionIGC::GetBase(ObjectType type, ObjectID id) const
{
    switch (type)
    {
        case OT_station:
            return GetStation(id);

        case OT_warp:
            return GetWarp(id);

        case OT_ship:
            return GetShip(id);

        case OT_asteroid:
            return GetAsteroid(id);

        case OT_treasure:
            return GetTreasure(id);

        case OT_probe:
            return GetProbe(id);

        case OT_mine:
            return GetMine(id);

        case OT_projectile:  
            assert(0);
            return NULL;//GetProjectile(id);

        case OT_side:        
            return GetSide(id);

        case OT_cluster:     
            return GetCluster(id);

        case OT_weapon:      
            return NULL;//GetWeapon(id);

        case OT_treasureSet:
            return GetTreasureSet(id);
/*
        case OT_shield:
        case OT_cloak:            
        case OT_pack:             
        case OT_magazine:         
        case OT_dispenser:        
        case OT_afterburner:     
        case OT_bucket:           
          return NULL;
*/
        case OT_projectileType:  
          return GetProjectileType(id);

        case OT_hullType:         
          return GetHullType(id);

        case OT_partType:         
          return GetPartType(id);

        case OT_missileType:   
            return NULL;//GetMissile??(id);

        case OT_mineType:         
            return GetMine(id);

        case OT_probeType:
            return GetProbe(id);

        case OT_civilization:     
            return GetCivilization(id);

        case OT_stationType:      
            return GetStationType(id);

        case OT_development:      
            return GetDevelopment(id);

        case OT_droneType:        
            return GetDroneType(id);
    }

    return NULL;
}

void                        CmissionIGC::AddWarp(IwarpIGC*  w)
{
    AddIbaseIGC((BaseListIGC*)&m_warps, w);
}
void                        CmissionIGC::DeleteWarp(IwarpIGC* w)
{
    DeleteIbaseIGC((BaseListIGC*)&m_warps, w);
}
IwarpIGC*                   CmissionIGC::GetWarp(WarpID     id) const
{
    return (IwarpIGC*)GetIbaseIGC((BaseListIGC*)&m_warps, id);
}
const WarpListIGC*    CmissionIGC::GetWarps(void) const
{
    return &m_warps;
}

void                        CmissionIGC::AddStation(IstationIGC*  s)
{
    AddIbaseIGC((BaseListIGC*)&m_stations, s);
}
void                        CmissionIGC::DeleteStation(IstationIGC* s)
{
    DeleteIbaseIGC((BaseListIGC*)&m_stations, s);
}
IstationIGC*                CmissionIGC::GetStation(StationID   id) const
{
    return (IstationIGC*)GetIbaseIGC((BaseListIGC*)&m_stations, id);
}
const StationListIGC* CmissionIGC::GetStations(void) const
{
    return &m_stations;
}

void                            CmissionIGC::AddTreasure(ItreasureIGC*  t)
{
    AddIbaseIGC((BaseListIGC*)&m_treasures, t);
}
void                            CmissionIGC::DeleteTreasure(ItreasureIGC* t)
{
    DeleteIbaseIGC((BaseListIGC*)&m_treasures, t);
}
ItreasureIGC*                   CmissionIGC::GetTreasure(TreasureID  id) const
{
    return (ItreasureIGC*)GetIbaseIGC((BaseListIGC*)&m_treasures, id);
}
const TreasureListIGC*    CmissionIGC::GetTreasures(void) const
{
    return &m_treasures;
}

void                        CmissionIGC::AddAsteroid(IasteroidIGC*  a)
{
    AddIbaseIGC((BaseListIGC*)&m_asteroids, a);
}
void                        CmissionIGC::DeleteAsteroid(IasteroidIGC* a)
{
    DeleteIbaseIGC((BaseListIGC*)&m_asteroids, a);
}
IasteroidIGC*                 CmissionIGC::GetAsteroid(AsteroidID       id) const
{
    return (IasteroidIGC*)GetIbaseIGC((BaseListIGC*)&m_asteroids, id);
}
const AsteroidListIGC*        CmissionIGC::GetAsteroids(void) const
{
    return &m_asteroids;
}

void                        CmissionIGC::AddProbe(IprobeIGC*  a)
{
    AddIbaseIGC((BaseListIGC*)&m_probes, a);
}
void                        CmissionIGC::DeleteProbe(IprobeIGC* a)
{
    DeleteIbaseIGC((BaseListIGC*)&m_probes, a);
}
IprobeIGC*                 CmissionIGC::GetProbe(ProbeID       id) const
{
    return (IprobeIGC*)GetIbaseIGC((BaseListIGC*)&m_probes, id);
}
const ProbeListIGC*        CmissionIGC::GetProbes(void) const
{
    return &m_probes;
}

void                        CmissionIGC::AddMine(ImineIGC*  a)
{
    AddIbaseIGC((BaseListIGC*)&m_mines, a);
}
void                        CmissionIGC::DeleteMine(ImineIGC* a)
{
    DeleteIbaseIGC((BaseListIGC*)&m_mines, a);
}
ImineIGC*                 CmissionIGC::GetMine(MineID       id) const
{
    return (ImineIGC*)GetIbaseIGC((BaseListIGC*)&m_mines, id);
}
const MineListIGC*        CmissionIGC::GetMines(void) const
{
    return &m_mines;
}

void                        CmissionIGC::AddSide(IsideIGC*  s)
{
    // if there is not a team lobby but the civs do exist, go ahead and 
    // create the team lobby.
    if (!m_sideTeamLobby && GetCivilizations()->first() && s->GetObjectID() != SIDE_TEAMLOBBY)
    {
        //create the lobby side
        DataSideIGC sidedata;
        strcpy(sidedata.name, "Team Lobby");
        sidedata.civilizationID = GetCivilizations()->first()->data()->GetObjectID();
        sidedata.sideID = SIDE_TEAMLOBBY;
        sidedata.gasAttributes.Initialize();
        sidedata.ttbmDevelopmentTechs.ClearAll();
        sidedata.ttbmInitialTechs.ClearAll();
        sidedata.color = Color(1.0f, 1.0f, 1.0f);
        sidedata.conquest = 0;
        sidedata.territory = 0;
        sidedata.nKills = sidedata.nEjections = sidedata.nDeaths = sidedata.nBaseKills 
            = sidedata.nBaseCaptures = sidedata.nFlags = sidedata.nArtifacts = 0;
        sidedata.squadID = NA;
		sidedata.allies = NA; // #ALLY

        m_sideTeamLobby = (IsideIGC*)CreateObject(Time::Now(), OT_side, &sidedata, sizeof(sidedata));
        m_sideTeamLobby->SetActiveF(true);
        DeleteSide(m_sideTeamLobby); // make sure it does not appear in the normal side list
        ZAssert(m_sideTeamLobby != NULL);
    }

    AddIbaseIGC((BaseListIGC*)&m_sides, s);
}
void                        CmissionIGC::DeleteSide(IsideIGC* s)
{
    DeleteIbaseIGC((BaseListIGC*)&m_sides, s);
}
IsideIGC*                   CmissionIGC::GetSide(SideID id) const
{
    if (id == SIDE_TEAMLOBBY)
        return m_sideTeamLobby;
    else
        return (IsideIGC*)GetIbaseIGC((BaseListIGC*)&m_sides, id);
}
const SideListIGC*          CmissionIGC::GetSides(void) const
{
    return &m_sides;
}

//Imago #120 #121 8/10 - 
	//look at enemy's scanner in pmodel's cluster and sets the entry in the ordered bool array to true if it has eye, 
	//if you specify poptionalmodel, it has to have been seen by the side as well in order for bseensides[team] = true.
	//why dont i just get the cluster's scannerlist where this is all happening? which way is "faster"...
void						CmissionIGC::GetSeenSides(ImodelIGC * pmodel, bool (&bseensides)[c_cSidesMax], ImodelIGC * poptionalmodel)
{

	for (ModelLinkIGC*  pml = pmodel->GetCluster()->GetModels()->first(); (pml != NULL); pml = pml->next()) {
		ImodelIGC*  pmodelother = pml->data();
		if ((pmodelother->GetObjectType() != OT_ship && pmodelother->GetObjectType() != OT_station  && pmodelother->GetObjectType() != OT_probe) || pmodel->GetObjectID() == pmodelother->GetObjectID())
			continue;
		for (SideLinkIGC* psl = GetSides()->first(); (psl != NULL); psl = psl->next()) {
			if (pmodelother->GetSide()->GetObjectID() != psl->data()->GetObjectID() || psl->data()->GetObjectID() == pmodel->GetSide()->GetObjectID())
				continue;
			if (pmodelother->GetObjectType() == OT_ship)
				if (((IshipIGC*)pmodelother)->InScannerRange(pmodel) && (!poptionalmodel || (poptionalmodel && poptionalmodel->SeenBySide(psl->data()))))
					bseensides[psl->data()->GetObjectID()] = true;
			if (pmodelother->GetObjectType() == OT_station)
				if (((IstationIGC*)pmodelother)->InScannerRange(pmodel) && (!poptionalmodel || (poptionalmodel && poptionalmodel->SeenBySide(psl->data()))))
					bseensides[psl->data()->GetObjectID()] = true;
			if (pmodelother->GetObjectType() == OT_probe)
				if (((IprobeIGC*)pmodelother)->InScannerRange(pmodel) && (!poptionalmodel || (poptionalmodel && poptionalmodel->SeenBySide(psl->data()))))
					bseensides[psl->data()->GetObjectID()] = true;
			if (bseensides[psl->data()->GetObjectID()])
				break;
		}
	}
}

void                        CmissionIGC::AddCluster(IclusterIGC*    c)
{
    AddIbaseIGC((BaseListIGC*)&m_clusters, c);
}
void                        CmissionIGC::DeleteCluster(IclusterIGC* c)
{
    DeleteIbaseIGC((BaseListIGC*)&m_clusters, c);
}
IclusterIGC*                CmissionIGC::GetCluster(SectorID id) const
{
    return (IclusterIGC*)GetIbaseIGC((BaseListIGC*)&m_clusters, id);
}
const ClusterListIGC*       CmissionIGC::GetClusters(void) const
{
    return &m_clusters;
}

void                        CmissionIGC::AddShip(IshipIGC*    s)
{
    AddIbaseIGC((BaseListIGC*)&m_ships, s);
}
void                        CmissionIGC::DeleteShip(IshipIGC* s)
{
    DeleteIbaseIGC((BaseListIGC*)&m_ships, s);
}
IshipIGC*                   CmissionIGC::GetShip(ShipID   id) const
{
    return (IshipIGC*)GetIbaseIGC((BaseListIGC*)&m_ships, id);
}
const ShipListIGC*          CmissionIGC::GetShips(void) const
{
    return &m_ships;
}

void                        CmissionIGC::AddBuoy(IbuoyIGC*    s)
{
    AddIbaseIGC((BaseListIGC*)&m_buoys, s);
}
void                        CmissionIGC::DeleteBuoy(IbuoyIGC* s)
{
    DeleteIbaseIGC((BaseListIGC*)&m_buoys, s);
}
IbuoyIGC*                   CmissionIGC::GetBuoy(BuoyID   id) const
{
    return (IbuoyIGC*)GetIbaseIGC((BaseListIGC*)&m_buoys, id);
}
const BuoyListIGC*          CmissionIGC::GetBuoys(void) const
{
    return &m_buoys;
}

void                        CmissionIGC::UpdateSides(Time now,
                                                     const MissionParams * pmp,
                                                     const char sideNames[c_cSidesMax][c_cbSideName])
{
	// mdvalley: declaration moved to here. Compiler doesn't like it in the for loop for some reason.
	SideID sid;

    static const float sideColors[c_cSidesMax][3] =
                                { {188.0f/255.0f, 160.0f/255.0f,   0.0f/255.0f}, //Gold
                                  {  0.0f/255.0f, 138.0f/255.0f, 217.0f/255.0f}, //Blue
                                  {156.0f/255.0f,  16.0f/255.0f, 102.0f/255.0f}, //Purple
                                  { 50.0f/255.0f, 140.0f/255.0f,  20.0f/255.0f}, //icky yellow
                                  {255.0f/255.0f, 145.0f/255.0f, 145.0f/255.0f}, //icky orange
                                  { 50.0f/255.0f, 200.0f/255.0f, 125.0f/255.0f}};//icky magenta

    for (sid = GetSides()->n(); sid < pmp->nTeams; sid++)
    {
        IcivilizationIGC*   pcivilization = GetCivilization(pmp->rgCivID[sid]);
        assert (pcivilization);

        DataSideIGC ds;
        ds.sideID = sid;
        ds.gasAttributes.Initialize();
        ds.civilizationID = pmp->rgCivID[sid];
        ds.color.SetRGBA(sideColors[sid][0],
                         sideColors[sid][1],
                         sideColors[sid][2]);
        memset(ds.name, 0x00, sizeof(ds.name));
        strcpy(ds.name, sideNames[sid]);
        ds.ttbmDevelopmentTechs.ClearAll();
        ds.ttbmInitialTechs.ClearAll();
        ds.conquest = 0;
        ds.territory = 0;
        ds.nKills = ds.nEjections = ds.nDeaths = ds.nBaseKills = ds.nBaseCaptures = ds.nFlags 
            = ds.nArtifacts = 0;
        ds.squadID = NA;

        IObject*    o = CreateObject(now,
                                     OT_side,
                                     &ds,
                                     sizeof(ds));
        assert (o);
        ((IsideIGC*)o)->SetActiveF(true);

        o->Release();
    }

    // if it looks like we need to delete a few sides to match the mission parameters, do so.
    if (sid > pmp->nTeams)
    {
        for (SideID sid = GetSides()->n() - 1; sid >= pmp->nTeams; sid--)
        {
            IsideIGC* pside = GetSide(sid);
            assert(pside->GetShips()->n() == 0); // side should be empty
            pside->Terminate();
        }
    }

    assert (GetSides()->n() == pmp->nTeams);
}
// #ALLY
void		CmissionIGC::UpdateAllies(const char Allies[c_cSidesMax])
{
	for (SideID s = 0; s < GetSides()->n(); s++)
	{
		IsideIGC* pside = GetSide(s);
		if (pside) // shouldnt happen but let's be cautious
			pside->SetAllies(Allies[s]);
	}
}
void                    CmissionIGC::ResetMission()
{
    {
        // move all of the ships to the null cluster
        for (ShipLinkIGC*  l = m_ships.first(); l != NULL; l = l->next())
        {
            l->data()->SetCluster(NULL);
            l->data()->SetFlag(NA);
        }
    }
    {
        // nuke all clusters
        const ClusterListIGC*   plistClusters = GetClusters();
        ClusterLinkIGC*         plinkCluster;
        while (plinkCluster = plistClusters->first())  //Not ==
            plinkCluster->data()->Terminate();
    }    

    // reset all of the side techs
    for (SideLinkIGC * plinkSides = GetSides()->first(); 
        (plinkSides != NULL); plinkSides = plinkSides->next())
    {
        plinkSides->data()->DestroyBuckets();
    }
}

void                    CmissionIGC::GenerateMission(Time                   now,
                                                     const MissionParams *  pmp,
                                                     TechTreeBitMask *      pttbmShouldOverride,
                                                     TechTreeBitMask *      pttbmOverrideValue)
{
    m_nextBuoyID = 0;
    m_nextShipID = 0;
    m_nextAsteroidID = 0;
    m_nextTreasureID = 0;
    m_nextMineID = 0;
    m_nextProbeID = 0;
    m_nextMissileID = 0;
    m_nextStationID = 0;

    m_missionParams = *pmp;

    // Increment replay count, if we have been generated once before
    if (m_bHasGenerated)
    {
      ++m_nReplayCount;

      // Clear the context name, so that it re-generates
      m_strContextName.SetEmpty();
    }
    else
    {
      m_bHasGenerated = true;
    }


    //Create buckets for all of the sides in the game
    {
        unsigned char   pctConquest = (unsigned char)(100 / m_sides.n());
        for (SideLinkIGC*   psl = m_sides.first(); (psl != NULL); psl = psl->next())
        {
            IsideIGC*           pside = psl->data();
            IcivilizationIGC*   pciv = pside->GetCivilization();

            TechTreeBitMask ttbmDefaultTechs;
            if (pmp->bAllowDevelopments)
            {
                ttbmDefaultTechs = pciv->GetBaseTechs();

                if (pmp->bAllowShipyardPath)
                    ttbmDefaultTechs.SetBit(c_ttbShipyardAllowed);
                if (pmp->bAllowExpansionPath)
                    ttbmDefaultTechs.SetBit(c_ttbExpansionAllowed);
                if (pmp->bAllowTacticalPath)
                    ttbmDefaultTechs.SetBit(c_ttbTacticalAllowed);
                if (pmp->bAllowSupremacyPath)
                    ttbmDefaultTechs.SetBit(c_ttbSupremacyAllowed);
            }
            else
                ttbmDefaultTechs = pciv->GetNoDevTechs();
            
            if (pttbmShouldOverride && pttbmOverrideValue)
            {
              int nID = pside->GetObjectID();
              pside->SetDevelopmentTechs((pttbmShouldOverride[nID] & pttbmOverrideValue[nID]) | 
                                          (ttbmDefaultTechs - pttbmShouldOverride[nID]));
            }
            else
              pside->SetDevelopmentTechs(ttbmDefaultTechs);

            pside->UpdateInitialTechs();

            pside->SetConquestPercent(pctConquest);

            psl->data()->CreateBuckets();
        }
    }

    // check for custom map, and make sure it exists.
    if (pmp->szCustomMapFile[0] == '\0' || !LoadIGCFile(pmp->szCustomMapFile, this))
    {
        ImapMakerIGC::Create(now, pmp, this);
    }
    else
    {
        //
        // Make a mission with a custom map
        //

		// KGJV - randomize alephs positions
		// ---------------------------------
		// code from CmapMakerIGC::GenerateMission
		// adapted for predefined alephs.
		{
			//Adjust the position of all alephs in all clusters
			float   majorRadius = GetFloatConstant(c_fcidRadiusUniverse);
			for (ClusterLinkIGC*    pcl = m_clusters.first(); (pcl != NULL); pcl = pcl->next())
			{
				IclusterIGC*    pcluster = pcl->data();
				const WarpListIGC*  pwarps = pcluster->GetWarps();
				if (pwarps->n() != 0)
				{
					float   nWarps = (float)(pwarps->n());
					const int c_maxWarps = 10;
					assert (pwarps->n() <= c_maxWarps);
					float   offset[c_maxWarps] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
					float   displacement;
					if (pwarps->n() > 1)
					{
						displacement =  sin(pi/nWarps) * 2.0f / 3.0f;
						for (int index = 0; (index < pwarps->n()); index++)
						{
							int swap = randomInt(0, pwarps->n() - 1);
							if (swap != index)
							{
								float   t = offset[index];
								offset[index] = offset[swap];
								offset[swap] = t;
							}
						}
					}
					else
						displacement = 2.0f / 3.0f;
					{
						float   bias = random(0.0f, 2.0f * pi);
						int     index = 0;

						for (WarpLinkIGC*   pwl = pwarps->first(); (pwl != NULL); pwl = pwl->next())
						{
							IwarpIGC*   pwarp = pwl->data();
							//float       r = pwarp->GetPosition().z * majorRadius;
							float       r = 0.6f * majorRadius;
							float       angle = bias + offset[index++] * (2.0f * pi) / nWarps;
							// KG- added. dont move fixed position aleph
							if (pwarp->IsFixedPosition()) continue;

							Vector      position;
							position = Vector::RandomPosition(r * displacement);
							position.x  += cos(angle) * r;
							position.y  += sin(angle) * r;
							position.z  *= 1.2f;
							pwarp->SetPosition(position);
							Orientation o(-position);
							pwarp->SetOrientation(o);
							// fix rotating alephs
							Rotation rot(0,0,1,0);
							pwarp->SetRotation(rot);
							pwarp->GetHitTest()->UpdateStaticBB();
						}
					}
				}
			}
		}
		//---------------------------------------
        float   amountHe3;
        {
            float   nHe3 = 0.0f;
            for (ClusterLinkIGC* pcl = m_clusters.first(); (pcl != NULL); pcl = pcl->next())
            {
                IclusterIGC*            pcluster = pcl->data();
                const AsteroidListIGC*  pasteroids = pcluster->GetAsteroids();
                if (pasteroids->n() == 0)
                {
                    nHe3 += float(pcluster->GetHomeSector()
                                  ? pmp->nPlayerSectorMineableAsteroids
                                  : pmp->nNeutralSectorMineableAsteroids);
                }
                else
                {
                    for (AsteroidLinkIGC*   pal = pasteroids->first(); (pal != NULL); pal = pal->next())
                    {
                        if (pal->data()->GetCapabilities() & c_aabmMineHe3)
                            nHe3 += 1.0f;
                    }
                }
            }

            if (nHe3 == 0.0f)
                amountHe3 = 0.0f;
            else
            {
                amountHe3 = GetFloatConstant(c_fcidAmountHe3) *
                            pmp->fHe3Density *
                            float(m_sides.n()) / nHe3;
            }
        }

        const StationTypeListIGC*   pstationtypes = GetStationTypes();
        for (ClusterLinkIGC* pcl = m_clusters.first(); (pcl != NULL); pcl = pcl->next())
        {
            IclusterIGC*    pcluster = pcl->data();

            {
                //Replace stations with ones they actually can get
                //Walk the list backwards because changing a station moves it to the end of the list.
                StationLinkIGC*   psl = pcluster->GetStations()->last();
                while (psl != NULL)
                {
                    IstationIGC*            pstation = psl->data();
                    psl = psl->txen();

                    IsideIGC*               pside = pstation->GetSide();
                    IstationTypeIGC*        pst = pstation->GetBaseStationType();

                    TechTreeBitMask  ttbm = pside->GetDevelopmentTechs() |
                                            (pst->GetLocalTechs() | pst->GetEffectTechs());
                    if (!(pst->GetRequiredTechs() <= ttbm))
                    {
                        StationClassID classID = pst->GetClassID();

                        for (StationTypeLinkIGC*    pstl = pstationtypes->first(); (pstl != NULL); pstl = pstl->next())
                        {
                            IstationTypeIGC*    pstPossible = pstl->data();
                            if ((pstPossible->GetClassID() == classID) &&
                                (pstPossible->GetRequiredTechs() <= ttbm))
                            {
                                pstation->SetBaseStationType(pstPossible);
                                break;
                            }
                        }
                    }
                }
            }

            PopulateCluster(this, pmp, pcluster,
                            amountHe3,
                            (pcluster->GetAsteroids()->n() == 0),
                            (pcluster->GetTreasures()->n() == 0));

            pcluster->SetActive(true);
        }

        if (pmp->bShowMap) 
        {
            for (WarpLinkIGC * pwl = m_warps.first();
                (pwl != NULL);
                pwl = pwl->next())
            {
                IwarpIGC*   pwarp = pwl->data();

                for (SideLinkIGC * psl = m_sides.first();
                    (psl != NULL);
                    psl = psl->next())
                {
                    pwarp->SetSideVisibility(psl->data(), true);
                }
            }
        }
    }

    //Go through and set the territory amounts for each side
    if (pmp->IsTerritoryGame())
    {
        assert (c_cSidesMax == 6);
        unsigned char nTerritoriesPerSide[c_cSidesMax] = {0, 0, 0, 0, 0, 0};

        for (ClusterLinkIGC*    pcl = m_clusters.first(); (pcl != NULL); pcl = pcl->next())
        {
            IclusterIGC*    pcluster = pcl->data();

            IsideIGC*       psideOwner = NULL;
            StationLinkIGC*    psl;
            for (psl = pcluster->GetStations()->first(); (psl != NULL); psl = psl->next())
            {
                IsideIGC*   pside = psl->data()->GetSide();
                if (psideOwner == NULL)
                    psideOwner = pside;
                else if (psideOwner != pside)
                    break;
            }

            if (psideOwner && (psl == NULL))
                nTerritoriesPerSide[psideOwner->GetObjectID()]++;
        }

        for (SideLinkIGC*   psl = m_sides.first(); (psl != NULL); psl = psl->next())
            psl->data()->SetTerritoryCount(nTerritoriesPerSide[psl->data()->GetObjectID()]);
    }
	// mmf log mission params server side via debugf's
	// may want to revisit this and make it a member function
    debugf("Name: %s\n",pmp->strGameName);
    debugf("Core Name: %s\n",pmp->szIGCStaticFile);
	debugf("Lives: %d\n",pmp->iLives);
	debugf("Map Type: %s\n",pmp->szCustomMapFile);
	debugf("Map Connectivity: %d\n",pmp->iRandomEncounters);
	debugf("Resources: %d\n",pmp->iResources);
	debugf("Total money: %f\n",pmp->fHe3Density);
	debugf("Player Sector He3 rocks: %d\n",pmp->nPlayerSectorMineableAsteroids);
	debugf("Neutral Sector He3 rocks: %d\n",pmp->nNeutralSectorMineableAsteroids);
	debugf("Max Drones: %d\n",pmp->nMaxDronesPerTeam);
    debugf("Scores Count: %s\n",pmp->bScoresCount ? "yes" : "no");
    debugf("Eject Pods: %s\n",pmp->bEjectPods ? "yes" : "no");
    debugf("Allow Friendly Fire: %s\n",pmp->bAllowFriendlyFire ? "yes" : "no");
    debugf("Allow Defections: %s\n",pmp->bAllowDefections ? "yes" : "no");
	debugf("Allow Joiners: %s\n",pmp->bAllowJoiners ? "yes" : "no");
    debugf("Invulnerable Stations: %s\n",pmp->bInvulnerableStations ? "yes" : "no");
    debugf("Developments: %s\n",pmp->bAllowDevelopments ? "yes" : "no");
    debugf("Allow Shipyards: %s\n",pmp->bAllowShipyardPath ? "yes" : "no");
    debugf("Experimental: %s\n",pmp->bExperimental ? "yes" : "no");  // mmf 10/07 Experimental game type
}

void    CmissionIGC::GenerateTreasure(Time         now,
                                      IclusterIGC* pCluster,
                                      short        tsi)
{
    DataTreasureIGC dt;
    if (tsi >= 0)
    {
        ItreasureSetIGC*    pts = GetTreasureSet(tsi);
        if (pts)
        {
            // Pick a random part ID given the treasure distribution
            // specified by the treasure chance.
            const TreasureData& td = pts->GetRandomTreasureData();
            dt.treasureCode = td.treasureCode;
            dt.treasureID = td.treasureID;
        }
        else
            return;
    }
    else if (tsi == NA)
        return;
    else
    {
        dt.treasureCode = c_tcFlag;
        dt.treasureID = SIDE_TEAMLOBBY;
    }

    if ((dt.treasureCode != c_tcCash) || (m_missionParams.bAllowDevelopments))
    {
        {
            int                 nTriesLeft = (dt.treasureCode != c_tcFlag) ? 3 : 0x7fffffff;    //Tries lots & lots for flags  
            const ModelListIGC* models = pCluster->GetModels();

            float   radius = c_radiusUniverse;

            ModelLinkIGC*   pmlink;
            do
            {
                //Pick a random position for the asteroid
                //in a squashed disk
                dt.p0 = Vector::RandomDirection();

                //Start 0.1 ... 1 the universe radius
                dt.p0 *= radius * pow(random(0.001f, 1.0f), 1.0f/3.0f);
                dt.p0.z *= c_zLensMultiplier;

                //Verify that it is not close to any other model in the cluster
                for (pmlink = models->first(); (pmlink != NULL); pmlink = pmlink->next())
                {
                    ImodelIGC*  pm = pmlink->data();
                    float       r = (pm->GetObjectType() == OT_ship         //Never create a treasure starting within 500*sqrt(10) of a player
                                     ? 500.0f
                                     : 25.0f) + pm->GetRadius();

                    if ((pm->GetPosition() - dt.p0).LengthSquared() < 10.0f * r * r)  //Lots of extra space (sqrt(10))
                    {
                        //Too close ... try again
                        radius *= 1.05f;
                        break;
                    }
                }
            }
            while (pmlink && (--nTriesLeft > 0));

            if (nTriesLeft == 0)
                return;     //Couldn't find a spot for a treasure
        }

        dt.amount = NA;

        dt.clusterID = pCluster->GetObjectID();
        dt.lifespan = (dt.treasureCode != c_tcFlag) ? random(300.0f, 600.0f) : 10.0f * 24.0f * 3600.0f;
        dt.time0 = now;

        dt.objectID = GenerateNewTreasureID();

        dt.v0 = Vector::GetZero();

        IObject*    o = CreateObject(now, OT_treasure,
                                     &dt, sizeof(dt));
        assert (o);
        o->Release();
    }
}


short                   CmissionIGC::GetReplayCount(void) const
{
   return m_nReplayCount;
}


const char*             CmissionIGC::GetContextName(void)
{
  // Return if it has already been generated
  if (!m_strContextName.IsEmpty())
     return m_strContextName;

  // Get the mission creation time
  SYSTEMTIME st;
  ZSucceeded(::FileTimeToSystemTime(&m_ftCreated, &st));

  // Format the context string
  char szContext[24];
  sprintf(szContext, "%c%02d%02d%02d%02d%02d%04d%02d",
      (GetMissionParams()->bObjectModelCreated && GetMissionParams()->bClubGame) ? 'Z' : 'U',
      st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, GetMissionID() % 10000, GetReplayCount() % 100);

  // Save the context string for next time
  m_strContextName = szContext;

  // Return the string (becomes invalid when m_strContextName changes)
  return m_strContextName;
}

//Imago NYI: TypesOfDamage.csv  - Core devs want to define the defense/damage, new OT_damageType:
//  DamageType = {          DefenseType = {
//      char name[maxname]      char name[maxname]
//      float value             float value
//      short id                short id
//  }                       }
ZString ConstantsDamageComment(int defenseid, int damageid) {
	ZString strComment = "Uncommented damage constant";
	ZString strComment2 = "Uncommented defense constant";
	switch(damageid) {
		case 0: {strComment = "Asteroid";} break;
		case 1: {strComment = "Plasma";} break;
		case 2: {strComment = "Gattling";} break;
		case 3: {strComment = "Disruptor";} break;
		case 4: {strComment = "Sniper";} break;
		case 5: {strComment = "Mini";} break;
		case 6: {strComment = "Cannon";} break;
		case 7: {strComment = "Galv";} break;
		case 8: {strComment = "Anti Base";} break;
		case 9: {strComment = "Seismic";} break;
		case 10: {strComment = "Nan";} break;
		case 11: {strComment = "AC Turret";} break;
		case 12: {strComment = "EMP";} break;
		case 13: {strComment = "More EMP";} break;
		case 14: {strComment = "Gas/HTT";} break;
		case 15: {strComment = "SkyRip";} break;
		case 16: {strComment = "Mine";} break;
		case 17: {strComment = "Stinger";} break;
		case 18: {strComment = "Lancer";} break;
		case 19: {strComment = "Ion";} break;
		default:
			break;
	}
	switch(defenseid) {
		case 0: {strComment2 = "Rock";} break;
		case 1: {strComment2 = "Light Ship";} break;
		case 2: {strComment2 = "Medium Ship";} break;
		case 3: {strComment2 = "Heavy Ship";} break;
		case 4: {strComment2 = "Super Heavy Ship";} break;
		case 5: {strComment2 = "Utility Ship";} break;
		case 6: {strComment2 = "Light Base";} break;
		case 7: {strComment2 = "Heavy Base";} break;
		case 8: {strComment2 = "Ship Shields";} break;
		case 9: {strComment2 = "Light Base Shields";} break;
		case 10: {strComment2 = "Heavy Base Shields";} break;
		case 11: {strComment2 = "Probe";} break;
		case 12: {strComment2 = "Ultra Light Base Armor";} break;
		case 13: {strComment2 = "Ultra Light Base Shields";} break;
		case 14: {strComment2 = "Large Ship Shields";} break;
	}

	if(defenseid >= 0) {
		return (damageid < 0) ? strComment2 : strComment + " vs. " + strComment2;
	}

	if(damageid >= 0) {
		return (defenseid < 0) ? strComment : strComment + " vs. " + strComment2;
	}
	return ZString("Invincible");
}

// Imago dump IGC data to text
void Obj2Txt(IbaseIGC * pIGC, ObjectType ot, ImissionIGC * pMission)
{
	ZString strCore = ZString(pMission->GetMissionParams()->szIGCStaticFile);
	switch(ot) {
		case OT_stationType: 
			{
				DataStationTypeIGC data;
				pIGC->Export(&data);
				DataBuyableIGC data2 = (DataBuyableIGC)data;
				ZString strLine = ZString(data.stationTypeID) + "\t"
					+ZString(data.signature) + "\t"
					+ZString(data.maxArmorHitPoints) + "\t"
					+ZString(data.maxShieldHitPoints) + "\t"
					+ZString(data.armorRegeneration) + "\t"
					+ZString(data.shieldRegeneration) + "\t"
					+ZString(data.scannerRange) + "\t"
					+ZString(data.income) + "\t"
					+ZString(data.radius) + "\t"
					+ZString(data.successorStationTypeID) + "\t"
					+ZString(data.defenseTypeArmor) + " (" + ConstantsDamageComment(data.defenseTypeArmor,-1) + ")\t"
					+ZString(data.defenseTypeShield)  + " (" + ConstantsDamageComment(data.defenseTypeShield,-1) + ")\t";
				
				ZString strCapabilities;
				if(data.sabmCapabilities & c_sabmUnload)
					strCapabilities += "CanOffloadMinerals ";
				if(data.sabmCapabilities & c_sabmStart)
					strCapabilities += "CanStartHere ";
				if(data.sabmCapabilities & c_sabmRestart)
					strCapabilities += "CanRestartHere ";
				if(data.sabmCapabilities & c_sabmRipcord)
					strCapabilities += "CanRipHere ";
				if(data.sabmCapabilities & c_sabmCapture)
					strCapabilities += "CanBeCaptured ";
				if(data.sabmCapabilities & c_sabmLand)
					strCapabilities += "CanLandHere ";
				if(data.sabmCapabilities & c_sabmRepair)
					strCapabilities += "CanRepair ";
				if(data.sabmCapabilities & c_sabmRemoteLeadIndicator)
					strCapabilities += "IsVisibleInLoadout ";
				if(data.sabmCapabilities & c_sabmReload)
					strCapabilities += "CanReload ";
				if(data.sabmCapabilities & c_sabmFlag)
					strCapabilities += "IsVictoryObjective ";
				if(data.sabmCapabilities & c_sabmPedestal)
					strCapabilities += "IsFlagPedestal ";
				if(data.sabmCapabilities & c_sabmTeleportUnload)
					strCapabilities += "CanTeleportOffloadMinerals ";
				if(data.sabmCapabilities & c_sabmCapLand)
					strCapabilities += "CanLandCapitols ";
				if(data.sabmCapabilities & c_sabmRescue)
					strCapabilities += "CanRescuePods ";
				if(data.sabmCapabilities & c_sabmRescueAny)
					strCapabilities += "CanRescueAnyPods ";

				strLine += strCapabilities + "\t";

				ZString strCapabilities1;
				if(data.aabmBuild & c_aabmBuildable)
					strCapabilities1 += "CanBeRegular ";
				if(data.aabmBuild & (c_aabmSpecial << 0))
					strCapabilities1 += "CanBeUranium ";
				if(data.aabmBuild & (c_aabmSpecial << 1))
					strCapabilities1 += "CanBeSilicon ";
				if(data.aabmBuild & (c_aabmSpecial << 2))
					strCapabilities1 += "CanBeCarbon ";
				if(data.aabmBuild & c_aabmMineHe3)
					strCapabilities1 += "CanBeHelium ";
				if(data.aabmBuild & c_aabmMineLotsHe3)
					strCapabilities1 += "CanBeDenseHelium ";
				if(data.aabmBuild & c_aabmMineGold)
					strCapabilities1 += "CanBeGold ";

				strLine += strCapabilities1 + "\t";

				strLine += ZString(data.classID) + "\t"
					+ZString(data.constructionDroneTypeID) + "\t";
					
				ZString strSFX = ZString(data.stationTypeID) + "\t"
					+ZString(data.constructorNeedRockSound) + "\t"
					+ZString(data.constructorUnderAttackSound) + "\t"
					+ZString(data.constructorDestroyedSound) + "\t"
					+ZString(data.completionSound) + "\t"
					+ZString(data.interiorSound) + "\t"
					+ZString(data.exteriorSound) + "\t"
					+ZString(data.interiorAlertSound) + "\t"
					+ZString(data.underAttackSound) + "\t"
					+ZString(data.criticalSound) + "\t"
					+ZString(data.destroyedSound) + "\t"
					+ZString(data.capturedSound) + "\t"
					+ZString(data.enemyCapturedSound) + "\t"
					+ZString(data.enemyDestroyedSound) + "\r\n";
					
				strLine += ZString(data.textureName) + "\t"
					+ZString(data.builderName) + "\t"
					+ZString(data2.price) + "\t"
					+ZString((int)data2.timeToBuild) + "\t"
					+ZString(data2.modelName) + "\t"
					+ZString(data2.iconName) + "\t"
					+ZString(data2.name) + "\t"
					+ZString(data2.description) + "\t"
					+ZString(data2.groupID) + "\t";
				strLine += pMission->BitsToTechsList(data.ttbmRequired) + "\t";
				strLine += pMission->BitsToTechsList(data.ttbmEffects) + "\t";
				strLine += pMission->BitsToTechsList(data.ttbmLocal) + "\r\n";

				ZString strFile = strCore + "\\TypesOfStations";
				char        szFilename[MAX_PATH + 1];
				HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true); //NYI check this!
				FILE*       file = fopen (szFilename, "ab");
				fwrite (strLine, sizeof(char), strLine.GetLength(), file);
				fclose (file);

				strFile = strCore + "\\StationsSFX";
				szFilename[MAX_PATH + 1];
				hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				file = fopen (szFilename, "ab");
				fwrite (strSFX, sizeof(char), strSFX.GetLength(), file);
				fclose (file);
			}
		break;
		case OT_projectileType:
			{
				DataProjectileTypeIGC data;
				pIGC->Export(&data);
				DataObjectIGC data2 = (DataObjectIGC)data;

				//NYI: just make the percentages
				ZString strColor = ZString(data2.color.r) + " " + ZString(data2.color.g) + " " + ZString(data2.color.b) + " " 
					+ ZString(data2.color.a);
				ZString strLine = ZString(data.projectileTypeID) + "\t"
					+ZString(data.power) + "\t"
					+ZString(data.blastPower) + "\t"
					+ZString(data.blastRadius) + "\t"
					+ZString(data.speed) + "\t"
					+ZString(data.lifespan) + "\t"
					+ZString(data.damageType) + " (" + ConstantsDamageComment(-1,data.damageType) + ")\t"
					+ZString(data.absoluteF) + "\t"
					+ZString(data.bDirectional) + "\t"
					+ZString(data.width) + "\t"
					+ZString(data.ambientSound) + "\t"
					+strColor + "\t"
					+ZString(data2.radius) + "\t"
					+ZString(data2.rotation) + "\t"
					+ZString(data2.modelName) + "\t"
					+ZString(data2.textureName) + "\t";
					
				//add the weapon/probe name (just for convenience/debugging)
				int iNotFound = 1;
				const ExpendableTypeListIGC* petlist = pMission->GetExpendableTypes();
				ExpendableTypeLinkIGC* petlink; 
				for (petlink = petlist->first(); petlink; petlink = petlink->next()) {
					if (petlink->data()->GetObjectType() == OT_probeType) {
						IexpendableTypeIGC* pexp = (IexpendableTypeIGC*)petlink->data();
						DataProbeTypeIGC* pdata  = (DataProbeTypeIGC*)pexp->GetData();
						if(pdata->projectileTypeID == data.projectileTypeID) {
							strLine +=ZString(pexp->GetName()) + " ";
							iNotFound = 0;
						}
					}
				}
				if(iNotFound) {
					const PartTypeListIGC* palist = pMission->GetPartTypes();
					PartTypeLinkIGC* pallink; 
					for (pallink = palist->first(); pallink; pallink = pallink->next()) {
						if (pallink->data()->GetEquipmentType() == ET_Weapon) {
							IpartTypeIGC* part = (IpartTypeIGC*)pallink->data();
							DataWeaponTypeIGC * pdata = (DataWeaponTypeIGC*)part->GetData();
							if(pdata->projectileTypeID == data.projectileTypeID) {
								strLine +=ZString(part->GetName()) + " ";
								iNotFound = 0;
								break;
							}
						}
					}
				}
				if(iNotFound) 
					strLine += "Unused\r\n";
				else
					strLine += "\r\n";
				
				ZString strFile = strCore + "\\TypesofProjectiles";
				char        szFilename[MAX_PATH + 1];
				HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				FILE*       file = fopen (szFilename, "ab");
				fwrite (strLine, sizeof(char), strLine.GetLength(), file);
				fclose (file);
			}
		break;
		case OT_missileType: 
			{
				DataMissileTypeIGC data;
				pIGC->Export(&data);
				DataExpendableTypeIGC data2 = (DataExpendableTypeIGC)data; // just for copy of name
				DataObjectIGC data3 = (DataObjectIGC)data2;
				LauncherDef data4 = data2.launcherDef;
				DataBuyableIGC data5 = (DataBuyableIGC)data4;
				ZString strColor = ZString(data3.color.r) + " " + ZString(data3.color.g) + " " + ZString(data3.color.b) + " " 
					+ ZString(data3.color.a);
				ZString strLine = ZString(data2.expendabletypeID) + "\t"
					+ZString(data.acceleration) + "\t"
					+ZString(data.turnRate) + "\t"
					+ZString(data.initialSpeed) + "\t"
					+ZString(data.lockTime) + "\t"
					+ZString(data.readyTime) + "\t"
					+ZString(data.maxLock) + "\t"
					+ZString(data.chaffResistance) + "\t"
					+ZString(data.dispersion) + "\t"
					+ZString(data.lockAngle) + "\t"
					+ZString(data.power) + "\t"
					+ZString(data.blastPower) + "\t"
					+ZString(data.blastRadius) + "\t"
					+ZString(data.width) + "\t"
					+ZString(data.damageType) + " (" + ConstantsDamageComment(-1,data.damageType) + ")\t"
					+ZString(data.bDirectional) + "\t"
					//
					+ZString(data.launchSound) + "\t"
					+ZString(data.ambientSound) + "\t"
					//
					+ZString(data2.loadTime) + "\t"
					+ZString(data2.lifespan) + "\t"
					+ZString(data2.signature) + "\t"
					+ZString(data2.hitPoints) + "\t"
					+ZString(data2.defenseType) + " (" + ConstantsDamageComment(data2.defenseType,-1)+ ")\t";

				ZString strCapabilities;
				if(data2.eabmCapabilities & c_eabmCapture)
					strCapabilities += "CanCaptureStations ";
				if(data2.eabmCapabilities & c_eabmWarpBombDual)
					strCapabilities += "CanWarpBombDual ";
				if(data2.eabmCapabilities & c_eabmWarpBombSingle)
					strCapabilities += "CanWarpBombSingle ";
				if(data2.eabmCapabilities & c_eabmQuickReady)
					strCapabilities += "CanQuickReady ";
				if(data2.eabmCapabilities & c_eabmRipcord)
					strCapabilities += "CanRipcord ";
				if(data2.eabmCapabilities & c_eabmShootStations)
					strCapabilities += "CanShootStations ";
				if(data2.eabmCapabilities & c_eabmShootShips)
					strCapabilities += "CanShootShips ";
				if(data2.eabmCapabilities & c_eabmShootMissiles)
					strCapabilities += "CanShootMissiles ";
				if(data2.eabmCapabilities & c_eabmShootOnlyTarget)
					strCapabilities += "CanShootTargetOnly ";
				if(data2.eabmCapabilities & c_eabmRescue)
					strCapabilities += "CanRescue ";
				if(data2.eabmCapabilities & c_eabmRescueAny)
					strCapabilities += "CanRescueAny ";

				strLine += strCapabilities + "\t"
					+ZString(data2.iconName) + "\t"   
					+strColor + "\t"
					+ZString(data3.radius) + "\t"
					+ZString(data3.rotation) + "\t"
					+ZString(data3.modelName) + "\t"
					+ZString(data3.textureName) + "\t" 
					+ZString(data4.signature) + "\t"
					+ZString(data4.mass) + "\t";
					
				strLine += pMission->BitsToPartsList(data4.partMask,ET_Magazine) + "\t";

				strLine += ZString(data4.expendableSize) + "\t"
					+ZString(data5.name) + "\r\n";

				ZString strFile = strCore + "\\ExpendableMissiles";
				char        szFilename[MAX_PATH + 1];
				HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				FILE*       file = fopen (szFilename, "ab");
				fwrite (strLine, sizeof(char), strLine.GetLength(), file);
				fclose (file);

			}
		break;
		case OT_mineType:
			{
				DataMineTypeIGC data;
				pIGC->Export(&data);

				DataExpendableTypeIGC data2 = (DataExpendableTypeIGC)data;
				DataObjectIGC data3 = (DataObjectIGC)data2;
				LauncherDef data4 = data2.launcherDef;
				DataBuyableIGC data5 = (DataBuyableIGC)data4;
				ZString strColor = ZString(data3.color.r) + " " + ZString(data3.color.g) + " " + ZString(data3.color.b) + " " 
					+ ZString(data3.color.a);
				ZString strLine = ZString(data2.expendabletypeID) + "\t"
					+ZString(data.radius) + "\t"
					+ZString(data.power) + "\t"
					+ZString(data.endurance) + "\t"
					+ZString(data.damageType) + " (" + ConstantsDamageComment(-1,data.damageType) + ")\t"
					+ZString(data2.loadTime) + "\t"
					+ZString(data2.lifespan) + "\t"
					+ZString(data2.signature) + "\t"
					+ZString(data2.hitPoints) + "\t"
					+ZString(data2.defenseType) + " (" + ConstantsDamageComment(data2.defenseType,-1)+ ")\t";

				ZString strCapabilities;
				if(data2.eabmCapabilities & c_eabmCapture)
					strCapabilities += "CanCaptureStations ";
				if(data2.eabmCapabilities & c_eabmWarpBombDual)
					strCapabilities += "CanWarpBombDual ";
				if(data2.eabmCapabilities & c_eabmWarpBombSingle)
					strCapabilities += "CanWarpBombSingle ";
				if(data2.eabmCapabilities & c_eabmQuickReady)
					strCapabilities += "CanQuickReady ";
				if(data2.eabmCapabilities & c_eabmRipcord)
					strCapabilities += "CanRipcord ";
				if(data2.eabmCapabilities & c_eabmShootStations)
					strCapabilities += "CanShootStations ";
				if(data2.eabmCapabilities & c_eabmShootShips)
					strCapabilities += "CanShootShips ";
				if(data2.eabmCapabilities & c_eabmShootMissiles)
					strCapabilities += "CanShootMissiles ";
				if(data2.eabmCapabilities & c_eabmShootOnlyTarget)
					strCapabilities += "CanShootTargetOnly ";
				if(data2.eabmCapabilities & c_eabmRescue)
					strCapabilities += "CanRescue ";
				if(data2.eabmCapabilities & c_eabmRescueAny)
					strCapabilities += "CanRescueAny ";

				strLine += strCapabilities + "\t"
					+ZString(data2.iconName) + "\t"   
					+strColor + "\t"
					+ZString(data3.radius) + "\t"
					+ZString(data3.rotation) + "\t"
					+ZString(data3.modelName) + "\t"
					+ZString(data3.textureName) + "\t" 
					+ZString(data4.signature) + "\t"
					+ZString(data4.mass) + "\t"
					+pMission->BitsToPartsList(data4.partMask,ET_Dispenser) + "\t"
					+ZString(data4.expendableSize) + "\t"
					+ZString(data5.name) + "\r\n";

				ZString strFile = strCore + "\\ExpendableMines";
				char        szFilename[MAX_PATH + 1];
				HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				FILE*       file = fopen (szFilename, "ab");
				fwrite (strLine, sizeof(char), strLine.GetLength(), file);
				fclose (file);

			}
		break;
		case OT_chaffType:
			{
				DataChaffTypeIGC data;
				pIGC->Export(&data);

				DataExpendableTypeIGC data2 = (DataExpendableTypeIGC)data;
				DataObjectIGC data3 = (DataObjectIGC)data2;
				LauncherDef data4 = data2.launcherDef;
				DataBuyableIGC data5 = (DataBuyableIGC)data4;
				ZString strColor = ZString(data3.color.r) + " " + ZString(data3.color.g) + " " + ZString(data3.color.b) + " " 
					+ ZString(data3.color.a);

				ZString strLine = ZString(data2.expendabletypeID) + "\t"
					+ZString(data.chaffStrength) + "\t" 
					+ZString(data2.loadTime) + "\t"
					+ZString(data2.lifespan) + "\t"
					+ZString(data2.signature) + "\t"
					+ZString(data2.hitPoints) + "\t"
					+ZString(data2.defenseType) + " (" + ConstantsDamageComment(data2.defenseType,-1)+ ")\t";

				ZString strCapabilities;
				if(data2.eabmCapabilities & c_eabmCapture)
					strCapabilities += "CanCaptureStations ";
				if(data2.eabmCapabilities & c_eabmWarpBombDual)
					strCapabilities += "CanWarpBombDual ";
				if(data2.eabmCapabilities & c_eabmWarpBombSingle)
					strCapabilities += "CanWarpBombSingle ";
				if(data2.eabmCapabilities & c_eabmQuickReady)
					strCapabilities += "CanQuickReady ";
				if(data2.eabmCapabilities & c_eabmRipcord)
					strCapabilities += "CanRipcord ";
				if(data2.eabmCapabilities & c_eabmShootStations)
					strCapabilities += "CanShootStations ";
				if(data2.eabmCapabilities & c_eabmShootShips)
					strCapabilities += "CanShootShips ";
				if(data2.eabmCapabilities & c_eabmShootMissiles)
					strCapabilities += "CanShootMissiles ";
				if(data2.eabmCapabilities & c_eabmShootOnlyTarget)
					strCapabilities += "CanShootTargetOnly ";
				if(data2.eabmCapabilities & c_eabmRescue)
					strCapabilities += "CanRescue ";
				if(data2.eabmCapabilities & c_eabmRescueAny)
					strCapabilities += "CanRescueAny ";

				strLine += strCapabilities + "\t"
					+ZString(data2.iconName) + "\t"   
					+strColor + "\t"
					+ZString(data3.radius) + "\t"
					+ZString(data3.rotation) + "\t"
					+ZString(data3.modelName) + "\t"
					+ZString(data3.textureName) + "\t" 
					+ZString(data4.signature) + "\t"
					+ZString(data4.mass) + "\t"
					+ pMission->BitsToPartsList(data4.partMask,ET_ChaffLauncher) + "\t"
					+ZString(data4.expendableSize) + "\t"
					+ZString(data5.name) + "\r\n";

				ZString strFile = strCore + "\\ExpendableChaff";
				char        szFilename[MAX_PATH + 1];
				HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				FILE*       file = fopen (szFilename, "ab");
				fwrite (strLine, sizeof(char), strLine.GetLength(), file);
				fclose (file);

			}
		break;
		case OT_probeType:
			{
				DataProbeTypeIGC data;
				pIGC->Export(&data);
				
				DataExpendableTypeIGC data2 = (DataExpendableTypeIGC)data;  //NYI: the psuedo OT_expendableType (no copy/paste)
				DataObjectIGC data3 = (DataObjectIGC)data2;
				LauncherDef data4 = data2.launcherDef;
				DataBuyableIGC data5 = (DataBuyableIGC)data4;
				ZString strColor = ZString(data3.color.r) + " " + ZString(data3.color.g) + " " + ZString(data3.color.b) + " " 
					+ ZString(data3.color.a);

				ZString strLine = ZString(data2.expendabletypeID) + "\t" 
					+ZString(data.scannerRange) + "\t" 
					+ZString(data.dtimeBurst) + "\t" 
					+ZString(data.dispersion) + "\t" 
					+ZString(data.accuracy) + "\t" 
					+ZString(data.ammo) + "\t" 
					+ZString(data.projectileTypeID) + "\t" 
					+ZString(data.ambientSound) + "\t" 
					+ZString(data.dtRipcord) + "\t" 
					+ZString(data2.loadTime) + "\t"
					+ZString(data2.lifespan) + "\t"
					+ZString(data2.signature) + "\t"
					+ZString(data2.hitPoints) + "\t"
					+ZString(data2.defenseType) + " (" + ConstantsDamageComment(data2.defenseType,-1)+ ")\t";

				ZString strCapabilities;
				if(data2.eabmCapabilities & c_eabmCapture)
					strCapabilities += "CanCaptureStations ";
				if(data2.eabmCapabilities & c_eabmWarpBombDual)
					strCapabilities += "CanWarpBombDual ";
				if(data2.eabmCapabilities & c_eabmWarpBombSingle)
					strCapabilities += "CanWarpBombSingle ";
				if(data2.eabmCapabilities & c_eabmQuickReady)
					strCapabilities += "CanQuickReady ";
				if(data2.eabmCapabilities & c_eabmRipcord)
					strCapabilities += "CanRipcord ";
				if(data2.eabmCapabilities & c_eabmShootStations)
					strCapabilities += "CanShootStations ";
				if(data2.eabmCapabilities & c_eabmShootShips)
					strCapabilities += "CanShootShips ";
				if(data2.eabmCapabilities & c_eabmShootMissiles)
					strCapabilities += "CanShootMissiles ";
				if(data2.eabmCapabilities & c_eabmShootOnlyTarget)
					strCapabilities += "CanShootTargetOnly ";
				if(data2.eabmCapabilities & c_eabmRescue)
					strCapabilities += "CanRescue ";
				if(data2.eabmCapabilities & c_eabmRescueAny)
					strCapabilities += "CanRescueAny ";

				strLine += strCapabilities + "\t"
					+ZString(data2.iconName) + "\t"   
					+strColor + "\t"
					+ZString(data3.radius) + "\t"
					+ZString(data3.rotation) + "\t"
					+ZString(data3.modelName) + "\t"
					+ZString(data3.textureName) + "\t" 
					+ZString(data4.signature) + "\t"
					+ZString(data4.mass) + "\t"
					+ pMission->BitsToPartsList(data4.partMask,ET_Dispenser) + "\t"
					+ZString(data4.expendableSize) + "\t"
					+ZString(data2.launcherDef.name) + "\r\n";

				ZString strFile = strCore + "\\ExpendableProbes";
				char        szFilename[MAX_PATH + 1];
				HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				FILE*       file = fopen (szFilename, "ab");
				fwrite (strLine, sizeof(char), strLine.GetLength(), file);
				fclose (file);
			}
		break;
		case OT_partType: // NYI: OT_launcherType
			{ 
				// *FACEPALM* launcher types are in the part types list
				IpartTypeIGC*    ppart = (IpartTypeIGC*)pIGC;			
				EquipmentType et = ppart->GetEquipmentType();
				DataPartTypeIGC* pdata = (DataPartTypeIGC*)ppart->GetData();

				ZString strLine = ZString(pdata->partID) + "\t";
				
				
				// either use launchertype data or specific parttype data
				if ((et == ET_ChaffLauncher) || (et == ET_Magazine) || (et == ET_Dispenser)) {

					//replicate part's buy data
					DataBuyableIGC* pdata2 = (DataBuyableIGC*)ppart->GetData();
					DataLauncherTypeIGC data;
					pIGC->Export(&data);

					//override id to expendable ID for launcher types
					strLine = ZString(data.expendabletypeID) + "\t" 
						+ZString(data.partID) + "\t"
						+ZString(data.amount) + "\t"
						+ZString(data.successorPartID) + "\t"
						+ZString(data.launchCount) + "\t"
						+ZString(data.inventoryLineMDL) + "\t"
						+ZString(pdata2->price) + "\t"
						+ZString((int)pdata2->timeToBuild) + "\t"
						+ZString(pdata2->modelName) + "\t"
						+ZString(pdata2->iconName) + "\t"
						+ZString(pdata2->name) + "\t"
						+ZString(pdata2->description) + "\t"
						+ZString(pdata2->groupID) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmRequired) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmEffects) + "\r\n";

					ZString strFile = strCore + "\\Expendables";
					char        szFilename[MAX_PATH + 1];
					HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
					FILE*       file = fopen (szFilename, "ab");
					fwrite (strLine, sizeof(char), strLine.GetLength(), file);
					fclose (file);
				} else if(et == ET_Cloak) {
					//not using PartType's implementation of ::Export
					DataBuyableIGC* pdata2 = (DataBuyableIGC*)pdata;
					DataCloakTypeIGC* pdata3 = (DataCloakTypeIGC*)pdata;

					strLine += ZString(pdata3->energyConsumption) + "\t"
						+ZString(pdata3->maxCloaking) + "\t"
						+ZString(pdata3->onRate) + "\t"
						+ZString(pdata3->offRate) + "\t"
						//
						+ZString(pdata3->engageSound) + "\t"
						+ZString(pdata3->disengageSound) + "\t"
						//
						+ZString(pdata->mass) + "\t"
						+ZString(pdata->signature) + "\t"
						+ZString(pdata->successorPartID) + "\t"
						+pMission->BitsToPartsList(pdata->partMask,ET_Cloak) + "\t"
						+ZString(pdata->inventoryLineMDL) + "\t"
						+ZString(pdata2->price) + "\t"
						+ZString((int)pdata2->timeToBuild) + "\t"
						+ZString(pdata2->modelName) + "\t"
						+ZString(pdata2->iconName) + "\t"
						+ZString(pdata2->name) + "\t"
						+ZString(pdata2->description) + "\t"
						+ZString(pdata2->groupID) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmRequired) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmEffects) + "\r\n";
					
					ZString strFile = strCore + "\\Cloaks";
					char        szFilename[MAX_PATH + 1];
					HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
					FILE*       file = fopen (szFilename, "ab");
					fwrite (strLine, sizeof(char), strLine.GetLength(), file);
					fclose (file);

				} else if(et == ET_Afterburner) { 
					//not using PartType's implementation of ::Export
					DataBuyableIGC* pdata2 = (DataBuyableIGC*)pdata;
					DataAfterburnerTypeIGC* pdata3 = (DataAfterburnerTypeIGC*)pdata;

					strLine += ZString(pdata3->fuelConsumption) + "\t"
						+ZString(pdata3->maxThrust) + "\t"
						+ZString(pdata3->onRate) + "\t"
						+ZString(pdata3->offRate) + "\t"
						//
						+ZString(pdata3->interiorSound) + "\t"
						+ZString(pdata3->exteriorSound) + "\t"
						//
						+ZString(pdata->mass) + "\t"
						+ZString(pdata->signature) + "\t"
						+ZString(pdata->successorPartID) + "\t"
						+pMission->BitsToPartsList(pdata->partMask,ET_Afterburner) + "\t"
						+ZString(pdata->inventoryLineMDL) + "\t"
						+ZString(pdata2->price) + "\t"
						+ZString((int)pdata2->timeToBuild) + "\t"
						+ZString(pdata2->modelName) + "\t"
						+ZString(pdata2->iconName) + "\t"
						+ZString(pdata2->name) + "\t"
						+ZString(pdata2->description) + "\t"
						+ZString(pdata2->groupID) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmRequired) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmEffects) + "\r\n";
					
					ZString strFile = strCore + "\\Afterburners";
					char        szFilename[MAX_PATH + 1];
					HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
					FILE*       file = fopen (szFilename, "ab");
					fwrite (strLine, sizeof(char), strLine.GetLength(), file);
					fclose (file);		
				} else if(et == ET_Pack) { 
				//not using PartType's implementation of ::Export
					DataBuyableIGC* pdata2 = (DataBuyableIGC*)pdata;
					DataPackTypeIGC* pdata3 = (DataPackTypeIGC*)pdata;

					strLine += ZString(pdata3->packType) + "\t"
						+ZString(pdata3->amount) + "\t"
						+ZString(pdata->mass) + "\t"
						+ZString(pdata->signature) + "\t"
						+ZString(pdata->successorPartID) + "\t"
						+pMission->BitsToPartsList(pdata->partMask,ET_Pack) + "\t"
						+ZString(pdata->inventoryLineMDL) + "\t"
						+ZString(pdata2->price) + "\t"
						+ZString((int)pdata2->timeToBuild) + "\t"
						+ZString(pdata2->modelName) + "\t"
						+ZString(pdata2->iconName) + "\t"
						+ZString(pdata2->name) + "\t"
						+ZString(pdata2->description) + "\t"
						+ZString(pdata2->groupID) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmRequired) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmEffects) + "\r\n";
					
					ZString strFile = strCore + "\\Packs";
					char        szFilename[MAX_PATH + 1];
					HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
					FILE*       file = fopen (szFilename, "ab");
					fwrite (strLine, sizeof(char), strLine.GetLength(), file);
					fclose (file);	
				} else if(et == ET_Shield) { 
					//not using PartType's implementation of ::Export
					DataBuyableIGC* pdata2 = (DataBuyableIGC*)pdata;
					DataShieldTypeIGC* pdata3 = (DataShieldTypeIGC*)pdata;

					strLine += ZString(pdata3->rateRegen) + "\t"
						+ZString(pdata3->maxStrength) + "\t"
						+ZString(pdata3->defenseType) + " (" + ConstantsDamageComment(pdata3->defenseType,-1)+ ")\t"
						//
						+ZString(pdata3->activateSound) + "\t"
						+ZString(pdata3->deactivateSound) + "\t"
						//
						+ZString(pdata->mass) + "\t"
						+ZString(pdata->signature) + "\t"
						+ZString(pdata->successorPartID) + "\t"
						+pMission->BitsToPartsList(pdata->partMask,ET_Shield) + "\t"
						+ZString(pdata->inventoryLineMDL) + "\t"
						+ZString(pdata2->price) + "\t"
						+ZString((int)pdata2->timeToBuild) + "\t"
						+ZString(pdata2->modelName) + "\t"
						+ZString(pdata2->iconName) + "\t"
						+ZString(pdata2->name) + "\t"
						+ZString(pdata2->description) + "\t"
						+ZString(pdata2->groupID) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmRequired) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmEffects) + "\r\n";
					
					ZString strFile = strCore + "\\Shields";
					char        szFilename[MAX_PATH + 1];
					HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
					FILE*       file = fopen (szFilename, "ab");
					fwrite (strLine, sizeof(char), strLine.GetLength(), file);
					fclose (file);				
				} else if(et == ET_Weapon) { 
					//not using PartType's implementation of ::Export
					DataBuyableIGC* pdata2 = (DataBuyableIGC*)pdata;
					DataWeaponTypeIGC* pdata3 = (DataWeaponTypeIGC*)pdata;

					strLine += ZString(pdata3->dtimeReady) + "\t"
						+ZString(pdata3->dtimeBurst) + "\t"
						+ZString(pdata3->energyPerShot) + "\t"
						+ZString(pdata3->dispersion) + "\t"
						+ZString(pdata3->cAmmoPerShot) + "\t"
						+ZString(pdata3->projectileTypeID) + "\t"
						+ZString(pdata3->activateSound) + "\t"
						+ZString(pdata3->singleShotSound) + "\t"
						+ZString(pdata3->burstSound) + "\t"
						+ZString(pdata->mass) + "\t"
						+ZString(pdata->signature) + "\t"
						+ZString(pdata->successorPartID) + "\t"
						+pMission->BitsToPartsList(pdata->partMask,ET_Weapon) + "\t"
						+ZString(pdata->inventoryLineMDL) + "\t"
						+ZString(pdata2->price) + "\t"
						+ZString((int)pdata2->timeToBuild) + "\t"
						+ZString(pdata2->modelName) + "\t"
						+ZString(pdata2->iconName) + "\t"
						+ZString(pdata2->name) + "\t"
						+ZString(pdata2->description) + "\t"
						+ZString(pdata2->groupID) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmRequired) + "\t";
					strLine += pMission->BitsToTechsList(pdata2->ttbmEffects) + "\r\n";

					ZString strFile = strCore + "\\Weapons";
					char        szFilename[MAX_PATH + 1];
					HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
					FILE*       file = fopen (szFilename, "ab");
					fwrite (strLine, sizeof(char), strLine.GetLength(), file);
					fclose (file);
				
				} else {
					debugf("Obj2Txt: unhandled equipment type %i\n",et);
				}
			}
		break;
		case OT_hullType: 
			{
				//not using HullType's implementation of ::Export
				IhullTypeIGC*    phull = (IhullTypeIGC*)pIGC;	
				DataHullTypeIGC* pdata = (DataHullTypeIGC*)phull->GetData();
				DataBuyableIGC* pdata2 = (DataBuyableIGC*)pdata;

				// dump ships hardpoints (proper order, turrets last (highest) hardpoint data array
				ZString strHardPoints;
				for (Mount i = pdata->maxWeapons; (i >= 1); i--)
				{
					const HardpointData& hp = phull->GetHardpointData(pdata->maxWeapons - i);
					PartMask pm = hp.partMask;
					strHardPoints += ZString(pIGC->GetObjectID()) + "\t"
						//
						+ZString(hp.interiorSound) + "\t"
						+ZString(hp.turnSound) + "\t"
						//
						+ZString(hp.frameName) + "\t"
						+ZString(hp.locationAbreviation) + "\t"
						+ pMission->BitsToPartsList(pm,ET_Weapon) + "\t"
						+ZString(hp.bFixed) + "\t"
						+ZString(pdata2->name) + " (" + ZString(pdata2->modelName) + ")\r\n";
				}
				ZString strFile = strCore + "\\Hardpoints";
				char        szFilename[MAX_PATH + 1];
				HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				FILE*       file = fopen (szFilename, "ab");
				fwrite (strHardPoints, sizeof(char), strHardPoints.GetLength(), file);
				fclose (file);

				ZString strMaxTurnRates;
				ZString strTurnTorques;
				for(int i=0;i<3;i++) {
					strMaxTurnRates += ZString(pdata->maxTurnRates[i]);
					strTurnTorques += ZString(pdata->turnTorques[i]);
					if (i != 2) {
						strMaxTurnRates += " ";
						strTurnTorques += " ";
					}
				}

				ZString strPreferredPartsTypes;
				for(int i=0;i<c_cMaxPreferredPartTypes;i++) {
					strPreferredPartsTypes += ZString(pdata->preferredPartsTypes[i]);
					if (i != c_cMaxPreferredPartTypes - 1) {
						strPreferredPartsTypes += " ";
					}
				}
				//Fixed copy/paste error on 8/10 thanks to Compellor
				ZString strPmEquipment = pMission->BitsToPartsList(pdata->pmEquipment[ET_ChaffLauncher],ET_ChaffLauncher) + "\t"
					+pMission->BitsToPartsList(pdata->pmEquipment[ET_Dispenser],ET_Dispenser) + "\t"
					+pMission->BitsToPartsList(pdata->pmEquipment[ET_Magazine],ET_Magazine) + "\t"
					+pMission->BitsToPartsList(pdata->pmEquipment[ET_Shield],ET_Shield) + "\t"
					+pMission->BitsToPartsList(pdata->pmEquipment[ET_Cloak],ET_Cloak) + "\t"
					+pMission->BitsToPartsList(pdata->pmEquipment[ET_Afterburner],ET_Afterburner) + "\t";

   				ZString strLine = ZString(pdata->hullID) + "\t"
					+ZString(pdata->mass) + "\t"
					+ZString(pdata->signature) + "\t"
					+ZString(pdata->speed) + "\t"
					+strMaxTurnRates + "\t"
					+strTurnTorques + "\t"
					+ZString(pdata->thrust) + "\t"
					+ZString(pdata->sideMultiplier) + "\t"
					+ZString(pdata->backMultiplier) + "\t"
					+ZString(pdata->scannerRange) + "\t"
					+ZString(pdata->maxFuel) + "\t"
					+ZString(pdata->ecm) + "\t"
					+ZString(pdata->length) + "\t"
					+ZString(pdata->maxEnergy) + "\t"
					+ZString(pdata->rechargeRate) + "\t"
					+ZString(pdata->ripcordSpeed) + "\t"
					+ZString(pdata->ripcordCost) + "\t"
					+ZString(pdata->maxAmmo) + "\t"
					+ZString(pdata->successorHullID) + "\t"
					+ZString(pdata->maxWeapons) + "\t"
					+ZString(pdata->maxFixedWeapons) + "\t"
					+ZString(pdata->hitPoints) + "\t"
					//+ZString(pdata->hardpointOffset) + "\t" //Imago doesn't need binary offsets with CSV!
					+ZString(pdata->defenseType) + " (" + ConstantsDamageComment(pdata->defenseType,-1) + ")\t"
					+ZString(pdata->capacityMagazine) + "\t"
					+ZString(pdata->capacityDispenser) + "\t"
					+ZString(pdata->capacityChaffLauncher) + "\t"
					+strPreferredPartsTypes + "\t";

				ZString strCapabilities;
				if(pdata->habmCapabilities & c_habmBoard)
					strCapabilities += "CanBoard ";
				if(pdata->habmCapabilities & c_habmRescue)
					strCapabilities += "CanRescue ";
				if(pdata->habmCapabilities & c_habmLifepod)
					strCapabilities += "IsPod ";
				if(pdata->habmCapabilities & c_habmCaptureThreat)
					strCapabilities += "CanCapture ";
				if(pdata->habmCapabilities & c_habmLandOnCarrier)
					strCapabilities += "CanLandOnCarrier ";
				if(pdata->habmCapabilities & c_habmNoRipcord)
					strCapabilities += "CanNotRip ";
				if(pdata->habmCapabilities & c_habmIsRipcordTarget)
					strCapabilities += "IsRipcordTarget ";
				if(pdata->habmCapabilities & c_habmFighter)
					strCapabilities += "IsFighter ";
				if(pdata->habmCapabilities & c_habmRemoteLeadIndicator)
					strCapabilities += "IsRemoteLeadIndicator ";
				if(pdata->habmCapabilities & c_habmThreatToStation)
					strCapabilities += "CanBomb ";
				if(pdata->habmCapabilities & c_habmCarrier)
					strCapabilities += "IsCarrier ";
				if(pdata->habmCapabilities & c_habmLeadIndicator)
					strCapabilities += "CanLeadIndicator ";
				if(pdata->habmCapabilities & c_habmIsLtRipcordTarget)
					strCapabilities += "IsLtRip ";
				if(pdata->habmCapabilities & c_habmCanLtRipcord)
					strCapabilities += "CanLtRip ";
				if(pdata->habmCapabilities & c_habmMiner)
					strCapabilities += "IsMiner ";
				if(pdata->habmCapabilities & c_habmBuilder)
					strCapabilities += "IsBuilder ";     
				
				strLine += strCapabilities + "\t"
					+ZString(pdata->textureName) + "\t"
					+strPmEquipment; // has trailing tab
					
				ZString strSFX = ZString(pdata->hullID) + "\t"
					+ZString(pdata2->name) + "\t"
					+ZString(pdata->interiorSound) + "\t"
					+ZString(pdata->exteriorSound) + "\t"
					+ZString(pdata->mainThrusterInteriorSound) + "\t"
					+ZString(pdata->mainThrusterExteriorSound) + "\t"
					+ZString(pdata->manuveringThrusterInteriorSound) + "\t"
					+ZString(pdata->manuveringThrusterExteriorSound) + "\r\n";

					strLine += ZString(pdata2->price) + "\t"
					+ZString((int)pdata2->timeToBuild) + "\t"
					+ZString(pdata2->modelName) + "\t"
					+ZString(pdata2->iconName) + "\t"
					+ZString(pdata2->name) + "\t"
					+ZString(pdata2->description) + "\t"
					+ZString(pdata2->groupID) + "\t";
				strLine += pMission->BitsToTechsList(pdata2->ttbmRequired) + "\t";
				strLine += pMission->BitsToTechsList(pdata2->ttbmEffects) + "\r\n";

				strFile = strCore + "\\TypesOfShips";
				szFilename[MAX_PATH + 1];
				hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				file = fopen (szFilename, "ab");
				fwrite (strLine, sizeof(char), strLine.GetLength(), file);
				fclose (file);

				strFile = strCore + "\\ShipsSFX";
				szFilename[MAX_PATH + 1];
				hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				file = fopen (szFilename, "ab");
				fwrite (strSFX, sizeof(char), strSFX.GetLength(), file);
				fclose (file);
			}
		break;
		case OT_development:
			{
				DataDevelopmentIGC data;
				pIGC->Export(&data);
				DataBuyableIGC data2 = (DataBuyableIGC)data;

				GlobalAttributeSet mtgas;
				mtgas.Initialize();
				if(!(mtgas == data.gas)) {
					ZString strGAS = ZString(pIGC->GetObjectID()) + "\t" + ZString(data2.name) + "\t";
					for(int i=0;i<c_gaMax;i++) {
						strGAS += ZString(data.gas.GetAttribute(i));
							if (i != c_gaMax - 1)
								strGAS += "\t";
					}
					strGAS += "\r\n";
					ZString strFile = strCore + "\\GlobalAttributes";
					char        szFilename[MAX_PATH + 1];
					HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
					FILE*       file = fopen (szFilename, "ab");
					fwrite (strGAS, sizeof(char), strGAS.GetLength(), file);
					fclose (file);
				}


				ZString strLine = ZString(data.developmentID) + "\t"
					//
					+ZString(data.completionSound) + "\t"
					//
					+ZString(data2.price) + "\t"
					+ZString((int)data2.timeToBuild) + "\t"
					+ZString(data2.modelName) + "\t"
					+ZString(data2.iconName) + "\t"
					+ZString(data2.name) + "\t"
					+ZString(data2.description) + "\t"
					+ZString(data2.groupID) + "\t";
				strLine += pMission->BitsToTechsList(data2.ttbmRequired) + "\t";
				strLine += pMission->BitsToTechsList(data2.ttbmEffects) + "\r\n";

				ZString strFile = strCore + "\\Developments";
				char szFilename[MAX_PATH + 1];
				HRESULT hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				FILE* file = fopen (szFilename, "ab");
				fwrite (strLine, sizeof(char), strLine.GetLength(), file);
				fclose (file);
			}
		break;
		case OT_droneType:
			{
				DataDroneTypeIGC data;
				pIGC->Export(&data);
				DataBuyableIGC data2 = (DataBuyableIGC)data;

				ZString strLine = ZString(data.droneTypeID) + "\t"
					+ZString(data.shootSkill) + "\t"
					+ZString(data.moveSkill) + "\t"
					+ZString(data.bravery) + "\t"
					+ZString(data.pilotType) + "\t"
					+ZString(data.hullTypeID) + "\t"
					+ZString(data.etidLaid) + "\t"
					+ZString(data2.price) + "\t"
					+ZString((int)data2.timeToBuild) + "\t"
					+ZString(data2.modelName) + "\t"
					+ZString(data2.iconName) + "\t"
					+ZString(data2.name) + "\t"
					+ZString(data2.description) + "\t"
					+ZString(data2.groupID) + "\t";
				strLine += pMission->BitsToTechsList(data2.ttbmRequired) + "\t";
				strLine += pMission->BitsToTechsList(data2.ttbmEffects) + "\r\n";

				ZString strFile = strCore + "\\TypesOfDrones";
				char        szFilename[MAX_PATH + 1];
				HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				FILE*       file = fopen (szFilename, "ab");
				fwrite (strLine, sizeof(char), strLine.GetLength(), file);
				fclose (file);

			}
		break;
		case OT_civilization:
			{ 
				DataCivilizationIGC data;
				pIGC->Export(&data);

				ZString strGAS = ZString(pIGC->GetObjectID()) + "\t" + ZString(data.name) + "\t";
				for(int i=0;i<c_gaMax;i++) {
					strGAS += ZString(data.gasBaseAttributes.GetAttribute(i));
					if (i != c_gaMax - 1)
						strGAS += "\t";
				}
				strGAS += "\r\n";
				ZString strFile = strCore + "\\GlobalAttributes";
				char        szFilename[MAX_PATH + 1];
				HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				FILE*       file = fopen (szFilename, "ab");
				fwrite (strGAS, sizeof(char), strGAS.GetLength(), file);
				fclose (file);

				ZString strLine = ZString(data.civilizationID) + "\t"
					+ZString(data.incomeMoney) + "\t"
					+ZString(data.bonusMoney) + "\t"
					+ZString(data.name) + "\t"
					+ZString(data.iconName) + "\t"
					+ZString(data.hudName) + "\t"
					+ZString(data.lifepod) + "\t"
					+ZString(data.initialStationTypeID) + "\t";
				strLine += pMission->BitsToTechsList(data.ttbmBaseTechs) + "\t";
				strLine += pMission->BitsToTechsList(data.ttbmNoDevTechs) + "\r\n";

				strFile = strCore + "\\Civilizations";
				szFilename[MAX_PATH + 1];
				hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				file = fopen (szFilename, "ab");
				fwrite (strLine, sizeof(char), strLine.GetLength(), file);
				fclose (file);
				
			}
		break;
		case OT_treasureSet:
			{
				ItreasureSetIGC*    ptset = (ItreasureSetIGC*)pIGC;
				int treasures = ptset->GetSize(); //more accuratley, NumOfTreasures
				int datasize =  int(sizeof(DataTreasureSetIGC) + treasures * sizeof(TreasureData));
				DataTreasureSetIGC* pdata = (DataTreasureSetIGC*)(new char [datasize]);
				pIGC->Export(pdata);
				TreasureData*   ptd = pdata->treasureData0();
				ZString strTreasures;

				for (short i = 0; (i < treasures); i++) {
					strTreasures += ZString((ptd + i)->treasureID) + " ";

					if((ptd + i)->treasureCode == c_tcPart)
						strTreasures += "Part ";
					if((ptd + i)->treasureCode == c_tcPowerup)
						strTreasures += "Powerup ";
					if((ptd + i)->treasureCode == c_tcDevelopment)
						strTreasures += "Development ";
					if((ptd + i)->treasureCode == c_tcCash)
						strTreasures += "Cash ";
					if((ptd + i)->treasureCode == c_tcFlag)
						strTreasures += "Flag ";

					strTreasures += ZString((ptd + i)->chance);
					if(i != treasures - 1)
						strTreasures += " ";
				}

				ZString strLine = ZString(pdata->treasureSetID) + "\t"
					+ZString(pdata->name) + "\t"
					+ZString(pdata->nTreasureData) + "\t"
					+ZString(pdata->bZoneOnly) + "\t"
					+strTreasures + "\r\n";

				ZString strFile = strCore + "\\TypesOfTreasures";
				char        szFilename[MAX_PATH + 1];
				HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
				FILE*       file = fopen (szFilename, "ab");
				fwrite (strLine, sizeof(char), strLine.GetLength(), file);
				fclose (file);
			}
		break;
		default: debugf("Obj2Txt: unhandled object type %i\n",ot);
	}
}

//...these meanings shouldn't change
ZString ConstantsGlobalComment(int constid) {
	ZString strComment = "New uncommented constant";
	switch(constid) {
		case 0: {strComment = "Universe (sizing) lens multiplier.";} break;
		case 1: {strComment = "Universe (sizing) radius modifier.";} break;
		case 2: {strComment = "Universe (sizing) outer bounds modifier.";} break;
		case 3: {strComment = "Ship (speed) modifier (exiting an aleph).";} break;
		case 4: {strComment = "Ship (speed) modifier (exiting a station).";} break;
		case 5: {strComment = "Shield (damage) 'downed' variable (determine if shields are down).";} break;
		case 6: {strComment = "Payday (economy) timer (# sec/payday).";} break;
		case 7: {strComment = "He3 asteroid (economy) capacity. ";} break;
		case 8: {strComment = "He3 mineral (economy) unit price (worth).";} break;
		case 9: {strComment = "He3 amount (economy) in universe per team.";} break;
		case 10: {strComment = "Ship (equipment, speed) time to mount.";} break;
		case 11: {strComment = "Starting cash base (economy).";} break;
		case 12: {strComment = "Prosperity cash (game type) goal base.";} break;
		case 13: {strComment = "Ripcord time (speed) base (seconds).";} break;
		case 14: {strComment = "He3 regeneration (economy) rate.";} break;
		case 15: {strComment = "Point modifier (rank) for finding an aleph.";} break;
		case 16: {strComment = "Point modifier (rank) for finding a rock.";} break;
		case 17: {strComment = "Point modifier (rank) for retrieving a tech.";} break;
		case 18: {strComment = "Point modifier (rank) for destroying a miner.";} break;
		case 19: {strComment = "Point modifier (rank) for destroying a builder (constructor).";} break;
		case 20: {strComment = "Point modifier (rank) for destroying a layer (caltrop/tower).";} break;
		case 21: {strComment = "Point modifier (rank) for destroying a carrier (drone).";} break;
		case 22: {strComment = "Point modifier (rank) for destroying a player.";} break;
		case 23: {strComment = "Point modifier (rank) for destorying a base.";} break;
		case 24: {strComment = "Point modifier (rank) for capturing a base.";} break;
		case 25: {strComment = "Point modifier (rank) for retrieving a flag.";} break;
		case 26: {strComment = "Point modifier (rank) for recovering an artifcat.";} break;
		case 27: {strComment = "Point modifier (rank) for picking up a pod.";} break;
		case 28: {strComment = "Combat rating modifier (rank) added to kill math";} break;
		case 29: {strComment = "Combat rating modifier (rank) divided by kill math.";} break;
		case 30: {strComment = "Payday (economy) size ($/payday).";} break;
		case 31: {strComment = "Escape pod (gameplay) O2 supply.";} break; 
		case 32: {strComment = "Warp bomb detonation timer.";} break;
		case 33: {strComment = "Cost to update (performance) lifepods.";} break; 
		case 34: {strComment = "Cost to update (performance) turrets.";} break;
		case 35: {strComment = "Cost to update (performance) drones.";} break;
		case 36: {strComment = "Cost to update (performance) players.";} break;
		case 37: {strComment = "Base sector update cost (performance) in a sector.";} break;
		case 38: {strComment = "Cost to update (performance) divided by move math.";} break;
		default:
			break;
	}

	return strComment;
}

/*-------------------------------------------------------------------------
 * ExportStaticIGCObjs
 *-------------------------------------------------------------------------
  Purpose:
      Send the client all the static IGC objects. A default recipient must be already set.
  Not being used. Keep it around for a while just in case we want to resurrect it.
  
  Imago ressurect this for dumping IGC objs to text! (original MSRG comment above)
  Let's see about adding the FM_EXPORT back into FedSrv.CPP 7/25/08
  
  TODO: maps - 
	  void ExportIGCObjs() {

		//OT_station
		//OT_cluster 
		//OT_mine
		//OT_probe
		//OT_treasure
		//OT_warp

	  }
	-
  TODO: math on various values to get proper units
  TODO: typesOfDamage.csv
  TODO: support gzipped (/w xml option) allowing use as a "general public" core format
		Implementations of Gzip are already in FedSrv using appweb.lib, and WinTrek using:
			FreeImage_ZLibGZip(BYTE *target, DWORD target_size, BYTE *source, DWORD source_size);
 			FreeImage_ZLibGUnzip(BYTE *target, DWORD target_size, BYTE *source, DWORD source_size);

  TODO: resurrect the "artwork directory monitor", to once again send the EXPORT Fedmessage(s) on change (dynamic cores)
 */
void CmissionIGC::ExportStaticIGCObjs()
{
	ImissionIGC * pMission = this;

	// Export Constants 
  {
	int size = pMission->GetSizeOfConstants();
	Constants* pdata = (Constants*)(new char [size]);
	memcpy(pdata, pMission->GetConstants(), pMission->GetSizeOfConstants()); //whatever

	ZString strLine = "GLOBALID OR DEFENSE CONSTANTIDS\tGLOBAL VALUE OR DAMAGE CONSTANTID\tCOMMENT\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Constants"; 
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);

	strLine.SetEmpty();
	for(int i=0;i<c_fcidMax;i++) {
		strLine += ZString(i) + "\t" + ZString(pdata->floatConstants[i]) + "\t" + ConstantsGlobalComment(i) +"\r\n";
	}
	for(int i=0;i<c_dmgidMax;i++) {
		for(int i2=0;i2<c_defidMax;i2++) {
			strLine += ZString(i) + "\t" + ZString(pdata->damageConstants[i][i2]) + "\t" + ZString(i2) + ": "+ ConstantsDamageComment(i,i2) + "\r\n";
		}

	}

	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);
  }

  {
	// GAS
	ZString strLine = "ID\tCOPY OF NAME\tMAX SPEED\tTHRUST\tTURN RATE\tTURN TORQUE\tMAX STATION ARMOR\tSTATION ARMOR REGEN\tMAX STATION SHIELD\tSTATION SHIELD REGEN\tSHIP MAX ARMOR\tSHIP MAX SHIELD\tSHIP SHIELD REGEN\tSCAN RANGE\tSIGNATURE\tMAX ENERGY\tAMMO RANGE\tENERGY LIFESPAN\tMISSILE TURN RATE\tMINING RATE\tMINING YIELD\tMINING CAPACITY\tIP TIME\tWEAPON DAMAGE\tMISSILE DAMAGE\tDEVELOPMENT COST\tDEVELOPMENT TIME\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\GlobalAttributes";
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);
  }

  //for commonly used object types that referr to lot's of SFX IDs (stationtypes & ships)
  //NYI include copy of file names for even more convienence (start at path \artwork)
  {	
	// Ship SFX
	ZString strLine = "ID\tCOPY OF NAME\tINTERIOR\tEXTERIOR\tBOOST INTERIOR\tBOOST EXTERIOR\tTHRUST INTERIOR\tTHRUST EXTERIOR\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\ShipsSFX";
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);
  }
  {
	// Station SFX
	ZString strLine = "ID\tCON NEEDS ROCK\tCON UNDER ATTACK\tCON DESTROYED\tBUILD COMPLETION\tINTERIOR\tEXTERIOR\tALERT\tUNDER ATTACK\tCRITICAL\tDESTROYED\tCAPTURED\tCAPTURED ENEMY LISTENER\tDESTROYED ENEMY LISTENER\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\StationsSFX";
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);
  }

  // Export station types
  {
    const StationTypeListIGC * pstlist = pMission->GetStationTypes();
    StationTypeLinkIGC * pstlink = NULL;
	
	ZString strLine = "ID\tSIGNATURE\tMAX ARMOR HP\tMAX SHIELD HP\tARMOR REGEN RATE\tSHIELD REGEN RATE\tSCANNER RANGE\tINCOME\tRADIUS\tSUCCESSOR STATION TYPE ID\tARMOR DEFENSE ID\tSHIELD DEFENSE ID\tABILITIES\tALLOWED ASTEROIDS\tCLASS ID\tCON DRONE ID\tTEXTURE NAME\tCON NAME\tPRICE\tTIME TO BUILD\tMODEL NAME\tICON NAME\tNAME\tDESCRIPTION\tGROUP\tTECH REQUIRED\tTECH EFFECT\tTECH SELF\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\TypesOfStations";
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

    for (pstlink = pstlist->first(); pstlink; pstlink = pstlink->next())
      Obj2Txt(pstlink->data(), OT_stationType, pMission);
  }

  // Export projectiles
  {
    const ProjectileTypeListIGC * pptlist = pMission->GetProjectileTypes();
    ProjectileTypeLinkIGC * pptlink = NULL;

	ZString strLine = "ID\tPOWER\tBLAST POWER\tBLAST RADIUS\tSPEED\tLIFE SPAN\tDAMAGE TYPE\tABSOLUTE\tDIRECTIONAL\tWIDTH\tAMBIENT SFX ID\tCOLOR R G B A PCT\tRADIUS\tROTATION\tMODEL NAME\tTEXTURE NAME\tCOPY OF NAMES\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\TypesOfProjectiles";
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

    for (pptlink = pptlist->first(); pptlink; pptlink = pptlink->next())
      Obj2Txt(pptlink->data(), OT_projectileType, pMission);
  }

  // Export missile/mine/chaff/probe types
  {
    const ExpendableTypeListIGC* petlist = pMission->GetExpendableTypes();
    ExpendableTypeLinkIGC* petlink;
			
	// (NYI OT_expendableType) hack to create blank file /w appropriate header row for "nested objects"
	int seenMissile = 0; 
	int seenMine = 0;
	int seenChaff = 0;
	int seenProbe = 0;

	for (petlink = petlist->first(); petlink; petlink = petlink->next()) {
		if (!(seenMissile) && petlink->data()->GetObjectType() == OT_missileType) {
			ZString strLine = "ID\tACCELLERATION\tTURN RATE\tINITIAL SPEED\tLOCK TIME\tREADY TIME\tMAX LOCK\tCHAFF RESISTANCE\tDISPERSION\tLOCK ANGLE\tPOWER\tBLAST POWER\tBLAST RADIUS\tWIDTH\tDAMAGE CONSTANT ID\tDIRECTIONAL\tLAUNCH SFX ID\tAMBIENT SFX ID\tLOAD TIME\tLIFE SPAN\tSIGNATURE\tHIT POINTS\tDEFENSE CONSTANT ID\tABILITIES\tICON NAME\tCOLOR R G B A PCT\tRADIUS\tROTATION\tMODEL NAME\tTEXTURE NAME\tLAUNCHER SIGNATURE\tMASS\tPART MASK\tEXPENDABLE SIZE\tCOPY OF NAME\r\n";
			ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\ExpendableMissiles";
			char        szFilename[MAX_PATH + 1];
			HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
			FILE*       file = fopen (szFilename, "wb");
			fwrite (strLine, sizeof(char), strLine.GetLength(), file);
			fclose (file);
			seenMissile = 1;
		}
		else if (!(seenMine) && petlink->data()->GetObjectType() == OT_mineType) {
			ZString strLine = "ID\tMINE RADIUS\tPOWER\tENDURANCE\tDAMAGE CONSTANT ID\tLOAD TIME\tLIFE SPAN\tSIGNATURE\tHIT POINTS\tDEFENSE CONSTANT ID\tABILITIES\tICON NAME\tCOLOR R G B A PCT\tRADIUS\tROTATION\tMODEL NAME\tTEXTURE NAME\tLAUNCHER SIGNATURE\tMASS\tPART MASK\tEXPENDABLE SIZE\tCOPY OF NAME\r\n";
			ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\ExpendableMines";
			char        szFilename[MAX_PATH + 1];
			HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
			FILE*       file = fopen (szFilename, "wb");
			fwrite (strLine, sizeof(char), strLine.GetLength(), file);
			fclose (file);
			seenMine = 1;
		}
		else if (!(seenChaff) && petlink->data()->GetObjectType() == OT_chaffType) {
			ZString strLine = "ID\tSTRENGTH\tLOAD TIME\tLIFE SPAN\tSIGNATURE\tHIT POINTS\tDEFENSE CONSTANT ID\tABILITIES\tICON NAME\tCOLOR R G B A PCT\tRADIUS\tROTATION\tMODEL NAME\tTEXTURE NAME\tLAUNCHER SIGNATURE\tMASS\tPART MASK\tEXPENDABLE SIZE\tCOPY OF NAME\r\n";
			ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\ExpendableChaff"; // not chaffs
			char        szFilename[MAX_PATH + 1];
			HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
			FILE*       file = fopen (szFilename, "wb");
			fwrite (strLine, sizeof(char), strLine.GetLength(), file);
			fclose (file);
			seenChaff = 1;
		}
		else if (!(seenProbe) && petlink->data()->GetObjectType() == OT_probeType) {
			ZString strLine = "ID\tSCAN RANGE\tTIME TO LIVE\tDISPURSION\tACCURACY\tAMMO\tPROJECTILE TYPE ID\tAMBIENT SFX ID\tTIME TO RIP\tLOAD TIME\tLIFE SPAN\tSIGNATURE\tHIT POINTS\tDEFENSE CONSTANT ID\tABILITIES\tICON NAME\tCOLOR R G B A PCT\tRADIUS\tROTATION\tMODEL NAME\tTEXTURE NAME\tLAUNCHER SIGNATURE\tMASS\tPART MASK\tEXPENDABLE SIZE\tCOPY OF NAME\r\n";
			ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\ExpendableProbes";
			char        szFilename[MAX_PATH + 1];
			HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
			FILE*       file = fopen (szFilename, "wb");
			fwrite (strLine, sizeof(char), strLine.GetLength(), file);
			fclose (file);
			seenProbe = 1;
		} else {
			debugf("ExportStaticIGCObjs: unhandled expendable type %i\n",petlink->data()->GetObjectType());
		}
      Obj2Txt(petlink->data(), petlink->data()->GetObjectType(), pMission);
	}
  }

  // Export part types into thier own files (changed on 7/28/08)
  {

	//expendables (aka launcher part types)
    const PartTypeListIGC * pptlist = pMission->GetPartTypes();
    PartTypeLinkIGC * pptlink = NULL;
	ZString strLine = "EXPENDABLE ID\tPART ID\tAMOUNT\tSUCCESSOR PART ID\tLAUNCH COUNT\tINVENTORY MODEL\tPRICE\tTIME TO BUILD\tMODEL NAME\tICON NAME\tNAME\tDESCRIPTION\tGROUP\tTECH REQUIRED\tTECH EFFECT\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Expendables";
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

	//weapons
	strLine = "ID\tREADY TIME\tBURST TIME\tENERGY PER SHOT\tDISPERSION\tAMMO PER SHOT\tPROJECTILE TYPE ID\tACTIVATE SFX ID\tSHOT SFX ID\tBURST SFX ID\tMASS\tSIGNATURE\tSUCCESSOR PART ID\tPART MASK\tINVENTORY MODEL\tPRICE\tTIME TO BUILD\tMODEL NAME\tICON NAME\tNAME\tDESCRIPTION\tGROUP\tTECH REQUIRED\tTECH EFFECT\r\n";
	strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Weapons";
	hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

	//shields
	strLine = "ID\tREGEN RATE\tMAX STRENGTH\tDEFENSE CONSTANT ID (COMMENT)\tACTIVATE SFX ID\tDEACTIVATE SFX ID\tMASS\tSIGNATURE\tSUCCESSOR PART ID\tPART MASK\tINVENTORY MODEL\tPRICE\tTIME TO BUILD\tMODEL NAME\tICON NAME\tNAME\tDESCRIPTION\tGROUP\tTECH REQUIRED\tTECH EFFECT\r\n";
	strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Shields";
	hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

	//cloaks
	strLine = "ID\tENERGY CONSUMPTION\tMAX CLOAKING\tPOWER ON RATE\tPOWER OFF RATE\tENGAGE SFX ID\tDISENGAGE SFX ID\tMASS\tSIGNATURE\tSUCCESSOR PART ID\tPART MASK\tINVENTORY MODEL\tPRICE\tTIME TO BUILD\tMODEL NAME\tICON NAME\tNAME\tDESCRIPTION\tGROUP\tTECH REQUIRED\tTECH EFFECT\r\n";
	strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Cloaks";
	hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

	//afterburners
	strLine = "ID\tFUEL CONSUMPTION\tMAX THRUST\tPOWER ON RATE\tPOWER OFF RATE\tINTERIOR SFX ID\tEXTERIOR SFX ID\tMASS\tSIGNATURE\tSUCCESSOR PART ID\tPART MASK\tINVENTORY MODEL\tPRICE\tTIME TO BUILD\tMODEL NAME\tICON NAME\tNAME\tDESCRIPTION\tGROUP\tTECH REQUIRED\tTECH EFFECT\r\n";
	strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Afterburners";
	hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

	//packs
	strLine = "ID\tTYPE\tAMOUNT\tMASS\tSIGNATURE\tSUCCESSOR PART ID\tPART MASK\tINVENTORY MODEL\tPRICE\tTIME TO BUILD\tMODEL NAME\tICON NAME\tNAME\tDESCRIPTION\tGROUP\tTECH REQUIRED\tTECH EFFECT\r\n";
	strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Packs";
	hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

	for (pptlink = pptlist->first(); pptlink; pptlink = pptlink->next()) 
     Obj2Txt(pptlink->data(), OT_partType, pMission);	
  }

  // Export hull types
  {
    const HullTypeListIGC * phtlist = pMission->GetHullTypes();
    HullTypeLinkIGC * phtlink = NULL;
		
	ZString strLine = "ID\tMASS\tSIGNATURE\tSPEED\tMAX TURN RATES X Y Z\tTURN TORQUES Z Y Z\tTHRUST\tSIDE MULTIPLIER\tBACK MULTIPLIER\tSCANNER RANGE\tMAX FUEL\tECM\tLENGTH\tMAX ENERGY\tRECHARGE RATE\tRIP SPEED\tRIP COST\tMAX AMMO\tSUCCESSOR HULL ID\tMAX WEAPONS\tMAX FIXED WEAPONS\tHIT POINTS\tDEFENSE CONSTNAT ID\tMAGAZINE CAPACITY\tDISPENSER CAPACITY\tCHAFF LAUNCHER CAPACITY\tPREFERRED PART TYPES\tABILITIES\tTEXTURE NAME\tALLOWED CHAFF\tALLOWED MISSILES\tALLOWED DISPENSERS\tALLOWED SHIELDS\tALLOWED CLOAKS\tALLOWED BOOSTERS\tPRICE\tTIME TO BUILD\tMODEL NAME\tICON NAME\tNAME\tDESCRIPTION\tGROUP\tTECH REQUIRED\tTECH EFFECT\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\TypesOfShips";
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

	// and the hardpoints
	strLine = "SHIP TYPE ID\tINTERIOR SFX ID\tTURN SFX ID\tFRAME NAME\tLOCATION ABBREVIATION\tALLOWED WEAPONS\tFIXED\tCOPY OF NAME MDL\r\n";
	strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Hardpoints";
	szFilename[MAX_PATH + 1];
	hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

    for (phtlink = phtlist->first(); phtlink; phtlink = phtlink->next())
      Obj2Txt(phtlink->data(), OT_hullType, pMission);
  }

  // Export developments
  {
    const DevelopmentListIGC * pdtlist = pMission->GetDevelopments();
    DevelopmentLinkIGC * pdtlink = NULL;

	ZString strLine = "ID\tCOMPLETION SFX ID\tPRICE\tTIME TO BUILD\tMODEL NAME\tICON NAME\tNAME\tDESCRIPTION\tGROUP\tTECH REQUIRED\tTECH EFFECT\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Developments";
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

    for (pdtlink = pdtlist->first(); pdtlink; pdtlink = pdtlink->next())
      Obj2Txt(pdtlink->data(), OT_development, pMission);
  }

  // Export drones types
  {
    const DroneTypeListIGC * pdtlist = pMission->GetDroneTypes();
    DroneTypeLinkIGC * pdtlink = NULL;

	ZString strLine = "ID\tSHOOT SKILL\tMOVE SKILL\tBRAVERY\tPILOT TYPE\tHULL TYPE ID\tEXPENDABLE TYPE ID\tPRICE\tTIME TO BUILD\tMODEL NAME\tICON NAME\tNAME\tDESCRIPTION\tGROUP\tTECH REQUIRED\tTECH EFFECT\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\TypesOfDrones";
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

    for (pdtlink = pdtlist->first(); pdtlink; pdtlink = pdtlink->next())
      Obj2Txt(pdtlink->data(), OT_droneType, pMission);
  }

  // Export civilizations
  {
    const CivilizationListIGC * pctlist = pMission->GetCivilizations();
    CivilizationLinkIGC * pctlink = NULL;

	ZString strLine = "ID\tINCOME MODIFIER\tBONUS MODIFIER\tNAME\tICON\tHUD\tPOD HULL TYPE ID\tINITIAL STATION TYPE ID\tSTARTING TECH\tSTARTING TECH NODEV\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\Civilizations";
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

    for (pctlink = pctlist->first(); pctlink; pctlink = pctlink->next())
      Obj2Txt(pctlink->data(), OT_civilization, pMission);
  }

  // Export treasure sets
  {
    const TreasureSetListIGC * pctlist = pMission->GetTreasureSets();
    TreasureSetLinkIGC * pctlink = NULL;

	ZString strLine = "ID\tNAME\tNUMTREASURES\tZONE ONLY\tTREASURES  OBJID TYPE CHANCEPCT\r\n";
	ZString strFile = ZString(pMission->GetMissionParams()->szIGCStaticFile) + "\\TypesOfTreasures";
	char        szFilename[MAX_PATH + 1];
	HRESULT     hr = UTL::getFile((PCC)strFile, ".csv", szFilename, true, true);
	FILE*       file = fopen (szFilename, "wb");
	fwrite (strLine, sizeof(char), strLine.GetLength(), file);
	fclose (file);

    for (pctlink = pctlist->first(); pctlink; pctlink = pctlink->next())
      Obj2Txt(pctlink->data(), OT_treasureSet, pMission);
  }
  
}