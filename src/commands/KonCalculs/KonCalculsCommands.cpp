// KonCalculsCommands.cpp
//
// Module "Kon. Calculs" (equivalent de "Cov. Calcul").
// Chaque future commande (compensation de cheminement, rattachement,
// intersections, calculs de surfaces, etc.) se declare ici ou dans un
// nouveau fichier de ce dossier, en suivant le modele ci-dessous.
//
// Ne touchez JAMAIS a KonyxApp.cpp pour ajouter une commande : il suffit
// d'utiliser KONYX_COMMAND, l'enregistrement est automatique.

#include "../../core/CommandRegistry.h"
#include "GeoBase/GeoBaseDialog.h"
#include <adscodes.h>
#include <tchar.h>
#include <acutads.h>

// Commande de verification : confirme que le module KonCalculs est bien
// charge et pret a recevoir ses vraies commandes.
KONYX_COMMAND(L"KONYX", L"KonCalculsVersion", L"KCVERSION", 0, KonCalculsVersion)
{
    acutPrintf(_T("\n[Kon. Calculs] squelette operationnel - v0.1\n"));
}

extern "C" HWND adsw_acadMainWnd();

KONYX_COMMAND(L"KONYX", L"KonCalculsEditionGeoBase", L"KONCALEDITIONGEOBASE", 0, KonCalculsEditionGeoBase)
{
    Konyx::GeoBaseDialog dlg(L"");
    dlg.Afficher(adsw_acadMainWnd());
}

// --- Prochaines commandes a implementer ici, par exemple : ---
// KONYX_COMMAND("KONYX", "KonCalculsCompensation", "KCCOMP", 0, KonCalculsCompensation)
// {
//     // TODO : compensation de cheminement
// }
