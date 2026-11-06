#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "GeoRecord.h"
#include "GeoBaseParser.h"

#define ID_FICHIER_NOUVEAU          2001
#define ID_FICHIER_OUVRIR           2002
#define ID_FICHIER_ENREGISTRER      2003
#define ID_FICHIER_ENREGISTRER_SOUS 2004
#define ID_FICHIER_CONCATENER       2005
#define ID_FICHIER_IMPRIMER_GEOBASE 2006
#define ID_FICHIER_IMPRIMER_LISTING 2007
#define ID_FICHIER_RECENT_BASE      2010 // 2010..2014
#define ID_FICHIER_QUITTER          2020

#define ID_EDITION_ANNULER          2101
#define ID_EDITION_RETABLIR         2102
#define ID_EDITION_MODIFIER         2103
#define ID_EDITION_SUPPRIMER        2104
#define ID_EDITION_AJOUTER          2105
#define ID_EDITION_INSERER_AVANT    2106
#define ID_EDITION_INSERER_APRES    2107
#define ID_EDITION_CHANGER_HP       2108
#define ID_EDITION_CHANGER_CSTE_PRISME 2109
#define ID_EDITION_CHANGER_ALTITUDE 2110
#define ID_EDITION_CHANGER_SYMBOLE  2111
#define ID_EDITION_XY_ZERO          2112
#define ID_EDITION_Z_ZERO           2113
#define ID_EDITION_SUPPR_V0         2114
#define ID_EDITION_SUPPR_HP_AV_DI   2115
#define ID_EDITION_SUPPR_Z          2116
#define ID_EDITION_SUPPR_XY         2117
#define ID_EDITION_XY_CONSTANT      2118
#define ID_EDITION_Z_CONSTANT       2119
#define ID_EDITION_XY_VARIABLE      2120
#define ID_EDITION_Z_VARIABLE       2121

#define ID_AFFICHE_TOUT             2201
#define ID_AFFICHE_RIEN             2202
#define ID_AFFICHE_STATIONS         2203
#define ID_AFFICHE_REFERENCES       2204
#define ID_AFFICHE_MESURES          2205
#define ID_AFFICHE_POINTS           2206
#define ID_AFFICHE_LECTURES_NIV     2207
#define ID_AFFICHE_COMMENTAIRES     2208
#define ID_AFFICHE_COMMANDES        2209
#define ID_AFFICHE_DEF_CALCULS      2210
#define ID_AFFICHE_APPAREIL         2211
#define ID_AFFICHE_PHOTOS           2212
#define ID_AFFICHE_INVERSE_COORDS   2213


#define ID_LECTURE_1                2301
#define ID_LECTURE_2                2302

#define ID_EXPORT_GEOBASE           2401
#define ID_EXPORT_TRIMBLE           2402
#define ID_EXPORT_LEICA_WILD        2403
#define ID_EXPORT_SOKKIA            2404
#define ID_EXPORT_SEPARATEUR        2405
#define ID_EXPORT_COLONNE           2406
#define ID_EXPORT_TOPOJIS           2407
#define ID_EXPORT_DXF               2408
#define ID_EXPORT_POINTS            2409
#define ID_EXPORT_LEICA_DBX         2410

#define ID_CALCULS_INTERSECTIONS    2501
#define ID_CALCULS_INTER_PLAN_VERT  2502
#define ID_CALCULS_RELEVEMENTS      2503
#define ID_CALCULS_RECOUPEMENTS     2504
#define ID_CALCULS_STATIONS_DEC     2505
#define ID_CALCULS_TRIANGULATION    2506
#define ID_CALCULS_V0_STATIONS      2507
#define ID_CALCULS_CHEMINEMENT_POLY 2508
#define ID_CALCULS_CALCUL_BLOC      2509
#define ID_CALCULS_POINTS_RAYONNES  2510
#define ID_CALCULS_NUAGE_POINTS     2511
#define ID_CALCULS_NIV_SIMPLE       2512
#define ID_CALCULS_NIV_DOUBLE_MIRE  2513
#define ID_CALCULS_NIV_DOUBLE_STATION  2514

#define ID_OUTILS_SEARCH_MAT           2601
#define ID_OUTILS_SEARCH_SYMB          2602
#define ID_OUTILS_VISUALISATION_POLYGO 2603
#define ID_OUTILS_CHEMINEMENT_AUTO     2604
#define ID_OUTILS_REIMATRICULER        2605
#define ID_OUTILS_TEST_COHERENCE       2606
#define ID_OUTILS_CREER_POINTS         2607
#define ID_OUTILS_CONTROLE_PTS_DOUBLES 2608
#define ID_OUTILS_ELIM_PTS_DOUBLES     2609
#define ID_OUTILS_TRI_FIN              2610
#define ID_OUTILS_TRI_ALTITUDE         2611
#define ID_OUTILS_CONTROLE_PRECISION   2612
#define ID_OUTILS_CHANGEMENT_SYSTEME   2613
#define ID_OUTILS_REDUCTION_REPRISES   2614
#define ID_OUTILS_CHANGEMENT_REPERE    2615
#define ID_OUTILS_OPTIONS              2616

