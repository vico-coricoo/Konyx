#include "GeoCalculV0Dialog.h"
#include <windowsx.h>
#include <commctrl.h>
#include <sstream>
#include <iomanip>
#include <cmath>

#pragma comment(lib, "comctl32.lib")

namespace Konyx {

static constexpr double PI_LOCAL = 3.141592653589793238462643383279502884;

static std::wstring VersWide(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
    if (!w.empty() && w.back() == L'\0') w.pop_back();
    return w;
}

static std::wstring FormatNombre(double v, int decimales) {
    std::wostringstream oss;
    oss << std::fixed << std::setprecision(decimales) << v;
    return oss.str();
}

// ---------------------------------------------------------------------
// Moyenne circulaire (grades) sur un sous-ensemble de contributions,
// identique au principe de GeoCalculV0::CalculerV0 mais applicable a une
// selection (cases cochees) plutot qu'a la totalite des visees valides.
static bool MoyenneCirculaire(const std::vector<ContributionV0>& contributions,
                               const std::vector<bool>& coches,
                               bool pondere,
                               double& resultat) {
    double sommeSin = 0.0, sommeCos = 0.0, sommeDist = 0.0;
    int nb = 0;

    for (size_t i = 0; i < contributions.size(); ++i) {
        const ContributionV0& c = contributions[i];
        if (!c.valide || !coches[i]) continue;

        double rad = c.v0 * (PI_LOCAL / 200.0);
        double poids = pondere ? c.distance : 1.0;
        sommeSin += std::sin(rad) * poids;
        sommeCos += std::cos(rad) * poids;
        sommeDist += c.distance;
        ++nb;
    }

    constexpr double EPSILON = 1e-9;
    if (nb == 0 || (std::abs(sommeSin) < EPSILON && std::abs(sommeCos) < EPSILON)) {
        return false;
    }

    double rad = std::atan2(sommeSin, sommeCos);
    double g = rad * (200.0 / PI_LOCAL);
    g = std::fmod(g, 400.0);
    if (g < 0.0) g += 400.0;
    resultat = g;
    return true;
}

// ---------------------------------------------------------------------

void GeoCalculV0Dialog::Afficher(HWND hwndParent, std::vector<GeoRecord>& enregistrements) {
    GeoCalculV0Dialog dlg(enregistrements);

    WNDCLASSEX wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = FenetreProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = ClasseFenetre;
    wc.hbrBackground = nullptr;
    RegisterClassEx(&wc); // echec silencieux si deja enregistree

    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icc);

    RECT rcParent{};
    GetWindowRect(hwndParent, &rcParent);
    int largeur = 660, hauteur = 620;
    int x = rcParent.left + (rcParent.right - rcParent.left - largeur) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - hauteur) / 2;

    HWND hwnd = CreateWindowEx(
        WS_EX_DLGMODALFRAME, ClasseFenetre, L"KONYX - Calcul des V0 de stations",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, largeur, hauteur,
        hwndParent, nullptr, GetModuleHandle(nullptr), &dlg);

    if (!hwnd) return;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    //EnableWindow(hwndParent, FALSE);

    MSG msg;
    while (IsWindow(hwnd) && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);
}

