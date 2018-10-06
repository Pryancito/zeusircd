#!/bin/bash
echo "alter table options add LANG TEXT;" > sqlite3 ../zeus.db
echo "update options SET LANG='es';" > sqlite3 ../zeus.db
echo "CREATE TABLE IF NOT EXISTS OPERS (NICK TEXT UNIQUE NOT NULL, OPERBY TEXT, TIEMPO INT );" > sqlite3 ../zeus.db
