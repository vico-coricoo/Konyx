#include "GeoAjouterTypeDialog.h"

namespace Konyx {

constexpr wchar_t GeoAjouterTypeDialog::ClasseFenetre[];

namespace {

struct EntreeType {
    const wchar_t* libelle;
    GeoType type;
};

// Memes libelles / ordre que la boite "Ajouter" de Covadis. Geocodification
// n'a pas d'equivalent dans GeoType (pas encore gere par le fichier .geo) et
// Cheminement renvoie un type sentinelle : dans les deux cas l'appelant
// affiche un message "pas encore implemente" plutot que d'ouvrir une
// fenetre d'edition, exactement comme pour les boutons du ruban.
const EntreeType ENTREES[] = {
    { L"Station",                       GeoType::Station },
    { L"Référence",                     GeoType::Reference },
    { L"Mesure",                        GeoType::Mesure },
    { L"Point (coordonnées)",           GeoType::Point },
    { L"Lecture de nivellement AR",     GeoType::NivLectAR },
    { L"Lecture de nivellement AV",     GeoType::NivLectAV },
    { L"Commande de géocodification",   GeoType::Inconnu },
    { L"Commentaire",                   GeoType::Commentaire },
    { L"Cheminement",                   GeoType::CheminAntenne },
    { L"Appareil de terrain",           GeoType::Appareil },
    { L"Photo",                         GeoType::Photo },
    { L"Polyligne",                     GeoType::Polyligne },
};
constexpr int NB_ENTREES = sizeof(ENTREES) / sizeof(ENTREES[0]);

constexpr int ID_BTN_BASE = 1001;
constexpr int ID_BTN_ANNULER = 1099;

} // namespace

LRESULT CALLBACK GeoAjouterTypeDialog::FenetreProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GeoAjouterTypeDialog* pThis;
    if (msg == WM_NCCREATE) {
        auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<GeoAjouterTypeDialog*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<GeoAjouterTypeDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (pThis) return pThis->GererMessage(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GeoAjouterTypeDialog::GererMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_BTN_ANNULER || id == IDCANCEL) {
            m_valide = false;
            DestroyWindow(hwnd);
            return 0;
        }
        if (id >= ID_BTN_BASE && id < ID_BTN_BASE + NB_ENTREES) {
            m_resultat = ENTREES[id - ID_BTN_BASE].type;
            m_valide = true;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        m_valide = false;
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY: {
        HWND hwndProprio = GetWindow(hwnd, GW_OWNER);
        //if (hwndProprio) EnableWindow(hwndProprio, TRUE);
        m_hwnd = nullptr;
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


std::optional<GeoType> GeoAjouterTypeDialog::Choisir(HWND hwndParent) {
    GeoAjouterTypeDialog boite;

    WNDCLASS wc{};
    wc.lpfnWndProc = FenetreProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = ClasseFenetre;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClass(&wc);

    constexpr int LARGEUR = 500;
    constexpr int MARGE = 15;
    constexpr int HAUTEUR_BTN = 40;
    constexpr int ESPACE = 10;
    constexpr int HAUTEUR_SEPARATEUR = 30;

    int hauteurClient = MARGE
        + NB_ENTREES * HAUTEUR_BTN + (NB_ENTREES - 1) * ESPACE
        + HAUTEUR_SEPARATEUR
        + HAUTEUR_BTN
        + MARGE;

    RECT rcFenetre{ 0, 0, LARGEUR, hauteurClient };
    AdjustWindowRectEx(&rcFenetre, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE, 0);
    int largeurFenetre = rcFenetre.right - rcFenetre.left;
    int hauteurFenetre = rcFenetre.bottom - rcFenetre.top;

    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    if (hwndParent) {
        RECT rcParent;
        GetWindowRect(hwndParent, &rcParent);
        x = rcParent.left + (rcParent.right - rcParent.left - largeurFenetre) / 2;
        y = rcParent.top + (rcParent.bottom - rcParent.top - hauteurFenetre) / 2;
    }

    HINSTANCE hInst = GetModuleHandle(nullptr);
    HWND hwnd = CreateWindowEx(
        WS_EX_DLGMODALFRAME, ClasseFenetre, L"Sélectionnez le nouvel éléments à ajouter",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, largeurFenetre, hauteurFenetre,
        hwndParent, nullptr, hInst, &boite);
    if (!hwnd) return std::nullopt;

    HFONT hFont = CreateFont(
        -22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    // Un bouton par type d'element.
    for (int i = 0; i < NB_ENTREES; ++i) {
        int yBtn = MARGE + i * (HAUTEUR_BTN + ESPACE);
        HWND hBtn = CreateWindowEx(0, L"BUTTON", ENTREES[i].libelle,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            MARGE, yBtn, LARGEUR - 2 * MARGE, HAUTEUR_BTN,
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_BASE + i)),
            hInst, nullptr);
        SendMessage(hBtn, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }

    // Bouton Annuler, separe des autres.
    HWND hAnnuler = CreateWindowEx(0, L"BUTTON", L"Annuler",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        MARGE + 150, hauteurClient - MARGE - HAUTEUR_BTN, LARGEUR - 2 * MARGE - 300, HAUTEUR_BTN,
        hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_ANNULER)),
        hInst, nullptr);
    SendMessage(hAnnuler, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

    // Modale : on bloque la fenetre parente pendant l'affichage.
    //if (hwndParent) EnableWindow(hwndParent, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // Boucle de messages locale : se termine quand la fenetre est detruite
    // (WM_DESTROY remet m_hwnd a nullptr).
    MSG msg;
    while (boite.m_hwnd) {
        BOOL r = GetMessage(&msg, nullptr, 0, 0);
        if (r == 0) {                       // WM_QUIT : on le repropage
            PostQuitMessage(static_cast<int>(msg.wParam));
            break;
        }
        if (r == -1) break;
        if (!IsDialogMessage(boite.m_hwnd, &msg)) {   // Tab, fleches, Entree, Echap
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (boite.m_valide) return boite.m_resultat;
    return std::nullopt;
}

} // namespace Konyx