#include "server.h"
#include "mainframe.h"
#include "oper.h"
#include "session.h"
#include "db.h"
#include "services.h"
#include "user.h"

extern CloneMap mClones;
extern Memos MemoMsg;

void Servidor::Message(Servidor *server, std::string message) {
	StrVec  x;
	boost::split(x, message, boost::is_any_of(" \t"), boost::token_compress_on);
	std::string cmd = x[0];
	boost::to_upper(cmd);
	Oper oper;

	if (cmd == "HUB") {
		if (x.size() < 2) {
			oper.GlobOPs("No hay HUB, cerrando conexion.");
			server->close();
			return;
		} else if (!boost::iequals(x[1], config->Getvalue("hub"))) {
			oper.GlobOPs("Cerrando conexion. Los HUB no coinciden. ( " + config->Getvalue("hub") + " > " + x[1] + " )");
			server->close();
			return;
		}
	} else if (cmd == "VERSION") {
		if (x.size() < 2) {
			oper.GlobOPs("Error en las BDDs, cerrando conexion.");
			server->close();
			return;
		} else if (DB::GetLastRecord() != x[1] && Server::HUBExiste() == true) {
				oper.GlobOPs("Sincronizando BDDs.");
				int syn = DB::Sync(server, x[1]);
				oper.GlobOPs("BDDs sincronizadas, se actualizaron: " + std::to_string(syn) + " registros.");
				return;
		}
	} else if (cmd == "DB") {
		std::string sql = message.substr(20);
		DB::SQLiteNoReturn(sql);
		DB::AlmacenaDB(message);
		Servidor::sendallbutone(server, message);
	} else if (cmd == "SERVER") {
		std::vector <std::string> conexiones;
		if (x.size() < 3) {
			oper.GlobOPs("ERROR: SERVER invalido. Cerrando conexion.");
			server->close();
			return;
		} else if (Servidor::Exists(x[1]) == false) {
			for (unsigned int i = 3; i < x.size(); ++i) { conexiones.push_back(x[i]); }
			if (server->ip() == x[2]) {
				Servidor::addServer(server, x[1], x[2], conexiones);
				server->setname(x[1]);
			} else
				Servidor::addServer(nullptr, x[1], x[2], conexiones);
			Servidor::addLink(config->Getvalue("serverName"), x[1]);
			Servidor::sendallbutone(server, "SLINK " + config->Getvalue("serverName") + " " + x[1]);
			Servidor::sendallbutone(server, message);
		}
	} else if (cmd == "SLINK") {
		if (x.size() < 3) {
			oper.GlobOPs("ERROR: SLINK invalido.");
			return;
		} else
			Servidor::addLink(x[1], x[2]);
	} else if (cmd == "SNICK") {
		if (x.size() < 8) {
			oper.GlobOPs("ERROR: SNICK invalido.");
			return;
		}
		User* target = Mainframe::instance()->getUserByName(x[1]);
		if (target) {
			if (target->server() == config->Getvalue("serverName"))
				target->QUIT();
			else
				Servidor::sendall("COLLISSION " + x[1]);
		} else if (!target) {
			User *user = new User(nullptr, x[6]);
			boost::to_lower(x[1]);
			user->SNICK(x[1], x[2], x[3], x[4], x[5], x[7]);
			Server::CloneUP(x[3]);
			if (!Mainframe::instance()->addUser(user, x[1]))
				oper.GlobOPs("ERROR: No se pudo introducir el usuario " + x[1] + " mediante el comando SNICK.");
			else
				Servidor::sendallbutone(server, message);
		}
	} else if (cmd == "SUSER") {
		if (x.size() < 3) {
			oper.GlobOPs("ERROR: SUSER invalido.");
			return;
		}
		User* target = Mainframe::instance()->getUserByName(x[1]);
		if (target)
			target->SUSER(x[2]);
			
		Servidor::sendallbutone(server, message);
	} else if (cmd == "COLLISSION") {
		if (x.size() < 2) {
			oper.GlobOPs("ERROR: COLLISSION invalido.");
			return;
		}
		User* target = Mainframe::instance()->getUserByName(x[1]);
		if (target) {
			if (target->server() == config->Getvalue("serverName"))
				target->cmdQuit();
			else
				Servidor::sendallbutone(server, message);
		}
	} else if (cmd == "SBAN") {
		if (x.size() < 5) {
			oper.GlobOPs("ERROR: SBAN invalido.");
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
		if (chan) {
			chan->SBAN(x[2], x[3], x[4]);
			Servidor::sendallbutone(server, message);
		} else
			oper.GlobOPs("ERROR: Canal inexistente en SBAN.");
	} else if (cmd == "SJOIN") {
		if (x.size() < 4) {
			oper.GlobOPs("ERROR: SJOIN invalido.");
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(x[2]);
		User* user = Mainframe::instance()->getUserByName(x[1]);
		if (!user) {
			oper.GlobOPs("ERROR: Usuario de SJOIN invalido.");
			return;
		} if (chan) {
			user->SJOIN(chan);
		} else {
			chan = new Channel(user, x[2]);
			if (chan) {
				user->SJOIN(chan);
				Mainframe::instance()->addChannel(chan);
			}
		} if (x[3][1] != 'x') {
			if (x[3][1] == 'o') {
				chan->giveOperator(user);
				chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + user->nick() + config->EOFMessage);
			} else if (x[3][1] == 'h') {
				chan->giveHalfOperator(user);
				chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +h " + user->nick() + config->EOFMessage);
			} else if (x[3][1] == 'v') {
				chan->giveVoice(user);
				chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +v " + user->nick() + config->EOFMessage);
			}
		}
		Servidor::sendallbutone(server, message);
	} else if (cmd == "SPART") {
		if (x.size() < 3) {
			oper.GlobOPs("ERROR: SPART invalido.");
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(x[2]);
		User* user = Mainframe::instance()->getUserByName(x[1]);
		if (chan && user) {
			user->cmdPart(chan);
			if (chan->userCount() == 0)
				Mainframe::instance()->removeChannel(chan->name());
		} else
			oper.GlobOPs("ERROR: Usuario o canal invalido en SPART.");
	} else if (cmd == "UMODE") {
		if (x.size() < 3) {
			oper.GlobOPs("ERROR: UMODE invalido.");
			return;
		}
		User* user = Mainframe::instance()->getUserByName(x[1]);
		bool add = false;
		if (x[2][0] == '+')
			add = true;
		if (x[2][1] == 'o')
			user->setMode('o', add);
		else if (x[2][1] == 'w')
			user->setMode('w', add);
		else if (x[2][1] == 'r')
			user->setMode('r', add);
		else if (x[2][1] == 'z')
			user->setMode('z', add);
		Servidor::sendallbutone(server, message);
	} else if (cmd == "CMODE") {
		if (x.size() < 4) {
			oper.GlobOPs("ERROR: CMODE invalido.");
			return;
		}
		User* target;
		if (x.size() == 5)
			target = Mainframe::instance()->getUserByName(x[4]);
		Channel* chan = Mainframe::instance()->getChannelByName(x[2]);
		bool add = false;
		if (x[3][0] == '+')
			add = true;
		if ((!target && x[3][1] != 'b' && x[3][1] != 'r') || !chan) {
			oper.GlobOPs("ERROR: CMODE sobre un usuario o nick invalido.");
			return;
		} if (x[3][1] == 'o' && add == true) {
			chan->giveOperator(target);
			chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +o " + target->nick() + config->EOFMessage);
		} else if (x[3][1] == 'o' && add == false) {
			chan->delOperator(target);
			chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
		} else if (x[3][1] == 'h' && add == true) {
			chan->giveHalfOperator(target);
			chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +h " + target->nick() + config->EOFMessage);
		} else if (x[3][1] == 'h' && add == false) {
			chan->delHalfOperator(target);
			chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
		} else if (x[3][1] == 'v') {
			chan->giveVoice(target);
			chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +v " + target->nick() + config->EOFMessage);
		} else if (x[3][1] == 'v' && add == false) {
			chan->delVoice(target);
			chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
		} else if (x[3][1] == 'b' && add == true) {
			chan->setBan(x[4], x[1]);
			chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +b " + x[4] + config->EOFMessage);
		} else if (x[3][1] == 'b' && add == false) {
			BanSet bans = chan->bans();
			BanSet::iterator it = bans.begin();
			for (; it != bans.end(); ++it) {
				if ((*it)->mask() == x[4]) {
					chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -b " + x[4] + config->EOFMessage);
					chan->UnBan(*it);
				}
			}
		} else if (x[3][1] == 'r' && add == true) {
			chan->setMode('r', true);
			chan->broadcast(":" + x[1] + " MODE " + chan->name() + " +r" + config->EOFMessage);
		} else if (x[3][1] == 'r' && add == false) {
			chan->setMode('r', false);
			chan->broadcast(":" + x[1] + " MODE " + chan->name() + " -r" + config->EOFMessage);
		}
		Servidor::sendallbutone(server, message);
	} else if (cmd == "QUIT") {
		if (x.size() < 2) {
			oper.GlobOPs("ERROR: QUIT invalido.");
			return;
		}
		User* target = Mainframe::instance()->getUserByName(x[1]);
		if (!target) {
			oper.GlobOPs("ERROR: QUIT de un usuario desconocido.");
			return;
		} else
			target->QUIT();
		Servidor::sendallbutone(server, message);
	} else if (cmd == "NICK") {
		if (x.size() < 3) {
			oper.GlobOPs("ERROR: NICK invalido.");
			return;
		}
		if(Mainframe::instance()->changeNickname(x[1], x[2])) {
			User* user = Mainframe::instance()->getUserByName(x[2]);
			if (!user) {
				oper.GlobOPs("ERROR: NICK sobre un usuario invalido.");
				return;
			}
			user->propagatenick(x[2]);
			user->setNick(x[2]);
			Servidor::sendallbutone(server, message);
        } else {
			oper.GlobOPs("ERROR: error en el cambio de nick de " + x[1] + " a " + x[2]);
			return;
		}
	} else if (cmd == "SQUIT") {
		if (x.size() < 2) {
			oper.GlobOPs("ERROR: SQUIT invalido.");
			return;
		} else if (boost::iequals(x[1], config->Getvalue("serverName"))) {
			Servidor::sendallbutone(server, "SQUIT " + server->name());
			Servidor::SQUIT(server->name());
		} else {
			Servidor::sendallbutone(server, message);
			Servidor::SQUIT(x[1]);
		}			
	} else if (cmd == "PRIVMSG" || cmd == "NOTICE") {
		if (x.size() < 4) {
			oper.GlobOPs("ERROR: PRIVMSG|NOTICE invalido.");
			return;
		}
		std::string mensaje = "";
		for (unsigned int i = 3; i < x.size(); ++i) { mensaje += x[i] + " "; }
		if (x[2][0] == '#') {
			Channel* chan = Mainframe::instance()->getChannelByName(x[2]);
			if (chan) {
				chan->broadcast(
					":" + x[1] + " "
					+ x[0] + " "
					+ chan->name() + " "
					+ mensaje + config->EOFMessage);
			}
		}
		else {
			User* target = Mainframe::instance()->getUserByName(x[2]);
			if (target && target->server() == config->Getvalue("serverName")) {
				target->session()->send(":" + x[1] + " "
					+ x[0] + " "
					+ target->nick() + " "
					+ message + config->EOFMessage);
			}
		}
		Servidor::sendallbutone(server, message);
	} else if (cmd == "SKICK") {
		if (x.size() < 5) {
			oper.GlobOPs("ERROR: SKICK invalido.");
			return;
		}
		std::string reason = "";
		User*  user = Mainframe::instance()->getUserByName(x[1]);
		Channel* chan = Mainframe::instance()->getChannelByName(x[2]);
		User*  victim = Mainframe::instance()->getUserByName(x[3]);
		if (chan && user && victim) {
			user->cmdKick(victim, reason, chan);
			victim->SKICK(chan);
			Servidor::sendallbutone(server, message);
		} else
			oper.GlobOPs("ERROR: Fallo de usuarios o canales en SKICK.");
	} else if (cmd == "PING") {
		server->send("PONG");
		Servidores::uPing(server->name());
	} else if (cmd == "PONG") {
		Servidores::uPing(server->name());
	} else if (cmd == "MEMO") {
		if (x.size() < 5) {
			oper.GlobOPs("ERROR: MEMO invalido.");
			return;
		}
		std::string mensaje = "";
		for (unsigned int i = 4; i < x.size(); ++i) { mensaje += " " + x[i]; }
		Memo *memo = new Memo();
			memo->sender = x[1];
			memo->receptor = x[2];
			memo->time = (time_t ) stoi(x[3]);
			memo->mensaje = mensaje;
		MemoMsg.insert(memo);
	} else if (cmd == "MEMODEL") {
		if (x.size() < 2) {
			oper.GlobOPs("ERROR: MEMODEL invalido.");
			return;
		}
		Memos::iterator it = MemoMsg.begin();
		while(it != MemoMsg.end())
			if (boost::iequals((*it)->receptor, x[1]))
				it = MemoMsg.erase(it);
		Servidor::sendallbutone(server, message);
	} else if (cmd == "WEBIRC") {
		if (x.size() < 3) {
			oper.GlobOPs("ERROR: WEBIRC invalido.");
			return;
		}
		User*  target = Mainframe::instance()->getUserByName(x[1]);
		target->WEBIRC(x[2]);
		Servidor::sendallbutone(server, message);
	}
}
