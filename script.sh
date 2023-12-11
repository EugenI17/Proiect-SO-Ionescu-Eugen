#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Utilizare: $0 <caracter>"
    exit 1
fi

caracter=$1
contor=0

while line_exists= read -r linie; do
    if [[ $linie =~ ^[A-Z] && $linie =~ [$caracter] && $linie =~ [\.!?]$ && ! $linie =~ ,\ și ]]; then
        ((contor++))
    fi
done

printf "%04d\n" $contor
