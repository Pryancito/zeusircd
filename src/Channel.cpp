/* 
 * This file is part of the ZeusiRCd distribution (https://github.com/Pryancito/zeusircd).
 * Copyright (c) 2019 Rodrigo Santidrian AKA Pryan.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ZeusiRCd.h"
#include "services.h"
#include "Utils.h"

std::mutex channel_mtx;

void Channel::part(User* user) {
  broadcast(user->messageHeader() + "PART " + name);
    std::string lowercaseNickname = user->mNickName;
    std::transform(lowercaseNickname.begin(), lowercaseNickname.end(),
                   lowercaseNickname.begin(), ::tolower);

    auto it = std::find_if(Users.begin(), Users.end(),
                           [nickname = std::move(lowercaseNickname)](const auto& userPair) {
                               return userPair.first == nickname;
                           });

  if (it != Users.end()) {
    user_mtx.lock();
    auto usr = it->second;
    usr->channels.erase(this);
    user_mtx.unlock();
    RemoveUser(user);
    Utils::log(Utils::make_string("", "Nick %s leaves channel: %s", user->mNickName.c_str(), name.c_str()));
    // If the channel is now empty, delete it
    if (users.empty()) {
      channel_mtx.lock();
      Channels.erase(name);  // No need for case conversion, as Channels should use canonical names
      channel_mtx.unlock();
      delete this;
    }
  } else {
    // Handle the case where the user is not found in the Users container
    // (e.g., log a warning or throw an exception)
    Utils::log(Utils::make_string("", "Nick unknown leaves channel: %s", name.c_str()));
  }
}

void Channel::join(User* user) {
  // Perform case-insensitive search for the user using std::find_if
    std::string lowercaseNickname = user->mNickName;
    std::transform(lowercaseNickname.begin(), lowercaseNickname.end(),
                   lowercaseNickname.begin(), ::tolower);

    auto it = std::find_if(Users.begin(), Users.end(),
                           [nickname = std::move(lowercaseNickname)](const auto& userPair) {
                               return userPair.first == nickname;
                           });
  if (it != Users.end()) {
    auto usr = it->second;
    // Add the channel to the user's list
    usr->channels.insert(this);
    InsertUser(user);
    broadcast(user->messageHeader() + "JOIN " + name);
    send_userlist(user);
    Utils::log(Utils::make_string("", "Nick %s joins channel: %s", user->mNickName.c_str(), name.c_str()));
  } else {
    // Handle the case where the user is not found in the Users container
    // (e.g., log a warning or throw an exception)
    Utils::log(Utils::make_string("", "Nick unknown joins channel: %s", name.c_str()));
  }
}

void Channel::quit(User* user) {
  RemoveUser(user);

  // If the channel is now empty, delete it
  if (users.empty()) {
    channel_mtx.lock();
    Channels.erase(name);  // No need for case conversion, assuming canonical names
    delete this;
    channel_mtx.unlock();
  }
}

void Channel::broadcast(const std::string& message) {
std::lock_guard<std::mutex> lock(mtx);  // Lock for consistency
  for (auto* user : users) {
    if (user->quit == true) continue;
    else if (user->is_local) {  // Use direct member access for clarity
      try {
        user->deliver(message);  // Handle potential exceptions
      } catch (...) {
        Utils::log(Utils::make_string("", "Error delivering message to user %s: %s", user->mNickName.c_str(), message.c_str()));
      }
    }
  }
}

void Channel::broadcast_except_me(std::string& nickname, const std::string& message) {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for consistency
  for (auto* user : users) {
    if (user->quit == true) continue;
    else if (user->is_local && user->mNickName != nickname) {  // Optimized condition check
      try {
        user->deliver(message);  // Handle potential exceptions
      } catch (...) {
        Utils::log(Utils::make_string("", "Error delivering message to user %s: %s", user->mNickName.c_str(), message.c_str()));
      }
    }
  }
}

void Channel::broadcast_away(User* user, std::string away, bool on) {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for consistency
  for (auto* usr : users) {
    if (usr != user) {  // Avoid sending messages to the user itself
      std::string message;
      if (usr->away_notify) {
        message = user->messageHeader() + "AWAY " + (on ? away : "");
      } else {
        message = user->messageHeader() + "NOTICE " + name + " :AWAY " + (on ? "ON " + away : "OFF");
      }
      try {
        usr->deliver(message);  // Handle potential exceptions
      } catch (...) {
        Utils::log(Utils::make_string("", "Error delivering away message to user %s: %s", usr->mNickName.c_str(), message.c_str()));
      }
    }
  }
}

bool Channel::FindChannel(std::string &nombre) {
  // Convierte la cadena a minúsculas de antemano para comparación eficiente
  std::string lowercaseNombre = nombre;
  std::transform(lowercaseNombre.begin(), lowercaseNombre.end(),
                 lowercaseNombre.begin(), ::tolower);

  // Usa std::any_of para encontrar si existe un canal con nombre coincidente
  return std::any_of(Channels.begin(), Channels.end(),
                      [&lowercaseNombre](const auto& channel) {
                        return channel.first == lowercaseNombre;
                      });
}

Channel* Channel::GetChannel(const std::string &chan) {
  // Convierte la cadena a minúsculas de antemano
  std::string lowercaseChan = chan;
  std::transform(lowercaseChan.begin(), lowercaseChan.end(),
                 lowercaseChan.begin(), ::tolower);

  // Utiliza std::find_if para encontrar el canal
  auto it = std::find_if(Channels.begin(), Channels.end(),
                          [&lowercaseChan](const auto& channel) {
                            return channel.first == lowercaseChan;
                          });

  // Devuelve el puntero al canal si se encontró, o nullptr en caso contrario
  return it != Channels.end() ? it->second : nullptr;
}

bool Channel::HasUser(User* user) {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for consistency

  // Using a set for efficient membership testing
  return users.find(user) != users.end();
}

void Channel::InsertUser(User* user) {
  std::lock_guard lock(mtx); // Bloqueo RAII para simplificar la gestión del mutex
  if (!users.insert(user).second) {
    // Handle the case where the user is already in the channel (optional)
    Utils::log(Utils::make_string("", "User %s already in channel: %s", user->mNickName.c_str(), name.c_str()));
  }
}

void Channel::RemoveUser(User* user) {
  std::lock_guard lock(mtx);
  if (HasUser(user)) {
    mtx.lock();
    users.erase(user);  // Rely on the container's erase for efficiency
    mtx.unlock();
  } else {
     // Handle the case where the user is not found (optional)
     Utils::log(Utils::make_string("", "User %s not found in channel: %s", user->mNickName.c_str(), name.c_str()));
  }

  // Remove from other roles as well
  RemoveFromRole(user, operators);
  RemoveFromRole(user, halfoperators);
  RemoveFromRole(user, voices);
}

void Channel::RemoveFromRole(User* user, std::set<User*>& roleSet) {
  auto it = roleSet.find(user);
  if (it != roleSet.end()) {
    roleSet.erase(it);
  }
}

void Channel::GiveOperator(User* user) {
  if (!IsOperator(user)) {  // Rely on IsOperator for membership check
    mtx.lock();
    operators.insert(user);
    mtx.unlock();
  } else {
    // Handle the case where the user is already an operator (optional)
    Utils::log(Utils::make_string("", "User %s already an operator in channel: %s", user->mNickName.c_str(), name.c_str()));
  }
}

bool Channel::IsOperator(User* user) {
  std::lock_guard<std::mutex> lock(mtx);
  return operators.find(user) != operators.end();  // Efficient lookup
}

void Channel::RemoveOperator(User* user) {
  std::lock_guard<std::mutex> lock(mtx);
  operators.erase(user);  // Directly erase for efficiency
}

void Channel::GiveHalfOperator(User* user) {
  if (!IsOperator(user)) {  // Rely on IsOperator for membership check
    mtx.lock();
    halfoperators.insert(user);
    mtx.unlock();
  } else {
    // Handle the case where the user is already an operator (optional)
    Utils::log(Utils::make_string("", "User %s already an halfoperator in channel: %s", user->mNickName.c_str(), name.c_str()));
  }
}

bool Channel::IsHalfOperator(User* user) {
  std::lock_guard<std::mutex> lock(mtx);
  return halfoperators.find(user) != halfoperators.end();  // Efficient lookup
}

void Channel::RemoveHalfOperator(User* user) {
  std::lock_guard<std::mutex> lock(mtx);
  halfoperators.erase(user);  // Directly erase for efficiency
}

void Channel::GiveVoice(User* user) {
  if (!IsOperator(user)) {  // Rely on IsOperator for membership check
    mtx.lock();
    voices.insert(user);
    mtx.unlock();
  } else {
    // Handle the case where the user is already an operator (optional)
    Utils::log(Utils::make_string("", "User %s already an voice in channel: %s", user->mNickName.c_str(), name.c_str()));
  }
}

bool Channel::IsVoice(User* user) {
  std::lock_guard<std::mutex> lock(mtx);
  return voices.find(user) != voices.end();  // Efficient lookup
}

void Channel::RemoveVoice(User* user) {
  std::lock_guard<std::mutex> lock(mtx);
  voices.erase(user);  // Directly erase for efficiency
}

bool Channel::getMode(char mode) {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for thread safety

  switch (mode) {
    case 'r': return mode_r;
    default: return false;
  }
}

void Channel::setMode(char mode, bool option) {
  std::lock_guard<std::mutex> lock(mtx);

  switch (mode) {
    case 'r': mode_r = option; break;
    default:
      Utils::log(Utils::make_string("", "Invalid mode: %c", mode));  // Log invalid mode
      break;
  }
}

void Ban::ExpireBan(const boost::system::error_code& e) {
  if (!e) {
    Channel* chan = Channel::GetChannel(canal);
    if (chan) {
      try {
        chan->broadcast(":" + config["chanserv"].as<std::string>() + " MODE " + chan->name + " -b " + this->mask());
        Server::Send("CMODE " + config["chanserv"].as<std::string>() + " " + chan->name + " -b " + this->mask());
        chan->UnBan(this);
      } catch (...) {
        // Handle potential exceptions during ban removal (optional)
        Utils::log(Utils::make_string("", "Error removing expired ban %s from channel %s", this->mask().c_str(), chan->name.c_str()));
      }
    }
  } else {
    // Handle error gracefully
    Utils::log(Utils::make_string("", "Error processing ban expiration for %s: %s", this->mask().c_str(), e.message().c_str()));
  }
}

void Channel::UnBan(Ban* ban) {

  auto it = bans.find(ban);
  if (it != bans.end()) {
    mtx.lock();
    bans.erase(it);
    delete ban;  // Ensure deletion within the lock's scope
    mtx.unlock();
  } else {
    // Handle the case where the ban is not found (optional)
    Utils::log(Utils::make_string("", "Ban %s not found in channel %s", ban->mask().c_str(), name.c_str()));
  }
}

void Channel::UnpBan(pBan* ban) {
  std::lock_guard<std::mutex> lock(channel_mtx);  // Lock for consistency and thread safety

  auto it = pbans.find(ban);
  if (it != pbans.end()) {
    mtx.lock();
    pbans.erase(it);
    delete ban;  // Ensure deletion within the lock's scope
    mtx.unlock();
  } else {
    // Handle the case where the pBan is not found (optional)
    Utils::log(Utils::make_string("", "pBan %s not found in channel %s", ban->mask().c_str(), name.c_str()));
  }
}

bool Channel::IsBan(const std::string& mask) {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for consistency and thread safety

  if (bans.size() == 0 && pbans.size() == 0)
    return false;

  std::string lowercaseMask = mask;  // Store lowercase version for efficiency
  std::transform(lowercaseMask.begin(), lowercaseMask.end(), lowercaseMask.begin(), ::tolower);

  // Check bans and pbans using efficient string comparison
  for (Ban* ban : bans) {
    if (Utils::Match(ban->mask().c_str(), lowercaseMask.c_str())) return true;
  }
  for (pBan* pban : pbans) {
    if (Utils::Match(pban->mask().c_str(), lowercaseMask.c_str())) return true;
  }

  return false;
}

void Channel::resetflood() {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for consistency and thread safety

  flood = 0;
  is_flood = false;

  // Broadcast notice within the lock's scope
  broadcast(":" + config["chanserv"].as<std::string>() + " NOTICE " + name + " :" +
            Utils::make_string("", "The channel has left flood mode."));

  // Send notice to server, potentially outside the lock (optional)
  Server::Send("NOTICE " + config["chanserv"].as<std::string>() + " " + name + " :" +
               Utils::make_string("", "The channel has left flood mode."));
}

void Channel::increaseflood() {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for thread safety

  // Check for flood mode and increase counter
  if (ChanServ::IsRegistered(name) && ChanServ::HasMode(name, "FLOOD")) {
    flood++;

    // Enter flood mode if threshold is reached
    if (flood >= ChanServ::HasMode(name, "FLOOD") && !is_flood) {
      is_flood = true;
      broadcast(":" + config["chanserv"].as<std::string>() + " NOTICE " + name + " :" +
                Utils::make_string("", "The channel has entered flood mode. Actions are restricted."));
      Server::Send("NOTICE " + config["chanserv"].as<std::string>() + " " + name + " :" +
                   Utils::make_string("", "The channel has entered flood mode. Actions are restricted."));
    }
  }
}

bool Channel::isonflood() {
	return is_flood;
}

void Channel::ExpireFlood() {
	if (is_flood == true)
		resetflood();
	else
		flood = 0;
}

void Channel::send_userlist(User* user) {

  std::stringstream names_stream;  // Use stringstream for efficient string building

  for (auto* usr : users) {
    std::string nickname = usr->mNickName;
    if (user->userhost_in_names) {
      nickname += "!" + usr->mIdent + "@" + usr->mvHost;
    }
    names_stream << (names_stream.tellp() > 0 ? " " : "") << get_prefix(usr) + nickname;

    if (names_stream.tellp() > 450) {  // Leave space for trailing characters
      user->SendAsServer("353 " + user->mNickName + " = " + name + " :" + names_stream.str());
      names_stream.str(std::string());  // Clear the stream for the next batch
    }
  }

  if (names_stream.tellp() > 0) {
    user->SendAsServer("353 " + user->mNickName + " = " + name + " :" + names_stream.str());
  }

  user->SendAsServer("366 " + user->mNickName + " " + name + " :" + Utils::make_string(user->mLang, "End of /NAMES list."));
}

std::string Channel::get_prefix(User* usr) {
  if (IsOperator(usr)) return "@";
  if (IsHalfOperator(usr)) return "%";
  if (IsVoice(usr)) return "+";
  return std::string();
}

bool Channel::canBeBanned(const std::string& mask) {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for consistency and thread safety

  std::string lowercaseMask = mask;  // Store lowercase version for efficiency
  std::transform(lowercaseMask.begin(), lowercaseMask.end(), lowercaseMask.begin(), ::tolower);

  // Check access and match for each user
  for (const User* usr : users) {
    // Create lowercase user masks for comparison
    std::string lowercaseMuser1 = usr->mNickName + "!" + usr->mIdent + "@" + usr->mvHost;
    std::string lowercaseMuser2 = usr->mNickName + "!" + usr->mIdent + "@" + usr->mCloak;
    std::transform(lowercaseMuser1.begin(), lowercaseMuser1.end(), lowercaseMuser1.begin(), ::tolower);
    std::transform(lowercaseMuser2.begin(), lowercaseMuser2.end(), lowercaseMuser2.begin(), ::tolower);

    if (ChanServ::Access(usr->mNickName, name) > 2 &&
        (Utils::Match(lowercaseMuser1.c_str(), lowercaseMask.c_str()) || Utils::Match(lowercaseMuser2.c_str(), lowercaseMask.c_str()))) {
      return false;  // User cannot be banned
    }
  }

  return true;  // No user matching the mask with high enough access
}

void Channel::setBan(const std::string& mask, const std::string& whois) {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for consistency and thread safety
  // Ensure mask is valid and ban doesn't already exist (optional)
  if (!Utils::isValidMask(mask)) {
    return;  // Handle invalid mask or duplicate ban (optional)
  }

  // Create ban with lowercase mask
  Ban* ban = new Ban(name, Utils::toLowercase(mask), whois, time(0));

  // Insert ban into container, handling potential exceptions
  try {
    bans.insert(ban);
  } catch (...) {
    delete ban;  // Clean up memory if insertion fails
    throw;  // Rethrow the exception
  }
}

void Channel::setpBan(const std::string& mask, const std::string& whois) {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for consistency and thread safety

  // Ensure mask is valid and pBan doesn't already exist (optional)
  if (!Utils::isValidMask(mask)) {
    return;  // Handle invalid mask or duplicate pBan (optional)
  }

  // Create pBan with lowercase mask
  pBan* ban = new pBan(name, Utils::toLowercase(mask), whois, time(0));

  // Insert pBan into container, handling potential exceptions
  try {
    pbans.insert(ban);
  } catch (...) {
    delete ban;  // Clean up memory if insertion fails
    throw;  // Rethrow the exception
  }
}

void Channel::SBAN(const std::string& mask, const std::string& whois, const std::string& time_str) {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for thread safety

  // Parse time string and handle errors
  time_t tiempo;
  try {
    tiempo = std::stoi(time_str);
  } catch (const std::exception& e) {
    // Handle invalid time string (e.g., log a warning)
    return;
  }

  // Create ban with lowercase mask
  Ban* ban = new Ban(name, Utils::toLowercase(mask), whois, tiempo);

  // Insert ban into container, handling potential exceptions
  try {
    bans.insert(ban);
  } catch (...) {
    delete ban;  // Clean up memory if insertion fails
    throw;  // Rethrow the exception
  }
}

void Channel::SPBAN(const std::string& mask, const std::string& whois, const std::string& time_str) {
  std::lock_guard<std::mutex> lock(mtx);  // Lock for thread safety

  // Parse time string and handle errors (same as in SBAN)
  time_t tiempo;
  try {
    tiempo = std::stoi(time_str);
  } catch (const std::exception& e) {
    // Handle invalid time string
    return;
  }

  // Create pBan with lowercase mask
  pBan* ban = new pBan(name, Utils::toLowercase(mask), whois, tiempo);

  // Insert pBan into container, handling potential exceptions (same as in SBAN)
  try {
    pbans.insert(ban);
  } catch (...) {
    delete ban;
    throw;
  }
}

void Channel::send_who_list(User* user) {
	for (auto *usr : users) {
		std::string oper = "";
		std::string away = "H";
		if (usr->getMode('o') == true)
			oper = "*";
		if (usr->bAway == true)
			away = "G";
		if(IsOperator(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ name + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "@ :0 " 
				+ "ZeusiRCd");
		} else if(IsHalfOperator(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ name + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "% :0 " 
				+ "ZeusiRCd");
		} else if(IsVoice(usr) == true) {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ name + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + "+ :0 " 
				+ "ZeusiRCd");
		} else {
			user->SendAsServer("352 "
				+ usr->mNickName + " " 
				+ name + " " 
				+ usr->mNickName + " " 
				+ usr->mvHost + " " 
				+ "*.* " 
				+ usr->mNickName + " " + away + oper + " :0 " 
				+ "ZeusiRCd");
		}
	}
	user->SendAsServer("315 " 
		+ user->mNickName + " " 
		+ name + " :" + "End of /WHO list.");
}