#define ID_CODIFICATION_FUSION_CODIF      2701
#define ID_CODIFICATION_TABLE_GEOCODIF    2702
#define ID_CODIFICATION_GENERER_DESSIN    2703
#define ID_CODIFICATION_LISTE_PTS_LEICA   2704
#define ID_CODIFICATION_LISTE_PTS_TRIMBLE 2705

namespace Konyx {

// Fenetre modale (Win32 natif, sans MFC) d'edition de GeoBase (.geo).
// Bloque l'interaction avec AutoCAD tant qu'elle est ouverte (boucle de
// messages locale demarree par Afficher(), qui ne rend la main qu'a la
// fermeture de la fenetre).
class GeoBaseDialog {
public:
    explicit GeoBaseDialog(const std::wstring& cheminFichier);
    ~GeoBaseDialog();

    // Affiche la fenetre et bloque jusqu'a sa fermeture.
    // hwndParent : fenetre AutoCAD (recuperable via adsw_acadMainWnd()).
    //void Afficher(HWND hwndParent);

    // Affiche la fenetre sans bloquer AutoCAD.
    // hwndParent : fenetre AutoCAD.
    void Afficher(HWND hwndParent);

private:
    static LRESULT CALLBACK FenetreProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT GererMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    //MENU FICHIER
    void CreerControles(HWND hwnd);
    void ChargerFichier(const std::wstring& chemin);
    void RafraichirListe();
    void AjouterLigneListe(int index, const GeoRecord& rec);
    COLORREF CouleurPourType(GeoType type) const;
    void GererDoubleClicListe(int index);
    void Fermer();

    void CreerMenu(HWND hwnd);
    void RafraichirMenuRecents();
    void ChargerRecents();
    void SauvegarderRecents();
    void AjouterRecent(const std::wstring& chemin);

    //MENU FICHIER
    void ActionNouveau();
    void ActionOuvrir();
    bool ActionEnregistrer();
    bool ActionEnregistrerSous();
    void ActionConcatener();
    void ActionImprimerGeoBase();
    void ActionImprimerListing();
    void ActionOuvrirRecent(int indice);


    // MENU EDITION
    void CreerMenuEdition();
    void MettreAJourEtatsMenuEdition();
    std::vector<int> IndicesSelectionnes() const;
    void SauvegarderEtatPourAnnulation();
    void ActionAnnuler();
    void ActionRetablir();
    void ActionModifier();
    void ActionSupprimer();
    void ActionAjouter();
    void ActionInsererAvant();
    void ActionInsererApres();
    void ActionChangerHP();
    void ActionChangerCstePrisme();
    void ActionChangerAltitude();
    void ActionChangerSymbole();
    void ActionXYaZero();
    void ActionZaZero();
    void ActionSupprV0();
    void ActionSupprHpAvDi();
    void ActionSupprZ();
    void ActionSupprXY();
    void ActionXYConstant(bool constant);
    void ActionZConstant(bool constant);

    void AjouterElement(int positionCible); // positionCible : 1-based
    void RenumeroterLignes();
    void CreerLecturesSuccessives(GeoType typeInitial);


    // MENU AFFICHAGE
    void CreerMenuAffichage();
    void ToggleAffichageTotal();
    void ToggleAffichageVide();
    void ToggleAffichageStations();
    void ToggleAffichageReferences();
    void ToggleAffichageMesures();
    void ToggleAffichagePoints();
    void ToggleAffichageLecturesNiv();
    void ToggleAffichageCommentaires();
    void ToggleAffichageCommandes();
    void ToggleAffichageDefinitionsCalculs();
    void ToggleAffichageAppareils();
    void ToggleAffichagePhotos();

    void InverserCoords_X_Y();
    

    // MENU LECTURE
    void CreerMenuLecture();


    // MENU EXPORT
    void CreerMenuExport();


    //MENU CALCULS
    void CreerMenuCalculs();
    void ActionIntersections();
    void ActionInterPlanVertical();
    void ActionRelevements();
    void ActionRecoupements();
    void ActionStationDecalees();
    void ActionTriangulation();
    void ActionCalculerV0Stations();
    void ActionCheminementPolygonal();
    void ActionCalculBloc();
    void ActionPointsRayonnes();
    void ActionRecalculNuagePoints();
    void ActionNivellementSimple();
    void ActionNivellementDoubleMire();
    void ActionNivellementDoubleStation();


    // MENU OUTILS
    void CreerMenuOutils();


    //MENU CODIFICATION
    void CreerMenuCodification();
    void FusionCodification();
    void TableGeocodification();
    void GenererDessin();
    void EcritureListeCodesLeica();
    void EcritureListeCodesTrimble();


