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
    VALUES ('"email.regexp"',
            '"[A-Za-z0-9\\._%-]+@[A-Za-z0-9\\.-]+\\.[A-Za-z]{2,}"',
            '"Regular expression to validate an e-mail address"', 1);
INSERT INTO configuration (name, value, title, read_only)
    VALUES ('"email.regexp.hint"',
            '"Must be a valid e-mail address."',
            '"E-mail address validation hint"', 1);

INSERT INTO configuration (name, value, title, read_only)
    VALUES ('"asset.name.regexp"', '"[a-zA-Z]+[-a-zA-Z0-9_\\.\\@]*"',
            '"Regular expression to validate asset name"', 1);
INSERT INTO configuration (name, value, title, read_only)
    VALUES ('"asset.name.regexp.hint"',
            '"Must be unique (case insensitively), and not empty. Must contain alphanumeric characters, dot, underscore, dash or at (@). Must start with a letter."',
            '"Asset name validation hint"', 1);
INSERT INTO configuration (name, value, title, read_only)
    VALUES ('"user.password.regexp"', '"(?=.*\\d)(?=.*[a-z])(?=.*[A-Z])(?!.*\\s).{6,32}"',
            '"Regular expression to validate user''s password"', 1);
INSERT INTO configuration (name, value, title, read_only)
    VALUES ('"user.password.regexp.hint"',
            '"Must be at least six, at most 32 characters long. Must contain at least one upper case letter, one lower case letter, one digit, no spaces."',
            '"User''s password validation hint"', 1);

COMMIT;
