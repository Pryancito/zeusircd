#!/usr/bin/env bash

process=`cat zeus.pid`

if [ ! -e "zeus.pid" ]; then
        ./Zeus -start
        echo -e "Zeus Iniciado"
elif [ ! -n "$(ps -p $process -o pid=)" ]; then
        ./Zeus -stop
        ./Zeus -start
        echo -e "Zeus reiniciado"
else
        echo -e "Zeus ya ejecutado"
fi