GeoCalculV0Dialog::GeoCalculV0Dialog(std::vector<GeoRecord>& enregistrements)
    : m_enregistrements(enregistrements) {
    m_hFont = CreateFont(
        -22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    m_hBrushFond = CreateSolidBrush(RGB(255, 255, 255));
    m_stations = ExtraireStationsEtVisees(m_enregistrements);
}

GeoCalculV0Dialog::~GeoCalculV0Dialog() {
    if (m_hFont) DeleteObject(m_hFont);
    if (m_hBrushFond) DeleteObject(m_hBrushFond);
}

LRESULT CALLBACK GeoCalculV0Dialog::FenetreProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GeoCalculV0Dialog* self = nullptr;
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        self = static_cast<GeoCalculV0Dialog*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<GeoCalculV0Dialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (self) return self->GererMessage(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GeoCalculV0Dialog::GererMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreerControles(hwnd);
        RemplirComboStations();
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkColor(hdc, RGB(255, 255, 255));
        return reinterpret_cast<LRESULT>(m_hBrushFond);
    }

    case WM_NOTIFY: {
        auto nmhdr = reinterpret_cast<NMHDR*>(lParam);
        if (nmhdr->idFrom == ID_LISTE_REF && nmhdr->code == LVN_ITEMCHANGED) {
            auto nmlv = reinterpret_cast<NMLISTVIEW*>(lParam);
            if (nmlv->uChanged & LVIF_STATE) {
                UINT ancien = (nmlv->uOldState & LVIS_STATEIMAGEMASK) >> 12;
                UINT nouveau = (nmlv->uNewState & LVIS_STATEIMAGEMASK) >> 12;
                if (ancien != nouveau && nouveau == 2 /* coche */) {
                    int i = nmlv->iItem;
                    if (i >= 0 && i < static_cast<int>(m_contributions.size()) && !m_contributions[i].valide) {
                        // Visee non exploitable : on interdit de la cocher.
                        ListView_SetCheckState(m_hListeRef, i, FALSE);
                    }
                }
            }
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (id == ID_COMBO_STATION && code == CBN_SELCHANGE) {
            int idx = ComboBox_GetCurSel(m_hComboStation);
            SelectionnerStation(idx);
        } else if (id == ID_CASE_PONDERE && code == BN_CLICKED) {
           // MajAffichageV0();
        } else if (id == ID_BTN_CALCULER) {
            Calculer();
        } else if (id == ID_BTN_ENREGISTRER) {
            Enregistrer();
        } else if (id == ID_BTN_LISTING) {
            MessageBox(m_hwnd, L"Listing RTF (à implémenter)", L"KONYX - Information", MB_ICONINFORMATION);
        } else if (id == ID_BTN_GRAPHIQUE) {
            OuvrirGraphique();
        } else if (id == ID_BTN_OK) {
            DestroyWindow(hwnd);
        } else if (id == ID_BTN_ANNULER) {
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        // Pas de PostQuitMessage ici : cette boite tourne dans sa propre
        // boucle GetMessage locale (voir Afficher()). Un PostQuitMessage
        // romprait la boucle de la fenetre GeoBase parente si elle
        // partage le meme thread (cf. bug deja rencontre sur GeoEditDialog).
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void GeoCalculV0Dialog::CreerControles(HWND hwnd) {
    static constexpr int xLabel = 20, xChamp = 110, wChampCourt = 100;
    static constexpr int yStation = 115, hLigne = 35;

    //position bouton radio
    int xRadio = xLabel;
    int yRadio = 20; // ta position

    auto appliquerPolice = [&](HWND h) { SendMessage(h, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE); };

    //radiobuttons
    HWND type_calcul = CreateWindow(L"STATIC", L"Type de calcul :", WS_CHILD | WS_VISIBLE,
                             xLabel, yRadio, 150, hLigne, hwnd, nullptr, nullptr, nullptr);
    appliquerPolice(type_calcul);

    HWND hRadioUnique = CreateWindow(L"BUTTON", L"Unique", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        xRadio + 180, yRadio, 100, hLigne, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_RADIO_UNIQUE)), nullptr, nullptr);
    appliquerPolice(hRadioUnique);

    HWND hRadioListe = CreateWindow(L"BUTTON", L"Liste", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        xRadio + 180 + 140, yRadio, 100, hLigne, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_RADIO_LISTE)), nullptr, nullptr);
    appliquerPolice(hRadioListe);

    HWND hRadioToutes = CreateWindow(L"BUTTON", L"Toutes", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        xRadio + 180 + 260, yRadio, 100, hLigne, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_RADIO_TOUTES)), nullptr, nullptr);
    appliquerPolice(hRadioToutes);

    Button_SetCheck(hRadioUnique, BST_CHECKED); // selection par defaut

    //liste des stations
    int yListeChamp = yRadio + hLigne + 10;

    HWND lblListe = CreateWindow(L"STATIC", L"Liste :", WS_CHILD | WS_VISIBLE,
        xLabel, yListeChamp, 100, hLigne, hwnd, nullptr, nullptr, nullptr);
    appliquerPolice(lblListe);

    m_hChampListe = CreateWindow(L"EDIT", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_DISABLED,
        xChamp, yListeChamp, 340 + 150 + 20, hLigne, hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_CHAMP_LISTE)), nullptr, nullptr);
    appliquerPolice(m_hChampListe);


    HWND lblStation = CreateWindow(L"STATIC", L"Station :", WS_CHILD | WS_VISIBLE,
                             xLabel, yStation, 100, hLigne, hwnd, nullptr, nullptr, nullptr);
    appliquerPolice(lblStation);

    m_hComboStation = CreateWindow(L"COMBOBOX", nullptr,
                                    WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                                    xChamp, yStation, 160, 300,
                                    hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_COMBO_STATION)),
                                    nullptr, nullptr);
    appliquerPolice(m_hComboStation);

    //forcage des ref
    HWND lblReferences = CreateWindow(L"STATIC", L"Références :", WS_CHILD | WS_VISIBLE,
                             xChamp + 160 + 30, yStation, 130, hLigne, hwnd, nullptr, nullptr, nullptr);
    appliquerPolice(lblReferences);

    m_hCaseRefForcees = CreateWindow(L"BUTTON", L"Forcer\nl'utilisation\ndes réf.\nchoisies",
                                   WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_MULTILINE | BS_TOP | BS_LEFT,
                                   xChamp + 160 + 30, yStation + hLigne, 130, hLigne*4, hwnd,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_CASE_PONDERE)),
                                   nullptr, nullptr);
    appliquerPolice(m_hCaseRefForcees);

    int yCoord = yListeChamp + hLigne + 100;
    const wchar_t* labelsCoord[3] = { L"E :", L"N :", L"H :" };
    HWND* champsCoord[3] = { &m_hX, &m_hY, &m_hZ };
    for (int i = 0; i < 3; ++i) {
        HWND l = CreateWindow(L"STATIC", labelsCoord[i], WS_CHILD | WS_VISIBLE,
                               xLabel + 25, yCoord, 30, hLigne, hwnd, nullptr, nullptr, nullptr);
        appliquerPolice(l);
        *champsCoord[i] = CreateWindow(L"EDIT", nullptr,
                                        WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER,
                                        xChamp, yCoord, 160, hLigne, hwnd, nullptr, nullptr, nullptr);
        appliquerPolice(*champsCoord[i]);
        yCoord += hLigne + 10;
    }

    m_hListeRef = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, nullptr,
                                  WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_NOCOLUMNHEADER,
                                  xChamp + 160 + 170, yStation, 180, 170,
                                  hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LISTE_REF)),
                                  nullptr, nullptr);
    ListView_SetExtendedListViewStyle(m_hListeRef,
                                       LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    appliquerPolice(m_hListeRef);

    LVCOLUMN col{};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.pszText = const_cast<wchar_t*>(L"Reference");
    col.cx = 110;
    ListView_InsertColumn(m_hListeRef, 0, &col);

    m_hBtnGraphique = CreateWindow(L"BUTTON", L"Graphique", WS_CHILD | WS_VISIBLE,
                                   xChamp + 160 + 130 + 2*20, yStation + 170 + 15, 130, 40, hwnd,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_GRAPHIQUE)), nullptr, nullptr);
    appliquerPolice(m_hBtnGraphique);

    int yApres = yCoord + 40;
    m_hCasePondere = CreateWindow(L"BUTTON", L"Pondérer par les distances",
                                   WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                   xChamp + 160 + 20, yApres + hLigne + 15, 280, hLigne, hwnd,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_CASE_PONDERE)),
                                   nullptr, nullptr);
    appliquerPolice(m_hCasePondere);

    HWND lblV0 = CreateWindow(L"STATIC", L"V0 :", WS_CHILD | WS_VISIBLE,
                               xLabel + 25, yApres, 40, hLigne, hwnd, nullptr, nullptr, nullptr);
    appliquerPolice(lblV0);
    m_hV0 = CreateWindow(L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER,
                          xChamp, yApres, 160, hLigne, hwnd,
                          reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_CHAMP_V0)), nullptr, nullptr);
    appliquerPolice(m_hV0);

    int yBtn = yApres + 2 * hLigne + 55;
    m_hBtnCalculer = CreateWindow(L"BUTTON", L"Calculer", WS_CHILD | WS_VISIBLE,
                                   xChamp + 160 + 20, yApres, 130, 40, hwnd,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_CALCULER)), nullptr, nullptr);
    appliquerPolice(m_hBtnCalculer);

    m_hBtnEnregistrer = CreateWindow(L"BUTTON", L"Enregistrer", WS_CHILD | WS_VISIBLE,
                                    xChamp + 160 + 130 + 2*20, yApres, 130, 40, hwnd,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_ENREGISTRER)), nullptr, nullptr);
    appliquerPolice(m_hBtnEnregistrer);


    m_hBtnListing = CreateWindow(L"BUTTON", L"Listing RTF", WS_CHILD | WS_VISIBLE,
                                xChamp + 160 - 130, yBtn, 130, 40, hwnd,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_LISTING)), nullptr, nullptr);
    appliquerPolice(m_hBtnListing);

    m_hBtnOK = CreateWindow(L"BUTTON", L"OK", WS_CHILD, // pas de WS_VISIBLE : cache au depart
                            660 - 40 - 130 - 20 - 130, yBtn, 130, 40, hwnd,
                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_OK)), nullptr, nullptr);
    appliquerPolice(m_hBtnOK);

    m_hBtnAnnuler = CreateWindow(L"BUTTON", L"Annuler", WS_CHILD | WS_VISIBLE,
                                660 - 40 - 130, yBtn, 130, 40, hwnd,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_ANNULER)), nullptr, nullptr);
    appliquerPolice(m_hBtnAnnuler);
}

