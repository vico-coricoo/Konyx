#include "GeoEditDialog.h"
#include <windowsx.h>
#include <commctrl.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "comctl32.lib")

namespace Konyx {

constexpr wchar_t GeoEditDialog::ClasseFenetre[];

namespace {
    GeoEditDialog* InstanceDepuisHwnd(HWND hwnd) {
        return reinterpret_cast<GeoEditDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    std::wstring TexteDe(HWND h) {
        wchar_t buf[256]{};
        GetWindowText(h, buf, 255);
        return buf;
    }

    void DefinirTexteNombre(HWND h, const std::optional<double>& v, int decimales = 3) {
        if (!h) return;
        if (!v) { SetWindowText(h, L""); return; }
        std::wostringstream os;
        os << std::fixed << std::setprecision(decimales) << *v;
        SetWindowText(h, os.str().c_str());
    }

    // Retourne false si le texte n'est pas vide et n'est pas un nombre valide.
    bool LireNombreOptionnel(HWND h, std::optional<double>& out) {
        if (!h) { out.reset(); return true; }
        std::wstring s = TexteDe(h);
        size_t deb = s.find_first_not_of(L" \t");
        if (deb == std::wstring::npos) { out.reset(); return true; }
        size_t fin = s.find_last_not_of(L" \t");
        s = s.substr(deb, fin - deb + 1);
        if (s.empty()) { out.reset(); return true; }
        wchar_t* finPtr = nullptr;
        double v = wcstod(s.c_str(), &finPtr);
        if (finPtr == s.c_str() || *finPtr != L'\0') return false;
        out = v;
        return true;
    }
}

GeoEditDialog::GeoEditDialog(const GeoRecord& rec, int nbLignesTotal, std::optional<GeoType> typeSuivant)
    : m_rec(rec), m_nbLignesTotal(nbLignesTotal), m_typeSuivant(typeSuivant) {

    switch (rec.type) {
    case GeoType::Station:
    case GeoType::Reference:
    case GeoType::Mesure:
    case GeoType::Point:
    case GeoType::NivLectAR:
    case GeoType::NivLectAV:
    case GeoType::Commentaire:
        m_estTypeGere = true;
        break;
    default:
        m_estTypeGere = false;
        break;
    }

    m_hFont = CreateFont(
        -22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    m_hBrushFond = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
}

GeoEditDialog::~GeoEditDialog() {
    if (m_hFont) DeleteObject(m_hFont);
    if (m_hBrushFond) DeleteObject(m_hBrushFond);
}

bool GeoEditDialog::Modifier(HWND hwndParent, GeoRecord& rec, int nbLignesTotal, std::optional<GeoType> typeSuivant, bool& allerSuivant, bool creation) {
    GeoEditDialog dlg(rec, nbLignesTotal, typeSuivant);
    dlg.m_creation = creation;
    allerSuivant = false;

    if (!dlg.m_estTypeGere) {
        MessageBox(hwndParent,
            L"L'édition de ce type d'élément n'est pas encore prise en charge.",
            L"Konyx - GéoBase", MB_ICONINFORMATION);
        return false;
    }

    WNDCLASS wc{};
    wc.lpfnWndProc = FenetreProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = ClasseFenetre;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClass(&wc); // echoue silencieusement si deja enregistree, sans gravite

    const wchar_t* titre = creation ? L"Création d'un élément" : L"Modifier l'element";
    switch (rec.type) {
    case GeoType::Station:       titre = creation ? L"Création d'une station" : L"Modification d'une station";          break;
    case GeoType::Reference:     titre = creation ? L"Création d'une référence" : L"Modification d'une référence";        break;
    case GeoType::Mesure:        titre = creation ? L"Création d'une mesure" : L"Modification d'une mesure";           break;
    case GeoType::Point:         titre = creation ? L"Création d'un point" : L"Modification d'un point";            break;
    case GeoType::NivLectAR:     titre = creation ? L"Création d'une lecture AR" : L"Modification d'une lecture AR";       break;
    case GeoType::NivLectAV:     titre = creation ? L"Création d'une lecture AV" : L"Modification d'une lecture AV";       break;
    case GeoType::Commentaire:   titre = creation ? L"Création d'un commentaire" : L"Modification d'un commentaire";      break;
    default: break;
    }

    //if (hwndParent) EnableWindow(hwndParent, FALSE);

    HWND hwnd = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        ClasseFenetre, titre,
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 440, 420,
        hwndParent, nullptr, GetModuleHandle(nullptr), &dlg);

    if (hwndParent) {
        RECT rcP, rcF;
        GetWindowRect(hwndParent, &rcP);
        GetWindowRect(hwnd, &rcF);
        int w = rcF.right - rcF.left, h = rcF.bottom - rcF.top;
        int x = rcP.left + ((rcP.right - rcP.left) - w) / 2;
        int y = rcP.top + ((rcP.bottom - rcP.top) - h) / 2;
        SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (IsWindow(hwnd) && GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BTN_ANNULER, BN_CLICKED), 0);
            continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            // Ne pas valider depuis le champ commentaire multiligne (Entree = saut de ligne).
            int idFocus = GetDlgCtrlID(GetFocus());
            if (idFocus != ID_COMMENTAIRE_TEXTE) {
                SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BTN_OK, BN_CLICKED), 0);
                continue;
            }
        }
        if (!IsDialogMessage(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (hwndParent) {
        SetForegroundWindow(hwndParent);
        EnableWindow(hwndParent, TRUE);
    }

    if (dlg.m_valide) {
        rec = dlg.m_rec;
        allerSuivant = dlg.m_allerSuivant;
        return true;
    }
    return false;
}

LRESULT CALLBACK GeoEditDialog::FenetreProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        auto* pThis = reinterpret_cast<GeoEditDialog*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    }
    GeoEditDialog* pThis = InstanceDepuisHwnd(hwnd);
    if (pThis) return pThis->GererMessage(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GeoEditDialog::GererMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreerControles(hwnd);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(m_hBrushFond);
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkColor(hdc, RGB(255, 255, 255));
        return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_CHAMP2_CASE && HIWORD(wParam) == BN_CLICKED) {
            bool coche = Button_GetCheck(m_hChamp2Case) == BST_CHECKED;
            ShowWindow(m_hChamp2, coche ? SW_SHOW : SW_HIDE);
            return 0;
        }
        if (id == ID_BTN_OK) {
            if (LireVersRecord()) {
                m_valide = true;
                DestroyWindow(hwnd);
            }
            return 0;
        }
        if (id == ID_BTN_ANNULER) {
            m_valide = false;
            DestroyWindow(hwnd);
            return 0;
        }
        if (id == ID_CASE_XY_ACTIF && HIWORD(wParam) == BN_CLICKED) {
            bool coche = Button_GetCheck(m_hCaseXYActif) == BST_CHECKED;
            ShowWindow(m_hChamp2, coche ? SW_SHOW : SW_HIDE);
            ShowWindow(m_hChamp3, coche ? SW_SHOW : SW_HIDE);
            ShowWindow(m_hCaseXYFixe, coche ? SW_SHOW : SW_HIDE);
            if (!coche) Button_SetCheck(m_hCaseXYFixe, BST_UNCHECKED);
            return 0;
        }
        if (id == ID_CASE_Z_ACTIF && HIWORD(wParam) == BN_CLICKED) {
            bool coche = Button_GetCheck(m_hCaseZActif) == BST_CHECKED;
            ShowWindow(m_hChamp4, coche ? SW_SHOW : SW_HIDE);
            ShowWindow(m_hCaseZFixe, coche ? SW_SHOW : SW_HIDE);
            if (!coche) Button_SetCheck(m_hCaseZFixe, BST_UNCHECKED);
            return 0;
        }
        if (id == ID_BTN_SUIVANT) {
            if (LireVersRecord()) {
                m_valide = true;
                m_allerSuivant = true;
                DestroyWindow(hwnd);
            }
            return 0;
        }
        break;
    }

    case WM_CLOSE: {
        m_valide = false;
        DestroyWindow(hwnd);
        return 0;
    }

    case WM_DESTROY: {
        // IMPORTANT : ne pas appeler PostQuitMessage ici. Cette fenetre
        // tourne sur le meme thread que la fenetre GeoBase (et sa propre
        // boucle de message locale) : poster un WM_QUIT ici le ferait
        // remonter et stopper prematurement la boucle de GeoBaseDialog,
        // qui arreterait alors de traiter WM_NOTIFY/NM_CUSTOMDRAW (perte de
        // la mise en forme / plantages). La boucle locale de cette boite
        // s'arrete toute seule via le test IsWindow(hwnd).
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void GeoEditDialog::CreerControles(HWND hwnd) {
    HINSTANCE hInst = GetModuleHandle(nullptr);
    static constexpr int xLabel = 30, wLabel = 230;
    static constexpr int xChamp = 250, wChamp = 180;
    static constexpr int hLigne = 35, pasY = 55;
    int y = 20;

    auto appliquerPolice = [&](HWND h) {
        if (h && m_hFont) SendMessage(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);
    };

    auto label = [&](const wchar_t* texte, int yy, int xx = xLabel) {
        HWND h = CreateWindowEx(0, L"STATIC", texte, WS_CHILD | WS_VISIBLE,
            xx, yy + 3, wLabel, hLigne, hwnd, nullptr, hInst, nullptr);
        appliquerPolice(h);
    };
    auto champTexte = [&](int id, int yy, int largeur) -> HWND {
        HWND h = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            xChamp, yy, largeur, hLigne, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInst, nullptr);
        appliquerPolice(h);
        return h;
    };
    auto caseACocher = [&](int id, const wchar_t* texte, int xx, int yy, int largeur) -> HWND {
        HWND h = CreateWindowEx(0, L"BUTTON", texte,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
            xx, yy + 2, largeur, hLigne, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInst, nullptr);
        appliquerPolice(h);
        return h;
    };

    // --- Ligne (commun) ---
    label(L"Numéro de ligne :", y);
    m_hLigne = champTexte(ID_LIGNE, y, wChamp);
    SetWindowText(m_hLigne, std::to_wstring(m_rec.numLigne).c_str());
    y += pasY;

    if (m_rec.type == GeoType::Commentaire) {
        label(L"Texte :", y);
        y += pasY;
        m_hCommentaireTexte = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_MULTILINE | WS_VSCROLL,
            xLabel, y, 380, 180, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_COMMENTAIRE_TEXTE)), hInst, nullptr);
        appliquerPolice(m_hCommentaireTexte);
        SetWindowText(m_hCommentaireTexte, std::wstring(m_rec.texteCommentaire.begin(), m_rec.texteCommentaire.end()).c_str());
        y += 190;
    } else {
        // --- Matricule (commun) ---
        label(L"Matricule du point :", y);
        m_hMatricule = champTexte(ID_MATRICULE, y, wChamp);
        SetWindowText(m_hMatricule, std::wstring(m_rec.matricule.begin(), m_rec.matricule.end()).c_str());
        y += pasY;

        switch (m_rec.type) {
        case GeoType::Station: {
            label(L"Hauteur d'instrument :", y);
            m_hChamp1 = champTexte(ID_CHAMP1, y, wChamp);
            DefinirTexteNombre(m_hChamp1, m_rec.HI);
            y += pasY;

            m_hChamp2Case = caseACocher(ID_CHAMP2_CASE, L"V0 de la station :", xLabel, y, 200);
            m_hChamp2 = champTexte(ID_CHAMP2, y, wChamp);
            std::optional<double> v0Affiche = (m_rec.V0 && *m_rec.V0 != -1.0) ? m_rec.V0 : std::nullopt;
            DefinirTexteNombre(m_hChamp2, v0Affiche, 4);
            y += pasY;

            bool v0Actif = m_rec.V0.has_value() && *m_rec.V0 != -1.0;
            Button_SetCheck(m_hChamp2Case, v0Actif ? BST_CHECKED : BST_UNCHECKED);
            ShowWindow(m_hChamp2, v0Actif ? SW_SHOW : SW_HIDE);
            break;
        }

        case GeoType::Reference:
        case GeoType::Mesure: {
            label(L"Hauteur de prisme :", y);
            m_hChamp1 = champTexte(ID_CHAMP1, y, wChamp);
            DefinirTexteNombre(m_hChamp1, m_rec.HP);
            y += pasY;

            label(L"Angle horizontal :", y);
            m_hChamp2 = champTexte(ID_CHAMP2, y, wChamp);
            DefinirTexteNombre(m_hChamp2, m_rec.AH, 4);
            y += pasY;

            label(L"Angle vertical :", y);
            m_hChamp3 = champTexte(ID_CHAMP3, y, wChamp);
            DefinirTexteNombre(m_hChamp3, m_rec.AV, 4);
            y += pasY;

            label(L"Distance inclinée :", y);
            m_hChamp4 = champTexte(ID_CHAMP4, y, wChamp);
            DefinirTexteNombre(m_hChamp4, m_rec.DI);
            y += pasY;
            break;
        }

        case GeoType::Point: {
            m_hCaseXYActif = caseACocher(ID_CASE_XY_ACTIF, L"E = abscisse :", xLabel, y, 200);
            m_hChamp2 = champTexte(ID_CHAMP2, y, wChamp);
            DefinirTexteNombre(m_hChamp2, m_rec.X);
            m_hCaseXYFixe = caseACocher(ID_CASE_XY_FIXE, L"Fixés", xChamp + wChamp + 20, y, 80);
            y += pasY;

            label(L"N = ordonnée :", y, xLabel + 27);
            m_hChamp3 = champTexte(ID_CHAMP3, y, wChamp);
            DefinirTexteNombre(m_hChamp3, m_rec.Y);
            y += pasY;

            bool xyActif = (m_rec.X || m_rec.Y);
            Button_SetCheck(m_hCaseXYActif, xyActif ? BST_CHECKED : BST_UNCHECKED);
            ShowWindow(m_hChamp2, xyActif ? SW_SHOW : SW_HIDE);
            ShowWindow(m_hChamp3, xyActif ? SW_SHOW : SW_HIDE);
            ShowWindow(m_hCaseXYFixe, xyActif ? SW_SHOW : SW_HIDE);

            m_hCaseZActif = caseACocher(ID_CASE_Z_ACTIF, L"H = altitude :", xLabel, y, 200);
            m_hChamp4 = champTexte(ID_CHAMP4, y, wChamp);
            DefinirTexteNombre(m_hChamp4, m_rec.Z);
            m_hCaseZFixe = caseACocher(ID_CASE_Z_FIXE, L"Fixé", xChamp + wChamp + 20, y, 80);
            y += pasY;

            bool zActif = m_rec.Z.has_value();
            Button_SetCheck(m_hCaseZActif, zActif ? BST_CHECKED : BST_UNCHECKED);
            ShowWindow(m_hChamp4, zActif ? SW_SHOW : SW_HIDE);
            ShowWindow(m_hCaseZFixe, zActif ? SW_SHOW : SW_HIDE);

            int cp = m_rec.CP ? *m_rec.CP : 0;
            if (cp & 0x1) Button_SetCheck(m_hCaseXYFixe, BST_CHECKED);
            if (cp & 0x2) Button_SetCheck(m_hCaseZFixe, BST_CHECKED);
            break;
        }

        case GeoType::NivLectAR:
        case GeoType::NivLectAV: {
            label(L"Lecture sur la mire", y);
            m_hChamp1 = champTexte(ID_CHAMP1, y, wChamp);
            DefinirTexteNombre(m_hChamp1, m_rec.LM);
            y += pasY;

            m_hChamp2Case = caseACocher(ID_CHAMP2_CASE, L"Distance horiz.", xLabel, y, 200);
            m_hChamp2 = champTexte(ID_CHAMP2, y, wChamp);
            DefinirTexteNombre(m_hChamp2, m_rec.DH);
            y += pasY;

            bool dhActif = m_rec.DH.has_value();
            Button_SetCheck(m_hChamp2Case, dhActif ? BST_CHECKED : BST_UNCHECKED);
            ShowWindow(m_hChamp2, dhActif ? SW_SHOW : SW_HIDE);
            break;
        }

        default:
            break;
        }

        // --- Symbole (commun, sauf Station / Cheminement / Photo / Appareil) ---
        if (m_rec.type != GeoType::Station) {
            label(L"Symbole :", y);
            m_hSymbole = champTexte(ID_SYMBOLE, y, wChamp);
            if (m_rec.SY) SetWindowText(m_hSymbole, std::to_wstring(*m_rec.SY).c_str());
            y += pasY;
        }
    }

    // --- Boutons OK / Annuler ---
    y += 10;
    HWND hBtnOk = CreateWindowEx(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        330, y, 100, 40, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_OK)), hInst, nullptr);
    HWND hBtnAnnuler = CreateWindowEx(0, L"BUTTON", L"Annuler", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        440, y, 100, 40, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_ANNULER)), hInst, nullptr);
    appliquerPolice(hBtnOk);
    appliquerPolice(hBtnAnnuler);

    // Afficher le bouton de lecture suivante pour AR et AV,
    // en création comme en modification.
    bool afficherSuivant =
        m_rec.type == GeoType::NivLectAR ||
        m_rec.type == GeoType::NivLectAV;

    if (afficherSuivant) {
        const wchar_t* texteBouton =
            (m_rec.type == GeoType::NivLectAR)
            ? L"--> Lect. AV"
            : L"--> Lect. AR";

        m_hBtnSuivant = CreateWindowEx(
            0, L"BUTTON", texteBouton,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            xLabel, y, 150, 40,
            hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_SUIVANT)),
            hInst, nullptr);

        appliquerPolice(m_hBtnSuivant);
    }

    RECT rc{ 0, 0, 550, y + 50 };
    AdjustWindowRectEx(&rc, static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_STYLE)), FALSE,
        static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_EXSTYLE)));
    SetWindowPos(hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
        SWP_NOMOVE | SWP_NOZORDER);
    // En mode création :
    // - Première lecture : focus sur le matricule.
    // - Lectures suivantes : focus sur la lecture sur mire.
    if (m_creation) {
        if (m_rec.matricule.empty()) {
            SetFocus(m_hMatricule);
        }
        else if (m_rec.type == GeoType::NivLectAR ||
                m_rec.type == GeoType::NivLectAV) {
            SetFocus(m_hChamp1);
        }
    }
}

