// CommandRegistry.cpp
//
// Implementation du registre de commandes. Voir CommandRegistry.h pour
// le pourquoi de ce mecanisme.

#include "CommandRegistry.h"

#include <tchar.h>    // _T(), TCHAR — doit etre inclus avant les headers ARX
#include <aced.h>     // acedRegCmds
#include <rxregsvc.h> // AcRxClass, enregistrement ObjectARX
#include <acutads.h>  // acutPrintf (messages de debug en ligne de commande)

namespace Konyx {

CommandRegistry& CommandRegistry::Instance()
{
    static CommandRegistry instance;
    return instance;
}

void CommandRegistry::Add(const CommandEntry& entry)
{
    m_entries.push_back(entry);
}

void CommandRegistry::RegisterAllWithAutoCAD()
{
    for (const auto& entry : m_entries) {
        // acedRegCmds est l'objet global fourni par ObjectARX pour declarer
        // une commande native (equivalent C++ de (defun C:MaCommande ...)
        // en AutoLISP, mais gere par le moteur de commandes AutoCAD).
        acedRegCmds->addCommand(
            entry.groupName.c_str(),   // const wchar_t* = ACHAR* en Unicode
            entry.globalName.c_str(),
            entry.localName.c_str(),
            static_cast<Adesk::UInt32>(entry.commandFlags),
            reinterpret_cast<AcRxFunctionPtr>(entry.function));
    }

    acutPrintf(
        _T("\nKonyx : %d commande(s) enregistree(s).\n"),
        static_cast<int>(m_entries.size()));

}

void CommandRegistry::UnregisterAllFromAutoCAD()
{
    for (const auto& entry : m_entries) {
        acedRegCmds->removeCmd(entry.groupName.c_str(), entry.localName.c_str()); // wchar_t* = ACHAR*
    }
}

AutoRegister::AutoRegister(const CommandEntry& entry)
{
    CommandRegistry::Instance().Add(entry);
}

} // namespace Konyx
