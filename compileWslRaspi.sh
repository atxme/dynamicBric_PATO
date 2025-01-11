#!/bin/bash


# Définition des chemins

WINDOWS_PROJECT_PATH="/mnt/c/Users/Christophe/source/repos/atxme/dynamicBric_PATO"

WSL_PROJECT_PATH="$HOME/dynamicBric_PATO"

BUILD_DIR="build-debug-rpi"


# Affichage des informations de configuration

echo "Configuration de la compilation Debug Raspberry Pi"

echo "------------------------------------------------"

echo "Chemin Windows : $WINDOWS_PROJECT_PATH"

echo "Chemin WSL : $WSL_PROJECT_PATH"

echo "Dossier de build : $BUILD_DIR"

echo "------------------------------------------------"


# Nettoyage des anciens fichiers si nécessaire

rm -rf $WSL_PROJECT_PATH

rm -rf $BUILD_DIR


# Création des répertoires nécessaires

mkdir -p $WSL_PROJECT_PATH


# Copie des fichiers du projet

echo "Copie des fichiers sources..."

cp -r $WINDOWS_PROJECT_PATH/* $WSL_PROJECT_PATH/


# Création du répertoire de build

cd $WSL_PROJECT_PATH

mkdir -p $BUILD_DIR

cd $BUILD_DIR


# Configuration avec CMake pour Debug Raspberry Pi

echo "Configuration CMake pour Debug Raspberry Pi..."

cmake -GNinja -DCMAKE_BUILD_TYPE=Debug -DENABLE_RASPBERRY=ON -DRPI_VERSION=4 ..


# Compilation avec Ninja

if [ $? -eq 0 ]; then

    echo "Compilation..."

    ninja

    

    # Vérification du statut de la compilation

    if [ $? -eq 0 ]; then

        echo "------------------------------------------------"

        echo "Compilation terminée avec succès"

        echo "Build disponible dans : $WSL_PROJECT_PATH/$BUILD_DIR"

        

        # Affichage des informations de build

        echo "------------------------------------------------"

        echo "Information sur le build :"

        file $WSL_PROJECT_PATH/$BUILD_DIR/libdynamicBric_PATO.so

        echo "------------------------------------------------"

    else

        echo "Erreur lors de la compilation avec ninja"

        exit 1

    fi

else

    echo "Erreur lors de la configuration CMake"

    exit 1

fi


