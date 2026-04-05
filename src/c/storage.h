#pragma once

// Restore all module state from flash. Call before window creation.
void storage_load(void);

// Persist all module state to flash. Call on back button press.
void storage_save(void);
