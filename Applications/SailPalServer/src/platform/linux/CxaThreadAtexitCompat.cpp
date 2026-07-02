extern "C" int __cxa_thread_atexit_impl(void (*func)(void*), void* obj, void* dso_symbol);

extern "C" int __cxa_thread_atexit(void (*func)(void*), void* obj, void* dso_symbol) {
	return __cxa_thread_atexit_impl(func, obj, dso_symbol);
}