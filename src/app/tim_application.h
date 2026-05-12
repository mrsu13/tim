#pragma once

#include <filesystem>
#include <memory>
#include <string>


namespace tim
{

namespace p
{

struct application;

}

/**
 * Корневой класс приложения TIM.
 *
 * Загружает настройки, поднимает подсистемы (MQTT-клиент, SQLite,
 * SSH inetd, серверные сервисы post/user/reaction) и крутит
 * event-loop через exec(). Конструируется один раз в main().
 */
class application
{

public:

    /**
     * Создаёт приложение и инициализирует все подсистемы.
     *
     * \param config_path Путь к JSON-файлу настроек. Пусто = путь
     *                    по умолчанию (~/.tim/config.json). Используется
     *                    для пробрасывания аргумента --config из main().
     */
    explicit application(const std::string &config_path = {});

    /**
     * Сворачивает подсистемы в обратном порядке и освобождает ресурсы
     * libmongoose. Восстанавливает прежние signal-обработчики SIGINT/SIGTERM.
     */
    ~application();

    /** \return Имя приложения (например, "tim"). */
    static const std::string &name();

    /**
     * Выставляет имя приложения. Должно быть вызвано один раз до
     * конструирования application — используется для размещения данных.
     *
     * \param name Новое имя приложения.
     */
    static void set_name(const std::string &name);

    /** \return Имя организации (например, "mrsu"). */
    static const std::string &org_name();

    /**
     * Выставляет имя организации. Должно быть вызвано один раз до
     * конструирования application.
     *
     * \param name Новое имя организации.
     */
    static void set_org_name(const std::string &name);

    /**
     * \return Каталог, в котором живут БД, SSH host-key, история шелла
     *         и TLS-сертификаты. Выставляется в конструкторе application
     *         из tim::settings::load_or_create(); после этого доступен
     *         как глобальная константа времени жизни приложения.
     */
    static const std::filesystem::path &data_dir();

    /**
     * Один шаг event-loop без блокировки. Вызывается из DISPATCH-обработчика
     * LIL между Tcl-операторами, чтобы внешние подсистемы продолжали тикать
     * во время работы скрипта.
     */
    void dispatch();

    /**
     * Основной цикл приложения. Чередует mg_mgr_poll и ssh_inetd::dispatch
     * с квантом 50 мс. Возвращается, когда quit() выставил флаг выхода.
     */
    void exec();

    /**
     * Запрашивает выход из exec(); срабатывает на следующем кванте цикла.
     * Также используется обработчиком SIGINT/SIGTERM.
     */
    void quit();

private:

    /** PIMPL: всё внутреннее состояние (signals, mongoose, подсистемы). */
    std::unique_ptr<tim::p::application> _d;
};

}
