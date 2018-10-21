#!/bin/bash
sqlite3 ../zeus.db "alter table options add LANG TEXT;"
sqlite3 ../zeus.db "update options SET LANG='en';"
sqlite3 ../zeus.db "CREATE TABLE IF NOT EXISTS OPERS (NICK TEXT UNIQUE NOT NULL, OPERBY TEXT, TIEMPO INT );"
sqlite3 ../zeus.db "CREATE TABLE IF NOT EXISTS SPAM (MASK TEXT UNIQUE NOT NULL, WHO TEXT, MOTIVO TEXT, TARGET TEXT );"
