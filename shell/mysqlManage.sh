#!/bin/bash


#mysql cmd
#show binary logs;
#show master status;
#reset master;
#purge binary logs to 'mysql-bin.000003';
#PURGE MASTER LOGS BEFORE '2008-06-22 13:00:00¡ä;
#PURGE MASTER LOGS BEFORE DATE_SUB(NOW(), INTERVAL 3 DAY);


#mysqldump -uDBuser -pPassword --quick --force --routines --add-drop-database --all-databases --add-drop-table > /data/bkup/mysqldump.sql
#mysql -uDBuser -pPassword < /data/bkup/mysqldump.sql