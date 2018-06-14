//Thanks to Sono for this code to subscribe to SRV events!

#include "srv.hpp"
#include "main.hpp"
#include "error.hpp"


Handle srv::notifsemaphore = 0;
RecursiveLock srv::rlock;
int srv::count = 0;
SignalHook srv::sigroot;
vu8 srv::running = 0;
Thread srv::thread = 0;

Result srv::init ()
{
    if(AtomicPostIncrement(&count)) return 0;
    
    Result res = srvInit();
    if(res < 0) goto fail;
    
    RecursiveLock_Init(&rlock);
    
    sigroot.notificaton = 0;
    sigroot.func = nullptr;
    sigroot.next = nullptr;
    
    srvEnableNotification(&notifsemaphore);
    
    srv::running = 1;
    thread = threadCreate(handlerloop, nullptr, 0x2000, 0x31, -2, true);
    if(!thread) return -1; //err(56, 0xF00FCACE);
    
    return 0;
    
    fail:
    AtomicDecrement(&count);
    return res;
}

void srv::exit ()
{
    if(AtomicDecrement(&count)) return;
    
    running = 0;
	//Result ret = svcSignalEvent(notifsemaphore);
    //error(tid2str(ret)); //FFFFFFFFD8E007F7
    lock();
    
    SignalHook* curr = sigroot.next;
    SignalHook* temp = nullptr;
    
    while(curr)
    {
        temp = curr->next;
        delete curr;
        curr = temp;
    }
    
    sigroot.notificaton = 0;
    sigroot.func = nullptr;
    sigroot.next = nullptr;
    
    unlock();
    
    srvExit();
}
void srv::lock ()
{
    RecursiveLock_Lock(&rlock);
}
//Crash here???
void srv::unlock ()
{
    RecursiveLock_Unlock(&rlock);
}

void srv::handlerloop (void * param)
{
    Result ret = 0;
    u32 NotificationID = 0;
    while(running)
    {
        ret = svcWaitSynchronization(notifsemaphore, -1ULL);
        if(!running) break;
        if(ret < 0) break;
        
        lock();
        
        ret = srvReceiveNotification(&NotificationID);
        if(ret < 0)
        {
            unlock();
            break;
        }
        
        
        SignalHook* curr = &sigroot;
        while(curr)
        {
			if (curr->func && (!curr->notificaton || curr->notificaton == NotificationID))
			{
				curr->func(NotificationID);
			}
            curr = curr->next;
        }
        unlock();
    }
    svcCloseHandle(notifsemaphore);
    
    //if(running) *(u32*)0x00100666 = ret;
}

void srv::hook (u32 notifid, SignalHandler func)
{
    if(!func) return;
    
    SignalHook* curr = &sigroot;
    if(sigroot.func)
    {
        while(curr->next)
        {
            curr = curr->next;
        }
        curr->next = new SignalHook;
        curr = curr->next;
        curr->next = nullptr;
    }
    
    curr->notificaton = notifid;
    curr->func = func;
    
    srvSubscribe(notifid);
}
