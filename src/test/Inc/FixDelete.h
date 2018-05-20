#ifndef __FixDelete_h__
#define __FixDelete_h__

#if _MSC_VER > 1000
  #pragma once
#endif
//
///////////////////////////////////////////////////////////////////////////////
//// This is 'borrowed' from the CRT to ensure that delete is as we expect.  //
////                                                                         //
//// The problem arises when using STL, which appears to me as a bug in its  //
//// implementation of the global operator delete. The STL implementation    //
//// (from <new> simply calls free, with no regard to how the block was      //
//// allocated. Under _DEBUG builds, this causes an assert failure since     //
//// some blocks are allocated with the _CLIENT_BLOCK flag, and free always  //
//// checks for the _NORMAL_BLOCK flag.                                      //
////                                                                         //
//// The following code correctly passes that flag to the _free_dbg function //
//// so that the assert never fails. The code has been written-out since, by //
//// including the STL file <new>, the CRT implementation is hidden from the //
//// linker. As far as I know, there is no simple way to ignore the source   //
//// code implementation (from <new>) and force the linker to use the CRT    //
//// implementation.                                                         //
//                                                                           //
//#ifdef _DEBUG                                                              //
//  // Structure definition (abridged) from <crt/src/dbgint.h>               //
//  #ifndef nNoMansLandSize                                                  //
//    #define nNoMansLandSize 4                                              //
//    typedef struct _CrtMemBlockHeader                                      //
//    {                                                                      //
//      struct _CrtMemBlockHeader * pBlockHeaderNext;                        //
//      struct _CrtMemBlockHeader * pBlockHeaderPrev;                        //
//      char *                      szFileName;                              //
//      int                         nLine;                                   //
//      size_t                      nDataSize;                               //
//      int                         nBlockUse;                               //
//      long                        lRequest;                                //
//      unsigned char               gap[nNoMansLandSize];                    //
//    } _CrtMemBlockHeader;                                                  //
//  #endif // !nNoMansLandSize                                               //
//                                                                           //
//  // Function definition (abridged) from <crt/src/dbgdel.cpp>              //
//  inline void __cdecl operator delete(void *pUserData)                     //
//  {                                                                        //
//    if (pUserData == NULL)                                                 //
//      return;                                                              //
//    _CrtMemBlockHeader* pHead = (((_CrtMemBlockHeader *)pUserData)-1);     //
//    _free_dbg(pUserData, pHead->nBlockUse);                                //
//  }                                                                        //
//#endif // _DEBUG                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#endif // !__FixDelete_h__
