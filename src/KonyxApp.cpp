// KonyxApp.cpp
//
// Cycle de vie du module Konyx cote AutoCAD.
//
// AutoCAD envoie des "messages" a cette fonction a differents moments :
// - kInitAppMsg   : le module vient d'etre charge (NETLOAD/ARX, ou demarrage
//                    si charge automatiquement) -> on enregistre les commandes.
// - kUnloadAppMsg : le module va etre decharge -> on desenregistre proprement.
//
// C'est ICI, et seulement ici, qu'on branche le CommandRegistry sur AutoCAD.
// Tout le reste du projet (KonCalculs, Konyx2D, ...) n'a jamais besoin de
// connaitre ce fichier : chaque commande s'auto-declare via KONYX_COMMAND.

#include <windows.h>
#include <tchar.h>
#include <aced.h>
#include <acutads.h>
#include <rxregsvc.h>
#include "KonyxApp.h"

IMPLEMENT_ARX_ENTRYPOINT(KonyxApp)