
CC=g++ -c -fPIC -g -DLINUX32 -fvisibility=hidden -lpthread -ldl
OBJ= EDvrCtrlAttach.o DeviceControl.o DllLoad.o InnerTransact.o InfoManage.o CommonDef.o ThreadPulse.o ArrayList.o WriteLog.o SyncLock.o toolsInsteadMFC.o 


libEDvrCtrlAttach.so : $(OBJ)
	g++ -g -fPIC -shared -lpthread -ldl -lrt $(OBJ) -o libEDvrCtrlAttach.so

EDvrCtrlAttach.o : EDvrCtrlAttach.h toolsInsteadMFC.h
	$(CC) EDvrCtrlAttach.c -o EDvrCtrlAttach.o -DEDVRCTRLATTACH_EXPORTS

DeviceControl.o : OV_DeviceControl.h InfoManage.h DllLoad.h InnerTransact.h toolsInsteadMFC.h
	$(CC) OV_DeviceControl.c -o DeviceControl.o

DllLoad.o : DllLoad.h 
	$(CC) DllLoad.cpp -o DllLoad.o
	
InnerTransact.o : InnerTransact.h InfoManage.h
	$(CC) InnerTransact.c -o InnerTransact.o

InfoManage.o : InfoManage.h CommonDef.h ArrayList.h
	$(CC) InfoManage.c -o InfoManage.o
	
CommonDef.o: CommonDef.h ControlPlug.h ThreadPulse.h
	$(CC) CommonDef.c -o CommonDef.o
	
ThreadPulse.o: ThreadPulse.h SyncLock.h
	$(CC) ThreadPulse.cpp -o ThreadPulse.o
	
ArrayList.o : ArrayList.h SyncLock.h
	$(CC) ArrayList.cpp -o ArrayList.o

WriteLog.o: WriteLog.h
	$(CC) WriteLog.cpp -o WriteLog.o

SyncLock.o:SyncLock.h
	$(CC) SyncLock.cpp -o SyncLock.o -lrt
	
toolsInsteadMFC.o: toolsInsteadMFC.h
	$(CC) toolsInsteadMFC.cpp -o toolsInsteadMFC.o
	
   
.PHONY:clean
clean:
	rm *.o *.so
 
