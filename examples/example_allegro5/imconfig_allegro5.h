//-----------------------------------------------------------------------------
// COMPILE-TIME OPTIONS FOR DEAR IMGUI ALLEGRO 5 EXAMPLE
// See imconfig.h for the full template
// Because Allegro doesn't support 16-bit vertex indices, we enable the compile-time option of imgui to use 32-bit indices
//-----------------------------------------------------------------------------

#pragma once

// Use 32-bit vertex indices because Allegro doesn't support 16-bit ones
// This allows us to avoid converting vertices format at runtime
#define ImDrawIdx  int