void GeoCalculV0Dialog::RemplirComboStations() {
    ComboBox_ResetContent(m_hComboStation);
    m_indexParStationFiltree.clear(); // nouveau membre, voir .h ci-dessous

    for (size_t i = 0; i < m_stations.size(); ++i) {
        const StationEtVisees& s = m_stations[i];
        const GeoRecord* pt = TrouverPointParMatricule(m_enregistrements, s.station->matricule);
        if (!pt || !pt->X.has_value() || !pt->Y.has_value()) {
            continue; // station non calculable : on ne la propose pas
        }

        std::wstring texte = VersWide(s.station->matricule);
        if (s.indiceReprise > 1) {
            texte += L" (" + std::to_wstring(s.indiceReprise) + L")";
        }
        ComboBox_AddString(m_hComboStation, texte.c_str());
        m_indexParStationFiltree.push_back(i); // position combo -> index reel dans m_stations
    }

    if (!m_indexParStationFiltree.empty()) {
        ComboBox_SetCurSel(m_hComboStation, 0);
        SelectionnerStation(0);
    }
}

void GeoCalculV0Dialog::SelectionnerStation(int indexCombo) {
    if (indexCombo < 0 || indexCombo >= static_cast<int>(m_indexParStationFiltree.size())) return;
    m_indexStationCourante = static_cast<int>(m_indexParStationFiltree[indexCombo]);
    m_pointStationCourante = TrouverPointParMatricule(m_enregistrements, m_stations[indexCombo].station->matricule);

    SetWindowText(m_hX, m_pointStationCourante && m_pointStationCourante->X ? FormatNombre(*m_pointStationCourante->X, 3).c_str() : L"");
    SetWindowText(m_hY, m_pointStationCourante && m_pointStationCourante->Y ? FormatNombre(*m_pointStationCourante->Y, 3).c_str() : L"");
    SetWindowText(m_hZ, m_pointStationCourante && m_pointStationCourante->Z ? FormatNombre(*m_pointStationCourante->Z, 3).c_str() : L"");

    ListView_DeleteAllItems(m_hListeRef);
    m_contributions.clear();
    SetWindowText(m_hV0, L"");

    PeuplerListeReferences();
}

