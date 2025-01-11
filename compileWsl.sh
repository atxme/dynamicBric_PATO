#!/bin/bash


# Définition des chemins

WINDOWS_PROJECT_PATH="/mnt/c/Users/Christophe/source/repos/atxme/dynamicBric_PATO"

WSL_PROJECT_PATH="$HOME/dynamicBric_PATO"

BUILD_DIR="build"


# Création des répertoires nécessaires

mkdir -p $WSL_PROJECT_PATH

mkdir -p $WSL_PROJECT_PATH/$BUILD_DIR


# Copie des fichiers du projet

echo "Copie des fichiers sources..."

cp -r $WINDOWS_PROJECT_PATH/* $WSL_PROJECT_PATH/


# Création et déplacement dans le répertoire de build

cd $WSL_PROJECT_PATH/$BUILD_DIR


# Configuration avec CMake

echo "Configuration CMake..."

cmake -GNinja ..


# Compilation avec Ninja

echo "Compilation..."

ninja


# Vérification du statut de la compilation

if [ $? -eq 0 ]; then

    echo "Compilation terminée avec succès"

else

    echo "Erreur lors de la compilation"

    exit 1

fi

