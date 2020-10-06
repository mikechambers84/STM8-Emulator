#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "timing.h"

uint64_t timing_cur;
uint64_t timing_freq;
TIMER* timers = NULL;
uint32_t timers_count = 0;

int timing_init() {
#ifdef _WIN32
	LARGE_INTEGER freq;
	//TODO: error handling
	QueryPerformanceFrequency(&freq);
	timing_freq = (uint64_t)freq.QuadPart;
#else
	timing_freq = 1000000;
#endif
	return 0;
}

void timing_loop() {
	uint32_t i;
#ifdef _WIN32
	LARGE_INTEGER cur;
	//TODO: error handling
	QueryPerformanceCounter(&cur);
	timing_cur = (uint64_t)cur.QuadPart;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	timing_cur = (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
#endif
	for (i = 0; i < timers_count; i++) {
		if (timing_cur >= (timers[i].previous + timers[i].interval)) {
			if (timers[i].enabled != TIMING_DISABLED) {
				if (timers[i].callback != NULL) {
					(*timers[i].callback)(timers[i].data);
				}
				timers[i].previous += timers[i].interval;
				if ((timing_cur - timers[i].previous) >= (timers[i].interval * 100)) {
					timers[i].previous = timing_cur;
				}
			}
		}
	}
}

//Just some code for performance testing
void timing_speedTest() {
#ifdef _WIN32
	uint64_t start, i;
	LARGE_INTEGER cur;
	//TODO: error handling
	QueryPerformanceCounter(&cur);
	start = (uint64_t)cur.QuadPart;

	i = 0;
	while (1) {
		QueryPerformanceCounter(&cur);
		timing_cur = (uint64_t)cur.QuadPart;
		i++;
		if ((timing_cur - start) >= timing_freq) break;
	}
	printf("%llu calls to QPC in 1 second\r\n", i);
#endif
}

uint32_t timing_addTimerUsingInterval(void* callback, void* data, uint64_t interval, uint8_t enabled) {
	TIMER* temp;
	uint32_t ret;
#ifdef _WIN32
	LARGE_INTEGER cur;

	//TODO: error handling
	QueryPerformanceCounter(&cur);
	timing_cur = (uint64_t)cur.QuadPart;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	timing_cur = (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
#endif
	temp = (TIMER*)realloc(timers, (size_t)sizeof(TIMER) * (timers_count + 1));
	if (temp == NULL) {
		//TODO: error handling
		return TIMING_ERROR; //NULL;
	}
	timers = temp;

	timers[timers_count].previous = timing_cur;
	timers[timers_count].interval = interval;
	timers[timers_count].callback = callback;
	timers[timers_count].data = data;
	timers[timers_count].enabled = enabled;

	ret = timers_count;
	timers_count++;

	return ret;
}

uint32_t timing_addTimer(void* callback, void* data, double frequency, uint8_t enabled) {
	return timing_addTimerUsingInterval(callback, data, (uint64_t)((double)timing_freq / frequency), enabled);
}

void timing_updateInterval(uint32_t tnum, uint64_t interval) {
	if (tnum >= timers_count) {
		return;
	}
	timers[tnum].interval = interval;
}

void timing_updateIntervalFreq(uint32_t tnum, double frequency) {
	if (tnum >= timers_count) {
		return;
	}
	timers[tnum].interval = (uint64_t)((double)timing_freq / frequency);
}

void timing_timerEnable(uint32_t tnum) {
	if (tnum >= timers_count) {
		return;
	}
	timers[tnum].enabled = TIMING_ENABLED;
	timers[tnum].previous = timing_getCur();
}

void timing_timerDisable(uint32_t tnum) {
	if (tnum >= timers_count) {
		return;
	}
	timers[tnum].enabled = TIMING_DISABLED;
}

uint64_t timing_getFreq() {
	return timing_freq;
}

uint64_t timing_getCur() {
#ifdef _WIN32
	LARGE_INTEGER cur;

	//TODO: error handling
	QueryPerformanceCounter(&cur);
	timing_cur = (uint64_t)cur.QuadPart;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	timing_cur = (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
#endif

	return timing_cur;
}
