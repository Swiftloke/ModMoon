#pragma once
#include <unistd.h>
int maxslotcheck(u64 optionaltid = -1); //When not provided this simply checks the current tid
void launch();
void threadfunc_fade(void* main);
int countEntriesInDir(const char* dirname);

extern Handle event_fadefinished;