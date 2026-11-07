#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include "GeoRecord.h"
#include "GeoStationVisees.h"
#include "GeoCalculV0.h"
#include "GeoCalculV0Graphique.h"

namespace Konyx {

// Boite "Calcul de V0 des stations" (v1 simplifiee : mode Unique
// seulement, pas de Liste/Toutes, pas de bouton Graphique).
//
// Modifie directement enregistrements : "Enregistrer" ecrit le V0
// calcule (ou force) dans le GeoRecord Station correspondant, en
// memoire uniquement (le fichier .geo n'est pas touche ici ; c'est a
// l'appelant de marquer la geobase comme modifiee / a sauvegarder).
class GeoCalculV0Dialog {
public:
    // cheminGeobase : chemin du fichier .geo actuellement ouvert (affiche
    // dans l'entete du listing RTF ; vide si le fichier n'a pas encore ete
    // enregistre).
    static void Afficher(HWND hwndParent, std::vector<GeoRecord>& enregistrements,
                          const std::wstring& cheminGeobase);

private:
    GeoCalculV0Dialog(std::vector<GeoRecord>& enregistrements, const std::wstring& cheminGeobase);
    ~GeoCalculV0Dialog();

    static LRESULT CALLBACK FenetreProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT GererMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void CreerControles(HWND hwnd);
    void RemplirComboStations();
    void SelectionnerStation(int indexCombo);
    void PeuplerListeReferences();
    void OuvrirGraphique();
    void Calculer();
    void Enregistrer();
    void GenererEtOuvrirListing();
    void MajAffichageV0(); // recalcule la moyenne sur les lignes cochees et l'affiche

    std::vector<GeoRecord>& m_enregistrements;
    std::wstring m_cheminGeobase;
    std::vector<StationEtVisees> m_stations; // toutes les stations + leurs visees
    int m_indexStationCourante = -1;         // index dans m_stations
    const GeoRecord* m_pointStationCourante = nullptr; // Point donnant X/Y/Z de la station, ou nullptr
    std::vector<ContributionV0> m_contributions; // derniere analyse (Calculer())
    std::vector<size_t> m_indexParStationFiltree;

    HWND m_hwnd = nullptr;
    HFONT m_hFont = nullptr;
    HBRUSH m_hBrushFond = nullptr;

    HWND m_hComboStation = nullptr;
    HWND m_hChampListe = nullptr;
    HWND m_hX = nullptr;
    HWND m_hY = nullptr;
    HWND m_hZ = nullptr;
    HWND m_hListeRef = nullptr;
    HWND m_hCaseRefForcees = nullptr;
    HWND m_hBtnGraphique = nullptr;
    HWND m_hCasePondere = nullptr;
    HWND m_hV0 = nullptr;
    HWND m_hBtnCalculer = nullptr;
    HWND m_hBtnEnregistrer = nullptr;
    HWND m_hBtnListing = nullptr;
    HWND m_hBtnOK = nullptr;
    HWND m_hBtnAnnuler = nullptr;
    

    static constexpr wchar_t ClasseFenetre[] = L"KonyxGeoCalculV0DialogClass";

    static constexpr int ID_COMBO_STATION = 201;
    static constexpr int ID_LISTE_REF = 202;
    static constexpr int ID_CASE_PONDERE = 203;
    static constexpr int ID_CHAMP_V0 = 204;
    static constexpr int ID_BTN_CALCULER = 210;
    static constexpr int ID_BTN_ENREGISTRER = 211;
    static constexpr int ID_BTN_ANNULER = 212;
    static constexpr int ID_BTN_LISTING = 213;
    static constexpr int ID_BTN_OK = 214;

    static constexpr int ID_RADIO_UNIQUE = 220;
    static constexpr int ID_RADIO_LISTE  = 221;
    static constexpr int ID_RADIO_TOUTES = 222;

    static constexpr int ID_CHAMP_LISTE = 223;
    static constexpr int ID_CASE_REF_FORCEES = 224;
    static constexpr int ID_BTN_GRAPHIQUE = 225;
};

} // namespace Konyx
