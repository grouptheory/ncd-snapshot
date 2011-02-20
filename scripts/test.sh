#!/bin/csh -f

date;
cat test4.sql | mysql -u snapshot -psnapshot >& /dev/null
date;
