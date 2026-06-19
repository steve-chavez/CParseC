// the non-freestanding utils for tests
#ifndef HOSTED_H_INCLUDED
#define HOSTED_H_INCLUDED

#define ASSERT(expr) assert(expr)
#define PUTS(msg) puts(msg)
#define STRCMP(lhs, rhs) strcmp((lhs), (rhs))
#define STRNCMP(lhs, rhs, n) strncmp((lhs), (rhs), (n))

#endif /* HOSTED_H_INCLUDED */
