#pragma once
#include <stdbool.h>

// Returns true if the play-clock type menu should be shown on this launch.
// False when the user chose "Yes" on today's date.
bool menu_should_show(void);

// Create both menu windows and push the type-selection window.
void menu_init(void);

// Destroy both menu windows. Safe to call even if menu_init was never called.
void menu_deinit(void);

// Returns the selected clock type (0=40/25s, 1=30/10s, 2=25s, 3=30s).
// Defaults to 2 (25s) if no selection has ever been persisted.
int menu_get_clock_type(void);
