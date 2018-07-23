#pragma once

#include <string>

#include "user.h"

class NickServ
{
	public:
		static void Message(User *user, std::string mensaje);
		static bool IsRegistered(std::string nickname);
		static bool Login (std::string nickname, std::string pass);
		static int GetNicks();
		static bool GetOption(std::string option, std::string nickname);
		static void UpdateLogin (User *user);
		static std::string GetvHost (std::string nickname);
		static int MemoNumber(const std::string& nick);
		static void checkmemos(User *user);
		static std::string GetLang (std::string nickname);
};

class ChanServ
{
	public:
		static void Message(User *user, std::string mensaje);
		static bool IsRegistered(std::string channel);
		static bool IsFounder(std::string nickname, std::string channel);
		static int Access (std::string nickname, std::string channel);
		static void CheckModes(User *user, std::string channel);
		static bool IsAKICK(std::string mascara, std::string canal);
		static bool CheckKEY(std::string canal, std::string key);
		static bool IsKEY(std::string canal);
		static int GetChans();
		static int HasMode(std::string canal, std::string mode);
		static bool CanRegister(User *user, std::string channel);
		static void DoRegister(User *user, Channel *chan);
};

class HostServ
{
	public:
		static void Message(User *user, std::string message);
		static bool CheckPath(std::string path);
		static bool Owns(User *u, std::string path);
		static bool DeletePath(std::string path);
		static bool PathIsInvalid (std::string path);
		static bool GotRequest (std::string user);
		const char *pseudoClient(void);
};

class OperServ
{
	public:
		static void Message(User *user, std::string message);
		static bool IsGlined(std::string ip);
		static bool IsSpammed(std::string mask);
		static bool IsSpam(std::string mask, std::string flags);
		static void ApplyGlines ();
};

class Memo
{
	public:
		std::string sender;
		std::string receptor;
		time_t time;
		std::string mensaje;
};

typedef std::set<Memo*> Memos;

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
