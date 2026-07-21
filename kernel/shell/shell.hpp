#pragma once

// ============================================================
// Interactive Kernel Shell — M7
// ============================================================

// Initialise and start the interactive shell task loop.
void shell_init();

// Execute a single shell command string.
void shell_execute(const char* cmdline);

// Main task entry point for the shell.
void shell_task();
