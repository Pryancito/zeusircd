#ifndef CLASES_H
#define CLASES_H

#include <string>
#include <thread>
#include <map>
#include <vector>
#include <list>
#include <random>
#include <deque>
#include <mutex>
#include <condition_variable>

using namespace std;

class Socket
{
	public:
		char *ip;
		int port;
		std::thread tw;
		bool IPv6;
		bool SSL;

	void MainSocket ();
	void ServerSocket ();
	void Cliente (TCPStream* s);
	void Servidor (TCPStream* s);
	std::thread Thread(TCPStream* s);
	std::thread SThread(TCPStream* s);
	std::thread MainThread();
	std::thread ServerThread();
	std::thread MainThreadv6();
	std::thread ServerThreadv6();
	void Write (TCPStream *stream, const string mensaje);
};

class Config
{
	public:
		std::map <std::string, std::string> conf;
		std::string version = "Zeus-1.0-Alpha5";
		std::string file = "server.conf";
		
	void Cargar ();
	void Procesa (std::string linea);
	void Configura (std::string dato, std::string valor);
	std::string Getvalue (std::string dato);
};

class Memo
{
	public:
		string sender;
		string receptor;
		long int time;
		string mensaje;
};

class Nick
{
    public:
    	TCPStream *stream;
		std::string nickname;
		std::string ident;
		std::string ip;
		std::string cloakip;
		long int login;
		std::string nodo;
		bool registrado;
		bool conectado;
		bool tiene_r;
		bool tiene_z;
		bool tiene_o;
		bool tiene_w;
    
    bool Existe (string nickname);
    bool Conectado (int ID);
    void CambioDeNick (int ID, string nuevonick);
    string FullNick (int ID);
    string GetNick(int ID);
    string GetIdent(int ID);
    string GetIP(int ID);
    string GetCloakIP(int ID);
    string GetvHost (int ID);
    void SetIdent(int ID, string _ident);
    bool Registrado (int ID);
	void Conectar (int ID);
	long int Creado(string _nick);
	bool EsMio(string nickname);
};

class Oper
{
	public:

		string nickoper;
		
		bool Login (std::string source, std::string nickname, std::string pass);
		void GlobOPs (std::string mensaje);
		string MkPassWD (std::string pass);
		int GetOperadores ();
		bool IsOper(string nick);
		void SetModeO (string nickname);
};

class Server
{
	public:
		TCPStream *stream;
		std::string nombre;
		std::string ip;
		int saltos;
		std::string hub;
		std::vector <std::string> connected;
		int maxusers;
		int maxchannels;
		
	bool IsAServer (std::string ip);
	bool IsConected (std::string ip);
	int GetID (std::string ip);
	void Conectar(std::string ip);
	void SendBurst (TCPStream* stream);
	void ListServers (TCPStream* stream);
	bool ProcesaMensaje (TCPStream* stream, const string mensaje);
	void SendToAllServers (const string std);
	void SendToAllButOne (TCPStream *stream, const string std);
	bool HUBExiste();
	bool SoyElHUB();
	void SQUITByServer(string server);
	bool CheckClone(string ip);
	string FindName(string ip);
	bool IsAServerTCP(TCPStream *stream);
	string GetServerTCP (TCPStream *stream);
	int GetIDS (TCPStream *stream);
	bool Existe(string ip);
};

class Cliente
{
	public:
	
	bool ProcesaMensaje (TCPStream* stream, std::string mensaje);
	void Bienvenida (TCPStream* stream, std::string nickname);
};

class Ban
{
	public:
		string mascara;
		string who;
		unsigned long fecha;
};

class User
{
	public:
		string nombre;
		char modo;
};

class Chan
{
	public:
		std::string nombre;
		deque <User*> usuarios;
		deque <Ban*> bans;
		time_t creado;
		bool tiene_r;
	
