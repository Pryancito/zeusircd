#include "include.h"

char *make_string(const char *fmt, ...)
{
   static char buf[512];
   va_list args;

   char *ptr = NULL;

   ptr = buf;

   bzero(buf, sizeof(buf));

   if (strlen(fmt) != 0 )
   {
      va_start(args, fmt);
      vsnprintf(buf, sizeof (buf), fmt, args);
      va_end(args);
   }

   return ptr;
}