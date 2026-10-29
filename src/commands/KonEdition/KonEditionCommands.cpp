// KonEditionCommands.cpp
//
// Module "Kon. Edition" (equivalent de "Cov. Edition") : mise en page,
// cartouches, export de documents, editions de resultats de calculs, etc.
// Voir KonCalculsCommands.cpp pour le modele d'ajout de commande.

#include "../../core/CommandRegistry.h"
#include <tchar.h>
#include <acutads.h>

KONYX_COMMAND(L"KONYX", L"KonEditionVersion", L"KEVERSION", 0, KonEditionVersion)
{
    acutPrintf(_T("\n[Kon. Edition] squelette operationnel - v0.1\n"));
}
