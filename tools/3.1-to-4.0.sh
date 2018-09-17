#!/bin/bash
echo "alter table options add LANG TEXT;" > sqlite3 ../zeus.db
echo "update options SET LANG='es';" > sqlite3 ../zeus.db
