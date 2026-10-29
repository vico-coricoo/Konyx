// Konyx2DCommands.cpp
//
// Module "Konyx 2D" (equivalent de "Covadis 2D") : habillage, courbes de
// niveau 2D, profils en long/travers, semis de points, polylignes 3D, etc.
// Voir KonCalculsCommands.cpp pour le modele d'ajout de commande.

#include "../../core/CommandRegistry.h"
#include <tchar.h>
#include <acutads.h>

KONYX_COMMAND(L"KONYX", L"Konyx2DVersion", L"K2DVERSION", 0, Konyx2DVersion)
{
    acutPrintf(_T("\n[Konyx 2D] squelette operationnel - v0.1\n"));
}
