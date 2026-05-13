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

std::filesystem::path complete_path(const std::filesystem::path &path,
                                    tim::create_path f = tim::create_path::none);

bool read_file(const std::filesystem::path &path, std::string &text);

/**
 * Режим записи файла.
 */
enum class file_write_mode
{
    append,    ///< Дописать в конец файла; создать, если не существует.
    overwrite  ///< Затереть содержимое (по умолчанию).
};

bool write_to_file(const std::filesystem::path &path,
                   const std::string &text,
                   tim::file_write_mode mode = tim::file_write_mode::overwrite);

}
