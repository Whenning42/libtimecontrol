# libtimecontrol

libtimecontrol is a dynamic library and python controller that allow you to control
the rate at which time passes for another process.

# Usage

```
$ # Launch app with LD_PRELOAD and time channel set.
$ # Control the app with time_control.py.
```

# How it works

- Uses LD_PRELOAD and dlsym overrides
- Calculates what time should be given the history of `set_speed()` calls.
- Uses a seqlocked clock state to get re-entrant and thus async-signal safe time
functions.


# Potential Gotchas

- Dlsym overriding is a tricky thing. I recommend putting this library as the first
LD_PREOLOAD in a chain if you're running multiple dlsym overriding LD_PRELOAD libraries
at once. This library should correctly dispatch to other LD_PRELOAD `dlsyms` whereas
other dlsym LD_PRELOAD libraries may skip other `dlsyms` in the LD_PRELOAD chain instead
dispatching directly to libc.

- Sleep handling is also a little tricky. As is, this library calculates how long
to sleep for using the set speed and then sleeps for that duration, regardless of if
a later set_speed() call comes along that ideally would change the sleep's duration.
In practice, for my use case (hiding latency between software agents and closed-source
games), this hasn't been a problem.

