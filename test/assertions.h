#ifndef ASSERTIONS_H_INCLUDED
#define ASSERTIONS_H_INCLUDED

#define ASSERT_OUT_SLICE_EQ(result, expected)                                                      \
  ASSERT((result).ok);                                                                             \
  ASSERT(cpc_is_slice(&(result).out));                                                             \
  ASSERT((result).out.as.slice.len == sizeof(expected) - 1);                                       \
  ASSERT(STRNCMP((result).out.as.slice.ptr, (expected), (result).out.as.slice.len) == 0)

#define ASSERT_OUT_NOTHING(result)                                                                 \
  ASSERT(!(result).ok);                                                                            \
  ASSERT(cpc_is_nothing(&(result).out));                                                           \
  ASSERT((result).out.as.slice.len == 0)

#define ASSERT_ERR_EQ(result, expected) ASSERT(STRCMP((result).err, (expected)) == 0)

#define ASSERT_REST_EQ(result, expected)                                                           \
  ASSERT((result).rest.len == sizeof(expected) - 1);                                               \
  ASSERT(STRNCMP((result).rest.ptr, (expected), (result).rest.len) == 0)

#define ASSERT_REST_EMPTY(result) ASSERT((result).rest.len == 0)

#endif /* ASSERTIONS_H_INCLUDED */
