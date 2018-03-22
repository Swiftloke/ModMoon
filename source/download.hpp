#ifdef BUILTFROM3DSX
#define BUILDIS3DSX true
#else
#define BUILDIS3DSX false
#endif

void initupdatechecker();
bool isupdateavailable();
void initdownloadandinstallupdate();
void downloadsignalandwaitforcancel();