#define DECLARE_POOL(classname, maxpool) \
	private: \
	enum { MAX_POOL = maxpool }; \
	static unsigned long allocationMask; \
	static char *allocationPool; \
	public: \
	static void *operator new(size_t size) throw() \
    { \
		static char chunk[MAX_POOL * sizeof(classname)]; \
		if(!allocationPool) allocationPool = chunk; \
		for(int bit = 0; bit < MAX_POOL; bit++) \
		{ \
			unsigned long mask = 1 << bit; \
			if(!(mask & allocationMask)) \
			{ \
				allocationMask |= mask; \
                void *addr=&allocationPool[bit * sizeof(classname)];\
                return addr;\
			} \
		} \
		return NULL; \
	} \
	static void operator delete(void *p) \
    { \
		if(!p) return; \
		size_t offset = (char*)p - (char*)&allocationPool[0]; \
		size_t index = offset / sizeof(classname); \
        allocationMask &= ~(1 << index); \
    }\
    static void ResetAllocationPool()\
    {\
        allocationMask = 0;\
    }\

	

#define DEFINE_POOL(classname) \
	unsigned long classname::allocationMask = 0; \
	char *classname::allocationPool = 0;


