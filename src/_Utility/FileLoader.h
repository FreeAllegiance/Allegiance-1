#pragma once

#include "tref.h"
#include "base.h"
#include "vector"
#include "memory"

class IFileLoader {
public:

    virtual bool HasFile(const ZString& path) = 0;
    virtual ZString GetFilePath(const ZString& path) = 0;
    virtual TRef<ZFile> GetFile(const ZString& path) = 0;
};

std::shared_ptr<IFileLoader> CreateSecureFileLoader(const std::vector<ZString>& vPaths, const ZString& pathSecure);