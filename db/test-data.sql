---------------------
-- Тестовые данные --
---------------------

BEGIN;

------------------
-- Пользователи --
------------------

INSERT INTO user (id, pkey, nick)
    VALUES ((SELECT id FROM generate_id LIMIT 1), "7937123456", '"user1"');

INSERT INTO user (id, pkey, nick)
    VALUES ((SELECT id FROM generate_id LIMIT 1), "7937123457", '"user2"');

COMMIT;
