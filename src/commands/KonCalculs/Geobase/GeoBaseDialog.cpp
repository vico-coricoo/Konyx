#include "GeoBaseDialog.h"
#include "GeoEditDialog.h"
#include "GeoCalculV0Dialog.h"
#include "GeoAjouterTypeDialog.h"
#include <windowsx.h>
#include "../../../core/UI/KonyxUI.h"
#include <commdlg.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cwchar>


#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

// Chemin du logo Konyx, partage par toutes les fenetres (voir KonyxUI.h).
// A ajuster selon l'emplacement reel ou tu deposes le fichier (ex: a cote
// du .arx, dans un dossier "resources" copie au build).
static const std::wstring LogoKonyxChemin = L"C:\\Dev\\Konyx\\resources\\logo\\konyx.ico";

namespace Konyx {

std::string IncrementerMatricule(const std::string& matricule) {
    if (matricule.empty())
        return matricule;

    // Cas 1 : matricule composé avec suffixe numérique (ex : X.9 -> X.10).
    size_t pos = matricule.find_last_of('.');

    if (pos != std::string::npos && pos + 1 < matricule.size()) {
        std::string suffixe = matricule.substr(pos + 1);

        bool numerique = true;
        for (char c : suffixe) {
            if (c < '0' || c > '9') {
                numerique = false;
                break;
            }
        }

        if (numerique) {
            for (int i = static_cast<int>(suffixe.size()) - 1; i >= 0; --i) {
                if (suffixe[i] < '9') {
                    ++suffixe[i];
                    return matricule.substr(0, pos + 1) + suffixe;
                }
                suffixe[i] = '0';
            }

            suffixe.insert(suffixe.begin(), '1');
            return matricule.substr(0, pos + 1) + suffixe;
        }
    }

    // Cas 2 : matricule entièrement numérique, sans limite de taille.
    bool numerique = true;
    for (char c : matricule) {
        if (c < '0' || c > '9') {
            numerique = false;
            break;
        }
    }

    if (numerique) {
        std::string resultat = matricule;

        for (int i = static_cast<int>(resultat.size()) - 1; i >= 0; --i) {
            if (resultat[i] < '9') {
                ++resultat[i];
                return resultat;
            }
            resultat[i] = '0';
        }

        resultat.insert(resultat.begin(), '1');
        return resultat;
    }

    // Cas 3 : matricule alphabétique (A -> B ... Z -> AA).
    bool alphabetique = true;
    for (char c : matricule) {
        if (c < 'A' || c > 'Z') {
            alphabetique = false;
            break;
        }
    }

    if (alphabetique) {
        std::string resultat = matricule;

        for (int i = static_cast<int>(resultat.size()) - 1; i >= 0; --i) {
            if (resultat[i] < 'Z') {
                ++resultat[i];
                return resultat;
            }
            resultat[i] = 'A';
        }

        resultat.insert(resultat.begin(), 'A');
        return resultat;
    }

    // Format non reconnu : on conserve le matricule sans le modifier.
    return matricule;
}

constexpr wchar_t GeoBaseDialog::ClasseFenetre[];

namespace {
    GeoBaseDialog* InstanceDepuisHwnd(HWND hwnd) {
        return reinterpret_cast<GeoBaseDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
}

GeoBaseDialog::GeoBaseDialog(const std::wstring& cheminFichier)
    : m_cheminFichier(cheminFichier) {
}

GeoBaseDialog::~GeoBaseDialog() {
    if (m_hFontListe) DeleteObject(m_hFontListe);

    if (m_hImageListEspacement) ImageList_Destroy(m_hImageListEspacement);
}

void GeoBaseDialog::RenumeroterLignes() {
    for (size_t i = 0; i < m_enregistrements.size(); ++i) {
        m_enregistrements[i].numLigne = static_cast<int>(i + 1);
    }
}

LRESULT CALLBACK GeoBaseDialog::FenetreProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        auto* pThis = reinterpret_cast<GeoBaseDialog*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    }
    GeoBaseDialog* pThis = InstanceDepuisHwnd(hwnd);
    if (pThis) return pThis->GererMessage(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GeoBaseDialog::GererMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreerControles(hwnd);
        ChargerRecents();
        CreerMenu(hwnd);
        CreerRubanBoutons(hwnd);
        if (m_cheminFichier.empty() && !m_recents.empty()) {
            m_cheminFichier = m_recents.front();
        }
        if (!m_cheminFichier.empty()) ChargerFichier(m_cheminFichier);
        return 0;

    case WM_SIZE: {
        if (!m_hRuban || !m_hwndListe) return 0;
        RECT rc; GetClientRect(hwnd, &rc);

        SendMessage(m_hRuban, TB_AUTOSIZE, 0, 0); // calcule la hauteur naturelle du ruban
        RECT rcRuban{};
        GetWindowRect(m_hRuban, &rcRuban);
        int hauteurRuban = rcRuban.bottom - rcRuban.top;

        MoveWindow(m_hRuban, 0, 0, rc.right, hauteurRuban, TRUE); // etire ensuite sur toute la largeur

        MoveWindow(m_hwndListe, 0, hauteurRuban, rc.right,
                   rc.bottom - hauteurRuban, TRUE);
        return 0;
    }

    case WM_SIZING: {
        RECT* rc = reinterpret_cast<RECT*>(lParam);
        constexpr int LARGEUR_FIXE = 1500;
        
        switch (wParam) {
        case WMSZ_LEFT:
        case WMSZ_TOPLEFT:
        case WMSZ_BOTTOMLEFT:
            rc->left = rc->right - LARGEUR_FIXE;
            break;

        default:
            rc->right = rc->left + LARGEUR_FIXE;
            break;
        }

        return TRUE;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        // Hauteur minimale de la fenetre (pixels).
        constexpr int HAUTEUR_MIN = 750;
        mmi->ptMinTrackSize.y = HAUTEUR_MIN;
    return 0;
    }

    case WM_NOTIFY: {
        auto* pnmh = reinterpret_cast<LPNMHDR>(lParam);

        if (pnmh->hwndFrom == m_hRuban && pnmh->code == NM_CUSTOMDRAW) {
            auto* cd = reinterpret_cast<LPNMTBCUSTOMDRAW>(lParam);
            if (cd->nmcd.dwDrawStage == CDDS_PREPAINT) {
                HBRUSH hBr = CreateSolidBrush(RGB(0xEE, 0xEE, 0xEE));
                FillRect(cd->nmcd.hdc, &cd->nmcd.rc, hBr);
                DeleteObject(hBr);
            }
            return CDRF_DODEFAULT;
        }

        if (pnmh->idFrom == ID_LISTE) {
            if (pnmh->code == NM_DBLCLK) {
                auto* pnmia = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
                if (pnmia->iItem >= 0) GererDoubleClicListe(pnmia->iItem);
                return 0;
            }
            if (pnmh->code == NM_CUSTOMDRAW) {
                auto* pcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
                switch (pcd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                    return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT: {
                    int idx = static_cast<int>(pcd->nmcd.dwItemSpec);
                    if (idx >= 0 && idx < static_cast<int>(m_enregistrements.size())) {
                        pcd->clrText = CouleurPourType(m_enregistrements[idx].type);
                    }
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, CDRF_DODEFAULT);
                    return CDRF_DODEFAULT;
                }
                default:
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, CDRF_DODEFAULT);
                    return CDRF_DODEFAULT;
                }
            }
        }
        break;
    }

    case WM_INITMENUPOPUP: {
        HMENU hMenuOuvert = reinterpret_cast<HMENU>(wParam);
        if (hMenuOuvert == m_hMenuEdition) MettreAJourEtatsMenuEdition();
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        //menu fichier
        if (id == ID_FICHIER_NOUVEAU) { ActionNouveau(); return 0; }
        if (id == ID_FICHIER_OUVRIR) { ActionOuvrir(); return 0; }
        if (id == ID_FICHIER_ENREGISTRER) { ActionEnregistrer(); return 0; }
        if (id == ID_FICHIER_ENREGISTRER_SOUS) { ActionEnregistrerSous(); return 0; }
        if (id == ID_FICHIER_CONCATENER) { ActionConcatener(); return 0; }
        if (id == ID_FICHIER_IMPRIMER_GEOBASE) { ActionImprimerGeoBase(); return 0; }
        if (id == ID_FICHIER_IMPRIMER_LISTING) { ActionImprimerListing(); return 0; }
        if (id == ID_FICHIER_QUITTER) { PostMessage(hwnd, WM_CLOSE, 0, 0); return 0; }
        if (id >= ID_FICHIER_RECENT_BASE && id < ID_FICHIER_RECENT_BASE + 5) {
            ActionOuvrirRecent(id - ID_FICHIER_RECENT_BASE);
            return 0;
        }


        //menu edition
        if (id == ID_EDITION_ANNULER) { ActionAnnuler(); return 0; }
        if (id == ID_EDITION_RETABLIR) { ActionRetablir(); return 0; }
        if (id == ID_EDITION_MODIFIER) { ActionModifier(); return 0; }
        if (id == ID_EDITION_SUPPRIMER) { ActionSupprimer(); return 0; }
        if (id == ID_EDITION_AJOUTER) { ActionAjouter(); return 0; }
        if (id == ID_EDITION_INSERER_AVANT) { ActionInsererAvant(); return 0; }
        if (id == ID_EDITION_INSERER_APRES) { ActionInsererApres(); return 0; }
        if (id == ID_EDITION_CHANGER_HP) { ActionChangerHP(); return 0; }
        if (id == ID_EDITION_CHANGER_CSTE_PRISME) { ActionChangerCstePrisme(); return 0; }
        if (id == ID_EDITION_CHANGER_ALTITUDE) { ActionChangerAltitude(); return 0; }
        if (id == ID_EDITION_CHANGER_SYMBOLE) { ActionChangerSymbole(); return 0; }
        if (id == ID_EDITION_XY_ZERO) { ActionXYaZero(); return 0; }
        if (id == ID_EDITION_Z_ZERO) { ActionZaZero(); return 0; }
        if (id == ID_EDITION_SUPPR_V0) { ActionSupprV0(); return 0; }
        if (id == ID_EDITION_SUPPR_HP_AV_DI) { ActionSupprHpAvDi(); return 0; }
        if (id == ID_EDITION_SUPPR_Z) { ActionSupprZ(); return 0; }
        if (id == ID_EDITION_SUPPR_XY) { ActionSupprXY(); return 0; }
        if (id == ID_EDITION_XY_CONSTANT) { ActionXYConstant(true); return 0; }
        if (id == ID_EDITION_Z_CONSTANT) { ActionZConstant(true); return 0; }
        if (id == ID_EDITION_XY_VARIABLE) { ActionXYConstant(false); return 0; }
        if (id == ID_EDITION_Z_VARIABLE) { ActionZConstant(false); return 0; }


        //menu affichage
        if (id == ID_AFFICHE_STATIONS) { ToggleAffichageStations(); return 0; }
        if (id == ID_AFFICHE_MESURES) { ToggleAffichageMesures(); return 0; }
        if (id == ID_AFFICHE_REFERENCES) { ToggleAffichageReferences(); return 0; }
        if (id == ID_AFFICHE_POINTS) { ToggleAffichagePoints(); return 0; }
        if (id == ID_AFFICHE_CHEMINEMENTS) { ToggleAffichageCheminements(); return 0; }
        if (id == ID_AFFICHE_LECTURES_NIV) { ToggleAffichageLecturesNiv(); return 0; }
        if (id == ID_AFFICHE_TOUT) { ToggleAffichageTotal(true); return 0; }
        if (id == ID_AFFICHE_RIEN) { ToggleAffichageTotal(false); return 0; }


        //menu lecture
        //...



        //menu export
        if (id == ID_EXPORT_GEOBASE) { ToggleAffichageStations(); return 0; }
        if (id == ID_EXPORT_TRIMBLE) { ToggleAffichageStations(); return 0; }
        if (id == ID_EXPORT_LEICA_WILD) { ToggleAffichageMesures(); return 0; }
        if (id == ID_EXPORT_SOKKIA) { ToggleAffichageReferences(); return 0; }
        if (id == ID_EXPORT_SEPARATEUR) { ToggleAffichagePoints(); return 0; }
        if (id == ID_EXPORT_COLONNE) { ToggleAffichageCheminements(); return 0; }
        if (id == ID_EXPORT_TOPOJIS) { ToggleAffichageLecturesNiv(); return 0; }
        if (id == ID_EXPORT_DXF) { ToggleAffichageLecturesNiv(); return 0; }
        if (id == ID_EXPORT_POINTS) { ToggleAffichageLecturesNiv(); return 0; }
        if (id == ID_EXPORT_LEICA_DBX) { ToggleAffichageLecturesNiv(); return 0; }


        //menu calculs
        if (id == ID_CALCULS_INTERSECTIONS) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_INTER_PLAN_VERT) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_RELEVEMENTS) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_RECOUPEMENTS) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_STATIONS_DEC) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_TRIANGULATION) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_V0_STATIONS) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_CHEMINEMENT_POLY) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_CALCUL_BLOC) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_POINTS_RAYONNES) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_NUAGE_POINTS) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_NIV_SIMPLE) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_NIV_DOUBLE_MIRE) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_CALCULS_NIV_DOUBLE_STATION) { ActionCalculerV0Stations(); return 0; }


        //menu outils
        if (id == ID_OUTILS_SEARCH_MAT) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_SEARCH_SYMB) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_VISUALISATION_POLYGO) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_CHEMINEMENT_AUTO) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_REIMATRICULER) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_TEST_COHERENCE) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_CREER_POINTS) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_CONTROLE_PTS_DOUBLES) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_ELIM_PTS_DOUBLES) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_TRI_FIN) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_TRI_ALTITUDE) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_CONTROLE_PRECISION) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_CHANGEMENT_SYSTEME) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_REDUCTION_REPRISES) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_CHANGEMENT_REPERE) { ActionCalculerV0Stations(); return 0; }
        if (id == ID_OUTILS_OPTIONS) { ActionCalculerV0Stations(); return 0; }


        //menu contextuel
        if (id == ID_CTX_VERS_REFERENCE)   { ActionVersReference(); return 0; }
        if (id == ID_CTX_VERS_MESURE)      { ActionVersMesure(); return 0; }
        if (id == ID_CTX_CERCLE_GAUCHE)    { ActionCercleGauche(); return 0; }
        if (id == ID_CTX_VERS_LECT_AV)     { ActionVersLectureAV(); return 0; }
        if (id == ID_CTX_VERS_LECT_AR)     { ActionVersLectureAR(); return 0; }
        if (id == ID_CTX_INVERSER_CHEMIN)  { ActionInverserChemin(); return 0; }
        if (id == ID_CTX_VERS_COMMENTAIRE) { ActionVersCommentaire(); return 0; }
        if (id == ID_CTX_RECHERCHER_MAT)   { ActionRechercherMatricule(); return 0; }
        if (id == ID_CTX_RECHERCHER_SYM)   { ActionRechercherSymbole(); return 0; }


        //ruban
        if (id == ID_TB_AJOUTER_STATION)     { ActionAjouterPointDirect(); return 0; }
        if (id == ID_TB_AJOUTER_REFERENCE)   { ActionAjouterPointDirect(); return 0; }
        if (id == ID_TB_AJOUTER_MESURE)      { ActionAjouterPointDirect(); return 0; }
        if (id == ID_TB_AJOUTER_POINT)       { ActionAjouterPointDirect(); return 0; }
        if (id == ID_TB_AJOUTER_LECT_AR)     { ActionAjouterLectureARDirect(); return 0; }
        if (id == ID_TB_AJOUTER_LECT_AV)     { ActionAjouterLectureAVDirect(); return 0; }
        if (id == ID_TB_AJOUTER_COMMANDE)    { ActionAjouterPointDirect(); return 0; }
        if (id == ID_TB_AJOUTER_COMMENTAIRE) { ActionAjouterPointDirect(); return 0; }
        if (id == ID_TB_AJOUTER_CHEMINEMENT) { ActionAjouterCheminementDirect(); return 0; }
        if (id == ID_TB_AJOUTER_APPAREIL)    { ActionAjouterPointDirect(); return 0; }

        break;
    }

    case WM_CONTEXTMENU: {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        AfficherMenuContextuel(pt);
        return 0;
    }

    case WM_CLOSE: {
        if (m_modifie) {
            int rep = MessageBox(hwnd,
                L"Des modifications n'ont pas été enregistrées. Voulez-vous les enregistrer avant de fermer ?",
                L"Konyx - GéoBase", MB_YESNOCANCEL | MB_ICONWARNING);
            if (rep == IDCANCEL) return 0;           // on annule la fermeture
            if (rep == IDYES) {
                if (!ActionEnregistrer()) return 0;  // echec/annulation de l'enregistrement -> on ne ferme pas
            }
            // IDNO : on ferme sans enregistrer
        }
        DestroyWindow(hwnd);
        return 0;
    }

    case WM_DESTROY:
        if (m_hIcon) {
            DestroyIcon(m_hIcon);
            m_hIcon = nullptr;
        }

        m_hwnd = nullptr;
        m_hwndListe = nullptr;
        m_hwndLogo = nullptr;
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void GeoBaseDialog::CreerControles(HWND hwnd) {
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icc);

    //m_hwndLogo = UI::CreerLogoKonyx(hwnd, LogoKonyxChemin);

    RECT rc; GetClientRect(hwnd, &rc);
    m_hwndListe = CreateWindowEx(
        0, WC_LISTVIEW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | WS_VSCROLL,
        0, 0, rc.right, rc.bottom,
        hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LISTE)),
        GetModuleHandle(nullptr), nullptr);
    
    m_hImageListEspacement = ImageList_Create(
        1,      // Largeur
        32,     // Hauteur des lignes en pixels
        ILC_COLOR32,
        1,
        0
    );

    ListView_SetImageList(
        m_hwndListe,
        m_hImageListEspacement,
        LVSIL_SMALL
    );

    ListView_SetExtendedListViewStyle(m_hwndListe, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    // Police a chasse fixe (type machine a ecrire), pour l'alignement
    // visuel des colonnes numeriques.
    m_hFontListe = CreateFont(
        -20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New");
    if (m_hFontListe) {
        SendMessage(m_hwndListe, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontListe), TRUE);
    }

    LVCOLUMN col{};
    col.mask = LVCF_TEXT | LVCF_WIDTH;

    auto ajouterColonne = [&](int i, const wchar_t* texte, int largeur) {
        col.pszText = const_cast<wchar_t*>(texte);
        col.cx = largeur;
        ListView_InsertColumn(m_hwndListe, i, &col);
    };

    ajouterColonne(0, L"Ligne", 130);
    ajouterColonne(1, L"Élement", 210);
    ajouterColonne(2, L"Matricule", 160);
    ajouterColonne(3, L"Paramètres", 835);
    ajouterColonne(4, L"Symbole", 105);
}

