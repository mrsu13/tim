#pragma once

#include "tim_a_inetd_service.h"


struct mg_connection;

namespace tim
{

// Реализация a_inetd_service поверх mongoose-соединения. Чтение и запись
// идут через буферы mg_connection.
class a_tcp_inetd_service : public tim::a_inetd_service
{

public:

    void close() override;
    std::size_t read(const char **data) override;
    bool write(const char *data, std::size_t size) override;

    mg_connection *connection() const noexcept;

protected:

    a_tcp_inetd_service(const std::string &name, mg_connection *c);
    ~a_tcp_inetd_service();

private:

    mg_connection *_c;
};

}
