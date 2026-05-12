#pragma once

#include "tim_language.h"

#include <cstdint>
#include <filesystem>
#include <string>


namespace tim
{

/**
 * Настройки TIM, читаемые из ~/.tim/config.json на старте.
 *
 * Поля инициализированы значениями по умолчанию, совпадающими с прежними
 * compile-time константами; отсутствующее в JSON поле сохраняет default.
 * Если файла нет, settings::load_or_create() создаёт ~/.tim/ и пишет туда
 * шаблон с комментариями — операторская "установка" в один запуск.
 *
 * Только то, что реально меняется между развёртываниями: SSH-порт,
 * URL MQTT-брокера, рабочий каталог, язык интерфейса. Внутренние имена
 * файлов и тайминги БД остаются в tim_config.h.
 */
struct settings
{
    /** Порт, на котором SSH-сервер TIM слушает входящие соединения. */
    std::uint16_t         ssh_port  = 2222;
    /** URL MQTT-брокера; mqtts:// — TLS, mqtt:// — без шифрования. */
    std::string           mqtt_url  = "mqtts://127.0.0.1:8883";
    /** Рабочий каталог: БД, SSH host-key, история шелла, TLS-сертификаты.
     *  По умолчанию $HOME/.tim. */
    std::filesystem::path data_dir;
    /** Язык, на котором TIM_TR выдаёт пользовательские сообщения. */
    tim::language         language  = tim::language::ru_ru;

    /**
     * Возвращает путь по умолчанию к файлу конфигурации (~/.tim/config.json).
     *
     * \return Абсолютный путь, построенный от $HOME.
     */
    static std::filesystem::path default_config_path();

    /**
     * Загружает настройки из JSON-файла.
     *
     * Если \a path пустой, читается default_config_path(); файла нет —
     * создаётся ~/.tim/ и туда пишется шаблон с комментариями.
     * Если \a path задан, но файла нет — логируется warning и возвращаются
     * значения по умолчанию (шаблон не пишется: оператор сам выбрал путь).
     * При ошибке парсинга также возвращаются значения по умолчанию,
     * процесс не падает.
     *
     * \param path Путь к JSON-файлу или пустая строка для default.
     * \return Заполненная структура tim::settings.
     */
    static tim::settings load_or_create(const std::string &path = {});
};

}
