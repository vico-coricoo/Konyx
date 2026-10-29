// Konyx3DCommands.cpp
//
// Module "Konyx 3D" (equivalent de "Covadis 3D") : modeles numeriques de
// terrain (MNT), triangulation, courbes de niveau 3D, cubatures, etc.
// Voir KonCalculsCommands.cpp pour le modele d'ajout de commande.

#include "../../core/CommandRegistry.h"
#include <tchar.h>
#include <acutads.h>

KONYX_COMMAND(L"KONYX", L"Konyx3DVersion", L"K3DVERSION", 0, Konyx3DVersion)
{
    acutPrintf(_T("\n[Konyx 3D] squelette operationnel - v0.1\n"));
}