void GeoBaseDialog::ChargerFichier(const std::wstring& chemin) {
    std::string cheminA(chemin.begin(), chemin.end());

    if (!GeoBaseParser::Charger(cheminA, m_entete, m_enregistrements)) {
        MessageBox(m_hwnd, L"Impossible d'ouvrir ce fichier .geo.", L"Konyx - Erreur", MB_ICONERROR);
        return;
    }
    m_cheminFichier = chemin;
    m_modifie = false;

    // Titre de la fenetre = chemin complet du fichier ouvert.
    if (m_hwnd) {
        std::wstring titre = L"Calculs topographiques - [" +
            (m_cheminFichier.empty() ? std::wstring(L"Sans titre") : m_cheminFichier) + L"]";
        SetWindowText(m_hwnd, titre.c_str());
    }

    RafraichirListe();
}

void GeoBaseDialog::RafraichirListe() {
    int topIndexAvant = ListView_GetTopIndex(m_hwndListe);
    std::vector<int> selectionAvant = IndicesSelectionnes();

    SendMessage(m_hwndListe, WM_SETREDRAW, FALSE, 0);

    ListView_DeleteAllItems(m_hwndListe);
    for (size_t i = 0; i < m_enregistrements.size(); ++i) {
        AjouterLigneListe(static_cast<int>(i), m_enregistrements[i]);
    }

    int nb = static_cast<int>(m_enregistrements.size());
    for (int idx : selectionAvant) {
        if (idx >= 0 && idx < nb) {
            ListView_SetItemState(m_hwndListe, idx, LVIS_SELECTED, LVIS_SELECTED);
        }
    }

    if (nb > 0) {
        int cible = (topIndexAvant >= 0 && topIndexAvant < nb) ? topIndexAvant : 0;
        ListView_EnsureVisible(m_hwndListe, nb - 1, FALSE);
        ListView_EnsureVisible(m_hwndListe, cible, FALSE);
    }

    SendMessage(m_hwndListe, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndListe, nullptr, TRUE);
    UpdateWindow(m_hwndListe);
}

