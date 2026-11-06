#pragma once

#include <vector>
#include <string>
#include <optional>
#include "GeoRecord.h"

namespace Konyx {

// Contribution d'une visee (Reference ou Mesure) au calcul du V0 d'une
// station. "valide" indique si cette visee a pu etre exploitee (coords
// de la reference connues ET AH mesure present) ; sinon raisonInvalide
// explique pourquoi (a afficher grise/desactive dans le futur dialogue).
struct ContributionV0 {
    const GeoRecord* visee = nullptr;      // la Reference/Mesure concernee
    const GeoRecord* pointRef = nullptr;   // le Point donnant ses coordonnees (nullptr si inconnu)
    bool valide = false;
    std::string raisonInvalide;            // "coordonnees de la reference inconnues", "AH non mesure", ...
    double gisement = 0.0;                 // grades, [0,400)
    double v0 = 0.0;                       // grades, [0,400)
    double distance = 0.0;                 // station -> reference
};

struct ResultatV0 {
    std::vector<ContributionV0> contributions; // une par visee, valides et invalides
    std::optional<double> v0Moyen;    // moyenne arithmetique des contributions valides
    std::optional<double> v0Pondere;  // moyenne ponderee par la distance
};

// Gisement (grades, [0,400)) du point (xSt,ySt) vers (xRef,yRef).
double CalculerGisement(double xSt, double ySt, double xRef, double yRef);

// Ramene un angle en grades dans l'intervalle [0,400).
double NormaliserGrades(double grades);

// Calcule le V0 d'une station a partir de ses visees (telles que
// retournees par ExtraireStationsEtVisees) et de la liste complete des
// enregistrements (pour retrouver les coordonnees des references via
// TrouverPointParMatricule). Si la station elle-meme n'a pas de
// coordonnees connues (pointStation == nullptr), toutes les contributions
// sont invalides et v0Moyen/v0Pondere restent vides : le calcul n'est
// pas possible ici (necessite de calculer la station comme un point
// isole au prealable, hors scope de cette fonction).
ResultatV0 CalculerV0(const GeoRecord* pointStation,
                       const std::vector<const GeoRecord*>& visees,
                       const std::vector<GeoRecord>& enregistrements);


// Determine automatiquement les visees a inclure dans la moyenne quand
// la case "Forcer l'utilisation des ref. choisies" n'est PAS cochee :
// toutes les contributions valides sont utilisees, sauf si (a partir de
// 3 references valides) l'une d'elles s'ecarte de plus de seuilGon des
// autres alors que les autres sont proches entre elles. En dessous de
// 3 references valides, aucun controle n'est possible : toutes sont
// gardees. Retourne un vecteur parallele a contributions (true = a
// cocher).
std::vector<bool> SelectionAutomatiqueV0(const std::vector<ContributionV0>& contributions, double seuilGon = 0.1);

} // namespace Konyx
