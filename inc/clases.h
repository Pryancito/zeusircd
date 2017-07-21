#ifndef CLASES_H
#define CLASES_H

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <map>
#include <vector>

class Socket : public boost::enable_shared_from_this<Socket>
{
	private:
		boost::asio::ip::tcp::socket s_socket;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> s_ssl;
		std::string ip;
		int port;
		bool is_IPv6;
		bool is_SSL;
		bool tipo;
		bool quit;
		
	public:
		Socket(boost::asio::io_service & io_service, boost::asio::ssl::context & context) : s_socket(io_service), s_ssl(io_service, context) { quit = 0; };
		~Socket() noexcept(false) {};
		boost::thread *tw;
		bool GetIPv6();
		void SetIPv6(bool ipv6);
		bool GetSSL();
		void SetSSL(bool ssl);
		bool GetTipo();
		void SetTipo(bool tipo_);
		std::string GetIP();
		void SetIP(std::string ipe);
		int GetPort();
		void SetPort(int puerto);
		bool IsQuit();
		void Quit();
		void Close();
		void Conectar(std::string ip, std::string port);
		bool CheckDNSBL(std::string ip);
		bool CheckDNSBL6(std::string ip);
		boost::asio::ip::tcp::socket &GetSocket();
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &GetSSLSocket();
		void MainSocket ();
		void ServerSocket ();
		void Cliente (Socket *s);
		void Servidor (Socket *s);
		void Write (const std::string mensaje);
};

class User
{
	private:
		Socket *socket;
		std::string id;
		std::string nickname;
		std::string ident;
		std::string ip;
		std::string cloakip;
		time_t login;
		time_t lastping;
		std::string nodo;
		bool registered;
		bool tiene_r;
		bool tiene_z;
		bool tiene_o;
		bool tiene_w;
		
	public:
		User (Socket *sock, std::string id_) : socket(sock), id(id_) { nickname = "ZeusiRCd"; lastping = time(0); ident = "ZeusiRCd"; registered = false; tiene_r = tiene_z = tiene_o = tiene_w = false; };
		User (std::string id_) : id(id_) { nickname = "ZeusiRCd"; lastping = time(0); ident = "ZeusiRCd"; registered = false; tiene_r = tiene_z = tiene_o = tiene_w = false; socket = NULL; };
		User () {};
		~User () {};
		void SetNick(std::string nick);
		std::string GetNick();
		std::string GetID();
		void SetLastPing(time_t tiempo);
		time_t GetLastPing();
		void SetIdent(std::string ident_);
		std::string GetIdent();
		void ProcesaMensaje(Socket *s, std::string mensaje);
		void Bienvenida(Socket *s, std::string nickname);
		Socket *GetSocket(std::string nickname);
		User *GetUser(std::string id);
		User *GetUserByNick(std::string nickname);
		Socket *GetSocketByID(std::string id);
		std::string GetNickByID(std::string id);
		bool FindNick(std::string nickname);
		void Quit(User *u, Socket *s);
		std::string FullNick();
		void SetCloakIP(std::string ip);
		std::string GetIP();
		void SetIP(std::string ipe);
		std::string GetCloakIP();
		void SetReg(bool reg);
		bool GetReg();
		void SetNodo (std::string nodo_);
		void SetLogin (long int log);
		void SendSNICK();
		bool Tiene_Modo (char modo);
		void Fijar_Modo (char modo, bool tiene);
		bool Match(const char *first, const char *second);
		long int GetLogin ();
		std::string GetServer ();
		bool EsMio(std::string);
		void CheckMemos(Socket *s, User *u);
		std::string Time(time_t tiempo);
};

class Servidor
{
	private:
		Socket *socket;
		std::string id;
		std::string nombre;
		std::string ip;
		int saltos;
		std::string hub;
		int maxusers;
		int maxchannels;
		
	public:
		std::vector <std::string> connected;
		Servidor (Socket *sock, std::string id_) : socket(sock), id(id_) {};
		Servidor () {};
		~Servidor () {};
		bool IsAServer (std::string ip);
		bool IsConected (std::string ip);
		int GetID (std::string ip);
		void SendBurst (Socket *s);
		void ListServers (Socket *s);
		void ProcesaMensaje (Socket *s, std::string mensaje);
		void SendToAllServers (const std::string std);
		void SendToAllButOne (Socket *s, const std::string std);
		bool HUBExiste();
		bool SoyElHUB();
		void SQUIT(std::string id);
		void SQUIT(Socket *s);
		bool CheckClone(std::string ip);
		bool Existe(std::string id);
		void SetIP (std::string ip_);
		void SetNombre (std::string nombre_);
		std::string GetIP ();
		std::string GetNombre ();
		void AddLeaf(std::string);
		int GetSaltos ();
		void SetSaltos (int salt);
		std::string GetID ();
		Socket *GetSocket(std::string nombre);
};

class Chan
{
	private:
		std::string nombre;
		time_t creado;
		bool tiene_r;
		
