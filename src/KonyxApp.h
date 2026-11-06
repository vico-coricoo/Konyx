#pragma once

#include <windows.h>
#include <tchar.h>
#include <rxobject.h>
#include <dbxEntryPoint.h>
#include <aced.h>
#include <acutads.h>
#include <acedCmdNF.h>
#include <adscodes.h>
#include "core/CommandRegistry.h"

class KonyxApp : public AcRxDbxApp {
public:
    KonyxApp() : AcRxDbxApp() {}

    virtual AcRx::AppRetCode On_kInitAppMsg(void* pkt) override
    {
        AcRx::AppRetCode retCode = AcRxDbxApp::On_kInitAppMsg(pkt);
        Konyx::CommandRegistry::Instance().RegisterAllWithAutoCAD();
        LoadKonyxRibbon();
        acutPrintf(_T("\n Konyx charge - v0.1\n"));
        return retCode;
    }

    virtual AcRx::AppRetCode On_kUnloadAppMsg(void* pkt) override
    {
        Konyx::CommandRegistry::Instance().UnregisterAllFromAutoCAD();
        return AcRxDbxApp::On_kUnloadAppMsg(pkt);
    }

    virtual void RegisterServerComponents() override {}


private:
    void LoadKonyxRibbon()
    {
        wchar_t arxPath[MAX_PATH] = { 0 };

        HMODULE hMod = GetModuleHandleW(L"Konyx.arx");
        if (hMod == NULL) {
            acutPrintf(_T("\n[Konyx] Module Konyx.arx introuvable."));
            return;
        }

        if (GetModuleFileNameW(hMod, arxPath, MAX_PATH) == 0) {
            acutPrintf(_T("\n[Konyx] Impossible de trouver le chemin du module."));
            return;
        }

        wchar_t* lastSlash = wcsrchr(arxPath, L'\\');
        if (lastSlash == nullptr) {
            acutPrintf(_T("\n[Konyx] Chemin du module invalide."));
            return;
        }

        *(lastSlash + 1) = L'\0';

        wchar_t cuixPath[MAX_PATH] = { 0 };
        if (wcscpy_s(cuixPath, MAX_PATH, arxPath) != 0 ||
            wcscat_s(cuixPath, MAX_PATH, L"Konyx.cuix") != 0) {
            acutPrintf(_T("\n[Konyx] Chemin CUIX trop long."));
            return;
        }

        if (GetFileAttributesW(cuixPath) == INVALID_FILE_ATTRIBUTES) {
            acutPrintf(_T("\n[Konyx] Konyx.cuix introuvable."));
            return;
        }

        // Prépare une commande AutoCAD à exécuter après l'initialisation.
        acedCommandS(
            RTSTR, _T("_.DELAY"),
            RTSTR, _T("1000"),
            RTNONE
        );

        acedCommandS(
            RTSTR, _T("_.CUILOAD"),
            RTSTR, cuixPath,
            RTNONE
        );
    }

};
