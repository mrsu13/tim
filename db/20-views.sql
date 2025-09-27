----------
-- Виды --
----------

BEGIN;

-- Генератор UUID.
DROP VIEW IF EXISTS generate_id;
CREATE VIEW generate_id AS
    SELECT '"{' || substr(u, 1, 8) || '-' || substr(u, 9, 4) || '-4' || substr(u, 13, 3)
                || '-' || v || substr(u, 17, 3) || '-' || substr(u, 21, 12) || '}"' AS id
        FROM
            (SELECT lower(hex(randomblob(16))) AS u, substr('89ab', abs(random()) % 4 + 1, 1) AS v);

COMMIT;
