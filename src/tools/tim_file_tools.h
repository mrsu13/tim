#pragma once

#include "tim_byte_vector.h"
#include "tim_json.h"

#include <filesystem>
#include <functional>
#include <regex>
#include <vector>


namespace tim
{

/**
 * Стратегия создания родительских каталогов перед записью файла.
 */
enum class create_path
{
    none,  ///< Не создавать ничего; вызов upfront ничего не проверяет.
    base,  ///< Создать только каталог, в котором будет лежать сам файл.
    full   ///< Создать весь указанный путь как каталог.
};

/**
 * Нормализует путь (expand "~"; lexically_normal) и опционально создаёт
 * каталог.
 *
 * \param path Исходный путь; может содержать "~" (раскрывается в $HOME).
 * \param f Стратегия создания родительских каталогов перед возвратом.
 * \return Нормализованный абсолютный путь; пустой при ошибке создания
 *         каталогов.
 */
std::filesystem::path complete_path(const std::filesystem::path &path,
                                    tim::create_path f = tim::create_path::none);

/**
 * Читает файл целиком в строку.
 *
 * \param path Путь к файлу (поддерживается "~").
 * \param text Сюда записывается содержимое.
 * \return true при успехе.
 */
bool read_file(const std::filesystem::path &path, std::string &text);

/**
 * Читает JSON-файл (с поддержкой `//`- и `/`*`*`/`-комментариев).
 *
 * \param path Путь к .json-файлу.
 * \param j Сюда записывается распарсенный объект.
 * \return true при успехе; false при IO-ошибке или ошибке парсинга
 *         (с подробностями в логе).
 */
bool read_json(const std::filesystem::path &path, nlohmann::json &j);

/**
 * Режим записи файла.
 */
enum class file_write_mode
{
    append,    ///< Дописать в конец файла; создать, если не существует.
    overwrite  ///< Затереть содержимое (по умолчанию).
};

/**
 * Пишет строку в файл (текстовый или бинарный, операция не различает).
 *
 * \param path Путь к файлу; родительский каталог создаётся при необходимости.
 * \param text Содержимое для записи.
 * \param mode Append или Overwrite.
 * \return true при успехе.
 */
bool write_to_file(const std::filesystem::path &path,
                   const std::string &text,
                   tim::file_write_mode mode = tim::file_write_mode::overwrite);

/**
 * Сериализует JSON и пишет в файл.
 *
 * \param path Путь к файлу.
 * \param j Объект для сериализации.
 * \param indent Размер отступа в JSON; -1 — без переносов строк.
 * \return true при успехе.
 */
bool write_json(const std::filesystem::path &path, const nlohmann::json &j, int indent = -1);

/**
 * Читает файл по кускам и зовёт callback на каждом куске. Удобно для
 * больших файлов, чтобы не держать всё в памяти.
 *
 * \param path Путь к файлу.
 * \param fn Колбэк, получающий кусок; возвращает false — прекратить чтение.
 * \param chunk_size Желаемый размер куска в байтах.
 * \return Общее число прочитанных байт; 0 при ошибке.
 */
std::size_t process_file(const std::filesystem::path &path,
                         std::function<bool(const tim::byte_vector &)> fn,
                         std::size_t chunk_size = 10240);

/**
 * \param path Путь, проверяемый на существование (с раскрытием ~).
 * \return true, если файл или каталог существует.
 */
bool path_exists(const std::filesystem::path &path);

/**
 * \param path Строка, проверяемая на форму "scheme://host[:port]/...".
 * \return true, если выглядит как сетевой URL.
 */
bool is_network_url(const std::filesystem::path &path);

/**
 * \param path Путь, проверяемый на синтаксическую валидность (без IO).
 * \return true, если путь синтаксически корректен на текущей платформе.
 */
bool is_valid_path(const std::filesystem::path &path);

/**
 * Перечисляет файлы непосредственно в каталоге \a root (без рекурсии).
 *
 * \param root Каталог для обхода.
 * \return Список путей.
 */
std::vector<std::filesystem::path> files(const std::filesystem::path &root);

/**
 * Перечисляет файлы в \a root, имена которых соответствуют регулярному
 * выражению \a re.
 *
 * \param root Каталог для обхода.
 * \param re Регулярное выражение; применяется к имени файла (не пути).
 * \return Список путей, прошедших фильтр.
 */
std::vector<std::filesystem::path> files(const std::filesystem::path &root, const std::regex &re);

}
