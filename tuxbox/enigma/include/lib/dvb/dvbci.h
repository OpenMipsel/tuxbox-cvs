#ifndef __core_dvb_ci_h
#define __core_dvb_ci_h

#include <lib/dvb/service.h>
#include <lib/base/ebase.h>
#include <lib/base/thread.h>
#include <lib/base/message.h>
#include <lib/system/elock.h>
#include <set>

#define PMT_ENTRYS  256

struct session_struct
{
	unsigned int tc_id;
	unsigned long service_class;
	unsigned int state;
	unsigned int internal_state;
};
				
struct tempPMT_t
{
	int type;     //0=prg-nr 1=pid 2=descriptor
	unsigned char *descriptor;
	unsigned short pid;
	unsigned short streamtype;
};


class eDVBCI: private eThread, public eMainloop, public Object
{
	static int instance_count;
protected:
 enum
	{
		stateInit, stateError, statePlaying, statePause
	};
	int state;
	int fd;
	eSocketNotifier *ci;

	int ci_state;
	int buffersize;	
		
	eTimer pollTimer;
	eLock lock;

	int tempPMTentrys;

	struct session_struct sessions[32];
	struct tempPMT_t tempPMT[PMT_ENTRYS];

	char appName[256];
	unsigned short caids[256];
	unsigned int caidcount;

	unsigned char ml_buffer[1024];
	int ml_bufferlen;
	int ml_buffersize;

	void clearCAIDs();
	void addCAID(int caid);	
	void pushCAIDs();	
	void PMTflush(int program);
	void PMTaddPID(int pid,int streamtype);
	void PMTaddDescriptor(unsigned char *data);
	void newService();
	void create_sessionobject(unsigned char *tag,unsigned char *data,unsigned int len,int session);

	void sendData(unsigned char tc_id,unsigned char *data,unsigned int len);	
	void sendTPDU(unsigned char tpdu_tag,unsigned int len,unsigned char tc_id,unsigned char *data);
	void help_manager(unsigned int session);
	void app_manager(unsigned int session);
	void ca_manager(unsigned int session);

	void handle_session(unsigned char *data,int len);
	int service_available(unsigned long service_class);
	void handle_spdu(unsigned int tpdu_tc_id,unsigned char *data,int len);	
	void receiveTPDU(unsigned char tpdu_tag,unsigned int len,unsigned char tpdu_tc_id,unsigned char *data);
	void incoming(unsigned char *buffer,int len);
	void dataAvailable(int what);
	void poll();
	void updateCIinfo(unsigned char *buffer);

	void mmi_begin();
	void mmi_end();
	void mmi_answ(unsigned char *answ,int len);
	void mmi_menuansw(int);

public:
	struct eDVBCIMessage
	{
		enum
		{
			start,
			reset, 
			init,
			exit,
			flush,
			addDescr,
			addVideo,
			addAudio,
			es,
			go,
			PMTflush,
			PMTaddPID,
			PMTaddDescriptor,
			mmi_begin,
			mmi_end,
			mmi_answ,
			mmi_menuansw,
			getcaids,
		};
		int type;
		unsigned char *data;
		int pid;
		int streamtype;

		eDVBCIMessage() { }
		eDVBCIMessage(int type): type(type) { }
		eDVBCIMessage(int type, unsigned char *data): type(type), data(data) { }
		eDVBCIMessage(int type, int pid): type(type),pid(pid) { }
		eDVBCIMessage(int type, int pid, int streamtype): type(type),pid(pid),streamtype(streamtype) { }

	};
	eFixedMessagePump<eDVBCIMessage> messages;
	
	void gotMessage(const eDVBCIMessage &message);

	eDVBCI();
	~eDVBCI();
	
	void thread();
	Signal1<void, const char*> ci_progress;
	Signal1<void, const char*> ci_mmi_progress;

};
#endif