void GeoBaseDialog::AjouterLigneListe(int index, const GeoRecord& rec) {
    std::wostringstream numero;
    numero << std::setw(6) << std::setfill(L'0') << rec.numLigne;

    LVITEM item{};
    item.mask = LVIF_TEXT;
    item.iItem = index;
    item.iSubItem = 0;
    std::wstring sNumero = numero.str();
    item.pszText = const_cast<wchar_t*>(sNumero.c_str());
    ListView_InsertItem(m_hwndListe, &item);

    std::string element = FormatElement(rec);
    std::wstring wElement(element.begin(), element.end());
    ListView_SetItemText(m_hwndListe, index, 1, const_cast<wchar_t*>(wElement.c_str()));

    std::wstring wMatricule(rec.matricule.begin(), rec.matricule.end());
    ListView_SetItemText(m_hwndListe, index, 2, const_cast<wchar_t*>(wMatricule.c_str()));

    std::string params = FormatParametres(rec);
    std::wstring wParams(params.begin(), params.end());
    ListView_SetItemText(m_hwndListe, index, 3, const_cast<wchar_t*>(wParams.c_str()));

    std::wstring wSymbole = rec.SY ? std::to_wstring(*rec.SY) : L"";
    ListView_SetItemText(m_hwndListe, index, 4, const_cast<wchar_t*>(wSymbole.c_str()));
}

COLORREF GeoBaseDialog::CouleurPourType(GeoType type) const {
    switch (type) {
    case GeoType::Station:          return RGB(255, 0, 0);
    case GeoType::Reference:        return RGB(0, 0, 255);
    case GeoType::Mesure:           return RGB(0, 128, 192);
    case GeoType::Point:            return RGB(0, 128, 0);
    case GeoType::NivLectAV:        return RGB(0, 0, 0);
    case GeoType::NivLectAR:        return RGB(0, 0, 0);
    case GeoType::CheminAntenne:    return RGB(0, 0, 0);
    case GeoType::CheminEncadre:    return RGB(0, 0, 0);
    case GeoType::Commentaire:      return RGB(150, 150, 150);
    default:                        return RGB(0, 0, 0);
    }
}

void GeoBaseDialog::GererDoubleClicListe(int index) {
    while (index >= 0 && index < static_cast<int>(m_enregistrements.size())) {
        GeoRecord& rec = m_enregistrements[index];
        std::optional<GeoType> typeSuivant;
        if (index + 1 < static_cast<int>(m_enregistrements.size()))
            typeSuivant = m_enregistrements[index + 1].type;

        bool allerSuivant = false;
        if (!GeoEditDialog::Modifier(m_hwnd, rec, static_cast<int>(m_enregistrements.size()), typeSuivant, allerSuivant))
            break; // Annuler

        m_modifie = true;
        int cibleIdx = rec.numLigne - 1;
        if (cibleIdx != index && cibleIdx >= 0 && cibleIdx < static_cast<int>(m_enregistrements.size())) {
            GeoRecord copie = rec;
            m_enregistrements.erase(m_enregistrements.begin() + index);
            m_enregistrements.insert(m_enregistrements.begin() + cibleIdx, copie);
            index = cibleIdx;
        }
        RenumeroterLignes();
        RafraichirListe();

        if (!allerSuivant) break;
        index += 1; // passe a la ligne suivante
    }
}

void GeoBaseDialog::Fermer() {
    if (m_hwnd) DestroyWindow(m_hwnd);
}

void GeoBaseDialog::Afficher(HWND hwndParent) {
    WNDCLASS wc{};
    wc.lpfnWndProc = FenetreProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = ClasseFenetre;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    if (hwndParent) EnableWindow(hwndParent, FALSE);

    // Titre initial : chemin si deja connu, sinon titre generique
    // (sera remplace par ChargerFichier si un fichier est ouvert ensuite).
    std::wstring titreInitial = L"Calculs topographiques - [" + (m_cheminFichier.empty() ? std::wstring(L"Sans titre") : m_cheminFichier) + L"]";

    HWND hwnd = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        ClasseFenetre, titreInitial.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1500, 1000,
        hwndParent, nullptr, GetModuleHandle(nullptr), this);

    ACCEL accels[] = {
        { FVIRTKEY | FCONTROL, 'N', ID_FICHIER_NOUVEAU },
        { FVIRTKEY | FCONTROL, 'O', ID_FICHIER_OUVRIR },
        { FVIRTKEY | FCONTROL, 'S', ID_FICHIER_ENREGISTRER },
        { FVIRTKEY | FCONTROL, 'G', ID_FICHIER_IMPRIMER_GEOBASE },
        { FVIRTKEY | FCONTROL, 'P', ID_FICHIER_IMPRIMER_LISTING },
        { FVIRTKEY | FCONTROL, 'Q', ID_FICHIER_QUITTER },

        { FVIRTKEY | FCONTROL, 'Z', ID_EDITION_ANNULER },
        { FVIRTKEY | FCONTROL, 'Y', ID_EDITION_RETABLIR },
        { FVIRTKEY, VK_DELETE, ID_EDITION_SUPPRIMER },
        { FVIRTKEY, VK_INSERT, ID_EDITION_AJOUTER },

        { FVIRTKEY | FCONTROL | FSHIFT, 'O', ID_OUTILS_OPTIONS },
    };
    m_hAccel = CreateAcceleratorTable(accels, static_cast<int>(std::size(accels)));
    
    m_hIcon = (HICON)LoadImage(
        nullptr,
        LogoKonyxChemin.c_str(),
        IMAGE_ICON,
        32, 32,
        LR_LOADFROMFILE
    );

    if (m_hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!m_hAccel || !TranslateAccelerator(hwnd, m_hAccel, &msg)) {
            if (!IsDialogMessage(hwnd, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        if (!IsWindow(hwnd)) break;
    }

    if (hwndParent) {
        EnableWindow(hwndParent, TRUE);

        SetWindowPos(
            hwndParent,
            HWND_TOP,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
        );

        SetForegroundWindow(hwndParent);
        SetFocus(hwndParent);
    }
}


// --- Menu Fichier ---------------------------------------------------------

void GeoBaseDialog::CreerMenu(HWND hwnd) {
    m_hMenu = CreateMenu();
    HMENU hMenuFichier = CreatePopupMenu();

    AppendMenu(hMenuFichier, MF_STRING, ID_FICHIER_NOUVEAU,          L"&Nouveau\tCtrl+N");
    AppendMenu(hMenuFichier, MF_STRING, ID_FICHIER_OUVRIR,           L"&Ouvrir...\tCtrl+O");
    AppendMenu(hMenuFichier, MF_STRING, ID_FICHIER_ENREGISTRER,      L"&Enregistrer\tCtrl+S");
    AppendMenu(hMenuFichier, MF_STRING, ID_FICHIER_ENREGISTRER_SOUS, L"Enregistrer &sous...");
    AppendMenu(hMenuFichier, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuFichier, MF_STRING, ID_FICHIER_CONCATENER,       L"&Concaténer...");
    AppendMenu(hMenuFichier, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuFichier, MF_STRING, ID_FICHIER_IMPRIMER_GEOBASE, L"Imprimer &GéoBase\tCtrl+G");
    AppendMenu(hMenuFichier, MF_STRING, ID_FICHIER_IMPRIMER_LISTING, L"Imprimer le &listing\tCtrl+P");
    AppendMenu(hMenuFichier, MF_SEPARATOR, 0, nullptr);
    // Les 5 entrees "recents" sont inserees par RafraichirMenuRecents()
    AppendMenu(hMenuFichier, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuFichier, MF_STRING, ID_FICHIER_QUITTER,          L"&Quitter\tCtrl+Q");

    AppendMenu(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuFichier), L"&Fichier");
    SetMenu(hwnd, m_hMenu);

    m_hMenuFichier = GetSubMenu(m_hMenu, 0);
    CreerMenuEdition();
    CreerMenuAffichage();
    CreerMenuLecture();
    CreerMenuCalculs();
    CreerMenuOutils();
    CreerMenuCodification();

    RafraichirMenuRecents();
}

