-- Включаем Write-Ahead Logging (https://www.sqlite.org/wal.html)
PRAGMA journal_mode = WAL;

BEGIN;

-------------
-- Таблицы --
-------------

-- Конфигурация
DROP TABLE IF EXISTS configuration;
CREATE TABLE configuration
(
    name VARCHAR UNIQUE COLLATE NOCASE NOT NULL,
    value VARCHAR NOT NULL,
    title VARCHAR UNIQUE NOT NULL,
    read_only INTEGER DEFAULT 0
);
CREATE UNIQUE INDEX configuration_name ON configuration(name);
CREATE UNIQUE INDEX configuration_title ON configuration(title);


-- Пользователи
DROP TABLE IF EXISTS user;
CREATE TABLE user
(
    id VARCHAR PRIMARY KEY NOT NULL CHECK(id != '""' AND id != '"{00000000-0000-0000-0000-000000000000}"'),

    name VARCHAR UNIQUE COLLATE NOCASE NOT NULL, -- Имя для входа.

    icon VARCHAR,

    first_name VARCHAR,
    middle_name VARCHAR,
    last_name VARCHAR,

    title VARCHAR,
    motto VARCHAR,

    email VARCHAR,
    phone VARCHAR
);
CREATE UNIQUE INDEX user_name ON user(name);

-- Пароли пользователей
DROP TABLE IF EXISTS user_password;
CREATE TABLE user_password
(
    user_id VARCHAR PRIMARY KEY NOT NULL REFERENCES user(id) ON DELETE CASCADE,
    password VARCHAR NOT NULL
);
CREATE INDEX user_password_user_id ON user_password(user_id);


-- Подписки
DROP TABLE IF EXISTS subscription;
CREATE TABLE subscription
(
    publisher_id VARCHAR NOT NULL REFERENCES user(id) ON DELETE CASCADE,
    subscriber_id VARCHAR NOT NULL REFERENCES user(id) ON DELETE CASCADE CHECK(subscriber_id != publisher_id)
);
CREATE INDEX subscription_publisher_id ON subscription(publisher_id);
CREATE INDEX subscription_subscriber_id ON subscription(subscriber_id);


-- Сообщения пользователя
DROP TABLE IF EXISTS message;
CREATE TABLE message
(
    id VARCHAR PRIMARY KEY NOT NULL CHECK(id != '""' AND id != '"{00000000-0000-0000-0000-000000000000}"'),
    user_id VARCHAR NOT NULL REFERENCES user(id) ON DELETE CASCADE,

    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now') * 1000), -- In milliseconds.

    text VARCHAR
);
CREATE INDEX message_user_id ON message(user_id);
CREATE INDEX message_timestamp ON message(timestamp);


-- Комментарии к сообщению
DROP TABLE IF EXISTS comment;
CREATE TABLE comment
(
    id VARCHAR PRIMARY KEY NOT NULL CHECK(id != '""' AND id != '"{00000000-0000-0000-0000-000000000000}"'),
    message_id VARCHAR NOT NULL REFERENCES message(id) ON DELETE CASCADE,

    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now') * 1000), -- In milliseconds.

    text VARCHAR
);
CREATE INDEX comment_message_id ON comment(message_id);
CREATE INDEX comment_timestamp ON comment(timestamp);


-- Реакция на сообщение
DROP TABLE IF EXISTS reaction;
CREATE TABLE reaction
(
    id VARCHAR PRIMARY KEY NOT NULL CHECK(id != '""' AND id != '"{00000000-0000-0000-0000-000000000000}"'),
    message_id VARCHAR NOT NULL REFERENCES message(id) ON DELETE CASCADE,

    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now') * 1000), -- In milliseconds.

    weight INTEGER DEFAULT 1
);
CREATE INDEX reaction_message_id ON reaction(message_id);
CREATE INDEX reaction_timestamp ON reaction(timestamp);

COMMIT;
