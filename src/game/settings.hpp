#define VERSION "cpp_dev0.0.1"
//#define RELEASE_MODE

#ifndef RELEASE_MODE
#if WIN32
#ifdef VS
#define PROJECT_DIR ""
#else
#define PROJECT_DIR "../../"
#endif
#else
#define PROJECT_DIR "../"
#endif
#else /*RELASE_MODE*/
#define PROJECT_DIR ""
#endif /*RELASE_MODE*/