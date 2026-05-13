#include "tim_file_tools.h"

#include "tim_trace.h"
#include "tim_translator.h"

#include <cstdlib>
#include <cstring>
#include <fstream>


/**
 * Нормализует путь (expand "~"; lexically_normal) и опционально создаёт
 * каталог.
 *
 * \param path Исходный путь; может содержать "~" (раскрывается в $HOME).
 * \param f Стратегия создания родительских каталогов перед возвратом.
 * \return Нормализованный абсолютный путь; пустой при ошибке создания
 *         каталогов.
 */
std::filesystem::path tim::complete_path(const std::filesystem::path &path,
                                         tim::create_path f)
{
    if (path.empty())
        return {};

    const std::string normal = path.lexically_normal().string();
    std::string completed;
    for (const char c: normal)
        switch (c)
        {
            case '~':
            {
                const char *home = std::getenv("HOME");
                completed += (home && *home) ? home : "";
                break;
            }

            default:
                completed += c;
                break;
        }

    if (f != tim::create_path::none)
    {
        std::filesystem::path path_to_create(completed);
        if (f == tim::create_path::base)
            path_to_create = path_to_create.parent_path();
        if (!path_to_create.empty()
                && !std::filesystem::exists(path_to_create))
        {
            std::error_code ec;
            if (!std::filesystem::create_directories(path_to_create, ec)
                    || ec)
            {
                TIM_TRACE(error,
                          TIM_TR("Failed to create path '%s': %s"_en,
                                 "Ошибка при создании файлового пути '%s': %s"_ru),
                          path_to_create.string().c_str(),
                          ec.message().c_str());
                return {};
            }
        }
    }

    return completed;
}

/**
 * Читает файл целиком в строку.
 *
 * \param path Путь к файлу (поддерживается "~").
 * \param text Сюда записывается содержимое.
 * \return true при успехе.
 */
bool tim::read_file(const std::filesystem::path &path, std::string &text)
{
    const std::filesystem::path epath = tim::complete_path(path);

    std::ifstream is(epath, std::ios::binary | std::ios::ate);
    if (!is)
        return TIM_TRACE(error,
                         TIM_TR("Failed to open file '%s' for reading: %s"_en,
                                "Ошибка при открытии файла '%s' на чтение: %s"_ru),
                         epath.string().c_str(),
                         std::strerror(errno));

    const std::ifstream::pos_type size = is.tellg();
    text.resize(size, '\0'); // Размер строки под весь поток.
    is.seekg(0);
    if (!is.read(&text[0], size))
        return TIM_TRACE(error,
                         TIM_TR("Failed to read file '%s': %s"_en,
                                "Ошибка при чтении файла '%s': %s"_ru),
                         epath.string().c_str(),
                         std::strerror(errno));

    return true;
}

/**
 * Пишет строку в файл (текстовый или бинарный, операция не различает).
 *
 * \param path Путь к файлу; родительский каталог создаётся при необходимости.
 * \param text Содержимое для записи.
 * \param mode Append или Overwrite.
 * \return true при успехе.
 */
bool tim::write_to_file(const std::filesystem::path &path,
                        const std::string &text,
                        tim::file_write_mode mode)
{
    const std::filesystem::path epath = tim::complete_path(path, tim::create_path::base);

    TIM_TRACE(debug, "Writing to '%s' ...", epath.string().c_str());

    if (epath.empty())
        return TIM_TRACE(error, "%s",
                         TIM_TR("Empty file path."_en,
                                "Пустой путь к файлу."_ru));

    std::ofstream os;
    os.open(epath, mode == tim::file_write_mode::append
                        ? std::ios_base::app
                        : std::ios_base::out);
    if (!os)
        return TIM_TRACE(error,
                         TIM_TR("Failed to open file '%s' for writing: %s"_en,
                                "Ошибка при открытии файла '%s' на чтение: %s"_ru),
                         epath.string().c_str(),
                         std::strerror(errno));
    os << text;
    if (os.bad()
            || os.fail())
        return TIM_TRACE(error,
                         TIM_TR("Failed to write to file '%s': %s"_en,
                                "Ошибка записи в файл '%s': %s"_ru),
                         epath.string().c_str(),
                         std::strerror(errno));

    return true;
}