void GeoBaseDialog::RafraichirMenuRecents() {
    if (!m_hMenu) return;
    HMENU hMenuFichier = GetSubMenu(m_hMenu, 0);
    if (!hMenuFichier) return;

    // Retire les anciennes entrees "recent" si presentes (IDs 2010-2014)
    for (int i = 0; i < 5; ++i) {
        RemoveMenu(hMenuFichier, ID_FICHIER_RECENT_BASE + i, MF_BYCOMMAND);
    }

    // Les reinsere juste avant le dernier separateur (celui avant Quitter).
    // On recherche la position du separateur precedant "Quitter".
    int nbItems = GetMenuItemCount(hMenuFichier);
    int posInsertion = nbItems - 2; // entre les deux separateurs, avant celui qui precede "Quitter"

    for (size_t i = 0; i < m_recents.size() && i < 5; ++i) {
        std::wstring libelle = L"&" + std::to_wstring(i + 1) + L" " + m_recents[i];
        InsertMenu(hMenuFichier, posInsertion + static_cast<int>(i), MF_BYPOSITION | MF_STRING,
                   ID_FICHIER_RECENT_BASE + i, libelle.c_str());
    }

    if (m_hwnd) DrawMenuBar(m_hwnd);
}

namespace {
    std::wstring CheminFichierRecents() {
        wchar_t chemin[MAX_PATH]{};
        DWORD n = GetEnvironmentVariable(L"APPDATA", chemin, MAX_PATH);
        std::wstring dossier = (n > 0) ? std::wstring(chemin) : L".";
        dossier += L"\\Konyx";
        CreateDirectory(dossier.c_str(), nullptr);
        return dossier + L"\\geobase_recents.txt";
    }
}

void GeoBaseDialog::ChargerRecents() {
    m_recents.clear();
    std::wifstream f(CheminFichierRecents());
    if (!f.is_open()) return;
    std::wstring ligne;
    while (std::getline(f, ligne) && m_recents.size() < 5) {
        if (!ligne.empty()) m_recents.push_back(ligne);
    }
}

void GeoBaseDialog::SauvegarderRecents() {
    std::wofstream f(CheminFichierRecents(), std::ios::trunc);
    if (!f.is_open()) return;
    for (const auto& chemin : m_recents) f << chemin << L"\n";
}

void GeoBaseDialog::AjouterRecent(const std::wstring& chemin) {
    // Retire les doublons eventuels, puis place en tete.
    m_recents.erase(std::remove(m_recents.begin(), m_recents.end(), chemin), m_recents.end());
    m_recents.insert(m_recents.begin(), chemin);
    if (m_recents.size() > 5) m_recents.resize(5);
    SauvegarderRecents();
    RafraichirMenuRecents();
}

void GeoBaseDialog::ActionNouveau() {
    // TODO : proposer de sauvegarder si m_modifie est vrai, avant de reinitialiser.
    m_enregistrements.clear();
    m_entete = GeoBaseEntete{};
    m_cheminFichier.clear();
    m_modifie = false;
    if (m_hwnd) SetWindowText(m_hwnd, L"Calculs topographiques - [nouveau]");
    RafraichirListe();
}

void GeoBaseDialog::ActionOuvrir() {
    wchar_t cheminBuf[MAX_PATH] = L"";
    OPENFILENAME ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"Fichiers GéoBase (*.geo)\0*.geo\0Tous les fichiers (*.*)\0*.*\0";
    ofn.lpstrFile = cheminBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"geo";

    if (GetOpenFileName(&ofn)) {
        ChargerFichier(cheminBuf);
        AjouterRecent(cheminBuf);
    }
}

bool GeoBaseDialog::ActionEnregistrer() {
    if (m_cheminFichier.empty()) return ActionEnregistrerSous();

    std::string cheminA(m_cheminFichier.begin(), m_cheminFichier.end());
    if (!GeoBaseParser::Sauvegarder(cheminA, m_entete, m_enregistrements)) {
        MessageBox(m_hwnd, L"Echec de l'enregistrement.", L"Konyx - Erreur", MB_ICONERROR);
        return false;
    }
    m_modifie = false;
    return true;
}

bool GeoBaseDialog::ActionEnregistrerSous() {
    wchar_t cheminBuf[MAX_PATH] = L"";
    if (!m_cheminFichier.empty()) {
        wcsncpy_s(cheminBuf, m_cheminFichier.c_str(), MAX_PATH - 1);
    }

    OPENFILENAME ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"Fichiers GéoBase (*.geo)\0*.geo\0Tous les fichiers (*.*)\0*.*\0";
    ofn.lpstrFile = cheminBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"geo";

    if (!GetSaveFileName(&ofn)) return false;

    m_cheminFichier = cheminBuf;
    std::string cheminA(m_cheminFichier.begin(), m_cheminFichier.end());
    if (!GeoBaseParser::Sauvegarder(cheminA, m_entete, m_enregistrements)) {
        MessageBox(m_hwnd, L"Échec de l'enregistrement.", L"Konyx - Erreur", MB_ICONERROR);
        return false;
    }
    m_modifie = false;
    if (m_hwnd) SetWindowText(m_hwnd, m_cheminFichier.c_str());
    AjouterRecent(m_cheminFichier);
    return true;
}

void GeoBaseDialog::ActionConcatener() {
    // TODO : choisir un second .geo, renumeroter ses lignes a la suite des
    // lignes courantes, et fusionner les deux listes d'enregistrements.
    MessageBox(m_hwnd, L"Concaténer... (à implémenter)", L"Konyx - GéoBase", MB_ICONINFORMATION);
}

void GeoBaseDialog::ActionImprimerGeoBase() {
    // TODO : impression du tableau (PrintDlg + rendu GDI des lignes).
    MessageBox(m_hwnd, L"Imprimer GéoBase (à implémenter)", L"Konyx - GéoBase", MB_ICONINFORMATION);
}

void GeoBaseDialog::ActionImprimerListing() {
    // TODO : variante "listing" (format rapport plutot que tableau brut).
    MessageBox(m_hwnd, L"Imprimer le listing (à implémenter)", L"Konyx - GéoBase", MB_ICONINFORMATION);
}

void GeoBaseDialog::ActionOuvrirRecent(int indice) {
    if (indice < 0 || indice >= static_cast<int>(m_recents.size())) return;
    std::wstring chemin = m_recents[indice];
    ChargerFichier(chemin);
    AjouterRecent(chemin); // remonte en tete de liste
}



// --- Menu Edition -----------------------------------------------------

void GeoBaseDialog::CreerMenuEdition() {
    m_hMenuEdition = CreatePopupMenu();

    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_ANNULER,  L"&Annuler\tCtrl+Z");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_RETABLIR, L"&Rétablir\tCtrl+Y");
    AppendMenu(m_hMenuEdition, MF_SEPARATOR, 0, nullptr);
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_MODIFIER,  L"&Modifier...");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_SUPPRIMER, L"&Supprimer\tSuppr");
    AppendMenu(m_hMenuEdition, MF_SEPARATOR, 0, nullptr);
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_AJOUTER,        L"A&jouter...\tInser");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_INSERER_AVANT, L"Insérer a&vant...");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_INSERER_APRES, L"Insérer a&près...");
    AppendMenu(m_hMenuEdition, MF_SEPARATOR, 0, nullptr);
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_CHANGER_HP,          L"Changer &HP...");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_CHANGER_CSTE_PRISME, L"Changer &Cste Prisme...");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_CHANGER_ALTITUDE,    L"Changer &altitude...");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_CHANGER_SYMBOLE,     L"Changer &symbole");
    AppendMenu(m_hMenuEdition, MF_SEPARATOR, 0, nullptr);
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_XY_ZERO, L"Mettre &X et Y a 0.0");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_Z_ZERO,  L"Mettre &Z a 0.0");
    AppendMenu(m_hMenuEdition, MF_SEPARATOR, 0, nullptr);
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_SUPPR_V0,       L"Supprimer &V0");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_SUPPR_HP_AV_DI, L"Supprimer HP, AV et &DI");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_SUPPR_Z,        L"Supprimer &Z");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_SUPPR_XY,       L"Supprimer &XY");
    AppendMenu(m_hMenuEdition, MF_SEPARATOR, 0, nullptr);
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_XY_CONSTANT, L"Rendre X et Y &constants");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_Z_CONSTANT,  L"Rendre Z c&onstant");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_XY_VARIABLE, L"Rendre X et Y &variables");
    AppendMenu(m_hMenuEdition, MF_STRING, ID_EDITION_Z_VARIABLE,  L"Rendre Z v&ariable");

    InsertMenu(m_hMenu, 1, MF_BYPOSITION | MF_POPUP,
               reinterpret_cast<UINT_PTR>(m_hMenuEdition), L"&Édition");
}

