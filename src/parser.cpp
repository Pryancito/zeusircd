#include "parser.h"
#include "server.h"
#include "mainframe.h"
#include "oper.h"
#include "services.h"
#include "utils.h"
#include "db.h"
#include "ircv3.h"
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>

#define GC_THREADS
#define GC_ALWAYS_MULTITHREADED
#include <gc_cpp.h>
#include <gc.h>

extern time_t encendido;
extern ServerSet Servers;
extern Memos MemoMsg;
boost::mutex log_mtx;

bool Parser::checknick (const std::string &nick) {
	if (nick.length() == 0)
		return false;
	for (unsigned int i = 0; i < nick.length(); i++)
		if (!std::isalnum(nick[i]) && nick[i] != '-' && nick[i] != '_')
			return false;
	return true;
}

bool Parser::checkchan (const std::string &chan) {
	if (chan.length() == 0)
		return false;
	if (chan[0] != '#')
		return false;
	for (unsigned int i = 1; i < chan.length(); i++)
		if (!std::isalnum(chan[i]) && chan[i] != '-' && chan[i] != '_')
			return false;
	return true;
}

void Parser::log(const std::string &message) {
	if (config->Getvalue("serverName") == config->Getvalue("hub")) {
		time_t now = time(0);
		struct tm *tm = localtime(&now);
		char date[32];
		strftime(date, sizeof(date), "%r %d-%m-%Y", tm);
		std::fstream fs;
		log_mtx.lock();
		fs.open ("ircd.log", std::fstream::in | std::fstream::out | std::fstream::app);
		fs << date << " -> " << message << std::endl;
		fs.close();
		log_mtx.unlock();
	}
}

