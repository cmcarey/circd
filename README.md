### CIRCD

A simple IRCd written in plain C.  Follows [RFC 2812](https://tools.ietf.org/html/rfc2812) with limitations.  GPLv3.


#### Compiling and running

```bash
$ make
$ ./circd
```

Runs on port 6667 by default, hardcoded as a `#DEFINE` in `circd.c`.  TLS not supported with no plans to do so - run behind a TLS terminator like [Hitch](https://hitch-tls.org/);


#### TODOs

- Mutexes to prevent race conditions
- Don't let nick be picked if nick is in use already
- Handle modes (channel/client/client in channel)
- Oper command
- Kick/ban etc
- Channel topic
- Send names on channel join
- `PRIVMSG` to another user
- Delete channel if all users exit (part/quit)
- Send NICK message on nick change (handshake and future)
- Don't allow multiple joins of same channel


#### Limitations

- No support for channel perms (invite/pass)
- Only single channel per join message supported
- Very little error handling (badly phrased messages etc)
- Does not send ping, only responds
