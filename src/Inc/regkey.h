#pragma once

#ifdef  _ALLEGIANCE_DEV_
#define ALLEGIANCE_REGISTRY_KEY_ROOT   "Software\\Microsoft\\Microsoft Games\\Allegiance\\Dev"
#else
#ifdef _ALLEGIANCE_PROD_
#define ALLEGIANCE_REGISTRY_KEY_ROOT   "Software\\Microsoft\\Microsoft Games\\Allegiance\\1.4"
#else
#define ALLEGIANCE_REGISTRY_KEY_ROOT   "Software\\Microsoft\\Microsoft Games\\Allegiance\\1.4"
#endif
#endif

// KGJV - added lobby & club
#if !defined(ALLSRV_STANDALONE)
  const char c_szSvcName[] = "AllSrv";
  #define HKLM_FedSrv        "SYSTEM\\CurrentControlSet\\Services\\AllSrv"
  #define HKLM_AllSrvUI      ALLEGIANCE_REGISTRY_KEY_ROOT "\\Server"
  #define HKLM_AllLobby		 "SYSTEM\\CurrentControlSet\\Services\\AllLobby"
  #define HKLM_AllClub		 ALLEGIANCE_REGISTRY_KEY_ROOT "\\AllClub"
//#define HKLM_AllClub       "SYSTEM\\CurrentControlSet\\Services\\AllClub"
  #define HKLM_PigSrv		 ALLEGIANCE_REGISTRY_KEY_ROOT "\\PigSrv"

#else // !defined(ALLSRV_STANDALONE)
  const char c_szSvcName[] = "AllSrv";
  #define HKLM_FedSrv        ALLEGIANCE_REGISTRY_KEY_ROOT "\\Server"
  #define HKLM_AllSrvUI      HKLM_FedSrv
  #define HKLM_AllLobby		 ALLEGIANCE_REGISTRY_KEY_ROOT "\\AllLobby"
  #define HKLM_AllClub		 ALLEGIANCE_REGISTRY_KEY_ROOT "\\AllClub"
  #define HKLM_PigSrv		 ALLEGIANCE_REGISTRY_KEY_ROOT "\\PigSrv"
#endif // !defined(ALLSRV_STANDALONE)


