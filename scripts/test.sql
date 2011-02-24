
use SNAPSHOT;

select DISTINCT a.item, b.item 
FROM image_snapshot_table AS a 
JOIN image_snapshot_table AS b 
JOIN image_list_table As i 
where a.image IN (select image FROM image_list_table WHERE image_casenumber = 4 ) 
  AND b.image IN (select image FROM image_list_table WHERE image_casenumber = 4 ) 
  AND NOT a.file_type = 'd' 
  AND NOT b.file_type = 'd' 
  AND (0  ) >= 0;
