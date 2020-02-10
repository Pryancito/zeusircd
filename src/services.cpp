#include "ZeusBaseClass.h"
#include "module.h"
#include "db.h"
#include "Server.h"
#include "oper.h"
#include "Utils.h"
#include "services.h"
#include "base64.h"
#include "Config.h"
#include "mainframe.h"
#include "sha256.h"

using namespace std;

Memos MemoMsg;

void ChanServ::DoRegister(LocalUser *user, Channel *chan) {
	string sql = "SELECT TOPIC FROM CANALES WHERE NOMBRE='" + chan->name() + "';";
	string topic = DB::SQLiteReturnString(sql);
	sql = "SELECT REGISTERED FROM CANALES WHERE NOMBRE='" + chan->name() + "';";
	int creado = DB::SQLiteReturnInt(sql);
	if (topic != "") {
		topic = decode_base64(topic);
		chan->mTopic = topic;
		user->SendAsServer("332 " + user->mNickName + " " + chan->name() + " :" + topic);
		user->SendAsServer("333 " + user->mNickName + " " + chan->name() + " " + config["chanserv"].as<std::string>() + " " + std::to_string(creado));
	}
	if (chan->getMode('r') == false) {
		chan->setMode('r', true);
		user->Send(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " +r");
		Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " +r");
	}
	user->SendAsServer("324 " + user->mNickName + " " + chan->name() + " +r");
	user->SendAsServer("329 " + user->mNickName + " " + chan->name() + " " + std::to_string(creado));
}

int ChanServ::HasMode(const string &canal, string mode) {
	std::transform(mode.begin(), mode.end(), mode.begin(), ::toupper);
	string sql;
	if (mode == "COUNTRY") {
		sql = "SELECT " + mode + " FROM CMODES WHERE CANAL='" + canal + "';";
		return (DB::SQLiteReturnString(sql) != "");
	} else {
		sql = "SELECT " + mode + " FROM CMODES WHERE CANAL='" + canal + "';";
		return (DB::SQLiteReturnInt(sql));
	}
}

void ChanServ::CheckModes(LocalUser *user, const string &channel) {
	if (NickServ::GetOption("NOOP", user->mNickName) == true)
		return;
	Channel* chan = Mainframe::instance()->getChannelByName(channel);
	if (!chan)
		return;
	if (!chan->hasUser(user))
		return;
	int access = ChanServ::Access(user->mNickName, chan->name());
	if (HasMode(channel, "AUTOVOICE") && access < 1) access = 1;
	
	if (chan->isVoice(user) == true) {
		if (access < 1) {
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " -v " + user->mNickName);
			chan->delVoice(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " -v " + user->mNickName);
		} else if (access == 2) {
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " -v " + user->mNickName);
			chan->delVoice(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " -v " + user->mNickName);
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " +h " + user->mNickName);
			chan->giveHalfOperator(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " +h " + user->mNickName);
		} else if (access > 2) {
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " -v " + user->mNickName);
			chan->delVoice(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " -v " + user->mNickName);
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " +o " + user->mNickName);
			chan->giveOperator(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " +o " + user->mNickName);
		}
	} else if (chan->isHalfOperator(user) == true) {
		if (access < 2) {
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " -h " + user->mNickName);
			chan->delHalfOperator(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " -h " + user->mNickName);
		} else if (access > 2) {
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " -h " + user->mNickName);
			chan->delHalfOperator(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " -h " + user->mNickName);
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " +o " + user->mNickName);
			chan->giveOperator(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " +o " + user->mNickName);
		}
	} else if (chan->isOperator(user) == true) {
		if (access < 3) {
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " -o " + user->mNickName);
			chan->delOperator(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " -o " + user->mNickName);
		}
	} else {
		if (access == 1) {
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " +v " + user->mNickName);
			chan->giveVoice(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " +v " + user->mNickName);
		} else if (access == 2) {
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " +h " + user->mNickName);
			chan->giveHalfOperator(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " +h " + user->mNickName);
		} else if (access > 2) {
			chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name() + " +o " + user->mNickName);
			chan->giveOperator(user);
			Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name() + " +o " + user->mNickName);
		}
	}
}