void GeoCalculV0Dialog::PeuplerListeReferences() {
    if (m_indexStationCourante < 0) return;
    const StationEtVisees& s = m_stations[m_indexStationCourante];

    std::vector<const GeoRecord*> references;
    for (const GeoRecord* v : s.visees) {
        if (v->type == GeoType::Reference) {
            references.push_back(v);
        }
    }

    ResultatV0 resultat = CalculerV0(m_pointStationCourante, references, m_enregistrements);
    m_contributions = resultat.contributions;

    ListView_DeleteAllItems(m_hListeRef);
    for (size_t i = 0; i < m_contributions.size(); ++i) {
        const ContributionV0& c = m_contributions[i];
        LVITEM item{};
        item.mask = LVIF_TEXT;
        item.iItem = static_cast<int>(i);
        std::wstring matricule = VersWide(c.visee->matricule);
        item.pszText = const_cast<wchar_t*>(matricule.c_str());
        ListView_InsertItem(m_hListeRef, &item);

        ListView_SetItemText(m_hListeRef, static_cast<int>(i), 1,
                              const_cast<wchar_t*>(c.valide ? FormatNombre(c.v0, 4).c_str() : L"-"));
        ListView_SetItemText(m_hListeRef, static_cast<int>(i), 2,
                              const_cast<wchar_t*>(c.valide ? FormatNombre(c.distance, 3).c_str() : L"-"));
        std::wstring statut = c.valide ? L"OK" : VersWide(c.raisonInvalide);
        ListView_SetItemText(m_hListeRef, static_cast<int>(i), 3, const_cast<wchar_t*>(statut.c_str()));

        ListView_SetCheckState(m_hListeRef, static_cast<int>(i), c.valide ? TRUE : FALSE);
    }
}

