---------------------
-- Тестовые данные --
---------------------

BEGIN;

------------------
-- Пользователи --
------------------

INSERT INTO user (id, name)
    VALUES ((SELECT id FROM generate_id LIMIT 1), '"user1"');

INSERT INTO user_password (user_id, password)
    VALUES ((SELECT id FROM user WHERE name = '"user1"' LIMIT 1), '"Qwerty"');

INSERT INTO user (id, name)
    VALUES ((SELECT id FROM generate_id LIMIT 1), '"user2"');

INSERT INTO user_password (user_id, password)
    VALUES ((SELECT id FROM user WHERE name = '"user2"' LIMIT 1), '"Qwerty"');

COMMIT;