bool ChanServ::IsRegistered(string channel) {
	string sql = "SELECT NOMBRE from CANALES WHERE NOMBRE='" + channel + "';";
	string retorno = DB::SQLiteReturnString(sql);
	if (strcasecmp(retorno.c_str(), channel.c_str()) == 0)
		return true;
	else
		return false;
}

bool ChanServ::IsFounder(string nickname, string channel) {
	string sql = "SELECT OWNER from CANALES WHERE NOMBRE='" + channel + "';";
	string retorno = DB::SQLiteReturnString(sql);
	if (strcasecmp(retorno.c_str(), nickname.c_str()) == 0)
		return true;
	else
		return false;
}

int ChanServ::Access (string nickname, string channel) {
	string sql = "SELECT ACCESO from ACCESS WHERE USUARIO='" + nickname + "'  AND CANAL='" + channel + "';";
	string retorno = DB::SQLiteReturnString(sql);
	LocalUser* user = Mainframe::instance()->getLocalUserByName(nickname);
	if (strcasecmp(retorno.c_str(), "VOP") == 0)
		return 1;
	else if (strcasecmp(retorno.c_str(), "HOP") == 0)
		return 2;
	else if (strcasecmp(retorno.c_str(), "AOP") == 0)
		return 3;
	else if (strcasecmp(retorno.c_str(), "SOP") == 0)
		return 4;
	else if (ChanServ::IsFounder(nickname, channel) == true)
		return 5;
	else if (user)
		if (user->getMode('o') == true)
			return 5;
	return 0;
}

bool ChanServ::IsAKICK(string mascara, const string &canal) {
	vector <string> akicks;
	std::transform(mascara.begin(), mascara.end(), mascara.begin(), ::tolower);
	string sql = "SELECT MASCARA from AKICK WHERE CANAL='" + canal + "';";
	akicks = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < akicks.size(); i++) {
		std::transform(akicks[i].begin(), akicks[i].end(), akicks[i].begin(), ::tolower);
		if (Utils::Match(akicks[i].c_str(), mascara.c_str()) == 1)
			return true;
	}
	return false;
}

bool ChanServ::CheckKEY(const string &canal, string key) {
	string sql = "SELECT CLAVE from CANALES WHERE NOMBRE='" + canal + "';";
	string retorno = DB::SQLiteReturnString(sql);
	if (retorno.length() == 0 || key.length() == 0)
		return true;
	if (retorno == key)
		return true;
	else
		return false;
}

bool ChanServ::IsKEY(const string &canal) {
	string sql = "SELECT CLAVE from CANALES WHERE NOMBRE='" + canal + "';";
	string retorno = DB::SQLiteReturnString(sql);
	return (retorno.length() > 0);
}

int ChanServ::GetChans () {
	string sql = "SELECT COUNT(*) FROM CANALES;";
	return DB::SQLiteReturnInt(sql);
}

bool ChanServ::CanRegister(LocalUser *user, string channel) {
	string sql = "SELECT COUNT(*) FROM CANALES WHERE FOUNDER='" + user->mNickName + "';";
	int channels = DB::SQLiteReturnInt(sql);
	if (channels >= config["maxchannels"].as<int>())
		return false;

	Channel* chan = Mainframe::instance()->getChannelByName(channel);
	if (chan)
		return (chan->hasUser(user) && chan->isOperator(user));

	return false;
}

bool ChanServ::CanGeoIP(LocalUser *user, string channel) {
	std::string country = Utils::GetGeoIP(user->mHost);
	std::string sql = "SELECT COUNTRY FROM CMODES WHERE CANAL = '" + channel + "';";
	std::string dbcountry = DB::SQLiteReturnString(sql);
	if (dbcountry == "")
		return true;
	return (strcasecmp(dbcountry.c_str(), country.c_str()));
}


