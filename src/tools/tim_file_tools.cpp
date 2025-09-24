#include "tim_file_tools.h"

#include "tim_application.h"
#include "tim_string_tools.h"
#include "tim_trace.h"
#include "tim_translator.h"

#include <fstream>


#ifndef TIM_OS_WINDOWS

std::filesystem::path tim::standard_location(tim::filesystem_location location)
{
    switch (location)
    {
        case tim::filesystem_location::AppConfig:
            return std::filesystem::path(std::getenv("HOME"))
                            / ".config"
                            / tim::to_lower(tim::application::org_name())
                            / tim::to_lower(tim::application::name());
        case tim::filesystem_location::AppData:
        case tim::filesystem_location::AppLocalData:
            return std::filesystem::path(std::getenv("HOME"))
                            / ".local/share"
                            / tim::to_lower(tim::application::org_name())
                            / tim::to_lower(tim::application::name());
        case tim::filesystem_location::Current:
            return std::filesystem::current_path();
        case tim::filesystem_location::Home:
            return std::getenv("HOME");
        case tim::filesystem_location::Desktop:
            return std::filesystem::path(std::getenv("HOME")) / "Desktop";
        case tim::filesystem_location::Documents:
            return std::filesystem::path(std::getenv("HOME")) / "Documents";
        case tim::filesystem_location::Download:
            return std::filesystem::path(std::getenv("HOME")) / "Downloads";
        case tim::filesystem_location::Pictures:
            return std::filesystem::path(std::getenv("HOME")) / "Pictures";
        case tim::filesystem_location::Music:
            return std::filesystem::path(std::getenv("HOME")) / "Music";
        case tim::filesystem_location::Videos:
            return std::filesystem::path(std::getenv("HOME")) / "Videos";
        case tim::filesystem_location::Temp:
            return std::filesystem::temp_directory_path();
    }
    return std::filesystem::path();
}

#else

std::filesystem::path tim::standard_location(tim::filesystem_location location)
{
    switch (location)
    {
        case tim::filesystem_location::AppConfig:
        case tim::filesystem_location::AppData:
            return std::filesystem::path(std::getenv("APPDATA"))
                                / tim::to_lower(tim::application::org_name())
                                / tim::to_lower(tim::application::name());
        case tim::filesystem_location::AppLocalData:
            return std::filesystem::path(std::getenv("LOCALAPPDATA"))
                                / tim::to_lower(tim::application::org_name())
                                / tim::to_lower(tim::application::name());
        case tim::filesystem_location::Current:
            return std::filesystem::current_path();
        case tim::filesystem_location::Home:
            return std::getenv("USERPROFILE");
        case tim::filesystem_location::Desktop:
            return std::filesystem::path(std::getenv("USERPROFILE")) / "Desktop";
        case tim::filesystem_location::Documents:
            return std::filesystem::path(std::getenv("USERPROFILE")) / "Documents";
        case tim::filesystem_location::Download:
            return std::filesystem::path(std::getenv("USERPROFILE")) / "Downloads";
        case tim::filesystem_location::Pictures:
            return std::filesystem::path(std::getenv("USERPROFILE")) / "Pictures";
        case tim::filesystem_location::Music:
            return std::filesystem::path(std::getenv("USERPROFILE")) / "Music";
        case tim::filesystem_location::Videos:
            return std::filesystem::path(std::getenv("USERPROFILE")) / "Videos";
        case tim::filesystem_location::Temp:
            return std::filesystem::temp_directory_path();
    }
    return std::filesystem::path();
}

