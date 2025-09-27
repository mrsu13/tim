BEGIN;

-- Глобальные настройки.

INSERT INTO configuration (name, value, title, read_only)
    VALUES ('"uuid.regexp"',
            '"[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}"',
            '"Regular expression to validate UUID"', 1);

INSERT INTO configuration (name, value, title, read_only)
    VALUES ('"nick.regexp"', '"[a-zA-Z]+[-a-zA-Z0-9_\\.\\@]*"',
            '"Regular expression to validate user nick name"', 1);

COMMIT;
