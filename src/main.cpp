#include "tim_version.h"

#include "tim_application.h"
#include "tim_config.h"

#include <cstdlib>


int main(int argc, char **argv)
{
    tim::application::set_name(tim::APP_NAME);
    tim::application::set_org_name(tim::ORG_NAME);

    std::unique_ptr<tim::application> app(new tim::application(argc, argv));

    app->exec();

    return EXIT_SUCCESS;
}
