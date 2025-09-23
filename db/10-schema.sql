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

    pkey VARCHAR UNIQUE NOT NULL, -- Public key.
    nick VARCHAR UNIQUE,
    icon VARCHAR,
    motto VARCHAR
);
CREATE UNIQUE INDEX user_pkey ON user(pkey);
CREATE UNIQUE INDEX user_nick ON user(nick);


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
DROP TABLE IF EXISTS post;
CREATE TABLE post
(
    id VARCHAR PRIMARY KEY NOT NULL CHECK(id != '""' AND id != '"{00000000-0000-0000-0000-000000000000}"'),
    user_id VARCHAR NOT NULL REFERENCES user(id) ON DELETE CASCADE,

    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now') * 1000), -- In milliseconds.

    text VARCHAR
);
CREATE INDEX post_user_id ON post(user_id);
CREATE INDEX post_timestamp ON post(timestamp);


-- Комментарии к сообщению
DROP TABLE IF EXISTS comment;
CREATE TABLE comment
(
    id VARCHAR PRIMARY KEY NOT NULL CHECK(id != '""' AND id != '"{00000000-0000-0000-0000-000000000000}"'),
    post_id VARCHAR NOT NULL REFERENCES post(id) ON DELETE CASCADE,

    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now') * 1000), -- In milliseconds.

    text VARCHAR
);
CREATE INDEX comment_post_id ON comment(post_id);
CREATE INDEX comment_timestamp ON comment(timestamp);


-- Реакция на сообщение
DROP TABLE IF EXISTS reaction;
CREATE TABLE reaction
(
    id VARCHAR PRIMARY KEY NOT NULL CHECK(id != '""' AND id != '"{00000000-0000-0000-0000-000000000000}"'),
    post_id VARCHAR NOT NULL REFERENCES post(id) ON DELETE CASCADE,

    timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now') * 1000), -- In milliseconds.

    weight INTEGER DEFAULT 1
);
CREATE INDEX reaction_post_id ON reaction(post_id);
CREATE INDEX reaction_timestamp ON reaction(timestamp);

COMMIT;
