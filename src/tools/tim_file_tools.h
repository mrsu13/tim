#pragma once

#include "tim_byte_vector.h"
#include "tim_json.h"

#include <filesystem>
#include <functional>
#include <regex>
#include <vector>


namespace tim
{

enum class filesystem_location
{
    AppConfig,
    AppData,
    AppLocalData,
    AppTlsData,
    Current,
    Home,
    Desktop,
    Documents,
    Download,
    Pictures,
    Music,
    Videos,
    Temp
};

std::filesystem::path standard_location(tim::filesystem_location location);

enum class create_path
{
    None,
    Base,
    Full
};

std::filesystem::path complete_path(const std::filesystem::path &path,
                                    tim::create_path f = tim::create_path::None);

bool read_file(const std::filesystem::path &path, std::string &text);
bool read_json(const std::filesystem::path &path, nlohmann::json &j);

enum class file_write_mode
{
    Append,
    Overwrite
};

bool write_to_file(const std::filesystem::path &path,
                   const std::string &text,
                   tim::file_write_mode mode = tim::file_write_mode::Overwrite);

bool write_json(const std::filesystem::path &path, const nlohmann::json &j, int indent = -1);

std::size_t process_file(const std::filesystem::path &path,
                         std::function<bool(const tim::byte_vector &)> fn,
                         std::size_t chunk_size = 10240);

bool path_exists(const std::filesystem::path &path);

bool is_network_url(const std::filesystem::path &path);
bool is_valid_path(const std::filesystem::path &path);

std::vector<std::filesystem::path> files(const std::filesystem::path &root);
std::vector<std::filesystem::path> files(const std::filesystem::path &root, const std::regex &re);

}
