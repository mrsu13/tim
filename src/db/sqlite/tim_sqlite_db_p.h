#pragma once

#include <tim_sqlite_db.h>

#include <functional>


namespace tim::p
{

/**
 * Внутреннее состояние tim::sqlite_db (PIMPL).
 *
 * Хранит сам sqlite3*, путь к файлу и счётчик вложенности транзакций
 * для рефкаунт-aware begin/commit.
 */
struct sqlite_db
{
    /**
     * Тип владельца sqlite3*: unique_ptr с пользовательским deleter
     * (на close_db) — чтобы автоматически закрывать соединение при
     * уничтожении.
     */
    using db_ptr = std::unique_ptr<sqlite3, std::function<void(sqlite3 *)>>;

    /**
     * Открывает sqlite3-соединение к файлу.
     *
     * \param path Путь к файлу.
     * \return RAII-указатель; nullptr при ошибке (залогирована).
     */
    static db_ptr open_db(const std::filesystem::path &path);

    /**
     * Закрывает sqlite3-соединение через sqlite3_close.
     *
     * \param db Указатель на соединение.
     * \return true при успехе.
     */
    static bool close_db(sqlite3 *db);

    /**
     * Обработчик для sqlite3_trace_v2: пишет SQL-стейтменты в лог.
     *
     * \param event Тип события sqlite (SQLITE_TRACE_*).
     * \param self Указатель на tim::sqlite_db; не используется.
     * \param p Контекст события (sqlite3_stmt* для STMT/PROFILE).
     * \param x Дополнительные данные (SQL-текст или продолжительность).
     * \return 0 (sqlite ожидает 0 в норме).
     */
    static int trace(unsigned event, void *self, void *p, void *x);

    /**
     * Обработчик для sqlite3_progress_handler — даёт возможность прерывать
     * долгие запросы.
     *
     * \param self Указатель на tim::sqlite_db; не используется.
     * \return 0 чтобы продолжать; ненулевое — прервать.
     */
    static int progress(void *self);

    /** Абсолютный путь к открытому файлу БД. */
    std::filesystem::path _path;

    /** Текущее sqlite3-соединение (или пустой ptr). */
    db_ptr _db;

    /** Счётчик вложенности begin()/commit() для refcount-aware транзакций. */
    int _transaction_count = 0;
};

}