	string GetRealName (string canal);
	bool IsInChan(string canal, string nickname);
	void Join (string canal, string nickname);
	void Part(string canal, string nickname);
	void SendNAMES(TCPStream *stream, string canal);
	void PropagateJoin (string canal, int sID);
	void PropagatePart (string canal, string nickname);
	void PropagarNick(string viejo, string nuevo);
	void PropagarQUIT(TCPStream *stream);
	void PropagarMSG(string nickname, string chan, string mensaje);
	void Lista (std::string canal, TCPStream *stream);
	void PropagarMODE(string who, string nickname, string chan, char modo, bool add);
	void PropagarQUITByNick(string nickname);
	void PropagateKICK(int sID, string canal, string nickname, string motivo);
	bool IsBanned(int sID, string canal);
	int MaxChannels(string nickname);
};

class DB
{
	public:
		
	void ProcesaMensaje(string mensaje);
	void AlmacenaDB(string cadena);
	void BorraDB(string id);
	void EscribeDB(DB *mensaje);
	string GetLastRecord ();
	void IniciarDB();
	string SQLiteReturnString (string sql);
	int SQLiteReturnInt (string sql);
	bool SQLiteNoReturn (string sql);
	int Sync(TCPStream *stream, string id);
	vector <string> SQLiteReturnVector (string sql);
	string GenerateID();
};

class NickServ
{
	public:
		
	void ProcesaMensaje (TCPStream *stream, string mensaje);
	bool IsRegistered(string nickname);
	string Register (string nickname, string pass);
	bool Login (string nickname, string pass);
	int GetNicks();
	bool GetOption(string option, string nickname);
	void CheckMemos (int sID);
	void UpdateLogin (int sID);
};

class ChanServ
{
	public:
		
	void ProcesaMensaje (TCPStream *stream, string mensaje);
	bool IsRegistered(string channel);
	bool IsFounder(string nickname, string channel);
	int Access (string nickname, string channel);
	void CheckModes(string nickname, string channel);
	bool IsAKICK(string mascara, string canal);
	int GetChans();
};

class OperServ
{
	public:
		
	void ProcesaMensaje (TCPStream *stream, string mensaje);
};

class Data
{
	public:
		deque <Nick*> nicks;
		deque <Server*> servers;
		deque <Chan*> canales;
		deque <Oper*> operadores;
		deque <Memo*> memos;
			
	/* Nicks */
	void CrearNick (TCPStream *stream, std::string nick);
	void BorrarNick (TCPStream *stream);
	void BorrarNickByNick(string nickname);
	int BuscarIDNick (std::string nick);
	int BuscarIDStream (TCPStream *stream);
	TCPStream *BuscarStream(std::string nick);
	void SNICK(string nickname, string ip, string cloakip, long int creado, string nodo, string modos);
	string Time(long int tiempo);
	int GetUsuarios ();
	/* Servers */
	void AddServer (TCPStream* stream, string nombre, string ip, int saltos);
	void DeleteServer (string name);
	int GetServidores();
	void Conexiones(string principal, string linkado);
	/* Canales */
	void CrearCanal (string nombre);
	int GetChanPosition (string canal);
	int GetNickPosition (string canal, string nickname);
	void DelUsersToChan (int id, int idn);
	void AddUsersToChan (int id, string nickname);
	bool Match(const char *first, const char *second);
	void ChannelBan (string who, string mascara, string canal);
	void UnBan(string mascara, string canal);
	int GetCanales ();
	/* Operadores */
	int GetOperadores ();
	void SetOper (string nick);
	void DelOper (string nick);
	/* Memos */
	void InsertMemo (string sender, string receptor, long int fecha, string mensa);
	void DeleteMemos (string receptor);
};

class Semaforo
{
	private:
		mutex mtx;
		condition_variable cv;
		bool lock;
  
	public:
		Semaforo() {};
		void notify();
		void wait();
		void close();
		void open();
};
#endif
