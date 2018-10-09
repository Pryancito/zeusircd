#include "db.h"
#include "server.h"
#include "oper.h"
#include "mainframe.h"
#include "utils.h"
#include "services.h"
#include "base64.h"

using namespace std;

void ChanServ::Message(User *user, string message) {
	StrVec  x;
	boost::split(x, message, boost::is_any_of(" \t"), boost::token_compress_on);
	std::string cmd = x[0];
	boost::to_upper(cmd);
	
	if (cmd == "HELP") {
		user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :[ /chanserv register|drop|vop|hop|aop|sop|topic|key|akick|op|deop|halfop|dehalfop|voice|devoice|transfer ]" + config->EOFMessage);
		return;
	} else if (cmd == "REGISTER") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == true) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s is already registered.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identificate first.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (ChanServ::CanRegister(user, x[1]) == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You need to be into the channel and got @ to make %s.", "REGISTER") + config->EOFMessage);
			return;
		} else {
			string sql = "INSERT INTO CANALES VALUES ('" + x[1] + "', '" + user->nick() + "', '+r', '', '" + Utils::make_string("", "The channel has been registered.") + "',  " + std::to_string(time(0)) + ", " + std::to_string(time(0)) + ");";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s cannot be registered. Please contact with an iRCop.", x[1].c_str()) + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "INSERT INTO CMODES VALUES ('" + x[1] + "', 0, 0, 0, 0, 0, 0);";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + Utils::make_string("", "The channel %s has been registered.", x[1].c_str()) + config->EOFMessage);
			Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
			if (chan->getMode('r') == false) {
				chan->setMode('r', true);
				chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +r" + config->EOFMessage);
				Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +r");
			}
			return;
		}
	} else if (cmd == "DROP") {
		if (x.size() < 2) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identificate first.") + config->EOFMessage);
			return;
		} else if (ChanServ::CanRegister(user, x[1]) == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "You need to be into the channel and got @ to make %s.", "DROP") + config->EOFMessage);
			return;
		} else if (ChanServ::IsFounder(user->nick(), x[1]) == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + Utils::make_string(user->nick(), "You are not the founder of the channel.") + config->EOFMessage);
			return;
		} else {
			string sql = "DELETE FROM CANALES WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + Utils::make_string(user->nick(), "The channel %s cannot be deleted. Please contact with an iRCop.", x[1].c_str()) + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "DELETE FROM ACCESS WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "DELETE FROM AKICK WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			sql = "DELETE FROM CMODES WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
			DB::SQLiteNoReturn(sql);
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s has been deleted.", x[1].c_str()) + config->EOFMessage);
			Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
			if (chan->getMode('r') == true) {
				chan->setMode('r', false);
				chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -r" + config->EOFMessage);
				Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -r");
			}
		}
	} else if (cmd == "VOP" || cmd == "HOP" || cmd == "AOP" || cmd == "SOP") {
		if (x.size() < 3) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == 0) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s is not registered.", x[1].c_str()) + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == 0) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identificate first.") + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 4) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :No tienes suficiente acceso." + config->EOFMessage);
			return;
		} else if (NickServ::GetOption("NOACCESS", user->nick()) == 1) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El nick tiene activada la opcion NOACCESS." + config->EOFMessage);
			return;
		} else {
			if (boost::iequals(x[2], "ADD")) {
				if (x.size() < 4) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				} else if (NickServ::IsRegistered(x[3]) == 0) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El nick no esta registrado." + config->EOFMessage);
					return;
				}
				if (ChanServ::Access(x[3], x[1]) != 0) {
					string acceso;
					switch (ChanServ::Access(x[3], x[1])) {
						case 1: acceso = "VOP"; break;
						case 2: acceso = "HOP"; break;
						case 3: acceso = "AOP"; break;
						case 4: acceso = "SOP"; break;
						case 5: acceso = "FUNDADOR"; break;
						default: acceso = "NINGUNO"; break;
					}
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El nick ya tiene acceso de " + acceso + config->EOFMessage);
					return;
				} else {
					string sql = "INSERT INTO ACCESS VALUES ('" + x[1] + "', '" + cmd + "', '" + x[3] + "', '" + user->nick() + "');";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El registro no se ha podido insertar." + config->EOFMessage);
						return;
					}
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :Se ha insertado el registro." + config->EOFMessage);
					User *target = Mainframe::instance()->getUserByName(x[3]);
					if (target)
						ChanServ::CheckModes(target, x[1]);
				}
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :Se ha dado " + cmd + " a " + x[3] + config->EOFMessage);
			} else if (boost::iequals(x[2], "DEL")) {
				if (x.size() < 4) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				}
				if (ChanServ::Access(x[3], x[1]) == 0) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El usuario no tiene acceso." + config->EOFMessage);
					return;
				}
				string sql = "DELETE FROM ACCESS WHERE USUARIO='" + x[3] + "' COLLATE NOCASE AND CANAL='" + x[1] + "' COLLATE NOCASE AND ACCESO='" + cmd + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El registro no se ha podido borrar." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				User *target = Mainframe::instance()->getUserByName(x[3]);
				if (target)
					ChanServ::CheckModes(target, x[1]);
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :Se ha quitado " + cmd + " a " + x[3] + config->EOFMessage);
			} else if (boost::iequals(x[2], "LIST")) {
				vector <string> usuarios;
				vector <string> who;
				string sql = "SELECT USUARIO FROM ACCESS WHERE CANAL='" + x[1] + "' COLLATE NOCASE AND ACCESO='" + cmd + "' COLLATE NOCASE;";
				usuarios = DB::SQLiteReturnVector(sql);
				sql = "SELECT ADDED FROM ACCESS WHERE CANAL='" + x[1] + "' COLLATE NOCASE AND ACCESO='" + cmd + "' COLLATE NOCASE;";
				who = DB::SQLiteReturnVector(sql);
				if (usuarios.size() == 0)
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :No hay accesos de " + cmd + config->EOFMessage);
				for (unsigned int i = 0; i < usuarios.size(); i++) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :\002" + usuarios[i] + "\002 accesado por " + who[i] + config->EOFMessage);
				}
			}
			return;
		}
	} else if (cmd == "TOPIC") {
		if (x.size() < 3) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identificate first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El canal no esta registrado." + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 3) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :No tienes acceso para cambiar el topic." + config->EOFMessage);
			return;
		} else {
			int pos = 7 + x[1].length();
			string topic = message.substr(pos);
			if (topic.length() > 250) {
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El topic es demasiado largo." + config->EOFMessage);
				return;
			}
			string sql = "UPDATE CANALES SET TOPIC='" + Base64::Encode(topic) + "' WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El topic no se ha podido cambiar." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
			if (chan) {
				chan->cmdTopic(topic);
				chan->broadcast(":" + config->Getvalue("chanserv") + " 332 " + user->nick() + " " + chan->name() + " :" + topic + config->EOFMessage);
			}
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El topic se ha cambiado." + config->EOFMessage);
			return;
		}
	} else if (cmd == "AKICK") {
		if (x.size() < 3) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identificate first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == 0) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El canal no esta registrado." + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 4) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :No tienes acceso para cambiar los AKICK." + config->EOFMessage);
			return;
		} else {
			if (boost::iequals(x[2], "ADD")) {
				if (x.size() < 5) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				}
				if (ChanServ::IsAKICK(x[3], x[1]) != 0) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :La mascara ya tiene AKICK." + config->EOFMessage);
					return;
				} else {
					int posicion = 4 + cmd.length() + x[1].length() + x[2].length() + x[3].length();
					string motivo = message.substr(posicion);
					if (DB::EscapeChar(motivo) == true || DB::EscapeChar(x[3]) == true) {
						user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El motivo o la mascara contienen caracteres no validos." + config->EOFMessage);
						return;
					}
					string sql = "INSERT INTO AKICK VALUES ('" + x[1] + "', '" + x[3] + "', '" + motivo + "', '" + user->nick() + "');";
					if (DB::SQLiteNoReturn(sql) == false) {
						user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El AKICK no se ha podido insertar." + config->EOFMessage);
						return;
					}
					sql = "DB " + DB::GenerateID() + " " + sql;
					DB::AlmacenaDB(sql);
					Servidor::sendall(sql);
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :Se ha insertado el AKICK." + config->EOFMessage);
				}
			} else if (boost::iequals(x[2], "DEL")) {
				if (x.size() < 4) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
					return;
				}
				if (ChanServ::IsAKICK(x[3], x[1]) == 0) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El usuario no tiene AKICK." + config->EOFMessage);
					return;
				}
				string sql = "DELETE FROM AKICK WHERE MASCARA='" + x[3] + "' COLLATE NOCASE AND CANAL='" + x[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El AKICK no se ha podido borrar." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :Se ha quitado AKICK a " + x[3] + config->EOFMessage);
			} else if (boost::iequals(x[2], "LIST")) {
				vector <string> akick;
				vector <string> who;
				vector <string> reason;
				string sql = "SELECT MASCARA FROM AKICK WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
				akick = DB::SQLiteReturnVector(sql);
				sql = "SELECT ADDED FROM AKICK WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
				who = DB::SQLiteReturnVector(sql);
				sql = "SELECT MOTIVO FROM AKICK WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
				reason = DB::SQLiteReturnVector(sql);
				if (akick.size() == 0)
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :No hay AKICKS en " + x[1] + config->EOFMessage);
				for (unsigned int i = 0; i < akick.size(); i++) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :\002" + akick[i] + "\002 realizado por " + who[i] + ". Motivo: " + reason[i] + config->EOFMessage);
				}
			}
			return;
		}
	} else if (cmd == "OP" || cmd == "DEOP" || cmd == "HALFOP" || cmd == "DEHALFOP" || cmd == "VOICE" || cmd == "DEVOICE") {
		if (x.size() < 3) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identificate first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El canal no esta registrado." + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 3) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :No tienes acceso para cambiar los modos." + config->EOFMessage);
			return;
		} else if (!Mainframe::instance()->getUserByName(x[2])) {
				user->session()->send(":" + config->Getvalue("serverName") + " 401 " + user->nick() + " :El Nick no existe." + config->EOFMessage);
				return;
		} else {
			char modo;
			bool action;
			if (cmd == "OP") {
				modo = 'o';
				action = 1;
			} else if (cmd == "DEOP") {
				modo = 'o';
				action = 0;
			} else if (cmd == "HALFOP") {
				modo = 'h';
				action = 1;
			} else if (cmd == "DEHALFOP") {
				modo = 'h';
				action = 0;
			} else if (cmd == "VOICE") {
				modo = 'v';
				action = 1;
			} else if (cmd == "DEVOICE") {
				modo = 'v';
				action = 0;
			} else
				return;

			Channel* chan = Mainframe::instance()->getChannelByName(x[1]);
			User *target = Mainframe::instance()->getUserByName(x[2]);
			
			if (!chan || !user)
				return;
			if (chan->isVoice(target)) {
				if (modo == 'h' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
					chan->delVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +h " + target->nick() + config->EOFMessage);
					chan->giveHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +h " + target->nick());
				} else if (modo == 'o' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
					chan->delVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + target->nick() + config->EOFMessage);
					chan->giveOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + target->nick());
				} else if (modo == 'v' && action == 1) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El nick ya tiene VOZ." + config->EOFMessage);
				} else if (modo == 'v' && action == 0) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
					chan->delVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + target->nick());
				}
			} else if (chan->isHalfOperator(target)) {
				if (modo == 'v' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
					chan->delHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -h " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +v " + target->nick() + config->EOFMessage);
					chan->giveVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +v " + target->nick());
				} else if (modo == 'o' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
					chan->delHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -h " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + target->nick() + config->EOFMessage);
					chan->giveOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + target->nick());
				} else if (modo == 'h' && action == 1) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El nick ya tiene HALFOP." + config->EOFMessage);
				} else if (modo == 'h' && action == 0) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
					chan->delHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -h " + target->nick());
				}
			} else if (chan->isOperator(target)) {
				if (modo == 'v' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
					chan->delOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -o " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +v " + target->nick() + config->EOFMessage);
					chan->giveVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +v " + target->nick());
				} else if (modo == 'h' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
					chan->delOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -o " + target->nick());
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +h " + target->nick() + config->EOFMessage);
					chan->giveHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +h " + target->nick());
				} else if (modo == 'o' && action == 1) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El nick ya tiene OP." + config->EOFMessage);
				} else if (modo == 'o' && action == 0) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
					chan->delOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -o " + target->nick());
				}
			} else if (chan->hasUser(target)) {
				if (modo == 'v' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +v " + target->nick() + config->EOFMessage);
					chan->giveVoice(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +v " + target->nick());
				} else if (modo == 'h' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +h " + target->nick() + config->EOFMessage);
					chan->giveHalfOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +h " + target->nick());
				} else if (modo == 'o' && action == 1) {
					chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + target->nick() + config->EOFMessage);
					chan->giveOperator(target);
					Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + target->nick());
				} else if (action == 0){
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El nick no tiene modos." + config->EOFMessage);
				}
			}
			return;
		}
	} else if (cmd == "KEY") {
		if (x.size() < 3) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El HUB no existe, las BDs estan en modo de solo lectura." + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identificate first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El canal no esta registrado." + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 5) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :No tienes acceso para cambiar la clave." + config->EOFMessage);
			return;
		} else {
			string key = x[2];
			if (DB::EscapeChar(key) == true) {
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :La clave contiene caracteres no validos." + config->EOFMessage);
				return;
			}
			if (key.length() > 32) {
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :La clave es demasiado larga." + config->EOFMessage);
				return;
			}
			string sql = "UPDATE CANALES SET KEY='" + key + "' WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :La clave no se ha podido cambiar." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :La clave se ha cambiado a: " + key + config->EOFMessage);
			return;
		}
	} else if (cmd == "MODE") {
		if (x.size() < 3) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identificate first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El canal no esta registrado." + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 4) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :No tienes acceso para cambiar los modos." + config->EOFMessage);
			return;
		} else {
			string mode = x[2].substr(1);
			boost::to_upper(mode);
			if (boost::iequals("LIST", x[2])) {
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :Los modos disponibles son: flood, onlyreg, autovoice, moderated, onlysecure, nonickchange" + config->EOFMessage);
				return;
			} else if (!boost::iequals("FLOOD", mode) && !boost::iequals("ONLYREG", mode) && !boost::iequals("AUTOVOICE", mode) &&
						!boost::iequals("MODERATED", mode) && !boost::iequals("ONLYSECURE", mode) && !boost::iequals("NONICKCHANGE", mode)) {
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :Modo desconocido." + config->EOFMessage);
				return;
			} if (x[2][0] == '+') {
				std::string sql;
				if (ChanServ::HasMode(x[1], mode) == true) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El canal " + x[1] + " ya tiene el modo " + mode + config->EOFMessage);
					return;
				} if (mode == "FLOOD" && x.size() != 4) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El modo flood tiene argumentos." + config->EOFMessage);
					return;
				} else if (mode == "FLOOD" && (!Utils::isnumber(x[3]) || stoi(x[3]) < 0 || stoi(x[3]) > 999)) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El argumento del modo flood es incorrecto." + config->EOFMessage);
					return;
				} else if (mode == "FLOOD")
					sql = "UPDATE CMODES SET " + mode + "=" + x[3] + " WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
				else
					sql = "UPDATE CMODES SET " + mode + "=1 WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El modo no se ha podido poner." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El modo se ha fijado." + config->EOFMessage);
				return;
			} else if (x[2][0] == '-') {
				if (ChanServ::HasMode(x[1], mode) == false) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El canal " + x[1] + " no tiene el modo " + mode + config->EOFMessage);
					return;
				}
				string sql = "UPDATE CMODES SET " + mode + "=0 WHERE CANAL='" + x[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteNoReturn(sql) == false) {
					user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El modo no se ha podido quitar." + config->EOFMessage);
					return;
				}
				sql = "DB " + DB::GenerateID() + " " + sql;
				DB::AlmacenaDB(sql);
				Servidor::sendall(sql);
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El modo se ha quitado." + config->EOFMessage);
				return;
			}
		}
	} else if (cmd == "TRANSFER") {
		if (x.size() < 3) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (NickServ::IsRegistered(x[2]) == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El nick de destino no esta registrado." + config->EOFMessage);
			return;
		} else if (Server::HUBExiste() == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The HUB doesnt exists, DBs are in read-only mode.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identificate first.") + config->EOFMessage);
			return;
		} else if (ChanServ::IsRegistered(x[1]) == false) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El canal no esta registrado." + config->EOFMessage);
			return;
		} else if (ChanServ::Access(user->nick(), x[1]) < 5) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :No tienes acceso para cambiar la clave." + config->EOFMessage);
			return;
		} else {
			string sql = "UPDATE CANALES SET OWNER='" + x[2] + "' WHERE NOMBRE='" + x[1] + "' COLLATE NOCASE;";
			if (DB::SQLiteNoReturn(sql) == false) {
				user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :No se ha podido cambiar el fundador." + config->EOFMessage);
				return;
			}
			sql = "DB " + DB::GenerateID() + " " + sql;
			DB::AlmacenaDB(sql);
			Servidor::sendall(sql);
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El fundador se ha cambiado a: " + x[2] + config->EOFMessage);
			return;
		}
	}
}

