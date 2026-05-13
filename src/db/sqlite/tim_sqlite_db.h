#pragma once

#include <filesystem>
#include <memory>
#include <string>


struct sqlite3;

namespace tim
{

namespace p
{

struct sqlite_db;

}

/**
 * Тонкая обёртка вокруг sqlite3 для одного файла БД.
 *
 * Управляет жизненным циклом sqlite3*, поддерживает refcount-aware
 * begin/commit (вложенные begin() инкрементируют счётчик, реальный
 * BEGIN/COMMIT выполняются только на внешнем уровне).
 */
class sqlite_db
{

public:

    /** Конструирует объект без открытой БД. */
    sqlite_db();

    /** Деструктор. Закрывает открытую БД через close(). */
    virtual ~sqlite_db();

    /**
     * Открывает указанный файл БД.
     *
     * \param path Путь к файлу. ~ и относительные пути нормализуются
     *             через tim::complete_path(); родительский каталог
     *             создаётся при отсутствии.
     * \return true при успехе, false при ошибке (залогирована).
     */
    bool open(const std::filesystem::path &path);

    /** \return true, если БД сейчас открыта. */
    bool is_open() const;

    /**
     * Делает sqlite "WAL checkpoint", сбрасывая страницы на диск.
     *
     * \return true при успехе.
     */
    bool flush();

    /** Закрывает БД; идемпотентно. */
    void close();

    /** \return Абсолютный путь к открытой БД, или пустой путь. */
    const std::filesystem::path &path() const;

#ifdef TIM_SQLITE_ENCRYPTION_ENABLED
    /**
     * Устанавливает ключ шифрования. Должен вызываться до первой
     * операции над БД.
     *
     * \param key Сырой ключ (бинарные данные либо PEM-строка).
     * \return true при успехе.
     */
    bool set_key(const std::string &key);

    /**
     * Меняет ключ шифрования открытой БД.
     *
     * \param key Новый ключ.
     * \return true при успехе.
     */
    bool rekey(const std::string &key);

    /**
     * Отключает шифрование БД (rekey на пустую строку).
     *
     * \return true при успехе.
     */
    bool clear_key();
#endif

    /**
     * Читает PRAGMA user_version, не открывая БД полностью.
     *
     * \param path Путь к файлу БД.
     * \param version Сюда записывается значение.
     * \return true при успехе.
     */
    static bool get_version(const std::filesystem::path &path, std::uint32_t &version);

    /**
     * Читает PRAGMA user_version открытой БД.
     *
     * \param version Сюда записывается значение.
     * \return true при успехе.
     */
    bool get_version(std::uint32_t &version) const;

    /**
     * Записывает PRAGMA user_version в открытую БД.
     *
     * \param version Новое значение.
     * \return true при успехе.
     */
    bool set_version(std::uint32_t version);

    /**
     * Удаляет файл БД и открывает его заново. Не создаёт схему —
     * это ответственность db/create-db.sh. virtual: наследник может
     * добавить логику пост-инициализации.
     *
     * \return true при успехе.
     */
    virtual bool recreate();

    /**
     * Выполняет SQL-стейтмент(ы), без bind.
     *
     * \param sql Любая корректная SQL-строка.
     * \return true при успехе, false при ошибке (залогирована).
     */
    bool exec(const std::string &sql);

    /**
     * Загружает SQL-файл с диска и выполняет его как один скрипт.
     *
     * \param path Путь к .sql-файлу.
     * \return true при успехе.
     */
    bool exec_file(const std::filesystem::path &path);

    /**
     * Оборачивает SQL в BEGIN...COMMIT транзакцию.
     *
     * \param sql SQL-скрипт.
     * \return true при успехе; при ошибке откатывает и возвращает false.
     */
    bool transaction(const std::string &sql);

    /**
     * Открывает (вложенную) транзакцию через рефкаунт.
     *
     * \return true при успехе; на внешнем уровне выполняется BEGIN.
     */
    bool begin();

    /**
     * Открывает named SAVEPOINT.
     *
     * \param save_point Имя точки сохранения.
     * \return true при успехе.
     */
    bool begin(const std::string &save_point);

    /** \return true, если в БД сейчас открыта (вложенная) транзакция. */
    bool is_transaction_active() const;

    /**
     * Фиксирует уровень вложенности; внешний уровень делает COMMIT.
     *
     * \return true при успехе.
     */
    bool commit();

    /**
     * Освобождает named SAVEPOINT.
     *
     * \param save_point Имя точки сохранения.
     * \return true при успехе.
     */
    bool commit(const std::string &save_point);

    /**
     * ROLLBACK всей транзакции, сбрасывает счётчик вложенности в 0.
     * Внимание: не refcount-aware — внутренний rollback отменит и
     * внешнюю транзакцию тоже.
     *
     * \return true при успехе.
     */
    bool rollback();

    /**
     * Откат к named SAVEPOINT.
     *
     * \param save_point Имя точки сохранения.
     * \return true при успехе.
     */
    bool rollback(const std::string &save_point);

    /**
     * Выполняет VACUUM, дефрагментирует файл БД.
     *
     * \return true при успехе.
     */
    bool vacuum();

    /**
     * \return Число строк, изменённых последним INSERT/UPDATE/DELETE
     *         (sqlite3_changes).
     */
    int change_count() const;

    /**
     * Сырой указатель sqlite3* — для редких операций, не покрытых
     * обёрткой (например, sqlite3_backup_init).
     *
     * \return Указатель на sqlite3*, либо nullptr если БД не открыта.
     */
    sqlite3 *sqlite() const;

private:

    /** PIMPL: путь к файлу, sqlite3*, refcount транзакций. */
    std::unique_ptr<tim::p::sqlite_db> _d;
};

}