#endif

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
                completed += tim::standard_location(tim::filesystem_location::Home).string();
                break;

            default:
                completed += c;
                break;
        }

    if (f != tim::create_path::None)
    {
        std::filesystem::path path_to_create(completed);
        if (f == tim::create_path::Base)
            path_to_create = path_to_create.parent_path();
        if (!path_to_create.empty()
                && !std::filesystem::exists(path_to_create))
        {
            std::error_code ec;
            if (!std::filesystem::create_directories(path_to_create, ec)
                    || ec)
            {
                TIM_TRACE(Error,
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

bool tim::read_file(const std::filesystem::path &path, std::string &text)
{
    const std::filesystem::path epath = tim::complete_path(path);

    std::ifstream is(epath, std::ios::binary | std::ios::ate);
    if (!is)
        return TIM_TRACE(Error,
                         TIM_TR("Failed to open file '%s' for reading: %s"_en,
                                "Ошибка при открытии файла '%s' на чтение: %s"_ru),
                         epath.string().c_str(),
                         std::strerror(errno));

    const std::ifstream::pos_type size = is.tellg();
    text.resize(size, '\0'); // Construct string to stream size.
    is.seekg(0);
    if (!is.read(&text[0], size))
        return TIM_TRACE(Error,
                         TIM_TR("Failed to read file '%s': %s"_en,
                                "Ошибка при чтении файла '%s': %s"_ru),
                         epath.string().c_str(),
                         std::strerror(errno));

    return true;
}

bool tim::read_json(const std::filesystem::path &path, nlohmann::json &j)
{
    const std::filesystem::path epath = tim::complete_path(path);

    if (epath.empty())
        return TIM_TRACE(Error,
                         "%s",
                         TIM_TR("Empty file path."_en,
                                "Пустой путь к файлу."_ru));

    {
        std::string js;
        if (!tim::read_file(epath, js))
            return false;

        j = nlohmann::json::parse(js.c_str(),
                                  js.c_str() + js.size(),
                                  nullptr, // Parser call-back.
                                  false, // Allow exceptions.
                                  true); // Ignore comments.
        if (j.is_discarded())
            return TIM_TRACE(Error,
                             TIM_TR("Failed to parse JSON file '%s' at position %s-%s."_en,
                                    "Ошибка при разборе JSON-файла '%s' в позиции %s-%s."_ru),
                             epath.string().c_str(),
                             j.start_pos() == std::string::npos
                                 ? tim::na()
                                 : std::to_string(j.start_pos()).c_str(),
                             j.end_pos() == std::string::npos
                                 ? tim::na()
                                 : std::to_string(j.end_pos()).c_str());
    }

    return true;
}

bool tim::write_to_file(const std::filesystem::path &path,
                       const std::string &text,
                       tim::file_write_mode mode)
{
    const std::filesystem::path epath = tim::complete_path(path, tim::create_path::Base);

    TIM_TRACE(Debug, "Writing to '%s' ...", epath.string().c_str());

    if (epath.empty())
        return TIM_TRACE(Error, "%s",
                        TIM_TR("Empty file path."_en,
                              "Пустой путь к файлу."_ru));

    std::ofstream os;
    os.open(epath, mode == tim::file_write_mode::Append
                        ? std::ios_base::app
                        : std::ios_base::out);
    if (!os)
        return TIM_TRACE(Error,
                         TIM_TR("Failed to open file '%s' for writing: %s"_en,
                                "Ошибка при открытии файла '%s' на чтение: %s"_ru),
                         epath.string().c_str(),
                         std::strerror(errno));
    os << text;
    if (os.bad()
            || os.fail())
        return TIM_TRACE(Error,
                         TIM_TR("Failed to write to file '%s': %s"_en,
                                "Ошибка записи в файл '%s': %s"_ru),
                         epath.string().c_str(),
                         std::strerror(errno));

    return true;
}

bool tim::write_json(const std::filesystem::path &path, const nlohmann::json &j, int indent)
{
    const std::filesystem::path epath = tim::complete_path(path, tim::create_path::Base);
    if (epath.empty())
        return TIM_TRACE(Error,
                         "%s",
                         TIM_TR("Empty file path."_en,
                                "Пустое имя файла."_ru));

    return tim::write_to_file(epath, j.dump(indent));
}

std::size_t tim::process_file(const std::filesystem::path &path,
                              std::function<bool(const tim::byte_vector &)> fn,
                              std::size_t chunk_size)
{
    assert(fn);
    assert(chunk_size);

    const std::filesystem::path epath = tim::complete_path(path);

    if (epath.empty())
        return 0;

    std::ifstream is(epath, std::ios::binary);

    if (!is.is_open())
        return TIM_TRACE(Error,
                         TIM_TR("Failed to open file '%s': %s"_en,
                                "Ошибка при открытии файла '%s': %s"_ru),
                         epath.string().c_str(),
                         std::strerror(errno));

    is.seekg(0, std::ios_base::end);
    std::size_t size = is.tellg();
    is.seekg(0, std::ios_base::beg);
    std::size_t bytes_processed = 0;

    tim::byte_vector buf(chunk_size);

    while (size > 0)
    {
        const std::size_t bytes_to_read = size > chunk_size
                                            ? chunk_size
                                            : size;
        buf.resize(bytes_to_read);

        if (!is.read((char *)(&buf[0]), bytes_to_read))
        {
            TIM_TRACE(Error,
                      TIM_TR("Failed to read file '%s': %s"_en,
                             "Ошибка при чтении файла '%s': %s"_ru),
                      epath.string().c_str(),
                      std::strerror(errno));
            return 0;
        }

        if (!fn(buf))
            return 0;

        size -= bytes_to_read;
        bytes_processed += bytes_to_read;
    }

    is.close();

    return bytes_processed;
}
/*
void tim::add_recent_file(const std::filesystem::path &path)
{
    if (path.empty())
        return;

    std::list<std::filesystem::path> files = tim::p::recent_files().value();
    for (std::list<std::filesystem::path>::iterator i = files.begin(); i != files.end();)
        if (*i == path)
            i = files.erase(i);
        else
            ++i;
    files.emplace_front(path);
    if (files.size() > tim::RECENT_FILES_MAX)
        files.resize(tim::RECENT_FILES_MAX);

    tim::p::recent_files().set_value(files);
}
*/
bool tim::path_exists(const std::filesystem::path &path)
{
    std::error_code ec;
    return std::filesystem::exists(tim::complete_path(path), ec);
}

bool tim::is_network_url(const std::filesystem::path &path)
{
    if (path.empty())
        return false;

    const std::string schema = path.begin()->string();

    return schema == "http:"
                || schema == "https:"
                || schema == "ftp:"
                || schema == "ftps:"
                || schema == "ws:"
                || schema == "wss:"
                || schema == "mailto:"
                || schema == "tel:";
}

bool tim::is_valid_path(const std::filesystem::path &path)
{
    return !path.empty()
                && (tim::is_network_url(path)
                        || tim::path_exists(path));
}

std::vector<std::filesystem::path> tim::files(const std::filesystem::path &root)
{
    const std::filesystem::path epath = tim::complete_path(root);

    std::vector<std::filesystem::path> list;
    if (std::filesystem::exists(epath))
        for (const std::filesystem::path &p: std::filesystem::directory_iterator(epath))
            if (!std::filesystem::is_directory(p))
                list.emplace_back(p);
    return list;
}

/** Get file vector in the directory non-recursively.

    \param root Directory path.
    \param re A regular expression to filter the file names.
    \return Absolute paths vector.
 */
std::vector<std::filesystem::path> tim::files(const std::filesystem::path &root, const std::regex &re)
{
    const std::filesystem::path epath = tim::complete_path(root);

    std::vector<std::filesystem::path> list;
    if (std::filesystem::exists(epath))
        for (const std::filesystem::path &p: std::filesystem::directory_iterator(epath))
            if (!std::filesystem::is_directory(p)
                    && std::regex_match(p.string(), re))
                list.emplace_back(p);
    return list;
}
