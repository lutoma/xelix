#pragma once

void memory_init_preprotected();
// dependencies: display (so we can output debug messages ;) )

void memory_init_postprotected();
// dependency: isr (we need the page fault interrupt)
