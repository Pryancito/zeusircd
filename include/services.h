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
#pragma once

#include <string>
#include <set>

#include "ZeusBaseClass.h"
#include "Channel.h"

class NickServ
{
	public:
		static void Message(LocalUser *user, std::string mensaje);
		static bool IsRegistered(std::string nickname);
		static bool Login (const std::string &nickname, const std::string &pass);
		static int GetNicks();
		static bool GetOption(const std::string &option, std::string nickname);
		static void UpdateLogin (LocalUser *user);
		static std::string GetvHost (std::string nickname);
		static int MemoNumber(const std::string& nick);
		static void checkmemos(LocalUser *user);
		static std::string GetLang (std::string nickname);
};

class ChanServ
{
	public:
		static void Message(LocalUser *user, std::string mensaje);
		static bool IsRegistered(std::string channel);
		static bool IsFounder(std::string nickname, std::string channel);
		static int Access (std::string nickname, std::string channel);
		static void CheckModes(LocalUser *user, const std::string &channel);
		static bool IsAKICK(std::string mascara, const std::string &canal);
		static bool CheckKEY(const std::string &canal, std::string key);
		static bool IsKEY(const std::string &canal);
		static int GetChans();
		static int HasMode(const std::string &canal, std::string mode);
		static bool CanRegister(LocalUser *user, std::string channel);
		static void DoRegister(LocalUser *user, Channel *chan);
		static bool CanGeoIP(LocalUser *user, std::string channel);
};

class HostServ
{
	public:
		static void Message(LocalUser *user, std::string message);
		static bool CheckPath(std::string path);
		static bool Owns(LocalUser *user, std::string path);
		static bool DeletePath(std::string &path);
		static bool PathIsInvalid (std::string path);
		static bool GotRequest (std::string user);
		static int HowManyPaths(const std::string &nickname);
		static bool IsReqRegistered(std::string path);
};

class OperServ
{
	public:
		static void Message(LocalUser *user, std::string message);
		static bool IsGlined(std::string ip);
		static std::string ReasonGlined(const std::string &ip);
		static bool IsTGlined(std::string ip);
		static void ExpireTGline ();
		static std::string ReasonTGlined(const std::string &ip);
		static bool IsSpammed(std::string mask);
		static bool IsOper(std::string nick);
		static bool IsSpam(std::string text);
		static void ApplyGlines ();
		static int IsException(std::string ip, std::string option);
		static bool CanGeoIP(std::string ip);
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