void ChanServ::DoRegister(User *user, Channel *chan) {
	string sql = "SELECT TOPIC FROM CANALES WHERE NOMBRE='" + chan->name() + "' COLLATE NOCASE;";
	string topic = DB::SQLiteReturnString(sql);
	topic = Base64::Decode(topic);
	sql = "SELECT REGISTERED FROM CANALES WHERE NOMBRE='" + chan->name() + "' COLLATE NOCASE;";
	int creado = DB::SQLiteReturnInt(sql);
	if (topic != "") {
		chan->cmdTopic(topic);
		user->session()->send(":" + config->Getvalue("serverName") + " 332 " + user->nick() + " " + chan->name() + " :" + topic + config->EOFMessage);
		user->session()->send(":" + config->Getvalue("serverName") + " 333 " + user->nick() + " " + chan->name() + " " + config->Getvalue("chanserv") + " " + std::to_string(creado) + config->EOFMessage);
	}
	user->session()->send(":" + config->Getvalue("serverName") + " 324 " + user->nick() + " " + chan->name() + " +r" + config->EOFMessage);
	user->session()->send(":" + config->Getvalue("serverName") + " 329 " + user->nick() + " " + chan->name() + " " + std::to_string(creado) + config->EOFMessage);
	if (chan->getMode('r') == false) {
		chan->setMode('r', true);
		user->session()->send(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +r" + config->EOFMessage);
		Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +r");
	}
}

int ChanServ::HasMode(string canal, string mode) {
	boost::to_upper(mode);
	string sql = "SELECT " + mode + " FROM CMODES WHERE CANAL='" + canal + "' COLLATE NOCASE;";
	return (DB::SQLiteReturnInt(sql));
}

void ChanServ::CheckModes(User *user, string channel) {
	if (NickServ::GetOption("NOOP", user->nick()) == true)
		return;
	Channel* chan = Mainframe::instance()->getChannelByName(channel);
	if (!chan)
		return;
	int access = ChanServ::Access(user->nick(), chan->name());
	if (HasMode(channel, "AUTOVOICE") && access < 1) access = 1;
	
	if (chan->isVoice(user) == true) {
		if (access < 1) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + user->nick() + config->EOFMessage);
			chan->delVoice(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + user->nick());
		} else if (access == 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + user->nick() + config->EOFMessage);
			chan->delVoice(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + user->nick());
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +h " + user->nick() + config->EOFMessage);
			chan->giveHalfOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +h " + user->nick());
		} else if (access > 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -v " + user->nick() + config->EOFMessage);
			chan->delVoice(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -v " + user->nick());
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + user->nick() + config->EOFMessage);
			chan->giveOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + user->nick());
		}
	} else if (chan->isHalfOperator(user) == true) {
		if (access < 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -h " + user->nick() + config->EOFMessage);
			chan->delHalfOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -h " + user->nick());
		} else if (access > 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -h " + user->nick() + config->EOFMessage);
			chan->delHalfOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -h " + user->nick());
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + user->nick() + config->EOFMessage);
			chan->giveOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + user->nick());
		}
	} else if (chan->isOperator(user) == true) {
		if (access < 3) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " -o " + user->nick() + config->EOFMessage);
			chan->delOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " -o " + user->nick());
		}
	} else {
		if (access == 1) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +v " + user->nick() + config->EOFMessage);
			chan->giveVoice(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +v " + user->nick());
		} else if (access == 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +h " + user->nick() + config->EOFMessage);
			chan->giveHalfOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +h " + user->nick());
		} else if (access > 2) {
			chan->broadcast(":" + config->Getvalue("chanserv") + " MODE " + chan->name() + " +o " + user->nick() + config->EOFMessage);
			chan->giveOperator(user);
			Servidor::sendall("CMODE " + config->Getvalue("chanserv") + " " + chan->name() + " +o " + user->nick());
		}
	}
	return;
}

