#pragma once
#include <stdbool.h>

// Create the type-selection menu window and push it onto the stack.
void menu_init(void);

// Destroy the menu window. Safe to call even if menu_init was never called.
void menu_deinit(void);

// Returns the clock type selected this session (0=40/25s, 1=30/10s, 2=25s, 3=30s).
// Defaults to 0 (40/25s) until the user makes a selection.
int menu_get_clock_type(void);