void GeoBaseDialog::MettreAJourEtatsMenuEdition() {
    if (!m_hMenuEdition) return;

    std::vector<int> sel = IndicesSelectionnes();
    bool unSeul = (sel.size() == 1);
    bool auMoinsUn = !sel.empty();

    auto def = [&](int id, bool actif) {
        EnableMenuItem(m_hMenuEdition, id, MF_BYCOMMAND | (actif ? MF_ENABLED : MF_GRAYED));
    };

    def(ID_EDITION_ANNULER, !m_pileAnnuler.empty());
    def(ID_EDITION_RETABLIR, !m_pileRetablir.empty());
    def(ID_EDITION_MODIFIER, unSeul);
    def(ID_EDITION_SUPPRIMER, auMoinsUn);
    def(ID_EDITION_INSERER_AVANT, unSeul);
    def(ID_EDITION_INSERER_APRES, unSeul);
    def(ID_EDITION_CHANGER_HP, auMoinsUn);
    def(ID_EDITION_CHANGER_CSTE_PRISME, auMoinsUn);
    def(ID_EDITION_CHANGER_ALTITUDE, auMoinsUn);
    def(ID_EDITION_CHANGER_SYMBOLE, auMoinsUn);
    def(ID_EDITION_XY_ZERO, auMoinsUn);
    def(ID_EDITION_Z_ZERO, auMoinsUn);
    def(ID_EDITION_SUPPR_V0, auMoinsUn);
    def(ID_EDITION_SUPPR_HP_AV_DI, auMoinsUn);
    def(ID_EDITION_SUPPR_Z, auMoinsUn);
    def(ID_EDITION_SUPPR_XY, auMoinsUn);
    def(ID_EDITION_XY_CONSTANT, auMoinsUn);
    def(ID_EDITION_Z_CONSTANT, auMoinsUn);
    def(ID_EDITION_XY_VARIABLE, auMoinsUn);
    def(ID_EDITION_Z_VARIABLE, auMoinsUn);
}

std::vector<int> GeoBaseDialog::IndicesSelectionnes() const {
    std::vector<int> indices;
    int i = -1;
    while ((i = ListView_GetNextItem(m_hwndListe, i, LVNI_SELECTED)) != -1) {
        indices.push_back(i);
    }
    return indices;
}

void GeoBaseDialog::SauvegarderEtatPourAnnulation() {
    m_pileAnnuler.push_back(m_enregistrements);
    if (m_pileAnnuler.size() > MAX_HISTORIQUE) m_pileAnnuler.erase(m_pileAnnuler.begin());
    m_pileRetablir.clear();
}

void GeoBaseDialog::ActionAnnuler() {
    if (m_pileAnnuler.empty()) return;
    m_pileRetablir.push_back(m_enregistrements);
    m_enregistrements = m_pileAnnuler.back();
    m_pileAnnuler.pop_back();
    m_modifie = true;
    RenumeroterLignes();
    RafraichirListe();
}

void GeoBaseDialog::ActionRetablir() {
    if (m_pileRetablir.empty()) return;
    m_pileAnnuler.push_back(m_enregistrements);
    m_enregistrements = m_pileRetablir.back();
    m_pileRetablir.pop_back();
    m_modifie = true;
    RenumeroterLignes();
    RafraichirListe();
}

void GeoBaseDialog::ActionModifier() {
    int index = ListView_GetNextItem(m_hwndListe, -1, LVNI_SELECTED);
    if (index < 0) return;
    GererDoubleClicListe(index);
}

void GeoBaseDialog::ActionSupprimer() {
    std::vector<int> sel = IndicesSelectionnes();
    if (sel.empty()) return;

    int rep = MessageBox(m_hwnd,
        (L"Supprimer " + std::to_wstring(sel.size()) + L" élément(s) sélectionné(s) ?").c_str(),
        L"Konyx - Confirmation", MB_YESNO | MB_ICONQUESTION);
    if (rep != IDYES) return;

    SauvegarderEtatPourAnnulation();

    // Suppression par indices decroissants pour ne pas decaler les positions.
    std::sort(sel.rbegin(), sel.rend());
    for (int idx : sel) {
        if (idx >= 0 && idx < static_cast<int>(m_enregistrements.size())) {
            m_enregistrements.erase(m_enregistrements.begin() + idx);
        }
    }
    m_modifie = true;
    RenumeroterLignes();
    RafraichirListe();
}

// Boite de choix du type, puis boite d'edition en mode creation, puis
// insertion a la position demandee (1-based).
void GeoBaseDialog::AjouterElement(int positionCible) {
    std::optional<GeoType> type = GeoAjouterTypeDialog::Choisir(m_hwnd);
    if (!type) return; // Annuler

    const int nbApresAjout = static_cast<int>(m_enregistrements.size()) + 1;

    GeoRecord nouveau;
    nouveau.type = *type;
    nouveau.numLigne = positionCible;

    bool allerSuivant = false;
    if (!GeoEditDialog::Modifier(m_hwnd, nouveau, nbApresAjout,
                                 std::nullopt, allerSuivant, /*creation=*/true))
        return; // Annule, ou type non pris en charge (message deja affiche)

    SauvegarderEtatPourAnnulation();
    int cible = nouveau.numLigne - 1;
    if (cible < 0) cible = 0;
    if (cible > static_cast<int>(m_enregistrements.size()))
        cible = static_cast<int>(m_enregistrements.size());
    m_enregistrements.insert(m_enregistrements.begin() + cible, nouveau);
    m_modifie = true;
    RenumeroterLignes();
    RafraichirListe();
}

void GeoBaseDialog::ActionAjouter() {
    AjouterElement(static_cast<int>(m_enregistrements.size()) + 1); // en fin de GeoBase
}

void GeoBaseDialog::ActionInsererAvant() {
    int index = ListView_GetNextItem(m_hwndListe, -1, LVNI_SELECTED);
    if (index < 0) return;
    AjouterElement(index + 1);
}

void GeoBaseDialog::ActionInsererApres() {
    int index = ListView_GetNextItem(m_hwndListe, -1, LVNI_SELECTED);
    if (index < 0) return;
    AjouterElement(index + 2);
}

void GeoBaseDialog::ActionAjouterPointDirect() {
    GeoRecord nouveau;
    nouveau.type = GeoType::Point;
    nouveau.numLigne = static_cast<int>(m_enregistrements.size()) + 1;

    bool allerSuivant = false;
    if (GeoEditDialog::Modifier(m_hwnd, nouveau, static_cast<int>(m_enregistrements.size()) + 1,
                                 std::nullopt, allerSuivant, /*creation=*/true)) {
        SauvegarderEtatPourAnnulation();
        int cible = nouveau.numLigne - 1;
        if (cible < 0) cible = 0;
        if (cible > static_cast<int>(m_enregistrements.size())) cible = static_cast<int>(m_enregistrements.size());
        m_enregistrements.insert(m_enregistrements.begin() + cible, nouveau);
        m_modifie = true;
        RenumeroterLignes();
        RafraichirListe();
    }
}

/*void GeoBaseDialog::ActionAjouterLectureARDirect() {
    GeoRecord nouveau;
    nouveau.type = GeoType::NivLectAR;
    nouveau.numLigne = static_cast<int>(m_enregistrements.size()) + 1;

    bool allerSuivant = false;
    if (GeoEditDialog::Modifier(m_hwnd, nouveau, static_cast<int>(m_enregistrements.size()) + 1,
                                 std::nullopt, allerSuivant, true)) {
        SauvegarderEtatPourAnnulation();
        int cible = nouveau.numLigne - 1;
        if (cible < 0) cible = 0;
        if (cible > static_cast<int>(m_enregistrements.size())) cible = static_cast<int>(m_enregistrements.size());
        m_enregistrements.insert(m_enregistrements.begin() + cible, nouveau);
        m_modifie = true;
        RenumeroterLignes();
        RafraichirListe();
    }
}

void GeoBaseDialog::ActionAjouterLectureAVDirect() {
    GeoRecord nouveau;
    nouveau.type = GeoType::NivLectAV;
    nouveau.numLigne = static_cast<int>(m_enregistrements.size()) + 1;

    bool allerSuivant = false;
    if (GeoEditDialog::Modifier(m_hwnd, nouveau, static_cast<int>(m_enregistrements.size()) + 1,
                                 std::nullopt, allerSuivant, creation=true)) {
        SauvegarderEtatPourAnnulation();
        int cible = nouveau.numLigne - 1;
        if (cible < 0) cible = 0;
        if (cible > static_cast<int>(m_enregistrements.size())) cible = static_cast<int>(m_enregistrements.size());
        m_enregistrements.insert(m_enregistrements.begin() + cible, nouveau);
        m_modifie = true;
        RenumeroterLignes();
        RafraichirListe();
    }
}
*/

void GeoBaseDialog::ActionAjouterLectureARDirect() {
    CreerLecturesSuccessives(GeoType::NivLectAR);
}

void GeoBaseDialog::ActionAjouterLectureAVDirect() {
    CreerLecturesSuccessives(GeoType::NivLectAV);
}

void GeoBaseDialog::CreerLecturesSuccessives(GeoType typeInitial) {
    GeoType typeCourant = typeInitial;
    std::string matriculeSuivant;

    while (true) {
        // Préparation d'une nouvelle lecture à la fin de la géobase.
        GeoRecord nouveau;
        nouveau.type = typeCourant;
        nouveau.numLigne = static_cast<int>(m_enregistrements.size()) + 1;
        nouveau.matricule = matriculeSuivant;

        bool allerSuivant = false;

        // Ouverture de la fenêtre de création.
        if (!GeoEditDialog::Modifier(
                m_hwnd,
                nouveau,
                static_cast<int>(m_enregistrements.size()) + 1,
                std::nullopt,
                allerSuivant,
                /*creation=*/true)) {
            // Annulation : on conserve toutes les lectures précédemment validées.
            break;
        }

        // La lecture validée est ajoutée définitivement à la géobase.
        SauvegarderEtatPourAnnulation();

        nouveau.numLigne = static_cast<int>(m_enregistrements.size()) + 1;
        m_enregistrements.push_back(nouveau);

        m_modifie = true;
        RenumeroterLignes();
        RafraichirListe();

        // Préparation du matricule pour la lecture suivante.
        // Préparation du matricule pour la lecture suivante.
        if (typeCourant == GeoType::NivLectAR) {
            // AR -> AV : on incrémente le matricule.
            matriculeSuivant = IncrementerMatricule(nouveau.matricule);
        }
        else if (typeCourant == GeoType::NivLectAV) {
            // AV -> AR : on conserve le même matricule.
            matriculeSuivant = nouveau.matricule;
        }

        // Clic sur OK : fin de la séquence.
        if (!allerSuivant)
            break;

        // Clic sur Lecture Av/Ar : préparation du type opposé.
        if (typeCourant == GeoType::NivLectAR)
            typeCourant = GeoType::NivLectAV;
        else
            typeCourant = GeoType::NivLectAR;
    }
}

