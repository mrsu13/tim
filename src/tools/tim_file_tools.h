#pragma once

#include <filesystem>
#include <string>


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

}
