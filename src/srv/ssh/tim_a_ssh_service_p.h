#pragma once

#include <libssh/callbacks.h>
#include <libssh/libssh.h>

#include <pty.h>

#include <cassert>


namespace tim
{

class a_ssh_service;

namespace p
{

struct a_ssh_service
{
    explicit a_ssh_service(tim::a_ssh_service *q)
        : _q(q)
    {
        assert(_q);
    }

    static int data_function(ssh_session session, ssh_channel channel, void *data,
                             std::uint32_t len, int is_stderr, void *user_data);
    static int pty_request(ssh_session session, ssh_channel channel,
                           const char *term, int cols, int rows, int py, int px,
                           void *user_data);
    static int pty_resize(ssh_session session, ssh_channel channel,
                          int cols, int rows, int py, int px, void *user_data);
    static int shell_request(ssh_session session, ssh_channel channel, void *user_data);
    static int auth_password(ssh_session session, const char *user,
                             const char *pass, void *user_data);
    static int auth_publickey(ssh_session session,
                              const char *user,
                              struct ssh_key_struct *pub_key,
                              char signature_state,
                              void *user_data);
    static ssh_channel channel_open(ssh_session session, void *user_data);

    tim::a_ssh_service *const _q;

    ssh_session _session;
    ssh_channel _channel;

    /* Event which is used to poll the above descriptors. */
    ssh_event _event;

    winsize _winsize; // Structure for storing the pty size.

    int _auth_attempts = 0;
    bool _authenticated = false;

    ssh_server_callbacks_struct _server_cb;
    ssh_channel_callbacks_struct _channel_cb;

    tim::byte_vector _incoming_data;
    tim::byte_vector _outgoing_data;

    bool _running = false;
};

}

}