void Parser::parse(std::string& message, User* user) {
	StrVec  split;
	boost::trim_right(message);
	boost::split(split, message, boost::is_any_of(" \t"), boost::token_compress_on);
	boost::to_upper(split[0]);

	if (split[0] == "NICK") {
		if (split.size() < 2) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NONICKNAMEGIVEN) + " "
				+ user->nick() + " :" + Utils::make_string(user->nick(), "No nickname: [ /nick yournick ]") + config->EOFMessage);
			return;
		}
		
		if (split[1][0] == ':')
			split[1] = split[1].substr(1, split[1].length());
		
		if (split[1] == user->nick())
			return;

		if (split[1].length() > (unsigned int ) stoi(config->Getvalue("nicklen"))) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_ERRONEUSNICKNAME)
					+ " " + user->nick() + " :" + Utils::make_string(user->nick(), "Nick too long.") + config->EOFMessage);
				return;
		}

		if (OperServ::IsSpam(split[1], "N") == true && OperServ::IsSpam(split[1], "E") == false) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_ERRONEUSNICKNAME)
					+ " " + user->nick() + " :" + Utils::make_string(user->nick(), "Your nick is marked as SPAM.") + config->EOFMessage);
				return;
		}

		if (user->nick() != "")
			if (user->canchangenick() == false) {
				user->session()->sendAsServer(ToString(Response::Error::ERR_ERRONEUSNICKNAME)
					+ " " + user->nick() + " :" + Utils::make_string(user->nick(), "You cannot change your nick.") + config->EOFMessage);
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
		} else if (user->ispassword())
			password = user->getPass();
		
		if (nickname == user->nick())
			return;
			
		if (checknick(nickname) == false) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_ERRONEUSNICKNAME)
				+ " " + nickname + " :" + Utils::make_string(user->nick(), "The nick contains no-valid characters.") + config->EOFMessage);
			return;
		}
		
		if (NickServ::IsRegistered(nickname) == true && password == "") {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + nickname + " :" + Utils::make_string(user->nick(), "You need a password: [ /nick yournick:yourpass ]") + config->EOFMessage);
			return;
		}
		
		if (NickServ::Login(nickname, password) == false && password != "" && NickServ::IsRegistered(nickname) == true) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + nickname + " :" + Utils::make_string(user->nick(), "Wrong password.") + config->EOFMessage);
			return;
		}
		
		User* target = Mainframe::instance()->getUserByName(nickname);
		
		if (NickServ::IsRegistered(nickname) == true && NickServ::Login(nickname, password) == true) {
			if (target)
				target->cmdQuit();

			user->cmdNick(nickname);
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "Welcome home.") + config->EOFMessage);
			if (user->getMode('r') == false) {
				user->setMode('r', true);
				user->session()->sendAsServer("MODE " + nickname + " +r" + config->EOFMessage);
				Servidor::sendall("UMODE " + user->nick() + " +r");
			}
			NickServ::UpdateLogin(user);
			
			return;
		}
		if (target) {
			user->session()->sendAsServer("433 " 
				+ user->nick() + " " 
				+ nickname + " :" + Utils::make_string(user->nick(), "The nick is used by somebody.")
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
			return;
		}
		user->cmdNick(nickname);
	}

	else if (split[0] == "USER") {
		std::string ident;
		if (split.size() < 5) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NEEDMOREPARAMS) + " "
				+ user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} if (split[1].length() > 10)
			ident = split[1].substr(0, 9);
		else
			ident = split[1];
		user->cmdUser(ident);
	}
	
	else if (split[0] == "PASS") {
		if (split.size() < 2) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NEEDMOREPARAMS) + " "
				+ user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		}
		user->setPass(split[1]);
	}

	else if (split[0] == "QUIT") {
		user->cmdQuit();
	}

	else if (split[0] == "JOIN") {
		if (split.size() < 2) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NEEDMOREPARAMS) + " "
				+ user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
			return;
		} else if (checkchan(split[1]) == false) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel contains no-valid characters.") + config->EOFMessage);
			return;
		} else if (split[1].size() < 2) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel name is empty.") + config->EOFMessage);
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
		if (split[1].length() > (unsigned int )stoi(config->Getvalue("chanlen"))) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel name is too long.") + config->EOFMessage);
			return;
		} else if (user->Channels() >= stoi(config->Getvalue("maxchannels"))) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You enter in too much channels.") + config->EOFMessage);
			return;
		} else if (ChanServ::HasMode(split[1], "ONLYREG") == true && NickServ::IsRegistered(user->nick()) == false) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The entrance is only allowed to registered nicks.") + config->EOFMessage);
			return;
		} else if (ChanServ::HasMode(split[1], "ONLYSECURE") == true && user->getMode('z') == false) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The entrance is only allowed to SSL users.") + config->EOFMessage);
			return;
		} else {
			if (ChanServ::IsAKICK(user->nick() + "!" + user->ident() + "@" + user->sha(), split[1]) == true && user->getMode('o') == false) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You got AKICK on this channel, you cannot pass.") + config->EOFMessage);
				return;
			}
			if (NickServ::IsRegistered(user->nick()) == true && !NickServ::GetvHost(user->nick()).empty()) {
				if (ChanServ::IsAKICK(user->nick() + "!" + user->ident() + "@" + NickServ::GetvHost(user->nick()), split[1]) == true && user->getMode('o') == false) {
					user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You got AKICK on this channel, you cannot pass.") + config->EOFMessage);
					return;
				}
			}
			if (ChanServ::IsKEY(split[1]) == true) {
				if (split.size() != 3) {
					user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "I need more data: [ /join #channel password ]") + config->EOFMessage);
					return;
				} else if (ChanServ::CheckKEY(split[1], split[2]) == false) {
					user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "Wrong password.") + config->EOFMessage);
					return;
				}
			}
		} if (chan) {
			if (chan->hasUser(user) == true) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You are already on this channel.") + config->EOFMessage);
				return;
			} else if (chan->IsBan(user->nick() + "!" + user->ident() + "@" + user->sha()) == true) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You are banned, cannot pass.") + config->EOFMessage);
				return;
			} else if (chan->IsBan(user->nick() + "!" + user->ident() + "@" + user->cloak()) == true) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You are banned, cannot pass.") + config->EOFMessage);
				return;
			} else if (chan->isonflood() == true && user->getMode('o') == false) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel is on flood, you cannot pass.") + config->EOFMessage);
				return;
			} else {
				user->cmdJoin(chan);
				Servidor::sendall("SJOIN " + user->nick() + " " + chan->name() + " +x");
				if (ChanServ::IsRegistered(chan->name()) == true) {
					ChanServ::DoRegister(user, chan);
					ChanServ::CheckModes(user, chan->name());
					chan->increaseflood();
				}
			}
		} else {
			chan = new (GC) Channel(user, split[1]);
			if (chan) {
				Mainframe::instance()->addChannel(chan);
				user->cmdJoin(chan);
				Servidor::sendall("SJOIN " + user->nick() + " " + chan->name() + " +x");
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
		else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
		if (chan) {
			if (chan->hasUser(user) == false) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You are not into the channel.") + config->EOFMessage);
				return;
			}
			user->cmdPart(chan);
			Servidor::sendall("SPART " + user->nick() + " " + chan->name());
			chan->increaseflood();
			if (chan->userCount() == 0)
				Mainframe::instance()->removeChannel(chan->name());
		}
	}

	else if (split[0] == "TOPIC") {
		if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
			return;
		}
		else if (split.size() == 2) {
			Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
			if (chan) {
				if (chan->topic().empty()) {
					user->session()->sendAsServer(ToString(Response::Reply::RPL_NOTOPIC)
						+ " " + split[1]
						+ " :" + Utils::make_string(user->nick(), "No topic is set !")
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
		if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
			return;
		}
		std::string comodin = "*";
		if (split.size() == 2)
			comodin = split[1];
		user->session()->sendAsServer(ToString(Response::Reply::RPL_LISTSTART)
			+ " " + user->nick()
			+ " " + Utils::make_string(user->nick(), "Channel :Users Name")
			+ config->EOFMessage);
		ChannelMap channels = Mainframe::instance()->channels();
		ChannelMap::iterator it = channels.begin();
		for (; it != channels.end(); ++it) {
			std::string mtch = it->second->name();
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
			+ " :" + Utils::make_string(user->nick(), "End of /LIST")
			+ config->EOFMessage);

	}

	else if (split[0] == "PRIVMSG" || split[0] == "NOTICE") {
		if (split.size() < 3) return;
		else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
			return;
		}
		std::string message = "";
		for (unsigned int i = 2; i < split.size(); ++i) { message += split[i] + " "; }

		if (split[1][0] == '#') {
			Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
			if (chan) {
				if (ChanServ::HasMode(chan->name(), "MODERATED") && !chan->isOperator(user) && !chan->isHalfOperator(user) && !chan->isVoice(user) && !user->getMode('o')) {
					user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel is moderated, you cannot speak.") + config->EOFMessage);
					return;
				} else if (chan->isonflood() == true && ChanServ::Access(user->nick(), chan->name()) == 0) {
					user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel is on flood, you cannot speak.") + config->EOFMessage);
					return;
				} else if (chan->IsBan(user->nick() + "!" + user->ident() + "@" + user->sha()) == true) {
					user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You are banned, cannot speak.") + config->EOFMessage);
					return;
				} else if (chan->IsBan(user->nick() + "!" + user->ident() + "@" + user->cloak()) == true) {
					user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You are banned, cannot speak.") + config->EOFMessage);
					return;
				} else if (OperServ::IsSpam(message, "C") == true && OperServ::IsSpam(message, "E") == false && user->getMode('o') == false) {
					Oper oper;
					oper.GlobOPs(Utils::make_string("", "Nickname %s try to make SPAM into channel: %s", user->nick().c_str(), chan->name().c_str()));
					user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The message of channel %s contains SPAM.", chan->name().c_str()) + config->EOFMessage);
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
			if (OperServ::IsSpam(message, "P") == true && OperServ::IsSpam(message, "E") == false && user->getMode('o') == false && target) {
				Oper oper;
				oper.GlobOPs(Utils::make_string("", "Nickname %s try to make SPAM to nick: %s", user->nick().c_str(), target->nick().c_str()));
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "Message to nick %s contains SPAM.", target->nick().c_str()) + config->EOFMessage);
				return;
			} else if (target && NickServ::GetOption("ONLYREG", split[1]) == true && user->getMode('r') == false) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick %s only can receive messages from registered nicks.", target->nick().c_str()) + config->EOFMessage);
				return;
			} else if (target && target->server() == config->Getvalue("serverName")) {
				target->session()->send(user->messageHeader()
					+ split[0] + " "
					+ target->nick() + " "
					+ message + config->EOFMessage);
			} else if (target) {
				Servidor::sendall(split[0] + " " + user->nick() + "!" + user->ident() + "@" + user->cloak() + " " + target->nick() + " " + message);
			} else if (!target && NickServ::IsRegistered(split[1]) == true && NickServ::MemoNumber(split[1]) < 50 && NickServ::GetOption("NOMEMO", split[1]) == 0) {
				std::string mensaje = "";
				for (unsigned int i = 2; i < split.size(); ++i) { mensaje += " " + split[i]; }
				Memo *memo = new (GC) Memo();
					memo->sender = user->nick();
					memo->receptor = split[1];
					memo->time = time(0);
					memo->mensaje = mensaje;
				MemoMsg.insert(memo);
				user->session()->send(":NiCK!*@* NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick is offline, MeMo has been sent.") + config->EOFMessage);
				Servidor::sendall("MEMO " + memo->sender + " " + memo->receptor + " " + std::to_string(memo->time) + " " + memo->mensaje);
			} else
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The nick doesnt exists or cannot receive messages.") + config->EOFMessage);
		}
	}

	else if (split[0] == "KICK") {
		if (split.size() < 3) return;
		else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
		User*  victim = Mainframe::instance()->getUserByName(split[2]);
		std::string reason = "";
		if (chan && victim) {
			for (unsigned int i = 3; i < split.size(); ++i) {
				reason += split[i] + " ";
			}
			if ((chan->isOperator(user) || chan->isHalfOperator(user)) && chan->hasUser(victim) && (!chan->isOperator(victim) || user->getMode('o') == true) && victim->getMode('o') == false) {
				user->cmdKick(victim, reason, chan);
				victim->cmdPart(chan);
				Servidor::sendall("SKICK " + user->nick() + " " + chan->name() + " " + victim->nick() + " " + reason);
				if (chan->userCount() == 0)
					Mainframe::instance()->removeChannel(chan->name());
			}
		}

	}

	else if (split[0] == "WHO") {
		if (split.size() < 2) return;
		else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
			return;
		}
		Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
		if (!chan) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NOSUCHCHANNEL)
				+ " " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel doesnt exists.") + config->EOFMessage);
		} else if (!chan->hasUser(user)) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_USERNOTINCHANNEL)
				+ " " + user->nick() + " :" + Utils::make_string(user->nick(), "You are not into the channel.") + config->EOFMessage);
		} else {
			chan->sendWhoList(user);
		}
	}

	else if (split[0] == "OPER") {
		Oper oper;
		if (split.size() < 3) {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NEEDMOREPARAMS)
				+ " " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
		} else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
			return;
		} else if (oper.IsOper(user) == true) {
			user->session()->sendAsServer(ToString(Response::Reply::RPL_YOUREOPER)
				+ " " + user->nick() + " :" + Utils::make_string(user->nick(), "You are already an iRCop.") + config->EOFMessage);
		} else if (oper.Login(user, split[1], split[2]) == true) {
			user->session()->sendAsServer(ToString(Response::Reply::RPL_YOUREOPER)
				+ " " + user->nick() + " :" + Utils::make_string(user->nick(), "Now you are an iRCop.") + config->EOFMessage);
		} else {
			user->session()->sendAsServer(ToString(Response::Error::ERR_NOPRIVILEGES)
				+ " " + user->nick() + " :" + Utils::make_string(user->nick(), "Login failed, your attempt has been notified.") + config->EOFMessage);
		}
	}

	else if (split[0] == "PING") { if (split.size() > 1) user->cmdPing(split[1]); else user->cmdPing(""); }
	
	else if (split[0] == "PONG") { user->UpdatePing(); }

	else if (split[0] == "USERHOST") { return; }

	else if (split[0] == "CAP") {
		if (split.size() < 2) return;
		else if (split[1] == "LS" || split[1] == "LIST")
			user->iRCv3()->sendCAP(split[1]);
		else if (split[1] == "REQ")
			user->iRCv3()->Request(message);
		else if (split[1] == "END")
			user->iRCv3()->recvEND();
	}

	else if (split[0] == "WEBIRC") {
		if (split.size() < 5) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
		} else if (split[1] == config->Getvalue("cgiirc")) {
			user->cmdWebIRC(split[4]);
		}
	}

	else if (split[0] == "STATS" || split[0] == "LUSERS") {
		user->session()->sendAsServer("002 " + user->nick() + " :" + Utils::make_string(user->nick(), "There are \002%s\002 users and \002%s\002 channels.", std::to_string(Mainframe::instance()->countusers()).c_str(), std::to_string(Mainframe::instance()->countchannels()).c_str()) + config->EOFMessage);
		user->session()->sendAsServer("002 " + user->nick() + " :" + Utils::make_string(user->nick(), "There are \002%s\002 registered nicks and \002%s\002 registered channels.", std::to_string(NickServ::GetNicks()).c_str(), std::to_string(ChanServ::GetChans()).c_str()) + config->EOFMessage);
		user->session()->sendAsServer("002 " + user->nick() + " :" + Utils::make_string(user->nick(), "There are \002%s\002 connected iRCops.", std::to_string(Oper::Count()).c_str()) + config->EOFMessage);
		user->session()->sendAsServer("002 " + user->nick() + " :" + Utils::make_string(user->nick(), "There are \002%s\002 connected servers.", std::to_string(Servidor::count()).c_str()) + config->EOFMessage);
	}
	
	else if (split[0] == "UPTIME") {
		user->session()->sendAsServer("002 " + user->nick() + " :" + Utils::make_string(user->nick(), "This server started as long as: %s", Utils::Time(encendido).c_str()) + config->EOFMessage);
	}

	else if (split[0] == "VERSION") {
		user->session()->sendAsServer("002 " + user->nick() + " :" + Utils::make_string(user->nick(), "Version of ZeusiRCd: %s", config->version.c_str()) + config->EOFMessage);
		if (user->getMode('o') == true)
			user->session()->sendAsServer("002 " + user->nick() + " :" + Utils::make_string(user->nick(), "Version of DataBase: %s", DB::GetLastRecord().c_str()) + config->EOFMessage);
	}

	else if (split[0] == "REHASH") {
		if (user->getMode('o') == false) {
			user->session()->sendAsServer("002 " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have iRCop privileges.") + config->EOFMessage);
			return;
		} else {
			config->conf.clear();
			config->Cargar();
			user->session()->sendAsServer("002 " + user->nick() + " :" + Utils::make_string(user->nick(), "The config has been reloaded.") + config->EOFMessage);
		}
	}
	
	else if (split[0] == "SHUTDOWN") {
		if (user->getMode('o') == false && user->getMode('r') == false) {
			user->session()->sendAsServer("002 " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have iRCop privileges.") + config->EOFMessage);
			return;
		} else if (user->getMode('r') == false) {
			user->session()->sendAsServer("002 " + user->nick() + " :" + Utils::make_string(user->nick(), "To make this action, you need identificate first.") + config->EOFMessage);
			return;
		} else {
			exit(0);
		}
	}
	
	else if (split[0] == "MODE") {
		if (split.size() < 2) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
			return;
		} else if (split[1][0] == '#') {
			if (Parser::checkchan(split[1]) == false) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel contains no-valid characters.") + config->EOFMessage);
				return;
			}
			Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
			if (ChanServ::IsRegistered(split[1]) == false && user->getMode('o') == false) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel %s is not registered.", split[1].c_str()) + config->EOFMessage);
				return;
			} else if (!chan) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The channel is empty.") + config->EOFMessage);
				return;
			} else if (chan->isOperator(user) == false && chan->isHalfOperator(user) == false && split.size() != 2 && user->getMode('o') == false) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have @ nor %.") + config->EOFMessage);
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
					user->session()->sendAsServer("368 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "End of banned.") + config->EOFMessage);
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
								user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The BAN already exist.") + config->EOFMessage);
							} else {
								chan->setBan(maskara, user->nick());
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +b " + maskara + config->EOFMessage);
								user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The BAN has been set.") + config->EOFMessage);
								Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " +b " + maskara);
							}
						} else {
							if (chan->IsBan(maskara) == false) {
								user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The BAN does not exist.") + config->EOFMessage);
							} else {
								BanSet bans = chan->bans();
								BanSet::iterator it = bans.begin();
								for (; it != bans.end(); ++it)
									if ((*it)->mask() == maskara) {
										chan->UnBan((*it));
										chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -b " + maskara + config->EOFMessage);
										user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "The BAN has been deleted.") + config->EOFMessage);
										Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -b " + maskara);
									}
							}
						}
						j++;
					} else if (split[2][i] == 'o') {
						User*  target = Mainframe::instance()->getUserByName(split[3+j]);
						if (!target) return;
						if (chan->hasUser(target) == false) return;
						if (action == 1) {
							if (user->getMode('o') == true && chan->isOperator(target) == false) {
								if (chan->isVoice(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -v " + target->nick());
									chan->delVoice(target);
								} if (chan->isHalfOperator(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -h " + target->nick());
									chan->delHalfOperator(target);
								}
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +o " + target->nick() + config->EOFMessage);
								Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " +o " + target->nick());
								chan->giveOperator(target);
							} else if (chan->isOperator(target) == true) return;
							else if (chan->isOperator(user) == false) return;
							else {
								if (chan->isVoice(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -v " + target->nick());
									chan->delVoice(target);
								} if (chan->isHalfOperator(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -h " + target->nick());
									chan->delHalfOperator(target);
								}
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +o " + target->nick() + config->EOFMessage);
								Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " +o " + target->nick());
								chan->giveOperator(target);
							}
						} else {
							if (chan->isOperator(user) == true && chan->isOperator(target) == true) {
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
								Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -o " + target->nick());
								chan->delOperator(target);
							}
						}
					} else if (split[2][i] == 'h') {
						User*  target = Mainframe::instance()->getUserByName(split[3+j]);
						if (!target) return;
						if (chan->hasUser(target) == false) return;
						if (action == 1) {
							if (user->getMode('o') == true && chan->isHalfOperator(target) == false) {
								if (chan->isOperator(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -o " + target->nick());
									chan->delOperator(target);
								} if (chan->isVoice(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -v " + target->nick());
									chan->delVoice(target);
								}
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +h " + target->nick() + config->EOFMessage);
								Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " +h " + target->nick());
								chan->giveHalfOperator(target);
							} else if (chan->isHalfOperator(target) == true) return;
							else if (chan->isOperator(user) == false) return;
							else {
								if (chan->isOperator(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -o " + target->nick());
									chan->delOperator(target);
								} if (chan->isVoice(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -v " + target->nick());
									chan->delVoice(target);
								}
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +h " + target->nick() + config->EOFMessage);
								Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " +h " + target->nick());
								chan->giveHalfOperator(target);
							}
						} else {
							if (chan->isHalfOperator(target) == true || chan->isOperator(user) == true) {
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
								Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -h " + target->nick());
								chan->delHalfOperator(target);
							}
						}
					} else if (split[2][i] == 'v') {
						User*  target = Mainframe::instance()->getUserByName(split[3+j]);
						if (!target) return;
						if (chan->hasUser(target) == false) return;
						if (action == 1) {
							if (user->getMode('o') == true && chan->isVoice(target) == false) {
								if (chan->isOperator(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -o " + target->nick());
									chan->delOperator(target);
								} if (chan->isHalfOperator(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -h " + target->nick());
									chan->delHalfOperator(target);
								}
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +v " + target->nick() + config->EOFMessage);
								Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " +v " + target->nick());
								chan->giveVoice(target);
							} else if (chan->isVoice(target) == true) return;
							else if (chan->isOperator(user) == false && chan->isHalfOperator(user) == false) return;
							else {
								if (chan->isOperator(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -o " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -o " + target->nick());
									chan->delOperator(target);
								} if (chan->isHalfOperator(target) == true) {
									chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -h " + target->nick() + config->EOFMessage);
									Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -h " + target->nick());
									chan->delHalfOperator(target);
								}
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " +v " + target->nick() + config->EOFMessage);
								Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " +v " + target->nick());
								chan->giveVoice(target);
							}
						} else {
							if (chan->isVoice(target) == true && (chan->isOperator(user) == true || chan->isHalfOperator(user) == true)) {
								chan->broadcast(user->messageHeader() + "MODE " + chan->name() + " -v " + target->nick() + config->EOFMessage);
								Servidor::sendall("CMODE " + user->nick() + " " + chan->name() + " -v " + target->nick());
								chan->delVoice(target);
							}
						}
					}
				}
			}
		}
	}
	
	else if (split[0] == "WHOIS") {
		if (split.size() < 2) {
			user->session()->sendAsServer("431 " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
			return;
		} else if (split[1][0] == '#') {
			if (checkchan (split[1]) == false) {
				user->session()->sendAsServer("401 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The channel contains no-valid characters.") + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "End of /WHOIS.") + config->EOFMessage);
				return;
			}
			Channel* chan = Mainframe::instance()->getChannelByName(split[1]);
			std::string sql;
			std::string mascara = user->nick() + "!" + user->ident() + "@" + user->sha();
			if (ChanServ::IsAKICK(mascara, split[1]) == true) {
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "STATUS: \0036AKICK\003.") + config->EOFMessage);
			} else if (chan && chan->IsBan(mascara) == true)
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "STATUS: \0036BANNED\003.") + config->EOFMessage);
			if (NickServ::IsRegistered(user->nick()) == true && !NickServ::GetvHost(user->nick()).empty()) {
				mascara = user->nick() + "!" + user->ident() + "@" + NickServ::GetvHost(user->nick());
				if (ChanServ::IsAKICK(mascara, split[1]) == true) {
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "STATUS: \0036AKICK\003.") + config->EOFMessage);
				} else if (chan && chan->IsBan(mascara) == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "STATUS: \0036BANNED\003.") + config->EOFMessage);
			} if (!chan)
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "STATUS: \0034EMPTY\003.") + config->EOFMessage);
			else
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "STATUS: \0033ACTIVE\003.") + config->EOFMessage);
			if (ChanServ::IsRegistered(split[1]) == 1) {
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The channel is registered.") + config->EOFMessage);
				switch (ChanServ::Access(user->nick(), split[1])) {
					case 0:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "You do not have access.") + config->EOFMessage);
						break;
					case 1:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "Your access is VOP.") + config->EOFMessage);
						break;
					case 2:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "Your access is HOP.") + config->EOFMessage);
						break;
					case 3:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "Your access is AOP.") + config->EOFMessage);
						break;
					case 4:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "Your access is SOP.") + config->EOFMessage);
						break;
					case 5:
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "Your access is FOUNDER.") + config->EOFMessage);
						break;
					default:
						break;
				}
				std::string modes;
				if (ChanServ::HasMode(split[1], "FLOOD") > 0) {
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
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The channel has no modes.") + config->EOFMessage);
				else
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The channel has modes:%s", modes.c_str()) + config->EOFMessage);
				if (ChanServ::IsKEY(split[1]) == 1 && ChanServ::Access(user->nick(), split[1]) != 0) {
					sql = "SELECT KEY FROM CANALES WHERE NOMBRE='" + split[1] + "' COLLATE NOCASE;";
					std::string key = DB::SQLiteReturnString(sql);
					if (key.length() > 0)
						user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The channel key is: %s", key.c_str()) + config->EOFMessage);
				}
				std::string sql = "SELECT OWNER FROM CANALES WHERE NOMBRE='" + split[1] + "' COLLATE NOCASE;";
				std::string owner = DB::SQLiteReturnString(sql);
				if (owner.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The founder of the channel is: %s", owner.c_str()) + config->EOFMessage);
				sql = "SELECT REGISTERED FROM CANALES WHERE NOMBRE='" + split[1] + "' COLLATE NOCASE;";
				int registro = DB::SQLiteReturnInt(sql);
				if (registro > 0) {
					std::string tiempo = Utils::Time(registro);
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "Registered for: %s", tiempo.c_str()) + config->EOFMessage);
				}
				user->session()->sendAsServer("318 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "End of /WHOIS.") + config->EOFMessage);
			} else {
				user->session()->sendAsServer("401 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The channel is not registered.") + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "End of /WHOIS.") + config->EOFMessage);
			}
		} else {
			if (checknick (split[1]) == false) {
				user->session()->sendAsServer("401 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The nick contains no-valid characters.") + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "End of /WHOIS.") + config->EOFMessage);
				return;
			}
			User* target = Mainframe::instance()->getUserByName(split[1]);
			std::string sql;
			if (!target && NickServ::IsRegistered(split[1]) == true) {
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "STATUS: \0034OFFLINE\003.") + config->EOFMessage);
				user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The nick is registered.") + config->EOFMessage);
				sql = "SELECT SHOWMAIL FROM OPTIONS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
				if (DB::SQLiteReturnInt(sql) == 1 || user->getMode('o') == true) {
					sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
					std::string email = DB::SQLiteReturnString(sql);
					if (email.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The email is: %s", email.c_str()) + config->EOFMessage);
				}
				sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
				std::string url = DB::SQLiteReturnString(sql);
				if (url.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The website is: %s", url.c_str()) + config->EOFMessage);
				sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
				std::string vHost = DB::SQLiteReturnString(sql);
				if (vHost.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The vHost is: %s", vHost.c_str()) + config->EOFMessage);
				sql = "SELECT REGISTERED FROM NICKS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
				int registro = DB::SQLiteReturnInt(sql);
				if (registro > 0) {
					std::string tiempo = Utils::Time(registro);
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "Registered for: %s", tiempo.c_str()) + config->EOFMessage);
				}
				sql = "SELECT LASTUSED FROM NICKS WHERE NICKNAME='" + split[1] + "' COLLATE NOCASE;";
				int last = DB::SQLiteReturnInt(sql);
				std::string tiempo = Utils::Time(last);
				if (tiempo.length() > 0 && last > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "Last seen on : %s", tiempo.c_str()) + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "End of /WHOIS.") + config->EOFMessage);
				return;
			} else if (target && NickServ::IsRegistered(split[1]) == 1) {
				user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "%s is: %s!%s@%s", target->nick().c_str(), target->nick().c_str(), target->ident().c_str(), target->sha().c_str()) + config->EOFMessage);
				user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "STATUS: \0033CONNECTED\003.") + config->EOFMessage);
				user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "The nick is registered.") + config->EOFMessage);
				if (user->getMode('o') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "The IP is: %s", target->host().c_str()) + config->EOFMessage);
				if (target->getMode('o') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "Is an iRCop.") + config->EOFMessage);
				if (target->getMode('z') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "Connects trough a secure channel SSL.") + config->EOFMessage);
				if (target->getMode('w') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "Connects trough WebChat.") + config->EOFMessage);
				if (NickServ::GetOption("SHOWMAIL", target->nick()) == true || user->getMode('o') == true) {
					sql = "SELECT EMAIL FROM NICKS WHERE NICKNAME='" + target->nick() + "' COLLATE NOCASE;";
					std::string email = DB::SQLiteReturnString(sql);
					if (email.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "The email is: %s", email.c_str()) + config->EOFMessage);
				}
				sql = "SELECT URL FROM NICKS WHERE NICKNAME='" + target->nick() + "' COLLATE NOCASE;";
				std::string url = DB::SQLiteReturnString(sql);
				if (url.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "The website is: %s", url.c_str()) + config->EOFMessage);
				sql = "SELECT VHOST FROM NICKS WHERE NICKNAME='" + target->nick() + "' COLLATE NOCASE;";
				std::string vHost = DB::SQLiteReturnString(sql);
				if (vHost.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "The vHost is: %s", vHost.c_str()) + config->EOFMessage);
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
						opciones = Utils::make_string(user->nick(), "None");
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "Your options are: %s", opciones.c_str()) + config->EOFMessage);
				}
				sql = "SELECT REGISTERED FROM NICKS WHERE NICKNAME='" + target->nick() + "' COLLATE NOCASE;";
				int registro = DB::SQLiteReturnInt(sql);
				if (registro > 0) {
					std::string tiempo = Utils::Time(registro);
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "Registered for: %s", tiempo.c_str()) + config->EOFMessage);
				}
				std::string tiempo = Utils::Time(target->GetLogin());
				if (tiempo.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "Connected from: %s", tiempo.c_str()) + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "End of /WHOIS.") + config->EOFMessage);
				return;
			} else if (target && NickServ::IsRegistered(split[1]) == 0) {
				user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "%s is: %s!%s@%s", target->nick().c_str(), target->nick().c_str(), target->ident().c_str(), target->sha().c_str()) + config->EOFMessage);
				user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "STATUS: \0033CONNECTED\003.") + config->EOFMessage);
				if (user->getMode('o') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "The IP is: %s", target->host().c_str()) + config->EOFMessage);
				if (target->getMode('o') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "Is an iRCop.") + config->EOFMessage);
				if (target->getMode('z') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "Connects trough a secure channel SSL.") + config->EOFMessage);
				if (target->getMode('w') == true)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "Connects trough WebChat.") + config->EOFMessage);
				std::string tiempo = Utils::Time(target->GetLogin());
				if (tiempo.length() > 0)
					user->session()->sendAsServer("320 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "Connected from: %s", tiempo.c_str()) + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + target->nick() + " :" + Utils::make_string(user->nick(), "End of /WHOIS.") + config->EOFMessage);
				return;
			} else {
				user->session()->sendAsServer("401 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "The nick does not exist.") + config->EOFMessage);
				user->session()->sendAsServer("318 " + user->nick() + " " + split[1] + " :" + Utils::make_string(user->nick(), "End of /WHOIS.") + config->EOFMessage);
				return;
			}
		}
	}

	else if (split[0] == "CONNECT") {
		if (split.size() < 3) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (user->getMode('o') == false) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have iRCop privileges.") + config->EOFMessage);
			return;
		} else if (Servidor::IsAServer(split[1]) == false) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The server is not listed in config.") + config->EOFMessage);
			return;
		} else if (Servidor::IsConected(split[1]) == true) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The server is already connected.") + config->EOFMessage);
			return;
		} else {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "Connecting ...") + config->EOFMessage);
			Servidor::Connect(split[1], split[2]);
			return;
		}
	}

	else if (split[0] == "SERVERS") {
		if (user->getMode('o') == false) {
			user->session()->sendAsServer("461" + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have iRCop privileges.") + config->EOFMessage);
			return;
		} else {
			ServerSet::iterator it = Servers.begin();
			for (; it != Servers.end(); ++it) {
				user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "Name: %s IP: %s", (*it)->name().c_str(), (*it)->ip().c_str()) + config->EOFMessage);
				for (unsigned int i = 0; i < (*it)->connected.size(); i++)
					user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "Connected at: %s", (*it)->connected[i].c_str()) + config->EOFMessage);
			}
		}
	}

	else if (split[0] == "SQUIT") {
		if (split.size() < 2) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (user->getMode('o') == false) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have iRCop privileges.") + config->EOFMessage);
			return;
		} else if (Servidor::Exists(split[1]) == false) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The server is not connected.") + config->EOFMessage);
			return;
		} else if (boost::iequals(split[1], config->Getvalue("serverName"))) {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You can not make an SQUIT to your own server.") + config->EOFMessage);
			return;
		} else {
			Servidor::sendall("SQUIT " + split[1]);
			Servidor::SQUIT(split[1]);
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "The server has been disconnected.") + config->EOFMessage);
			return;
		}
	}
	
	else if (split[0] == "NICKSERV" || split[0] == "NS") {
		if (split.size() < 2) {
			user->session()->send(":" + config->Getvalue("nickserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
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
			user->session()->send(":" + config->Getvalue("chanserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
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
			user->session()->send(":" + config->Getvalue("hostserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
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
			user->session()->send(":" + config->Getvalue("serverName") + " 461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You do not have iRCop privileges.") + config->EOFMessage);
			return;
		} else if (split.size() < 2) {
			user->session()->send(":" + config->Getvalue("operserv") + " NOTICE " + user->nick() + " :" + Utils::make_string(user->nick(), "More data is needed.") + config->EOFMessage);
			return;
		} else if (user->nick() == "") {
			user->session()->sendAsServer("461 " + user->nick() + " :" + Utils::make_string(user->nick(), "You havent used the NICK command yet, you have limited access.") + config->EOFMessage);
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
			+ " " + user->nick() + " :" + Utils::make_string(user->nick(), "Unknown command.") + config->EOFMessage);
	}
}
