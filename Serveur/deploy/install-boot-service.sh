#!/bin/bash
# Installe le script SysVinit security-server.init sur l'Odroid-C2 et
# l'active pour le runlevel par defaut (5), afin que le serveur demarre
# automatiquement au demarrage de la carte (bonus du Livrable 5).
#
# A executer une seule fois (ou apres modification de security-server.init).
# Le binaire prog lui-meme continue d'etre deploye separement via
# odroid-deploy-gdbserver.sh ; ce script ne touche qu'a la configuration de
# demarrage automatique.
readonly TARGET_IP="192.168.7.2"
readonly INIT_SCRIPT="deploy/security-server.init"
readonly TARGET_INIT_PATH="/etc/init.d/security-server"

echo "Installation du service de demarrage automatique sur la cible"

scp "${INIT_SCRIPT}" "root@${TARGET_IP}:${TARGET_INIT_PATH}"

ssh "root@${TARGET_IP}" "\
  chmod +x '${TARGET_INIT_PATH}' && \
  update-rc.d security-server defaults 25 75 && \
  echo 'Service installe et active (runlevel 5).'"

echo "Redemarrez l'Odroid-C2 (reboot) pour valider le demarrage automatique."
