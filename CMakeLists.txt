# CMakeLists.txt - Konyx (.arx)
#
# Ce projet se compile UNIQUEMENT sous Windows avec Visual Studio (MSVC),
# car ObjectARX est une techno Windows/AutoCAD. Vous pouvez tres bien
# ecrire et relire le code sur votre MBP2024 (VSCode), mais la compilation
# et les tests se feront sur votre MBP2019 en dual-boot Windows 11.
#
# Variable d'environnement attendue : KONYX_ARX_SDK
#   -> chemin vers le dossier racine du ObjectARX SDK pour AutoCAD 2027
#      (celui qui contient inc/, lib-x64/, etc.)
#
# Exemple (PowerShell, avant de lancer cmake) :
#   $env:KONYX_ARX_SDK = "C:\ObjectARX 2027 SDK"

cmake_minimum_required(VERSION 3.20)
project(Konyx LANGUAGES CXX)

if(NOT WIN32)
    message(WARNING
        "Konyx (ObjectARX) ne peut etre COMPILE que sous Windows avec MSVC. "
        "Cette configuration sur macOS/Linux sert uniquement a l'edition du "
        "code (coloration syntaxique, navigation) dans VSCode, pas a la "
        "compilation.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Localisation du SDK ObjectARX ---
if(DEFINED ENV{KONYX_ARX_SDK})
    set(ARX_SDK_DIR "$ENV{KONYX_ARX_SDK}")
else()
    set(ARX_SDK_DIR "" CACHE PATH "Chemin vers le ObjectARX SDK (ex: C:/ObjectARX 2027 SDK)")
endif()

if(WIN32 AND ARX_SDK_DIR STREQUAL "")
    message(FATAL_ERROR
        "Definissez KONYX_ARX_SDK (variable d'environnement) ou ARX_SDK_DIR "
        "(cache CMake) pointant vers le ObjectARX SDK d'AutoCAD 2027.")
endif()

# --- Sources du module ---
set(KONYX_SOURCES
    src/KonyxApp.cpp
    src/core/CommandRegistry.cpp
    src/commands/KonCalculs/KonCalculsCommands.cpp
    src/commands/KonCalculs/GeoBase/GeoRecord.cpp
    src/commands/KonCalculs/GeoBase/GeoBaseParser.cpp
    src/commands/KonCalculs/GeoBase/GeoBaseDialog.cpp
    src/commands/Konyx2D/Konyx2DCommands.cpp
    src/commands/Konyx3D/Konyx3DCommands.cpp
    src/commands/KonEdition/KonEditionCommands.cpp
    src/core/UI/KonyxUI.cpp
    src/commands/KonCalculs/GeoBase/GeoEditDialog.cpp
    src/commands/KonCalculs/GeoBase/GeoStationVisees.cpp
    src/commands/KonCalculs/GeoBase/GeoCalculV0.cpp
    src/commands/KonCalculs/GeoBase/GeoCalculV0Dialog.cpp
    src/commands/KonCalculs/GeoBase/GeoCalculV0Graphique.cpp
    src/commands/KonCalculs/GeoBase/GeoBaseRuban.cpp
    src/commands/KonCalculs/GeoBase/GeoAjouterTypeDialog.cpp
)

# Le module ObjectARX est une DLL Windows, simplement renommee en .arx.
add_library(Konyx SHARED ${KONYX_SOURCES})

if(WIN32)
    target_include_directories(Konyx PRIVATE "${ARX_SDK_DIR}/inc")
    target_link_directories(Konyx PRIVATE "${ARX_SDK_DIR}/lib-x64")

    # Bibliotheques ObjectARX minimales pour un module de commandes.
    # Noms confirmes dans lib-x64/ du SDK ObjectARX 2027 installe
    # (suffixe "26" = version interne du SDK pour AutoCAD 2027).
    target_link_libraries(Konyx PRIVATE
        rxapi       # services de base ObjectRx (classes, enregistrement)
        ac1st26     # runtime ARX de base (point d'entree, cycle de vie)
        acdb26      # base de donnees du dessin (AcDb*, entites, blocs...)
        acge26      # geometrie (AcGe*, points, vecteurs, courbes...)
	    accore      # AcEdCommandStack, acedRegCmds et commandes editeur
        gdiplus
        comctl32
    )

    target_compile_definitions(Konyx PRIVATE
        _USRDLL
        UNICODE
        _UNICODE
        NOMINMAX
    )

    # Renomme la sortie .dll en .arx : c'est l'extension attendue par AutoCAD.
    set_target_properties(Konyx PROPERTIES
        SUFFIX ".arx"
        OUTPUT_NAME "Konyx"
    )

    target_compile_options(Konyx PRIVATE /utf-8)
endif()
