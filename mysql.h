

#define TABLE1 "\
create table image_snapshot_table (\
item SERIAL,\
image BIGINT,\
directory varchar(255) BINARY,\
filename varchar(255) BINARY,\
file_type char(1),\
file_perm char(10),\
file_size int(32) UNSIGNED,\
num_links smallint(3) UNSIGNED,\
unix_uid varchar(255),\
unix_gid varchar(255),\
timestamp_modify DATETIME,\
timestamp_access DATETIME,\
timestamp_create DATETIME,\
DOS_attrib char(6),\
file_hash char(255) BINARY\
);"

#define TABLE2 "\
create table image_list_table (\
image SERIAL,\
image_time timestamp,\
image_start varchar(255) BINARY,\
image_casenumber bigint(20) unsigned,\
image_investigator_name varchar(255) BINARY,\
image_imagename varchar(255) BINARY\
);"

#define CREATE_CASETABLE  "\
create table case_table (\
case_number SERIAL,\
case_name varchar(255) BINARY\
);"

#define CREATE_NCDTABLE "\
create table NCD_table (\
ncd_key SERIAL,\
querynumber INT,\
file_one BIGINT,\
file_two BIGINT\
);"

#define CREATE_NCDRESULT_TABLE "\
create table NCD_result (\
ncd_key bigint(20) unsigned,\
NCD_normal FLOAT,\
NCD_double FLOAT\
);"

#define CREATE_QUERYTABLE "\
create table query_table (\
querynumber INT,\
casenumber INT,\
delta_time_threshold BIGINT,\
delta_size_threshold BIGINT,\
delta_unit VARCHAR(20),\
delta_modify TINYINT(1),\
delta_access TINYINT(1),\
delta_create TINYINT(1),\
delta_size TINYINT(1),\
compare_file BIGINT,\
best_effort INT\
);"

#define CREATE_AnomalyConfig "\
CREATE TABLE AnomalyConfig (\
cutoff FLOAT,\
query INT\
);"

#define CREATE_AnomalyCurve "\
CREATE TABLE AnomalyCurve (\
number INT,\
anomaly FLOAT\
);"

#define CREATE_AnomalyQueryStart "\
CREATE VIEW AnomalyQueryStart AS \
select \
a.image AS IMG1,\
n.file_one AS F1,\
b.image AS IMG2,\
n.file_two AS F2,\
ncd_normal \
FROM NCD_table AS n \
JOIN NCD_result AS r \
JOIN image_snapshot_table AS a \
JOIN image_snapshot_table AS b \
WHERE n.ncd_key = r.ncd_key \
AND n.file_one = a.item AND n.file_two = b.item \
AND n.querynumber = (select query FROM AnomalyConfig) \
AND ncd_normal <= (select cutoff FROM AnomalyConfig) \
ORDER BY IMG1, F1;"

#define CREATE_Anomaly_Number_Collabrators "\
CREATE VIEW Anomaly_Number_Collabrators AS \
select DISTINCT IMG1, F1, COUNT(IMG2) As NUM FROM AnomalyQueryStart GROUP BY F1;"

#define CREATE_AnomalyJoin "\
CREATE VIEW AnomalyJoin AS \
SELECT A.IMG1, A.F1, C.anomaly \
FROM Anomaly_Number_Collabrators AS A \
JOIN AnomalyCurve AS C \
WHERE A.NUM = C.number;"

#define CREATE_Anomaly_Sum_of_Anomaly_Levels "\
CREATE VIEW Anomaly_Sum_of_Anomaly_Levels AS \
SELECT IMG1, ROUND(SUM(anomaly),3) AS Similar FROM AnomalyJoin GROUP BY IMG1; "

#define CREATE_COLLABRATION_CONFIG "\
CREATE TABLE Collabrative_Config (\
cutoff FLOAT\
);"

#define CREATE_COLLABORATION_START "\
CREATE VIEW Collabrative_Start AS \
select S.IMG1 AS IMG1, S.F1 AS F1, A1.anomaly AS A1, S.IMG2 AS IMG2 , S.F2 AS F2, A2.anomaly AS A2, S.ncd_normal AS NCD, ROUND((A1.anomaly + A2.anomaly),3) AS AnomalySUM \
FROM AnomalyQueryStart AS S \
JOIN AnomalyJoin AS A1 \
JOIN AnomalyJoin AS A2 \
WHERE S.IMG1 = A1.IMG1 \
AND S.F1 = A1.F1 \
AND S.IMG2 = A2.IMG1 \
AND S.F2 = A2.F1;"

#define CREATE_COLLABORATION_RESULT "\
CREATE VIEW Collabrative_Result AS \
select DISTINCT IMG1, IMG2, SUM(AnomalySUM) \
FROM Collabrative_Start \
WHERE IMG1 != IMG2 AND NCD < (SELECT cutoff FROM Collabrative_Config LIMIT 1) \
GROUP BY IMG1,IMG2;"

#ifndef DEFAULT_USER
#define DEFAULT_USER "snapshot"
#endif

#ifndef DEFAULT_PASS
#define DEFAULT_PASS "snapshot"
#endif

/*
INPUT: MySQL Handler

Outputs to STDERR - Errno and message
*/
void mysql_print_error(MYSQL *);

/*
INPUT
hostname 
username
password
database name
port number
socket name

NULL == Default

Returns pointer to MySQL handler or it exits the program
*/
MYSQL *mysql_connect(char *, char *, char *, char *, unsigned int, char *,unsigned int);

/*
INPUT 
MySQL Connection
Table
Limit Value (If 0, ignored)
*/
void showTable(MYSQL *, char*, int);
