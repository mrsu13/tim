#pragma once

#include <stdint.h>


typedef struct tim_user tim_user_t;
typedef uint8_t tim_country_t;
typedef uint64_t tim_phone_t;

tim_user_t *tim_user_new(tim_country_t country, tim_phone_t phone);
void tim_user_free(tim_user_t *user);

const char *tim_user_id(const tim_user_t *user);
void tim_user_set_id(tim_user_t *user, const char *id);

const char *tim_user_nick_name(const tim_user_t *user);
void tim_user_set_nick_name(tim_user_t *user, const char *nick_name);
