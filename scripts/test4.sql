use SNAPSHOT;

select n.ncd_key, CONCAT(a.directory,'/', a.filename), CONCAT(b.directory, '/', b.filename) 
FROM image_snapshot_table AS a 
JOIN image_snapshot_table AS b 
JOIN NCD_table AS n ON n.file_one = a.item AND n.file_two = b.item AND n.querynumber = 33;

