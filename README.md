### CIRCD

A simple IRCd written in plain C.  Adheres loosely to (RFC 2812)[https://tools.ietf.org/html/rfc2812].


#### Compiling and running

```bash
$ make
$ ./circd
```

Runs on port 6667 by default.  Hardcoded as a `#DEFINE` in `circd.c`.


#### TODOs

- Mutexes
- Client leave cleanup
- Don't let nick be picked if nick is in use already
- Handle modes
- Oper command
- Kick/ban etc
- Channel topic
- Send names on channel join


#### Limitations

- No support for channel perms (invite/pass)
- Only single channel per join message supported
- Very little error handling (badly phrased messages etc)
- Does not send ping, only responds