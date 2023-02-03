#ifndef _LIBCPP_SUPPORT_MIEROS_XLOCALE_H
#define _LIBCPP_SUPPORT_MIEROS_XLOCALE_H

#if defined(__mieros__)

#include <cstdlib>
#include <clocale>
#include <cwctype>
#include <ctype.h>
//#include <__support/xlocale/__nop_locale_mgmt.h>
//#include <__support/xlocale/__posix_l_fallback.h>
//#include <__support/xlocale/__strtonum_fallback.h>

// Bring in some stuff from __support/xlocale
inline _LIBCPP_HIDE_FROM_ABI size_t
strftime_l(char *__s, size_t __max, const char *__format, const struct tm *__tm,
           locale_t) {
  return ::strftime(__s, __max, __format, __tm);
}

inline _LIBCPP_HIDE_FROM_ABI size_t
wcsxfrm_l(wchar_t *__dest, const wchar_t *__src, size_t __n, locale_t) {
  return ::wcsxfrm(__dest, __src, __n);
}

inline _LIBCPP_HIDE_FROM_ABI int
wcscoll_l(const wchar_t *__ws1, const wchar_t *__ws2, locale_t) {
  return ::wcscoll(__ws1, __ws2);
}

inline _LIBCPP_HIDE_FROM_ABI size_t
strxfrm_l(char *__dest, const char *__src, size_t __n, locale_t) {
  return ::strxfrm(__dest, __src, __n);
}

inline _LIBCPP_HIDE_FROM_ABI long long
strtoll_l(const char *__nptr, char **__endptr, int __base, locale_t) {
  return ::strtoll(__nptr, __endptr, __base);
}

inline _LIBCPP_HIDE_FROM_ABI unsigned long long
strtoull_l(const char *__nptr, char **__endptr, int __base, locale_t) {
  return ::strtoull(__nptr, __endptr, __base);
}

#endif // __mieros__

#endif

