#include "tim_user.h"

#include "tim_uuid.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


typedef struct tim_user
{
    tim_uuid_t id;
    char *name;
} tim_user_t;

tim_user_t *tim_user_new(void)
{
    tim_user_t *user = (tim_user_t *)calloc(1, sizeof(tim_user_t));
    assert(user);
    user->name = NULL;
    return user;
}

void tim_user_free(tim_user_t *user)
{
    assert(user);
    free(user->name);
    free(user);
}

const char *tim_user_id(const tim_user_t *user)
{
    assert(user);
    return user->id;
}

void tim_user_set_id(tim_user_t *user, const char *id)
{
    assert(user);
    if (id)
        memcpy(user->id, id, sizeof(user->id));
    else
        memset(user->id, 0, sizeof(user->id));
}

const char *tim_user_name(const tim_user_t *user)
{
    assert(user);
    return user->name;
}

void tim_user_set_name(tim_user_t *user, const char *name)
{
    assert(user);
    free(user->name);
    if (name
            && *name)
        user->name = strdup(name);
    else
        user->name = NULL;
}
