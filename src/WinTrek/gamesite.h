
#ifndef _GAMESITE_H_
#define _GAMESITE_H_

int GameSite::MessageBox(const ZString& strText, const ZString& strCaption, UINT nType)
{
    return GetEngineWindow()->MessageBox(strText, strCaption, nType);
}

#endif
