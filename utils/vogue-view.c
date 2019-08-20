#include "config.h"
#include "viewer.h"

extern const VogueViewer voguecairo_viewer;
extern const VogueViewer voguexft_viewer;
extern const VogueViewer vogueft2_viewer;
extern const VogueViewer voguex_viewer;

const VogueViewer *viewers[] = {
#ifdef HAVE_CAIRO
  &voguecairo_viewer,
#endif
#ifdef HAVE_XFT
  &voguexft_viewer,
#endif
#ifdef HAVE_FREETYPE
  &vogueft2_viewer,
#endif
#ifdef HAVE_X
  &voguex_viewer,
#endif
  NULL
};
