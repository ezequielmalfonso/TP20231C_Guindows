#!/bin/bash
path=$(pwd)

echo "Ubicacion actual: $path"
#cd ..
path2=$(pwd)
echo "Ubicacion actual: $path2"
echo "#############################################################"
echo "Ejecuto prueba ERROR_3"
echo "#############################################################"
LD_LIBRARY_PATH="/home/utnso/tp-2023-3c-Guindows/shared-lib/Debug" ./Debug/consola "/home/utnso/tp-2023-3c-Guindows/consola/entrega/ERROR_3" "/home/utnso/tp-2023-3c-Guindows/consola/consolaConfig/consola.conf"

