#pragma once

#include "tim_mqtt_subscription.h"
#include "tim_mqtt_topic.h"
#include "tim_profile_cache.h"
#include "tim_signal_connection.h"
#include "tim_user.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>


namespace tim
{

class mqtt_client;
class prompt_service;
class prompt_shell;
class sqlite_db;
class ssh_terminal_protocol;
class tcl;
class vt;

namespace p
{

/**
 * Внутреннее состояние tim::prompt_service (PIMPL).
 *
 * Владеет цепочкой "ssh-протокол → vt-терминал → tcl + prompt_shell",
 * кэшем профилей и набором MQTT-подписок текущей сессии.
 */
struct prompt_service
{
    /**
     * \param q Указатель на внешний prompt_service.
     * \param mqtt MQTT-клиент приложения.
     * \param db Подключение к БД.
     * \param self_id UUID пользователя этой сессии (для profile_cache).
     */
    prompt_service(tim::prompt_service *q,
                   tim::mqtt_client &mqtt,
                   tim::sqlite_db &db,
                   const tim::uuid &self_id)
        : _q(q)
        , _mqtt(mqtt)
        , _db(db)
        , _profiles(mqtt, db, self_id)
    {
        assert(_q);
    }

    /** Подтягивает текущие подписки пользователя из БД в локальный кэш. */
    void load_subscriptions();

    /**
     * Печатает в терминал последние N сообщений из БД, чтобы при входе
     * в чат пользователь сразу видел контекст разговора.
     */
    void load_post_history();

    /**
     * Регистрирует MQTT-подписки сессии (POST, REACT_EVENT, user/subscribe|
     * unsubscribe/&lt;self&gt;, session/notice/&lt;self&gt;). Вызывается однократно
     * из конструктора; оформление у брокера mqtt_client выполняет сам.
     */
    void subscribe();

    /**
     * Обработчик восстановления соединения с брокером: сбрасывает кэши,
     * перечитывает подписки из БД, повторно объявляет присутствие.
     */
    void on_broker_connected();

    /**
     * Выводит служебное уведомление сессии поверх строки ввода.
     *
     * \param text Текст уведомления.
     */
    void notice(const std::string &text);

    /**
     * Обработчик байтов, поступивших из SSH-протокола. Передаёт их
     * командному интерпретатору; закрывает соединение при ошибке записи.
     *
     * \param data Сырые байты.
     * \param size Размер.
     */
    void on_data_ready(const char *data, std::size_t size);

    /**
     * Обработчик входящего post/&lt;author&gt;/&lt;post-id&gt;. Разбирает UUID,
     * сохраняет пост как последний увиденный и выводит карточку сообщения.
     *
     * \param topic Полный топик публикации.
     * \param data Текст сообщения.
     * \param size Размер.
     */
    void on_post(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    /**
     * Обработчик react_event/&lt;post&gt;/&lt;reactor&gt;. Выводит уведомление
     * "X отреагировал(а) на ваш пост" в чат.
     *
     * \param topic Топик события реакции.
     * \param data Payload (целочисленный вес).
     * \param size Размер.
     */
    void on_react_event(const tim::mqtt_topic &topic, const char *data, std::size_t size);

    void render_post(const tim::uuid &publisher_id, std::string_view text);

    /** Внешний prompt_service (владелец). */
    tim::prompt_service *const _q;
    /** MQTT-клиент сессии. */
    tim::mqtt_client &_mqtt;
    /** Подключение к БД. */
    tim::sqlite_db &_db;
    /** Кэш профилей (свой + чужие) + подписки на user/setnick|seticon|setmotto. */
    tim::profile_cache                          _profiles;

    /** SSH-терминальный протокол (источник байт). */
    std::unique_ptr<tim::ssh_terminal_protocol> _proto;
    /** VT-эмулятор. */
    std::unique_ptr<tim::vt>                    _terminal;
    /** Tcl-интерпретатор. */
    std::unique_ptr<tim::tcl>                   _tcl;
    /** Командный интерпретатор чата (prompt_shell : vt_shell). */
    std::unique_ptr<tim::prompt_shell>          _shell;
    /** UUID последнего увиденного сообщения (цель /react). */
    tim::uuid                                   _last_seen_post;
    /** UUID автора последнего увиденного сообщения. */
    tim::uuid                                   _last_seen_post_author;

    // Подписки и сигнал-соединения объявлены ПОСЛЕ полей-зависимостей
    // (_mqtt, _db, _profiles, _proto, _terminal, _tcl, _shell и т.д.)
    // намеренно: при разрушении они уничтожаются ПЕРВЫМИ, и их
    // обработчики обращаются к этим полям. Перестановка проверяется
    // static_assert ниже.

    /** Сигнальное соединение: data_ready от SSH-протокола. */
    tim::signal_connection                      _on_data_ready;
    /** Сигнальное соединение: posted от prompt_shell. */
    tim::signal_connection                      _on_posted;
    /** Сигнальное соединение: mqtt.connected для обновления кэшей домена. */
    tim::signal_connection                      _on_connected;
    /** Подписка на POST_FILTER. */
    tim::mqtt_subscription                      _sub_post;
    /** Подписка на REACT_EVENT_FILTER. */
    tim::mqtt_subscription                      _sub_react_event;
    /**
     * Свои события subscribe — чтобы поддерживать актуальным локальный
     * кэш подписок без дополнительного запроса в БД.
     */
    tim::mqtt_subscription                      _sub_self_subscribe;
    /** Свои события unsubscribe — см. _sub_self_subscribe. */
    tim::mqtt_subscription                      _sub_self_unsubscribe;
    /** Личные уведомления от сервера (например, "ник занят"). */
    tim::mqtt_subscription                      _sub_notice;

    /**
     * UUID пользователей, на которых подписан текущий пользователь.
     * Используется в on_post для пометки сообщений (звёздочка перед ником).
     */
    std::unordered_set<tim::uuid>               _subscriptions;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
static_assert(offsetof(prompt_service, _on_data_ready) > offsetof(prompt_service, _last_seen_post_author),
              "MQTT-подписки и сигнал-соединения должны быть объявлены ПОСЛЕ "
              "полей-зависимостей — они уничтожаются первыми, и их обработчики "
              "читают эти поля.");
#pragma GCC diagnostic pop

}

}