void GeoBaseDialog::ActionAjouterCheminementDirect() {
    MessageBox(m_hwnd, L"L'édition des cheminements n'est pas encore implémentée.",
               L"Konyx - GeoBase", MB_ICONINFORMATION);
}

void GeoBaseDialog::ActionChangerHP() {
    // TODO : boite "ajouter une constante" / "nouvelle valeur" (voir doc Covadis).
    MessageBox(m_hwnd, L"Changer HP... (à implémenter)", L"Konyx - GéoBase", MB_ICONINFORMATION);
}

void GeoBaseDialog::ActionChangerCstePrisme() {
    // TODO : meme principe que Changer HP.
    MessageBox(m_hwnd, L"Changer Cste Prisme... (à implémenter)", L"Konyx - GéoBase", MB_ICONINFORMATION);
}

void GeoBaseDialog::ActionChangerAltitude() {
    // TODO : meme principe que Changer HP, mais sur Z des points.
    MessageBox(m_hwnd, L"Changer altitude... (à implémenter)", L"Konyx - GéoBase", MB_ICONINFORMATION);
}

void GeoBaseDialog::ActionChangerSymbole() {
    std::vector<int> sel = IndicesSelectionnes();
    if (sel.empty()) return;

    // Saisie simple via InputBox systeme minimal (boite de confirmation
    // Windows n'a pas de champ de saisie natif : on utilise ici une
    // approche temporaire par boite de dialogue systeme de type "GetOpenFileName"
    // n'etant pas adaptee, on garde un stub tant que le prompt generique
    // n'est pas encore code (voir TODO global "boite de saisie reutilisable").
    MessageBox(m_hwnd, L"Changer symbole (saisie à implémenter)", L"Konyx - GéoBase", MB_ICONINFORMATION);
}

void GeoBaseDialog::ActionXYaZero() {
    std::vector<int> sel = IndicesSelectionnes();
    if (sel.empty()) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) {
        if (m_enregistrements[idx].type == GeoType::Point) {
            m_enregistrements[idx].X = 0.0;
            m_enregistrements[idx].Y = 0.0;
        }
    }
    m_modifie = true;
    RafraichirListe();
}

void GeoBaseDialog::ActionZaZero() {
    std::vector<int> sel = IndicesSelectionnes();
    if (sel.empty()) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) {
        if (m_enregistrements[idx].type == GeoType::Point) {
            m_enregistrements[idx].Z = 0.0;
        }
    }
    m_modifie = true;
    RafraichirListe();
}

void GeoBaseDialog::ActionSupprV0() {
    std::vector<int> sel = IndicesSelectionnes();
    if (sel.empty()) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) {
        if (m_enregistrements[idx].type == GeoType::Station) {
            m_enregistrements[idx].V0.reset();
        }
    }
    m_modifie = true;
    RafraichirListe();
}

void GeoBaseDialog::ActionSupprHpAvDi() {
    std::vector<int> sel = IndicesSelectionnes();
    if (sel.empty()) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) {
        auto& rec = m_enregistrements[idx];
        if (rec.type == GeoType::Reference || rec.type == GeoType::Mesure) {
            rec.HP.reset();
            rec.AV.reset();
            rec.DI.reset();
        }
    }
    m_modifie = true;
    RafraichirListe();
}

void GeoBaseDialog::ActionSupprZ() {
    std::vector<int> sel = IndicesSelectionnes();
    if (sel.empty()) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) {
        if (m_enregistrements[idx].type == GeoType::Point) {
            m_enregistrements[idx].Z.reset();
        }
    }
    m_modifie = true;
    RafraichirListe();
}

void GeoBaseDialog::ActionSupprXY() {
    std::vector<int> sel = IndicesSelectionnes();
    if (sel.empty()) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) {
        if (m_enregistrements[idx].type == GeoType::Point) {
            m_enregistrements[idx].X.reset();
            m_enregistrements[idx].Y.reset();
        }
    }
    m_modifie = true;
    RafraichirListe();
}

// CP (constance du point) : bit0 = X,Y invariants (valeurs 1 ou 3),
// bit1 = Z invariant (valeurs 2 ou 3). Voir doc Covadis - Types d'elements.
void GeoBaseDialog::ActionXYConstant(bool constant) {
    std::vector<int> sel = IndicesSelectionnes();
    if (sel.empty()) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) {
        auto& rec = m_enregistrements[idx];
        if (rec.type != GeoType::Point) continue;
        int cp = rec.CP.value_or(0);
        if (constant) cp |= 1; else cp &= ~1;
        rec.CP = cp;
    }
    m_modifie = true;
    RafraichirListe();
}

void GeoBaseDialog::ActionZConstant(bool constant) {
    std::vector<int> sel = IndicesSelectionnes();
    if (sel.empty()) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) {
        auto& rec = m_enregistrements[idx];
        if (rec.type != GeoType::Point) continue;
        int cp = rec.CP.value_or(0);
        if (constant) cp |= 2; else cp &= ~2;
        rec.CP = cp;
    }
    m_modifie = true;
    RafraichirListe();
}



// --- Menu Affichage -----------------------------------------------------

void GeoBaseDialog::CreerMenuAffichage() {
    HMENU hMenuAffichage = CreatePopupMenu();
    AppendMenu(hMenuAffichage, MF_STRING, ID_AFFICHE_STATIONS, L"&Stations");
    AppendMenu(hMenuAffichage, MF_STRING, ID_AFFICHE_MESURES, L"&Mesures");
    AppendMenu(hMenuAffichage, MF_STRING, ID_AFFICHE_REFERENCES, L"&Références");
    AppendMenu(hMenuAffichage, MF_STRING, ID_AFFICHE_POINTS, L"&Points");
    AppendMenu(hMenuAffichage, MF_STRING, ID_AFFICHE_CHEMINEMENTS, L"&Cheminements");
    AppendMenu(hMenuAffichage, MF_STRING, ID_AFFICHE_LECTURES_NIV, L"&Lectures de nivellement");
    AppendMenu(hMenuAffichage, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuAffichage, MF_STRING, ID_AFFICHE_TOUT, L"Tout &afficher");
    AppendMenu(hMenuAffichage, MF_STRING, ID_AFFICHE_RIEN, L"Tout &masquer");

    AppendMenu(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuAffichage), L"&Affichage");
    m_hMenuAffichage = GetSubMenu(m_hMenu, 2);
}

void GeoBaseDialog::ToggleAffichageStations() {
    // code à faire
    RafraichirListe();
}

void GeoBaseDialog::ToggleAffichageMesures() {
    // code à faire
    RafraichirListe();
}

void GeoBaseDialog::ToggleAffichageReferences() {
    // code à faire
    RafraichirListe();
}

void GeoBaseDialog::ToggleAffichagePoints() {
    // code à faire
    RafraichirListe();
}

void GeoBaseDialog::ToggleAffichageCheminements() {
    // code à faire
    RafraichirListe();
}

void GeoBaseDialog::ToggleAffichageLecturesNiv() {
    // code à faire
    RafraichirListe();
}

void GeoBaseDialog::ToggleAffichageTotal(bool typeAffichage) {
    // code à faire
    RafraichirListe();
}


// --- Menu Lecture -----------------------------------------------------

void GeoBaseDialog::CreerMenuLecture() {
    HMENU hMenuLecture = CreatePopupMenu();
    AppendMenu(hMenuLecture, MF_STRING, ID_LECTURE_1, L"Lecture &1");
    AppendMenu(hMenuLecture, MF_STRING, ID_LECTURE_2, L"Lecture &2");

    AppendMenu(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuLecture), L"&Lecture");
    m_hMenuLecture = GetSubMenu(m_hMenu, 3);
}


// --- Menu Lecture -----------------------------------------------------

void GeoBaseDialog::CreerMenuExport() {
    HMENU hMenuExport = CreatePopupMenu();
    AppendMenu(hMenuExport, MF_STRING, ID_EXPORT_GEOBASE, L"Format &Géobase");
    AppendMenu(hMenuExport, MF_STRING, ID_EXPORT_TRIMBLE, L"Format &Trimble GDM");
    AppendMenu(hMenuExport, MF_STRING, ID_EXPORT_LEICA_WILD, L"Format Wild-&Leica");
    AppendMenu(hMenuExport, MF_STRING, ID_EXPORT_SOKKIA, L"Format So&kkia");
    AppendMenu(hMenuExport, MF_STRING, ID_EXPORT_SEPARATEUR, L"Format &séparateur");
    AppendMenu(hMenuExport, MF_STRING, ID_EXPORT_COLONNE, L"Format &colonné");
    AppendMenu(hMenuExport, MF_STRING, ID_EXPORT_TOPOJIS, L"Format &Topojis-&PC");
    AppendMenu(hMenuExport, MF_STRING, ID_EXPORT_DXF, L"Format &Dxf version 12");
    AppendMenu(hMenuExport, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuExport, MF_STRING, ID_EXPORT_POINTS, L"&Exportation de points...");
    AppendMenu(hMenuExport, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuExport, MF_STRING, ID_EXPORT_LEICA_DBX, L"Export. Leica DB-&X");

    AppendMenu(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuExport), L"&Export");
    m_hMenuExport = GetSubMenu(m_hMenu, 4);
}



