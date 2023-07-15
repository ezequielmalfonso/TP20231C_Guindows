#!/bin/bash
path=$(pwd)

echo "Ubicacion actual: $path"
#cd ..
path2=$(pwd)
echo "Ubicacion actual: $path2"
echo "#############################################################"
echo "Ejecuto prueba DEADLOCK_1"
echo "#############################################################"
LD_LIBRARY_PATH="/home/utnso/tp-2023-1c-Guindows/shared-lib/Debug" ./Debug/consola "/home/utnso/tp-2023-1c-Guindows/consola/entrega/DEADLOCK_1" "/home/utnso/tp-2023-1c-Guindows/consola/consolaConfig/consola.conf"

