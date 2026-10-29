// KonyxApp.h
//
// Declaration du point d'entree ObjectARX. AutoCAD recherche une fonction
// exportee nommee "acrxEntryPoint" dans la DLL (.arx) : c'est par elle que
// tout commence et que tout se termine proprement.

#pragma once

#include <windows.h>
#include <rxobject.h>
#include <dbxEntryPoint.h>
#include "core/CommandRegistry.h"

class KonyxApp : public AcRxDbxApp {
public:
    KonyxApp() : AcRxDbxApp() {}

    virtual AcRx::AppRetCode On_kInitAppMsg(void* pkt) override
    {
        AcRx::AppRetCode retCode = AcRxDbxApp::On_kInitAppMsg(pkt);
        Konyx::CommandRegistry::Instance().RegisterAllWithAutoCAD();
        acutPrintf(_T("\n--------------------------------------------------"));
        acutPrintf(_T("\n Konyx charge avec succes - v0.1"));
        acutPrintf(_T("\n Tapez KCVERSION pour verifier le chargement."));
        acutPrintf(_T("\n--------------------------------------------------\n"));
        return retCode;
    }

    virtual AcRx::AppRetCode On_kUnloadAppMsg(void* pkt) override
    {
        Konyx::CommandRegistry::Instance().UnregisterAllFromAutoCAD();
        return AcRxDbxApp::On_kUnloadAppMsg(pkt);
    }

    virtual void RegisterServerComponents() override {}
};