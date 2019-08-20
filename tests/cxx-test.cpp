/* This test makes sure that all Vogue headers can be included
 * and compiled in a C++ program.
 */

#include <vogue/vogue.h>

#ifdef HAVE_WIN32
#include <vogue/voguewin32.h>
#endif

#ifdef HAVE_XFT
#include <vogue/voguexft.h>
#endif

#ifdef HAVE_FREETYPE
#include <vogue/vogueft2.h>
#endif

#ifdef HAVE_CAIRO
#include <vogue/voguecairo.h>
#endif

int
main ()
{
  return 0;
}
