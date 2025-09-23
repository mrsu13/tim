#include "tim_user.h"

#include "tim_uuid.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


typedef struct tim_user
{
    tim_uuid_t id;
    tim_country_t country;
    tim_phone_t phone;
    char *nick_name;
    const char *icon;
    char *motto;
} tim_user_t;

tim_user_t *tim_user_new(tim_country_t country, tim_phone_t phone)
{
    assert(phone > 1000000000 && phone < 9999999999
                && "Invalid phone number.");

    tim_user_t *user = (tim_user_t *)calloc(1, sizeof(tim_user_t));
    assert(user);
    user->country = country;
    user->phone = phone;
    return user;
}

void tim_user_free(tim_user_t *user)
{
    assert(user);
    free(user->nick_name);
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

const char *tim_user_nick_name(const tim_user_t *user)
{
    assert(user);
    return user->nick_name;
}

void tim_user_set_nick_name(tim_user_t *user, const char *nick_name)
{
    assert(user);
    free(user->nick_name);
    if (nick_name
            && *nick_name)
        user->nick_name = strdup(nick_name);
    else
        user->nick_name = NULL;
}
