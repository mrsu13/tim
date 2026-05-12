#pragma once

#include "tim_language.h"

#include <cstdint>
#include <filesystem>
#include <string>


namespace tim
{

// Настройки TIM, читаемые из ~/.tim/config.json на старте.
//
// Поля инициализированы значениями по умолчанию, совпадающими с прежними
// compile-time константами; отсутствующее в JSON поле сохраняет default.
// Если файла нет, settings::load_or_create() создаёт ~/.tim/ и пишет туда
// шаблон с комментариями — операторская "установка" в один запуск.
//
// Только то, что реально меняется между развёртываниями: SSH-порт,
// URL MQTT-брокера, рабочий каталог, язык интерфейса. Внутренние имена
// файлов и тайминги БД остаются в tim_config.h.
struct settings
{
    std::uint16_t         ssh_port  = 2222;
    std::string           mqtt_url  = "mqtts://127.0.0.1:8883";
    std::filesystem::path data_dir;             // По умолчанию: $HOME/.tim
    tim::language         language  = tim::language::ru_RU;

    // Возвращает путь к файлу конфигурации (~/.tim/config.json).
    static std::filesystem::path config_path();

    // Загружает настройки из config_path(). Если файла нет — создаёт каталог
    // и пишет туда шаблон с комментариями; затем возвращает значения по
    // умолчанию. Если файл повреждён — логирует и возвращает значения по
    // умолчанию (не падает).
    static tim::settings load_or_create();
};

}
