#include "ZeusBaseClass.h"
#include "module.h"
#include "Utils.h"
#include "mainframe.h"
#include "Channel.h"
#include "services.h"

std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");

extern Memos MemoMsg;

class CMD_Privmsg : public Module
{
	public:
	CMD_Privmsg() : Module("PRIVMSG", 50, false) {};
	~CMD_Privmsg() {};
	virtual void command(LocalUser *user, std::string message) override {
		std::vector<std::string> results;
		Utils::split(message, results, " ");
		if (!user->bSentNick) {
			user->SendAsServer("461 ZeusiRCd :" + Utils::make_string(user->mLang, "You havent used the NICK command yet, you have limited access."));
			return;
		}
		else if (results.size() < 3) return;
		std::string mensaje = "";
		for (unsigned int i = 2; i < results.size(); ++i) { mensaje.append(results[i] + " "); }
		trim(mensaje);
		if (results[1][0] == '#') {
			Channel* chan = Mainframe::instance()->getChannelByName(results[1]);
			if (chan) {
				if (ChanServ::HasMode(chan->name(), "MODERATED") && !chan->isOperator(user) && !chan->isHalfOperator(user) && !chan->isVoice(user) && user->getMode('o') == false) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel is moderated, you cannot speak."));
					return;
				} else if (chan->isonflood() == true && ChanServ::Access(user->mNickName, chan->name()) == 0) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The channel is on flood, you cannot speak."));
					return;
				} else if (chan->hasUser(user) == false) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are not into the channel."));
					return;
				} else if (chan->IsBan(user->mNickName + "!" + user->mIdent + "@" + user->mCloak) == true) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are banned, cannot speak."));
					return;
				} else if (chan->IsBan(user->mNickName + "!" + user->mIdent + "@" + user->mvHost) == true) {
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "You are banned, cannot speak."));
					return;
				} else if (OperServ::IsSpam(mensaje, "C") == true && OperServ::IsSpam(mensaje, "E") == false && user->getMode('o') == false && strcasecmp(chan->name().c_str(), "#spam") != 0) {
					Oper oper;
					oper.GlobOPs(Utils::make_string("", "Nickname %s try to make SPAM into channel: %s", user->mNickName.c_str(), chan->name().c_str()));
					user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The message of channel %s contains SPAM.", chan->name().c_str()));
					return;
				}
				chan->increaseflood();
				chan->broadcast_except_me(user->mNickName,
					user->messageHeader()
					+ "PRIVMSG "
					+ chan->name() + " "
					+ mensaje);
				Server::Send(cmd + " " + user->mNickName + "!" + user->mIdent + "@" + user->mvHost + " " + chan->name() + " " + mensaje);
			}
		}
		else {
			User* target = Mainframe::instance()->getUserByName(results[1]);
			if (OperServ::IsSpam(mensaje, "P") == true && OperServ::IsSpam(mensaje, "E") == false && user->getMode('o') == false && target) {
				Oper oper;
				oper.GlobOPs(Utils::make_string("", "Nickname %s try to make SPAM to nick: %s", user->mNickName.c_str(), target->mNickName.c_str()));
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Message to nick %s contains SPAM.", target->mNickName.c_str()));
				return;
			} else if (NickServ::GetOption("NOCOLOR", results[1]) == true && target && mensaje.find("\003") != std::string::npos) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "Message to nick %s contains colours.", target->mNickName.c_str()));
				return;
			} else if (target && NickServ::GetOption("ONLYREG", results[1]) == true && user->getMode('r') == false) {
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick %s only can receive messages from registered nicks.", target->mNickName.c_str()));
				return;
			}
			LocalUser* tuser = Mainframe::instance()->getLocalUserByName(results[1]);
			if (tuser) {
				if (tuser->bAway == true) {
					user->Send(tuser->messageHeader()
						+ "NOTICE "
						+ user->mNickName + " :AWAY " + tuser->mAway);
				}
				tuser->Send(user->messageHeader()
					+ "PRIVMSG "
					+ tuser->mNickName + " "
					+ mensaje);
				return;
			} else {
				RemoteUser *u = Mainframe::instance()->getRemoteUserByName(results[1]);
				if (u) {
					if (u->bAway == true) {
						user->Send(u->messageHeader()
							+ "NOTICE "
							+ user->mNickName + " :AWAY " + u->mAway);
					}
					Server::Send(cmd + " " + user->mNickName + "!" + user->mIdent + "@" + user->mvHost + " " + u->mNickName + " " + mensaje);
					return;
				}
			} if (!target && NickServ::IsRegistered(results[1]) == true && NickServ::MemoNumber(results[1]) < 50 && NickServ::GetOption("NOMEMO", results[1]) == 0) {
				Memo *memo = new Memo();
					memo->sender = user->mNickName;
					memo->receptor = results[1];
					memo->time = time(0);
					memo->mensaje = mensaje;
				MemoMsg.insert(memo);
				user->Send(":NiCK!*@* NOTICE " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick is offline, MeMo has been sent."));
				Server::Send("MEMO " + memo->sender + " " + memo->receptor + " " + std::to_string(memo->time) + " " + memo->mensaje);
				return;
			} else
				user->SendAsServer("461 " + user->mNickName + " :" + Utils::make_string(user->mLang, "The nick doesnt exists or cannot receive messages."));
		}
	}
};

extern "C" Widget* factory(void) {
	return static_cast<Widget*>(new CMD_Privmsg);
}
