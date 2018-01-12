#ifndef EXTERN_H
#define EXTERN_H

extern time_t encendido;

extern Config *config;

extern List<Socket*> sock;
extern List<User*> users;
extern List<Servidor*> servidores;
extern List<Chan*> canales;
extern List<UserChan*> usuarios;
extern List<BanChan*> bans;
extern List<Memo*> memos;

void mayuscula (std::string &str);
bool checknick (const std::string nick);
bool checkchan (const std::string chan);

extern int shouldNotExit;
extern std::locale loc;

extern std::mutex usuarios_mtx;
extern std::mutex users_mtx;
extern std::mutex sock_mtx;
extern std::mutex bans_mtx;
extern std::mutex memos_mtx;

#define strlcpy(x, y, z) strncpy(x, y, z)

#endif
