#!/bin/bash

# Script di installazione per zc bundle
# Permette di installare zc come software portatile senza installazione system

# Verifica della directory di installazione
if [ ! -d "./install" ]; then
  echo "Creazione directory di installazione..."
  mkdir -p ./install
fi

# Copia il bundle
echo "Copio il bundle in ./install/"
cp -r dist/zc-bundle ./install/

# Aggiungo la variabile d'ambiente per l'esecuzione del bundle
export ZC_BUNDLE_PATH="./install/zc-bundle"

# Verifica dell'installazione
if [ -f "./install/zc-bundle/zc" ]; then
  echo "Installazione completata!"
  echo "" 
  echo "Uso:"
  echo "  ./install/zc-bundle/zc"
  echo "" 
  echo "Informazioni:"
  echo "  Per verificare l'installazione, esegui:"
  echo "  ./install/zc-bundle/zc --version"
else
  echo "Errore: non è stato possibile installare zc"
  exit 1
fi

# Aggiungo le istruzioni per l'uso
echo "" 
echo "Informazioni aggiuntive:"
echo "  - Il bundle è stato installato in ./install/zc-bundle"
echo "  - L'ambiente è configurato con la variabile ZC_BUNDLE_PATH"
echo "  - Per rimuovere l'installazione, esegui:"
echo "  rm -rf ./install/zc-bundle"

# Versione del bundle
./install/zc-bundle/zc --version
