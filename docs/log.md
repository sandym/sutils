
## `su/log/logger.h`

Strongly inspired by https://github.com/Iyengar111/NanoLog. But should works correctly on all platforms (NanoLog is linux only).

example:
```C++
  log_warn() << "wrong number: " << v;
```

Threading is optional, just instanciate a `su::logger_thread`:
```C++
  su::logger_thread start_logger_thread;
  // logging will be delegated to a thread
```

It will log to `std::clog` by default. Log output can be redirected and also support multiple loggers.

@todo

## `su/log/logger_file.h`

Redirect a logger to a file.
@todo