	public:
		Chan (std::string name) : nombre(name) { creado = time(0); tiene_r = false; };
		Chan () {};
		~Chan () {};
		bool FindChan(std::string kanal);
		void Join (User *u, std::string canal);
		void Part (User *u, std::string canal);
		void PropagarJOIN (User *u, std::string canal);
		void PropagarPART (User *u, std::string canal);
		std::string GetNombre();
		void SendNAMES (User *u, std::string canal);
		bool IsInChan (User *u, std::string canal);
		bool IsEmpty(std::string canal);
		void DelChan(std::string canal);
		int MaxChannels(User *u);
		void PropagarMSG(User *u, std::string canal, std::string mensaje);
		void PropagarQUIT (User *u, std::string canal);
		void PropagarMODE(std::string who, std::string nickname, std::string canal, char modo, bool add);
		void PropagarNICK(User *u, std::string nuevo);
		void PropagarKICK (User *u, std::string canal, User *user, std::string motivo);
		void Lista (std::string canal, User *u);
		bool IsBanned(User *u, std::string canal);
		bool Tiene_Modo (char modo);
		void Fijar_Modo (char modo, bool tiene);
		void ChannelBan(std::string who, std::string mascara, std::string channel);
		void UnBan(std::string mascara, std::string channel);
};

class UserChan
{
	private:
		std::string id;
		std::string canal;
		char modo;
		
	public:
		UserChan (std::string id_, std::string chan) : id(id_), canal(chan) , modo('x') {};
		UserChan () {};
		~UserChan () {};
		std::string GetID();
		char GetModo();
		void SetModo(char mode);
		std::string GetNombre();
};

class BanChan
{
	private:
		std::string canal;
		std::string mascara;
		std::string who;
		time_t fecha;
		
	public:
		BanChan (std::string chan, std::string mask, std::string usr, time_t hora) : canal(chan), mascara(mask), who(usr), fecha(hora) { };
		~BanChan () {};
		std::string GetNombre();
		std::string GetMask();
		std::string GetWho();
		time_t GetTime();
};

template <class T>

class Node
{
    public:
        Node();
        Node(T);
        ~Node();
 
        Node *next;
        T data;
 
        void delete_all();
        void print();
};

template <class T>

class List
{
    public:
        List();
        ~List();
 
 		void add(T);
 		void del(T);
        void concat(List);
        void del_all();
        void intersection(List);
        void invert();
        void print();
        T search(T);
        void sort();
        T next(T);
        T first();
        int count();
 
    private:
        Node<T> *m_head;
        int m_num_nodes;
        std::map<T, Node<T>*> jash;
};

class Config
{
	public:
		std::map <std::string, std::string> conf;
		std::string version = "Zeus-1.0-RC2";
		std::string file = "server.conf";
		
	void Cargar ();
	void Procesa (std::string linea);
	void Configura (std::string dato, std::string valor);
	std::string Getvalue (std::string dato);
};

class Oper
{
	public:
		bool Login (User *u, std::string nickname, std::string pass);
		void GlobOPs (std::string mensaje);
		std::string MkPassWD (std::string pass);
		bool IsOper(User *u);
		void SetModeO (User *u);
		int Count ();
};

class DB
{
	public:
		void AlmacenaDB(std::string cadena);
		void BorraDB(std::string id);
		std::string GetLastRecord ();
		void IniciarDB();
		std::string SQLiteReturnString (std::string sql);
		int SQLiteReturnInt (std::string sql);
		bool SQLiteNoReturn (std::string sql);
		int Sync(Socket *s, std::string id);
		std::vector <std::string> SQLiteReturnVector (std::string sql);
		std::string GenerateID();
};

class NickServ
{
	public:
		void ProcesaMensaje(Socket *s, User *u, std::string mensaje);
		bool IsRegistered(std::string nickname);
		bool Login (std::string nickname, std::string pass);
		int GetNicks();
		bool GetOption(std::string option, std::string nickname);
		void CheckMemos (User *u);
		void UpdateLogin (User *u);
		std::string GetvHost (std::string nickname);
};

class ChanServ
{
	public:
		void ProcesaMensaje(Socket *s, User *u, std::string mensaje);
		bool IsRegistered(std::string channel);
		bool IsFounder(std::string nickname, std::string channel);
		int Access (std::string nickname, std::string channel);
		void CheckModes(User *u, std::string channel);
		bool IsAKICK(std::string mascara, std::string canal);
		bool CheckKEY(std::string canal, std::string key);
		bool IsKEY(std::string canal);
		int GetChans();
};

class OperServ
{
	public:
		void ProcesaMensaje(Socket *s, User *u, std::string mensaje);
		bool IsGlined(std::string ip);
		void ApplyGlines ();
};

class Memo
{
	public:
		std::string sender;
		std::string receptor;
		time_t time;
		std::string mensaje;
};

class GLine
{
	private:
		std::string ip;
		std::string who;
		std::string reason;
		
	public:
		GLine() {};
		struct fw_rule GetRule();
		std::string GetIP();
		std::string GetWho();
		std::string GetReason();
		void SetIP(std::string ipe);
		void SetWho(std::string whois);
		void SetReason (std::string motivo);
};

#endif
