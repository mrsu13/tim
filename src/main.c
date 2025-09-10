#include "tim_version.h"

#include "tim_app.h"

#include <stdlib.h>


int main(int argc, char **argv)
{
    tim_app_t *app = tim_app_new(argc, argv);

    tim_app_exec(app);
    tim_app_free(app);

    return EXIT_SUCCESS;
}
