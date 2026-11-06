#pragma once

#include <windows.h>
#include <string>

namespace Konyx {

// Petites briques d'interface reutilisables par toutes les fenetres Konyx
// (coherence visuelle : meme logo, meme emplacement, dans chaque fenetre).
namespace UI {

// Cree un controle statique affichant le logo Konyx en haut a gauche
// d'une fenetre. cheminLogo : fichier .ico (recommande, gere la
// transparence nativement) ou .bmp. x/y/taille en pixels.
// Retourne le HWND du controle (a detruire avec la fenetre parente,
// Windows s'en charge automatiquement a la destruction du parent).
HWND CreerLogoKonyx(HWND hwndParent, const std::wstring& cheminLogo,
                     int x = 8, int y = 8, int taille = 24);

} // namespace UI
} // namespace Konyx
