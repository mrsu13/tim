#include "tim_version.h"

#include "tim_application.h"

#include <cstdlib>


int main(int argc, char **argv)
{
    std::unique_ptr<tim::application> app(new tim::application(argc, argv));

    app->exec();

    return EXIT_SUCCESS;
}
