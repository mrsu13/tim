--------------
-- Триггеры --
--------------

BEGIN;

-------------------------------------------
-- Идентификаторы не могут быть изменены --
-------------------------------------------

-- Пользователь
DROP TRIGGER IF EXISTS user_update_id;
CREATE TRIGGER user_update_id BEFORE UPDATE ON user
    WHEN NEW.id IS NOT NULL AND NEW.id != OLD.id
BEGIN
    SELECT RAISE(ROLLBACK, 'User ID may not be changed.');
END;

-- Сообщение
DROP TRIGGER IF EXISTS post_update_id;
CREATE TRIGGER post_update_id BEFORE UPDATE ON post
    WHEN NEW.id IS NOT NULL AND NEW.id != OLD.id
BEGIN
    SELECT RAISE(ROLLBACK, 'Post ID may not be changed.');
END;

-- Реакция
DROP TRIGGER IF EXISTS reaction_update_id;
CREATE TRIGGER reaction_update_id BEFORE UPDATE ON reaction
    WHEN NEW.id IS NOT NULL AND NEW.id != OLD.id
BEGIN
    SELECT RAISE(ROLLBACK, 'Reaction ID may not be changed.');
END;

COMMIT;
