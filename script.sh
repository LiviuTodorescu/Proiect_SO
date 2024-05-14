#!/bin/bash

# Verificăm dacă există exact un argument
if [ "$#" -ne 1 ]; then 
    echo "Numărul de argumente este total incorect!"
    exit 1
fi 

nume_fisier="$1"

# Verificăm existența și accesibilitatea fișierului
if [ ! -f "$nume_fisier" ]; then
    echo "Fișierul $nume_fisier nu există sau nu este un fișier!"
    exit 1
fi

# Verificăm dacă utilizatorul are permisiunea de citire pentru fișier
if [ ! -r "$nume_fisier" ]; then
    echo "Nu ai permisiunea de a citi fișierul $nume_fisier"
    exit 1
fi

# Salvăm drepturile inițiale ale fișierului
drepturi_initiale=$(stat -c %a "$nume_fisier")

# Acordăm drepturile 777 dacă fișierul nu are niciun drept
if [ "$drepturi_initiale" == "000" ]; then
    echo "Fișierul nu are niciun drept. Acordăm drepturile 777."
    chmod 777 "$nume_fisier"
fi

gasit=0

# Verificăm conținutul fișierului pentru caractere non-ASCII
if grep -q -P '[^\x00-\x7F]' "$nume_fisier"; then    
    echo "Fișierul $nume_fisier conține caractere non-ASCII!"
    gasit=1
fi 

# Verificăm conținutul fișierului pentru cuvinte periculoase
if grep -q -E 'malefic|periculos|parola' "$nume_fisier"; then
    echo "Fișierul $nume_fisier conține cuvinte periculoase"
    gasit=1
fi

# Restaurăm drepturile inițiale ale fișierului
if [ "$drepturi_initiale" == "000" ]; then
    echo "Restaurăm drepturile inițiale."
    chmod "$drepturi_initiale" "$nume_fisier"
fi

exit $gasit
