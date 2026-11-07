#pragma once

#include <string>
#include <vector>
#include <optional>

namespace Konyx {

// Types de lignes possibles dans un fichier .geo
// (voir doc Covadis - Editeur de GeoBases - Types d'elements)
enum class GeoType {
    Station,
    Reference,
    Mesure,
    Point,
    NivLectAR,
    NivLectAV,
    Commande,
    Commentaire,
    CheminAntenne,
    CheminEncadre,
    Appareil,
    Photo,
    Polyligne,
    ExcentVer,
    ExcentRad,
    ExcentTan,
    OptionEntete,   // lignes d'entete : Option_Correc, Option_Projec, Option_Nivell, Option_Planim
    Inconnu
};

// Une ligne de GeoBase. Les champs non utilises par le type courant
// restent vides (std::nullopt) plutot que d'utiliser une union fragile :
// la syntaxe reelle des lignes .geo est a nombre de champs variable
// (ex: Reference peut n'avoir que <AH>, ou HP+AH+AV+DI, avec ou sans SY).
struct GeoRecord {
    int numLigne = 0;              // numero affiche en colonne "Ligne" (000001, ...)
    GeoType type = GeoType::Inconnu;
    std::string typeBrut;          // mot-cle tel que lu dans le fichier (utile pour Inconnu/debug)
    bool estCommentaire = false;   // prefixe "Commentaire" devant un autre type (ligne desactivee)

    std::string matricule;         // nom du point / station / cheminement

    // Station
    std::optional<double> HI;      // hauteur d'instrument
    std::optional<double> V0;      // orientation de la station (-1.0 = absent)

    // Reference / Mesure
    std::optional<double> HP;      // hauteur de prisme
    std::optional<double> AH;      // angle horizontal (grades)
    std::optional<double> AV;      // angle vertical (grades)
    std::optional<double> DI;      // distance inclinee (m)

    // Point
    std::optional<double> X;
    std::optional<double> Y;
    std::optional<double> Z;
    std::optional<int> CP;         // constance du point (0-3)

    // NivLectAR / NivLectAV
    std::optional<double> LM;      // lecture sur mire
    std::optional<double> DH;      // distance horizontale

    // Commun a plusieurs types
    std::optional<int> SY;         // code symbole

    // CheminAntenne / CheminEncadre
    std::vector<std::string> stationsChemin; // ex: "S.18-1", "S.17-1", ...

    // Excentrement (ExcentVer/Rad/Tan)
    std::optional<double> excentrement;

    // Commentaire pur (ligne "Commentaire <texte libre>")
    std::string texteCommentaire;

    // Appareil
    std::optional<double> appCV, appCP, appPAh, appPAv, appPDi, appPPM;
    std::string appNom;

    // Photo
    std::string cheminPhoto;

    // Ligne d'entete brute (Option_Correc, Option_Projec, Option_Nivell, Option_Planim)
    std::string texteEnteteBrut;

    // Ligne non reconnue : on garde le texte original pour ne rien perdre
    // en re-sauvegardant le fichier.
    std::string texteBrutOriginal;
};

// Construit la chaine affichee dans la colonne "Parametres" du tableau,
// au format Covadis (ex: "HP = 1.600 , AH = 126.38870 , AV = 99.65870 , DI = 78.954").
std::string FormatParametres(const GeoRecord& rec);

// Libelle affiche dans la colonne "Element".
std::string FormatElement(const GeoRecord& rec);

} // namespace Konyx
