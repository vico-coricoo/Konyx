#pragma once

#include <windows.h>
#include <optional>
#include <vector>
#include <string>
#include "GeoRecord.h"

namespace Konyx {

// Petite boite de dialogue listant les types d'elements ajoutables (meme
// esprit que le menu "Ajouter" de Covadis) : un bouton par type, et
// "Annuler". Renvoie le type choisi, ou std::nullopt si l'utilisateur a
// annule / ferme la fenetre.
class GeoAjouterTypeDialog {
public:
    static std::optional<GeoType> Choisir(HWND hwndParent);

private:
    GeoAjouterTypeDialog() = default;

    static constexpr wchar_t ClasseFenetre[] = L"KonyxGeoAjouterTypeDialog";

    LRESULT GererMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK FenetreProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    std::optional<GeoType> m_resultat;
    bool m_valide = false; // true si un bouton de type a ete clique (par opposition a Annuler/fermeture)
};

} // namespace Konyx