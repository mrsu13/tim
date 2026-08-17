#pragma once

#include "mongoose.h"

#include <atomic>
#include <filesystem>
#include <memory>

#ifdef TIM_OS_LINUX
#   include <signal.h>
#endif

namespace tim
{

class mqtt_client;
class post_service;
class reaction_service;
class ssh_inetd;
class user_service;
class sqlite_db;

namespace p
{

/**
 * Внутреннее состояние tim::application (PIMPL).
 *
 * Хранит mongoose-менеджер, владеет всеми подсистемами через unique_ptr
 * и публикует static-аксессоры к глобальным константам приложения
 * (имя, организация, рабочий каталог, экземпляр для signal-обработчика).
 */
struct application
{
    /**
     * Статический указатель на единственный существующий экземпляр application.
     * Используется signal_handler(), который не может принимать аргументы.
     *
     * \return Ссылка на mutable указатель. Конструктор application выставляет
     *         её в this; деструктор не сбрасывает (instance() читается
     *         только в обработчике сигналов).
     */
    static tim::application *&instance()
    {
        static tim::application *app = nullptr;
        return app;
    };

    /**
     * Имя приложения, выставляется через application::set_name().
     *
     * \return Ссылка на mutable строку. Хранится как static — нужна на
     *         всё время жизни процесса, в т.ч. в обработчиках сигналов.
     */
    static std::string &name()
    {
        static std::string _name;
        return _name;
    }

    /**
     * Имя организации, выставляется через application::set_org_name().
     *
     * \return Ссылка на mutable строку (см. name()).
     */
    static std::string &org_name()
    {
        static std::string _name;
        return _name;
    }

    /**
     * Рабочий каталог приложения. Выставляется в конструкторе из
     * tim::settings::load_or_create(); далее доступен везде.
     *
     * \return Ссылка на mutable путь (см. name()).
     */
    static std::filesystem::path &data_dir()
    {
        static std::filesystem::path _data_dir;
        return _data_dir;
    }

#ifdef TIM_OS_LINUX

    /**
     * Обработчик SIGINT/SIGTERM. Через instance() запрашивает выход
     * у существующего application. Async-signal-safe: устанавливает
     * только атомарные флаги (_quit и номер сигнала) — никакого
     * вывода в журнал и других небезопасных вызовов; сообщение
     * о причине выхода печатает exec() после завершения цикла.
     *
     * \param sig_num Номер пришедшего сигнала.
     */
    static void signal_handler(int sig_num);

    /**
     * Номер сигнала, вызвавшего выход; 0 — выход запрошен программно.
     *
     * \return Ссылка на атомарный номер сигнала. Хранится как static
     *         по той же причине, что и instance(): обработчик сигналов
     *         не имеет доступа к состоянию экземпляра.
     */
    static std::atomic<int> &exit_signal()
    {
        static std::atomic<int> sig{0};
        return sig;
    }

    /** Сохранённый обработчик SIGINT для восстановления в деструкторе. */
    struct sigaction _old_sig_int;
    /** Сохранённый обработчик SIGTERM для восстановления в деструкторе. */
    struct sigaction _old_sig_term;
#endif
    /**
     * Флаг выхода из exec(). Выставляется quit() — в том числе из
     * обработчика сигналов ОС, поэтому атомарный.
     */
    std::atomic<bool> _quit{false};

    /** Mongoose-менеджер: shared между mqtt_client и таймерами. */
    struct mg_mgr _mg;
    /** MQTT-клиент. Уничтожается ПЕРЕД mg_mgr_free. */
    std::unique_ptr<tim::mqtt_client> _mqtt;
    /** Подключение к SQLite. */
    std::unique_ptr<tim::sqlite_db> _db;
    /** SSH-inetd: слушает входящие SSH-соединения. */
    std::unique_ptr<tim::ssh_inetd> _ssh_inetd;
    /** Сервис постов: пишет входящие посты в БД. */
    std::unique_ptr<tim::post_service> _post_service;
    /** Сервис пользователей: ник/иконка/девиз/подписки/публичный ключ. */
    std::unique_ptr<tim::user_service> _user_service;
    /** Сервис реакций: пишет реакции и публикует react_event. */
    std::unique_ptr<tim::reaction_service> _reaction_service;
};

}

}