bool GeoEditDialog::LireVersRecord() {
    std::wstring sLigne = TexteDe(m_hLigne);
    int ligneCible = _wtoi(sLigne.c_str());
    if (ligneCible < 1 || ligneCible > m_nbLignesTotal) {
        MessageBox(m_hwnd, L"Le numéro de ligne doit etre compris entre 1 et le nombre total de lignes.",
            L"Konyx - Saisie invalide", MB_ICONWARNING);
        return false;
    }
    m_rec.numLigne = ligneCible;

    if (m_rec.type == GeoType::Commentaire) {
        int len = GetWindowTextLength(m_hCommentaireTexte);
        std::vector<wchar_t> buf(len + 1);
        GetWindowText(m_hCommentaireTexte, buf.data(), len + 1);
        std::wstring texte(buf.data());
        m_rec.texteCommentaire.assign(texte.begin(), texte.end());
        return true;
    }

    std::wstring sMatricule = TexteDe(m_hMatricule);
    m_rec.matricule.assign(sMatricule.begin(), sMatricule.end());

    auto erreurNombre = [&](const wchar_t* nomChamp) {
        std::wstring msg = std::wstring(L"Valeur numérique invalide pour : ") + nomChamp;
        MessageBox(m_hwnd, msg.c_str(), L"Konyx - Saisie invalide", MB_ICONWARNING);
    };

    switch (m_rec.type) {
    case GeoType::Station: {
        if (!LireNombreOptionnel(m_hChamp1, m_rec.HI)) { erreurNombre(L"HI"); return false; }
        if (Button_GetCheck(m_hChamp2Case) == BST_CHECKED) {
            if (!LireNombreOptionnel(m_hChamp2, m_rec.V0) || !m_rec.V0) { erreurNombre(L"V0"); return false; }
        } else {
            m_rec.V0.reset();
        }
        break;
    }

    case GeoType::Reference:
    case GeoType::Mesure: {
        if (!LireNombreOptionnel(m_hChamp1, m_rec.HP)) { erreurNombre(L"HP"); return false; }
        if (!LireNombreOptionnel(m_hChamp2, m_rec.AH)) { erreurNombre(L"AH"); return false; }
        if (!LireNombreOptionnel(m_hChamp3, m_rec.AV)) { erreurNombre(L"AV"); return false; }
        if (!LireNombreOptionnel(m_hChamp4, m_rec.DI)) { erreurNombre(L"DI"); return false; }
        break;
    }

    case GeoType::Point: {
        bool xyActif = Button_GetCheck(m_hCaseXYActif) == BST_CHECKED;
        bool zActif = Button_GetCheck(m_hCaseZActif) == BST_CHECKED;

        if (xyActif) {
            if (!LireNombreOptionnel(m_hChamp2, m_rec.X)) { erreurNombre(L"X"); return false; }
            if (!LireNombreOptionnel(m_hChamp3, m_rec.Y)) { erreurNombre(L"Y"); return false; }
        } else {
            m_rec.X.reset();
            m_rec.Y.reset();
        }

        if (zActif) {
            if (!LireNombreOptionnel(m_hChamp4, m_rec.Z)) { erreurNombre(L"Z"); return false; }
        } else {
            m_rec.Z.reset();
        }

        int cp = 0;
        if (xyActif && Button_GetCheck(m_hCaseXYFixe) == BST_CHECKED) cp |= 0x1;
        if (zActif && Button_GetCheck(m_hCaseZFixe) == BST_CHECKED) cp |= 0x2;
        m_rec.CP = cp;
        break;
    }

    case GeoType::NivLectAR:
    case GeoType::NivLectAV: {
        if (!LireNombreOptionnel(m_hChamp1, m_rec.LM)) {
            erreurNombre(L"LM");
            return false;
        }
        // Pour une lecture AR ou AV, le matricule et la lecture sur mire
        // sont tous les deux obligatoires.
        bool matriculeVide =
            sMatricule.find_first_not_of(L" \t\r\n") == std::wstring::npos;

        if (matriculeVide || !m_rec.LM) {
            MessageBox(
                m_hwnd,
                L"Le matricule et la lecture sur mire (LM) doivent être renseignés.",
                L"Konyx - Saisie invalide",
                MB_ICONWARNING
            );

            if (matriculeVide)
                SetFocus(m_hMatricule);
            else
                SetFocus(m_hChamp1);

            return false;
        }

        if (m_rec.LM && (*m_rec.LM < 0.0 || *m_rec.LM > 100.0)) {
            MessageBox(m_hwnd, L"La lecture sur mire (LM) doit être comprise entre 0 et 100.",
                L"Konyx - Saisie invalide", MB_ICONWARNING);
            return false;
        }
        if (Button_GetCheck(m_hChamp2Case) == BST_CHECKED) {
            if (!LireNombreOptionnel(m_hChamp2, m_rec.DH) || !m_rec.DH) { erreurNombre(L"DH"); return false; }
        } else {
            m_rec.DH.reset();
        }
        break;
    }

    default:
        break;
    }

    if (m_rec.type != GeoType::Station) {
        std::wstring sSymbole = TexteDe(m_hSymbole);
        if (sSymbole.empty()) {
            m_rec.SY.reset();
        } else {
            wchar_t* finPtr = nullptr;
            long v = wcstol(sSymbole.c_str(), &finPtr, 10);
            if (finPtr == sSymbole.c_str() || *finPtr != L'\0') { erreurNombre(L"Symbole (SY)"); return false; }
            m_rec.SY = static_cast<int>(v);
        }
    }

    return true;
}

} // namespace Konyx
