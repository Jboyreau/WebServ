#!/bin/bash

# Afficher un message de début
echo "Liste des fichiers .c et .h dans le projet :"

# Trouver et lister les fichiers .c et .h
find . -type f \( -name "*.c" -o -name "*.h" \) | while read file; do
    # Afficher le nom du fichier
    echo "Fichier : $file"
    # Afficher le contenu du fichier
    cat "$file"
    # Ajouter une ligne vide pour la lisibilité
    echo ""
done

# Afficher un message de fin
echo "Liste complète des fichiers trouvés."

