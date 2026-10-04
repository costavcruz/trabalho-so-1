#!/bin/bash
echo "A lançar as aplicações do Cenário 1..."

# Lança cada processo em background (graças ao & no final)
./cmake-build-debug/application scenarios/1/A.csv &
./cmake-build-debug/application scenarios/1/B.csv &
./cmake-build-debug/application scenarios/1/C.csv &

echo "Aplicações lançadas! A aguardar que o SO as processe..."

# O wait diz ao script para não fechar até que o A, B e C terminem
wait

echo "Todas as aplicações terminaram!"