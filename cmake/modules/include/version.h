#ifndef VERSION_H
#define VERSION_H

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifdef GIT_DESCRIBE
#define VERSION QUOTE(GIT_DESCRIBE)
#else
#define VERSION "unknown"
#endif

#endif /* VERSION_H */
