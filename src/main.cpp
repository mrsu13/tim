#include "tim_version.h"

#include "tim_application.h"
#include "tim_config.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>


namespace
{

/**
 * Печатает справку по аргументам командной строки в stdout.
 *
 * \param argv0 Имя исполняемого файла (argv[0]); подставляется
 *              в Usage-строку.
 */
void print_usage(const char *argv0)
{
    std::printf(
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  -h, --help            show this help and exit\n"
        "  -V, --version         print version and build info, then exit\n"
        "      --config <path>   path to JSON config file\n"
        "                        (default: ~/.tim/config.json)\n",
        argv0);
}

/**
 * Печатает версию и метку сборки в stdout.
 */
void print_version()
{
    std::printf("%s %s (built %s)\n", tim::APP_NAME, tim::VERSION, tim::BUILD);
}

}


/**
 * Точка входа: разбирает аргументы (--help/--version/--config),
 * заполняет имя/организацию приложения и крутит exec().
 *
 * \param argc Число аргументов.
 * \param argv Массив аргументов.
 * \return EXIT_SUCCESS при штатном выходе; EXIT_FAILURE при неверных
 *         аргументах командной строки.
 */
int main(int argc, char **argv)
{
    std::string config_path; // пусто = путь по умолчанию (~/.tim/config.json)

    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (!std::strcmp(arg, "--help") || !std::strcmp(arg, "-h"))
        {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }
        if (!std::strcmp(arg, "--version") || !std::strcmp(arg, "-V"))
        {
            print_version();
            return EXIT_SUCCESS;
        }
        if (!std::strcmp(arg, "--config"))
        {
            if (i + 1 >= argc)
            {
                std::fprintf(stderr, "%s: --config requires a path argument\n", argv[0]);
                return EXIT_FAILURE;
            }
            config_path = argv[++i];
            continue;
        }
        std::fprintf(stderr, "%s: unknown option '%s'; try --help\n", argv[0], arg);
        return EXIT_FAILURE;
    }

    tim::application::set_name(tim::APP_NAME);
    tim::application::set_org_name(tim::ORG_NAME);

    std::unique_ptr<tim::application> app(new tim::application(config_path));

    app->exec();

    return EXIT_SUCCESS;
}
