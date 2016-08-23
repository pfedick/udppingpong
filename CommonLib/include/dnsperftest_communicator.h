#ifndef INCLUDE_DNSPERFTEST_COMMUNICATOR_H_
#define INCLUDE_DNSPERFTEST_COMMUNICATOR_H_
#include <list>

class SystemStat;

class Communicator : private ppl7::TCPSocket
{
	private:
		ppluint64		lastuse;
		ppluint64		lastping;
		double			pingtime;
		ppl7::SocketMessage	Msg;

		int timeout_connect_sec, timeout_connect_usec;
		int timeout_read_sec, timeout_read_usec;

	public:
		Communicator();
		~Communicator();

		void connect(const ppl7::String &Hostname, int Port);
		void disconnect();
		bool ping();

		void proxyTo(const ppl7::String &Hostname, int Port);

		void setConnectTimeout(int sec, int usec);
		void setReadTimeout(int sec, int usec);
		bool talk(const ppl7::AssocArray &msg, ppl7::AssocArray &answer, ppl7::Thread *watch_thread=NULL);

		void startSensor();
		void stopSensor();
		void getSensorData(std::list<SystemStat> &data);


};


#ifdef TO_IMPLEMENT

class CTestAgent : public ppl6::CListItem, private ppl6::CTCPSocket
{
	public:
		class Config
		{
			public:
				ppl6::CString Hostname;
				ppl6::CString Login;
				ppl6::CString Password;
				ppl6::CString Certname;
				int Port;
				bool UseSSL;
				Config() {
					Port=52220;
					UseSSL=false;
				}
				Config(const ppl6::CString &Hostname, int Port, const ppl6::CString &Login, const ppl6::CString &Password) {
					this->Hostname=Hostname;
					this->Port=Port;
					this->Login=Login;
					this->Password=Password;
					this->UseSSL=false;
				}

				Config(const ppl6::CString &Hostname, int Port, const ppl6::CString &Login, const ppl6::CString &Password, bool useSSL, const ppl6::CString &Certname=ppl6::CString()) {
					this->Hostname=Hostname;
					this->Port=Port;
					this->Login=Login;
					this->Password=Password;
					this->UseSSL=useSSL;
					this->Certname=Certname;
				}
		};

	private:
		Config			Target;
		Config			Proxy;

		CString			Name;
		CString			Identifier;
		ppl6::CSocketMessage	Msg;
		ppluint64		lastuse;
		ppluint64		lastping;
		double			pingtime;
		ppl6::CSSL		*ssl;
		bool			dead;

	public:


		CTestAgent();
		~CTestAgent();
		int Ping(ppl6::CAssocArray &answer);
		int Ping();
		int Call(ppl6::CAssocArray &cmd, ppl6::CAssocArray &answer);
		int IsAnswerSuccess(ppl6::CAssocArray &answer);
		int IsAnswerSuccess(ppl6::CAssocArray &answer, ppl6::CString &Error);
		const char *GetName();

		int connect(const Config &conf, ppl6::CSSL *ssl=NULL);
		int connect(const Config &target, const Config &proxy, ppl6::CSSL *ssl=NULL);
		void disconnect();
		int reconnect();
		int proxyTo(const Config &conf);
		int authorize(const ppl6::CString &Login, const ppl6::CString &Password);
		int getVersion(ppl6::CAssocArray &answer);
		ppluint64 lastUsage() const;
		ppluint64 lastPing() const;
		double lastPingTime() const;
		void updateUsage();
		bool isDead() const;

		void setWatchThread(ppl6::CThread *thread);
		void setConnectTimeout(int sec, int usec);
		void setReadTimeout(int sec, int usec);
		void setName(const ppl6::CString &name);
		void setIdentifier(const ppl6::CString &name);
		const ppl6::CString &identifier() const;
		const ppl6::CString &name() const;
		const ppl6::CString &hostname() const;
		const ppl6::CString &proxyname() const;

};
#endif

#endif /* INCLUDE_DNSPERFTEST_COMMUNICATOR_H_ */
