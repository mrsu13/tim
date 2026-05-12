#include "tim_settings.h"

#include "tim_file_tools.h"
#include "tim_json.h"
#include "tim_string_tools.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include <cstdlib>
#include <fstream>
#include <system_error>


namespace
{

std::filesystem::path tim_home_dir()
{
    const char *home = std::getenv("HOME");
    if (home && *home)
        return std::filesystem::path(home) / ".tim";
    // $HOME пуст — крайне маловероятно на Linux, но если случилось,
    // приземляемся в текущий каталог, чтобы не писать в "/".
    return std::filesystem::current_path() / ".tim";
}

// Шаблон с комментариями, который пишется в config.json при первом запуске.
// %s заполняется абсолютным путём data_dir (по умолчанию $HOME/.tim).
const char *const CONFIG_TEMPLATE =
R"({
    // SSH-порт сервера TIM.
    "ssh_port": 2222,

    // URL MQTT-брокера. Для TLS используйте mqtts://, для plaintext — mqtt://.
    "mqtt_url": "mqtts://127.0.0.1:8883",

    // Рабочий каталог: сюда складываются БД (tim.db) и SSH host-key.
    "data_dir": "%s",

    // Язык сообщений: "ru" или "en".
    "language": "ru"
}
)";

}


std::filesystem::path tim::settings::config_path()
{
    return tim_home_dir() / "config.json";
}

tim::settings tim::settings::load_or_create()
{
    tim::settings s;
    s.data_dir = tim_home_dir();

    const std::filesystem::path path = config_path();
    std::error_code ec;

    if (!std::filesystem::exists(path, ec))
    {
        // Первый запуск: создаём каталог и кладём туда шаблон.
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec)
        {
            TIM_TRACE(Warning,
                      "Failed to create config directory '%s': %s. Using defaults.",
                      path.parent_path().string().c_str(),
                      ec.message().c_str());
            return s;
        }
        const std::string body = tim::sprintf(CONFIG_TEMPLATE, s.data_dir.string().c_str());
        if (!tim::write_to_file(path, body))
            TIM_TRACE(Warning,
                      "Failed to write default config to '%s'. Using defaults.",
                      path.string().c_str());
        else
            TIM_TRACE(Info,
                      "Wrote default config to '%s'.",
                      path.string().c_str());
        return s;
    }

    // Файл есть — читаем и парсим с разрешёнными //- и /* */-комментариями.
    std::ifstream in(path);
    if (!in.is_open())
    {
        TIM_TRACE(Warning,
                  "Failed to open config '%s'. Using defaults.",
                  path.string().c_str());
        return s;
    }

    nlohmann::json j = nlohmann::json::parse(in,
                                             /*cb=*/nullptr,
                                             /*allow_exceptions=*/false,
                                             /*ignore_comments=*/true);
    if (j.is_discarded())
    {
        TIM_TRACE(Warning,
                  "Failed to parse config '%s'. Using defaults.",
                  path.string().c_str());
        return s;
    }

    // Каждое поле — необязательное; отсутствие в JSON оставляет default.
    if (j.contains("ssh_port"))
        s.ssh_port = j.value("ssh_port", s.ssh_port);
    if (j.contains("mqtt_url"))
        s.mqtt_url = j.value("mqtt_url", s.mqtt_url);
    if (j.contains("data_dir"))
        s.data_dir = j.value("data_dir", s.data_dir.string());
    if (j.contains("language"))
        s.language = j.value("language", s.language);

    return s;
}
