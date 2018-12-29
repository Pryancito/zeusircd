#/usr/bin/env bash

sqlite3 ../zeus.db "alter table CMODES add ONLYWEB INT;"
