#include "tim_version.h"

#include "tim_app.h"

#include <cstdlib>


int main(int argc, char **argv)
{
    std::unique_ptr<tim::app> app(new tim::app(argc, argv));

    app->exec();

    return EXIT_SUCCESS;
}
