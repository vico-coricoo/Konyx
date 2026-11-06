#pragma once

#include <windows.h>
#include <string>
#include "GeoRecord.h"
#include <optional>

namespace Konyx {

// Boite modale d'edition d'un seul enregistrement de GeoBase, qui adapte
// ses champs selon rec.type (voir GeoEditDialog.cpp / CreerControles()).
//
// Champs communs a tous les types geres :
//   - Ligne (position dans le fichier, 1-based ; permet de deplacer l'element)
//   - Matricule
//   - Symbole (SY), sauf pour Station, CheminAntenne/Encadre, Appareil, Photo
//   - Case "Ligne desactivee (commentaire)" -> rec.estCommentaire
//
// Types geres en v1 : Station, Reference, Mesure, Point, NivLectAR,
// NivLectAV, Commentaire (texte libre). Les autres types affichent un
// message "non pris en charge" (voir Modifier()).
class GeoEditDialog {
public:
    // Affiche la boite modale. nbLignesTotal = nombre d'enregistrements
    // dans la GeoBase (pour valider la case "Ligne" demandee).
    // Si l'utilisateur valide (OK), rec est mis a jour en place et la
    // fonction retourne true. rec.numLigne en sortie est la position
    // CIBLE demandee (1-based) : c'est a l'appelant de deplacer
    // l'enregistrement dans son vecteur si cette valeur differe de la
    // position actuelle, puis de renumeroter.
    static bool Modifier(HWND hwndParent, GeoRecord& rec, int nbLignesTotal,
                    std::optional<GeoType> typeSuivant, bool& allerSuivant, bool creation = false);

private:
    GeoEditDialog(const GeoRecord& rec, int nbLignesTotal, std::optional<GeoType> typeSuivant);
    ~GeoEditDialog();

    static LRESULT CALLBACK FenetreProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT GererMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void CreerControles(HWND hwnd);
    bool LireVersRecord(); // false si une saisie est invalide (affiche un message)
    void MajEtatChampsOptionnels(); // active/desactive les champs lies aux cases a cocher

    bool m_estTypeGere = true;
    GeoRecord m_rec;        // copie de travail ; recopiee vers l'appelant seulement si OK
    int m_nbLignesTotal = 0;
    bool m_valide = false;
    bool m_creation = false;

    HWND m_hwnd = nullptr;
    HFONT m_hFont = nullptr;
    HBRUSH m_hBrushFond = nullptr;

    // Champs communs
    HWND m_hLigne = nullptr;
    HWND m_hMatricule = nullptr;
    HWND m_hCommentaireCase = nullptr;
    HWND m_hSymbole = nullptr;

    // Champs specifiques (reutilises selon le type, voir CreerControles)
    HWND m_hChamp1 = nullptr;          // HI / HP / LM (Point n'utilise plus ce champ, voir ci-dessous)
    HWND m_hChamp2Case = nullptr;      // case "V0 connu" / "Distance horiz. (DH)"
    HWND m_hChamp2 = nullptr;          // V0 / AH / X / DH
    HWND m_hChamp3 = nullptr;          // AV / Y
    HWND m_hChamp4 = nullptr;          // DI / Z

    HWND m_hCaseXYActif = nullptr;
    HWND m_hCaseXYFixe = nullptr;      // Point : case "Fixes" (X,Y)
    HWND m_hCaseZActif = nullptr;
    HWND m_hCaseZFixe = nullptr;       // Point : case "Fixe" (Z)
    HWND m_hCommentaireTexte = nullptr;// texte libre (Commentaire pur uniquement)

    static constexpr wchar_t ClasseFenetre[] = L"KonyxGeoEditDialogClass";

    static constexpr int ID_LIGNE = 101;
    static constexpr int ID_MATRICULE = 102;
    static constexpr int ID_COMMENTAIRE_CASE = 103;
    static constexpr int ID_SYMBOLE = 104;
    static constexpr int ID_CHAMP1 = 110;
    static constexpr int ID_CHAMP2_CASE = 111;
    static constexpr int ID_CHAMP2 = 112;
    static constexpr int ID_CHAMP3 = 113;
    static constexpr int ID_CHAMP4 = 114;
    static constexpr int ID_CASE_XY_FIXE = 115;
    static constexpr int ID_CASE_Z_FIXE = 116;
    static constexpr int ID_COMMENTAIRE_TEXTE = 120;
    static constexpr int ID_BTN_OK = 130;
    static constexpr int ID_BTN_ANNULER = 131;

    static constexpr int ID_CASE_XY_ACTIF = 117;
    static constexpr int ID_CASE_Z_ACTIF = 118;

    std::optional<GeoType> m_typeSuivant;
    bool m_allerSuivant = false;
    HWND m_hBtnSuivant = nullptr;
    static constexpr int ID_BTN_SUIVANT = 132;
};

} // namespace Konyx
