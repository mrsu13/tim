#pragma once

#include <chrono>
#include <cstdint>


namespace tim
{

/** Префикс TCL-команд, набираемых в чате: например, /post, /react. */
static const char COMMAND_PREFIX = '/';
/** Имя приложения, используется для логов и баннеров. */
static const char APP_NAME[] = "tim";
/** Имя организации; исторически хранилось вместе с APP_NAME. */
static const char ORG_NAME[] = "mrsu";
/** Имя файла истории шелла внутри data_dir. */
static const char HISTORY_FNAME[] = "history.txt";
/** Имя файла SSH host-key. Лежит в data_dir/SSH_DATA_SUBDIR. */
static const char SSH_HOST_KEY_FNAME[] = "host_ed25519";
/** Подкаталог в data_dir, в котором хранится SSH host-key. */
static const char SSH_DATA_SUBDIR[] = "ssh";

/**
 * SQLite-настройки.
 */
/** Таймаут на ожидание занятой БД между попытками. */
static const std::chrono::microseconds DB_BUSY_TIMEOUT(100000);
/** Количество попыток выполнения SQL при SQLITE_BUSY. */
static const std::size_t DB_BUSY_TRIES = 5;
/** Имя файла основной БД внутри data_dir. */
static const char DB_FILE_NAME[] = "tim.db";
/**
 * Ожидаемая бинарником версия схемы БД. Должна соответствовать содержимому
 * файла DB_SCHEMA_VERSION в корне репо (на момент сборки). При несовпадении
 * с PRAGMA user_version из открытой БД приложение закрывает БД и продолжает
 * работу без сохранения данных — оператору предложено перезапустить
 * db/create-db.sh.
 */
static const std::uint32_t EXPECTED_DB_SCHEMA_VERSION = 2;

/**
 * Максимальная глубина рекурсивного вызова ssh_inetd::dispatch.
 * Защищает стек от бесконечной рекурсии, если Tcl-скрипт неправильно
 * пользуется DISPATCH-обработчиком (например, бесконечно дёргает dispatch
 * из обработчика события). При достижении лимита вложенный опрос
 * пропускается с предупреждением в лог.
 */
static const std::size_t MAX_DISPATCH_DEPTH = 8;
}