bool ChanServ::IsRegistered(string channel) {
	string sql = "SELECT NOMBRE from CANALES WHERE NOMBRE='" + channel + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	return (boost::iequals(retorno, channel));
}

bool ChanServ::IsFounder(string nickname, string channel) {
	string sql = "SELECT OWNER from CANALES WHERE NOMBRE='" + channel + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	return (boost::iequals(retorno, nickname));
}

int ChanServ::Access (string nickname, string channel) {
	string sql = "SELECT ACCESO from ACCESS WHERE USUARIO='" + nickname + "' COLLATE NOCASE AND CANAL='" + channel + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	User* user = Mainframe::instance()->getUserByName(nickname);
	if (boost::iequals(retorno, "VOP"))
		return 1;
	else if (boost::iequals(retorno, "HOP"))
		return 2;
	else if (boost::iequals(retorno, "AOP"))
		return 3;
	else if (boost::iequals(retorno, "SOP"))
		return 4;
	else if (ChanServ::IsFounder(nickname, channel) == 1)
		return 5;
	else if (user)
		if (user->getMode('o') == true)
			return 5;
	return 0;
}

bool ChanServ::IsAKICK(string mascara, string canal) {
	vector <string> akicks;
	string sql = "SELECT MASCARA from AKICK WHERE CANAL='" + canal + "' COLLATE NOCASE;";
	akicks = DB::SQLiteReturnVector(sql);
	for (unsigned int i = 0; i < akicks.size(); i++) {
		boost::algorithm::to_lower(akicks[i]);
		boost::algorithm::to_lower(mascara);
		if (Utils::Match(akicks[i].c_str(), mascara.c_str()) == 1)
			return true;
	}
	return false;
}

bool ChanServ::CheckKEY(string canal, string key) {
	string sql = "SELECT KEY from CANALES WHERE NOMBRE='" + canal + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	if (retorno.length() == 0 || key.length() == 0)
		return true;
	if (retorno == key)
		return true;
	else
		return false;
}

bool ChanServ::IsKEY(string canal) {
	string sql = "SELECT KEY from CANALES WHERE NOMBRE='" + canal + "' COLLATE NOCASE;";
	string retorno = DB::SQLiteReturnString(sql);
	return (retorno.length() > 0);
}

int ChanServ::GetChans () {
	string sql = "SELECT COUNT(*) FROM CANALES;";
	return DB::SQLiteReturnInt(sql);
}

bool ChanServ::CanRegister(User *user, string channel) {
	Channel* chan = Mainframe::instance()->getChannelByName(channel);
	if (chan)
		return (chan->hasUser(user) && chan->isOperator(user));
	return false;
}
