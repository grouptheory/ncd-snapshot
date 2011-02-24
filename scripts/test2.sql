
use SNAPSHOT;

DROP VIEW IF EXISTS images_for_case;
CREATE VIEW images_for_case AS select image FROM image_list_table WHERE image_casenumber = 5;

DROP VIEW IF EXISTS items_for_case;
CREATE VIEW items_for_case AS select item FROM image_snapshot_table WHERE (NOT file_type = 'd') AND (image IN (SELECT image FROM images_for_case));

select DISTINCT a.item, b.item 
FROM items_for_case AS a 
JOIN items_for_case AS b 
ON (0  ) >= 0;
