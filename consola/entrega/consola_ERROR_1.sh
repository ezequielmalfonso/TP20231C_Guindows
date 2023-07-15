#!/bin/bash
path=$(pwd)

echo "Ubicacion actual: $path"
#cd ..
path2=$(pwd)
echo "Ubicacion actual: $path2"
echo "#############################################################"
echo "Ejecuto prueba ERROR_1"
echo "#############################################################"
LD_LIBRARY_PATH="/home/utnso/tp-2023-2c-Guindows/shared-lib/Debug" ./Debug/consola "/home/utnso/tp-2023-2c-Guindows/consola/entrega/ERROR_1" "/home/utnso/tp-2023-2c-Guindows/consola/consolaConfig/consola.conf"

