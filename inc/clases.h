#ifndef CLASES_H
#define CLASES_H

#include <string>
#include <thread>
#include <map>
#include <vector>

using namespace std;

class Socket
{
	public:
		char *ip;
		int port;
		std::thread tw;
		TCPStream *stream;

	void MainSocket ();
	void ServerSocket ();
	void Cliente (TCPStream* s);
	void Servidor (TCPStream* s);
	std::thread Thread(TCPStream* s);
	std::thread SThread(TCPStream* s);
	std::thread MainThread();
	std::thread ServerThread();
	void Write (TCPStream *stream, const string mensaje);
};

class Config
{
	public:
		std::map <std::string, std::string> conf;
		std::string version = "Zeus-1.0-Alpha3";
		
	void Cargar ();
	void Procesa (std::string linea);
	void Configura (std::string dato, std::string valor);
	std::string Getvalue (std::string dato);
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
    
    bool Existe (string _nick);
    bool Conectado (int ID);
    void CambioDeNick (int ID, string nuevonick);
    string FullNick (int ID);
    string GetNick(int ID);
    string GetIdent(int ID);
    string GetIP(int ID);
    string GetCloakIP(int ID);
    void SetIdent(int ID, string _ident);
    bool Registrado (int ID);
	void Conectar (int ID);
	long int Creado(string _nick);
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
};

class Cliente
{
	public:
	
	bool ProcesaMensaje (TCPStream* stream, std::string mensaje);
	void Bienvenida (TCPStream* stream, std::string nickname);
};

class Chan
{
	public:
		std::string nombre;
		std::string modos;
		std::vector <std::string> usuarios;
		std::vector <char> umodes;
		time_t creado;

	
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
};

class Data
{
	public:
		std::vector <Socket*> sockets;
		std::vector <Nick*> nicks;
		std::vector <Server*> servers;
		std::vector <Chan*> canales;
		std::vector <Oper*> operadores;
	
	/* Sockets */
	Socket* CrearSocket();
	void CerrarSocket(TCPStream *stream);
	int BuscarSocket(TCPStream *stream);
	/* Nicks */
	void CrearNick (TCPStream *stream, std::string nick);
	void BorrarNick (TCPStream *stream);
	void BorrarNickByNick(string nickname);
	int BuscarIDNick (std::string nick);
	int BuscarIDStream (TCPStream *stream);
	TCPStream *BuscarStream(std::string nick);
	void SNICK(string nickname, string ip, string cloakip, long int creado, string nodo);
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
	int GetCanales ();
	/* Operadores */
	int GetOperadores ();
	void SetOper (string nick);
	void DelOper (string nick);
};

#endif
