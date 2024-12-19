#ifndef NEUROLIB_H
#define NEUROLIB_H

// Initializes the neurosymbolic library. This function
// must be called *exactly once* before calling any other
// function in the library.
void neurosym_init();

// Returns the response from the API given a prompt.
// The caller is responsible for freeing the returned string.
// If anything goes wrong, returns NULL.
char * response(const char * prompt);

#endif
