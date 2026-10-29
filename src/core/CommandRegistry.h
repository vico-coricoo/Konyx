// CommandRegistry.h
//
// Systeme d'auto-enregistrement des commandes Konyx.
//
// Pourquoi ce fichier existe :
// Covadis (et donc Konyx a terme) compte des centaines de commandes
// reparties sur 4 menus (Kon. Calculs, Konyx 2D, Konyx 3D, Kon. Edition).
// Plutot que d'enregistrer chaque commande "a la main" dans KonyxApp.cpp
// (ce qui obligerait a modifier un fichier central a chaque ajout, avec
// risque de conflit et de relecture complete), chaque commande s'auto-
// enregistre au demarrage grace a la macro KONYX_COMMAND ci-dessous.
//
// Utilisation dans un fichier de commande (voir commands/KonCalculs/*.cpp) :
//
//   KONYX_COMMAND("KONYX", "KonCalculsVersion", "KCVERSION", 0, KonCalculsVersion)
//   {
//       acutPrintf(_T("\nKonyx pret.\n"));
//   }
//
// - groupName   : nom du groupe de commandes AutoCAD (apparait dans GROUPCODE)
// - globalName  : nom "officiel" de la commande (CMDECHO, macros, LISP)
// - localName   : alias tape par l'utilisateur dans la ligne de commande
// - flags       : flags AutoCAD (ACRX_CMD_TRANSPARENT, ACRX_CMD_MODAL, ...)
// - func        : nom de la fonction C++ generee automatiquement

#pragma once

#include <vector>
#include <string>
#include <tchar.h>

// ACHAR est le type caractere natif d'AutoCAD/ObjectARX : en build Unicode
// (obligatoire depuis AutoCAD 2013), ACHAR = wchar_t (16-bit).
// On utilise donc std::wstring partout dans Konyx, et L"..." pour les
// litteraux (ou _T("...") une fois tchar.h inclus via les headers ARX).
namespace Konyx {

// Une commande enregistree, en attente d'etre declaree a AutoCAD.
struct CommandEntry {
    std::wstring groupName;
    std::wstring globalName;
    std::wstring localName;
    int commandFlags;
    void (*function)();
};

// Registre global : collecte toutes les commandes declarees via KONYX_COMMAND
// dans n'importe quel fichier .cpp du projet, puis les enregistre d'un coup
// aupres d'AutoCAD au chargement du module (voir KonyxApp.cpp).
class CommandRegistry {
public:
    static CommandRegistry& Instance();

    void Add(const CommandEntry& entry);

    // Appele depuis acrxEntryPoint() au message kInitAppMsg
    void RegisterAllWithAutoCAD();

    // Appele depuis acrxEntryPoint() au message kUnloadAppMsg
    void UnregisterAllFromAutoCAD();

    size_t Count() const { return m_entries.size(); }

private:
    CommandRegistry() = default;
    std::vector<CommandEntry> m_entries;
};

// Petite classe "trampoline" : son seul role est de s'executer au moment
// de l'initialisation des variables statiques (avant main()/acrxEntryPoint),
// pour enregistrer automatiquement chaque commande dans le CommandRegistry.
struct AutoRegister {
    explicit AutoRegister(const CommandEntry& entry);
};

} // namespace Konyx

// Macro principale : declare une commande ET genere son enregistrement
// automatique. Le bloc de code qui suit l'appel de la macro devient le
// corps de la fonction executee quand l'utilisateur tape la commande.
// Les arguments group, globalName, localName doivent etre des litteraux
// wide : utilisez L"MONGROUPE", L"MACOMMANDE", L"MONALIAS".
// Exemple : KONYX_COMMAND(L"KONYX", L"KonCalculsVersion", L"KCVERSION", 0, KonCalculsVersion)
#define KONYX_COMMAND(group, globalName, localName, flags, func)           \
    static void func();                                                    \
    namespace {                                                            \
        static ::Konyx::AutoRegister autoReg_##func(                       \
            ::Konyx::CommandEntry{ group, globalName, localName, flags, func }); \
    }                                                                       \
    static void func()
