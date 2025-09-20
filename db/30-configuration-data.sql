BEGIN;

-- Глобальные настройки.

INSERT INTO configuration (name, value, title, read_only)
    VALUES ('"uuid.regexp"',
            '"[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}"',
            '"Regular expression to validate UUID"', 1);
INSERT INTO configuration (name, value, title, read_only)
    VALUES ('"uuid.regexp.hint"',
            '"Must be a valid UUID."',
            '"UUID validation hint"', 1);

INSERT INTO configuration (name, value, title, read_only)
    VALUES ('"nick_name.regexp"', '"[a-zA-Z]+[-a-zA-Z0-9_\\.\\@]*"',
            '"Regular expression to validate user nick name"', 1);
INSERT INTO configuration (name, value, title, read_only)
    VALUES ('"nick_name.regexp.hint"',
            '"Must be unique (case insensitively), and not empty. Must contain alphanumeric characters, dot, underscore, dash or at (@). Must start with a letter."',
            '"User nick name validation hint"', 1);

COMMIT;
