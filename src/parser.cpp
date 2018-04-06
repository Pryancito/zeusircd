#include "parser.h"
#include "server.h"
#include "mainframe.h"
#include "oper.h"
#include "services.h"
#include "utils.h"
#include "db.h"
#include "ircv3.h"

extern time_t encendido;
extern ServerSet Servers;
extern Memos MemoMsg;

bool Parser::checknick (const std::string nick) {
	if (nick.length() == 0)
		return false;
	for (unsigned int i = 0; i < nick.length(); i++)
		if (!std::isalnum(nick[i]))
			return false;
	return true;
}

bool Parser::checkchan (const std::string chan) {
	if (chan.length() == 0)
		return false;
	if (chan[0] != '#')
		return false;
	for (unsigned int i = 1; i < chan.length(); i++)
		if (!std::isalnum(chan[i]))
			return false;
	return true;
}

void Parser::parse(const std::string& message, User* user) {
	StrVec  split;
	boost::split(split, message, boost::is_any_of(" \t"), boost::token_compress_on);
	boost::to_upper(split[0]);

	if (split[0] == "NICK") {
		if (split.size() < 2) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NONICKNAMEGIVEN) + " "
				+ user->nick() + " No has proporcionado un nick" + config->EOFMessage);
			return;
		}
		
		if (split[1] == user->nick())
			return;

		if (user->canchangenick() == false) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_ERRONEUSNICKNAME)
				+ " " + user->nick() + " No puedes cambiar el nick." + config->EOFMessage);
			return;
		}

		if (boost::iequals(split[1], user->nick()) == true) {
			user->cmdNick(split[1]);
			return;
		}

		std::string nickname = split[1];
		std::string password = "";
		
		if (nickname.find("!") != std::string::npos || nickname.find(":") != std::string::npos) {
			StrVec nickpass;
			boost::split(nickpass,nickname,boost::is_any_of(":!"), boost::token_compress_on);
			nickname = nickpass[0];
			password = nickpass[1];
		}
		
		if (checknick(nickname) == false) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_ERRONEUSNICKNAME)
				+ " " + nickname + " :Nick erroneo." + config->EOFMessage);
			return;
		}
		if (NickServ::IsRegistered(nickname) == true && password == "") {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + nickname + " :No has proporcionado una password. [ /nick tunick:tupass ]" + config->EOFMessage);
			return;
		}
		
		if (NickServ::Login(nickname, password) == false && password != "") {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + nickname + " :La password es incorrecta." + config->EOFMessage);
			return;
		}
		
		User* target = Mainframe::instance()->getUserByName(nickname);
		
		if (NickServ::Login(nickname, password) == true) {
			if (target)
				target->cmdQuit();

			user->cmdNick(nickname);
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + nickname + " :Bienvenido a casa." + config->EOFMessage);
			if (user->getMode('r') == false) {
				user->setMode('r', true);
				user->session()->sendAsServer("MODE " + nickname + " +r" + config->EOFMessage);
				Servidor::sendall("UMODE " + user->nick() + " +r");
			}
			NickServ::UpdateLogin(user);
			
			if (NickServ::GetvHost(nickname) != "")
				user->Cycle();
        
			return;
		}
		if (target) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NICKCOLLISION) + " " 
				+ user->nick() + " " 
				+ nickname + " :El nick ya esta en uso." 
				+ config->EOFMessage);
			return;
		}
		
		if (NickServ::IsRegistered(user->nick()) == true && NickServ::IsRegistered(nickname) == false) {
			user->cmdNick(nickname);
			if (user->getMode('r') == true) {
				user->setMode('r', false);
				user->session()->sendAsServer("MODE " + user->nick() + " -r" + config->EOFMessage);
				Servidor::sendall("UMODE " + user->nick() + " -r");
			}
			user->Cycle();
			return;
		}

		if (NickServ::IsRegistered(user->nick()) == true && NickServ::IsRegistered(nickname) == true) {
			user->cmdNick(nickname);
			if (NickServ::GetvHost(nickname) != "")
				user->Cycle();
			return;
		}
		
		user->cmdNick(nickname);
	}

	else if (split[0] == "USER") {
		std::string ident;
		if (split.size() < 5) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NEEDMOREPARAMS) + " "
				+ user->nick() + " :Necesito mas datos." + config->EOFMessage);
			return;
		} if (split[1].length() > 10)
			ident = split[1].substr(0, 9);
		else
			ident = split[1];
		user->cmdUser(ident);
	}

	else if (split[0] == "QUIT") {
		user->cmdQuit();
	}

	else if (split[0] == "JOIN") {
		if (split.size() < 2) return;
		if (checkchan(split[1]) == false) {
			user->session()->sendAsServer("461 " + user->nick() + " :El Canal contiene caracteres incorrectos." + config->EOFMessage);
			return;
		} else if (split[1].size() < 2) {
			user->session()->sendAsServer("461 " + user->nick() + " :Falta el nombre del canal." + config->EOFMessage);
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
		if (split[1].length() > (unsigned int )stoi(config->Getvalue("chanlen"))) {
			user->session()->sendAsServer("461 " + user->nick() + " :El Canal es demasiado largo." + config->EOFMessage);
			return;
		} else if (user->Channels() >= stoi(config->Getvalue("maxchannels"))) {
			user->session()->sendAsServer("461 " + user->nick() + " :Has entrado en demasiados canales." + config->EOFMessage);
			return;
		} else if (ChanServ::HasMode(split[1], "ONLYREG") == true && NickServ::IsRegistered(user->nick()) == false) {
			user->session()->sendAsServer("461 " + user->nick() + " :Solo se permite la entrada a nicks registrados." + config->EOFMessage);
			return;
		} else if (ChanServ::HasMode(split[1], "ONLYSECURE") == true && user->getMode('z') == false) {
			user->session()->sendAsServer("461 " + user->nick() + " :Solo se permite la entrada a usuarios conectados a traves de SSL." + config->EOFMessage);
			return;
		} else {
			if (ChanServ::IsAKICK(user->nick() + "!" + user->ident() + "@" + user->sha(), split[1]) == true && user->getMode('o') == false) {
				user->session()->sendAsServer("461 " + user->nick() + " :Tienes AKICK en este canal, no puedes entrar." + config->EOFMessage);
				return;
			}
			if (NickServ::IsRegistered(user->nick()) == true && !NickServ::GetvHost(user->nick()).empty()) {
				if (ChanServ::IsAKICK(user->nick() + "!" + user->ident() + "@" + NickServ::GetvHost(user->nick()), split[1]) == true && user->getMode('o') == false) {
					user->session()->sendAsServer("461 " + user->nick() + " :Tienes AKICK en este canal, no puedes entrar." + config->EOFMessage);
					return;
				}
			}
			if (ChanServ::IsKEY(split[1]) == true) {
				if (split.size() != 3) {
					user->session()->sendAsServer("461 " + user->nick() + " :Necesito mas datos. [ /join #canal password ]" + config->EOFMessage);
					return;
				} else if (ChanServ::CheckKEY(split[1], split[2]) == false) {
					user->session()->sendAsServer("461 " + user->nick() + " :Password incorrecto." + config->EOFMessage);
					return;
				}
			}
		} if (chan) {
			if (chan->hasUser(user) == true) {
				user->session()->sendAsServer("461 " + user->nick() + " :Ya estas dentro del canal." + config->EOFMessage);
				return;
			} else if (chan->IsBan(user->nick() + "!" + user->ident() + "@" + user->sha()) == true) {
				user->session()->sendAsServer("461 " + user->nick() + " :Estas baneado, no puedes entrar al canal." + config->EOFMessage);
				return;
			} else if (chan->IsBan(user->nick() + "!" + user->ident() + "@" + user->cloak()) == true) {
				user->session()->sendAsServer("461 " + user->nick() + " :Estas baneado, no puedes entrar al canal." + config->EOFMessage);
				return;
			} else if (chan->isonflood() == true) {
				user->session()->sendAsServer("461 " + user->nick() + " :El canal esta en flood, no puedes entrar al canal." + config->EOFMessage);
				return;
			}
			user->cmdJoin(chan);
			Servidor::sendall("SJOIN " + user->nick() + " " + chan->name() + " +x");
			if (ChanServ::IsRegistered(chan->name()) == true) {
				ChanServ::DoRegister(user, chan);
				ChanServ::CheckModes(user, chan->name());
				chan->increaseflood();
			}
		} else {
			chan = new Channel(user, split[1]);
			if (chan) {
				user->cmdJoin(chan);
				Mainframe::instance()->addChannel(chan);
				Servidor::sendall("SJOIN " + user->nick() + " " + split[1] + " +x");
				if (ChanServ::IsRegistered(chan->name()) == true) {
					ChanServ::DoRegister(user, chan);
					ChanServ::CheckModes(user, chan->name());
					chan->increaseflood();
				}
			}
		}
	}

	else if (split[0] == "PART") {
		if (split.size() < 2) return;
		Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
		if (chan) {
			user->cmdPart(chan);
			Servidor::sendall("SPART " + user->nick() + " " + chan->name());
			chan->increaseflood();
			if (chan->userCount() == 0)
				Mainframe::instance()->removeChannel(chan->name());
		}
	}

	else if (split[0] == "TOPIC") {
		if (split.size() == 2) {
			Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
			if (chan) {
				if (chan->topic().empty()) {
					user->session()->sendAsServer(ToString(Response::Reply::RPL_NOTOPIC)
						+ " " + split[1]
						+ " :No topic is set !"
						+ config->EOFMessage);
				}
				else {
					user->session()->sendAsServer(ToString(Response::Reply::RPL_TOPIC)
						+ " " + user->nick()
						+ " " + split[1]
						+ " :" + chan->topic()
						+ config->EOFMessage);
				}
			}
		}
	}

	else if (split[0] == "LIST") {
		std::string comodin = "*";
		if (split.size() == 2)
			comodin = split[1];
		user->session()->sendAsServer(ToString(Response::Reply::RPL_LISTSTART)
			+ " " + user->nick()
			+ " Channel :Users Name"
			+ config->EOFMessage);
		ChannelMap channels = Mainframe::instance()->channels();
		ChannelMap::iterator it = channels.begin();
		for (; it != channels.end(); ++it) {
			std::string mtch = it->first;
			boost::to_lower(comodin);
			boost::to_lower(mtch);
			if (Utils::Match(comodin.c_str(), mtch.c_str()) == 1)
				user->session()->sendAsServer(ToString(Response::Reply::RPL_LIST) + " "
					+ user->nick() + " "
					+ it->second->name() + " "
					+ ToString(it->second->userCount()) + " :"
					+ it->second->topic()
					+ config->EOFMessage);
		}
		user->session()->sendAsServer(ToString(Response::Reply::RPL_LISTEND) + " "
			+ user->nick()
			+ " :End of /LIST"
			+ config->EOFMessage);

	}

	else if (split[0] == "PRIVMSG" || split[0] == "NOTICE") {
		if (split.size() < 3) return;
		std::string message = "";
		for (unsigned int i = 2; i < split.size(); ++i) { message += split[i] + " "; }

		if (split[1][0] == '#') {
			Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
			if (chan) {
				if (ChanServ::HasMode(chan->name(), "MODERATED") && !chan->isOperator(user) && !chan->isHalfOperator(user) && !chan->isVoice(user) && !user->getMode('o')) {
					user->session()->sendAsServer("461 " + user->nick() + " :El canal esta moderado, no puedes hablar." + config->EOFMessage);
					return;
				} else if (chan->isonflood() == true) {
					user->session()->sendAsServer("461 " + user->nick() + " :El canal esta en flood, no puedes hablar." + config->EOFMessage);
					return;
				} else if (chan->IsBan(user->nick() + "!" + user->ident() + "@" + user->sha()) == true) {
					user->session()->sendAsServer("461 " + user->nick() + " :Estas baneado, no puedes hablar en el canal." + config->EOFMessage);
					return;
				} else if (chan->IsBan(user->nick() + "!" + user->ident() + "@" + user->cloak()) == true) {
					user->session()->sendAsServer("461 " + user->nick() + " :Estas baneado, no puedes hablar en el canal." + config->EOFMessage);
					return;
				}
				chan->increaseflood();
				chan->broadcast_except_me(user,
					user->messageHeader()
					+ split[0] + " "
					+ chan->name() + " "
					+ message + config->EOFMessage);
				Servidor::sendall(split[0] + " " + user->nick() + "!" + user->ident() + "@" + user->cloak() + " " + chan->name() + " " + message);
			}
		}
		else {
			User* target = Mainframe::instance()->getUserByName(split[1]);
			if (target && target->server() == config->Getvalue("serverName")) {
				target->session()->send(user->messageHeader()
					+ split[0] + " "
					+ target->nick() + " "
					+ message + config->EOFMessage);
			} else if (target) {
				Servidor::sendall(split[0] + " " + user->nick() + "!" + user->ident() + "@" + user->cloak() + " " + target->nick() + " " + message);
			} else if (!target && NickServ::IsRegistered(split[1]) == true && NickServ::MemoNumber(split[1]) < 50) {
				std::string mensaje = "";
				for (unsigned int i = 2; i < split.size(); ++i) { mensaje += " " + split[i]; }
				Memo *memo = new Memo();
					memo->sender = user->nick();
					memo->receptor = split[1];
					memo->time = time(0);
					memo->mensaje = mensaje;
				MemoMsg.insert(memo);
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :El nick no esta conectado, se le ha dejado un MeMo." + config->EOFMessage);
				Servidor::sendall("MEMO " + memo->sender + " " + memo->receptor + " " + std::to_string(memo->time) + " " + memo->mensaje);
			} else
				user->session()->sendAsServer("461 " + user->nick() + " :El nick no existe y no esta registrado." + config->EOFMessage);
		}
	}

	else if (split[0] == "KICK") {
		if (split.size() < 3) return;

		Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
		User*  victim = Mainframe::instance()->getUserByName(split[2]);
		std::string reason = "";
		if (chan && victim) {
			for (unsigned int i = 3; i < split.size(); ++i) {
				reason += split[i] + " ";
			}
			if ((chan->isOperator(user) || chan->isHalfOperator(user)) && chan->hasUser(victim) && !chan->isOperator(victim)) {
				user->cmdKick(victim, reason, chan);
				victim->cmdPart(chan);
				Servidor::sendall("SKICK " + user->nick() + " " + chan->name() + " " + victim->nick() + " " + reason);
			}
		}

	}

	else if (split[0] == "WHO") {
		if (split.size() < 2) return;
		Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
		if (!chan) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NOSUCHCHANNEL)
				+ " " + user->nick() + " :El canal no existe." + config->EOFMessage);
		} else if (!chan->hasUser(user)) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_USERNOTINCHANNEL)
				+ " " + user->nick() + " :No estas en el canal." + config->EOFMessage);
		} else {
			chan->sendWhoList(user);
		}
	}

	else if (split[0] == "OPER") {
		Oper oper;
		if (split.size() < 3) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NEEDMOREPARAMS)
				+ " " + user->nick() + " :Necesito mas parametros [ /oper nick pass ]" + config->EOFMessage);
		} else if (oper.IsOper(user) == true) {
			user->session()->sendAsServer(ToString(Response::Reply::RPL_YOUREOPER)
				+ " " + user->nick() + " :Ya eres un iRCop." + config->EOFMessage);
		} else if (oper.Login(user, split[1], split[2]) == true) {
			user->session()->sendAsServer(ToString(Response::Reply::RPL_YOUREOPER)
				+ " " + user->nick() + " :Ahora eres un iRCop." + config->EOFMessage);
		} else {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NOPRIVILEGES)
				+ " " + user->nick() + " :Login Fallido, tu intento ha sido notificado." + config->EOFMessage);
			oper.GlobOPs("Intento fallido de /oper del nick: " + user->nick());
		}
	}

	else if (split[0] == "PING") { if (split.size() > 1) user->cmdPing(split[1]); else user->cmdPing(""); }
	
	else if (split[0] == "PONG") { user->UpdatePing(); }

	else if (split[0] == "CAP") {
		if (split.size() < 2) return;
		else if (split[1] == "LS" || split[1] == "LIST")
			user->iRCv3()->sendCAP(split[1]);
		else if (split[1] == "END")
			user->iRCv3()->recvEND();
	}

	else if (split[0] == "WEBIRC") {
		if (split.size() < 5) {
			user->session()->sendAsServer("461 " + user->nick() + " :Necesito mas datos." + config->EOFMessage);
		} else if (split[1] == config->Getvalue("cgiirc")) {
			user->cmdWebIRC(split[4]);
		}
	}

	else if (split[0] == "STATS") {
		user->session()->sendAsServer("002 " + user->nick() + " :Hay \002" + std::to_string(Mainframe::instance()->countusers()) + "\002 usuarios y \002" + std::to_string(Mainframe::instance()->countchannels()) + "\002 canales." + config->EOFMessage);
		user->session()->sendAsServer("002 " + user->nick() + " :Hay \002" + std::to_string(NickServ::GetNicks()) + "\002 nicks registrados y \002" + std::to_string(ChanServ::GetChans()) + "\002 canales registrados." + config->EOFMessage);
		user->session()->sendAsServer("002 " + user->nick() + " :Hay \002" + std::to_string(Oper::Count()) + "\002 iRCops conectados." + config->EOFMessage);
		user->session()->sendAsServer("002 " + user->nick() + " :Hay \002" + std::to_string(Servidor::count()) + "\002 servidores conectados." + config->EOFMessage);
	}
	
	else if (split[0] == "UPTIME") {
		user->session()->sendAsServer("002 " + user->nick() + " :Este servidor lleva encendido: " + Utils::Time(encendido) + config->EOFMessage);
	}

	else if (split[0] == "REHASH") {
		if (user->getMode('o') == false) {
			user->session()->sendAsServer("002 " + user->nick() + " :No tienes privilegios de iRCop." + config->EOFMessage);
		} else {
			config->conf.clear();
			config->Cargar();
			user->session()->sendAsServer("002 " + user->nick() + " :La configuracion ha sido recargada." + config->EOFMessage);
		}
	}
	
	else if (split[0] == "MODE") {
		if (split.size() < 2) {
			user->session()->sendAsServer("461 " + user->nick() + " :Necesito mas datos. [ /mode #canal (+modo) (nick) ]" + config->EOFMessage);
			return;
		} else if (split[1][0] == '#') {
			Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
			if (ChanServ::IsRegistered(split[1]) == false) {
				user->session()->sendAsServer("461 " + user->nick() + " :El canal no esta registrado." + config->EOFMessage);
				return;
			} else if (!chan) {
				user->session()->sendAsServer("461 " + user->nick() + " :No hay nadie en el canal." + config->EOFMessage);
				return;
			} else if (chan->isOperator(user) == false && chan->isHalfOperator(user) == false && split.size() != 2) {
				user->session()->sendAsServer("461 " + user->nick() + " :No tienes @ ni %." + config->EOFMessage);
				return;
			} else if (split.size() == 2) {
				std::string sql = "SELECT MODOS from CANALES WHERE NOMBRE='" + split[1] + "' COLLATE NOCASE;";
				std::string modos = DB::SQLiteReturnString(sql);
				user->session()->sendAsServer("324 " + user->nick() + " " + split[1] + " " + modos + config->EOFMessage);
				return;
			} else if (split.size() == 3) {
				if (split[2] == "+b" || split[2] == "b") {
					BanSet bans = chan->bans();
					BanSet::iterator it = bans.begin();
					for (; it != bans.end(); ++it)
						user->session()->sendAsServer("367 " + user->nick() + " " + split[1] + " " + (*it)->mask() + " " + (*it)->whois() + " " + std::to_string((*it)->time()) + config->EOFMessage);
					user->session()->sendAsServer("368 " + user->nick() + " " + split[1] + " :Fin de la lista de baneados." + config->EOFMessage);
				}
			} else if (split.size() > 3) {
				bool action = 0;
				unsigned int j = 0;
				std::string ban;
				std::string msg = message.substr(5);
				for (unsigned int i = 0; i < split[2].length(); i++) {
					if (split[2][i] == '+') {
						action = 1;
					} else if (split[2][i] == '-') {
						action = 0;
					} else if (split[2][i] == 'b') {
						StrVec baneos;
						std::string maskara = split[3+j];
						boost::to_lower(maskara);
						if (action == 1) {
							if (chan->IsBan(maskara) == true) {
								user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El BAN ya existe." + config->EOFMessage);
							} else {
								chan->setBan(maskara, user->nick());
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +b " + maskara + config->EOFMessage);
								user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El BAN ha sido fijado." + config->EOFMessage);
								Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " +b " + maskara);
							}
						} else {
							if (chan->IsBan(maskara) == false) {
								user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El BAN no existe." + config->EOFMessage);
							} else {
								BanSet bans = chan->bans();
								BanSet::iterator it = bans.begin();
								for (; it != bans.end(); ++it)
									if ((*it)->mask() == maskara) {
										chan->UnBan((*it));
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -b " + maskara + config->EOFMessage);
										user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :El BAN ha sido eliminado." + config->EOFMessage);
										Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -b " + maskara);
									}
							}
						}
						j++;
					}
				}
			}
		}
	}
	
	else if (split[0] == "WHOIS") {
		if (split.size() < 2) {
			user->session()->sendAsServer("431 " + user->nick() + " :Necesito mas datos. [ /whois nick|#canal ]" + config->EOFMessage);
			return;
		} else if (split[1][0] == '#') {
			if (checkchan (split[1]) == false) {
				user->session()->sendAsServer("401 " + user->nick() + " " + split[1] + " :El canal contiene caracteres no validos." + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + split[1] + " :Fin de /WHOIS." + config->EOFMessage);
				return;
			}
			Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
			std::string sql;
			std::string mascara = user->nick() + "!" + user->ident() + "@" + user->sha();
			if (ChanServ::IsAKICK(mascara, split[1]) == true) {
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :STATUS: \0036AKICK\003." + config->EOFMessage);
			} else if (chan && chan->IsBan(mascara) == true)
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :STATUS: \0036BANEADO\003." + config->EOFMessage);
			if (NickServ::IsRegistered(user->nick()) == true && !NickServ::GetvHost(user->nick()).empty()) {
				mascara = user->nick() + "!" + user->ident() + "@" + NickServ::GetvHost(user->nick());
				if (ChanServ::IsAKICK(mascara, split[1]) == true) {
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :STATUS: \0036AKICK\003." + config->EOFMessage);
				} else if (chan && chan->IsBan(mascara) == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :STATUS: \0036BANEADO\003." + config->EOFMessage);
			} if (!chan)
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :STATUS: \0034VACIO\003." + config->EOFMessage);
			else
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :STATUS: \0033ACTIVO\003." + config->EOFMessage);
			if (ChanServ::IsRegistered(split[1]) == 1) {
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :El canal esta registrado." + config->EOFMessage);
				switch (ChanServ::Access(user->nick(), split[1])) {
					case 0:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :No tienes acceso." + config->EOFMessage);
						break;
					case 1:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Tu acceso es de VOP." + config->EOFMessage);
						break;
					case 2:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Tu acceso es de HOP." + config->EOFMessage);
						break;
					case 3:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Tu acceso es de AOP." + config->EOFMessage);
						break;
					case 4:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Tu acceso es de SOP." + config->EOFMessage);
						break;
					case 5:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Tu acceso es de FUNDADOR." + config->EOFMessage);
						break;
					default:
						break;
				}
				std::string modes;
				if (ChanServ::HasMode(split[1], "FLOOD") == true) {
					modes.append(" flood");
				} if (ChanServ::HasMode(split[1], "ONLYREG") == true) {
					modes.append(" onlyreg");
				} if (ChanServ::HasMode(split[1], "AUTOVOICE") == true) {
					modes.append(" autovoice");
				} if (ChanServ::HasMode(split[1], "MODERATED") == true) {
					modes.append(" moderated");
				} if (ChanServ::HasMode(split[1], "ONLYSECURE") == true) {
					modes.append(" onlysecure");
				} if (ChanServ::HasMode(split[1], "NONICKCHANGE") == true) {
					modes.append(" nonickchange");
				}
				if (modes.empty() == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :El canal no tiene modos." + config->EOFMessage);
				else
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :El canal tiene los modos:" + modes + config->EOFMessage);
				if (ChanServ::IsKEY(split[1]) == 1 && ChanServ::Access(user->nick(), split[1]) != 0) {
					sql = "SELECT KEY FROM CANALES WHERE NOMBRE='" + split[1] + "' COLLATE NOCASE;";
					std::string key = DB::SQLiteReturnString(sql);
					if (key.length() > 0)
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :La clave del canal es: " + key + config->EOFMessage);
				}
				std::string sql = "SELECT OWNER FROM CANALES WHERE NOMBRE='" + split[1] + "' COLLATE NOCASE;";
				std::string owner = DB::SQLiteReturnString(sql);
				if (owner.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :El fundador del canal es: " + owner + config->EOFMessage);
				sql = "SELECT REGISTERED FROM CANALES WHERE NOMBRE='" + split[1] + "' COLLATE NOCASE;";
				int registro = DB::SQLiteReturnInt(sql);
				if (registro > 0) {
					std::string tiempo = Utils::Time(registro);
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Registrado desde: " + tiempo + config->EOFMessage);
				}
			} else
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :El canal no esta registrado." + config->EOFMessage);
			user->session()->sendAsServer("318 " + user->nick() + " " + split[1] + " :Fin de /WHOIS." + "\r\n");
		} else {
			if (checknick (split[1]) == false) {
				user->session()->sendAsServer("401 :El nick contiene caracteres no validos." + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + split[1] + " :Fin de /WHOIS." + config->EOFMessage);
				return;
			}
			User* target = Mainframe::instance()->getUserByName(split[1]);
			std::string sql;
			if (!target && NickServ::IsRegistered(split[1]) == true) {
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :STATUS: \0034DESCONECTADO\003." + config->EOFMessage);
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Tiene el nick registrado." + config->EOFMessage);
				sql = "SELECT SHOWMAIL FROM OPTIONS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteReturnInt(sql) == 1 || user->getMode('o') == true) {
					sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
					std::string email = DB::SQLiteReturnString(sql);
					if (email.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Su correo electronico es: " + email + config->EOFMessage);
				}
				sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
				std::string url = DB::SQLiteReturnString(sql);
				if (url.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Su Web es: " + url + config->EOFMessage);
				sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
				std::string vHost = DB::SQLiteReturnString(sql);
				if (vHost.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Su vHost es: " + vHost + config->EOFMessage);
				sql = "SELECT REGISTERED FROM NICKS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
				int registro = DB::SQLiteReturnInt(sql);
				if (registro > 0) {
					std::string tiempo = Utils::Time(registro);
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Registrado desde: " + tiempo + config->EOFMessage);
				}
				sql = "SELECT LASTUSED FROM NICKS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
				int last = DB::SQLiteReturnInt(sql);
				std::string tiempo = Utils::Time(last);
				if (tiempo.length() > 0 && last > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :Visto por ultima vez: " + tiempo + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + split[1] + " :Fin de /WHOIS." + config->EOFMessage);
				return;
			} else if (target && NickServ::IsRegistered(split[1]) == 1) {
				user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + target->nick() + " es: " + target->nick() + "!" + target->ident() + "@" + target->sha() + config->EOFMessage);
				user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :STATUS: \0033CONECTADO\003." + config->EOFMessage);
				user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Tiene el nick registrado." + config->EOFMessage);
				if (user->getMode('o') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Su IP es: " + target->host() + config->EOFMessage);
				if (target->getMode('o') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Es un iRCop." + config->EOFMessage);
				if (target->getMode('z') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Conecta mediante un canal seguro SSL." + config->EOFMessage);
				if (target->getMode('w') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Conecta mediante WebChat." + config->EOFMessage);
				if (NickServ::GetOption("SHOWMAIL", target->nick()) == true) {
					sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + target->nick() + "' COLLATE NOCASE;";
					std::string email = DB::SQLiteReturnString(sql);
					if (email.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Su correo electronico es: " + email + config->EOFMessage);
				}
				sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + target->nick() + "' COLLATE NOCASE;";
				std::string url = DB::SQLiteReturnString(sql);
				if (url.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Su Web es: " + url + config->EOFMessage);
				sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + target->nick() + "' COLLATE NOCASE;";
				std::string vHost = DB::SQLiteReturnString(sql);
				if (vHost.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Su vHost es: " + vHost + config->EOFMessage);
				if (user == target && NickServ::IsRegistered(user->nick()) == 1) {
					std::string opciones;
					if (NickServ::GetOption("NOACCESS", user->nick()) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOACCESS");
					}
					if (NickServ::GetOption("SHOWMAIL", user->nick()) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("SHOWMAIL");
					}
					if (NickServ::GetOption("NOMEMO", user->nick()) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOMEMO");
					}
					if (NickServ::GetOption("NOOP", user->nick()) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("NOOP");
					}
					if (NickServ::GetOption("ONLYREG", user->nick()) == 1) {
						if (!opciones.empty())
							opciones.append(", ");
						opciones.append("ONLYREG");
					}
					if (opciones.length() == 0)
						opciones = "Ninguna";
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Tus opciones son: " + opciones + config->EOFMessage);
				}
				sql = "SELECT REGISTERED FROM NICKS WHERE NICKNAME='" + target->nick() + "' COLLATE NOCASE;";
				int registro = DB::SQLiteReturnInt(sql);
				if (registro > 0) {
					std::string tiempo = Utils::Time(registro);
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Registrado desde: " + tiempo + config->EOFMessage);
				}
				std::string tiempo = Utils::Time(target->GetLogin());
				if (tiempo.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Entro hace: " + tiempo + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + target->nick() + " :Fin de /WHOIS." + config->EOFMessage);
				return;
			} else if (target && NickServ::IsRegistered(split[1]) == 0) {
				user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + target->nick() + " es: " + target->nick() + "!" + target->ident() + "@" + target->sha() + config->EOFMessage);
				user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :STATUS: \0033CONECTADO\003.\r\n");
				if (user->getMode('o') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Su IP es: " + target->host() + config->EOFMessage);
				if (target->getMode('o') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Es un iRCop." + config->EOFMessage);
				if (target->getMode('z') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Conecta mediante un canal seguro SSL." + config->EOFMessage);
				if (target->getMode('w') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Conecta mediante WebChat." + config->EOFMessage);
				std::string tiempo = Utils::Time(target->GetLogin());
				if (tiempo.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :Entro hace: " + tiempo + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + target->nick() + " :Fin de /WHOIS." + config->EOFMessage);
				return;
			} else {
				user->session()->sendAsServer("401 " + user->nick() + " " + split[1] + " :El nick no existe." + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + split[1] + " :Fin de /WHOIS." + config->EOFMessage);
				return;
			}
		}
	}

	else if (split[0] == "CONNECT") {
		if (split.size() < 3) {
			user->session()->sendAsServer("461 " + user->nick() + " :Necesito mas datos." + config->EOFMessage);
			return;
		} else if (user->getMode('o') == false) {
			user->session()->sendAsServer("002 " + user->nick() + " :No tienes privilegios de iRCop." + config->EOFMessage);
			return;
		} else if (Servidor::IsAServer(split[1]) == false) {
			user->session()->sendAsServer("002 " + user->nick() + " :El servidor no esta en mi lista." + config->EOFMessage);
			return;
		} else if (Servidor::IsConected(split[1]) == true) {
			user->session()->sendAsServer("002 " + user->nick() + " :El servidor ya esta conectado." + config->EOFMessage);
			return;
		} else {
			user->session()->sendAsServer("002 " + user->nick() + " :Conectando..." + config->EOFMessage);
			Servidor::Connect(split[1], split[2]);
			return;
		}
	}

	else if (split[0] == "SERVERS") {
		if (user->getMode('o') == false) {
			user->session()->sendAsServer("002" + user->nick() + " :No tienes privilegios de iRCop." + config->EOFMessage);
			return;
		} else {
			ServerSet::iterator it = Servers.begin();
			for (; it != Servers.end(); ++it) {
				user->session()->sendAsServer("002 " + user->nick() + " :Nombre: " + (*it)->name() + " IP: " + (*it)->ip() + config->EOFMessage);
				for (unsigned int i = 0; i < (*it)->connected.size(); i++)
					user->session()->sendAsServer("002 " + user->nick() + " :Conectado a: " + (*it)->connected[i] + config->EOFMessage);
			}
		}
	}

	else if (split[0] == "SQUIT") {
		if (split.size() < 2) {
			user->session()->sendAsServer("461 " + user->nick() + " :Necesito mas datos." + config->EOFMessage);
			return;
		} else if (user->getMode('o') == false) {
			user->session()->sendAsServer("002 " + user->nick() + " :No tienes privilegios de iRCop." + config->EOFMessage);
			return;
		} else if (Servidor::Exists(split[1]) == false) {
			user->session()->sendAsServer("002 " + user->nick() + " :El servidor no esta conectado." + config->EOFMessage);
			return;
		} else if (boost::iequals(split[1], config->Getvalue("serverName"))) {
			user->session()->sendAsServer("002 " + user->nick() + " :No puedes hacer un SQUIT a tu propio servidor." + config->EOFMessage);
			return;
		} else {
			Servidor::sendall("SQUIT " + split[1]);
			Servidor::SQUIT(split[1]);
			user->session()->sendAsServer("002 " + user->nick() + " :El servidor ha sido desconectado." + config->EOFMessage);
			return;
		}
	}
	
	else if (split[0] == "NICKSERV" || split[0] == "NS") {
		if (split.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /nickserv help ]" + config->EOFMessage);
			return;
		} else if (split[0] == "NICKSERV"){
			NickServ::Message(user, message.substr(9));
			return;
		} else if (split[0] == "NS"){
			NickServ::Message(user, message.substr(3));
			return;
		}
	} 

	else if (split[0] == "CHANSERV" || split[0] == "CS") {
		if (split.size() < 2) {
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /chanserv help ]" + config->EOFMessage);
			return;
		} else if (split[0] == "CHANSERV"){
			ChanServ::Message(user, message.substr(9));
			return;
		} else if (split[0] == "CS"){
			ChanServ::Message(user, message.substr(3));
			return;
		}
	} 

	else if (split[0] == "HOSTSERV" || split[0] == "HS") {
		if (split.size() < 2) {
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /hostserv help ]" + config->EOFMessage);
			return;
		} else if (split[0] == "HOSTSERV"){
			HostServ::Message(user, message.substr(9));
			return;
		} else if (split[0] == "HS"){
			HostServ::Message(user, message.substr(3));
			return;
		}
	}

	else if (split[0] == "OPERSERV" || split[0] == "OS") {
		if (user->getMode('o') == false) {
			user->session()->send(":" + config->Getvalue("serverName") + " 461 :No eres un operador." + config->EOFMessage);
			return;
		} else if (split.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :Necesito mas datos. [ /operserv help ]" + config->EOFMessage);
			return;
		} else if (split[0] == "OPERSERV"){
			OperServ::Message(user, message.substr(9));
			return;
		} else if (split[0] == "OS"){
			OperServ::Message(user, message.substr(3));
			return;
		}
	}

	else {
		user->session()->sendAsServer(ToString(Response::Error::ERR_UNKNOWNCOMMAND)
			+ " " + user->nick() + " :Comando desconocido." + config->EOFMessage);
	}
}