void NickServ::UpdateLogin (LocalUser *user) {
	if (Server::HUBExiste() == false) return;

	string sql = "UPDATE NICKS SET LASTUSED=" + std::to_string(time(0)) + " WHERE NICKNAME='" + user->mNickName + "';";
	if (DB::SQLiteNoReturn(sql) == false) return;
	if (config["database"]["cluster"].as<bool>() == false) {
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Server::Send(sql);
	}
	return;
}

bool NickServ::IsRegistered(string nickname) {
	if (nickname.empty())
		return false;
	string sql = "SELECT NICKNAME from NICKS WHERE NICKNAME='" + nickname + "';";
	string retorno = DB::SQLiteReturnString(sql);
	return (strcasecmp(retorno.c_str(), nickname.c_str()) == 0);
} 

bool NickServ::Login (const string &nickname, const string &pass) {
	string sql = "SELECT PASS from NICKS WHERE NICKNAME='" + nickname + "';";
	string retorno = DB::SQLiteReturnString(sql);
	return (retorno == sha256(pass));
}

int NickServ::GetNicks () {
	string sql = "SELECT COUNT(*) FROM NICKS;";
	return DB::SQLiteReturnInt(sql);
}

bool NickServ::GetOption(const string &option, string nickname) {
	if (IsRegistered(nickname) == false)
		return false;
	string sql = "SELECT " + option + " FROM OPTIONS WHERE NICKNAME='" + nickname + "';";
	return DB::SQLiteReturnInt(sql);
}

std::string NickServ::GetLang(string nickname) {
	if (IsRegistered(nickname) == false)
		return "";
	string sql = "SELECT LANG FROM OPTIONS WHERE NICKNAME='" + nickname + "';";
	return DB::SQLiteReturnString(sql);
}

string NickServ::GetvHost (string nickname) {
	if (IsRegistered(nickname) == false)
		return "";
	string sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + nickname + "';";
	return DB::SQLiteReturnString(sql);
}

int NickServ::MemoNumber(const std::string& nick) {
	int i = 0;
	Memos::iterator it = MemoMsg.begin();
	for(; it != MemoMsg.end(); ++it)
		if (strcasecmp((*it)->receptor.c_str(), nick.c_str()) == 0)
			i++;
	return i;
}

void NickServ::checkmemos(LocalUser* user) {
	if (user->getMode('r') == false)
		return;
    Memos::iterator it = MemoMsg.begin();
    while (it != MemoMsg.end()) {
		if (strcasecmp((*it)->receptor.c_str(), user->mNickName.c_str()) == 0) {
			struct tm tm;
			localtime_r(&(*it)->time, &tm);
			char date[32];
			strftime(date, sizeof(date), "%r %d-%m-%Y", &tm);
			string fecha = date;
			user->Send(":" + config["nickserv"].as<std::string>() + " PRIVMSG " + user->mNickName + " :" + Utils::make_string(user->mLang, "Memo from: %s Received %s Message: %s", (*it)->sender.c_str(), fecha.c_str(), (*it)->mensaje.c_str()));
			it = MemoMsg.erase(it);
		} else
			++it;
    }
    Server::Send("MEMODEL " + user->mNickName);
}

int HostServ::HowManyPaths(const string &nickname) {
	string sql = "SELECT COUNT(*) from PATHS WHERE OWNER='" + nickname + "';";
	return DB::SQLiteReturnInt(sql);
}

bool HostServ::CheckPath(string path) {
	std::vector<std::string> subpaths;
	Utils::split(path, subpaths, "/");
	if (subpaths.size() < 1 || subpaths.size() > 10)
		return false;
	for (unsigned int i = 0; i < subpaths.size(); i++) {
		if (subpaths[i].length() == 0 || subpaths[i].length() > 32)
			return false;
		else if (Utils::checkstring(subpaths[i]) == false)
			return false;
	}
	return true;
}

bool HostServ::IsReqRegistered(string path) {
	std::vector<std::string> subpaths;
	Utils::split(path, subpaths, "/");
	string pp = "";
	for (unsigned int i = 0; i < subpaths.size(); i++) {
		pp.append(subpaths[i]);
		string sql = "SELECT PATH from PATHS WHERE PATH='" + pp + "';";
		string retorno = DB::SQLiteReturnString(sql);
		if (retorno.empty())
			return false;
		pp.append("/");
	}
	return true;
}

