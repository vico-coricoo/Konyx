#pragma once

#include <vector>
#include <optional>
#include "GeoRecord.h"

namespace Konyx {

// Une station et le groupe de References/Mesures qui la suivent
// immediatement dans la GeoBase (visees depuis cette station).
//
// Regle (imposee par l'utilisateur) : l'ordre des lignes dans la geobase
// fait foi. Les visees d'une station sont les References/Mesures situees
// juste apres elle, jusqu'a ce qu'un element d'un autre type apparaisse
// (une autre Station, un Point, un Commentaire, etc.), ce qui marque la
// fin du groupe.
//
// "Reprise" : une station peut etre revisee plus tard dans la geobase
// avec le meme matricule (ex : retour sur le point A l'apres-midi).
// indiceReprise numerote ces occurrences : 1 pour la premiere apparition
// du matricule, 2 pour la seconde, etc.
struct StationEtVisees {
    const GeoRecord* station = nullptr;
    int indiceReprise = 1;                  // 1-based parmi les stations de meme matricule
    std::vector<const GeoRecord*> visees;    // References/Mesures suivant la station (dans l'ordre)
};

// Parcourt toute la GeoBase et regroupe chaque Station avec ses visees.
// Les lignes marquees estCommentaire sont ignorees (elles ne comptent ni
// comme Station, ni comme visee, ni comme "autre type" qui romprait un
// groupe) : elles n'existent pas pour cette analyse, comme un commentaire
// insere au milieu d'un groupe de visees.
std::vector<StationEtVisees> ExtraireStationsEtVisees(const std::vector<GeoRecord>& enregistrements);

// Cherche un Point (coordonnees connues) portant ce matricule. Retourne
// nullptr si aucun Point ne correspond. En cas de doublon (plusieurs
// Points de meme matricule), retourne le premier trouve.
const GeoRecord* TrouverPointParMatricule(const std::vector<GeoRecord>& enregistrements,
                                           const std::string& matricule);

// Liste, sans doublon et dans l'ordre d'apparition, les matricules de
// toutes les stations de la geobase (utile pour peupler le combo
// "Station" du futur dialogue V0 ; chaque matricule peut correspondre a
// plusieurs reprises, cf ExtraireStationsEtVisees).
std::vector<std::string> ListerMatriculesStations(const std::vector<GeoRecord>& enregistrements);

} // namespace Konyx