    // MENU CONTEXTUEL CLIC DROIT
    void AfficherMenuContextuel(POINT ptEcran);
    void ActionVersReference();
    void ActionVersMesure();
    void ActionCercleGauche();
    void ActionVersLectureAV();
    void ActionVersLectureAR();
    void ActionInverserChemin();
    void ActionVersCommentaire();
    void ActionRechercherMatricule();
    void ActionRechercherSymbole();
    bool RechercherEtSelectionner(bool parSymbole, const std::wstring& valeur);

    // MENUBAR DES BOUTONS DE RACCOURCI
    void ActionAjouterPointDirect();
    void ActionAjouterLectureARDirect();
    void ActionAjouterLectureAVDirect();
    void ActionAjouterCheminementDirect();
    void CreerRubanBoutons(HWND hwnd);


    HWND m_hwnd = nullptr;
    HWND m_hwndListe = nullptr;    // SysListView32, style report, LVS_EX_FULLROWSELECT
    HWND m_hwndLogo = nullptr;

    HFONT m_hFontListe = nullptr;  // police a chasse fixe (Courier New)
    HIMAGELIST m_hImageListEspacement = nullptr;
    HICON m_hIcon = nullptr;

    HMENU m_hMenu = nullptr;
    HACCEL m_hAccel = nullptr;

    std::vector<std::wstring> m_recents;
    std::wstring m_cheminFichier;
    GeoBaseEntete m_entete;
    std::vector<GeoRecord> m_enregistrements;
    bool m_modifie = false;

    // Filtrage d'affichage (menu Affichage) : n'affecte que la liste,
    // jamais m_enregistrements. m_indexListeVersReel[i] donne l'index
    // reel dans m_enregistrements correspondant a la ligne i de la ListView.
    bool m_afficheStations = true;
    bool m_afficheReferences = true;
    bool m_afficheMesures = true;
    bool m_affichePoints = true;
    bool m_afficheLecturesNiv = true;
    bool m_afficheCommentaires = true;
    bool m_afficheCommandes = true;
    bool m_afficheDefinitionsCalculs = true;
    bool m_afficheAppareils = true;
    bool m_affichePhotos = true;
    std::vector<int> m_indexListeVersReel;

    bool TypeEstAffiche(GeoType type) const;
    int IndexReel(int indexListe) const; // -1 si hors bornes


    static constexpr wchar_t ClasseFenetre[] = L"KonyxGeoBaseDialogClass";
    static constexpr int ID_LISTE = 1001;
    static constexpr int HAUT_BANDEAU = 40;


    HMENU m_hMenuFichier = nullptr;
    HMENU m_hMenuEdition = nullptr;
    HMENU m_hMenuAffichage = nullptr;
    HMENU m_hMenuLecture = nullptr;
    HMENU m_hMenuExport = nullptr;
    HMENU m_hMenuCalculs = nullptr;
    HMENU m_hMenuOutils = nullptr;
    HMENU m_hMenuCodification = nullptr;

    HWND m_hRuban = nullptr;

    std::vector<std::vector<GeoRecord>> m_pileAnnuler;
    std::vector<std::vector<GeoRecord>> m_pileRetablir;
    static constexpr size_t MAX_HISTORIQUE = 20;

    static constexpr int ID_CTX_VERS_REFERENCE    = 3201;
    static constexpr int ID_CTX_VERS_MESURE       = 3202;
    static constexpr int ID_CTX_CERCLE_GAUCHE     = 3203;
    static constexpr int ID_CTX_VERS_LECT_AV      = 3204;
    static constexpr int ID_CTX_VERS_LECT_AR      = 3205;
    static constexpr int ID_CTX_INVERSER_CHEMIN   = 3206;
    static constexpr int ID_CTX_VERS_COMMENTAIRE  = 3207;
    static constexpr int ID_CTX_RECHERCHER_MAT    = 3208;
    static constexpr int ID_CTX_RECHERCHER_SYM    = 3209;

    static constexpr int ID_TB_AJOUTER_STATION = 2320;
    static constexpr int ID_TB_AJOUTER_REFERENCE = 2321;
    static constexpr int ID_TB_AJOUTER_MESURE = 2322;
    static constexpr int ID_TB_AJOUTER_POINT = 2323;
    static constexpr int ID_TB_AJOUTER_LECT_AR = 2324;
    static constexpr int ID_TB_AJOUTER_LECT_AV = 2325;
    static constexpr int ID_TB_AJOUTER_COMMANDE = 2326;
    static constexpr int ID_TB_AJOUTER_COMMENTAIRE = 2327;
    static constexpr int ID_TB_AJOUTER_CHEMINEMENT = 2328;
    static constexpr int ID_TB_AJOUTER_APPAREIL = 2329;
    static constexpr int ID_TB_AJOUTER_PHOTO = 2330;
    static constexpr int ID_TB_AJOUTER_POLYLIGNE = 2331;
};

} // namespace Konyx