bool HostServ::Owns(LocalUser *user, string path) {
	std::vector<std::string> subpaths;
	Utils::split(path, subpaths, "/");
	string pp = "";
	for (unsigned int i = 0; i < subpaths.size(); i++) {
		pp.append(subpaths[i]);
		string sql = "SELECT OWNER from PATHS WHERE PATH='" + pp + "';";
		string retorno = DB::SQLiteReturnString(sql);
		if (strcasecmp(retorno.c_str(), user->mNickName.c_str()) == 0)
			return true;
		else if (user->getMode('o') == true)
			return true;
		pp.append("/");
	}
	return false;
}

bool HostServ::DeletePath(string &path) {
	if (path.back() != '/')
		path.append("/");
	string sql = "SELECT PATH from PATHS WHERE PATH LIKE '" + path + "%';";
	std::vector<std::string> retorno = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < retorno.size(); i++) {
		string sql = "DELETE FROM PATHS WHERE PATH='" + retorno[i] + "';";
		if (DB::SQLiteNoReturn(sql) == false) {
			return false;
		}
		if (config["database"]["cluster"].as<bool>() == false) {
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Server::Send(sql);
		}
	}
	path.erase( path.end()-1 );
	sql = "DELETE FROM PATHS WHERE PATH='" + path + "';";
	if (DB::SQLiteNoReturn(sql) == false) {
		return false;
	}
	if (config["database"]["cluster"].as<bool>() == false) {
		sql = "DB " + DB::GenerateID() + " " + sql;
		DB::AlmacenaDB(sql);
		Server::Send(sql);
	}
	sql = "SELECT NICKNAME from NICKS WHERE VHOST LIKE '" + path + "/%';";
	retorno = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < retorno.size(); i++) {
		string sql = "UPDATE NICKS SET VHOST='' WHERE NICKNAME='" + retorno[i] + "';";
		if (DB::SQLiteNoReturn(sql) == false) {
			return false;
		}
		if (config["database"]["cluster"].as<bool>() == false) {
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Server::Send(sql);
		}
		if (Mainframe::instance()->doesNicknameExists(retorno[i]) == true) {
			LocalUser *target = Mainframe::instance()->getLocalUserByName(retorno[i]);
			if (target) {
				target->Cycle();
			} else {
				Server::Send("VHOST " + retorno[i]);
			}
		}
	}
	return true;
}

bool HostServ::GotRequest (string user) {
	string sql = "SELECT OWNER from REQUEST WHERE OWNER='" + user + "';";
	string retorno = DB::SQLiteReturnString(sql);
	if (strcasecmp(retorno.c_str(), user.c_str()) == 0)
		return true;
	else
		return false;
}

bool HostServ::PathIsInvalid (string path) {
	std::string sql = "SELECT VHOST from NICKS WHERE VHOST='" + path + "';";
	std::string retorno = DB::SQLiteReturnString(sql);
	if (strcasecmp(retorno.c_str(), path.c_str()) == 0)
		return true;
	else
		return false;
}

bool OperServ::IsGlined(string ip) {
	std::string sql = "SELECT IP from GLINE WHERE IP='" + ip + "';";
	std::string retorno = DB::SQLiteReturnString(sql);
	if (strcasecmp(retorno.c_str(), ip.c_str()) == 0)
		return true;
	else
		return false;
}

std::string OperServ::ReasonGlined(const string &ip) {
	std::string sql = "SELECT MOTIVO from GLINE WHERE IP='" + ip + "';";
	return DB::SQLiteReturnString(sql);
}

bool OperServ::IsTGlined(string ip) {
	std::string sql = "SELECT IP from TGLINE WHERE IP='" + ip + "';";
	std::string retorno = DB::SQLiteReturnString(sql);
	if (strcasecmp(retorno.c_str(), ip.c_str()) == 0)
		return true;
	else
		return false;
}

