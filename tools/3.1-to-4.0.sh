#!/bin/bash
sqlite3 ../zeus.db "alter table options add LANG TEXT;"
sqlite3 ../zeus.db "update options SET LANG='es';"
sqlite3 ../zeus.db "CREATE TABLE IF NOT EXISTS OPERS (NICK TEXT UNIQUE NOT NULL, OPERBY TEXT, TIEMPO INT );"
