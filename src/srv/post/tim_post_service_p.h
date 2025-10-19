#pragma once

#include <filesystem>


namespace tim::p
{

struct post_service
{
    void subscribe();
    void on_post(const std::filesystem::path &topic, const char *data, std::size_t size);
};
}
