#pragma once

typedef struct tim_user tim_user_t;

tim_user_t *tim_user_new(void);
void tim_user_free(tim_user_t *user);

const char *tim_user_id(const tim_user_t *user);
void tim_user_set_id(tim_user_t *user, const char *id);

const char *tim_user_name(const tim_user_t *user);
void tim_user_set_name(tim_user_t *user, const char *name);