// --- Menu Calculs -----------------------------------------------------

void GeoBaseDialog::CreerMenuCalculs() {
    HMENU hMenuCalculs = CreatePopupMenu();
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_INTERSECTIONS, L"&Intersections");
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_INTER_PLAN_VERT, L"Intersections / &Plan vertical");
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_RELEVEMENTS, L"Re&lèvements");
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_RECOUPEMENTS, L"Re&coupements");
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_STATIONS_DEC, L"&Stations decalées");
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_TRIANGULATION, L"&Triangulation");
    AppendMenu(hMenuCalculs, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_V0_STATIONS, L"&V0 des stations");
    AppendMenu(hMenuCalculs, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_CHEMINEMENT_POLY, L"&Cheminements polygonaux");
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_CALCUL_BLOC, L"Calcul en &bloc du réseau");
    AppendMenu(hMenuCalculs, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_POINTS_RAYONNES, L"Points &rayonnés");
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_NUAGE_POINTS, L"Recalcul &nuages de points");
    AppendMenu(hMenuCalculs, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_NIV_SIMPLE, L"&Nivellement simple");
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_NIV_DOUBLE_MIRE, L"Nivellement double &mire");
    AppendMenu(hMenuCalculs, MF_STRING, ID_CALCULS_NIV_DOUBLE_STATION, L"Nivellement &double station");

    AppendMenu(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuCalculs), L"&Calculs");
    m_hMenuCalculs = GetSubMenu(m_hMenu, 5);
}

void GeoBaseDialog::ActionCalculerV0Stations() {
    GeoCalculV0Dialog::Afficher(m_hwnd, m_enregistrements);
    RafraichirListe();
}



// --- Menu Outils -----------------------------------------------------

void GeoBaseDialog::CreerMenuOutils() {
    HMENU hMenuOutils = CreatePopupMenu();
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_SEARCH_MAT, L"Rechercher &Matricule...");
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_SEARCH_SYMB, L"Rechercher &Symbole...");
    AppendMenu(hMenuOutils, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_VISUALISATION_POLYGO, L"&Visualisation la polygonation");
    AppendMenu(hMenuOutils, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_CHEMINEMENT_AUTO, L"&Cheminements auto.");
    AppendMenu(hMenuOutils, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_REIMATRICULER, L"&Réimatriculer...");
    AppendMenu(hMenuOutils, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_TEST_COHERENCE, L"&Tester la cohérence");
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_CREER_POINTS, L"Créer tous les &points");
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_CONTROLE_PTS_DOUBLES, L"Contrôle des points &doubles (code)");
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_ELIM_PTS_DOUBLES, L"&Élimination des points doubles (matricule)");
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_TRI_FIN, L"Points triés a la &fin");
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_TRI_ALTITUDE, L"Points triés selon &altitude");
    AppendMenu(hMenuOutils, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_CONTROLE_PRECISION, L"Contrô&le de précision (Arrêté 16.09.2003)");
    AppendMenu(hMenuOutils, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_CHANGEMENT_SYSTEME, L"Chan&gement de système");
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_REDUCTION_REPRISES, L"Ré&duction des reprises");
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_CHANGEMENT_REPERE, L"Cha&ngement de repère");
    AppendMenu(hMenuOutils, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuOutils, MF_STRING, ID_OUTILS_OPTIONS, L"Options\tCtrl+Shift+O");

    AppendMenu(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuOutils), L"&Outils");
    m_hMenuOutils = GetSubMenu(m_hMenu, 6);
}



// --- Menu Codification -----------------------------------------------------

void GeoBaseDialog::CreerMenuCodification() {
    HMENU hMenuCodification = CreatePopupMenu();
    AppendMenu(hMenuCodification, MF_STRING, ID_CODIFICATION_FUSION_CODIF, L"&Fusion d'une codification");
    AppendMenu(hMenuCodification, MF_STRING, ID_CODIFICATION_TABLE_GEOCODIF, L"&Table de géocodification");
    AppendMenu(hMenuCodification, MF_STRING, ID_CODIFICATION_GENERER_DESSIN, L"&Génération du dessin");
    AppendMenu(hMenuCodification, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenuCodification, MF_STRING, ID_CODIFICATION_LISTE_PTS_LEICA, L"Écriture liste de codes &Leica");
    AppendMenu(hMenuCodification, MF_STRING, ID_CODIFICATION_LISTE_PTS_TRIMBLE, L"Écriture liste de codes Trim&ble");

    AppendMenu(m_hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuCodification), L"Co&dification");
    m_hMenuCodification = GetSubMenu(m_hMenu, 7);
}


// --- Menu contextuel --------------------------------------------------

namespace {
    bool TousDuType(const std::vector<GeoRecord>& recs, const std::vector<int>& sel, GeoType type) {
        if (sel.empty()) return false;
        for (int idx : sel) {
            if (idx < 0 || idx >= static_cast<int>(recs.size())) return false;
            if (recs[idx].type != type) return false;
        }
        return true;
    }
    bool TousCheminements(const std::vector<GeoRecord>& recs, const std::vector<int>& sel) {
        if (sel.empty()) return false;
        for (int idx : sel) {
            if (idx < 0 || idx >= static_cast<int>(recs.size())) return false;
            GeoType t = recs[idx].type;
            if (t != GeoType::CheminAntenne && t != GeoType::CheminEncadre) return false;
        }
        return true;
    }
}

void GeoBaseDialog::AfficherMenuContextuel(POINT ptEcran) {
    POINT ptClient = ptEcran;
    if (ptEcran.x == -1 && ptEcran.y == -1) {
        // Ouvert au clavier (Shift+F10 / touche Menu) : position par defaut.
        GetCursorPos(&ptEcran);
        ptClient = ptEcran;
    }
    ScreenToClient(m_hwndListe, &ptClient);

    LVHITTESTINFO lvhti{};
    lvhti.pt = ptClient;
    int idx = ListView_HitTest(m_hwndListe, &lvhti);
    bool surLigne = (idx >= 0) && (lvhti.flags & LVHT_ONITEM);

    if (surLigne) {
        bool dejaSelectionne = (ListView_GetItemState(m_hwndListe, idx, LVIS_SELECTED) & LVIS_SELECTED) != 0;
        if (!dejaSelectionne) {
            // Clic droit sur une ligne non selectionnee : on la selectionne seule.
            int i = -1;
            while ((i = ListView_GetNextItem(m_hwndListe, i, LVNI_SELECTED)) != -1) {
                ListView_SetItemState(m_hwndListe, i, 0, LVIS_SELECTED);
            }
            ListView_SetItemState(m_hwndListe, idx, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        }
    }

    std::vector<int> sel = IndicesSelectionnes();
    bool unSeul = (sel.size() == 1);

    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING | (surLigne ? 0 : MF_GRAYED), ID_EDITION_MODIFIER, L"&Modifier...");
    AppendMenu(hMenu, MF_STRING | (surLigne ? 0 : MF_GRAYED), ID_EDITION_SUPPRIMER, L"&Supprimer");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);

    if (TousDuType(m_enregistrements, sel, GeoType::Mesure))
        AppendMenu(hMenu, MF_STRING, ID_CTX_VERS_REFERENCE, L"--> &Réference");
    if (TousDuType(m_enregistrements, sel, GeoType::Reference))
        AppendMenu(hMenu, MF_STRING, ID_CTX_VERS_MESURE, L"--> &Mesure");
    
    bool tousCercleDroit = !sel.empty();
    for (int i : sel) {
        if (i < 0 || i >= static_cast<int>(m_enregistrements.size())) { tousCercleDroit = false; break; }
        const auto& rec = m_enregistrements[i];
        if (!rec.AV || *rec.AV <= 200.0) { tousCercleDroit = false; break; }
    }
    if (TousDuType(m_enregistrements, sel, GeoType::Mesure) && tousCercleDroit)
        AppendMenu(hMenu, MF_STRING, ID_CTX_CERCLE_GAUCHE, L"--> &Cercle gauche");

    bool tousNivLecture = !sel.empty();
    for (int i : sel) {
        if (i < 0 || i >= static_cast<int>(m_enregistrements.size())) { tousNivLecture = false; break; }
        GeoType t = m_enregistrements[i].type;
        if (t != GeoType::NivLectAR && t != GeoType::NivLectAV) { tousNivLecture = false; break; }
    }
    if (tousNivLecture) {
        AppendMenu(hMenu, MF_STRING, ID_CTX_VERS_LECT_AV, L"--> Lecture &AV");
        AppendMenu(hMenu, MF_STRING, ID_CTX_VERS_LECT_AR, L"--> Lecture A&R");
    }

    if (TousCheminements(m_enregistrements, sel))
        AppendMenu(hMenu, MF_STRING, ID_CTX_INVERSER_CHEMIN, L"--> &Invers. Chemin.");

    AppendMenu(hMenu, MF_STRING | (sel.empty() ? MF_GRAYED : 0), ID_CTX_VERS_COMMENTAIRE, L"--> C&ommentaire");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);

    AppendMenu(hMenu, MF_STRING, ID_EDITION_AJOUTER, L"A&jouter...");
    AppendMenu(hMenu, MF_STRING | (unSeul ? 0 : MF_GRAYED), ID_EDITION_INSERER_AVANT, L"Insérer a&vant...");
    AppendMenu(hMenu, MF_STRING | (unSeul ? 0 : MF_GRAYED), ID_EDITION_INSERER_APRES, L"Insérer a&près...");
    AppendMenu(hMenu, MF_STRING, ID_CTX_RECHERCHER_MAT, L"&Rechercher Mat...");
    AppendMenu(hMenu, MF_STRING, ID_CTX_RECHERCHER_SYM, L"Rechercher &Sym...");

    TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, ptEcran.x, ptEcran.y, 0, m_hwnd, nullptr);
    DestroyMenu(hMenu);
}

