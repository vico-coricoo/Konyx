#pragma once

#include <string>
#include <vector>
#include "GeoRecord.h"

namespace Konyx {

struct GeoBaseEntete {
    std::string optionCorrec;   // ligne "Option_Correc" brute
    std::string optionProjec;   // ligne "Option_Projec" brute
    std::string optionNivell;   // ligne "Option_Nivell" brute
    std::string optionPlanim;   // ligne "Option_Planim" brute
};

class GeoBaseParser {
public:
    // Charge un fichier .geo. Retourne false si le fichier n'a pas pu etre ouvert.
    // En cas de ligne non reconnue, elle est conservee telle quelle (type Inconnu)
    // pour ne perdre aucune donnee a la re-sauvegarde.
    static bool Charger(const std::string& cheminFichier,
                         GeoBaseEntete& entete,
                         std::vector<GeoRecord>& enregistrements);

    // Sauvegarde dans le meme format que l'original.
    static bool Sauvegarder(const std::string& cheminFichier,
                             const GeoBaseEntete& entete,
                             const std::vector<GeoRecord>& enregistrements);

private:
    static GeoRecord ParserLigne(int numLigne, const std::string& ligne);
    static std::string SerialiserLigne(const GeoRecord& rec);
};

} // namespace Konyx
