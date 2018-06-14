//Thanks to Sono for this code to subscribe to SRV events!

#ifndef LZZ_srv_h
#define LZZ_srv_h
#include <3ds.h>

typedef void (* SignalHandler) (u32);
struct SignalHook
{
  u32 notificaton;
  SignalHandler func;
  SignalHook * next;
};
class srv
{
public:
  static Handle notifsemaphore;
  static RecursiveLock rlock;
  static int count;
  static SignalHook sigroot;
  static vu8 running;
  static Thread thread;
  static Result init ();
  static void exit ();
  static void lock ();
  static void unlock ();
  static void handlerloop (void * param);
  static void hook (u32 notifid, SignalHandler func);
};
#endif