void GeoBaseDialog::ActionVersReference() {
    std::vector<int> sel = IndicesSelectionnes();
    if (!TousDuType(m_enregistrements, sel, GeoType::Mesure)) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) m_enregistrements[idx].type = GeoType::Reference;
    m_modifie = true;
    RafraichirListe();
}

void GeoBaseDialog::ActionVersMesure() {
    std::vector<int> sel = IndicesSelectionnes();
    if (!TousDuType(m_enregistrements, sel, GeoType::Reference)) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) m_enregistrements[idx].type = GeoType::Mesure;
    m_modifie = true;
    RafraichirListe();
}

void GeoBaseDialog::ActionCercleGauche() {
    std::vector<int> sel = IndicesSelectionnes();
    if (!TousDuType(m_enregistrements, sel, GeoType::Mesure)) return;
    SauvegarderEtatPourAnnulation();

    // Traite par indices decroissants pour que les insertions ne decalent
    // pas les indices restants a traiter.
    std::sort(sel.rbegin(), sel.rend());
    for (int idx : sel) {
        GeoRecord& orig = m_enregistrements[idx];
        if (!orig.AV || *orig.AV <= 200.0) continue; // seulement si cercle droit (AV > 200)

        GeoRecord nouvelle = orig;
        if (nouvelle.AH) {
            double ah = *nouvelle.AH + 200.0;
            if (ah >= 400.0) ah -= 400.0;
            nouvelle.AH = ah;
        }
        nouvelle.AV = 400.0 - *orig.AV;

        orig.estCommentaire = true;
        m_enregistrements.insert(m_enregistrements.begin() + idx + 1, nouvelle);
    }

    m_modifie = true;
    RenumeroterLignes();
    RafraichirListe();
}

void GeoBaseDialog::ActionVersLectureAV() {
    std::vector<int> sel = IndicesSelectionnes();
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) {
        if (m_enregistrements[idx].type == GeoType::NivLectAR)
            m_enregistrements[idx].type = GeoType::NivLectAV;
    }
    m_modifie = true;
    RafraichirListe();
}

void GeoBaseDialog::ActionVersLectureAR() {
    std::vector<int> sel = IndicesSelectionnes();
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) {
        if (m_enregistrements[idx].type == GeoType::NivLectAV)
            m_enregistrements[idx].type = GeoType::NivLectAR;
    }
    m_modifie = true;
    RafraichirListe();
}

void GeoBaseDialog::ActionInverserChemin() {
    std::vector<int> sel = IndicesSelectionnes();
    if (!TousCheminements(m_enregistrements, sel)) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) {
        auto& stations = m_enregistrements[idx].stationsChemin;
        std::reverse(stations.begin(), stations.end());
    }
    m_modifie = true;
    RafraichirListe();
}

void GeoBaseDialog::ActionVersCommentaire() {
    std::vector<int> sel = IndicesSelectionnes();
    if (sel.empty()) return;
    SauvegarderEtatPourAnnulation();
    for (int idx : sel) m_enregistrements[idx].estCommentaire = !m_enregistrements[idx].estCommentaire;
    m_modifie = true;
    RafraichirListe();
}



// ---------------------------------------------------------------------------
// Petite boite de saisie reutilisable (DLGTEMPLATE construit en memoire)
// ---------------------------------------------------------------------------
namespace {

struct SaisieCtx {
    const wchar_t* titre;
    const wchar_t* invite;
    std::wstring   valeur;
};

INT_PTR CALLBACK SaisieProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_INITDIALOG: {
        auto* c = reinterpret_cast<SaisieCtx*>(l);
        SetWindowLongPtr(h, GWLP_USERDATA, l);
        SetWindowText(h, c->titre);
        SetDlgItemText(h, 101, c->invite);
        SetFocus(GetDlgItem(h, 102));
        return FALSE; // on a gere le focus nous-memes
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDOK) {
            auto* c = reinterpret_cast<SaisieCtx*>(GetWindowLongPtr(h, GWLP_USERDATA));
            wchar_t buf[256] = L"";
            GetDlgItemText(h, 102, buf, 256);
            c->valeur = buf;
            EndDialog(h, IDOK);
            return TRUE;
        }
        if (LOWORD(w) == IDCANCEL) { EndDialog(h, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}

void Aligner(std::vector<BYTE>& b) { while (b.size() % 4) b.push_back(0); }
template <class T> void Pousser(std::vector<BYTE>& b, const T& v) {
    auto p = reinterpret_cast<const BYTE*>(&v);
    b.insert(b.end(), p, p + sizeof(T));
}
void PousserTexte(std::vector<BYTE>& b, const wchar_t* s) {
    auto p = reinterpret_cast<const BYTE*>(s);
    b.insert(b.end(), p, p + (wcslen(s) + 1) * sizeof(wchar_t));
}
void AjouterItem(std::vector<BYTE>& b, DWORD style, short x, short y, short cx, short cy,
                 WORD id, WORD atomeClasse, const wchar_t* texte) {
    Aligner(b);
    DLGITEMTEMPLATE it{};
    it.style = style | WS_CHILD | WS_VISIBLE;
    it.x = x; it.y = y; it.cx = cx; it.cy = cy; it.id = id;
    Pousser(b, it);
    Pousser<WORD>(b, 0xFFFF);
    Pousser<WORD>(b, atomeClasse);   // 0x0080 button, 0x0081 edit, 0x0082 static
    PousserTexte(b, texte);
    Pousser<WORD>(b, 0);             // pas de donnees de creation
}

// Retourne true si l'utilisateur valide (OK) ; la saisie est dans 'valeur'.
bool Saisir(HWND parent, const wchar_t* titre, const wchar_t* invite, std::wstring& valeur) {
    std::vector<BYTE> b;
    DLGTEMPLATE t{};
    t.style = DS_MODALFRAME | DS_SETFONT | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    t.cdit = 4; t.x = 0; t.y = 0; t.cx = 214; t.cy = 62;
    Pousser(b, t);
    Pousser<WORD>(b, 0);              // pas de menu
    Pousser<WORD>(b, 0);              // classe par defaut
    PousserTexte(b, L"");             // titre (fixe dans WM_INITDIALOG)
    Pousser<WORD>(b, 9);              // taille de police
    PousserTexte(b, L"Segoe UI");

    AjouterItem(b, SS_LEFT,                                    7,  7, 200, 10, 101, 0x0082, L"");
    AjouterItem(b, WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,    7, 20, 200, 12, 102, 0x0081, L"");
    AjouterItem(b, WS_TABSTOP | BS_DEFPUSHBUTTON,             95, 40,  50, 14, IDOK,     0x0080, L"OK");
    AjouterItem(b, WS_TABSTOP | BS_PUSHBUTTON,               157, 40,  50, 14, IDCANCEL, 0x0080, L"Annuler");

    SaisieCtx ctx{ titre, invite, valeur };
    INT_PTR r = DialogBoxIndirectParam(GetModuleHandle(nullptr),
                                       reinterpret_cast<LPCDLGTEMPLATE>(b.data()),
                                       parent, SaisieProc, reinterpret_cast<LPARAM>(&ctx));
    if (r != IDOK) return false;
    valeur = ctx.valeur;
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Recherche (menu contextuel)
// ---------------------------------------------------------------------------
void GeoBaseDialog::ActionRechercherMatricule() {
    std::wstring v;
    if (!Saisir(m_hwnd, L"Rechercher Mat", L"Matricule :", v) || v.empty()) return;
    if (!RechercherEtSelectionner(false, v))
        MessageBox(m_hwnd, L"Aucun matricule correspondant.", L"Konyx - GéoBase", MB_ICONINFORMATION);
}

void GeoBaseDialog::ActionRechercherSymbole() {
    std::wstring v;
    if (!Saisir(m_hwnd, L"Rechercher Sym", L"Symbole :", v) || v.empty()) return;
    if (!RechercherEtSelectionner(true, v))
        MessageBox(m_hwnd, L"Aucun symbole correspondant.", L"Konyx - GéoBase", MB_ICONINFORMATION);
}

// Cherche a partir de la ligne suivant la selection courante (puis reboucle).
// Compare le texte affiche dans la colonne Matricule (2) ou Symbole (4),
// sans tenir compte de la casse.
bool GeoBaseDialog::RechercherEtSelectionner(bool parSymbole, const std::wstring& valeur) {
    const int nb = ListView_GetItemCount(m_hwndListe);
    if (nb == 0) return false;

    const int colonne = parSymbole ? 4 : 2;
    const int debut = ListView_GetNextItem(m_hwndListe, -1, LVNI_SELECTED);

    for (int k = 1; k <= nb; ++k) {
        const int i = ((debut < 0 ? -1 : debut) + k) % nb;

        wchar_t texte[256] = L"";
        ListView_GetItemText(m_hwndListe, i, colonne, texte, 256);
        if (_wcsicmp(texte, valeur.c_str()) != 0) continue;

        int s = -1;
        while ((s = ListView_GetNextItem(m_hwndListe, s, LVNI_SELECTED)) != -1)
            ListView_SetItemState(m_hwndListe, s, 0, LVIS_SELECTED);

        ListView_SetItemState(m_hwndListe, i, LVIS_SELECTED | LVIS_FOCUSED,
                                              LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(m_hwndListe, i, FALSE);
        SetFocus(m_hwndListe);
        return true;
    }
    return false;
}

} // namespace Konyx
