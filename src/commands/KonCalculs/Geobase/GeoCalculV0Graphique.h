#pragma once

#define NOMINMAX
#include <windows.h>
#include <vector>
#include <string>

namespace Konyx {

// Boite "Graphique" du calcul de V0 : affiche la station (rouge) et ses
// references (jaune/orange selon selection) dans un repere XY
// orthonorme, avec coordonnees du curseur affichees en direct en bas a
// gauche. Clic gauche sur un point ou sa fleche toggle sa selection.
// Double-clic gauche = tout selectionner. Double-clic droit = tout
// deselectionner. OK valide la selection (ecrite dans "references" en
// sortie), Annuler ferme sans rien changer.
class GeoCalculV0Graphique {
public:
    struct RefGraphique {
        std::string matricule;
        double x = 0.0;
        double y = 0.0;
        bool selectionne = false;
    };

    // Retourne true si OK a ete clique (references est alors mis a jour
    // en place avec le nouvel etat de selection). Retourne false si
    // Annuler/fermeture (references n'est pas modifie).
    static bool Afficher(HWND hwndParent, const std::string& matriculeStation,
                          double xStation, double yStation,
                          std::vector<RefGraphique>& references);

private:
    GeoCalculV0Graphique(const std::string& matriculeStation, double xStation, double yStation,
                          const std::vector<RefGraphique>& references);
    ~GeoCalculV0Graphique();

    static LRESULT CALLBACK FenetreProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT GererMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void CreerControles(HWND hwnd);
    void CalculerTransformation(HWND hwnd);
    POINT VersEcran(double x, double y) const;
    void VersMonde(int sx, int sy, double& x, double& y) const;
    POINT PositionFleche(size_t idx, POINT pStation) const; // point milieu de la fleche station->ref i
    int TrouverPointSousCurseur(int sx, int sy) const; // -1 si aucun ; teste points ET fleches
    void Dessiner(HDC hdc, int largeur, int hauteur);

    std::string m_matriculeStation;
    double m_xStation = 0.0;
    double m_yStation = 0.0;
    std::vector<RefGraphique> m_references;
    bool m_valide = false;

    double m_xMin = 0.0, m_yMin = 0.0, m_echelle = 1.0;
    static constexpr int MARGE = 40;
    int m_largeurDessin = 0, m_hauteurDessin = 0; // zone de dessin (hors bande du bas)

    HWND m_hwnd = nullptr;
    HWND m_hLabelCoord = nullptr;
    HWND m_hBtnOK = nullptr;
    HWND m_hBtnAnnuler = nullptr;
    HFONT m_hFont = nullptr;      // police des controles (bande du bas)
    HFONT m_hFontDessin = nullptr; // police des libelles sur le dessin (+30%, gras)

    static constexpr wchar_t ClasseFenetre[] = L"KonyxGeoCalculV0GraphiqueClass";
    static constexpr int ID_BTN_OK = 301;
    static constexpr int ID_BTN_ANNULER = 302;
    static constexpr int HAUTEUR_BANDE_BASSE = 50;

    static constexpr int ID_LABEL_COORD = 303;
};

} // namespace Konyx
