--------------------------------------------
-- Данные, необходимые для работы системы --
--------------------------------------------

BEGIN;

------------------
-- Пользователи --
------------------

INSERT INTO user (id, name)
    VALUES ((SELECT id FROM generate_id LIMIT 1), '"admin"');

INSERT INTO user_password (user_id, password)
    VALUES ((SELECT id FROM user WHERE name = '"admin"' LIMIT 1), '"Qwerty"');

COMMIT;
