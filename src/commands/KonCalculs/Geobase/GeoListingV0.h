#pragma once

#include <string>
#include <vector>
#include "GeoRecord.h"

namespace Konyx {

// Tolerance fixe utilisee pour le controle de chaque visee (gon),
// affichee dans l'entete et utilisee pour detecter les references hors
// tolerance dans les "Remarques". En attendant un vrai parametrage des
// tolerances de geobase (projection, classe de canevas...), cette valeur
// et le texte TOLERANCES_LIBELLE ci-dessous reprennent tels quels
// l'entete du listing Covadis de reference ; a brancher sur un reglage
// reel le jour ou ce parametrage existera dans Konyx.
constexpr double TOLERANCE_V0_GON = 0.0013;
constexpr const char* TOLERANCES_LIBELLE = "Cadastrales 1980 - Canevas : polygonal de précision";
constexpr const char* PROJECTION_LIBELLE = "Aucune projection";

// Genere le contenu RTF complet du listing "CALCUL DES V0 DE STATIONS" :
// une page par station calculable de la geobase (coordonnees de station
// connues ET au moins une visee de type Reference), dans l'ordre ou les
// stations apparaissent dans 'enregistrements'. Chaque page reprend
// l'entete general (geobase, projection, tolerances) puis le detail de
// la station (coordonnees d'appui, tableau Gi/Dh/Ah/V0/eV0, remarques
// de tolerance).
//
// 'cheminGeobase' est le chemin du fichier .geo a afficher tel quel dans
// l'entete (vide si le fichier n'a pas encore ete enregistre).
//
// Retourne une chaine RTF (encodage ASCII avec echappements \uN pour les
// caracteres non-ASCII) directement ecrivable telle quelle dans un
// fichier .rtf.
std::string GenererListingV0(const std::vector<GeoRecord>& enregistrements,
                              const std::wstring& cheminGeobase);

} // namespace Konyx