void GeoCalculV0Dialog::Calculer() {
    bool force = Button_GetCheck(m_hCaseRefForcees) == BST_CHECKED;
    if (!force) {
        std::vector<bool> auto_ = SelectionAutomatiqueV0(m_contributions);
        for (size_t i = 0; i < m_contributions.size(); ++i) {
            if (m_contributions[i].valide) {
                ListView_SetCheckState(m_hListeRef, static_cast<int>(i), auto_[i]);
            }
        }
    }
    MajAffichageV0();
    ShowWindow(m_hBtnOK, SW_SHOW);
}

void GeoCalculV0Dialog::OuvrirGraphique() {
    if (!m_pointStationCourante || !m_pointStationCourante->X || !m_pointStationCourante->Y) return;
    if (m_contributions.empty()) return; // il faut avoir peuple la liste (fait des l'ouverture)

    std::vector<GeoCalculV0Graphique::RefGraphique> refs;
    for (size_t i = 0; i < m_contributions.size(); ++i) {
        const ContributionV0& c = m_contributions[i];
        if (!c.valide || !c.pointRef || !c.pointRef->X || !c.pointRef->Y) continue;

        GeoCalculV0Graphique::RefGraphique r;
        r.matricule = c.visee->matricule;
        r.x = *c.pointRef->X;
        r.y = *c.pointRef->Y;
        r.selectionne = ListView_GetCheckState(m_hListeRef, static_cast<int>(i)) != 0;
        refs.push_back(r);
    }

    if (GeoCalculV0Graphique::Afficher(m_hwnd, m_stations[m_indexStationCourante].station->matricule,
                                        *m_pointStationCourante->X, *m_pointStationCourante->Y, refs)) {
        // Reporte la selection choisie graphiquement sur les cases a cocher.
        for (const auto& r : refs) {
            for (size_t i = 0; i < m_contributions.size(); ++i) {
                if (m_contributions[i].valide && m_contributions[i].visee->matricule == r.matricule) {
                    ListView_SetCheckState(m_hListeRef, static_cast<int>(i), r.selectionne);
                    break;
                }
            }
        }
    }
}

void GeoCalculV0Dialog::MajAffichageV0() {
    std::vector<bool> coches(m_contributions.size(), false);
    for (size_t i = 0; i < m_contributions.size(); ++i) {
        coches[i] = ListView_GetCheckState(m_hListeRef, static_cast<int>(i)) != 0;
    }
    bool pondere = Button_GetCheck(m_hCasePondere) == BST_CHECKED;

    double v0;
    if (MoyenneCirculaire(m_contributions, coches, pondere, v0)) {
        SetWindowText(m_hV0, FormatNombre(v0, 4).c_str());
    } else {
        SetWindowText(m_hV0, L"");
    }
}

void GeoCalculV0Dialog::Enregistrer() {
    if (m_indexStationCourante < 0) return;

    wchar_t buffer[64];
    GetWindowText(m_hV0, buffer, 64);
    if (wcslen(buffer) == 0) return; // rien a enregistrer

    try {
        double v0 = std::stod(buffer);
        // Retrouve l'enregistrement Station reel (pointeur const dans
        // m_stations -> on le relocalise dans m_enregistrements, car
        // ExtraireStationsEtVisees travaille sur des pointeurs const).
        const GeoRecord* stationConst = m_stations[m_indexStationCourante].station;
        for (GeoRecord& rec : m_enregistrements) {
            if (&rec == stationConst) {
                rec.V0 = v0;
                break;
            }
        }
    } catch (...) {
        MessageBox(m_hwnd, L"Valeur de V0 invalide.", L"Calcul de V0", MB_OK | MB_ICONERROR);
    }
}

} // namespace Konyx
