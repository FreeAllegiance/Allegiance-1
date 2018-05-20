#include "pch.h"

#include "FileLoader.h"

#ifdef STEAMSECURE
#include "steam_api.h"
#include <AllegianceSecurity.h>
#endif

void ExitWithError(const char* message) {
    PostMessageA(GetActiveWindow(), WM_SYSCOMMAND, SC_MINIMIZE, 0);

    MessageBoxA(GetDesktopWindow(), message, "Allegiance: Fatal modeler error", MB_ICONERROR);

    exit(0);
}

class FileLoader : public IFileLoader {
private:
    std::vector<ZString> m_vPaths;
public:

    FileLoader(const std::vector<ZString>& vPaths) :
        m_vPaths(vPaths)
    {}

    bool HasFile(const ZString& path) {
        TRef<ZFile> file;

        for (auto entry : m_vPaths) {
            file = new ZFile(entry + "/" + path, OF_READ | OF_SHARE_DENY_WRITE);
            if (file->IsValid()) {
                return true;
            }
        }
        return false;
    }

    ZString GetFilePath(const ZString& path) {
        TRef<ZFile> file;

        for (auto entry : m_vPaths) {
            file = new ZFile(entry + "/" + path, OF_READ | OF_SHARE_DENY_WRITE);
            if (file->IsValid()) {
                return entry + "/" + path;
            }
        }

        ExitWithError("Artwork file could not be opened: " + path + ".");
        return "";
    }

    TRef<ZFile> GetFile(const ZString& path) {
        TRef<ZFile> file;

        for (auto entry : m_vPaths) {
            file = new ZFile(entry + "/" + path, OF_READ | OF_SHARE_DENY_WRITE);
            if (file->IsValid()) {
                return file;
            }
        }

        ExitWithError("Artwork file could not be opened: " + path + ".");
        return nullptr;
    }
};

#ifdef STEAMSECURE
class SteamSecureFileLoader : public IFileLoader {
    FileHashTable			m_fileHashTable;

    std::shared_ptr<IFileLoader> m_loaderInner;
    ZString m_pathSecure;

    TRef<ZFile> Verify(const ZString& path) {
        ZString fullPath = m_pathSecure + "/" + path;
        TRef<ZFile> file = new ZFile(fullPath, OF_READ | OF_SHARE_DENY_WRITE);

        if (file->IsValid() == false || m_fileHashTable.IsHashCorrect(path, file) == false) {
            // BT - STEAM - Queue up a full content re-verify in case the user has a corrupted file.
            if (SteamUser() != nullptr && SteamUser()->BLoggedOn() == true) {
                SteamApps()->MarkContentCorrupt(false);
            }

            ExitWithError("Artwork file failed to validate: " + fullPath + ", we have queued up an installation reverification. Check your Steam App in the downloads section for details.");

            return nullptr;
        }

        return file;
    }

public:
    SteamSecureFileLoader(const std::shared_ptr<IFileLoader>& inner, const ZString& pathSecure) :
        m_loaderInner(inner),
        m_pathSecure(pathSecure)
    {}

    bool HasFile(const ZString& path) {
        if (m_fileHashTable.DoesFileHaveHash(path)) {
            TRef<ZFile> file = Verify(path);
            return true;
        }

        return m_loaderInner->HasFile(path);
    }

    ZString GetFilePath(const ZString& path) {
        if (m_fileHashTable.DoesFileHaveHash(path)) {
            TRef<ZFile> file = Verify(path);
            return m_pathSecure + "/" + path;
        }

        return m_loaderInner->GetFilePath(path);
    }

    TRef<ZFile> GetFile(const ZString& path) {
        if (m_fileHashTable.DoesFileHaveHash(path)) {
            TRef<ZFile> file = Verify(path);
            return file;
        }

        return m_loaderInner->GetFile(path);
    }
};
#endif

std::shared_ptr<IFileLoader> CreateFileLoader(const std::vector<ZString>& vPaths) {
    return std::make_shared<FileLoader>(vPaths);
}

std::shared_ptr<IFileLoader> CreateSecureFileLoader(const std::vector<ZString>& vPaths, const ZString& pathSecure) {
    std::shared_ptr<IFileLoader> loader = CreateFileLoader(vPaths);

    #ifdef STEAMSECURE
    loader = std::make_shared<SteamSecureFileLoader>(loader, pathSecure);
    #endif

    return loader;
}