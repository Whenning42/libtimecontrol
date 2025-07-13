
#define LOG_IF_ERROR(msg, fn) \
{ \
  int r = fn; \
  if (r == -1) perror(msg); \
}

#define EXIT_IF_ERROR(msg, fn) \
{ \
  int r = fn; \
  if (r == -1) { \
    perror(msg); \
    exit(1); \
  } \
}
