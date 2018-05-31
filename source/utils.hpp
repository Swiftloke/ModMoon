#pragma once
int maxslotcheck(u64 optionaltid = -1); //When not provided this simply checks the current tid
void launch();
void threadfunc_fade(void* main);

extern Handle event_fadefinished;