void OperServ::ExpireTGline () {
	std::string sql = "SELECT IP from TGLINE WHERE (TIME + EXPIRE) < " + std::to_string(time(0)) + ";";
	std::vector<std::string> ips = DB::SQLiteReturnVector(sql);

	for (unsigned int i = 0; i < ips.size(); i++) {
		Oper oper;
		std::string sql = "DELETE FROM TGLINE WHERE IP='" + ips[i] + "';";
		if (DB::SQLiteNoReturn(sql) == true)
			oper.GlobOPs("The TGLINE for ip: " + ips[i] + " has expired now.");
		else
			oper.GlobOPs("Error expiring TGLINE for ip: " + ips[i] + ".");
		if (config["database"]["cluster"].as<bool>() == false) {
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Server::Send(sql);
		}
	}
}

std::string OperServ::ReasonTGlined(const string &ip) {
	std::string sql = "SELECT MOTIVO from TGLINE WHERE IP='" + ip + "';";
	return DB::SQLiteReturnString(sql);
}

bool OperServ::IsOper(string nick) {
	std::string sql = "SELECT NICK from OPERS WHERE NICK='" + nick + "';";
	std::string retorno = DB::SQLiteReturnString(sql);
	if (strcasecmp(retorno.c_str(), nick.c_str()) == 0)
		return true;
	else
		return false;
}

bool OperServ::IsSpammed(string mask) {
	std::vector<std::string> vect;
	std::string sql = "SELECT MASK from SPAM;";
	vect = DB::SQLiteReturnVector(sql);
	std::transform(mask.begin(), mask.end(), mask.begin(), ::tolower);
	for (unsigned int i = 0; i < vect.size(); i++) {
		std::transform(vect[i].begin(), vect[i].end(), vect[i].begin(), ::tolower);
		if (Utils::Match(vect[i].c_str(), mask.c_str()) == true)
			return true;
	}
	return false;
}

bool OperServ::IsSpam(string mask, string flags) {
	std::vector<std::string> vect;
	std::transform(flags.begin(), flags.end(), flags.begin(), ::tolower);
	std::string sql = "SELECT MASK from SPAM WHERE TARGET LIKE '%" + flags + "%' COLLATE NOCASE;";
	vect = DB::SQLiteReturnVector(sql);
	std::transform(mask.begin(), mask.end(), mask.begin(), ::tolower);
	for (unsigned int i = 0; i < vect.size(); i++) {
		std::transform(vect[i].begin(), vect[i].end(), vect[i].begin(), ::tolower);
		if (Utils::Match(vect[i].c_str(), mask.c_str()) == true)
			return true;
	}
	return false;
}

int OperServ::IsException(std::string ip, std::string option) {
	std::transform(option.begin(), option.end(), option.begin(), ::tolower);
	std::string sql = "SELECT VALUE from EXCEPTIONS WHERE IP='" + ip + "' AND OPTION='" + option + "';";
	return DB::SQLiteReturnInt(sql);
}

bool OperServ::CanGeoIP(std::string ip) {
	std::vector<std::string> vect;
	if (IsException(ip, "geoip") == 1)
		return true;
	if (config["GeoIP-ALLOWED"]) {
		std::string allowed = config["GeoIP-ALLOWED"].as<std::string>();
		std::string country = Utils::GetGeoIP(ip);
		if (allowed.length() > 0) {
			Utils::split(allowed, vect, ",");
			for (unsigned int i = 0; i < vect.size(); i++)
				if (strcasecmp(country.c_str(), vect[i].c_str()) == 0)
					return true;
			return false;
		}
	}
	if (config["GeoIP-DENIED"]) {
		std::string denied = config["GeoIP-DENIED"].as<std::string>();
		std::string country = Utils::GetGeoIP(ip);
		if (denied.length() > 0) {
			Utils::split(denied, vect, ",");
			for (unsigned int i = 0; i < vect.size(); i++)
				if (strcasecmp(country.c_str(), vect[i].c_str()) == 0)
					return false;
			return true;
		}
	}
	return true;
}
