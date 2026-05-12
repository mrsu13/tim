#pragma once

#include <chrono>
#include <cstdint>


namespace tim
{

static const char COMMAND_PREFIX = '/';
static const char APP_NAME[] = "tim";
static const char ORG_NAME[] = "mrsu";
static const char HISTORY_FNAME[] = "history.txt";
static const char SSH_HOST_KEY_FNAME[] = "host_ed25519";
static const char SSH_DATA_SUBDIR[] = "ssh";

/**
 * SQLite
 */
static const std::chrono::microseconds DB_BUSY_TIMEOUT(100000);
static const std::size_t DB_BUSY_TRIES = 5;
static const char DB_FILE_NAME[] = "tim.db";
}
